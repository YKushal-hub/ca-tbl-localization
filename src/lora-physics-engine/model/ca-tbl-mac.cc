#include "ca-tbl-mac.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

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
      codingRate(1)
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

void
CaTblMac::Send(CaTblMsgType type, CaTblTier tier, uint32_t nEntries, uint16_t destId)
{
    PendingFrame f;
    f.hdr.SetPacketId(NextPacketId());
    f.hdr.SetOriginId(m_cfg.nodeId);
    f.hdr.SetHopLimit(CaTblIsFlooded(type) ? m_cfg.defaultHopLimit : 1);
    f.hdr.SetInitialHopLimit(f.hdr.GetHopLimit());
    f.hdr.SetMsgType(type);
    f.hdr.SetTier(static_cast<uint8_t>(tier));
    f.tier = tier;
    f.nEntries = nEntries;
    f.destId = destId;
    f.cadAttempts = 0;
    f.txAttempts = 0;
    // §2.8: the urgent class substitutes redundancy for a round-trip.
    f.repeatsLeft = (CaTblAckPolicyFor(type) == CA_TBL_ACK_REDUNDANT)
                        ? m_cfg.triggeredUpdateRepeats
                        : 1;
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
    if (m_channel == nullptr)
    {
        return;
    }

    const uint32_t payload = CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(f.hdr.GetMsgType(), f.nEntries);
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

    const uint32_t payload =
        CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(f.hdr.GetMsgType(), f.nEntries);
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
    c.airtimeS += toa.GetSeconds();
    if (f.hdr.GetOriginId() != m_cfg.nodeId)
    {
        ++c.rebroadcast;
    }

    m_channel->Transmit(m_cfg.nodeId, f.tier, f.hdr, toa, m_txPowerDbm, f.destId);

    // Explicit ACK (§2.8): only the escalation exchange and an own-entry QUERY
    // response. Arm a timeout so a lost grant surfaces as a retransmission
    // rather than as a node silently believing it holds a UWB slot.
    if (CaTblAckPolicyFor(f.hdr.GetMsgType()) == CA_TBL_ACK_EXPLICIT &&
        f.hdr.GetMsgType() != CA_TBL_MSG_ACK)
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
CaTblMac::DeliverRx(const CaTblHeader& hdr, const CaTblLinkOutcome& out)
{
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

    // Before the flood-forwarding branch below, which is where a relayed copy
    // would otherwise become indistinguishable from a direct one.
    NoteNeighbour(hdr, out);

    UpdateApc(out.rxPowerDbm, static_cast<CaTblTier>(hdr.GetTier()));

    if (!m_rxCb.IsNull())
    {
        m_rxCb(m_cfg.nodeId, hdr, out);
    }

    // Explicit ACK obligation.
    if (CaTblAckPolicyFor(hdr.GetMsgType()) == CA_TBL_ACK_EXPLICIT &&
        hdr.GetMsgType() != CA_TBL_MSG_ACK)
    {
        ++Counters(CA_TBL_MSG_ACK).acksSent;
        Send(CA_TBL_MSG_ACK, static_cast<CaTblTier>(hdr.GetTier()), 0, hdr.GetOriginId());
    }

    if (hdr.GetMsgType() == CA_TBL_MSG_ACK)
    {
        // Clearing the pending entry is what stops the timeout from firing. The
        // ACK payload carries the acked packet_id in the real frame; here the
        // directed reply is matched by clearing the oldest outstanding request,
        // which is exact whenever at most one escalation is outstanding per node
        // -- true by construction, since a node holds at most one UWB slot.
        if (!m_awaitingAck.empty())
        {
            m_awaitingAck.erase(m_awaitingAck.begin());
        }
        ++c.acksReceived;
        return;
    }

    // Flood forwarding (§2.6): dedup, decrement, jitter, rebroadcast.
    if (CaTblIsFlooded(hdr.GetMsgType()) && hdr.GetOriginId() != m_cfg.nodeId)
    {
        CaTblHeader fwd = hdr;
        if (!fwd.DecrementHopLimit())
        {
            ++c.droppedHopLimit;
            return;
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
        const CaTblLinkOutcome o =
            m_phy->Evaluate(f.txId, observerId, tier, f.txPowerDbm, f.hdr.GetHopsTraversed());
        if (o.received)
        {
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

        const CaTblLinkOutcome mine =
            m_phy->Evaluate(me.txId, rxId, me.tier, me.txPowerDbm, me.hdr.GetHopsTraversed());
        if (!mine.received)
        {
            continue; // lost to path loss / fading / external interference
        }

        // Capture arbitration against our own concurrent transmitters.
        bool lost = false;
        bool captured = false;
        for (size_t k = 0; k < overlapping.size(); ++k)
        {
            const InFlight& o = overlapping[k];
            if (o.txId == rxId)
            {
                continue; // a node does not collide with itself
            }
            const CaTblLinkOutcome other = m_phy->Evaluate(o.txId,
                                                           rxId,
                                                           o.tier,
                                                           o.txPowerDbm,
                                                           o.hdr.GetHopsTraversed());
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
                break;
            }
        }

        if (lost)
        {
            ++m_totalCollisions;
            rx->NoteCollisionLoss(me.hdr.GetMsgType());
            continue;
        }
        if (captured)
        {
            ++m_totalCaptures;
            rx->NoteCapture(me.hdr.GetMsgType());
        }

        ++m_totalDeliveries;
        rx->DeliverRx(me.hdr, mine);
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
