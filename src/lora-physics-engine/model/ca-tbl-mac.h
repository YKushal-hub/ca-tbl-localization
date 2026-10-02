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
#include "ca-tbl-grant-coord.h"
#include "ca-tbl-header.h"

#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <ostream>
#include <map>
#include <tuple>
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

    /**
     * \brief Evaluate the link for one specific frame. What CaTblChannel calls.
     *
     * The default is Evaluate() with the frame's hop count and nothing else, so a
     * provider that does not override it behaves exactly as before this existed. It
     * exists because one frame type -- the anchor GRANT_GOSSIP (Network sect 6) -- is
     * carried on a pair class the scenario's provider otherwise has no link for
     * (anchor to anchor), and that decision needs the frame, not just the endpoints.
     */
    virtual CaTblLinkOutcome EvaluateFrame(uint16_t txId,
                                           uint16_t rxId,
                                           CaTblTier tier,
                                           double txPowerDbm,
                                           const CaTblHeader& hdr)
    {
        return Evaluate(txId, rxId, tier, txPowerDbm, hdr.GetHopsTraversed());
    }

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
    /**
     * Fuse an entry whose sequence number EQUALS the held one, by Covariance
     * Intersection, instead of discarding it. FALSE by default.
     *
     * The spec's table rule has three branches and the code had two: newer supersedes,
     * older is discarded, and EQUAL was falling into the discard path. Equal is the
     * interesting case -- it is the same report arriving by a second mesh path, which
     * is information about the same instant from a route that saw different NLOS.
     */
    bool tableFuseCI;
    /**
     * Carry the {node_id, seq_no} summary in a DIGEST, and answer it. FALSE by default.
     *
     * The airtime for this has been billed since S4 and the frame carried nothing, so a
     * responder could not tell what the asker was missing and pushed its own entry
     * regardless. That is a request for one specific thing dressed as a request for
     * everything.
     */
    bool digestSummary;

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

    //! Network §4: answer a QUERY on behalf of a fresh table entry, not only
    //! when this node IS the target. The spec calls this the case that makes the
    //! query useful at all -- the target may be out of the requester's range but
    //! reachable through the mesh that already gossiped its position here.
    bool queryProxy = false;
    //! An entry older than this is not proxied. Answering from an arbitrarily old
    //! entry reports a position the mesh has no current reason to believe.
    double queryStalenessBoundS = 120.0;

    //! Transport §1: cache the node a QUERY arrived from, and unicast the
    //! RESPONSE back along that path instead of flooding it. Borrowed from
    //! AODV/DSR reactive routing. Falls back to flood on a cache miss, so a miss
    //! costs airtime rather than the answer.
    bool reversePath = false;
    //! Bounded and time-windowed on the same discipline as the dedup cache.
    double reversePathWindowS = 300.0;
    uint32_t reversePathMaxEntries = 64;
    //! NW-8. With reversePath: cache the PREVIOUS HOP a QUERY arrived from (the channel's
    //! transmitter), not its originator; unicast the RESPONSE to it, and every node on the
    //! way forwards it to its own cached previous hop; honour reversePathWindowS; and on a
    //! miss FLOOD the RESPONSE. Without it, reversePath caches the originator and both
    //! branches send the same one-hop frame (the pre-NW-8 behaviour, kept).
    bool reversePathHop = false;
    //! NW-9. QUERY and RESPONSE carry what the spec says. A QUERY carries its query id,
    //! manifest list, requester and priority; a node answers EVERY listed manifest it can
    //! (as the target, or by proxy from a fresh entry), with a RESPONSE carrying the
    //! answered entry, its manifest and the producer's status; it rebroadcasts the QUERY
    //! with the answered manifests removed, and not at all if it answered every one. The
    //! requester counts the answers and hands them to SetResponseCallback(). False keeps
    //! the single-manifest QUERY and the empty RESPONSE.
    bool queryV2 = false;

    //! NW-0. Under mesh links DISCOVERY is one-hop: neighbour discovery is one-hop by
    //! definition (MAC sect 8, "next beacon supersedes"), and flooding it through a mesh
    //! where every pair can hear every other is a beacon storm. False keeps it flooded.
    bool discoveryOneHop = false;

    //! MC-1 (docs/SPEC_CONFORMANCE_PLAN.md). The escalation handshake as MAC sect 8
    //! specifies it: an ACK matched by the packet id it names; an anchor that arbitrates
    //! FIRST and answers every request with a GRANT saying granted or denied (no bare
    //! ACK); a requester that treats a granted GRANT as the answer and retries a denied
    //! one. False reproduces the build before it.
    bool handshakeFixes = false;

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

/**
 * \brief L.2.5: one proxy check -- a QUERY reached a radio that is not the target it
 * names, and this is what that radio's table held about the target at that instant.
 *
 * The refusal counter could say THAT the staleness bound refused, never by how much:
 * "7 refusals / 0 answers" is equally consistent with entries 121 s old and 1 200 s
 * old, and those call for opposite changes. So the age is recorded BEFORE the freshness
 * decision and whatever it decides, which keeps the distribution independent of the
 * bound it is used to judge. A check that found no matching entry is recorded too,
 * because "held nothing" and "held something old" are different failures and share a
 * denominator. Observation only: nothing reads a record back into the MAC.
 */
struct CaTblProxyCheck
{
    double tS = 0.0;         //!< when the query arrived at this radio
    uint32_t queryId = 0;    //!< the query's packet_id, so arrivals group into queries
    //! The tier the QUERY frame is LABELLED with -- not necessarily the radio that heard
    //! it. The C3 QueryDriver binds every tier's driver to the first anchor MAC, which is
    //! gateway 101's 868 MAC, so every query travels on the 868 channel and only 868
    //! radios are asked; a "tier 1" label means 2.4 GHz link physics on an 868 radio.
    uint8_t tier = 0;
    uint8_t manifestId = 0;  //!< the target the query named
    bool held = false;       //!< a matching entry was in this radio's table
    bool answered = false;   //!< ...and it was inside the staleness bound
    CaTblTableEntry entry;   //!< the matching entry; meaningful only when held
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

    /** Publish this node's own position estimate and bump its sequence number. This
     *  is what an ENTRY_PUSH from this node then carries. */
    void PublishSelfEntry(double x, double y, double varX, double varY, double covXY,
                          double vx, double vy, double nowS);

    //! What this node knows about \p id, from any source. False if never heard of.
    bool GetTableEntry(uint16_t id, CaTblTableEntry& out) const;

    //! Everything this node knows. Read-only; nothing is polled from it.
    const std::map<uint16_t, CaTblTableEntry>& GetTable() const { return m_table; }

    //! Entries that ARRIVED rather than being produced here, so propagation is
    //! measurable instead of assumed.
    uint64_t GetTableMerges() const { return m_tableMerges; }

    //! Entries fused under the equal-sequence branch (Covariance Intersection).
    uint64_t GetTableFuses() const { return m_tableFuses; }
    //! Entries refused as strictly superseded. Counted so "nothing arrived" and
    //! "everything arriving was stale" are distinguishable, which they were not.
    uint64_t GetTableDiscards() const { return m_tableDiscards; }

    //! Entries pushed because a digest showed the asker lacked them.
    uint64_t GetDigestPushes() const { return m_digestPushes; }
    //! Digests answered with NOTHING because the asker was already current. This is
    //! the saving: without a summary every one of these was a wasted ENTRY_PUSH.
    uint64_t GetDigestNoOps() const { return m_digestNoOps; }


    /**
     * \brief Silence this radio without destroying it (sect 49 fault injection).
     *
     * A silenced MAC accepts calls and transmits nothing. It is NOT the same as an
     * unregistered node and must not be: the node still exists, still holds its table,
     * and resumes with that table intact. Deleting it would test a different fault and
     * would make reappearance indistinguishable from a new node joining, which is the
     * distinction CaTblNodeLifecycle exists to preserve.
     *
     * THIS IS TRANSMIT SUPPRESSION ONLY, and the limit is worth stating because the
     * obvious reading is wrong. It models a node that cannot TALK -- a transmit chain
     * that has failed, a duty budget exhausted, a power amplifier browning out on a
     * cold battery -- and the node goes on hearing everyone normally.
     *
     * IT DOES NOT MODEL AN OBSTRUCTION. A node behind a ridge is deaf as well as mute,
     * and the difference is observable: this one keeps its table current throughout the
     * outage and rejoins knowing everything that happened, where an obstructed node
     * rejoins with a table as stale as the outage was long. Anything reading this as
     * "the node went out of coverage" is reading in a fault that was not injected.
     *
     * Suppressed frames are COUNTED. A fault that leaves no trace cannot be told apart
     * from a quiet period, and an injection whose firing is unobservable is rule 9's
     * failure: a check for something whose signature is absence cannot be a scan of
     * present data.
     */
    void SetSilent(bool s) { m_silent = s; }
    //! NW-0. A radio the node has asleep this cycle neither receives nor rebroadcasts
    //! (under mesh links a sleeping 2.4 GHz/UWB radio would otherwise relay peers'
    //! floods on a tier the arm never woke, unbilled -- RULES.md sect 6).
    void SetAsleep(bool a) { m_asleep = a; }
    bool IsAsleep() const { return m_asleep; }
    void NoteAsleepRefusal() { ++m_asleepRefused; }
    uint64_t GetAsleepRefused() const { return m_asleepRefused; }
    //! NW-0. Airtime and frames of rebroadcasts this MAC transmitted (others' frames).
    double GetRebroadcastAirtimeS() const { return m_rebroadcastAirS; }
    uint64_t GetRebroadcastFrames() const { return m_rebroadcastFrames; }
    bool IsSilent() const { return m_silent; }
    //! Frames this MAC declined to transmit while silenced.
    uint64_t GetSuppressedTx() const { return m_suppressedTx; }

    void SetChannel(CaTblChannel* ch) { m_channel = ch; }
    //! The channel this MAC is registered on, i.e. which radio it is. Read by the
    //! scenario to bind a driver to the radio it claims to use (NW-3).
    const CaTblChannel* GetChannel() const { return m_channel; }
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
    //! NW-9. A query for a LIST of manifests at a priority (0 ROUTINE). Its query id is
    //! its packet id and its requester this node.
    void SendQueryV2(CaTblTier tier, const std::vector<uint8_t>& manifests, uint8_t priority);
    //! NW-9. The status the next PublishSelfEntry() stamps on this node's entry.
    void SetSelfStatus(uint8_t pcsQ, uint8_t integrityFlag, uint8_t batteryQ,
                       uint8_t degradationQ, bool uwbActive, uint16_t grantedBy);
    //! NW-9. Every first copy of an answer that reaches this node as its requester.
    void SetResponseCallback(std::function<void(const CaTblHeader&)> cb) { m_respCb = cb; }
    //! NW-9. One row per decision on a v2 QUERY (answered, proxied, relayed with the
    //! listed manifests left, suppressed), for the gate. Null = no log.
    void SetQueryLog(std::ostream* os) { m_queryLog = os; }
    uint64_t GetV2Answered() const { return m_v2Answered; }
    uint64_t GetV2Proxied() const { return m_v2Proxied; }
    uint64_t GetV2Suppressed() const { return m_v2Suppressed; }
    uint64_t GetV2Reduced() const { return m_v2Reduced; }
    uint64_t GetV2ResponsesHeard() const { return m_v2ResponsesHeard; }
    uint64_t GetV2EntriesHeard() const { return m_v2EntriesHeard; }

    /** Network §4: is there a fresh table entry about \p manifestId to answer on
     *  behalf of? Freshness is checked here, not by the caller, because a stale
     *  match and no match are different outcomes and are counted separately. */
    bool ProxyAnswerFor(uint8_t manifestId, double nowS, CaTblTier tier, uint32_t queryId,
                        CaTblTableEntry* answered = nullptr);
    /** Network §1: extra wire bytes for target_manifest_id, zero unless the
     *  proxy responder is enabled and the message actually carries entries. */
    uint32_t ManifestOverheadBytes(uint8_t msgType, uint32_t nEntries) const;
    /** Transport §1: record which neighbour a QUERY arrived from. */
    void RememberReversePath(uint32_t queryId, uint16_t fromId, double nowS);
    /** Transport §1: unicast a RESPONSE home, or flood if no path is cached. NW-8:
     *  answererId 0 means this node is answering; a forwarder passes the original's. */
    void SendResponse(CaTblTier tier, uint32_t queryId, uint16_t requesterId,
                      uint16_t answererId = 0);

    //! C4. The two U(r) terms an anchor cannot observe, quantised to 0..255 over
    //! [0,1]. Set before an ESC_REQUEST; consumed by the next Send() and cleared,
    //! so a stale value cannot leak onto an unrelated frame.
    /**
     * \brief MC-2. A TRIGGERED_UPDATE carrying this node's own entry and why it was sent
     * (1 = large correction, 2 = last gasp).
     *
     * A large correction goes like any TRIGGERED_UPDATE, with the redundant class's
     * jittered repeats. A last gasp goes ONCE and unrepeated, because the caller silences
     * the radio straight after. SetSilent() gates Send() only, so a frame already queued
     * still leaves -- which is what lets the last gasp out -- but repeats would leak past
     * the silence, and a dead battery sends nothing more.
     */
    void SendTriggeredUpdate(CaTblTier tier, uint8_t reason);
    //! MC-2. Time on air of a frame of this type on this MAC, as DoTransmit computes it.
    double FrameAirtimeS(CaTblMsgType type, CaTblTier tier, uint32_t nEntries) const;
    //! MC-2. TRIGGERED_UPDATE frames with a reason this node originated and transmitted
    //! (repeats included), and receptions here of such frames, by reason (1, 2).
    uint64_t GetTrigFramesSent() const { return m_trigFramesSent; }
    uint64_t GetTrigHeard(uint8_t reason) const { return reason < 3 ? m_trigHeard[reason] : 0; }
    //! MC-1. The tier an ESC_REQUEST asks for. Consumed by the next Send(), like the
    //! terms; read only under handshakeFixes.
    void SetRequestTargetTier(uint8_t tier) { m_nextReqTargetTier = tier; }
    //! MC-1 counters. All 0 unless handshakeFixes.
    uint64_t GetHsGrantsSent() const { return m_hsGrantsSent; }
    uint64_t GetHsDeniesSent() const { return m_hsDeniesSent; }
    uint64_t GetHsGrantsHeard() const { return m_hsGrantsHeard; }
    uint64_t GetHsDeniesHeard() const { return m_hsDeniesHeard; }
    uint64_t GetHsRequestsAnswered() const { return m_hsRequestsAnswered; }
    uint64_t GetHsAcksMatched() const { return m_hsAcksMatched; }
    uint64_t GetHsAcksUnmatched() const { return m_hsAcksUnmatched; }
    uint64_t GetHsRequestAcksSkipped() const { return m_hsRequestAcksSkipped; }
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

    //! Network §4 / Transport §1 accounting. Separated because they answer
    //! different questions: how often the mesh answered for someone else, and
    //! how often the answer found its way home without a flood.
    uint64_t GetProxyResponses() const { return m_proxyResponses; }
    uint64_t GetProxyRefusedStale() const { return m_proxyRefusedStale; }
    uint64_t GetResponsesUnicast() const { return m_responsesUnicast; }
    uint64_t GetResponsesFlooded() const { return m_responsesFlooded; }
    uint64_t GetReversePathHits() const { return m_reversePathHits; }
    uint64_t GetReversePathMisses() const { return m_reversePathMisses; }
    //! NW-8 accounting (reversePathHop): RESPONSE frames sent unicast to a cached
    //! previous hop (by the answerer and by every forwarder), misses that flooded,
    //! RESPONSEs forwarded on behalf of another node, and RESPONSEs that reached this
    //! node as their requester.
    uint64_t GetRpUnicastHops() const { return m_rpUnicastHops; }
    uint64_t GetRpFloods() const { return m_rpFloods; }
    uint64_t GetRpForwarded() const { return m_rpForwarded; }
    uint64_t GetRpDelivered() const { return m_rpDelivered; }
    uint64_t GetRpExpired() const { return m_rpExpired; }
    //! Copies of an answer already forwarded or delivered: retransmissions after a lost
    //! ACK, which carry a fresh packet id.
    uint64_t GetRpDuplicates() const { return m_rpDuplicates; }

    /**
     * \brief NW-7. Told the outcome of this node's own outstanding ESC_REQUEST: 1 a
     * GRANT(granted) answering it, 0 a GRANT(denied) answering it (both under
     * handshakeFixes, which carries the verdict and the acked packet id), -1 its
     * retransmissions ran out unanswered. The first argument is the answering anchor, 0
     * for -1. Unset (the default) changes nothing.
     */
    void SetUwbGrantCallback(std::function<void(uint16_t, int)> cb) { m_uwbGrantCb = cb; }
    //! NW-7. Drop this node's outstanding ESC_REQUESTs, so a denied one is not retried
    //! after the caller has given up on it. Returns how many.
    uint32_t CancelPendingRequests();
    //! L.2.5: every proxy check this radio made, in arrival order. Empty unless
    //! --queryProxy is on, because nothing else reaches ProxyAnswerFor.
    const std::vector<CaTblProxyCheck>& GetProxyChecks() const { return m_proxyChecks; }

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

    // ---- L.1.4 / Network sect 6: the anchor grant tie-break -----------------
    /**
     * \brief Attach the anchor's grant coordinator. Null (the default) leaves every
     *        path exactly as it was: no ESC_REQUEST is ever withheld and a
     *        GRANT_GOSSIP frame, should one arrive, changes nothing.
     *
     * One coordinator per ANCHOR, shared by its three MACs. A gateway is one node with
     * three radios, and the slot it holds for a field node is the same slot whichever
     * radio the request happened to arrive on.
     */
    void SetGrantCoordinator(CaTblGrantCoordinator* gc) { m_grantCoord = gc; }
    CaTblGrantCoordinator* GetGrantCoordinator() const { return m_grantCoord; }

    /** Per-verdict event sink for the tie-break. Written, never read; null writes
     *  nothing. Columns: t,anchorId,targetId,otherAnchorId,source,verdict,action,
     *  windowStartS,windowEndS. */
    void SetGrantLog(std::ostream* os) { m_grantLog = os; }

    /**
     * \brief Broadcast this anchor's currently held grants on \p tier (Network sect 6).
     *
     * One frame, one hop, LOW access category, through the ordinary CAD and duty
     * governor. Sends NOTHING when the anchor holds no active grant: an empty gossip
     * carries nothing a receiver could act on, and the channel it would occupy is the
     * one the field node ranges on. That silence is a design choice made here, not one
     * the spec states. A separate entry point, not a Send() argument, for the arity
     * reason SendQuery() records.
     */
    void SendGrantGossip(CaTblTier tier);

    //! Grant items heard in GRANT_GOSSIP frames (new ones only, echoes excluded).
    uint64_t GetGossipItemsHeard() const { return m_gossipItemsHeard; }
    //! Gossip rounds skipped because this anchor held nothing to report.
    uint64_t GetGossipSkippedEmpty() const { return m_gossipSkippedEmpty; }
    //! ESC_REQUESTs NOT answered with a grant because a higher-id anchor holds the slot.
    uint64_t GetGrantsWithheld() const { return m_grantsWithheld; }
    //! Own grants given up on hearing a higher-id anchor's overlapping grant.
    uint64_t GetGrantsYielded() const { return m_grantsYielded; }
    //! Airtime of GRANT_GOSSIP frames delivered to this MAC -- the receive half of what
    //! the gossip costs, which CaTblMsgCounters (transmit airtime only) cannot give.
    double GetGossipRxAirtimeS() const { return m_gossipRxAirtimeS; }

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

    /** Delivered by the channel after propagation and capture arbitration. txId is
     *  the transmitter of THIS copy -- the previous hop, which NW-8 needs. */
    void DeliverRx(const CaTblHeader& hdr, const CaTblLinkOutcome& out, uint16_t txId = 0);

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
    // L.1.4 tie-break state. All inert while m_grantCoord is null.
    CaTblGrantCoordinator* m_grantCoord = nullptr;
    std::ostream* m_grantLog = nullptr;
    std::vector<CaTblGrantItem> m_nextGossip; //!< consumed by the next Send()
    uint64_t m_gossipItemsHeard = 0;
    uint64_t m_gossipSkippedEmpty = 0;
    uint64_t m_grantsWithheld = 0;
    uint64_t m_grantsYielded = 0;
    double m_gossipRxAirtimeS = 0.0;
    void LogGrantVerdict(uint16_t targetId, uint16_t otherId, const char* source,
                         CaTblGrantVerdict v, const char* action, double startS,
                         double endS) const;
    //! MC-1. Names what the next Send() answers and, for a GRANT, what it says. Does
    //! nothing unless handshakeFixes.
    void PrepareReply(uint32_t ackedPacketId, bool isGrant, bool granted);
    uint64_t m_queriesIgnored = 0;
    DropCallback m_dropCb;

    double m_txPowerDbm;
    uint32_t m_apcReportCount;
    double m_apcMarginAccum;

    uint32_t m_packetIdCounter;
    std::map<uint16_t, CaTblTableEntry> m_table;
    uint16_t m_selfSeq;
    uint64_t m_tableMerges;
    uint64_t m_tableFuses;
    uint64_t m_tableDiscards;
    uint64_t m_digestPushes = 0;
    uint64_t m_digestNoOps = 0;
    bool m_silent = false;
    uint64_t m_suppressedTx = 0;
    const CaTblTableEntry* m_nextPushEntry = nullptr;

    // Dedup: bounded LRU with a time window (§2.6).
    std::deque<std::pair<uint32_t, Time>> m_dedupOrder;
    std::map<uint32_t, Time> m_dedupSeen;

    //! Transport §1. query_id -> the neighbour it arrived from. Bounded and
    //! time-windowed exactly like m_dedupSeen; an eviction is a correctness
    //! event only in that it forces the flood fallback.
    std::map<uint32_t, std::pair<uint16_t, double> > m_reversePath;

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
    uint64_t m_proxyResponses = 0;
    uint64_t m_proxyRefusedStale = 0;
    uint64_t m_responsesUnicast = 0;
    uint64_t m_responsesFlooded = 0;
    uint64_t m_reversePathHits = 0;
    uint64_t m_reversePathMisses = 0;
    // NW-8 (reversePathHop).
    uint32_t m_nextRespQueryId = 0;   //!< consumed by the next Send(), like the C4 terms
    uint16_t m_nextRespRequester = 0;
    uint16_t m_nextRespAnswerer = 0;
    uint64_t m_rpDuplicates = 0;
    //! (query, answerer) already forwarded or delivered here, with when; bounded like
    //! m_reversePath and windowed by dedupWindowS, like m_dedupSeen.
    std::map<std::tuple<uint32_t, uint16_t, uint8_t>, double> m_rpSeen;
    //! NW-9 adds the answered manifest to the key: one proxy can answer two manifests of a
    //! batch query, and those are two answers, not a retransmission. 0 outside queryV2.
    bool RpFirstCopy(uint32_t queryId, uint16_t answererId, uint8_t manifest, double nowS);
    // NW-7.
    std::function<void(uint16_t, int)> m_uwbGrantCb;
    // NW-9.
    std::function<void(const CaTblHeader&)> m_respCb;
    std::ostream* m_queryLog = nullptr;
    std::vector<uint8_t> m_nextV2Manifests;
    uint8_t m_nextV2Priority = 0;
    bool m_nextV2 = false;
    CaTblTableEntry m_nextRespEntry;
    bool m_nextHasRespEntry = false;
    uint8_t m_nextRespManifest = 0;
    uint8_t m_selfPcsQ = 0;
    uint8_t m_selfIntegrity = 0;
    uint8_t m_selfBatteryQ = 0;
    uint8_t m_selfDegradationQ = 0;
    bool m_selfUwbActive = false;
    uint16_t m_selfGrantedBy = 0;
    uint64_t m_v2Answered = 0;
    uint64_t m_v2Proxied = 0;
    uint64_t m_v2Suppressed = 0;
    uint64_t m_v2Reduced = 0;
    uint64_t m_v2ResponsesHeard = 0;
    uint64_t m_v2EntriesHeard = 0;
    void LogQueryV2(double tS, uint8_t tier, uint32_t queryId, const char* what,
                    uint8_t manifest, const std::vector<uint8_t>& left);
    bool m_nextForceFlood = false;
    uint64_t m_rpUnicastHops = 0;
    uint64_t m_rpFloods = 0;
    uint64_t m_rpForwarded = 0;
    uint64_t m_rpDelivered = 0;
    uint64_t m_rpExpired = 0;
    uint32_t RouteOverheadBytes(uint8_t msgType) const;
    std::vector<CaTblProxyCheck> m_proxyChecks;
    uint64_t m_dutyViolationAttempts;
    // MC-1 (handshakeFixes). Consumed by the next Send(), like the C4 terms.
    uint8_t m_nextReqTargetTier = 0xFF;
    uint32_t m_nextAckedPacketId = 0;
    bool m_nextHasVerdict = false;
    bool m_nextGranted = true;
    uint64_t m_hsGrantsSent = 0;
    uint64_t m_hsDeniesSent = 0;
    uint64_t m_hsGrantsHeard = 0;
    uint64_t m_hsDeniesHeard = 0;
    uint64_t m_hsRequestsAnswered = 0;
    uint64_t m_hsAcksMatched = 0;
    uint64_t m_hsAcksUnmatched = 0;
    uint64_t m_hsRequestAcksSkipped = 0;
    // MC-2 (triggered updates).
    uint8_t m_nextTriggerReason = 0; //!< consumed by the next Send()
    uint64_t m_trigFramesSent = 0;
    uint64_t m_trigHeard[3] = {0, 0, 0};
    // NW-0.
    bool m_asleep = false;
    uint64_t m_asleepRefused = 0;
    double m_rebroadcastAirS = 0.0;
    uint64_t m_rebroadcastFrames = 0;
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
