#include "ca-tbl-energy-model.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ns3
{

// ---------------------------------------------------------------------------
// Datasheet-derived profiles.
//
// Sources (record these in the paper's methodology table, do not leave them as
// bare constants):
//   SX1262  -- Semtech SX1262 datasheet Rev 2.1, Table "Power Consumption".
//              TX +14 dBm ~ 45 mA (DC-DC, 3.3V); RX boosted ~ 5.3 mA;
//              CAD draws RX-equivalent current; cold sleep ~ 1.2 uA (RC wake).
//   SX1280  -- Semtech SX1280 datasheet Rev 3.2. TX +12.5 dBm ~ 24 mA;
//              RX ~ 7.0 mA; sleep ~ 1.2 uA. Ranging engine runs the RX chain,
//              so ranging RX is charged at the RX current.
//   DW3000  -- Qorvo DW3000 datasheet / user manual. TX ~ 82 mA;
//              RX ~ 94 mA (note: RX exceeds TX, unlike the LoRa parts);
//              IDLE_PLL ~ 12 mA; DEEPSLEEP ~ 0.7 uA.
//
// The RX > TX asymmetry on DW3000 is the single most important number in this
// file: it means a UWB TWR exchange is dominated by the listen windows, not the
// transmissions, and therefore that reducing the *number* of UWB activations
// (what the CA-TBL policy does) matters more than shortening its frames.
// ---------------------------------------------------------------------------

RadioPowerProfile
RadioPowerProfile::Sx1262_868()
{
    RadioPowerProfile p;
    p.supplyVoltageV = 3.3;
    p.txCurrentMa = 45.0;
    p.rxCurrentMa = 5.3;
    p.cadCurrentMa = 5.3;
    p.idleCurrentMa = 1.5;
    p.sleepCurrentMa = 0.0012;
    p.cadDurationS = 0.0021; // ~2 symbols at SF7/125 kHz plus detect overhead
    p.turnaroundS = 0.0005;
    p.wakeupS = 0.0012;
    p.rxWindowGuardS = 0.0100; // guard for clock drift on the reply window
    p.rxTimeoutS = 0.0500;     // broadcast tier: no reply expected, unused
    return p;
}

RadioPowerProfile
RadioPowerProfile::Sx1280_2p4()
{
    RadioPowerProfile p;
    p.supplyVoltageV = 3.3;
    p.txCurrentMa = 24.0;
    p.rxCurrentMa = 7.0;
    p.cadCurrentMa = 7.0;
    p.idleCurrentMa = 1.4;
    p.sleepCurrentMa = 0.0012;
    p.cadDurationS = 0.0004;
    p.turnaroundS = 0.0002;
    p.wakeupS = 0.0010;
    p.rxWindowGuardS = 0.0020;
    p.rxTimeoutS = 0.0100; // SX1280 ranging reply timeout
    return p;
}

RadioPowerProfile
RadioPowerProfile::Dw3000_Uwb()
{
    RadioPowerProfile p;
    p.supplyVoltageV = 3.3;
    p.txCurrentMa = 82.0;
    p.rxCurrentMa = 94.0; // intentionally higher than TX -- see note above
    p.cadCurrentMa = 0.0; // no CAD on UWB
    p.idleCurrentMa = 12.0;
    p.sleepCurrentMa = 0.0007;
    p.cadDurationS = 0.0;
    p.turnaroundS = 0.0003;
    p.wakeupS = 0.0025; // DEEPSLEEP -> IDLE_PLL is slow and non-negligible
    p.rxWindowGuardS = 0.0005;
    // A failed TWR poll leaves the receiver open at 94 mA for the whole timeout.
    // At 3 ms that is ~0.93 mJ per unanswered anchor -- roughly 3x the cost of a
    // successful exchange, which is the correct sign for the incentive.
    p.rxTimeoutS = 0.0030;
    return p;
}

const char*
CaTblTierName(CaTblTier tier)
{
    switch (tier)
    {
    case CA_TBL_TIER_868:
        return "868MHz";
    case CA_TBL_TIER_2P4:
        return "2.4GHz";
    case CA_TBL_TIER_UWB:
        return "UWB";
    default:
        return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// TierEnergyAccount
// ---------------------------------------------------------------------------

TierEnergyAccount::TierEnergyAccount()
{
    Reset();
}

void
TierEnergyAccount::Reset()
{
    txJ = rxJ = cadJ = idleJ = sleepJ = 0.0;
    txTimeS = rxTimeS = cadTimeS = idleTimeS = sleepTimeS = 0.0;
    txFrames = rxFrames = cadProbes = rangingExchanges = 0;
}

double
TierEnergyAccount::TotalJ() const
{
    return txJ + rxJ + cadJ + idleJ + sleepJ;
}

double
TierEnergyAccount::DutyCycle(double elapsedS) const
{
    if (elapsedS <= 0.0)
    {
        return 0.0;
    }
    return txTimeS / elapsedS;
}

// ---------------------------------------------------------------------------
// CaTblEnergyModel
// ---------------------------------------------------------------------------

CaTblEnergyModel::CaTblEnergyModel()
{
    m_profiles[CA_TBL_TIER_868] = RadioPowerProfile::Sx1262_868();
    m_profiles[CA_TBL_TIER_2P4] = RadioPowerProfile::Sx1280_2p4();
    m_profiles[CA_TBL_TIER_UWB] = RadioPowerProfile::Dw3000_Uwb();

    // Defaults mirror ca_tbl_trajectory_generator_multi.cc so energy numbers and
    // physics numbers refer to the same PHY configuration.
    m_sf[CA_TBL_TIER_868] = 7;
    m_bandwidthHz[CA_TBL_TIER_868] = 125e3;
    m_codingRate[CA_TBL_TIER_868] = 1;

    m_sf[CA_TBL_TIER_2P4] = 7;
    m_bandwidthHz[CA_TBL_TIER_2P4] = 800e3;
    m_codingRate[CA_TBL_TIER_2P4] = 1;

    m_sf[CA_TBL_TIER_UWB] = 0; // unused; UWB is not a CSS waveform
    m_bandwidthHz[CA_TBL_TIER_UWB] = 499.2e6;
    m_codingRate[CA_TBL_TIER_UWB] = 1;

    // WPC G.S.R. 853(E) Table-II: 2.5% for a non-access-point device in
    // 865-868 MHz. The 2.4 GHz ISM band and UWB carry no equivalent ceiling.
    // The asymmetry is a real architectural constraint, not a modelling choice:
    // it is why the wide-area tier must be the slow one.
    m_dutyLimit[CA_TBL_TIER_868] = 0.025;
    m_dutyLimit[CA_TBL_TIER_2P4] = 1.0;
    m_dutyLimit[CA_TBL_TIER_UWB] = 1.0;

    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        m_nextAllowedTxS[i] = 0.0;
        m_deferred[i] = 0;
    }
}

void
CaTblEnergyModel::SetDutyCycleLimit(CaTblTier tier, double fraction)
{
    m_dutyLimit[tier] = fraction;
}

double
CaTblEnergyModel::GetDutyCycleLimit(CaTblTier tier) const
{
    return m_dutyLimit[tier];
}

bool
CaTblEnergyModel::CanTransmit(CaTblTier tier, double nowS) const
{
    if (m_dutyLimit[tier] <= 0.0 || m_dutyLimit[tier] >= 1.0)
    {
        return true;
    }
    return nowS >= m_nextAllowedTxS[tier];
}

double
CaTblEnergyModel::GetNextAllowedTxS(CaTblTier tier) const
{
    return m_nextAllowedTxS[tier];
}

void
CaTblEnergyModel::NoteDeferred(CaTblTier tier)
{
    m_deferred[tier]++;
}

uint64_t
CaTblEnergyModel::GetDeferredCount(CaTblTier tier) const
{
    return m_deferred[tier];
}

void
CaTblEnergyModel::ArmDutyCycle(CaTblTier tier, double nowS, double airtimeS)
{
    if (m_dutyLimit[tier] <= 0.0 || m_dutyLimit[tier] >= 1.0 || airtimeS <= 0.0)
    {
        return;
    }
    // Standard sub-GHz off-time rule: occupying the channel for T obliges the
    // transmitter to stay silent until T/limit has elapsed since it started.
    m_nextAllowedTxS[tier] = nowS + airtimeS / m_dutyLimit[tier];
}

void
CaTblEnergyModel::Reset()
{
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        m_accounts[i].Reset();
    }
}

void
CaTblEnergyModel::SetProfile(CaTblTier tier, const RadioPowerProfile& profile)
{
    m_profiles[tier] = profile;
}

const RadioPowerProfile&
CaTblEnergyModel::GetProfile(CaTblTier tier) const
{
    return m_profiles[tier];
}

void
CaTblEnergyModel::SetLoRaPhy(CaTblTier tier, uint8_t sf, double bandwidthHz, uint8_t codingRate)
{
    m_sf[tier] = sf;
    m_bandwidthHz[tier] = bandwidthHz;
    m_codingRate[tier] = codingRate;
}

double
CaTblEnergyModel::LoRaTimeOnAirS(uint8_t sf,
                                 double bandwidthHz,
                                 uint32_t payloadBytes,
                                 uint8_t codingRate,
                                 uint32_t preambleSymbols,
                                 bool explicitHeader,
                                 bool crcEnabled)
{
    if (bandwidthHz <= 0.0 || sf < 6 || sf > 12)
    {
        return 0.0;
    }

    const double tSym = std::pow(2.0, static_cast<double>(sf)) / bandwidthHz;
    const double tPreamble = (static_cast<double>(preambleSymbols) + 4.25) * tSym;

    // Low-data-rate optimisation is mandated by the datasheet when the symbol
    // period exceeds 16 ms (SF11/SF12 at 125 kHz). Deriving it here rather than
    // taking it as an argument removes a common source of inconsistent ToA.
    const int de = (tSym > 0.016) ? 1 : 0;
    const int ih = explicitHeader ? 0 : 1;
    const int crc = crcEnabled ? 1 : 0;

    const double denom = 4.0 * (static_cast<double>(sf) - 2.0 * de);
    if (denom <= 0.0)
    {
        return tPreamble;
    }

    const double numer = 8.0 * static_cast<double>(payloadBytes) - 4.0 * static_cast<double>(sf) +
                         28.0 + 16.0 * crc - 20.0 * ih;

    double nPayload = std::ceil(numer / denom) * (static_cast<double>(codingRate) + 4.0);
    nPayload = std::max(nPayload, 0.0);
    const double payloadSymbols = 8.0 + nPayload;

    return tPreamble + payloadSymbols * tSym;
}

double
CaTblEnergyModel::UwbFrameTimeS(uint32_t payloadBytes, uint32_t preambleSymbols, double dataRateBps)
{
    if (dataRateBps <= 0.0)
    {
        return 0.0;
    }
    // IEEE 802.15.4z HRP: SHR = preamble symbols at the 1.0176 us PRF symbol
    // period plus an 8-symbol SFD; PHR is 19 bits sent at the base rate.
    const double symbolS = 1.0176e-6;
    const double shrS = (static_cast<double>(preambleSymbols) + 8.0) * symbolS;
    const double phrS = 19.0 / 850e3;
    const double payloadS = (8.0 * static_cast<double>(payloadBytes) + 48.0) / dataRateBps; // +CRC/RS
    return shrS + phrS + payloadS;
}

void
CaTblEnergyModel::Charge(CaTblTier tier,
                         double seconds,
                         double currentMa,
                         double& bucketJ,
                         double& bucketTimeS)
{
    if (seconds <= 0.0)
    {
        return;
    }
    const double v = m_profiles[tier].supplyVoltageV;
    bucketJ += (currentMa * 1e-3) * v * seconds;
    bucketTimeS += seconds;
}

void
CaTblEnergyModel::AccountCad(CaTblTier tier)
{
    if (tier == CA_TBL_TIER_UWB)
    {
        return; // no CAD primitive on UWB
    }
    const RadioPowerProfile& p = m_profiles[tier];
    TierEnergyAccount& a = m_accounts[tier];

    Charge(tier, p.wakeupS, p.idleCurrentMa, a.idleJ, a.idleTimeS);
    Charge(tier, p.cadDurationS, p.cadCurrentMa, a.cadJ, a.cadTimeS);
    a.cadProbes++;
}

void
CaTblEnergyModel::AccountRangingExchange(CaTblTier tier,
                                         uint32_t nAnchorsAddressed,
                                         uint32_t nRepliesReceived,
                                         double nowS,
                                         uint32_t payloadBytes)
{
    const RadioPowerProfile& p = m_profiles[tier];
    TierEnergyAccount& a = m_accounts[tier];

    Charge(tier, p.wakeupS, p.idleCurrentMa, a.idleJ, a.idleTimeS);

    const uint32_t replies = std::min(nRepliesReceived, nAnchorsAddressed);
    const uint32_t misses = nAnchorsAddressed - replies;
    double airtimeThisExchange = 0.0;

    if (tier == CA_TBL_TIER_868)
    {
        // One broadcast. Anchors do the timestamping and RSSI work; the field
        // node's cost is independent of how many anchors happen to hear it.
        // This is the tier's structural energy advantage and should be called
        // out explicitly rather than buried in an aggregate number. It is also
        // why a miss costs nothing extra here -- there is no reply to wait for.
        const double toa =
            LoRaTimeOnAirS(m_sf[tier], m_bandwidthHz[tier], payloadBytes, m_codingRate[tier]);
        Charge(tier, toa, p.txCurrentMa, a.txJ, a.txTimeS);
        a.txFrames++;
        airtimeThisExchange += toa;
    }
    else if (tier == CA_TBL_TIER_2P4)
    {
        // Symmetric two-way ToF per anchor: TX request, turnaround, RX reply.
        const double toa =
            LoRaTimeOnAirS(m_sf[tier], m_bandwidthHz[tier], payloadBytes, m_codingRate[tier]);

        for (uint32_t i = 0; i < replies; ++i)
        {
            Charge(tier, toa, p.txCurrentMa, a.txJ, a.txTimeS);
            a.txFrames++;
            airtimeThisExchange += toa;
            Charge(tier, p.turnaroundS, p.idleCurrentMa, a.idleJ, a.idleTimeS);
            Charge(tier, toa + p.rxWindowGuardS, p.rxCurrentMa, a.rxJ, a.rxTimeS);
            a.rxFrames++;
        }
        for (uint32_t i = 0; i < misses; ++i)
        {
            // Poll sent, nothing came back: the receiver stays open for the full
            // timeout. This is strictly more expensive than a success.
            Charge(tier, toa, p.txCurrentMa, a.txJ, a.txTimeS);
            a.txFrames++;
            airtimeThisExchange += toa;
            Charge(tier, p.turnaroundS, p.idleCurrentMa, a.idleJ, a.idleTimeS);
            Charge(tier, p.rxTimeoutS, p.rxCurrentMa, a.rxJ, a.rxTimeS);
        }
    }
    else
    {
        // Three-message TWR per anchor: poll (TX), response (RX), final (TX).
        // The RX window is charged at 94 mA and is what dominates the exchange.
        const double frame = UwbFrameTimeS(payloadBytes);

        for (uint32_t i = 0; i < replies; ++i)
        {
            Charge(tier, frame, p.txCurrentMa, a.txJ, a.txTimeS); // poll
            a.txFrames++;
            airtimeThisExchange += frame;
            Charge(tier, p.turnaroundS, p.idleCurrentMa, a.idleJ, a.idleTimeS);
            Charge(tier, frame + p.rxWindowGuardS, p.rxCurrentMa, a.rxJ, a.rxTimeS); // response
            a.rxFrames++;
            Charge(tier, p.turnaroundS, p.idleCurrentMa, a.idleJ, a.idleTimeS);
            Charge(tier, frame, p.txCurrentMa, a.txJ, a.txTimeS); // final
            a.txFrames++;
            airtimeThisExchange += frame;
        }
        for (uint32_t i = 0; i < misses; ++i)
        {
            // Poll goes out, no response arrives, so the final message is never
            // sent -- but the node has already burnt the full RX timeout at
            // 94 mA. An unreachable anchor costs roughly three times what a
            // reachable one does. This is the mechanism that should make
            // escalating into a dead zone visibly expensive rather than free.
            Charge(tier, frame, p.txCurrentMa, a.txJ, a.txTimeS);
            a.txFrames++;
            airtimeThisExchange += frame;
            Charge(tier, p.turnaroundS, p.idleCurrentMa, a.idleJ, a.idleTimeS);
            Charge(tier, p.rxTimeoutS, p.rxCurrentMa, a.rxJ, a.rxTimeS);
        }
    }

    a.rangingExchanges++;
    ArmDutyCycle(tier, nowS, airtimeThisExchange);
}

double
CaTblEnergyModel::HandshakeFrameTimeS(CaTblTier tier, uint32_t payloadBytes) const
{
    if (tier == CA_TBL_TIER_UWB)
    {
        return UwbFrameTimeS(payloadBytes);
    }
    return LoRaTimeOnAirS(m_sf[tier], m_bandwidthHz[tier], payloadBytes, m_codingRate[tier]);
}

void
CaTblEnergyModel::AccountEscalationHandshake(CaTblTier currentTier,
                                             double nowS,
                                             uint32_t payloadBytes)
{
    const RadioPowerProfile& p = m_profiles[currentTier];
    TierEnergyAccount& a = m_accounts[currentTier];

    double frame;
    if (currentTier == CA_TBL_TIER_UWB)
    {
        frame = UwbFrameTimeS(payloadBytes);
    }
    else
    {
        frame = LoRaTimeOnAirS(m_sf[currentTier],
                               m_bandwidthHz[currentTier],
                               payloadBytes,
                               m_codingRate[currentTier]);
    }

    // Escalation request is the one ACK'd message class in the MAC design, so a
    // grant round-trip is charged: TX request + guarded RX window for the grant.
    Charge(currentTier, frame, p.txCurrentMa, a.txJ, a.txTimeS);
    a.txFrames++;
    Charge(currentTier, p.turnaroundS, p.idleCurrentMa, a.idleJ, a.idleTimeS);
    Charge(currentTier, frame + p.rxWindowGuardS, p.rxCurrentMa, a.rxJ, a.rxTimeS);
    a.rxFrames++;
}

void
CaTblEnergyModel::AccountSleep(CaTblTier tier, double seconds)
{
    const RadioPowerProfile& p = m_profiles[tier];
    TierEnergyAccount& a = m_accounts[tier];
    Charge(tier, seconds, p.sleepCurrentMa, a.sleepJ, a.sleepTimeS);
}

void
CaTblEnergyModel::AccountIdle(CaTblTier tier, double seconds)
{
    const RadioPowerProfile& p = m_profiles[tier];
    TierEnergyAccount& a = m_accounts[tier];
    Charge(tier, seconds, p.idleCurrentMa, a.idleJ, a.idleTimeS);
}

void
CaTblEnergyModel::AccountListen(CaTblTier tier, double seconds)
{
    const RadioPowerProfile& p = m_profiles[tier];
    TierEnergyAccount& a = m_accounts[tier];
    Charge(tier, seconds, p.rxCurrentMa, a.rxJ, a.rxTimeS);
}

const TierEnergyAccount&
CaTblEnergyModel::GetAccount(CaTblTier tier) const
{
    return m_accounts[tier];
}

double
CaTblEnergyModel::GetTierJ(CaTblTier tier) const
{
    return m_accounts[tier].TotalJ();
}

double
CaTblEnergyModel::GetTotalJ() const
{
    double sum = 0.0;
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        sum += m_accounts[i].TotalJ();
    }
    return sum;
}

std::string
CaTblEnergyModel::CsvHeader()
{
    std::ostringstream os;
    os << "TotalJ";
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        const std::string t = CaTblTierName(static_cast<CaTblTier>(i));
        os << "," << t << "_J"
           << "," << t << "_TxJ"
           << "," << t << "_RxJ"
           << "," << t << "_CadJ"
           << "," << t << "_IdleJ"
           << "," << t << "_SleepJ"
           << "," << t << "_AirtimeS"
           << "," << t << "_DutyCycle"
           << "," << t << "_Exchanges";
    }
    return os.str();
}

std::string
CaTblEnergyModel::CsvRow(double elapsedS) const
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(9) << GetTotalJ();
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        const TierEnergyAccount& a = m_accounts[i];
        os << "," << a.TotalJ() << "," << a.txJ << "," << a.rxJ << "," << a.cadJ << "," << a.idleJ
           << "," << a.sleepJ << "," << a.txTimeS << "," << a.DutyCycle(elapsedS) << ","
           << a.rangingExchanges;
    }
    return os.str();
}

std::string
CaTblEnergyModel::Summary(double elapsedS) const
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(6);
    os << "  Energy breakdown over " << elapsedS << " s\n";
    os << "  " << std::setw(8) << "tier" << std::setw(14) << "total_J" << std::setw(14) << "tx_J"
       << std::setw(14) << "rx_J" << std::setw(12) << "airtime_s" << std::setw(11) << "duty_%"
       << std::setw(12) << "exchanges" << "\n";
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        const TierEnergyAccount& a = m_accounts[i];
        os << "  " << std::setw(8) << CaTblTierName(static_cast<CaTblTier>(i)) << std::setw(14)
           << a.TotalJ() << std::setw(14) << a.txJ << std::setw(14) << a.rxJ << std::setw(12)
           << a.txTimeS << std::setw(11) << (a.DutyCycle(elapsedS) * 100.0) << std::setw(12)
           << a.rangingExchanges << "\n";
    }
    os << "  " << std::setw(8) << "TOTAL" << std::setw(14) << GetTotalJ() << "\n";
    return os.str();
}

} // namespace ns3
