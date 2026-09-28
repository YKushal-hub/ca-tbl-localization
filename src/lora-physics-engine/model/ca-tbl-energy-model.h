#ifndef CA_TBL_ENERGY_MODEL_H
#define CA_TBL_ENERGY_MODEL_H

/**
 * \file ca-tbl-energy-model.h
 * \brief CA-TBL Phase S2 -- per-tier radio energy accounting.
 *
 * Rationale
 * ---------
 * The architectural claim ("a 868/2.4GHz/UWB hybrid beats any single band") and
 * the procedural claim ("the handoff is worth its cost") are both energy claims.
 * Neither can be evaluated from position RMSE alone: a system that simply leaves
 * UWB on permanently wins on accuracy and loses on battery, and that trade is the
 * entire point of the architecture.
 *
 * This model therefore charges every radio action to a per-tier energy account:
 * time-on-air is computed from the Semtech CSS formula (not assumed), multiplied
 * by datasheet current draw, and integrated. Sleep current between cycles is
 * charged too, because at a 2.5% duty cycle sleep is a non-trivial fraction of
 * the budget and omitting it flatters every duty-cycled arm equally but not
 * equally *much*.
 *
 * Currents are datasheet typicals at 3.3V. They are declared as named constants
 * rather than inlined so a reviewer can check them against the part datasheets,
 * and so a sensitivity sweep over them is a one-line change.
 *
 * Deliberate scope limits (state these in the paper rather than letting a
 * reviewer find them):
 *   - MCU energy is not modelled. Only radio energy. The EKF/PCS compute cost is
 *     real but is identical across arms except for the UWB arms, and modelling it
 *     credibly needs a target MCU that is not yet fixed.
 *   - Regulator efficiency is not modelled; energy is charged at the radio rail.
 *   - PA ramp transients are not modelled; TX current is treated as a step.
 */

#include <cstdint>
#include <string>

namespace ns3
{

/** The three CA-TBL radio tiers, in escalation order. */
enum CaTblTier
{
    CA_TBL_TIER_868 = 0, //!< Tier 1: LoRa 868 MHz (SX1262 class), wide-area, RSSI/TDoA
    CA_TBL_TIER_2P4 = 1, //!< Tier 2: LoRa 2.4 GHz (SX1280 class), medium range, two-way ToF
    CA_TBL_TIER_UWB = 2  //!< Tier 3: UWB (DW3000 class), short range, TWR
};

/** Number of tiers; kept out of the enum so it cannot be passed as a tier. */
static const int CA_TBL_TIER_COUNT = 3;

/** Human-readable tier name, matching the Tier column emitted by the generators. */
const char* CaTblTierName(CaTblTier tier);

/**
 * \brief Datasheet-derived electrical profile for one radio.
 *
 * All currents in milliamps at \c supplyVoltageV.
 */
struct RadioPowerProfile
{
    double supplyVoltageV; //!< rail voltage the currents are quoted at
    double txCurrentMa;    //!< transmit, at the tier's configured TX power
    double rxCurrentMa;    //!< receive / listen
    double cadCurrentMa;   //!< channel activity detection (LoRa tiers only)
    double idleCurrentMa;  //!< oscillator/PLL running but not TX/RX (turnaround)
    double sleepCurrentMa; //!< deep sleep between duty cycles

    double cadDurationS;      //!< duration of one CAD probe
    double turnaroundS;       //!< TX->RX (or RX->TX) settling time
    double wakeupS;           //!< sleep->idle wake time, charged at idle current
    double rxWindowGuardS;    //!< extra listen time around an expected reply
    /**
     * Listen time burnt when an expected reply never arrives.
     *
     * This matters far more than it looks. A ranging attempt against an anchor
     * that is out of range does not cost less than a successful one -- it costs
     * MORE, because the receiver sits open for the full timeout instead of
     * closing as soon as the frame lands. Charging only successful exchanges
     * would make escalating into a dead zone look free, which is precisely the
     * behaviour the tier policy is supposed to be penalised for.
     */
    double rxTimeoutS;

    /** SX1262-class sub-GHz LoRa transceiver, TX at 14 dBm. */
    static RadioPowerProfile Sx1262_868();
    /** SX1280-class 2.4 GHz LoRa transceiver with ranging engine, TX at 12.5 dBm. */
    static RadioPowerProfile Sx1280_2p4();
    /** DW3000-class UWB transceiver. Note rxCurrentMa > txCurrentMa -- this is
     *  real, and it is why "listen for the reply" dominates a TWR exchange. */
    static RadioPowerProfile Dw3000_Uwb();
};

/**
 * \brief Accumulated energy and time for one radio, broken down by state.
 *
 * Kept as a breakdown rather than a single scalar so the evaluation can answer
 * "where did the joules go" -- which is the question that distinguishes a real
 * energy result from a plausible-looking number.
 */
struct TierEnergyAccount
{
    double txJ;
    double rxJ;
    double cadJ;
    double idleJ;
    double sleepJ;

    double txTimeS;   //!< airtime -- also the duty-cycle numerator
    double rxTimeS;
    double cadTimeS;
    double idleTimeS;
    double sleepTimeS;

    uint64_t txFrames;
    uint64_t rxFrames;
    uint64_t cadProbes;
    uint64_t rangingExchanges;

    TierEnergyAccount();
    void Reset();

    double TotalJ() const;
    /** Airtime fraction over \p elapsedS -- compare against the WPC Table-II
     *  budget (10% for network access points, 2.5% otherwise). */
    double DutyCycle(double elapsedS) const;
};

/**
 * \brief Charges radio activity to per-tier energy accounts.
 *
 * Usage is deliberately explicit: the caller states what physically happened
 * (a CAD probe, a ranging exchange with N anchors, a sleep interval) and the
 * model converts that to joules. Nothing is charged implicitly, so an arm that
 * never activates UWB accrues exactly zero UWB energy -- which is the whole
 * point of Phase S1 making the tier decision causal.
 */
class CaTblEnergyModel
{
  public:
    CaTblEnergyModel();

    /** Reset all accounts. Profiles are retained. */
    void Reset();

    /** Override a tier's electrical profile (for sensitivity sweeps). */
    void SetProfile(CaTblTier tier, const RadioPowerProfile& profile);
    const RadioPowerProfile& GetProfile(CaTblTier tier) const;

    /** Set the CSS PHY parameters used for time-on-air on the two LoRa tiers. */
    void SetLoRaPhy(CaTblTier tier, uint8_t sf, double bandwidthHz, uint8_t codingRate);

    // ---------------------------------------------------------------- ToA ---

    /**
     * \brief Semtech LoRa time-on-air.
     *
     * Tsym          = 2^SF / BW
     * Tpreamble     = (nPreamble + 4.25) * Tsym
     * nPayloadSym   = 8 + max( ceil( (8*PL - 4*SF + 28 + 16*CRC - 20*IH)
     *                                / (4*(SF - 2*DE)) ) * (CR + 4), 0 )
     * ToA           = Tpreamble + nPayloadSym * Tsym
     *
     * Low-data-rate optimisation (DE) is enabled automatically when the symbol
     * period exceeds 16 ms, per the datasheet rule, rather than being a caller
     * argument that is easy to set inconsistently.
     */
    static double LoRaTimeOnAirS(uint8_t sf,
                                 double bandwidthHz,
                                 uint32_t payloadBytes,
                                 uint8_t codingRate = 1,
                                 uint32_t preambleSymbols = 8,
                                 bool explicitHeader = true,
                                 bool crcEnabled = true);

    /**
     * \brief DW3000-class UWB frame duration.
     *
     * Not a CSS waveform, so the LoRa formula does not apply. Modelled as
     * preamble symbols at the 1.0176 us PRF symbol period, plus SHR/PHR
     * overhead, plus payload at the selected data rate (6.81 Mbps default).
     */
    static double UwbFrameTimeS(uint32_t payloadBytes,
                                uint32_t preambleSymbols = 128,
                                double dataRateBps = 6.81e6);

    // ------------------------------------------------------------ charging ---

    /** Charge one CAD probe on a LoRa tier. No-op for the UWB tier. */
    void AccountCad(CaTblTier tier);

    /**
     * \brief Charge one complete ranging cycle against \p nAnchorsAddressed anchors.
     *
     * The exchange structure differs per tier, and that difference is a large
     * part of why the tiers cost what they cost:
     *   - 868 MHz: one broadcast TX. Anchors timestamp it (TDoA) and read RSSI.
     *     The field node pays for exactly one frame regardless of anchor count.
     *   - 2.4 GHz: symmetric two-way ToF per anchor -- TX, turnaround, RX reply.
     *     Cost scales linearly with anchor count.
     *   - UWB: three-message TWR per anchor (poll, response, final), with the
     *     node listening through the response window at ~94 mA. Cost scales
     *     linearly and the RX term dominates.
     *
     * \param nRepliesReceived how many of those anchors actually answered. The
     *        remainder are charged the full \c rxTimeoutS instead of a short
     *        reply window, and the final TWR message is not sent for them.
     * \param nowS current simulated time, used to arm the duty-cycle governor.
     */
    void AccountRangingExchange(CaTblTier tier,
                                uint32_t nAnchorsAddressed,
                                uint32_t nRepliesReceived,
                                double nowS,
                                uint32_t payloadBytes = 20);

    /**
     * \brief Charge tier 1 for the downlink that carries the ranges back.
     *
     * FALSE by default, which preserves every published number.
     *
     * Tier 1 is billed as ONE BROADCAST AND NOTHING ELSE: the node transmits, the
     * anchors timestamp and measure, and the node's cost does not scale with how
     * many heard it. That is a TDoA shape, and it is the correct cost when THE
     * NETWORK localises -- anchors measure, a gateway multilaterates, and the field
     * node is never told the answer.
     *
     * But this model does not do that. `CaTblPolicy` runs ON THE FIELD NODE, and it
     * consumes a range per anchor every cycle. Those ranges have to reach it, and
     * nothing charges for their arrival: there is no per-beacon RX accounting, and
     * AccountListen only fires when the neighbour table is EMPTY. So the node runs a
     * filter on data it was never billed to receive, and tier 1's structural energy
     * advantage -- the thing every accuracy-per-joule figure rests on -- is measured
     * against an exchange that is one message short of closing.
     *
     * Setting this true charges the missing half: an RX window per responding anchor
     * and a full rxTimeoutS per silent one, the same treatment tier 2 already gets
     * for its replies. It does NOT add a second TX, because RSSI inversion needs no
     * two-way exchange -- only the answer coming back.
     *
     * This is a MEASUREMENT of the gap, not a claim that either shape is right. The
     * shapes answer different questions: TDoA-shaped is what a gateway-side estimator
     * costs the node, reply-shaped is what a node-side estimator costs it. Which one
     * applies is decided by the CONTRADICTION row in MASTER_PLAN 2.1 -- the spec puts
     * the estimator on the gateway and the code puts it on the tag -- and that is not
     * settled here.
     */
    void SetTier1ChargeReplies(bool on) { m_tier1ChargeReplies = on; }

    /**
     * \brief Put the estimator on the gateways.
     *
     * The node broadcasts once; the gateways measure and report over the
     * backhaul; the network computes the fix and sends back ONE downlink. So the
     * tier-1 reply windows are not charged to the node -- there are none -- and a
     * single guarded RX is charged instead, on cycles that produced a fix.
     *
     * Tier 1 only. Tiers 2 and 3 range by two-way time-of-flight because the
     * endpoints share no clock, so their exchange cannot move network-side
     * without the synchronisation TECH_DEBT J.2 records as absent.
     */
    void SetEdgeServer(bool on) { m_edgeServer = on; }
    bool GetEdgeServer() const { return m_edgeServer; }

    //! Charge the one downlink that returns a network-computed fix.
    void AccountEdgeServerDownlink(CaTblTier tier, uint32_t payloadBytes = 12);
    bool GetTier1ChargeReplies() const { return m_tier1ChargeReplies; }

    /**
     * \brief Charge the escalation request/grant round-trip.
     *
     * Escalation is the one ACK'd message class in the MAC design, so it costs a
     * TX plus a guarded RX window on the *current* tier. Charging it is what
     * makes threshold hysteresis measurable: without this cost, thrashing between
     * tiers is free and the hysteresis parameters look pointless.
     */
    void AccountEscalationHandshake(CaTblTier currentTier, double nowS, uint32_t payloadBytes = 12);

    // --------------------------------------------------------- duty cycle ---

    /**
     * \brief Set the regulatory airtime fraction for a tier.
     *
     * India WPC G.S.R. 853(E) Table-II: 10% for network access points, 2.5%
     * otherwise, in 865-868 MHz. The 2.4 GHz ISM band and UWB have no
     * comparable duty-cycle ceiling, so those default to 1.0 (unrestricted).
     *
     * Set to <= 0 to disable the governor for a tier.
     */
    void SetDutyCycleLimit(CaTblTier tier, double fraction);
    double GetDutyCycleLimit(CaTblTier tier) const;

    /**
     * \brief Whether \p tier is permitted to transmit at \p nowS.
     *
     * Implements the standard sub-GHz off-time rule: after occupying the
     * channel for T seconds, the transmitter must stay off for T/limit - T.
     * This is how the regulation is actually enforced in LoRa firmware, and it
     * is what makes the 868 tier's low cadence a physical constraint rather
     * than a tuning choice.
     */
    bool CanTransmit(CaTblTier tier, double nowS) const;
    double GetNextAllowedTxS(CaTblTier tier) const;

    /** Record that a transmission was suppressed by the governor. */
    void NoteDeferred(CaTblTier tier);
    uint64_t GetDeferredCount(CaTblTier tier) const;

    /** Charge \p seconds of deep sleep on \p tier. */
    void AccountSleep(CaTblTier tier, double seconds);

    /** Charge \p seconds of idle (oscillator up, no traffic) on \p tier. */
    void AccountIdle(CaTblTier tier, double seconds);

    /**
     * \brief Charge \p seconds with the receiver open, at the RX current.
     *
     * Added for the discovery path: a node that learns which anchors exist by
     * listening for their beacons is paying for that listening, and if it is not
     * charged then gating ranging on a neighbour cache would look like a free
     * energy saving. It is not free -- it moves cost from RX-timeout-per-dead-
     * anchor to RX-window-per-beacon, and the whole point of the change is to find
     * out which of those is larger.
     *
     * Distinct from AccountIdle(), which charges the idle current: on DW3000 the
     * two differ by a factor of ~7.8 (94 mA RX vs 12 mA IDLE_PLL), so charging a
     * listen window at the idle rate would understate discovery by nearly an order
     * of magnitude on the tier where it matters most.
     */
    void AccountListen(CaTblTier tier, double seconds);

    /**
     * \brief Charge \p airtimeS of transmission at the TX current, as \p frames frames.
     *
     * For traffic whose airtime the MAC has ALREADY measured -- CaTblMsgCounters::airtimeS
     * counts only frames that actually went on air, after the duty governor and CAD had
     * their say. Re-deriving it here from a payload size would be a second source of
     * truth for the same number, and would charge frames the governor refused. Used for
     * the anchor grant gossip (Network sect 6); nothing on the field node's path calls it.
     */
    void AccountTx(CaTblTier tier, double airtimeS, uint64_t frames);

    // ------------------------------------------------------------- readout ---

    const TierEnergyAccount& GetAccount(CaTblTier tier) const;
    double GetTotalJ() const;
    double GetTierJ(CaTblTier tier) const;

    /** CSV header matching WriteCsvRow(). */
    static std::string CsvHeader();
    /** One row summarising every tier plus the total. */
    std::string CsvRow(double elapsedS) const;
    /** Multi-line human-readable breakdown for stdout. */
    std::string Summary(double elapsedS) const;

  private:
    /** Charge \p seconds at \p currentMa on \p tier's rail, into \p bucketJ. */
    void Charge(CaTblTier tier, double seconds, double currentMa, double& bucketJ, double& bucketTimeS);

    RadioPowerProfile m_profiles[CA_TBL_TIER_COUNT];
    TierEnergyAccount m_accounts[CA_TBL_TIER_COUNT];

    uint8_t m_sf[CA_TBL_TIER_COUNT];
    double m_bandwidthHz[CA_TBL_TIER_COUNT];
    uint8_t m_codingRate[CA_TBL_TIER_COUNT];

    bool m_tier1ChargeReplies = false;
    bool m_edgeServer = false;

    double m_dutyLimit[CA_TBL_TIER_COUNT];
    double m_nextAllowedTxS[CA_TBL_TIER_COUNT];
    uint64_t m_deferred[CA_TBL_TIER_COUNT];

    /** Arm the off-time governor after occupying the channel for \p airtimeS. */
    void ArmDutyCycle(CaTblTier tier, double nowS, double airtimeS);
};

} // namespace ns3

#endif // CA_TBL_ENERGY_MODEL_H
