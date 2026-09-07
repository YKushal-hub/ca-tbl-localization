#ifndef CA_TBL_MAC_H
#define CA_TBL_MAC_H

/**
 * \file ca-tbl-mac.h
 * \brief CA-TBL Phase S4 -- MAC/link layer: CAD-CSMA/CA, duty cycle, APC, flood, ACK.
 *
 * What this changes about the claims
 * ---------------------------------
 * Before S4 the simulation had no frames and no channel contention. Interference
 * was a probability drawn inside the physics submodule per link sample. That is a
 * legitimate model of *external* radios, but it means nothing about our own
 * network: two of our nodes could never actually collide, because they never
 * actually transmitted at a time.
 *
 * Here they do. CaTblChannel tracks in-flight transmissions and, at each
 * receiver, arbitrates among the ones that overlap in time on the same tier using
 * a capture-threshold rule. A collision is therefore an emergent consequence of
 * who transmitted when, which is a consequence of CSMA/CA backoff, which is a
 * consequence of the priority contention window the message's access category
 * selected. That chain is what makes the §2.3.1 contention-window design
 * measurable rather than merely described -- and it is the substrate for claim 4.
 *
 * The external-interferer model inside the physics engine is left alone and still
 * runs. The two are complementary: the engine models radios we do not control,
 * this layer models the ones we do.
 *
 * Specification source
 * --------------------
 * finalyearprojectcontext.md §2.1 (WPC G.S.R. 853(E) Table-II duty cycle and
 * mandatory APC), §2.1.1 (APC loop), §2.3 (CAD-CSMA/CA), §2.3.1 (priority CW),
 * §2.6 (flood dedup + jitter), §2.8 (selective ACK), §2.9 (mobility-state
 * message selection).
 *
 * What is NOT here, deliberately
 * ------------------------------
 * Routing (§3.2, §3.3) and the identity-scoped query state machine (§3.4) are S5
 * and S6. This layer forwards floods by hop_limit only -- there is no route table
 * and no confidence-weighted path selection, so hop-provenance confidence
 * remains unclaimable. See CA_TBL_SIM_AUDIT.md.
 */

#include "ca-tbl-energy-model.h"
#include "ca-tbl-header.h"

#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"

#include <cstdint>
#include <deque>
#include <ostream>
#include <map>
#include <string>
#include <vector>

namespace ns3
{

/** Outcome of one physical link evaluation, as the MAC needs to see it. */
struct CaTblLinkOutcome
{
    bool received;      //!< the physics engine decoded it (before our own contention)
    double rxPowerDbm;  //!< received power -- used for capture arbitration and APC
    double snrDb;
    bool nlos;
    double estimatedRangeM;

    CaTblLinkOutcome();
};

/**
 * \brief One entry of the ONE-HOP neighbour table.
 *
 * Why this exists
 * ---------------
 * Before this, the scenario polled every anchor of a tier every cycle, including
 * anchors kilometres away that could not possibly answer, and the energy model
 * correctly charged the full RX timeout for each non-responder. The measured
 * consequence was that total energy tracked ANCHOR COUNT rather than radio cost:
 * 9 / 12 / 20 anchors gave 12.19 / 16.44 / 28.35 J, a 2.2x spread in count against
 * a spread in per-anchor exchange cost of only 1.13x on the two tiers where the
 * cost is per-anchor at all. Every accuracy-per-joule number in the project sat on
 * that denominator.
 *
 * A real node does not blind-poll a deployment map it was never given. It learns
 * who is audible by hearing them, and ranges against those. That is what this
 * table is, and gating the poll on it is what makes the energy axis measure the
 * radio rather than the deployment.
 *
 * ONE HOP is the load-bearing word. DISCOVERY is a flooded message type (§2.6), so
 * a beacon relayed by a third party arrives carrying the ORIGINATOR's origin_id
 * with a decremented hop_limit. Recording that originator would readmit exactly
 * the unreachable anchors this table exists to exclude, and the resulting energy
 * number would look fixed while being wrong in the same direction as before. See
 * CaTblMac::NoteNeighbour().
 */
struct CaTblNeighbour
{
    uint16_t id;
    CaTblTier tier;
    double lastHeardS;      //!< simulation time of the most recent direct frame
    double lastRxPowerDbm;
    double lastSnrDb;
    uint64_t heardCount;

    CaTblNeighbour();
};

/**
 * \brief What the MAC needs from the physics layer.
 *
 * An interface rather than a direct dependency on the three engines, so the MAC
 * can be unit-tested against a deterministic stub. The scenario implements this
 * with LoRaPhysicsEngine / LoRa2p4PhysicsEngine / LoRaUWBPhysicsEngine.
 */
class CaTblPhyProvider
{
  public:
    virtual ~CaTblPhyProvider() {}

    /** Evaluate the tx->rx link on \p tier at the current simulation time. */
    virtual CaTblLinkOutcome Evaluate(uint16_t txId,
                                      uint16_t rxId,
                                      CaTblTier tier,
                                      double txPowerDbm,
                                      uint8_t hopsTraversed) = 0;

    /** Sensitivity floor for the APC loop (§2.1.1). */
    virtual double SensitivityDbm(CaTblTier tier, uint8_t sf) = 0;
};

/** Mobility state driving message selection (§2.9). */
enum CaTblMobilityState : uint8_t
{
    CA_TBL_MOB_STATIONARY = 0,
    CA_TBL_MOB_APPROACHING = 1,
    CA_TBL_MOB_RECEDING = 2
};

const char* CaTblMobilityStateName(CaTblMobilityState s);

/**
 * \brief MAC configuration.
 *
 * Every default below that is not traceable to a citation is marked. The
 * distinction matters: a reviewer will ask which numbers are regulatory, which
 * are datasheet, and which are ours.
 */
struct CaTblMacConfig
{
    uint16_t nodeId;
    CaTblIdentityClass identityClass;
    uint8_t targetManifestId;

    /**
     * WPC G.S.R. 853(E) Table-II: <=10% duty for network access points
     * (gateways), <=2.5% otherwise (field nodes). REGULATORY, not a tunable.
     * Applies to 865-868 MHz only; 2.4 GHz ISM and UWB have no comparable cap.
     */
    bool isAccessPoint;

    // ---- CAD-CSMA/CA (§2.3) --------------------------------------------------
    double slotTimeS;        //!< backoff slot. OURS -- not calibrated.
    double cadDurationS;     //!< CAD listen window. Radio-dependent; SX1262-class default.
    uint32_t maxCadRetries;  //!< OURS. On exhaustion the frame is dropped, not queued forever.

    // ---- Flood control (§2.6) -----------------------------------------------
    double rebroadcastJitterMaxS; //!< decorrelates simultaneous rebroadcasts
    uint32_t dedupCacheEntries;   //!< bounded LRU; reported design parameter
    double dedupWindowS;          //!< time-windowed expiry
    uint8_t defaultHopLimit;

    // ---- APC (§2.1.1) — MANDATED by Table-II, not optional ------------------
    double txPowerDbm;         //!< current, mutated by the APC loop
    double txPowerMinDbm;
    double txPowerMaxDbm;      //!< 500 mW e.r.p. = 26.99 dBm at 868; ISM limits at 2.4
    double apcStepDb;
    double apcSafetyMarginDb;
    double apcUpperBoundDb;    //!< margin above this => reduce power
    double apcLowerBoundDb;    //!< margin below this => raise power
    uint32_t apcEvalEveryNReports;

    // ---- Selective ACK (§2.8) ----------------------------------------------
    double ackTimeoutS;
    uint32_t maxRetransmissions;
    uint32_t triggeredUpdateRepeats; //!< §2.8: "2-3x jittered rebroadcast"

    // ---- Neighbour table ----------------------------------------------------
    /**
     * How long a directly-heard peer stays in the one-hop table.
     *
     * OURS, and NOT calibrated. It must exceed the beacon period or the table
     * empties between beacons and the node stops ranging; it must be short enough
     * that a mobile node drops an anchor it has walked away from before it spends
     * many cycles paying RX timeouts for it. Those two constraints bracket it
     * against the scenario's beacon period and walk speed, they do not determine
     * it. Sweep it: it trades discovery energy against stale-entry energy, and the
     * position of that optimum is a result, not a setting.
     */
    double neighbourTtlS;

    // ---- Duty cycle accounting ---------------------------------------------
    double dutyWindowS; //!< sliding window over which duty is enforced

    uint8_t sf;
    double bandwidthHz;
    uint8_t codingRate;

    // ---- C1: the handshake as frames rather than as an accounting call ----
    /**
     * Whether ESC_REQUEST/ESC_GRANT and the DIGEST->ENTRY_PUSH reply are
     * actually transmitted.
     *
     * FALSE (default) keeps the pre-existing behaviour exactly: the
     * escalation handshake is charged through
     * CaTblEnergyModel::AccountEscalationHandshake() as airtime and energy
     * and no frame exists. Every duty-cycle result in the tree was measured
     * that way, so the default has to reproduce it byte-for-byte.
     *
     * TRUE makes the exchange real: the frames contend for the medium, pass
     * through CAD, and meet the duty governor AS FRAMES. Expect every
     * escalating arm to move -- that is the treatment, not a regression.
     */
    bool sendHandshakeFrames;

    // ---- C4: multi-target UWB arbitration --------------------------------
    //! Whether an anchor arbitrates between competing UWB requests instead of
    //! granting whoever asked. False (default) keeps the C1 behaviour, in which
    //! every ESC_REQUEST is granted and contention alone decides who lands.
    bool uwbArbitration = false;
    //! U(r) weights, in the order the Python harness defines them:
    //! degradation, battery, staleness, query urgency. All 1.0 is the harness
    //! default and is OURS pending CAL-09, which this mechanism produces.
    double arbLambdaDeg = 1.0;
    double arbLambdaBatt = 1.0;
    double arbLambdaStale = 1.0;
    double arbLambdaQuery = 1.0;
    //! Seconds after which a held UWB slot is considered free again.
    double arbSlotHoldS = 10.0;

    CaTblMacConfig();
};

/** Per-message-type counters. Kept per type rather than aggregated because the
 *  whole point of the taxonomy is that the types behave differently -- an
 *  aggregate would hide exactly the effect being claimed. */
struct CaTblMsgCounters
{
    uint64_t sent;
    uint64_t rebroadcast;
    uint64_t received;
    uint64_t droppedDedup;
    uint64_t droppedHopLimit;
    uint64_t droppedDutyCycle;
    uint64_t droppedCadExhausted;
    uint64_t lostCollision;
    uint64_t wonCapture;
    uint64_t acksSent;
    uint64_t acksReceived;
    uint64_t retransmissions;
    uint64_t ackTimeouts;
    double airtimeS;
    double totalBackoffS;
    uint64_t cadBusyEvents;

    CaTblMsgCounters();
};

class CaTblChannel;

/**
 * \brief One node's MAC.
 */
class CaTblMac
{
  public:
    CaTblMac();
    ~CaTblMac();

    void Configure(const CaTblMacConfig& cfg);
    const CaTblMacConfig& GetConfig() const { return m_cfg; }
    uint16_t GetNodeId() const { return m_cfg.nodeId; }

    void SetChannel(CaTblChannel* ch) { m_channel = ch; }
    void SetEnergyModel(CaTblEnergyModel* e) { m_energy = e; }
    void SetRng(Ptr<UniformRandomVariable> rng) { m_rng = rng; }

    void SetMobilityState(CaTblMobilityState s) { m_mobility = s; }
    CaTblMobilityState GetMobilityState() const { return m_mobility; }

    /** Called on every delivered frame. Signature: (rxNodeId, header, outcome). */
    typedef Callback<void, uint16_t, const CaTblHeader&, const CaTblLinkOutcome&> RxCallback;
    void SetRxCallback(RxCallback cb) { m_rxCb = cb; }

    /** Called when a frame is enqueued but cannot be sent. Signature:
     *  (nodeId, msgType, reason). Reason is one of "duty", "cad", "retx". */
    typedef Callback<void, uint16_t, CaTblMsgType, std::string> DropCallback;
    void SetDropCallback(DropCallback cb) { m_dropCb = cb; }

    /**
     * \brief Queue a frame for transmission. Runs CAD, backs off, transmits.
     *
     * \param type     message type -- determines access category, ACK policy and
     *                 whether it floods
     * \param tier     radio tier
     * \param nEntries payload sizing (table entries / digest entries / manifest ids)
     * \param destId   0 for broadcast, otherwise the directed peer
     */
    void Send(CaTblMsgType type, CaTblTier tier, uint32_t nEntries, uint16_t destId = 0);

    //! C3. A separate entry point rather than a defaulted parameter on Send(): ns-3
    //! Simulator::Schedule binds a member function at EXACT arity, so widening Send
    //! breaks every scheduled call to it. Found by the compiler, not by inspection.
    void SendQuery(CaTblTier tier, uint8_t manifestId);

    //! C4. The two U(r) terms an anchor cannot observe, quantised to 0..255 over
    //! [0,1]. Set before an ESC_REQUEST; consumed by the next Send() and cleared,
    //! so a stale value cannot leak onto an unrelated frame.
    void SetRequestTerms(uint8_t degradation, uint8_t battery)
    {
        m_nextReqDegradation = degradation;
        m_nextReqBattery = battery;
    }

  private:
    //! C4. Returns true if this request wins the UWB slot.
    bool ArbitrateRequest(const CaTblHeader& hdr);

  public:

    //! Which identity class this MAC was configured as. Needed by the scenario to
    //! pick an anchor to originate queries from; the config is otherwise private.
    CaTblIdentityClass GetIdentityClass() const { return m_cfg.identityClass; }

    //! C3: identity-scoped queries answered, and responses sent. Counted so the
    //! mechanism is measurable rather than asserted.
    uint64_t GetQueriesAnswered() const { return m_queriesAnswered; }
    uint64_t GetQueriesIgnored() const { return m_queriesIgnored; }

    //! C4. Grants, arbitration denials, and pre-emptions of a held slot.
    uint64_t GetArbGrants() const { return m_arbGrants; }
    uint64_t GetArbDenials() const { return m_arbDenials; }
    uint64_t GetArbPreemptions() const { return m_arbPreemptions; }
    //! Per-target grant counts, so a fairness delta is measurable rather than
    //! asserted. Keyed by originating node id.
    const std::map<uint16_t, uint64_t>& GetGrantsByTarget() const
    {
        return m_grantsByTarget;
    }

    /**
     * \brief The §2.9 handshake: mobility state selects which messages get sent.
     *
     * Stationary/approaching: Beacon -> Digest -> targeted Entry-Push, CW_normal.
     * Receding: skip the Digest, go straight to an urgent-flagged one-shot
     * Entry-Push at CW_high, because the link is degrading and a round-trip may
     * not complete. This is the whole of claim 4's "custom handshake" -- it is a
     * message-selection policy, not a separate protocol.
     */
    void StartHandshake(CaTblTier tier, uint16_t peerId, uint32_t nEntriesToPush);

    /** Delivered by the channel after propagation and capture arbitration. */
    void DeliverRx(const CaTblHeader& hdr, const CaTblLinkOutcome& out);

    /** Channel-side notifications, so per-message-type collision and capture
     *  counts are attributable instead of only existing as channel totals.
     *  Attributed to the receiver that observed the event, since that is where
     *  the arbitration happened. */
    void NoteCollisionLoss(CaTblMsgType t);
    void NoteCapture(CaTblMsgType t);

    /** Duty-cycle state, sliding window (§2.1). */
    double GetDutyCycle(CaTblTier tier) const;
    double GetDutyCycleLimit() const { return m_cfg.isAccessPoint ? 0.10 : 0.025; }
    bool WouldViolateDutyCycle(CaTblTier tier, Time toa) const;

    /** Current APC transmit power. */
    double GetTxPowerDbm() const { return m_txPowerDbm; }

    // ---- One-hop neighbour table -------------------------------------------

    /**
     * \brief Peers on \p tier heard directly within neighbourTtlS of \p nowS.
     *
     * \p nowS is passed rather than read from Simulator::Now() so this is callable
     * from a scenario loop that is slicing the simulator, and so a test can age
     * the table without advancing time.
     */
    std::vector<CaTblNeighbour> GetNeighbours(CaTblTier tier, double nowS) const;
    size_t GetNeighbourCount(CaTblTier tier, double nowS) const;

    /** Total insertions of a peer not already in the table -- the discovery rate.
     *  Distinct from the table size: a peer that ages out and is re-heard counts
     *  twice, which is what makes churn visible instead of averaged away. */
    uint64_t GetNeighbourInsertions() const { return m_neighbourInsertions; }
    void ClearNeighbours() { m_neighbours.clear(); }

    const CaTblMsgCounters& GetCounters(CaTblMsgType t) const;
    CaTblMsgCounters GetTotals() const;
    uint64_t GetDutyViolationAttempts() const { return m_dutyViolationAttempts; }

    /** Reset for a new run. */
    void Reset();

  private:
    struct PendingFrame
    {
        CaTblHeader hdr;
        CaTblTier tier;
        uint32_t nEntries;
        uint16_t destId;
        uint32_t cadAttempts;
        uint32_t txAttempts;
        uint32_t repeatsLeft;
        Time enqueuedAt;
    };

    void AttemptCad(PendingFrame f);
    void DoTransmit(PendingFrame f);
    void OnAckTimeout(uint32_t packetId);
    void MaybeRebroadcast(CaTblHeader hdr, CaTblTier tier);
    void UpdateApc(double rxPowerDbm, CaTblTier tier);
    void NoteNeighbour(const CaTblHeader& hdr, const CaTblLinkOutcome& out);
    void RecordAirtime(CaTblTier tier, Time toa);
    uint32_t NextPacketId();
    CaTblMsgCounters& Counters(CaTblMsgType t);

    CaTblMacConfig m_cfg;
    CaTblChannel* m_channel;
    CaTblEnergyModel* m_energy;
    Ptr<UniformRandomVariable> m_rng;
    CaTblMobilityState m_mobility;
    RxCallback m_rxCb;
    uint8_t m_nextQueryManifest = 0; //!< C3, consumed by the next Send()
    uint8_t m_nextReqDegradation = 0; //!< C4, consumed by the next Send()
    uint8_t m_nextReqBattery = 0;     //!< C4, consumed by the next Send()
    uint64_t m_queriesAnswered = 0;
    // C4 arbiter state: which target holds the slot and since when.
    uint16_t m_slotHolder = 0;
    double m_slotGrantedAtS = -1.0;
    double m_slotHolderScore = 0.0;
    std::map<uint16_t, double> m_lastGrantAtS;
    uint64_t m_arbGrants = 0;
    uint64_t m_arbDenials = 0;
    uint64_t m_arbPreemptions = 0;
    std::map<uint16_t, uint64_t> m_grantsByTarget;
    uint64_t m_queriesIgnored = 0;
    DropCallback m_dropCb;

    double m_txPowerDbm;
    uint32_t m_apcReportCount;
    double m_apcMarginAccum;

    uint32_t m_packetIdCounter;

    // Dedup: bounded LRU with a time window (§2.6).
    std::deque<std::pair<uint32_t, Time>> m_dedupOrder;
    std::map<uint32_t, Time> m_dedupSeen;

    // Sliding-window airtime per tier, for duty-cycle enforcement.
    std::deque<std::pair<Time, double>> m_airtime[CA_TBL_TIER_COUNT];

    // Explicit-ACK bookkeeping.
    std::map<uint32_t, PendingFrame> m_awaitingAck;

    // One-hop neighbour table, keyed (tier << 16) | peerId so the same peer id on
    // two tiers cannot alias. Ordered map, not unordered: iteration order feeds the
    // poll order, which feeds RNG draw order in the engines, and an unordered
    // container's iteration order is not part of the standard's guarantees.
    // Determinism was only just recovered (see the chrono defect); do not spend it
    // on a hash table.
    std::map<uint32_t, CaTblNeighbour> m_neighbours;
    uint64_t m_neighbourInsertions;

    std::map<uint8_t, CaTblMsgCounters> m_counters;
    uint64_t m_dutyViolationAttempts;
};

/**
 * \brief The shared medium.
 *
 * Owns the in-flight transmission list and does capture arbitration. This is the
 * piece that makes collisions emergent.
 *
 * Capture rule: among transmissions overlapping in time on the same tier and
 * audible at a given receiver, the strongest wins if it exceeds every other by
 * \c captureThresholdDb; otherwise all of them are lost. This is the standard
 * co-channel capture model and it matches what the physics engine's own
 * interference submodule already does for external interferers -- deliberately,
 * so the two layers do not disagree about what a collision is.
 */
class CaTblChannel
{
  public:
    CaTblChannel();

    void SetPhyProvider(CaTblPhyProvider* phy) { m_phy = phy; }

    /** Per-frame event sink for the visualiser. NOT a simulation input: the channel
     *  writes to it and never reads it, and when it is null nothing is written. The
     *  aggregate counters remain the authority on what happened; this is the same
     *  events, itemised, so an animation can show them. */
    void SetEventLog(std::ostream* os) { m_eventLog = os; }
    void SetCaptureThresholdDb(double db) { m_captureThresholdDb = db; }
    double GetCaptureThresholdDb() const { return m_captureThresholdDb; }

    void Register(CaTblMac* mac);
    void Clear();

    /** Begin a transmission. Schedules delivery at now + \p toa. */
    void Transmit(uint16_t txId,
                  CaTblTier tier,
                  const CaTblHeader& hdr,
                  Time toa,
                  double txPowerDbm,
                  uint16_t destId);

    /**
     * \brief CAD: is the channel busy as observed by \p observerId?
     *
     * True if any in-flight transmission on \p tier is audible at the observer.
     * "Audible" is the physics engine's own reception decision, which is what
     * makes hidden terminals fall out of the model rather than being assumed
     * away -- a node whose signal does not reach the observer is genuinely
     * invisible to its CAD, so it will transmit into a collision.
     */
    bool IsBusy(uint16_t observerId, CaTblTier tier);

    uint64_t GetTotalTransmissions() const { return m_totalTx; }
    uint64_t GetTotalCollisions() const { return m_totalCollisions; }
    uint64_t GetTotalCaptures() const { return m_totalCaptures; }
    uint64_t GetTotalDeliveries() const { return m_totalDeliveries; }

  private:
    struct InFlight
    {
        uint64_t txnId;
        uint16_t txId;
        CaTblTier tier;
        CaTblHeader hdr;
        Time start;
        Time end;
        double txPowerDbm;
        uint16_t destId;
    };

    void CompleteTransmission(uint64_t txnId);
    void PruneExpired();

    CaTblPhyProvider* m_phy;
    std::ostream* m_eventLog = nullptr;
    std::map<uint16_t, CaTblMac*> m_nodes;
    std::vector<InFlight> m_inFlight;
    double m_captureThresholdDb;
    uint64_t m_txnCounter;
    uint64_t m_totalTx;
    uint64_t m_totalCollisions;
    uint64_t m_totalCaptures;
    uint64_t m_totalDeliveries;
};

} // namespace ns3

#endif // CA_TBL_MAC_H
