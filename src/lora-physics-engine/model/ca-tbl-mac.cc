#include "ca-tbl-mac.h"
#include <iomanip>
#include <sstream>

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

namespace
{
bool g_digestOwnFirst = false;     // --digestOwnFirst; see CaTblMac::SetDigestOwnFirst
bool g_beaconCarriesEntry = false; // --beaconCarriesEntry; see CaTblMac::SetBeaconCarriesEntry
} // namespace

void
CaTblMac::SetBeaconCarriesEntry(bool on)
{
    g_beaconCarriesEntry = on;
}

bool
CaTblMac::BeaconCarriesEntry()
{
    return g_beaconCarriesEntry;
}

uint32_t
CaTblMac::BeaconEntryBytes(const CaTblHeader& h)
{
    return (g_beaconCarriesEntry && h.GetMsgType() == CA_TBL_MSG_DISCOVERY && h.HasSelfEntry() &&
            h.GetSelfEntry().valid)
               ? CA_TBL_TABLE_ENTRY_BYTES
               : 0u;
}

void
CaTblMac::SetDigestOwnFirst(bool on)
{
    g_digestOwnFirst = on;
}

bool
CaTblMac::DigestOwnFirst()
{
    return g_digestOwnFirst;
}

namespace
{
/**
 * What a frame is CARRYING, as one compact field for the event log.
 *
 * The log recorded a message TYPE and nothing about its contents, so a viewer could see
 * that an ENTRY_PUSH happened and not what position it moved -- which is the only part a
 * person watching a dissemination demo actually wants.
 *
 * Semicolons, never commas: this lands in a CSV cell and a comma here would silently
 * shift every column to its right. That failure is invisible in the file and obvious
 * only downstream, which is the worst place to find it.
 */
std::string
CaTblEventPayload(const CaTblHeader& h)
{
    std::ostringstream o;
    const CaTblMsgType t = h.GetMsgType();
    o << "pkt=" << h.GetPacketId() << ";hops=" << static_cast<int>(h.GetHopLimit());
    // VALID, not merely PRESENT. The header carries a self-entry slot on every frame and
    // HasSelfEntry() reports the slot, so a DIGEST or a QUERY -- which attach nothing --
    // were emitting "entry=0;x=0.0;y=0.0". A viewer reads that as a packet carrying a
    // position at the origin, which is a fact the frame does not contain.
    if (h.HasSelfEntry() && h.GetSelfEntry().valid)
    {
        const CaTblTableEntry& e = h.GetSelfEntry();
        o << ";entry=" << e.nodeId << ";x=" << std::fixed << std::setprecision(1) << e.x
          << ";y=" << e.y << ";sig=" << std::setprecision(1) << std::sqrt(std::max(e.varX, 0.0))
          << ";seq=" << e.seqNo << ";hop=" << static_cast<int>(e.hopCount);
        if (e.targetManifestId != 0)
        {
            o << ";mid=" << static_cast<int>(e.targetManifestId);
        }
    }
    if (t == CA_TBL_MSG_QUERY || t == CA_TBL_MSG_RESPONSE)
    {
        o << ";wants=" << static_cast<int>(h.GetQueryManifestId());
    }
    if (t == CA_TBL_MSG_DIGEST)
    {
        o << ";summary=" << h.GetDigest().size();
    }
    if (t == CA_TBL_MSG_ESC_REQUEST)
    {
        o << ";degr=" << static_cast<int>(h.GetReqDegradation())
          << ";batt=" << static_cast<int>(h.GetReqBattery());
    }
    if (t == CA_TBL_MSG_GRANT_GOSSIP)
    {
        o << ";grants=" << h.GetGrants().size();
        if (!h.GetQueueItems().empty())
        {
            o << ";queue=" << h.GetQueueItems().size(); // NW-6
        }
    }
    // MC-1. Non-default only under --handshakeFixes, so printed only there.
    if (t == CA_TBL_MSG_ESC_REQUEST && h.GetReqTargetTier() != 0xFF)
    {
        o << ";target=" << static_cast<int>(h.GetReqTargetTier());
    }
    if (h.GetReqQueryTriggered() != 0)
    {
        o << ";qt=1"; // NW-4
    }
    if (h.GetGrantOnBehalf())
    {
        o << ";obo=1"; // TA-2
    }
    if (h.GetReqSubject() != 0)
    {
        o << ";for=" << h.GetReqSubject(); // N.10
    }
    if (h.GetAckedPacketId() != 0)
    {
        o << ";acks=" << h.GetAckedPacketId();
    }
    // NW-9. Set only under queryV2, so printed only there.
    if (h.HasQueryV2())
    {
        o << ";q2=" << h.GetQueryV2Id() << ";prio=" << static_cast<int>(h.GetQueryPriority())
          << ";list=";
        for (size_t k = 0; k < h.GetQueryManifests().size(); ++k)
        {
            o << (k ? "|" : "") << static_cast<int>(h.GetQueryManifests()[k]);
        }
    }
    if (h.GetRespManifest() != 0)
    {
        o << ";rm=" << static_cast<int>(h.GetRespManifest());
    }
    // NW-8. Non-zero only under reversePathHop, so printed only there.
    if (h.GetRespQueryId() != 0)
    {
        o << ";rq=" << h.GetRespQueryId() << ";req=" << h.GetRespRequesterId()
          << ";ans=" << h.GetRespAnswererId() << ";flood=" << (h.GetForceFlood() ? 1 : 0);
    }
    if (h.HasGrantVerdict())
    {
        o << ";granted=" << (h.GetGranted() ? 1 : 0)
          << ";qpos=" << static_cast<int>(h.GetQueuePos());
    }
    // MC-2. Non-zero only under --triggeredUpdates.
    if (h.GetTriggerReason() != 0)
    {
        o << ";reason=" << static_cast<int>(h.GetTriggerReason());
    }
    return o.str();
}
} // namespace


NS_LOG_COMPONENT_DEFINE("CaTblMac");

CaTblLinkOutcome::CaTblLinkOutcome()
    : received(false),
      rxPowerDbm(-200.0),
      snrDb(-200.0),
      nlos(false),
      estimatedRangeM(0.0)
{
}

CaTblNeighbour::CaTblNeighbour()
    : id(0),
      tier(CA_TBL_TIER_868),
      lastHeardS(-1e18),
      lastRxPowerDbm(-200.0),
      lastSnrDb(-200.0),
      heardCount(0)
{
}

const char*
CaTblMobilityStateName(CaTblMobilityState s)
{
    switch (s)
    {
    case CA_TBL_MOB_STATIONARY:
        return "STATIONARY";
    case CA_TBL_MOB_APPROACHING:
        return "APPROACHING";
    case CA_TBL_MOB_RECEDING:
        return "RECEDING";
    default:
        return "UNKNOWN";
    }
}

CaTblMsgCounters::CaTblMsgCounters()
    : sent(0),
      rebroadcast(0),
      received(0),
      droppedDedup(0),
      droppedHopLimit(0),
      droppedDutyCycle(0),
      droppedCadExhausted(0),
      lostCollision(0),
      wonCapture(0),
      acksSent(0),
      acksReceived(0),
      retransmissions(0),
      ackTimeouts(0),
      airtimeS(0.0),
      totalBackoffS(0.0),
      cadBusyEvents(0)
{
}

CaTblMacConfig::CaTblMacConfig()
    : nodeId(0),
      identityClass(CA_TBL_ID_UNASSIGNED),
      targetManifestId(0),
      isAccessPoint(false),
      // Slot time is set to one SF7/125 kHz symbol period rounded up. Choosing the
      // slot as a symbol multiple rather than an arbitrary millisecond figure at
      // least ties it to the waveform, but it is OURS and is not calibrated.
      slotTimeS(0.001024),
      // CAD on SX126x is roughly 2 symbol periods of listen plus overhead.
      // SF7/125 kHz => ~2 ms. Radio-dependent.
      cadDurationS(0.002),
      maxCadRetries(4),
      // Jitter window: the design document says "tens to low hundreds of ms
      // depending on ToA at the active SF". 200 ms is the upper end of that.
      rebroadcastJitterMaxS(0.200),
      dedupCacheEntries(64),
      dedupWindowS(120.0),
      defaultHopLimit(3),
      txPowerDbm(14.0),
      txPowerMinDbm(2.0),
      // 500 mW e.r.p. = 26.99 dBm, the WPC Table-II ceiling. Note the default
      // starting power is 14 dBm, not the ceiling: APC is mandated to raise power
      // only when the margin demands it.
      txPowerMaxDbm(26.99),
      apcStepDb(2.0),
      apcSafetyMarginDb(6.0),
      apcUpperBoundDb(10.0),
      apcLowerBoundDb(3.0),
      apcEvalEveryNReports(8),
      ackTimeoutS(2.0),
      maxRetransmissions(2),
      triggeredUpdateRepeats(3),
      // Three beacon periods at the scenario's 5 s default. OURS, uncalibrated --
      // see the field comment in the header.
      neighbourTtlS(15.0),
      dutyWindowS(3600.0),
      sf(7),
      bandwidthHz(125e3),
      codingRate(1),
      sendHandshakeFrames(false),
      tableFuseCI(false),
      digestSummary(false)
{
}

// ---------------------------------------------------------------------------
// CaTblMac
// ---------------------------------------------------------------------------

CaTblMac::CaTblMac()
    : m_channel(nullptr),
      m_energy(nullptr),
      m_mobility(CA_TBL_MOB_STATIONARY),
      m_txPowerDbm(14.0),
      m_apcReportCount(0),
      m_apcMarginAccum(0.0),
      m_packetIdCounter(0),
      m_selfSeq(0),
      m_tableMerges(0),
      m_tableFuses(0),
      m_tableDiscards(0),
      m_neighbourInsertions(0),
      m_dutyViolationAttempts(0)
{
}

CaTblMac::~CaTblMac()
{
}

void
CaTblMac::Configure(const CaTblMacConfig& cfg)
{
    m_cfg = cfg;
    m_txPowerDbm = cfg.txPowerDbm;
    // packet_id must be globally unique or the dedup cache silently drops other
    // nodes' frames. Seeding the high bits with the node id is the cheap way to
    // get that without a coordinator.
    m_packetIdCounter = static_cast<uint32_t>(cfg.nodeId) << 20;
}

void
CaTblMac::Reset()
{
    m_dedupOrder.clear();
    m_dedupSeen.clear();
    for (int t = 0; t < CA_TBL_TIER_COUNT; ++t)
    {
        m_airtime[t].clear();
    }
    m_awaitingAck.clear();
    m_neighbours.clear();
    m_neighbourInsertions = 0;
    m_counters.clear();
    m_dutyViolationAttempts = 0;
    m_apcReportCount = 0;
    m_apcMarginAccum = 0.0;
    m_txPowerDbm = m_cfg.txPowerDbm;
    m_packetIdCounter = static_cast<uint32_t>(m_cfg.nodeId) << 20;
    m_nextGossip.clear();
    m_gossipItemsHeard = 0;
    m_gossipSkippedEmpty = 0;
    m_grantsWithheld = 0;
    m_grantsYielded = 0;
    m_gossipRxAirtimeS = 0.0;
    m_nextReqTargetTier = 0xFF;
    m_nextAckedPacketId = 0;
    m_nextHasVerdict = false;
    m_nextGranted = true;
    m_hsGrantsSent = 0;
    m_hsDeniesSent = 0;
    m_hsGrantsHeard = 0;
    m_hsDeniesHeard = 0;
    m_hsRequestsAnswered = 0;
    m_hsAcksMatched = 0;
    m_hsAcksUnmatched = 0;
    m_hsRequestAcksSkipped = 0;
}

void
CaTblMac::PublishSelfEntry(double x, double y, double varX, double varY, double covXY,
                           double vx, double vy, double nowS)
{
    CaTblTableEntry& e = m_table[m_cfg.nodeId];
    e.nodeId = m_cfg.nodeId;
    e.x = x;
    e.y = y;
    e.varX = varX;
    e.varY = varY;
    e.covXY = covXY;
    e.vx = vx;
    e.vy = vy;
    e.timestampS = nowS;
    e.hopCount = 0; // produced here, so zero hops by definition
    e.sourceType = static_cast<uint8_t>((static_cast<uint8_t>(m_cfg.identityClass) << 4) |
                                        (m_cfg.sf & 0x0F));
    e.targetManifestId = m_cfg.targetManifestId;
    e.seqNo = ++m_selfSeq;
    e.valid = true;
    if (m_cfg.queryV2)
    {
        // NW-9: the status the caller set for this publication.
        e.pcsQ = m_selfPcsQ;
        e.integrityFlag = m_selfIntegrity;
        e.batteryQ = m_selfBatteryQ;
        e.degradationQ = m_selfDegradationQ;
        e.uwbActive = m_selfUwbActive;
        e.grantedBy = m_selfGrantedBy;
        e.tierSource = m_selfTierSource; // TA-1
        e.fixHop = m_selfFixHop;
        e.reachArbiter = m_selfReachArbiter; // TA-2
    }
}

void
CaTblMac::SetSelfStatus(uint8_t pcsQ, uint8_t integrityFlag, uint8_t batteryQ,
                        uint8_t degradationQ, bool uwbActive, uint16_t grantedBy,
                        uint8_t tierSource, uint8_t fixHop, uint16_t reachArbiter)
{
    m_selfTierSource = tierSource;
    m_selfFixHop = fixHop;
    m_selfReachArbiter = reachArbiter;
    m_selfPcsQ = pcsQ;
    m_selfIntegrity = integrityFlag;
    m_selfBatteryQ = batteryQ;
    m_selfDegradationQ = degradationQ;
    m_selfUwbActive = uwbActive;
    m_selfGrantedBy = grantedBy;
}

void
CaTblMac::SendQueryV2(CaTblTier tier, const std::vector<uint8_t>& manifests, uint8_t priority)
{
    // NW-9. nEntries = the list length: CaTblPayloadBytes(QUERY, n) bills 6 + n.
    m_nextV2 = true;
    m_nextV2Manifests = manifests;
    m_nextV2Priority = priority;
    Send(CA_TBL_MSG_QUERY, tier, static_cast<uint32_t>(manifests.empty() ? 1 : manifests.size()),
         0);
    m_nextV2 = false;
    m_nextV2Manifests.clear();
}

void
CaTblMac::LogQueryV2(double tS, uint8_t tier, uint32_t queryId, const char* what,
                     uint8_t manifest, const std::vector<uint8_t>& left)
{
    if (m_queryLog == nullptr)
    {
        return;
    }
    *m_queryLog << std::fixed << std::setprecision(6) << tS << ',' << m_cfg.nodeId << ','
                << static_cast<int>(tier) << ',' << queryId << ',' << what << ','
                << static_cast<int>(manifest) << ',';
    for (size_t k = 0; k < left.size(); ++k)
    {
        *m_queryLog << (k ? "|" : "") << static_cast<int>(left[k]);
    }
    *m_queryLog << '\n';
}

bool
CaTblMac::GetTableEntry(uint16_t id, CaTblTableEntry& out) const
{
    std::map<uint16_t, CaTblTableEntry>::const_iterator it = m_table.find(id);
    if (it == m_table.end() || !it->second.valid)
    {
        return false;
    }
    out = it->second;
    return true;
}

uint32_t
CaTblMac::NextPacketId()
{
    return ++m_packetIdCounter;
}

CaTblMsgCounters&
CaTblMac::Counters(CaTblMsgType t)
{
    return m_counters[static_cast<uint8_t>(t)];
}

const CaTblMsgCounters&
CaTblMac::GetCounters(CaTblMsgType t) const
{
    static const CaTblMsgCounters empty;
    std::map<uint8_t, CaTblMsgCounters>::const_iterator it =
        m_counters.find(static_cast<uint8_t>(t));
    return (it == m_counters.end()) ? empty : it->second;
}

CaTblMsgCounters
CaTblMac::GetTotals() const
{
    CaTblMsgCounters tot;
    for (std::map<uint8_t, CaTblMsgCounters>::const_iterator it = m_counters.begin();
         it != m_counters.end();
         ++it)
    {
        const CaTblMsgCounters& c = it->second;
        tot.sent += c.sent;
        tot.rebroadcast += c.rebroadcast;
        tot.received += c.received;
        tot.droppedDedup += c.droppedDedup;
        tot.droppedHopLimit += c.droppedHopLimit;
        tot.droppedDutyCycle += c.droppedDutyCycle;
        tot.droppedCadExhausted += c.droppedCadExhausted;
        tot.lostCollision += c.lostCollision;
        tot.wonCapture += c.wonCapture;
        tot.acksSent += c.acksSent;
        tot.acksReceived += c.acksReceived;
        tot.retransmissions += c.retransmissions;
        tot.ackTimeouts += c.ackTimeouts;
        tot.airtimeS += c.airtimeS;
        tot.totalBackoffS += c.totalBackoffS;
        tot.cadBusyEvents += c.cadBusyEvents;
    }
    return tot;
}

// ---- duty cycle (§2.1) ----------------------------------------------------

/**
 * Duty cycle is airtime within the trailing observation window divided by the
 * FULL window length -- not by elapsed simulation time.
 *
 * This distinction is not pedantic; getting it wrong the other way is a bug the
 * S4 validator caught. Normalising by elapsed time makes the first frame of any
 * run look like ~100% duty and then decays, so a compliant node is refused
 * transmissions during startup for no physical reason. ETSI EN 300 220 and the
 * WPC G.S.R. 853(E) Table-II cap are both airtime budgets per observation period
 * (conventionally one hour), i.e. a field node may transmit for 0.025 * 3600 =
 * 90 s in any trailing hour and a gateway for 360 s. Below that budget it is
 * compliant regardless of how concentrated the airtime is.
 */
double
CaTblMac::GetDutyCycle(CaTblTier tier) const
{
    const double now = Simulator::Now().GetSeconds();
    const double cutoff = now - m_cfg.dutyWindowS;
    double sum = 0.0;
    for (std::deque<std::pair<Time, double>>::const_iterator it = m_airtime[tier].begin();
         it != m_airtime[tier].end();
         ++it)
    {
        if (it->first.GetSeconds() >= cutoff)
        {
            sum += it->second;
        }
    }
    return (m_cfg.dutyWindowS > 0.0) ? sum / m_cfg.dutyWindowS : 0.0;
}

bool
CaTblMac::WouldViolateDutyCycle(CaTblTier tier, Time toa) const
{
    // The cap is regulatory and applies to the 865-868 MHz band only. 2.4 GHz ISM
    // and UWB are license-exempt with no comparable duty-cycle restriction, which
    // is a real asymmetry the design exploits: routine gossip is cheaper to carry
    // on tier 2 precisely because tier 1's airtime is rationed by law.
    if (tier != CA_TBL_TIER_868)
    {
        return false;
    }
    // Same budget interpretation as GetDutyCycle(): would this frame push trailing
    // airtime past limit * window seconds?
    const double now = Simulator::Now().GetSeconds();
    const double cutoff = now - m_cfg.dutyWindowS;
    double sum = toa.GetSeconds();
    for (std::deque<std::pair<Time, double>>::const_iterator it = m_airtime[tier].begin();
         it != m_airtime[tier].end();
         ++it)
    {
        if (it->first.GetSeconds() >= cutoff)
        {
            sum += it->second;
        }
    }
    if (m_cfg.dutyWindowS <= 0.0)
    {
        return false;
    }
    const double budgetS = GetDutyCycleLimit() * m_cfg.dutyWindowS;
    return sum > budgetS;
}

void
CaTblMac::RecordAirtime(CaTblTier tier, Time toa)
{
    m_airtime[tier].push_back(std::make_pair(Simulator::Now(), toa.GetSeconds()));
    const double cutoff = Simulator::Now().GetSeconds() - m_cfg.dutyWindowS;
    while (!m_airtime[tier].empty() && m_airtime[tier].front().first.GetSeconds() < cutoff)
    {
        m_airtime[tier].pop_front();
    }
}

// ---- APC (§2.1.1) ---------------------------------------------------------

void
CaTblMac::UpdateApc(double rxPowerDbm, CaTblTier tier)
{
    // Table-II makes APC a CONDITION of operating in the band, not an
    // optimisation. It runs on 868 only; the other tiers are not subject to it.
    if (tier != CA_TBL_TIER_868 || m_channel == nullptr)
    {
        return;
    }

    // The loop needs the margin as seen by the RECEIVER, which the receiver has
    // and the transmitter does not. In hardware this arrives on the link-check /
    // ACK path. Here it is fed in from the receive side of the same node, which
    // means a node only adapts when it is being talked to -- a real limitation,
    // not a modelling shortcut, and it is why apcEvalEveryNReports exists rather
    // than adapting on every frame.
    m_apcMarginAccum += rxPowerDbm;
    ++m_apcReportCount;
    if (m_apcReportCount < m_cfg.apcEvalEveryNReports)
    {
        return;
    }

    const double meanRx = m_apcMarginAccum / static_cast<double>(m_apcReportCount);
    m_apcReportCount = 0;
    m_apcMarginAccum = 0.0;

    // Provider gives the sensitivity floor for the current PHY setting.
    const double floorDbm = -137.0; // conservative SF12/125k-class default
    const double margin = meanRx - floorDbm - m_cfg.apcSafetyMarginDb;

    if (margin > m_cfg.apcUpperBoundDb)
    {
        m_txPowerDbm = std::max(m_cfg.txPowerMinDbm, m_txPowerDbm - m_cfg.apcStepDb);
    }
    else if (margin < m_cfg.apcLowerBoundDb)
    {
        m_txPowerDbm = std::min(m_cfg.txPowerMaxDbm, m_txPowerDbm + m_cfg.apcStepDb);
    }
}

// ---- one-hop neighbour table ----------------------------------------------

void
CaTblMac::NoteNeighbour(const CaTblHeader& hdr, const CaTblLinkOutcome& out)
{
    // ONE HOP ONLY, and this guard is the whole correctness of the table.
    //
    // DISCOVERY, TRIGGERED_UPDATE and QUERY are flooded types. A relayed copy
    // arrives with the ORIGINATOR in origin_id -- the relay does not rewrite it,
    // because the flood's dedup and supersession logic keys on the originator.
    // So a frame that reached us in two hops still names a peer we may be quite
    // unable to hear. Admitting it would put an unreachable anchor into the poll
    // set, and the poll would then burn a full rxTimeoutS against it every cycle:
    // 0.93 mJ on UWB, three times what a SUCCESSFUL exchange costs. That is the
    // exact cost this table was added to remove, so a naive implementation here
    // does not merely fail to help, it reproduces the original defect while
    // appearing to have fixed it.
    //
    // A frame is first-hop iff its hop_limit has not yet been decremented.
    if (hdr.GetHopsTraversed() != 0)
    {
        return;
    }

    const uint16_t peer = hdr.GetOriginId();
    if (peer == m_cfg.nodeId)
    {
        return;
    }

    const CaTblTier tier = static_cast<CaTblTier>(hdr.GetTier());
    const uint32_t key = (static_cast<uint32_t>(tier) << 16) | static_cast<uint32_t>(peer);

    std::map<uint32_t, CaTblNeighbour>::iterator it = m_neighbours.find(key);
    if (it == m_neighbours.end())
    {
        CaTblNeighbour n;
        n.id = peer;
        n.tier = tier;
        n.heardCount = 0;
        it = m_neighbours.insert(std::make_pair(key, n)).first;
        ++m_neighbourInsertions;
    }

    it->second.lastHeardS = Simulator::Now().GetSeconds();
    it->second.lastRxPowerDbm = out.rxPowerDbm;
    it->second.lastSnrDb = out.snrDb;
    ++it->second.heardCount;
}

std::vector<CaTblNeighbour>
CaTblMac::GetNeighbours(CaTblTier tier, double nowS) const
{
    std::vector<CaTblNeighbour> live;
    for (std::map<uint32_t, CaTblNeighbour>::const_iterator it = m_neighbours.begin();
         it != m_neighbours.end();
         ++it)
    {
        if (it->second.tier != tier)
        {
            continue;
        }
        if ((nowS - it->second.lastHeardS) <= m_cfg.neighbourTtlS)
        {
            live.push_back(it->second);
        }
    }
    return live;
}

size_t
CaTblMac::GetNeighbourCount(CaTblTier tier, double nowS) const
{
    return GetNeighbours(tier, nowS).size();
}

// ---- transmit path (§2.3) -------------------------------------------------

bool
CaTblMac::ArbitrateRequest(const CaTblHeader& hdr)
{
    // C4. U(r) = l1*T_deg + l2*T_batt + l3*T_stale + l4*T_query, ported from
    // scratch/python_handoff/arbitration_harness.py rather than reinvented.
    //
    // T_deg and T_batt ride on the request because an anchor cannot see another
    // node's filter history or its battery. T_stale and T_query are computed here
    // because the anchor CAN see them: it knows when it last granted this target,
    // and urgency is the frame's own access category.
    const double now = Simulator::Now().GetSeconds();
    const uint16_t who = hdr.GetOriginId();

    const double tDeg = static_cast<double>(hdr.GetReqDegradation()) / 255.0;
    const double tBatt = static_cast<double>(hdr.GetReqBattery()) / 255.0;

    // STALENESS_BOUND = 10 s in the harness. A target never granted before is
    // maximally stale, which is what stops a newcomer being starved by an
    // incumbent that keeps renewing.
    double dt = m_cfg.arbSlotHoldS;
    std::map<uint16_t, double>::const_iterator it = m_lastGrantAtS.find(who);
    if (it != m_lastGrantAtS.end())
    {
        dt = now - it->second;
    }
    const double tStale = std::min(1.0, dt / 10.0);

    // Harness mapping: URGENT 1.0, normal 0.5, ambient 0.0. CA_TBL_AC_URGENT and
    // CA_TBL_AC_HIGH both count as urgent here -- HIGH is the category the taxonomy
    // assigns to UWB arbitration request/grant precisely because it is time-sensitive.
    //
    // NOTE, and it bounds what this term can do: EVERY ESC_REQUEST carries the same
    // access category, so across competing requests T_query is a CONSTANT and
    // contributes nothing to the ordering. It is scored anyway because the harness
    // defines U(r) with four terms and dropping one silently would make this a
    // different function; but lambda_4 cannot discriminate between UWB requests until
    // the taxonomy gives them differing urgency.
    const CaTblAccessCategory ac = hdr.GetAccessCategory();
    const double tQuery = (ac == CA_TBL_AC_URGENT || ac == CA_TBL_AC_HIGH)
                              ? 1.0
                              : ((ac == CA_TBL_AC_NORMAL) ? 0.5 : 0.0);

    const double u = m_cfg.arbLambdaDeg * tDeg + m_cfg.arbLambdaBatt * tBatt +
                     m_cfg.arbLambdaStale * tStale + m_cfg.arbLambdaQuery * tQuery;

    const bool slotFree = (m_slotGrantedAtS < 0.0) ||
                          ((now - m_slotGrantedAtS) >= m_cfg.arbSlotHoldS) ||
                          (m_slotHolder == who);

    if (slotFree)
    {
        m_slotHolder = who;
        m_slotGrantedAtS = now;
        m_slotHolderScore = u;
        m_lastGrantAtS[who] = now;
        ++m_arbGrants;
        ++m_grantsByTarget[who];
        return true;
    }

    if (u > m_slotHolderScore)
    {
        // Pre-emption. Counted separately from an ordinary grant because
        // "the arbiter changed its mind" and "the slot was free" are different
        // events and pooling them would hide how often priority actually inverts.
        m_slotHolder = who;
        m_slotGrantedAtS = now;
        m_slotHolderScore = u;
        m_lastGrantAtS[who] = now;
        ++m_arbGrants;
        ++m_arbPreemptions;
        ++m_grantsByTarget[who];
        return true;
    }

    ++m_arbDenials;
    return false;
}

double
CaTblMac::ScoreV2(double tDeg, double rem, double tStale, uint8_t qt, double* tBatt) const
{
    // NW-5. U(r) = l1*T_deg + l2*T_batt + l3*T_stale + l4*T_query, Network sect 5, with
    // T_batt the spec's 1/battery_remaining normalised: min(1/max(rem, e), 1/e) * e lies in
    // [e, 1] and is 1 for a battery at or under e (OURS, e = arbBattEps).
    const double e = m_cfg.arbBattEps;
    const double tb = std::min(1.0 / std::max(rem, e), 1.0 / e) * e;
    if (tBatt != nullptr)
    {
        *tBatt = tb;
    }
    return m_cfg.arbLambdaDeg * tDeg + m_cfg.arbLambdaBatt * tb + m_cfg.arbLambdaStale * tStale +
           m_cfg.arbLambdaQuery * static_cast<double>(qt);
}

void
CaTblMac::LogArb(double tS, uint16_t who, const char* ev, double u, double tDeg, double rem,
                 double tBatt, double tStale, uint8_t qt, uint32_t queuePos)
{
    if (m_arbLog == nullptr)
    {
        return;
    }
    std::map<uint16_t, double>::const_iterator ev2 = m_uwbEvidence.find(who);
    *m_arbLog << std::fixed << std::setprecision(6) << tS << ',' << m_cfg.nodeId << ',' << who
              << ',' << ev << ',' << u << ',' << tDeg << ',' << rem << ',' << tBatt << ','
              << tStale << ',' << static_cast<int>(qt) << ',' << queuePos << ','
              << (ev2 == m_uwbEvidence.end() ? -1.0 : tS - ev2->second) << '\n';
}

int
CaTblMac::ArbitrateV2(const CaTblHeader& hdr, uint8_t* queuePos)
{
    const double now = Simulator::Now().GetSeconds();
    const uint16_t who = hdr.GetOriginId();
    *queuePos = 0;
    const double tDeg = static_cast<double>(hdr.GetReqDegradation()) / 255.0;
    const double rem = static_cast<double>(hdr.GetReqBattery()) / 255.0; // REMAINING under V2
    const uint8_t qt = hdr.GetReqQueryTriggered();
    std::map<uint16_t, double>::const_iterator evd = m_uwbEvidence.find(who);
    if (evd == m_uwbEvidence.end() || now - evd->second > m_cfg.arbUwbEvidenceS)
    {
        // Hearing the request proves nothing about UWB reach: it came on 2.4 GHz.
        ++m_arbOutOfRange;
        ++m_arbDenials;
        LogArb(now, who, "out_of_range", 0.0, tDeg, rem, 0.0, 0.0, qt, 0);
        return -1;
    }
    double dtS = m_cfg.arbSlotHoldS;
    std::map<uint16_t, double>::const_iterator it = m_lastGrantAtS.find(who);
    if (it != m_lastGrantAtS.end())
    {
        dtS = now - it->second;
    }
    const double tStale = std::min(1.0, dtS / 10.0);
    double tBatt = 0.0;
    const double u = ScoreV2(tDeg, rem, tStale, qt, &tBatt);
    const bool slotFree = (m_slotGrantedAtS < 0.0) ||
                          ((now - m_slotGrantedAtS) >= m_cfg.arbSlotHoldS) || (m_slotHolder == who);
    if (slotFree || u > m_slotHolderScore)
    {
        const bool pre = !slotFree;
        m_slotHolder = who;
        m_slotGrantedAtS = now;
        m_slotHolderScore = u;
        m_lastGrantAtS[who] = now;
        ++m_arbGrants;
        if (pre)
        {
            ++m_arbPreemptions;
        }
        ++m_grantsByTarget[who];
        for (size_t k = 0; k < m_arbQueue.size(); ++k)
        {
            if (m_arbQueue[k].who == who)
            {
                m_arbQueue.erase(m_arbQueue.begin() + static_cast<long>(k));
                break;
            }
        }
        LogArb(now, who, pre ? "preempt" : "grant", u, tDeg, rem, tBatt, tStale, qt, 0);
        PublishQueue(now);
        return 1;
    }
    // NW-6: a request a higher-id anchor already queues or has granted is not queued here.
    if (m_cfg.gossipQueue && m_grantCoord != nullptr &&
        m_grantCoord->QueueYields(who, now, m_cfg.arbQueueMaxS))
    {
        ++m_queueYields;
        ++m_arbDenials;
        LogArb(now, who, "yield", u, tDeg, rem, tBatt, tStale, qt, 0);
        return -1;
    }
    // Queued, or re-queued with fresh terms and its original request time (its age).
    bool found = false;
    for (size_t k = 0; k < m_arbQueue.size(); ++k)
    {
        if (m_arbQueue[k].who == who)
        {
            m_arbQueue[k].pid = hdr.GetPacketId();
            m_arbQueue[k].tier = hdr.GetTier();
            m_arbQueue[k].tDeg = tDeg;
            m_arbQueue[k].rem = rem;
            m_arbQueue[k].qt = qt;
            m_arbQueue[k].onBehalf = m_curOnBehalf; // TA-2: the latest request names it
            found = true;
            break;
        }
    }
    if (!found)
    {
        ArbQueued q;
        q.who = who;
        q.pid = hdr.GetPacketId();
        q.tier = hdr.GetTier();
        q.tReq = now;
        q.tDeg = tDeg;
        q.rem = rem;
        q.qt = qt;
        q.onBehalf = m_curOnBehalf; // TA-2
        m_arbQueue.push_back(q);
        ++m_arbQueuedN;
    }
    uint32_t pos = 1;
    for (size_t k = 0; k < m_arbQueue.size(); ++k)
    {
        const ArbQueued& o = m_arbQueue[k];
        if (o.who != who &&
            ScoreV2(o.tDeg, o.rem, std::min(1.0, (now - o.tReq) / 10.0), o.qt, nullptr) > u)
        {
            ++pos;
        }
    }
    *queuePos = static_cast<uint8_t>(std::min<uint32_t>(pos, 255));
    ++m_arbDenials;
    LogArb(now, who, "queue", u, tDeg, rem, tBatt, tStale, qt, pos);
    PublishQueue(now);
    return 0;
}

void
CaTblMac::ArbitrationCycle()
{
    if (!m_cfg.arbitrationV2)
    {
        return;
    }
    if (m_arbQueue.empty())
    {
        if (m_cfg.gossipQueue && m_grantCoord != nullptr && !m_grantCoord->OwnQueue().empty())
        {
            PublishQueue(Simulator::Now().GetSeconds());
        }
        return;
    }
    const double now = Simulator::Now().GetSeconds();
    if (m_cfg.gossipQueue && m_grantCoord != nullptr && m_slotGrantedAtS >= 0.0)
    {
        // NW-6: a slot whose grant this anchor has yielded by gossip is free again.
        m_grantCoord->Expire(now);
        const std::vector<CaTblGrantRecord> own = m_grantCoord->OwnActiveGrants(now);
        bool held = false;
        for (size_t i = 0; i < own.size(); ++i)
        {
            held = held || own[i].targetId == m_slotHolder;
        }
        if (!held && now - m_slotGrantedAtS < m_cfg.arbSlotHoldS)
        {
            m_slotGrantedAtS = -1.0;
        }
    }
    // Drop what has waited too long, or whose requester this arbiter no longer has UWB
    // evidence for -- or (NW-6) that a higher-id anchor queues or has granted -- with a
    // final denial (queue position 0).
    for (size_t k = 0; k < m_arbQueue.size();)
    {
        const ArbQueued& q = m_arbQueue[k];
        std::map<uint16_t, double>::const_iterator evd = m_uwbEvidence.find(q.who);
        const bool inRange =
            evd != m_uwbEvidence.end() && now - evd->second <= m_cfg.arbUwbEvidenceS;
        const bool yields = m_cfg.gossipQueue && m_grantCoord != nullptr &&
                            m_grantCoord->QueueYields(q.who, now, m_cfg.arbQueueMaxS);
        if (yields)
        {
            m_nextQueuePos = 0;
            PrepareReply(q.pid, true, false);
            Send(CA_TBL_MSG_ESC_GRANT, static_cast<CaTblTier>(q.tier), 0, q.who);
            ++m_queueYields;
            LogArb(now, q.who, "yield", 0.0, q.tDeg, q.rem, 0.0, 0.0, q.qt, 0);
            m_arbQueue.erase(m_arbQueue.begin() + static_cast<long>(k));
            continue;
        }
        if (now - q.tReq > m_cfg.arbQueueMaxS || !inRange)
        {
            m_nextQueuePos = 0;
            PrepareReply(q.pid, true, false);
            Send(CA_TBL_MSG_ESC_GRANT, static_cast<CaTblTier>(q.tier), 0, q.who);
            ++m_arbExpired;
            LogArb(now, q.who, "expire", 0.0, q.tDeg, q.rem, 0.0, 0.0, q.qt, 0);
            m_arbQueue.erase(m_arbQueue.begin() + static_cast<long>(k));
            continue;
        }
        ++k;
    }
    const bool slotFree =
        (m_slotGrantedAtS < 0.0) || ((now - m_slotGrantedAtS) >= m_cfg.arbSlotHoldS);
    if (!slotFree || m_arbQueue.empty())
    {
        PublishQueue(now);
        return;
    }
    // "Denied requests remain queued, re-scored next cycle (aging prevents starvation)":
    // staleness here is the time each has waited.
    size_t best = 0;
    double bestU = -1.0;
    for (size_t k = 0; k < m_arbQueue.size(); ++k)
    {
        const ArbQueued& q = m_arbQueue[k];
        const double tStale = std::min(1.0, (now - q.tReq) / 10.0);
        double tBatt = 0.0;
        const double u = ScoreV2(q.tDeg, q.rem, tStale, q.qt, &tBatt);
        LogArb(now, q.who, "score", u, q.tDeg, q.rem, tBatt, tStale, q.qt, 0);
        if (u > bestU || (u == bestU && q.tReq < m_arbQueue[best].tReq))
        {
            best = k;
            bestU = u;
        }
    }
    const ArbQueued w = m_arbQueue[best];
    m_arbQueue.erase(m_arbQueue.begin() + static_cast<long>(best));
    m_slotHolder = w.who;
    m_slotGrantedAtS = now;
    m_slotHolderScore = bestU;
    m_lastGrantAtS[w.who] = now;
    ++m_arbGrants;
    ++m_arbAgedGrants;
    ++m_grantsByTarget[w.who];
    if (m_grantCoord != nullptr)
    {
        m_grantCoord->NoteOwnGrant(w.who, now, now + m_cfg.arbSlotHoldS);
    }
    double tBatt = 0.0;
    ScoreV2(w.tDeg, w.rem, std::min(1.0, (now - w.tReq) / 10.0), w.qt, &tBatt);
    LogArb(now, w.who, "aged_grant", bestU, w.tDeg, w.rem, tBatt,
           std::min(1.0, (now - w.tReq) / 10.0), w.qt, 0);
    m_nextQueuePos = 0;
    m_nextOnBehalf = w.onBehalf; // TA-2
    PrepareReply(w.pid, true, true);
    Send(CA_TBL_MSG_ESC_GRANT, static_cast<CaTblTier>(w.tier), 0, w.who);
    PublishQueue(now);
}

uint32_t
CaTblMac::GossipQueueBytes(const CaTblHeader& h) const
{
    // NW-6: the queue items a gossip carries, billed only under the flag that sends them.
    return (m_cfg.gossipQueue && h.GetMsgType() == CA_TBL_MSG_GRANT_GOSSIP)
               ? CA_TBL_QUEUE_ITEM_BYTES * static_cast<uint32_t>(h.GetQueueItems().size())
               : 0u;
}

void
CaTblMac::PublishQueue(double nowS)
{
    // NW-6: the coordinator is shared by an anchor's radios, so the radio that gossips
    // (tier 1) states the queue this arbiter (2.4 GHz) holds.
    if (!m_cfg.gossipQueue || m_grantCoord == nullptr)
    {
        return;
    }
    std::vector<CaTblQueueRecord> q;
    for (size_t k = 0; k < m_arbQueue.size(); ++k)
    {
        const ArbQueued& e = m_arbQueue[k];
        CaTblQueueRecord r;
        r.anchorId = m_cfg.nodeId;
        r.targetId = e.who;
        const double u = ScoreV2(e.tDeg, e.rem, std::min(1.0, (nowS - e.tReq) / 10.0), e.qt,
                                 nullptr);
        r.uQ = static_cast<uint8_t>(std::min(255.0, std::max(0.0, u / 4.0 * 255.0 + 0.5)));
        r.queuedSinceS = e.tReq;
        q.push_back(r);
    }
    m_grantCoord->SetOwnQueue(q);
}

int
CaTblMac::SubmitRequestOnBehalf(uint16_t target, uint8_t degradationQ, uint8_t remainingQ,
                                uint8_t queryTriggered, CaTblTier tier)
{
    if (!m_cfg.arbitrationV2)
    {
        return -1;
    }
    CaTblHeader h;
    h.SetPacketId(NextPacketId());
    h.SetOriginId(target);
    h.SetMsgType(CA_TBL_MSG_ESC_REQUEST);
    h.SetTier(static_cast<uint8_t>(tier));
    h.SetReqDegradation(degradationQ);
    h.SetReqBattery(remainingQ);
    h.SetReqQueryTriggered(queryTriggered);
    LogArb(Simulator::Now().GetSeconds(), target, "obo_submit", 0.0,
           static_cast<double>(degradationQ) / 255.0, static_cast<double>(remainingQ) / 255.0, 0.0,
           0.0, queryTriggered, 0);
    uint8_t qpos = 0;
    m_curOnBehalf = true;
    const int v = ArbitrateV2(h, &qpos);
    m_curOnBehalf = false;
    if (v == 1)
    {
        m_nextQueuePos = 0;
        m_nextOnBehalf = true;
        PrepareReply(h.GetPacketId(), true, true);
        Send(CA_TBL_MSG_ESC_GRANT, tier, 0, target);
    }
    return v;
}

void
CaTblMac::SendRequestOnBehalf(uint16_t arbiterId, uint16_t subject, uint8_t degradationQ,
                              uint8_t remainingQ, uint8_t queryTriggered, CaTblTier tier)
{
    m_nextReqDegradation = degradationQ;
    m_nextReqBattery = remainingQ;
    m_nextReqSubject = subject;
    m_nextReqSubjectQT = queryTriggered;
    if (!m_silent)
    {
        ++m_oboFramesSent;
    }
    Send(CA_TBL_MSG_ESC_REQUEST, tier, 0, arbiterId);
}

bool
CaTblMac::TakeUrgentQuery()
{
    const bool was = m_urgentQueryPending;
    m_urgentQueryPending = false;
    return was;
}

void
CaTblMac::SendQuery(CaTblTier tier, uint8_t manifestId)
{
    // C3. Identical to Send(QUERY, ...) except that the frame carries WHO is being
    // looked for. A separate entry point because ns-3 Simulator::Schedule binds a
    // member function at exact arity, so widening Send() with a defaulted parameter
    // breaks every scheduled call to it.
    //
    // nEntries = 1: one manifest id. CaTblPayloadBytes(QUERY, n) already bills a
    // query for carrying n manifest ids, so the airtime is charged correctly by the
    // existing sizing and nothing about the wire format changes.
    m_nextQueryManifest = manifestId;
    Send(CA_TBL_MSG_QUERY, tier, 1, 0);
    m_nextQueryManifest = 0;
}

uint32_t
CaTblMac::ManifestOverheadBytes(uint8_t msgType, uint32_t nEntries) const
{
    // Network §1: target_manifest_id has to travel on an entry for a proxy
    // responder to match it. Billed only when the proxy is enabled, because a
    // byte that is not transmitted must not be charged -- and because an unbilled
    // byte here would quietly subsidise the airtime saving measured for §1's
    // reverse path.
    if (!m_cfg.queryProxy)
    {
        return 0;
    }
    if (msgType == CA_TBL_MSG_ENTRY_PUSH || msgType == CA_TBL_MSG_RESPONSE ||
        msgType == CA_TBL_MSG_TRIGGERED_UPDATE)
    {
        return nEntries * CA_TBL_MANIFEST_ID_BYTES;
    }
    return 0;
}

bool
CaTblMac::ProxyAnswerFor(uint8_t manifestId, double nowS, CaTblTier tier, uint32_t queryId,
                         CaTblTableEntry* answered)
{
    if (manifestId == 0)
    {
        return false;
    }
    // L.2.5: filled in as the scan goes and pushed on every exit below -- once per
    // matching entry examined, or once with held=false when nothing matched -- so no
    // outcome can go unrecorded, least of all the refusals, which were the only
    // outcome anyone had seen. A manifest id names one target and the table is keyed
    // by node, so in practice a check yields exactly one record.
    CaTblProxyCheck rec;
    rec.tS = nowS;
    rec.queryId = queryId;
    rec.tier = static_cast<uint8_t>(tier);
    rec.manifestId = manifestId;
    for (std::map<uint16_t, CaTblTableEntry>::const_iterator it = m_table.begin();
         it != m_table.end(); ++it)
    {
        const CaTblTableEntry& e = it->second;
        if (!e.valid || e.nodeId == m_cfg.nodeId ||
            e.targetManifestId != manifestId)
        {
            continue;
        }
        rec.held = true;
        rec.entry = e;
        // FRESHNESS IS THE WHOLE GUARD. A matching entry that is too old is not
        // an answer; reporting it would state a position nothing currently
        // supports. Counted rather than dropped silently, so a search that found
        // only stale knowledge is distinguishable from one that found none.
        // Classified rather than compared, so the refusal reason is nameable. The
        // bound stays exactly where the spec puts it -- agingS defaults to the same
        // 120 s -- so this changes what can be REPORTED, not what is refused.
        CaTblFreshnessBounds fb;
        fb.agingS = m_cfg.queryStalenessBoundS;
        const CaTblFreshness fresh = CaTblClassifyFreshness(nowS - e.timestampS, fb);
        if (fresh == CA_TBL_STALE || fresh == CA_TBL_EXPIRED)
        {
            ++m_proxyRefusedStale;
            m_proxyChecks.push_back(rec);
            continue;
        }
        rec.answered = true;
        m_proxyChecks.push_back(rec);
        if (answered != nullptr)
        {
            *answered = e; // NW-9: the answer carries the entry it is given on
        }
        return true;
    }
    if (!rec.held)
    {
        m_proxyChecks.push_back(rec);
    }
    return false;
}

uint32_t
CaTblMac::RouteOverheadBytes(uint8_t msgType) const
{
    // A RESPONSE's header beyond the 28-byte entry CaTblPayloadBytes already bills, under
    // the flags that carry it -- the same rule as ManifestOverheadBytes.
    // - NW-9 (queryV2): query_id(4) + requester(2) + manifest(1) + tier_source(1) + pcs(1)
    //   + integrity(1) + battery(1) + degradation(1) = 12.
    // - NW-8 (reversePathHop): query_id(4) + requester(2) + answerer(2) = 8, or only the
    //   answerer's 2 when queryV2 already bills the other six.
    if (msgType != CA_TBL_MSG_RESPONSE)
    {
        return 0u;
    }
    return (m_cfg.queryV2 ? 12u : 0u) + (m_cfg.reversePathHop ? (m_cfg.queryV2 ? 2u : 8u) : 0u);
}

uint32_t
CaTblMac::CancelPendingRequests()
{
    uint32_t n = 0;
    for (std::map<uint32_t, PendingFrame>::iterator it = m_awaitingAck.begin();
         it != m_awaitingAck.end();)
    {
        if (it->second.hdr.GetMsgType() == CA_TBL_MSG_ESC_REQUEST)
        {
            it = m_awaitingAck.erase(it);
            ++n;
        }
        else
        {
            ++it;
        }
    }
    return n;
}

bool
CaTblMac::RpFirstCopy(uint32_t queryId, uint16_t answererId, uint8_t manifest, double nowS)
{
    const std::tuple<uint32_t, uint16_t, uint8_t> key(queryId, answererId, manifest);
    std::map<std::tuple<uint32_t, uint16_t, uint8_t>, double>::iterator it = m_rpSeen.find(key);
    // The dedup window, not the reverse-path one: this is duplicate suppression, and a
    // short path window must not let a retransmitted copy through as new.
    if (it != m_rpSeen.end() && (nowS - it->second) < m_cfg.dedupWindowS)
    {
        ++m_rpDuplicates;
        return false;
    }
    if (it == m_rpSeen.end() && m_rpSeen.size() >= m_cfg.reversePathMaxEntries)
    {
        std::map<std::tuple<uint32_t, uint16_t, uint8_t>, double>::iterator oldest =
            m_rpSeen.begin();
        for (std::map<std::tuple<uint32_t, uint16_t, uint8_t>, double>::iterator o =
                 m_rpSeen.begin();
             o != m_rpSeen.end(); ++o)
        {
            if (o->second < oldest->second)
            {
                oldest = o;
            }
        }
        m_rpSeen.erase(oldest);
    }
    m_rpSeen[key] = nowS;
    return true;
}

void
CaTblMac::RememberReversePath(uint32_t queryId, uint16_t fromId, double nowS)
{
    // Bounded and time-windowed, same discipline as the dedup cache. Evict the
    // oldest rather than refusing to record: a fresh query is more useful to
    // remember than an old one, and the flood fallback covers what is lost.
    if (m_reversePath.size() >= m_cfg.reversePathMaxEntries)
    {
        std::map<uint32_t, std::pair<uint16_t, double> >::iterator oldest =
            m_reversePath.begin();
        for (std::map<uint32_t, std::pair<uint16_t, double> >::iterator it =
                 m_reversePath.begin();
             it != m_reversePath.end(); ++it)
        {
            if (it->second.second < oldest->second.second)
            {
                oldest = it;
            }
        }
        m_reversePath.erase(oldest);
    }
    m_reversePath[queryId] = std::make_pair(fromId, nowS);
}

void
CaTblMac::SendResponse(CaTblTier tier, uint32_t queryId, uint16_t requesterId,
                       uint16_t answererId)
{
    // Transport §1. Unicast home along the cached reverse path; flood only when
    // there is no path to unicast along. The fallback is what makes this safe to
    // enable: a cache miss costs airtime, never the answer.
    if (m_cfg.reversePathHop || m_cfg.queryV2)
    {
        // NW-8 / NW-9: the answer names its query, its requester and its answerer.
        m_nextRespQueryId = queryId;
        m_nextRespRequester = requesterId;
        m_nextRespAnswerer = (answererId != 0) ? answererId : m_cfg.nodeId;
    }
    if (m_cfg.reversePath && m_cfg.reversePathHop)
    {
        // NW-8. The cache holds the PREVIOUS HOP, so this is one hop of the path home;
        // the node it reaches does the same with its own entry. An entry older than the
        // window is not a path: the neighbour it names may have moved or slept since.
        const double nowS = Simulator::Now().GetSeconds();
        std::map<uint32_t, std::pair<uint16_t, double> >::const_iterator it =
            m_reversePath.find(queryId);
        const bool fresh =
            it != m_reversePath.end() && (nowS - it->second.second) <= m_cfg.reversePathWindowS;
        if (it != m_reversePath.end() && !fresh)
        {
            ++m_rpExpired;
        }
        if (fresh)
        {
            ++m_reversePathHits;
            ++m_responsesUnicast;
            ++m_rpUnicastHops;
            Send(CA_TBL_MSG_RESPONSE, tier, 1, it->second.first);
            return;
        }
        // Transport sect 1, "fall back to flood": a real flood, with the default hop
        // limit -- the QUERY's remaining limit h cannot carry an answer back over the
        // H - h hops the query travelled whenever h < H - h.
        ++m_reversePathMisses;
        ++m_responsesFlooded;
        ++m_rpFloods;
        m_nextForceFlood = true;
        Send(CA_TBL_MSG_RESPONSE, tier, 1, 0);
        return;
    }
    if (m_cfg.reversePath)
    {
        std::map<uint32_t, std::pair<uint16_t, double> >::const_iterator it =
            m_reversePath.find(queryId);
        if (it != m_reversePath.end())
        {
            ++m_reversePathHits;
            ++m_responsesUnicast;
            Send(CA_TBL_MSG_RESPONSE, tier, 1, it->second.first);
            return;
        }
        ++m_reversePathMisses;
    }
    ++m_responsesFlooded;
    Send(CA_TBL_MSG_RESPONSE, tier, 1, requesterId);
}

void
CaTblMac::SendGrantGossip(CaTblTier tier)
{
    if (m_grantCoord == nullptr)
    {
        return;
    }
    const double nowS = Simulator::Now().GetSeconds();
    m_grantCoord->Expire(nowS);
    const std::vector<CaTblGrantRecord> own = m_grantCoord->OwnActiveGrants(nowS);
    // NW-6: the queue rides too, and a gossip with only a queue is still worth sending.
    m_nextGossipQueue.clear();
    if (m_cfg.gossipQueue)
    {
        const std::vector<CaTblQueueRecord>& q = m_grantCoord->OwnQueue();
        for (size_t i = 0; i < q.size(); ++i)
        {
            CaTblQueueGossipItem it;
            it.targetId = q[i].targetId;
            it.uQ = q[i].uQ;
            it.queuedSinceS = q[i].queuedSinceS;
            m_nextGossipQueue.push_back(it);
        }
    }
    if (own.empty() && m_nextGossipQueue.empty())
    {
        ++m_gossipSkippedEmpty;
        return;
    }
    m_queueItemsSent += m_nextGossipQueue.size();
    m_nextGossip.clear();
    for (size_t i = 0; i < own.size(); ++i)
    {
        CaTblGrantItem gi;
        gi.targetId = own[i].targetId;
        gi.startS = own[i].startS;
        gi.endS = own[i].endS;
        m_nextGossip.push_back(gi);
    }
    // Send() copies the list into the frame synchronously; clearing afterwards keeps a
    // stale list from riding any later frame, the same discipline as SetRequestTerms().
    Send(CA_TBL_MSG_GRANT_GOSSIP, tier, static_cast<uint32_t>(m_nextGossip.size()), 0);
    m_nextGossip.clear();
    m_nextGossipQueue.clear();
}

void
CaTblMac::SendTriggeredUpdate(CaTblTier tier, uint8_t reason)
{
    m_nextTriggerReason = reason;
    Send(CA_TBL_MSG_TRIGGERED_UPDATE, tier, 1, 0);
}

double
CaTblMac::FrameAirtimeS(CaTblMsgType type, CaTblTier tier, uint32_t nEntries) const
{
    const uint32_t payload = CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(type, nEntries) +
                             ManifestOverheadBytes(type, nEntries) + RouteOverheadBytes(type);
    return (tier == CA_TBL_TIER_UWB)
               ? CaTblEnergyModel::UwbFrameTimeS(payload)
               : CaTblEnergyModel::LoRaTimeOnAirS(m_cfg.sf, m_cfg.bandwidthHz, payload,
                                                  m_cfg.codingRate);
}

void
CaTblMac::PrepareReply(uint32_t ackedPacketId, bool isGrant, bool granted)
{
    if (!m_cfg.handshakeFixes)
    {
        return;
    }
    m_nextAckedPacketId = ackedPacketId;
    if (isGrant)
    {
        m_nextHasVerdict = true;
        m_nextGranted = granted;
        if (granted)
        {
            ++m_hsGrantsSent;
        }
        else
        {
            ++m_hsDeniesSent;
        }
    }
}

void
CaTblMac::LogGrantVerdict(uint16_t targetId,
                          uint16_t otherId,
                          const char* source,
                          CaTblGrantVerdict v,
                          const char* action,
                          double startS,
                          double endS) const
{
    if (m_grantLog == nullptr)
    {
        return;
    }
    *m_grantLog << std::fixed << std::setprecision(6) << Simulator::Now().GetSeconds() << ','
                << m_cfg.nodeId << ',' << targetId << ',' << otherId << ',' << source << ','
                << CaTblGrantCoordinator::VerdictName(v) << ',' << action << ',' << startS
                << ',' << endS << '\n';
}

void
CaTblMac::Send(CaTblMsgType type, CaTblTier tier, uint32_t nEntries, uint16_t destId)
{
    // sect 49: a silenced radio consumes the call and emits nothing. Counted, because a
    // fault that leaves no trace is indistinguishable from a quiet period.
    if (m_silent)
    {
        ++m_suppressedTx;
        m_nextTriggerReason = 0; // MC-2: 0 unless SendTriggeredUpdate set it
        m_nextRespQueryId = 0;   // NW-8, the same one-frame discipline
        m_nextRespRequester = 0;
        m_nextRespAnswerer = 0;
        m_nextForceFlood = false;
        m_nextHasRespEntry = false; // NW-9
        m_nextRespManifest = 0;
        m_nextReqQT = 0; // NW-4 / NW-5
        m_nextReqSubject = 0; // N.10
        m_nextReqSubjectQT = 0;
        m_nextQueuePos = 0;
        if (m_cfg.handshakeFixes)
        {
            // MC-1: what this frame would have answered must not ride a later one.
            m_nextReqTargetTier = 0xFF;
            m_nextAckedPacketId = 0;
            m_nextHasVerdict = false;
            m_nextGranted = true;
        }
        return;
    }

    PendingFrame f;
    f.hdr.SetPacketId(NextPacketId());
    // The anti-entropy exchange the design specifies: an ENTRY_PUSH carries the
    // sender's own entry. Attached only when this node has actually published one, so
    // a node with nothing to say sends nothing and is not billed as though it had.
    if (type == CA_TBL_MSG_ENTRY_PUSH || type == CA_TBL_MSG_TRIGGERED_UPDATE)
    {
        // An anti-entropy answer carries the entry the ASKER was missing, which is
        // usually not this node's own. m_nextPushEntry names it for exactly one send.
        if (m_nextPushEntry != nullptr)
        {
            f.hdr.SetSelfEntry(*m_nextPushEntry);
        }
        else
        {
            std::map<uint16_t, CaTblTableEntry>::const_iterator se =
                m_table.find(m_cfg.nodeId);
            if (se != m_table.end() && se->second.valid)
            {
                f.hdr.SetSelfEntry(se->second);
            }
        }
    }
    else if (type == CA_TBL_MSG_DISCOVERY && g_beaconCarriesEntry)
    {
        // --beaconCarriesEntry: the beacon carries this node's own entry, when it has published one.
        std::map<uint16_t, CaTblTableEntry>::const_iterator se = m_table.find(m_cfg.nodeId);
        if (se != m_table.end() && se->second.valid)
        {
            f.hdr.SetSelfEntry(se->second);
        }
    }
    else if (type == CA_TBL_MSG_DIGEST && m_cfg.digestSummary)
    {
        // "I hold version N of peer X", for the most recently heard peers. Bounded,
        // because a digest is only cheap while it stays a summary.
        std::vector<CaTblDigestItem> d;
        for (std::map<uint16_t, CaTblTableEntry>::const_iterator it = m_table.begin();
             it != m_table.end() && d.size() < CA_TBL_DIGEST_MAX_ITEMS; ++it)
        {
            if (it->second.valid)
            {
                CaTblDigestItem di;
                di.nodeId = it->second.nodeId;
                di.seqNo = it->second.seqNo;
                d.push_back(di);
            }
        }
        f.hdr.SetDigest(d);
        f.nEntries = static_cast<uint32_t>(d.empty() ? 1 : d.size());
        // MAC sect 2: what pushing the whole table instead would have cost.
        uint32_t nValid = 0;
        for (std::map<uint16_t, CaTblTableEntry>::const_iterator it = m_table.begin();
             it != m_table.end(); ++it)
        {
            nValid += it->second.valid ? 1 : 0;
        }
        f.cfFullPushS = (nValid > 0) ? FrameAirtimeS(CA_TBL_MSG_ENTRY_PUSH, tier, nValid) : 0.0;
    }
    else if (type == CA_TBL_MSG_GRANT_GOSSIP)
    {
        f.hdr.SetGrants(m_nextGossip);
        if (m_cfg.gossipQueue)
        {
            f.hdr.SetQueueItems(m_nextGossipQueue); // NW-6
        }
    }
    f.hdr.SetOriginId(m_cfg.nodeId);
    // NW-0: a one-hop DISCOVERY leaves with hop limit 0. A limit of 1 would still be
    // relayed once -- DecrementHopLimit() refuses only at 0 -- which is two hops.
    // NW-8: the fallback RESPONSE floods with the flooded types' limit.
    const bool forceFlood = (type == CA_TBL_MSG_RESPONSE) && m_nextForceFlood;
    f.hdr.SetHopLimit((m_cfg.discoveryOneHop && type == CA_TBL_MSG_DISCOVERY)
                          ? 0
                          : ((CaTblIsFlooded(type) || forceFlood) ? m_cfg.defaultHopLimit : 1));
    if (type == CA_TBL_MSG_RESPONSE && (m_cfg.reversePathHop || m_cfg.queryV2))
    {
        f.hdr.SetRespRoute(m_nextRespQueryId, m_nextRespRequester, m_nextRespAnswerer,
                           forceFlood);
    }
    if (m_cfg.queryV2)
    {
        // NW-9. A QUERY carries its id (its packet id), requester, priority and list; a
        // RESPONSE the entry it answers with and the manifest. Consumed here, one frame.
        if (type == CA_TBL_MSG_QUERY && m_nextV2)
        {
            f.hdr.SetQueryV2(f.hdr.GetPacketId(), m_cfg.nodeId, m_nextV2Priority,
                             m_nextV2Manifests);
        }
        if (type == CA_TBL_MSG_RESPONSE)
        {
            if (m_nextHasRespEntry)
            {
                f.hdr.SetSelfEntry(m_nextRespEntry);
            }
            f.hdr.SetRespManifest(m_nextRespManifest);
        }
    }
    m_nextHasRespEntry = false;
    m_nextRespManifest = 0;
    m_nextRespQueryId = 0;
    m_nextRespRequester = 0;
    m_nextRespAnswerer = 0;
    m_nextForceFlood = false;
    f.hdr.SetInitialHopLimit(f.hdr.GetHopLimit());
    f.hdr.SetMsgType(type);
    f.hdr.SetTier(static_cast<uint8_t>(tier));
    f.hdr.SetQueryManifestId(m_nextQueryManifest);
    f.hdr.SetReqDegradation(m_nextReqDegradation);
    f.hdr.SetReqBattery(m_nextReqBattery);
    // Cleared immediately: these describe ONE request, and letting them persist
    // would attach a stale degradation figure to the next unrelated frame.
    m_nextReqDegradation = 0;
    m_nextReqBattery = 0;
    if (m_cfg.handshakeFixes)
    {
        // MC-1, on the same one-frame discipline.
        f.hdr.SetReqTargetTier(m_nextReqTargetTier);
        f.hdr.SetAckedPacketId(m_nextAckedPacketId);
        if (m_nextHasVerdict)
        {
            f.hdr.SetGrantVerdict(m_nextGranted, m_nextQueuePos); // NW-5: 0 outside V2
        }
        m_nextReqTargetTier = 0xFF;
        m_nextAckedPacketId = 0;
        m_nextHasVerdict = false;
        m_nextGranted = true;
    }
    m_nextQueuePos = 0;
    if (type == CA_TBL_MSG_ESC_GRANT && m_nextOnBehalf)
    {
        f.hdr.SetGrantOnBehalf(true); // TA-2
    }
    m_nextOnBehalf = false;
    if (type == CA_TBL_MSG_ESC_REQUEST && m_cfg.queryTriggeredEscalation)
    {
        f.hdr.SetReqQueryTriggered(m_nextReqQT); // NW-4
    }
    m_nextReqQT = 0;
    if (type == CA_TBL_MSG_ESC_REQUEST && m_nextReqSubject != 0)
    {
        f.hdr.SetReqSubject(m_nextReqSubject); // N.10: the subject's request, not ours
        f.hdr.SetReqQueryTriggered(m_nextReqSubjectQT);
    }
    m_nextReqSubject = 0;
    m_nextReqSubjectQT = 0;
    f.tier = tier;
    f.nEntries = nEntries;
    f.destId = destId;
    f.cadAttempts = 0;
    f.txAttempts = 0;
    // §2.8: the urgent class substitutes redundancy for a round-trip.
    f.repeatsLeft = (CaTblAckPolicyFor(type) == CA_TBL_ACK_REDUNDANT)
                        ? m_cfg.triggeredUpdateRepeats
                        : 1;
    if (m_nextTriggerReason != 0)
    {
        // MC-2. Set only by SendTriggeredUpdate, for this one frame. A last gasp is
        // unrepeated: see SendTriggeredUpdate.
        f.hdr.SetTriggerReason(m_nextTriggerReason);
        if (m_nextTriggerReason == 2)
        {
            f.repeatsLeft = 1;
        }
        m_nextTriggerReason = 0;
    }
    f.enqueuedAt = Simulator::Now();

    // Its own packet_id goes in the dedup cache immediately, so a flooded frame
    // that loops back is not re-forwarded by its originator.
    m_dedupSeen[f.hdr.GetPacketId()] = Simulator::Now();
    m_dedupOrder.push_back(std::make_pair(f.hdr.GetPacketId(), Simulator::Now()));

    AttemptCad(f);
}

void
CaTblMac::AttemptCad(PendingFrame f)
{
    if (m_asleep && f.hdr.GetOriginId() != m_cfg.nodeId)
    {
        return; // NW-0: a relay queued before the radio fell asleep does not leave
    }
    if (m_cfg.silenceStopsRelay && m_silent && f.hdr.GetOriginId() != m_cfg.nodeId)
    {
        // N.1: a silenced radio relays nothing, new or already queued; its own frames
        // (the last gasp) are Send()'s business and are not refused here.
        ++m_silentRelayRefused;
        return;
    }
    if (m_channel == nullptr)
    {
        return;
    }

    const uint32_t payload = CA_TBL_MAC_HEADER_BYTES +
                             CaTblPayloadBytes(f.hdr.GetMsgType(), f.nEntries) +
                             ManifestOverheadBytes(f.hdr.GetMsgType(), f.nEntries) +
        RouteOverheadBytes(f.hdr.GetMsgType()) + GossipQueueBytes(f.hdr) +
        BeaconEntryBytes(f.hdr);
    const Time toa =
        (f.tier == CA_TBL_TIER_UWB)
            ? Seconds(CaTblEnergyModel::UwbFrameTimeS(payload))
            : Seconds(CaTblEnergyModel::LoRaTimeOnAirS(m_cfg.sf,
                                                       m_cfg.bandwidthHz,
                                                       payload,
                                                       m_cfg.codingRate));

    // Regulatory gate comes BEFORE contention. A frame that would breach the duty
    // cycle is not deferred into a backoff loop, it is refused: the cap is a legal
    // ceiling on airtime, and burning CAD probes trying to sneak under it would be
    // both non-compliant in spirit and a misleading energy number.
    if (WouldViolateDutyCycle(f.tier, toa))
    {
        ++m_dutyViolationAttempts;
        ++Counters(f.hdr.GetMsgType()).droppedDutyCycle;
        if (m_energy != nullptr)
        {
            m_energy->NoteDeferred(f.tier);
        }
        if (!m_dropCb.IsNull())
        {
            m_dropCb(m_cfg.nodeId, f.hdr.GetMsgType(), "duty");
        }
        return;
    }

    // UWB uses randomized-wait TWR initiation, not CAD-CSMA (§2.5). Short range
    // means few simultaneous contenders and TWR is a directed 3-message exchange,
    // so borrowing Poucet's randomized wait is sufficient.
    if (f.tier == CA_TBL_TIER_UWB)
    {
        const double wait = (m_rng != nullptr) ? m_rng->GetValue(0.0, 0.010) : 0.0;
        Simulator::Schedule(Seconds(wait), &CaTblMac::DoTransmit, this, f);
        return;
    }

    if (m_energy != nullptr)
    {
        m_energy->AccountCad(f.tier);
    }

    if (!m_channel->IsBusy(m_cfg.nodeId, f.tier))
    {
        Simulator::Schedule(Seconds(m_cfg.cadDurationS), &CaTblMac::DoTransmit, this, f);
        return;
    }

    ++Counters(f.hdr.GetMsgType()).cadBusyEvents;

    if (f.cadAttempts >= m_cfg.maxCadRetries)
    {
        ++Counters(f.hdr.GetMsgType()).droppedCadExhausted;
        if (!m_dropCb.IsNull())
        {
            m_dropCb(m_cfg.nodeId, f.hdr.GetMsgType(), "cad");
        }
        return;
    }

    // ---- Priority-differentiated backoff (§2.3.1) -------------------------
    // This is the only place the access category has any effect, and therefore
    // the only place the §2.3.1 design is actually being tested. A smaller CW
    // yields less expected backoff, so an URGENT frame statistically wins the
    // channel against a LOW one under load. Whether that translates into a
    // measurable latency separation is an empirical question -- report the
    // per-class access delay, do not assert it.
    const CaTblAccessCategory ac = f.hdr.GetAccessCategory();
    const CaTblCwBounds cw = CaTblCwFor(ac);

    // Binary exponential growth within the category's bounds, capped at cwMax.
    uint32_t span = cw.cwMin + ((cw.cwMax - cw.cwMin + 1) << f.cadAttempts) - 1;
    span = std::min(span, cw.cwMax);

    const double slots = (m_rng != nullptr) ? m_rng->GetValue(0.0, static_cast<double>(span) + 1.0)
                                           : static_cast<double>(span) * 0.5;
    const double backoff = slots * m_cfg.slotTimeS;

    Counters(f.hdr.GetMsgType()).totalBackoffS += backoff;
    ++f.cadAttempts;

    Simulator::Schedule(Seconds(backoff + m_cfg.cadDurationS), &CaTblMac::AttemptCad, this, f);
}

void
CaTblMac::DoTransmit(PendingFrame f)
{
    if (m_channel == nullptr)
    {
        return;
    }
    // N.6. This radio's own previous frame is still on air -- or, per chip, the other
    // band's frame on the same chip.
    Time busyUntil = m_txBusyUntil;
    if (m_chipSibling != nullptr && m_chipSibling->TxBusyUntil() > busyUntil)
    {
        busyUntil = m_chipSibling->TxBusyUntil();
    }
    if (m_cfg.halfDuplex != 0 && Simulator::Now() < busyUntil)
    {
        if (Simulator::Now() < m_txBusyUntil)
        {
            ++m_selfOverlapTx;
        }
        else
        {
            ++m_chipOverlapTx;
        }
        if (m_cfg.halfDuplex == 2)
        {
            // Half-duplex: wait for the frame to end, then contend again (a fresh CAD).
            ++m_hdDeferred;
            Simulator::Schedule(busyUntil - Simulator::Now(), &CaTblMac::AttemptCad, this, f);
            return;
        }
    }

    const uint32_t payload =
        CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(f.hdr.GetMsgType(), f.nEntries) +
        ManifestOverheadBytes(f.hdr.GetMsgType(), f.nEntries) +
        RouteOverheadBytes(f.hdr.GetMsgType()) + GossipQueueBytes(f.hdr) +
        BeaconEntryBytes(f.hdr);
    const Time toa =
        (f.tier == CA_TBL_TIER_UWB)
            ? Seconds(CaTblEnergyModel::UwbFrameTimeS(payload))
            : Seconds(CaTblEnergyModel::LoRaTimeOnAirS(m_cfg.sf,
                                                       m_cfg.bandwidthHz,
                                                       payload,
                                                       m_cfg.codingRate));

    RecordAirtime(f.tier, toa);
    CaTblMsgCounters& c = Counters(f.hdr.GetMsgType());
    ++c.sent;
    if (f.hdr.GetTriggerReason() != 0 && f.hdr.GetOriginId() == m_cfg.nodeId)
    {
        ++m_trigFramesSent; // MC-2
    }
    else if (f.hdr.GetMsgType() == CA_TBL_MSG_TRIGGERED_UPDATE &&
             f.hdr.GetOriginId() == m_cfg.nodeId)
    {
        ++m_recedingSent; // M.9: the only own TRIGGERED_UPDATE without an MC-2 reason
    }
    c.airtimeS += toa.GetSeconds();
    if (f.hdr.GetMsgType() == CA_TBL_MSG_DIGEST && f.hdr.GetOriginId() == m_cfg.nodeId)
    {
        m_digestFullPushCfS += f.cfFullPushS; // MAC sect 2, report only
    }
    if (f.hdr.GetOriginId() != m_cfg.nodeId)
    {
        ++c.rebroadcast;
        m_rebroadcastAirS += toa.GetSeconds(); // NW-0
        ++m_rebroadcastFrames;
    }

    m_channel->Transmit(m_cfg.nodeId, f.tier, f.hdr, toa, m_txPowerDbm, f.destId);
    if (Simulator::Now() + toa > m_txBusyUntil)
    {
        m_txBusyUntil = Simulator::Now() + toa; // N.6: read only under halfDuplex
    }
    if (m_cfg.halfDuplex != 0)
    {
        m_ownTx.emplace_back(Simulator::Now().GetSeconds(), (Simulator::Now() + toa).GetSeconds());
        if (m_ownTx.size() > 16)
        {
            m_ownTx.pop_front();
        }
    }

    // Explicit ACK (§2.8): only the escalation exchange and an own-entry QUERY
    // response. Arm a timeout so a lost grant surfaces as a retransmission
    // rather than as a node silently believing it holds a UWB slot.
    if (CaTblAckPolicyFor(f.hdr.GetMsgType()) == CA_TBL_ACK_EXPLICIT &&
        f.hdr.GetMsgType() != CA_TBL_MSG_ACK && !f.hdr.GetForceFlood())
    {
        m_awaitingAck[f.hdr.GetPacketId()] = f;
        Simulator::Schedule(Seconds(m_cfg.ackTimeoutS),
                            &CaTblMac::OnAckTimeout,
                            this,
                            f.hdr.GetPacketId());
    }

    // Redundant class: repeat with jitter instead of waiting for an ACK.
    if (f.repeatsLeft > 1)
    {
        PendingFrame again = f;
        --again.repeatsLeft;
        again.cadAttempts = 0;
        const double jitter =
            (m_rng != nullptr) ? m_rng->GetValue(0.0, m_cfg.rebroadcastJitterMaxS) : 0.05;
        Simulator::Schedule(toa + Seconds(jitter), &CaTblMac::AttemptCad, this, again);
    }
}

void
CaTblMac::OnAckTimeout(uint32_t packetId)
{
    std::map<uint32_t, PendingFrame>::iterator it = m_awaitingAck.find(packetId);
    if (it == m_awaitingAck.end())
    {
        return; // ACKed in time
    }

    PendingFrame f = it->second;
    m_awaitingAck.erase(it);
    ++Counters(f.hdr.GetMsgType()).ackTimeouts;

    if (f.txAttempts >= m_cfg.maxRetransmissions)
    {
        if (!m_dropCb.IsNull())
        {
            m_dropCb(m_cfg.nodeId, f.hdr.GetMsgType(), "retx");
        }
        if (m_uwbGrantCb && f.hdr.GetMsgType() == CA_TBL_MSG_ESC_REQUEST &&
            f.hdr.GetReqSubject() == 0) // N.10: a subject's request is not this node's
        {
            m_uwbGrantCb(0, -1); // NW-7: the request ran out of retries unanswered
        }
        return;
    }

    ++f.txAttempts;
    f.cadAttempts = 0;
    // A retransmission needs a fresh packet_id or every receiver's dedup cache
    // will discard it as already-seen. This is a real interaction between §2.6
    // and §2.8 that is easy to get wrong and produces a protocol that cannot
    // retransmit anything.
    f.hdr.SetPacketId(NextPacketId());
    ++Counters(f.hdr.GetMsgType()).retransmissions;
    AttemptCad(f);
}

// ---- receive path (§2.6) --------------------------------------------------

void
CaTblMac::DeliverRx(const CaTblHeader& hdr, const CaTblLinkOutcome& out, uint16_t txId)
{
    if (m_asleep)
    {
        return; // NW-0: the channel skips a sleeping radio (and counts it) before this
    }
    const uint32_t pid = hdr.GetPacketId();
    CaTblMsgCounters& c = Counters(hdr.GetMsgType());

    // Dedup: bounded LRU with a time window.
    const double now = Simulator::Now().GetSeconds();
    std::map<uint32_t, Time>::iterator seen = m_dedupSeen.find(pid);
    if (seen != m_dedupSeen.end() && (now - seen->second.GetSeconds()) < m_cfg.dedupWindowS)
    {
        ++c.droppedDedup;
        return;
    }

    m_dedupSeen[pid] = Simulator::Now();
    m_dedupOrder.push_back(std::make_pair(pid, Simulator::Now()));
    while (m_dedupOrder.size() > m_cfg.dedupCacheEntries)
    {
        // Bounded RAM is the reason this cache is finite. Evicting the oldest can
        // in principle let a slow multi-hop copy of an already-forwarded frame
        // look new again -- that is the documented trade-off of the sizing
        // parameter, and it is why dedupCacheEntries is a reported design value
        // rather than an arbitrary constant.
        m_dedupSeen.erase(m_dedupOrder.front().first);
        m_dedupOrder.pop_front();
    }

    ++c.received;
    if (hdr.GetMsgType() == CA_TBL_MSG_TRIGGERED_UPDATE && hdr.GetTriggerReason() != 0 &&
        hdr.GetTriggerReason() < 3)
    {
        ++m_trigHeard[hdr.GetTriggerReason()]; // MC-2
    }

    // Before the flood-forwarding branch below, which is where a relayed copy
    // would otherwise become indistinguishable from a direct one.
    NoteNeighbour(hdr, out);

    // Anti-entropy merge. Accepted from ANY hop, unlike the neighbour table above --
    // knowing where a node is does not imply being able to reach it, and only the
    // neighbour table feeds the poll set, so admitting a far-away entry here costs
    // nothing in airtime. Strictly superseded: an older copy of something already held
    // is dropped rather than overwriting a newer one, which is what the modular
    // sequence comparison exists for.
    if (g_beaconCarriesEntry && !m_mergesBeaconEntries && hdr.GetMsgType() == CA_TBL_MSG_DISCOVERY &&
        hdr.HasSelfEntry() && hdr.GetSelfEntry().valid)
    {
        ++m_beaconIgnored;
    }
    if (hdr.HasSelfEntry() &&
        !(hdr.GetMsgType() == CA_TBL_MSG_DISCOVERY && !(g_beaconCarriesEntry && m_mergesBeaconEntries)))
    {
        const CaTblTableEntry& in = hdr.GetSelfEntry();
        if (hdr.GetMsgType() == CA_TBL_MSG_DISCOVERY && in.valid && in.nodeId != m_cfg.nodeId)
        {
            ++m_beaconMerges;
        }
        if (in.valid && in.nodeId != m_cfg.nodeId)
        {
            std::map<uint16_t, CaTblTableEntry>::iterator ex = m_table.find(in.nodeId);
            // THE SPEC'S RULE HAS THREE BRANCHES AND THIS HAD TWO. Newer supersedes,
            // older is discarded, and EQUAL was silently falling into the discard
            // path. Equal is the interesting case: the same report arriving by a
            // second mesh route, which is independent evidence about the same instant
            // from a path that saw different NLOS.
            if (ex == m_table.end() || CaTblSeqNewer(in.seqNo, ex->second.seqNo))
            {
                CaTblTableEntry e = in;
                // One more hop than the sender recorded, saturating rather than
                // wrapping: a hop count that rolled over would read as a direct entry.
                e.hopCount =
                    (in.hopCount < 255) ? static_cast<uint8_t>(in.hopCount + 1) : 255;
                m_table[in.nodeId] = e;
                ++m_tableMerges;
            }
            else if (m_cfg.tableFuseCI && in.seqNo == ex->second.seqNo)
            {
                // Covariance Intersection, because the two copies share an upstream
                // relay that cannot be identified from the entry. A Kalman fuse here
                // would assume independence and return a covariance tighter than
                // either input -- the double-counting failure that makes a mesh
                // confidently wrong.
                CaTblTableEntry fused;
                if (CaTblCovarianceIntersect(ex->second, in, fused))
                {
                    fused.hopCount = (ex->second.hopCount <= in.hopCount)
                                         ? ex->second.hopCount
                                         : static_cast<uint8_t>(
                                               (in.hopCount < 255) ? in.hopCount + 1 : 255);
                    m_table[in.nodeId] = fused;
                    ++m_tableFuses;
                }
                else
                {
                    // A fuse that cannot be computed must not silently keep one side
                    // and call it a fuse.
                    ++m_tableDiscards;
                }
            }
            else
            {
                ++m_tableDiscards;
            }
        }
    }

    // APC is fed on the link-check path (see UpdateApc). Anchor gossip is not that path:
    // at a gateway its margin is an INTER-GATEWAY link's, and letting it steer the one
    // per-radio power state would let the gossip set the beacon power the field node
    // sees. At a field node it is an overheard broadcast, not a link check either.
    if (hdr.GetMsgType() != CA_TBL_MSG_GRANT_GOSSIP)
    {
        UpdateApc(out.rxPowerDbm, static_cast<CaTblTier>(hdr.GetTier()));
    }

    if (!m_rxCb.IsNull())
    {
        m_rxCb(m_cfg.nodeId, hdr, out);
    }

    // Explicit ACK obligation.
    if (CaTblAckPolicyFor(hdr.GetMsgType()) == CA_TBL_ACK_EXPLICIT &&
        hdr.GetMsgType() != CA_TBL_MSG_ACK && !hdr.GetForceFlood())
    {
        if (m_cfg.handshakeFixes && hdr.GetMsgType() == CA_TBL_MSG_ESC_REQUEST &&
            hdr.GetReqSubject() == 0) // N.10: a subject's request is ACKed to its carrier
        {
            // MC-1. No bare ACK for a request. An ACK sent here, before the arbiter below
            // has run, told the requester "settled" whatever the arbiter then decided, so
            // a denied request was acknowledged and never retried. An anchor answers
            // with the GRANT below, after arbitrating; a node that is not an arbiter does
            // not answer a request at all.
            ++m_hsRequestAcksSkipped;
        }
        else
        {
            ++Counters(CA_TBL_MSG_ACK).acksSent;
            PrepareReply(pid, false, true);
            Send(CA_TBL_MSG_ACK, static_cast<CaTblTier>(hdr.GetTier()), 0, hdr.GetOriginId());
        }
    }

    // ---- C1: request/response semantics (default off) ---------------------
    //
    // Until this existed the MAC counted and charged per message type but
    // implemented NO reply logic, so five of the nine types were defined in the
    // header and never constructed on the live path. Two are constructed here.
    //
    // GRANT/DENY IS NOT A SEPARATE DECISION. The taxonomy has no deny type, and
    // ca-tbl-header.cc is deliberately not being changed -- its sizes, access
    // categories and ACK policies are already correct for all nine types and the
    // validator covers them 38/38. So the anchor always ATTEMPTS the grant, and
    // whether it reaches the requester is decided by CAD and the duty governor,
    // which is the real constraint: a denial is a grant the regulation would not
    // let through. That is the coupling C1 exists to expose -- under the
    // accounting path the handshake never contended for anything. The requester
    // surfaces the failure through its existing ACK timeout.
    // ---- C3: identity-scoped search, independent of the handshake flag -----
    //
    // NOT nested in the sendHandshakeFrames guard below. They are unrelated
    // mechanisms, and nesting them meant a query was only answered when the
    // handshake flag happened to be on -- which is exactly how the first two-sided
    // test came back showing no queries reaching the measured tag at all.
    //
    // THE SCOPING IS AT THE RESPONDER and it is the whole mechanism: a query floods,
    // every target hears it, and only the one whose manifest id matches answers.
    // Without the id every target answers every query and the search is a broadcast
    // with extra steps. Anchors do not answer -- they issue.
    // NW-9: what this node does with a v2 QUERY's list, read by the flood branch below.
    bool v2Handled = false;
    std::vector<uint8_t> v2Left;
    if (hdr.GetMsgType() == CA_TBL_MSG_QUERY)
    {
        const CaTblTier qTier = static_cast<CaTblTier>(hdr.GetTier());
        const double nowS = Simulator::Now().GetSeconds();
        // Transport §1: remember where this query came from BEFORE answering it,
        // because the answer is what needs the route and this node may be the
        // one that answers.
        if (m_cfg.reversePath)
        {
            // NW-8: the previous hop -- the transmitter of this copy -- not the
            // originator, which is a neighbour only when the query came one hop.
            RememberReversePath(hdr.GetPacketId(),
                                (m_cfg.reversePathHop && txId != 0) ? txId : hdr.GetOriginId(),
                                nowS);
        }
        if (m_cfg.queryV2 && hdr.HasQueryV2())
        {
            // NW-9. Answer EVERY listed manifest this node can -- itself as the target, or
            // others by proxy from a fresh entry -- each with its own RESPONSE carrying the
            // entry. Whatever it could not answer is left for the rebroadcast; if nothing is
            // left, the query stops here (the batch-safe rule: a blanket "do not
            // rebroadcast after answering" would stop target 1 passing a five-manifest
            // batch on to targets 2-5).
            v2Handled = true;
            const std::vector<uint8_t>& want = hdr.GetQueryManifests();
            bool ownListed = false;
            for (size_t k = 0; k < want.size(); ++k)
            {
                const uint8_t m = want[k];
                CaTblTableEntry e;
                bool ok = false;
                const char* how = "";
                std::map<uint16_t, CaTblTableEntry>::const_iterator self =
                    m_table.find(m_cfg.nodeId);
                if (m_cfg.identityClass == CA_TBL_ID_TARGET && m == m_cfg.targetManifestId)
                {
                    ownListed = true;
                    // NW-4. Priority 1 is URGENT (0 ROUTINE; the byte mapping is OURS).
                    if (m_cfg.queryTriggeredEscalation && hdr.GetQueryPriority() >= 1)
                    {
                        m_urgentQueryPending = true;
                        ++m_urgentQueries;
                    }
                    if (self != m_table.end() && self->second.valid)
                    {
                        e = self->second;
                        ok = true;
                        how = "answer";
                        ++m_queriesAnswered;
                        ++m_v2Answered;
                    }
                }
                else if (m_cfg.queryProxy &&
                         ProxyAnswerFor(m, nowS, qTier, hdr.GetPacketId(), &e))
                {
                    ok = true;
                    how = "proxy";
                    ++m_proxyResponses;
                    ++m_v2Proxied;
                }
                if (ok)
                {
                    m_nextRespEntry = e;
                    m_nextHasRespEntry = true;
                    m_nextRespManifest = m;
                    SendResponse(qTier, hdr.GetQueryV2Id(), hdr.GetQueryRequesterId());
                    LogQueryV2(nowS, hdr.GetTier(), hdr.GetQueryV2Id(), how, m,
                               std::vector<uint8_t>());
                }
                else
                {
                    v2Left.push_back(m);
                }
            }
            if (m_cfg.identityClass == CA_TBL_ID_TARGET && !ownListed)
            {
                ++m_queriesIgnored;
            }
        }
        else if (m_cfg.identityClass == CA_TBL_ID_TARGET &&
            hdr.GetQueryManifestId() == m_cfg.targetManifestId)
        {
            ++m_queriesAnswered;
            SendResponse(qTier, hdr.GetPacketId(), hdr.GetOriginId());
        }
        else if (m_cfg.queryProxy &&
                 ProxyAnswerFor(hdr.GetQueryManifestId(), nowS, qTier, hdr.GetPacketId()))
        {
            // Network §4. This node is not the target, but it holds a fresh
            // enough entry ABOUT the target, so it answers on that entry's
            // behalf. The spec's point: the target may be unreachable from the
            // requester while its position is not.
            ++m_proxyResponses;
            SendResponse(qTier, hdr.GetPacketId(), hdr.GetOriginId());
        }
        else if (m_cfg.identityClass == CA_TBL_ID_TARGET)
        {
            // Counted, not dropped silently. A query that REACHES a target and is
            // correctly not answered is the evidence that scoping works; a query
            // that reaches nothing is evidence it does not, and without this counter
            // the two look identical from the outside.
            ++m_queriesIgnored;
        }
    }

    // ---- NW-8: a RESPONSE on the reverse path --------------------------------
    //
    // A unicast RESPONSE reaches only the node it was addressed to (the channel drops it
    // at every other receiver), so a copy here that is not this node's own answer is a
    // hop this node must carry on, using ITS OWN cached previous hop. A flooded copy is
    // forwarded by the flood branch below instead.
    // NW-9: the same first-copy rule hands each answer to the requester once.
    if ((m_cfg.reversePathHop || m_cfg.queryV2) && hdr.GetMsgType() == CA_TBL_MSG_RESPONSE &&
        hdr.GetRespQueryId() != 0)
    {
        const double nowRp = Simulator::Now().GetSeconds();
        if (hdr.GetRespRequesterId() == m_cfg.nodeId)
        {
            if (RpFirstCopy(hdr.GetRespQueryId(), hdr.GetRespAnswererId(),
                            hdr.GetRespManifest(), nowRp))
            {
                if (m_cfg.reversePathHop)
                {
                    ++m_rpDelivered;
                }
                if (m_cfg.queryV2)
                {
                    ++m_v2ResponsesHeard;
                    if (hdr.HasSelfEntry() && hdr.GetSelfEntry().valid)
                    {
                        ++m_v2EntriesHeard;
                    }
                    if (m_respCb)
                    {
                        m_respCb(hdr);
                    }
                }
            }
        }
        else if (m_cfg.reversePathHop && !hdr.GetForceFlood() &&
                 RpFirstCopy(hdr.GetRespQueryId(), hdr.GetRespAnswererId(),
                             hdr.GetRespManifest(), nowRp))
        {
            ++m_rpForwarded;
            if (m_cfg.queryV2)
            {
                // NW-9: the hop carries the answer on unchanged, entry and manifest.
                m_nextHasRespEntry = hdr.HasSelfEntry() && hdr.GetSelfEntry().valid;
                m_nextRespEntry = hdr.GetSelfEntry();
                m_nextRespManifest = hdr.GetRespManifest();
            }
            SendResponse(static_cast<CaTblTier>(hdr.GetTier()), hdr.GetRespQueryId(),
                         hdr.GetRespRequesterId(), hdr.GetRespAnswererId());
        }
    }

    // ---- L.1.4 / Network sect 6: grant gossip, received --------------------
    //
    // Only an anchor with a coordinator acts on it; anywhere else the frame is an
    // overheard broadcast and is merely counted as received above. Each heard grant goes
    // through Reconcile(), which is where a double grant made in the same instant by two
    // anchors that could not yet know of each other is finally settled: the lower id
    // yields, the higher id keeps it.
    if (hdr.GetMsgType() == CA_TBL_MSG_GRANT_GOSSIP && m_grantCoord != nullptr)
    {
        const CaTblTier gTier = static_cast<CaTblTier>(hdr.GetTier());
        const uint32_t gPayload =
            CA_TBL_MAC_HEADER_BYTES +
            CaTblPayloadBytes(CA_TBL_MSG_GRANT_GOSSIP,
                              static_cast<uint32_t>(hdr.GetGrants().size())) +
            GossipQueueBytes(hdr);
        m_gossipRxAirtimeS += (gTier == CA_TBL_TIER_UWB)
                                  ? CaTblEnergyModel::UwbFrameTimeS(gPayload)
                                  : CaTblEnergyModel::LoRaTimeOnAirS(m_cfg.sf,
                                                                     m_cfg.bandwidthHz,
                                                                     gPayload,
                                                                     m_cfg.codingRate);
        const double nowS = Simulator::Now().GetSeconds();
        m_grantCoord->Expire(nowS);
        const std::vector<CaTblGrantItem>& items = hdr.GetGrants();
        for (size_t k = 0; k < items.size(); ++k)
        {
            CaTblGrantRecord g;
            g.anchorId = hdr.GetOriginId();
            g.targetId = items[k].targetId;
            g.startS = items[k].startS;
            g.endS = items[k].endS;
            if (g.anchorId == m_cfg.nodeId || g.endS <= nowS)
            {
                continue; // our own echo, or a slot already over
            }
            const size_t before = m_grantCoord->Size();
            const CaTblGrantVerdict v = m_grantCoord->Reconcile(g);
            if (v == CA_TBL_GRANT_DEFER_LOWER_ID)
            {
                ++m_gossipItemsHeard;
                ++m_grantsYielded;
                // The local U(r) arbiter (C4) must stop treating the slot as held, or a
                // yielded anchor would go on refusing other targets for a slot it no
                // longer holds. Limit: the arbiter is per radio and only THIS radio's is
                // freed, so a slot it awarded on another radio of the same gateway stays
                // held until arbSlotHoldS lapses. Matters only under --uwbArbitration,
                // which no L.1.4 measurement used.
                if (m_slotHolder == g.targetId)
                {
                    m_slotGrantedAtS = -1.0;
                }
                LogGrantVerdict(g.targetId, g.anchorId, "gossip", v, "yield", g.startS,
                                g.endS);
            }
            else if (v == CA_TBL_GRANT_WIN_HIGHER_ID)
            {
                ++m_gossipItemsHeard;
                LogGrantVerdict(g.targetId, g.anchorId, "gossip", v, "keep", g.startS, g.endS);
            }
            else if (m_grantCoord->Size() > before)
            {
                ++m_gossipItemsHeard; // new, uncontested: stored for later requests
            }
        }
        if (m_cfg.gossipQueue)
        {
            // NW-6: the sender's whole queue, replacing what it said before.
            const std::vector<CaTblQueueGossipItem>& qi = hdr.GetQueueItems();
            std::vector<CaTblQueueRecord> recs;
            for (size_t k = 0; k < qi.size(); ++k)
            {
                CaTblQueueRecord r;
                r.targetId = qi[k].targetId;
                r.uQ = qi[k].uQ;
                r.queuedSinceS = qi[k].queuedSinceS;
                recs.push_back(r);
            }
            m_queueItemsHeard += recs.size();
            m_grantCoord->NoteGossipedQueue(hdr.GetOriginId(), recs, nowS);
        }
    }

    if (hdr.GetMsgType() == CA_TBL_MSG_ESC_REQUEST && hdr.GetReqSubject() != 0)
    {
        // N.10. A requester's request for another node, carried here as a frame (it was
        // addressed to this radio, and the bare ACK above settles it with its carrier).
        // Scored as one handed over directly; the GRANT, if any, goes to the subject.
        ++m_oboFramesHeard;
        const int v = SubmitRequestOnBehalf(hdr.GetReqSubject(), hdr.GetReqDegradation(),
                                            hdr.GetReqBattery(), hdr.GetReqQueryTriggered(),
                                            static_cast<CaTblTier>(hdr.GetTier()));
        ++m_oboFramesVerdict[(v == 1) ? 0 : ((v == 0) ? 1 : 2)];
    }
    else if (m_cfg.sendHandshakeFrames)
    {
        const CaTblTier rxTier = static_cast<CaTblTier>(hdr.GetTier());
        if (hdr.GetMsgType() == CA_TBL_MSG_ESC_REQUEST &&
            (m_cfg.identityClass == CA_TBL_ID_ANCHOR ||
             (m_cfg.arbitrationV2 && m_cfg.identityClass == CA_TBL_ID_RESCUER)))
        {
            // L.1.4 / Network sect 6. With a coordinator attached, the cross-anchor
            // tie-break runs FIRST: if a higher-id anchor is known to hold an overlapping
            // grant for this node, this anchor defers and sends nothing, and its local
            // U(r) arbiter is never consulted -- the slot is not its to award. Without a
            // coordinator this block is the C1/C4 behaviour, unchanged.
            const double reqS = Simulator::Now().GetSeconds();
            const double reqEndS = reqS + m_cfg.arbSlotHoldS;
            CaTblGrantVerdict tb = CA_TBL_GRANT_OK;
            if (m_grantCoord != nullptr)
            {
                m_grantCoord->Expire(reqS);
                tb = m_grantCoord->Evaluate(hdr.GetOriginId(), reqS, reqEndS);
            }
            if (m_grantCoord != nullptr && tb == CA_TBL_GRANT_DEFER_LOWER_ID)
            {
                ++m_grantsWithheld;
                LogGrantVerdict(hdr.GetOriginId(), m_grantCoord->LastConflictAnchor(),
                                "request", tb, "withhold", reqS, reqEndS);
            }
            else
            {
                bool granted = false;
                if (m_cfg.arbitrationV2)
                {
                    // NW-5: grant, or queue with a position, or refuse out of range.
                    uint8_t qpos = 0;
                    const int v = ArbitrateV2(hdr, &qpos);
                    granted = (v == 1);
                    m_nextQueuePos = granted ? 0 : qpos;
                    PrepareReply(hdr.GetPacketId(), true, granted);
                    Send(CA_TBL_MSG_ESC_GRANT, rxTier, 0, hdr.GetOriginId());
                }
                else if (!m_cfg.uwbArbitration)
                {
                    // C1 behaviour: grant whoever asked, and let CAD and the duty
                    // governor decide whether it lands.
                    PrepareReply(hdr.GetPacketId(), true, true);
                    Send(CA_TBL_MSG_ESC_GRANT, rxTier, 0, hdr.GetOriginId());
                    granted = true;
                }
                else if (ArbitrateRequest(hdr))
                {
                    PrepareReply(hdr.GetPacketId(), true, true);
                    Send(CA_TBL_MSG_ESC_GRANT, rxTier, 0, hdr.GetOriginId());
                    granted = true;
                }
                else if (m_cfg.handshakeFixes)
                {
                    // MC-1. A denial is answered too: the requester "must know
                    // definitively whether the grant was received" (MAC sect 8). It
                    // carries no ACK of the request, so the requester's ACK timeout
                    // retransmits and the arbiter scores the retry afresh.
                    PrepareReply(hdr.GetPacketId(), true, false);
                    Send(CA_TBL_MSG_ESC_GRANT, rxTier, 0, hdr.GetOriginId());
                }
                if (m_grantCoord != nullptr)
                {
                    // SELF_REPEAT re-sends the grant -- the requester asked again, so it
                    // did not hear the first -- but records no second window: the slot
                    // is the one already held.
                    if (granted && tb != CA_TBL_GRANT_SELF_REPEAT)
                    {
                        m_grantCoord->NoteOwnGrant(hdr.GetOriginId(), reqS, reqEndS);
                    }
                    LogGrantVerdict(hdr.GetOriginId(),
                                    (tb == CA_TBL_GRANT_WIN_HIGHER_ID)
                                        ? m_grantCoord->LastConflictAnchor()
                                        : static_cast<uint16_t>(0),
                                    "request", tb, granted ? "grant" : "denied", reqS,
                                    reqEndS);
                }
            }
        }
        else if (hdr.GetMsgType() == CA_TBL_MSG_DIGEST && m_cfg.digestSummary &&
                 !hdr.GetDigest().empty())
        {
            // ANTI-ENTROPY PROPER: answer what was actually asked. Everything this node
            // holds that the asker either lacks entirely, or holds a strictly older
            // version of, is a candidate; the rest is silence.
            const std::vector<CaTblDigestItem>& theirs = hdr.GetDigest();
            const CaTblTableEntry* best = nullptr;
            if (DigestOwnFirst())
            {
                // --digestOwnFirst. What the asker lacks of THIS node is what only this node can tell it: its own
                // entry first if unlisted or listed stale. Then an entry listed stale. An unlisted one only when the
                // digest was not full: past CA_TBL_DIGEST_MAX_ITEMS, unlisted is not known to be missing.
                const bool full = theirs.size() >= CA_TBL_DIGEST_MAX_ITEMS;
                const CaTblTableEntry* stale = nullptr;
                const CaTblTableEntry* unlisted = nullptr;
                for (std::map<uint16_t, CaTblTableEntry>::const_iterator it = m_table.begin();
                     it != m_table.end(); ++it)
                {
                    if (!it->second.valid)
                    {
                        continue;
                    }
                    bool listed = false;
                    bool current = false;
                    for (size_t k = 0; k < theirs.size(); ++k)
                    {
                        if (theirs[k].nodeId == it->second.nodeId)
                        {
                            listed = true;
                            current = !CaTblSeqNewer(it->second.seqNo, theirs[k].seqNo);
                            break;
                        }
                    }
                    if (it->second.nodeId == m_cfg.nodeId && (!listed || !current))
                    {
                        best = &it->second;
                        ++m_digestOwnPushes;
                        break;
                    }
                    if (listed && !current && stale == nullptr)
                    {
                        stale = &it->second;
                    }
                    else if (!listed && !full && unlisted == nullptr)
                    {
                        unlisted = &it->second;
                    }
                    else if (!listed && full)
                    {
                        ++m_digestFullSkips;
                    }
                }
                if (best == nullptr)
                {
                    best = (unlisted != nullptr) ? unlisted : stale;
                }
            }
            for (std::map<uint16_t, CaTblTableEntry>::const_iterator it = m_table.begin();
                 !DigestOwnFirst() && it != m_table.end(); ++it)
            {
                if (!it->second.valid)
                {
                    continue;
                }
                bool theyHaveCurrent = false;
                bool theyKnowIt = false;
                for (size_t k = 0; k < theirs.size(); ++k)
                {
                    if (theirs[k].nodeId != it->second.nodeId)
                    {
                        continue;
                    }
                    theyKnowIt = true;
                    // Modular comparison, the same rule supersession uses. A plain >
                    // would freeze every entry the first time a counter wrapped.
                    theyHaveCurrent = !CaTblSeqNewer(it->second.seqNo, theirs[k].seqNo);
                    break;
                }
                if (!theyKnowIt || !theyHaveCurrent)
                {
                    // ONE PER EXCHANGE, not everything at once. A responder that
                    // answered a digest with eight entries would spend more airtime
                    // than the summary saved, and the next digest will ask again.
                    // Prefer the entry the asker does not have at all over one it
                    // merely has stale.
                    if (best == nullptr || !theyKnowIt)
                    {
                        best = &it->second;
                        if (!theyKnowIt)
                        {
                            break;
                        }
                    }
                }
            }
            if (best != nullptr)
            {
                m_nextPushEntry = best;
                Send(CA_TBL_MSG_ENTRY_PUSH, rxTier, 1, hdr.GetOriginId());
                m_nextPushEntry = nullptr;
                ++m_digestPushes;
            }
            else
            {
                // The asker is current on everything this node knows. Sending an
                // ENTRY_PUSH here is the waste the summary exists to remove, and
                // counting it is how that saving becomes a number rather than a claim.
                ++m_digestNoOps;
            }
        }
        else if (hdr.GetMsgType() == CA_TBL_MSG_DIGEST)
        {
            // The reply StartHandshake's comment has always promised and nothing
            // has ever generated. One entry: this node's own, which is what an
            // anti-entropy digest exchange settles.
            Send(CA_TBL_MSG_ENTRY_PUSH, rxTier, 1, hdr.GetOriginId());
        }
    }

    // MC-1. A granted GRANT is the answer to the request it names: the request is
    // settled and its timeout must not retransmit it. A denied one leaves the request
    // pending on purpose -- the timeout retransmits it, which is the retry.
    if (m_cfg.handshakeFixes && hdr.GetMsgType() == CA_TBL_MSG_ESC_GRANT &&
        hdr.HasGrantVerdict())
    {
        if (hdr.GetGranted())
        {
            ++m_hsGrantsHeard;
            std::map<uint32_t, PendingFrame>::iterator rq =
                m_awaitingAck.find(hdr.GetAckedPacketId());
            if (rq != m_awaitingAck.end())
            {
                m_awaitingAck.erase(rq);
                ++m_hsRequestsAnswered;
                if (m_uwbGrantCb)
                {
                    m_uwbGrantCb(hdr.GetOriginId(), 1); // NW-7: only the answer to ours
                }
            }
            else if (m_cfg.arbitrationV2 && m_queuedReqs.erase(hdr.GetAckedPacketId()) != 0)
            {
                // NW-5: the slot a queue gave us, cycles after we asked.
                ++m_hsRequestsAnswered;
                if (m_uwbGrantCb)
                {
                    m_uwbGrantCb(hdr.GetOriginId(), 1);
                }
            }
            else if (hdr.GetGrantOnBehalf() && m_uwbGrantCb)
            {
                m_uwbGrantCb(hdr.GetOriginId(), 3); // TA-2: a requester asked for us
            }
        }
        else
        {
            ++m_hsDeniesHeard;
            const uint32_t acked = hdr.GetAckedPacketId();
            std::map<uint32_t, std::set<uint16_t>>::iterator qr = m_queuedReqs.find(acked);
            if (m_cfg.arbitrationV2 && hdr.GetQueuePos() > 0 &&
                (m_awaitingAck.count(acked) != 0 || qr != m_queuedReqs.end()))
            {
                // NW-5: queued, not refused. The anchor holds the request, so it is not
                // retried; the eventual GRANT will name it. A broadcast request can sit in
                // several anchors' queues: each is remembered, and only the first is news.
                const bool first = (m_awaitingAck.erase(acked) != 0);
                m_queuedReqs[acked].insert(hdr.GetOriginId());
                ++m_queuedHeard;
                if (first && m_uwbGrantCb)
                {
                    m_uwbGrantCb(hdr.GetOriginId(), 2);
                }
            }
            else if (m_cfg.arbitrationV2 && qr != m_queuedReqs.end())
            {
                // NW-5: dropped from the queue of an anchor that held it (expired, or out
                // of range) -- final only when no other anchor still holds it. A refusal
                // from an anchor that never queued it is the out-of-range kind: ignored.
                if (qr->second.erase(hdr.GetOriginId()) != 0 && qr->second.empty())
                {
                    m_queuedReqs.erase(qr);
                    if (m_uwbGrantCb)
                    {
                        m_uwbGrantCb(hdr.GetOriginId(), 0);
                    }
                }
            }
            else if (m_cfg.arbitrationV2 && m_awaitingAck.count(acked) != 0)
            {
                // NW-5: refused out of range by THIS anchor. The request was broadcast and
                // another anchor may have the requester in reach, so the refusal does not
                // settle it: it stays pending, MC-1's timeout retries it, and only running
                // out of retries ends it.
                ++m_outOfRangeHeard;
            }
            else if (m_uwbGrantCb && m_awaitingAck.count(acked) != 0)
            {
                m_uwbGrantCb(hdr.GetOriginId(), 0); // NW-7
            }
        }
    }

    if (hdr.GetMsgType() == CA_TBL_MSG_ACK)
    {
        // Clearing the pending entry is what stops the timeout from firing. The
        // ACK payload carries the acked packet_id in the real frame. By default the
        // directed reply is matched by clearing the oldest outstanding frame, which
        // is exact only while one ACK'd frame is outstanding -- and is not, because a
        // broadcast ESC_REQUEST draws an ACK from every node that hears it, and a
        // RESPONSE awaits its ACK on the same list. Under --handshakeFixes (MC-1) the
        // ACK names the frame it acknowledges and only that one is cleared.
        if (m_cfg.handshakeFixes)
        {
            std::map<uint32_t, PendingFrame>::iterator ak =
                m_awaitingAck.find(hdr.GetAckedPacketId());
            if (ak != m_awaitingAck.end())
            {
                m_awaitingAck.erase(ak);
                ++m_hsAcksMatched;
            }
            else
            {
                ++m_hsAcksUnmatched;
            }
        }
        else if (!m_awaitingAck.empty())
        {
            m_awaitingAck.erase(m_awaitingAck.begin());
        }
        ++c.acksReceived;
        return;
    }

    // Flood forwarding (§2.6): dedup, decrement, jitter, rebroadcast.
    // NW-8: the fallback RESPONSE floods too, except at the requester it is for.
    if ((CaTblIsFlooded(hdr.GetMsgType()) ||
         (hdr.GetForceFlood() && hdr.GetRespRequesterId() != m_cfg.nodeId)) &&
        hdr.GetOriginId() != m_cfg.nodeId)
    {
        CaTblHeader fwd = hdr;
        if (v2Handled)
        {
            // NW-9: relay only what this node could not answer.
            const double nowQ = Simulator::Now().GetSeconds();
            if (v2Left.empty())
            {
                ++m_v2Suppressed;
                LogQueryV2(nowQ, hdr.GetTier(), hdr.GetQueryV2Id(), "suppress", 0, v2Left);
                return;
            }
            if (v2Left.size() < hdr.GetQueryManifests().size())
            {
                fwd.SetQueryManifests(v2Left);
            }
        }
        if (!fwd.DecrementHopLimit())
        {
            ++c.droppedHopLimit;
            return;
        }
        if (v2Handled)
        {
            // Counted only once it is really relayed, past the hop limit.
            if (v2Left.size() < hdr.GetQueryManifests().size())
            {
                ++m_v2Reduced;
            }
            LogQueryV2(Simulator::Now().GetSeconds(), hdr.GetTier(), hdr.GetQueryV2Id(), "relay",
                       0, v2Left);
        }
        const double jitter =
            (m_rng != nullptr) ? m_rng->GetValue(0.0, m_cfg.rebroadcastJitterMaxS) : 0.05;
        Simulator::Schedule(Seconds(jitter),
                            &CaTblMac::MaybeRebroadcast,
                            this,
                            fwd,
                            static_cast<CaTblTier>(hdr.GetTier()));
    }
}

void
CaTblMac::NoteCollisionLoss(CaTblMsgType t)
{
    ++Counters(t).lostCollision;
}

void
CaTblMac::NoteCapture(CaTblMsgType t)
{
    ++Counters(t).wonCapture;
}

void
CaTblMac::MaybeRebroadcast(CaTblHeader hdr, CaTblTier tier)
{
    if (m_asleep)
    {
        return; // NW-0: fell asleep between hearing the frame and its jittered relay
    }
    PendingFrame f;
    f.hdr = hdr;
    f.tier = tier;
    // A relay does not know the originator's entry count. Forwarding at the
    // originator's payload size would require carrying it in the header; instead
    // the relay reproduces the size implied by the message type with one entry,
    // which is exact for the fixed-size types and an underestimate for
    // multi-entry pushes. ENTRY_PUSH is not flooded, so the only multi-entry
    // flooded type is QUERY, whose manifest list is small. Flagged rather than
    // hidden: a relay's airtime for a flooded QUERY is slightly optimistic.
    f.nEntries = 1;
    if (m_cfg.queryV2 && hdr.GetMsgType() == CA_TBL_MSG_QUERY && hdr.HasQueryV2() &&
        !hdr.GetQueryManifests().empty())
    {
        // NW-9: the relayed list is carried, so it is billed (6 + n).
        f.nEntries = static_cast<uint32_t>(hdr.GetQueryManifests().size());
    }
    f.destId = 0;
    f.cadAttempts = 0;
    f.txAttempts = 0;
    f.repeatsLeft = 1;
    f.enqueuedAt = Simulator::Now();
    AttemptCad(f);
}

// ---------------------------------------------------------------------------
// CaTblChannel
// ---------------------------------------------------------------------------

CaTblChannel::CaTblChannel()
    : m_phy(nullptr),
      // 6 dB is the conventional LoRa co-channel capture margin. It is a model
      // parameter, not a measurement, and the collision rate is sensitive to it
      // -- sweep it before quoting any contention result as robust.
      m_captureThresholdDb(6.0),
      m_txnCounter(0),
      m_totalTx(0),
      m_totalCollisions(0),
      m_totalCaptures(0),
      m_totalDeliveries(0)
{
}

void
CaTblChannel::Register(CaTblMac* mac)
{
    if (mac != nullptr)
    {
        m_nodes[mac->GetNodeId()] = mac;
        mac->SetChannel(this);
    }
}

void
CaTblChannel::Clear()
{
    m_nodes.clear();
    m_inFlight.clear();
    m_txnCounter = 0;
    m_totalTx = 0;
    m_totalCollisions = 0;
    m_totalCaptures = 0;
    m_totalDeliveries = 0;
}

void
CaTblChannel::PruneExpired()
{
    const Time now = Simulator::Now();
    std::vector<InFlight> keep;
    keep.reserve(m_inFlight.size());
    for (size_t i = 0; i < m_inFlight.size(); ++i)
    {
        if (m_inFlight[i].end >= now)
        {
            keep.push_back(m_inFlight[i]);
        }
    }
    m_inFlight.swap(keep);
}

void
CaTblChannel::Transmit(uint16_t txId,
                       CaTblTier tier,
                       const CaTblHeader& hdr,
                       Time toa,
                       double txPowerDbm,
                       uint16_t destId)
{
    PruneExpired();

    InFlight f;
    f.txnId = ++m_txnCounter;
    f.txId = txId;
    f.tier = tier;
    f.hdr = hdr;
    f.start = Simulator::Now();
    f.end = Simulator::Now() + toa;
    f.txPowerDbm = txPowerDbm;
    f.destId = destId;
    m_inFlight.push_back(f);
    ++m_totalTx;

    Simulator::Schedule(toa, &CaTblChannel::CompleteTransmission, this, f.txnId);
}

bool
CaTblChannel::IsBusy(uint16_t observerId, CaTblTier tier)
{
    PruneExpired();
    if (m_phy == nullptr)
    {
        return false;
    }

    for (size_t i = 0; i < m_inFlight.size(); ++i)
    {
        const InFlight& f = m_inFlight[i];
        if (f.tier != tier || f.txId == observerId)
        {
            continue;
        }
        // Audibility is the physics engine's own decision. A transmitter whose
        // signal does not reach the observer is invisible to its CAD, so the
        // observer will transmit into a collision -- hidden terminals emerge from
        // the model rather than being assumed away.
        const CaTblLinkOutcome o = m_phy->SenseFrame(f.txId, observerId, tier, f.txPowerDbm, f.hdr);
        if (o.received)
        {
            if (m_phy->IsSensePair(f.txId, observerId))
            {
                // TECH_DEBT 3.5: would the channel have been busy without the sense-only pair? Earlier frames were
                // not heard; look at the later ones.
                bool other = false;
                for (size_t j = i + 1; j < m_inFlight.size() && !other; ++j)
                {
                    const InFlight& g = m_inFlight[j];
                    if (g.tier == tier && g.txId != observerId && !m_phy->IsSensePair(g.txId, observerId))
                    {
                        other = m_phy->SenseFrame(g.txId, observerId, tier, g.txPowerDbm, g.hdr).received;
                    }
                }
                m_sensePairOnlyBusy += other ? 0 : 1;
            }
            return true;
        }
    }
    return false;
}

void
CaTblChannel::CompleteTransmission(uint64_t txnId)
{
    if (m_phy == nullptr)
    {
        return;
    }

    // Locate the finishing transmission.
    const InFlight* self = nullptr;
    for (size_t i = 0; i < m_inFlight.size(); ++i)
    {
        if (m_inFlight[i].txnId == txnId)
        {
            self = &m_inFlight[i];
            break;
        }
    }
    if (self == nullptr)
    {
        return;
    }
    const InFlight me = *self; // copy: m_inFlight may be mutated below

    // Everything that overlapped it in time on the same tier is a potential
    // collider. Overlap is computed from real start/end times, which is what
    // makes the outcome a consequence of the backoff draw rather than a
    // probability.
    std::vector<InFlight> overlapping;
    for (size_t i = 0; i < m_inFlight.size(); ++i)
    {
        const InFlight& o = m_inFlight[i];
        if (o.txnId == txnId || o.tier != me.tier)
        {
            continue;
        }
        if (o.start < me.end && me.start < o.end)
        {
            overlapping.push_back(o);
        }
    }
    // N.22. Frames that completed while this one was on air. Judged after the in-flight
    // ones, so a loss to one of them is a loss the default channel would have delivered.
    const size_t nLive = overlapping.size();
    if (m_collisionBothLost)
    {
        for (size_t i = 0; i < m_done.size(); ++i)
        {
            const InFlight& o = m_done[i];
            if (o.tier == me.tier && o.start < me.end && me.start < o.end)
            {
                overlapping.push_back(o);
            }
        }
    }

    for (std::map<uint16_t, CaTblMac*>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
    {
        const uint16_t rxId = it->first;
        CaTblMac* rx = it->second;
        if (rxId == me.txId)
        {
            continue;
        }
        // Directed frames are only processed by their addressee. Broadcast
        // (destId 0) reaches everyone in range.
        if (me.destId != 0 && me.destId != rxId)
        {
            continue;
        }
        // NW-0: a radio asleep this cycle hears nothing -- no link is evaluated, nothing is
        // logged. Never true unless the scenario sets it (--meshLinks=all).
        if (rx->IsAsleep())
        {
            rx->NoteAsleepRefusal();
            continue;
        }

        const CaTblLinkOutcome mine =
            m_phy->EvaluateFrame(me.txId, rxId, me.tier, me.txPowerDbm, me.hdr);
        if (!mine.received)
        {
            continue; // lost to path loss / fading / external interference
        }
        // N.6. The receiving radio transmitted during part of this frame. Evaluated after
        // the link so that count-only mode draws exactly what mode 0 draws.
        if (rx->HalfDuplexMode() != 0)
        {
            const bool own = rx->TransmittedDuring(me.start.GetSeconds(), me.end.GetSeconds());
            const bool deaf =
                own || rx->SiblingTransmittedDuring(me.start.GetSeconds(), me.end.GetSeconds());
            if (deaf)
            {
                rx->NoteDeafRx();
                if (!own)
                {
                    rx->NoteChipDeafRx(); // N.6, per chip
                }
                if (rx->HalfDuplexMode() == 2)
                {
                    if (m_eventLog != nullptr)
                    {
                        *m_eventLog << Simulator::Now().GetSeconds() << ','
                                    << static_cast<int>(me.tier) << ',' << me.txId << ',' << rxId
                                    << ',' << CaTblMsgTypeName(me.hdr.GetMsgType())
                                    << ",halfduplex," << mine.rxPowerDbm << ','
                                    << CaTblEventPayload(me.hdr) << '\n';
                    }
                    continue;
                }
            }
        }

        // Capture arbitration against our own concurrent transmitters.
        bool lost = false;
        bool late = false; // N.22: lost to a frame that had already completed
        uint16_t lateBy = 0;
        uint32_t lateByPkt = 0;
        bool captured = false;
        for (size_t k = 0; k < overlapping.size(); ++k)
        {
            const InFlight& o = overlapping[k];
            if (o.txId == rxId)
            {
                continue; // a node does not collide with itself
            }
            const CaTblLinkOutcome other =
                m_phy->EvaluateFrame(o.txId, rxId, o.tier, o.txPowerDbm, o.hdr);
            if (!other.received && other.rxPowerDbm < -190.0)
            {
                continue; // not audible here at all
            }
            if (mine.rxPowerDbm - other.rxPowerDbm >= m_captureThresholdDb)
            {
                captured = true; // strong enough to be captured despite the overlap
            }
            else
            {
                lost = true;
                late = (k >= nLive);
                lateBy = o.txId;
                lateByPkt = o.hdr.GetPacketId();
                break;
            }
        }

        if (lost)
        {
            ++m_totalCollisions;
            if (late)
            {
                ++m_lateCollisions;
            }
            if (m_eventLog != nullptr)
            {
                *m_eventLog << Simulator::Now().GetSeconds() << ',' << static_cast<int>(me.tier)
                            << ',' << me.txId << ',' << rxId << ','
                            << CaTblMsgTypeName(me.hdr.GetMsgType())
                            << (late ? ",collision_late," : ",collision,")
                            << mine.rxPowerDbm << ','
                            << CaTblEventPayload(me.hdr);
                if (late)
                {
                    // N.22: the completed frame's sender and packet id.
                    *m_eventLog << ";by=" << lateBy << ':' << lateByPkt;
                }
                *m_eventLog << '\n';
            }
            rx->NoteCollisionLoss(me.hdr.GetMsgType());
            continue;
        }
        if (captured)
        {
            ++m_totalCaptures;
            rx->NoteCapture(me.hdr.GetMsgType());
        }

        ++m_totalDeliveries;
        if (m_eventLog != nullptr)
        {
            *m_eventLog << Simulator::Now().GetSeconds() << ',' << static_cast<int>(me.tier)
                        << ',' << me.txId << ',' << rxId << ','
                        << CaTblMsgTypeName(me.hdr.GetMsgType()) << ','
                        << (captured ? "capture" : "delivered") << ',' << mine.rxPowerDbm
                        << ',' << CaTblEventPayload(me.hdr) << '\n';
        }
        rx->DeliverRx(me.hdr, mine, me.txId);
    }

    // Remove the completed transmission.
    for (size_t i = 0; i < m_inFlight.size(); ++i)
    {
        if (m_inFlight[i].txnId == txnId)
        {
            m_inFlight.erase(m_inFlight.begin() + static_cast<long>(i));
            break;
        }
    }
    if (m_collisionBothLost)
    {
        // N.22. Keep it while a frame still on air started before it ended; drop every
        // completed frame no frame on air overlaps any more.
        m_done.push_back(me);
        std::vector<InFlight> keep;
        for (size_t i = 0; i < m_done.size(); ++i)
        {
            bool needed = false;
            for (size_t j = 0; j < m_inFlight.size() && !needed; ++j)
            {
                needed = (m_inFlight[j].start < m_done[i].end);
            }
            if (needed)
            {
                keep.push_back(m_done[i]);
            }
        }
        m_done.swap(keep);
    }
}

// ---------------------------------------------------------------------------
// The §2.9 handshake
// ---------------------------------------------------------------------------

void
CaTblMac::StartHandshake(CaTblTier tier, uint16_t peerId, uint32_t nEntriesToPush)
{
    // This function IS claim 4. It is deliberately short, because the
    // contribution is the conditioning, not a new protocol: the same message
    // types, sequenced differently depending on whether the link is improving or
    // degrading. The underlying digest/anti-entropy vs eager-push distinction is
    // Demers et al.; conditioning it on link mobility state for a localisation
    // mesh is the contributed adaptation.
    const Time toaGuess = Seconds(CaTblEnergyModel::LoRaTimeOnAirS(
        m_cfg.sf, m_cfg.bandwidthHz, CA_TBL_MAC_HEADER_BYTES + 5, m_cfg.codingRate));

    if (m_mobility == CA_TBL_MOB_RECEDING)
    {
        // Skip the Digest entirely. A digest costs a round-trip, and on a
        // degrading link the reply may never arrive -- so push the self-entry
        // once, urgent-flagged, and take the higher contention priority that
        // TRIGGERED_UPDATE's access category confers. One shot, no negotiation.
        ++m_recedingStarted; // M.9
        Send(CA_TBL_MSG_TRIGGERED_UPDATE, tier, 1, 0);
        return;
    }

    // Stationary / approaching: the full three-step exchange at normal priority.
    // Beacon first (LOW), then the digest (NORMAL) after the beacon clears the
    // air, then the peer's entry-push arrives as a reply.
    Send(CA_TBL_MSG_DISCOVERY, tier, 0, 0);
    Simulator::Schedule(toaGuess + Seconds(m_cfg.slotTimeS * 4.0),
                        &CaTblMac::Send,
                        this,
                        CA_TBL_MSG_DIGEST,
                        tier,
                        nEntriesToPush,
                        peerId);
}

} // namespace ns3
