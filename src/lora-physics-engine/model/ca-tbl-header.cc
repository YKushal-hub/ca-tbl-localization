#include "ca-tbl-header.h"

#include "ns3/log.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

CaTblTableEntry::CaTblTableEntry()
    : nodeId(0), x(0.0), y(0.0), varX(0.0), varY(0.0), covXY(0.0), timestampS(0.0),
      seqNo(0), hopCount(0), sourceType(0), vx(0.0), vy(0.0), targetManifestId(0),
      valid(false)
{
}

bool
CaTblSeqNewer(uint16_t a, uint16_t b)
{
    // Modular comparison, as the seq_no field comment requires: the counter wraps, so
    // a plain a > b would treat 0 as older than 65535 forever and the table would
    // freeze the first time a node wrapped. Half the space is "newer".
    return static_cast<uint16_t>(a - b) != 0 &&
           static_cast<uint16_t>(a - b) < 0x8000;
}


NS_LOG_COMPONENT_DEFINE("CaTblHeader");
NS_OBJECT_ENSURE_REGISTERED(CaTblHeader);

// ---------------------------------------------------------------------------
// Taxonomy tables (§2.3.1, §2.8)
// ---------------------------------------------------------------------------

const char*
CaTblMsgTypeName(CaTblMsgType t)
{
    switch (t)
    {
    case CA_TBL_MSG_DISCOVERY:
        return "DISCOVERY";
    case CA_TBL_MSG_DIGEST:
        return "DIGEST";
    case CA_TBL_MSG_ENTRY_PUSH:
        return "ENTRY_PUSH";
    case CA_TBL_MSG_TRIGGERED_UPDATE:
        return "TRIGGERED_UPDATE";
    case CA_TBL_MSG_ESC_REQUEST:
        return "ESC_REQUEST";
    case CA_TBL_MSG_ESC_GRANT:
        return "ESC_GRANT";
    case CA_TBL_MSG_QUERY:
        return "QUERY";
    case CA_TBL_MSG_RESPONSE:
        return "RESPONSE";
    case CA_TBL_MSG_ACK:
        return "ACK";
    case CA_TBL_MSG_GRANT_GOSSIP:
        return "GRANT_GOSSIP";
    default:
        return "UNKNOWN";
    }
}

const char*
CaTblMsgClassName(CaTblMsgType t)
{
    // Collapses the two escalation codes into the single class the design
    // document's tables use, so "seven message classes" in the report and nine
    // wire codes in the trace reconcile without hand-waving.
    if (t == CA_TBL_MSG_ESC_REQUEST || t == CA_TBL_MSG_ESC_GRANT)
    {
        return "ESCALATION";
    }
    return CaTblMsgTypeName(t);
}

const char*
CaTblAccessCategoryName(CaTblAccessCategory ac)
{
    switch (ac)
    {
    case CA_TBL_AC_URGENT:
        return "URGENT";
    case CA_TBL_AC_HIGH:
        return "HIGH";
    case CA_TBL_AC_NORMAL:
        return "NORMAL";
    case CA_TBL_AC_LOW:
        return "LOW";
    default:
        return "UNKNOWN";
    }
}

CaTblCwBounds
CaTblCwFor(CaTblAccessCategory ac)
{
    // URGENT 0-3 is verbatim from §2.3.1. The rest follow the 802.11e doubling
    // pattern; they are NOT calibrated and have not been swept.
    switch (ac)
    {
    case CA_TBL_AC_URGENT:
        return {0, 3};
    case CA_TBL_AC_HIGH:
        return {0, 7};
    case CA_TBL_AC_NORMAL:
        return {0, 15};
    case CA_TBL_AC_LOW:
        return {0, 31};
    default:
        return {0, 15};
    }
}

CaTblAccessCategory
CaTblAccessCategoryFor(CaTblMsgType t)
{
    switch (t)
    {
    case CA_TBL_MSG_TRIGGERED_UPDATE:
        return CA_TBL_AC_URGENT;

    case CA_TBL_MSG_ESC_REQUEST:
    case CA_TBL_MSG_ESC_GRANT:
    case CA_TBL_MSG_ACK:
        // The ACK inherits HIGH because the only explicitly-ACKed exchange is the
        // escalation one. An ACK that loses contention to routine gossip defeats
        // the purpose of ACKing that exchange at all.
        return CA_TBL_AC_HIGH;

    case CA_TBL_MSG_QUERY:
    case CA_TBL_MSG_RESPONSE:
        // Not in the §2.3.1 table, which predates §3.4. Assigned HIGH because a
        // QUERY is operator-initiated and §3.4 lets an URGENT query bypass PCS
        // gating entirely -- a human is waiting on it. Flagged as an assignment
        // made here, not one the design document states.
        return CA_TBL_AC_HIGH;

    case CA_TBL_MSG_ENTRY_PUSH:
        return CA_TBL_AC_NORMAL;

    case CA_TBL_MSG_DIGEST:
        return CA_TBL_AC_NORMAL;

    case CA_TBL_MSG_DISCOVERY:
        return CA_TBL_AC_LOW;

    case CA_TBL_MSG_GRANT_GOSSIP:
        // Network sect 6 says "low priority CW" in so many words. It is also the right
        // side to err on: a gossip that loses the channel to a field node's frame costs
        // one gossip interval of staleness, and a field node's frame that loses to a
        // gossip costs a measurement.
        return CA_TBL_AC_LOW;

    default:
        return CA_TBL_AC_NORMAL;
    }
}

const char*
CaTblAckPolicyName(CaTblAckPolicy p)
{
    switch (p)
    {
    case CA_TBL_ACK_NONE:
        return "NONE";
    case CA_TBL_ACK_IMPLICIT:
        return "IMPLICIT";
    case CA_TBL_ACK_EXPLICIT:
        return "EXPLICIT";
    case CA_TBL_ACK_REDUNDANT:
        return "REDUNDANT";
    default:
        return "UNKNOWN";
    }
}

CaTblAckPolicy
CaTblAckPolicyFor(CaTblMsgType t)
{
    switch (t)
    {
    case CA_TBL_MSG_DISCOVERY:
        return CA_TBL_ACK_NONE; // next beacon supersedes
    case CA_TBL_MSG_DIGEST:
        return CA_TBL_ACK_IMPLICIT; // the entry-push reply confirms receipt
    case CA_TBL_MSG_ENTRY_PUSH:
        return CA_TBL_ACK_IMPLICIT; // seq_no convergence at next digest confirms
    case CA_TBL_MSG_ESC_REQUEST:
    case CA_TBL_MSG_ESC_GRANT:
        // The one explicit ACK in the protocol. A silently lost grant leaves a
        // node believing it holds a UWB slot it does not hold, which is a wrong
        // radio state rather than merely a stale table entry.
        return CA_TBL_ACK_EXPLICIT;
    case CA_TBL_MSG_TRIGGERED_UPDATE:
        return CA_TBL_ACK_REDUNDANT; // 2-3x jittered rebroadcast instead
    case CA_TBL_MSG_QUERY:
        return CA_TBL_ACK_NONE; // flood; the RESPONSE is the answer
    case CA_TBL_MSG_RESPONSE:
        return CA_TBL_ACK_EXPLICIT; // §3.4: own-entry response is ACKed, highest priority
    case CA_TBL_MSG_ACK:
        return CA_TBL_ACK_NONE; // do not ACK an ACK
    case CA_TBL_MSG_GRANT_GOSSIP:
        return CA_TBL_ACK_NONE; // periodic; the next gossip supersedes a lost one
    default:
        return CA_TBL_ACK_NONE;
    }
}

bool
CaTblIsFlooded(CaTblMsgType t)
{
    switch (t)
    {
    case CA_TBL_MSG_DISCOVERY:
    case CA_TBL_MSG_TRIGGERED_UPDATE:
    case CA_TBL_MSG_QUERY:
        return true;

    // Directed exchanges. DIGEST/ENTRY_PUSH are a targeted pairwise
    // anti-entropy exchange, not a flood -- flooding them would defeat the
    // digest mechanism's entire purpose, which is to avoid pushing table
    // contents further than necessary.
    case CA_TBL_MSG_DIGEST:
    case CA_TBL_MSG_ENTRY_PUSH:
    case CA_TBL_MSG_ESC_REQUEST:
    case CA_TBL_MSG_ESC_GRANT:
    case CA_TBL_MSG_RESPONSE:
    case CA_TBL_MSG_ACK:
        return false;

    // Network sect 6 addresses NEIGHBOURING anchors. Flooding it would let a grant
    // constrain an anchor too far away to ever contend for the same field node.
    case CA_TBL_MSG_GRANT_GOSSIP:
        return false;

    default:
        return false;
    }
}

bool
CaTblCarriesPositionFix(CaTblMsgType t)
{
    // §3.3 scopes the confidence-weighted routing metric to position-fix traffic
    // only. Widening this set widens the routing claim, which the design document
    // is explicit about not making.
    return t == CA_TBL_MSG_ENTRY_PUSH || t == CA_TBL_MSG_TRIGGERED_UPDATE ||
           t == CA_TBL_MSG_RESPONSE;
}

// ---------------------------------------------------------------------------
// Payload sizing
// ---------------------------------------------------------------------------

uint32_t
CaTblPayloadBytes(CaTblMsgType t, uint32_t nEntries)
{
    switch (t)
    {
    case CA_TBL_MSG_DISCOVERY:
        // identity_class(1) + target_manifest_id(1) + battery_pct(1) + pcs_q(1)
        // + tx_power_dbm(1). Battery and PCS ride the beacon because the anchor
        // arbiter's U(r) needs both for every candidate and cannot afford a
        // separate poll per target under a 2.5% duty cycle.
        return 5;

    case CA_TBL_MSG_DIGEST:
        return nEntries * CA_TBL_DIGEST_ENTRY_BYTES;

    case CA_TBL_MSG_ENTRY_PUSH:
    case CA_TBL_MSG_RESPONSE:
        return nEntries * CA_TBL_TABLE_ENTRY_BYTES;

    case CA_TBL_MSG_TRIGGERED_UPDATE:
        // One entry plus a reason code: LARGE_CORRECTION | LOW_BATTERY | SUSPECT.
        return CA_TBL_TABLE_ENTRY_BYTES + 1;

    case CA_TBL_MSG_ESC_REQUEST:
        // requester_id(2) + pcs_q(2) + degradation_rate_q(2) + battery_pct(1)
        // + staleness_s(2) + query_triggered(1). These are exactly the U(r)
        // terms from §3.5 -- the request carries its own arbitration inputs so
        // the anchor does not have to interrogate each requester.
        return 10;

    case CA_TBL_MSG_ESC_GRANT:
        // target_id(2) + granted(1) + slot_start_ms(2) + slot_len_ms(2) + queue_pos(1)
        return 8;

    case CA_TBL_MSG_QUERY:
        // query_id(2) + requester_id(2) + ttl(1) + priority(1) + manifest ids
        return 6 + nEntries;

    case CA_TBL_MSG_ACK:
        // acked packet_id(4)
        return 4;

    case CA_TBL_MSG_GRANT_GOSSIP:
        // n_grants(1) + n x (target_id(2) + slot_start_ms(2) + slot_len_ms(2)). The spec
        // also names the request queue Q; CaTblMac holds no queue, so there is no Q to send
        // and no byte is billed for one. What happens to a request it does not grant
        // depends on --handshakeFixes (MC-1). By default the anchor has already ACKed the
        // request before arbitrating, so the requester's pending copy is cleared and a
        // denied request is NEVER retried -- this comment said it was until 2026-10-02.
        // Under the flag the anchor answers with a GRANT saying denied and no ACK, so the
        // requester's ACK timeout retransmits and the anchor scores the retry afresh.
        return 1 + nEntries * CA_TBL_GRANT_ITEM_BYTES;

    default:
        return 0;
    }
}

// ---------------------------------------------------------------------------
// Time on air -- Semtech AN1200.13. Borrowed, unmodified.
// ---------------------------------------------------------------------------

Time
CaTblSymbolTime(uint8_t sf, double bandwidthHz)
{
    if (bandwidthHz <= 0.0)
    {
        return Seconds(0.0);
    }
    return Seconds(std::pow(2.0, static_cast<double>(sf)) / bandwidthHz);
}

bool
CaTblRecommendedLowDataRateOpt(uint8_t sf, double bandwidthHz)
{
    // Datasheet rule: enable DE when the symbol period exceeds 16 ms.
    return CaTblSymbolTime(sf, bandwidthHz).GetSeconds() > 0.016;
}

Time
CaTblTimeOnAir(uint32_t payloadBytes,
               uint8_t sf,
               double bandwidthHz,
               uint8_t cr,
               bool explicitHeader,
               bool crcOn,
               bool lowDataRateOpt,
               uint32_t nPreamble)
{
    if (bandwidthHz <= 0.0 || sf < 5 || sf > 12)
    {
        NS_LOG_WARN("CaTblTimeOnAir: sf=" << static_cast<uint32_t>(sf) << " bw=" << bandwidthHz
                                          << " out of range; returning 0");
        return Seconds(0.0);
    }

    const double tSym = std::pow(2.0, static_cast<double>(sf)) / bandwidthHz;
    const double tPreamble = (static_cast<double>(nPreamble) + 4.25) * tSym;

    const int de = lowDataRateOpt ? 1 : 0;
    const int h = explicitHeader ? 0 : 1;
    const int crcBit = crcOn ? 1 : 0;

    // Guard the DE trap called out in the header: at low SF, DE=1 shrinks the
    // denominator and silently inflates the symbol count instead of failing.
    const int denom = 4 * (static_cast<int>(sf) - 2 * de);
    if (denom <= 0)
    {
        NS_LOG_WARN("CaTblTimeOnAir: DE=1 invalid at SF" << static_cast<uint32_t>(sf));
        return Seconds(0.0);
    }

    const int numer = 8 * static_cast<int>(payloadBytes) - 4 * static_cast<int>(sf) + 28 +
                      16 * crcBit - 20 * h;

    const double ceilTerm = std::ceil(static_cast<double>(numer) / static_cast<double>(denom));
    const double payloadSymbols =
        8.0 + std::max(ceilTerm * static_cast<double>(cr + 4), 0.0);

    return Seconds(tPreamble + payloadSymbols * tSym);
}

// ---------------------------------------------------------------------------
// CaTblHeader
// ---------------------------------------------------------------------------

CaTblHeader::CaTblHeader()
    : m_packetId(0),
      m_originId(0),
      m_hopLimit(0),
      m_msgType(CA_TBL_MSG_DISCOVERY),
      m_tier(0),
      m_initialHopLimit(0)
{
}

TypeId
CaTblHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CaTblHeader")
                            .SetParent<Header>()
                            .SetGroupName("LoraPhysicsEngine")
                            .AddConstructor<CaTblHeader>();
    return tid;
}

TypeId
CaTblHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
CaTblHeader::GetSerializedSize() const
{
    return CA_TBL_MAC_HEADER_BYTES;
}

void
CaTblHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteHtonU32(m_packetId);
    start.WriteHtonU16(m_originId);
    start.WriteU8(m_hopLimit);
    start.WriteU8(static_cast<uint8_t>(m_msgType));
    start.WriteU8(m_tier);
}

uint32_t
CaTblHeader::Deserialize(Buffer::Iterator start)
{
    m_packetId = start.ReadNtohU32();
    m_originId = start.ReadNtohU16();
    m_hopLimit = start.ReadU8();
    m_msgType = static_cast<CaTblMsgType>(start.ReadU8());
    m_tier = start.ReadU8();
    return CA_TBL_MAC_HEADER_BYTES;
}

void
CaTblHeader::Print(std::ostream& os) const
{
    os << "id=" << m_packetId << " origin=" << m_originId
       << " hop=" << static_cast<uint32_t>(m_hopLimit) << " type=" << CaTblMsgTypeName(m_msgType)
       << " tier=" << static_cast<uint32_t>(m_tier)
       << " ac=" << CaTblAccessCategoryName(GetAccessCategory());
}

bool
CaTblHeader::DecrementHopLimit()
{
    if (m_hopLimit == 0)
    {
        return false;
    }
    --m_hopLimit;
    return true;
}

uint8_t
CaTblHeader::GetHopsTraversed() const
{
    if (m_initialHopLimit < m_hopLimit)
    {
        return 0;
    }
    return static_cast<uint8_t>(m_initialHopLimit - m_hopLimit);
}


namespace
{
//! Inverse of a symmetric 2x2 [[a,b],[b,d]]. False if singular or not positive.
bool
Inv2x2(double a, double b, double d, double& ia, double& ib, double& id)
{
    const double det = a * d - b * b;
    if (!(det > 1e-12) || !(a > 0.0) || !(d > 0.0))
    {
        return false;
    }
    ia = d / det;
    ib = -b / det;
    id = a / det;
    return true;
}
} // namespace

bool
CaTblCovarianceIntersect(const CaTblTableEntry& a, const CaTblTableEntry& b, CaTblTableEntry& out)
{
    double ia, ib, id, ja, jb, jd;
    if (!Inv2x2(a.varX, a.covXY, a.varY, ia, ib, id) ||
        !Inv2x2(b.varX, b.covXY, b.varY, ja, jb, jd))
    {
        return false;
    }

    // Bounded scan on omega. 101 points is far finer than the covariances are known
    // to, and a closed form would buy precision the inputs do not have.
    double bestW = 0.5;
    double bestTrace = -1.0;
    double bX = 0.0, bXY = 0.0, bY = 0.0;
    for (int k = 0; k <= 100; ++k)
    {
        const double w = static_cast<double>(k) / 100.0;
        const double pa = w * ia + (1.0 - w) * ja;
        const double pb = w * ib + (1.0 - w) * jb;
        const double pd = w * id + (1.0 - w) * jd;
        double fa, fb, fd;
        if (!Inv2x2(pa, pb, pd, fa, fb, fd))
        {
            continue;
        }
        const double tr = fa + fd;
        if (bestTrace < 0.0 || tr < bestTrace)
        {
            bestTrace = tr;
            bestW = w;
            bX = fa;
            bXY = fb;
            bY = fd;
        }
    }
    if (bestTrace < 0.0)
    {
        return false;
    }

    // x_fused = P_fused * (w * Pa^-1 xa + (1-w) * Pb^-1 xb)
    const double w = bestW;
    const double rx = w * (ia * a.x + ib * a.y) + (1.0 - w) * (ja * b.x + jb * b.y);
    const double ry = w * (ib * a.x + id * a.y) + (1.0 - w) * (jb * b.x + jd * b.y);

    // The NEWER of the two contributors supplies the non-statistical fields. Fusing a
    // timestamp would invent a moment neither report was made at, and fusing a hop
    // count would claim a path neither entry travelled.
    out = a;
    out.x = bX * rx + bXY * ry;
    out.y = bXY * rx + bY * ry;
    out.varX = bX;
    out.varY = bY;
    out.covXY = bXY;
    out.timestampS = (a.timestampS >= b.timestampS) ? a.timestampS : b.timestampS;
    out.hopCount = (a.hopCount <= b.hopCount) ? a.hopCount : b.hopCount;
    out.valid = true;
    return true;
}


CaTblFreshness
CaTblClassifyFreshness(double ageS, const CaTblFreshnessBounds& b)
{
    // An entry timestamped in the future is not evidence about the present. It means a
    // clock disagreement, and treating it as the freshest thing held is how a bad clock
    // wins every supersession contest it enters.
    if (ageS < 0.0)
    {
        return CA_TBL_EXPIRED;
    }
    if (ageS <= b.freshS)
    {
        return CA_TBL_FRESH;
    }
    if (ageS <= b.agingS)
    {
        return CA_TBL_AGING;
    }
    if (ageS <= b.staleS)
    {
        return CA_TBL_STALE;
    }
    return CA_TBL_EXPIRED;
}

const char*
CaTblFreshnessName(CaTblFreshness f)
{
    switch (f)
    {
    case CA_TBL_FRESH:
        return "FRESH";
    case CA_TBL_AGING:
        return "AGING";
    case CA_TBL_STALE:
        return "STALE";
    default:
        return "EXPIRED";
    }
}

} // namespace ns3
