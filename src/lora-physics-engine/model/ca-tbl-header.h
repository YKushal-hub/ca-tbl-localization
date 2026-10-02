#ifndef CA_TBL_HEADER_H
#define CA_TBL_HEADER_H

/**
 * \file ca-tbl-header.h
 * \brief CA-TBL Phase S4 -- the 9-byte MAC header, message taxonomy, and airtime model.
 *
 * Why this exists
 * ---------------
 * Up to S2.5 the simulation had no packets. `ProcessTransmission()` took
 * (SF, BW, txPower) and returned a statistical link outcome. Nothing constructed
 * a frame, nothing carried a message type, nothing computed time-on-air, and
 * nothing contended for the channel. Two of the six claims sat at exactly zero
 * as a result:
 *
 *   claim 4 (custom handshake)    -- no messages to sequence into a handshake
 *   claim 5 (specialized packets) -- no packets to specialise
 *
 * and claim 6 ("real physics applied to packets") was only half-true: the ~35
 * physics submodules were excellent, but they were applied to link samples
 * rather than to frames in a protocol, so interference was a probability drawn
 * per sample rather than an emergent consequence of two nodes transmitting at
 * overlapping times.
 *
 * This file is the frame. ca-tbl-mac.h is the protocol that uses it.
 *
 * Specification source
 * --------------------
 * Implemented to finalyearprojectcontext.md §2.7 (frame structure), §2.2
 * (time-on-air), §2.3.1 (priority contention windows), §2.8 (selective ACK) and
 * §3.1/§3.4 (identity fields). Where the design document cites a borrowed
 * mechanism, the citation is reproduced here rather than paraphrased, because
 * which parts are borrowed and which are contributed is the whole basis of the
 * novelty claim and must not drift between the document and the code:
 *
 *   - ToA equations: Semtech SX1276 datasheet / AN1200.13. Borrowed, unmodified.
 *   - CAD-based CSMA: To & Duda. Borrowed.
 *   - Priority contention windows: access-category concept adapted from IEEE
 *     802.11e EDCA/WMM to LoRa CAD backoff. CONTRIBUTED adaptation; the
 *     mechanism class itself is 802.11e.
 *   - Uniform 9-byte header across all message types: conventional framing.
 *   - Selective-ACK mapping onto this urgency taxonomy under the WPC duty-cycle
 *     cap: CONTRIBUTED as a policy.
 */

#include "ns3/header.h"
#include "ns3/nstime.h"

#include <cstdint>
#include <vector>
#include <string>

namespace ns3
{

/**
 * \brief Message taxonomy (design doc §2.7 payload table, §2.8, §3.4).
 *
 * The document describes seven message *classes*. Escalation request and grant
 * are one class in the document's tables (they share a contention window and an
 * ACK policy) but they are two distinct messages on the wire, so they get two
 * codes here. That is eight codes over seven classes, not a departure from the
 * taxonomy -- see CaTblMsgClassName().
 *
 * Values are explicit and stable: msg_type is a 1-byte wire field, so renumbering
 * silently invalidates any capture or trace already collected.
 */
enum CaTblMsgType : uint8_t
{
    CA_TBL_MSG_DISCOVERY = 1,        //!< periodic beacon; next beacon supersedes a lost one
    CA_TBL_MSG_DIGEST = 2,           //!< seq_no summary -- anti-entropy, avoids pushing full tables
    CA_TBL_MSG_ENTRY_PUSH = 3,       //!< table entries, in reply to a digest or eagerly
    CA_TBL_MSG_TRIGGERED_UPDATE = 4, //!< large correction or dying battery ("last gasp")
    CA_TBL_MSG_ESC_REQUEST = 5,      //!< request a UWB slot from the arbiter
    CA_TBL_MSG_ESC_GRANT = 6,        //!< grant/deny a UWB slot
    CA_TBL_MSG_QUERY = 7,            //!< identity-scoped search flood (§3.4)
    CA_TBL_MSG_RESPONSE = 8,         //!< answer to a QUERY, own entry or proxied
    CA_TBL_MSG_ACK = 9,              //!< explicit ACK; only ever for ESC_REQUEST/GRANT (§2.8)
    //! Network sect 6: an anchor telling neighbouring anchors which UWB slots it holds.
    //! Anchor-to-anchor, one hop, LOW access category, unacknowledged -- the next gossip
    //! supersedes a lost one. Constructed only under --anchorGrantGossipS > 0.
    CA_TBL_MSG_GRANT_GOSSIP = 10
};

/** Wire name, for traces. */
const char* CaTblMsgTypeName(CaTblMsgType t);

/** Taxonomy class name -- collapses ESC_REQUEST/ESC_GRANT into one label so the
 *  seven-class structure in the report and the nine wire codes stay reconcilable. */
const char* CaTblMsgClassName(CaTblMsgType t);

/**
 * \brief Access categories for CAD backoff (§2.3.1).
 *
 * Smaller CW means less expected backoff and therefore higher statistical
 * priority. Directly analogous to 802.11e/WMM access categories; the adaptation
 * to LoRa CAD backoff and to this message taxonomy is the contributed part.
 */
enum CaTblAccessCategory : uint8_t
{
    CA_TBL_AC_URGENT = 0, //!< triggered-update / last-gasp -- must win fast
    CA_TBL_AC_HIGH = 1,   //!< UWB arbitration request/grant -- time-sensitive
    CA_TBL_AC_NORMAL = 2, //!< entry-push -- routine
    CA_TBL_AC_LOW = 3     //!< discovery beacon -- delay-tolerant
};

const char* CaTblAccessCategoryName(CaTblAccessCategory ac);

/** Contention-window bounds in slots, per access category (§2.3.1). */
struct CaTblCwBounds
{
    uint32_t cwMin;
    uint32_t cwMax;
};

/**
 * \brief CW bounds for an access category.
 *
 * URGENT is 0-3 slots, taken verbatim from the design document. The remaining
 * three double at each step, which is the 802.11e pattern rather than an
 * independently justified choice -- and it is a tunable that has not been swept.
 * Do not present these four numbers as calibrated.
 */
CaTblCwBounds CaTblCwFor(CaTblAccessCategory ac);

/** Access category for a message type (§2.3.1 table). */
CaTblAccessCategory CaTblAccessCategoryFor(CaTblMsgType t);

/**
 * \brief ACK policy (§2.8).
 *
 * The point of the taxonomy is that ACKing everything is unaffordable at high SF
 * under a 2.5% duty cycle. Only the escalation exchange is explicitly ACKed,
 * because a silently lost grant leaves a node believing it holds a UWB slot it
 * does not hold -- the one failure here whose consequence is a wrong radio state
 * rather than a stale table entry.
 */
enum CaTblAckPolicy : uint8_t
{
    CA_TBL_ACK_NONE = 0,      //!< lost frame is superseded by the next one
    CA_TBL_ACK_IMPLICIT = 1,  //!< a subsequent message confirms receipt
    CA_TBL_ACK_EXPLICIT = 2,  //!< real ARQ: ACK required, retransmit on timeout
    CA_TBL_ACK_REDUNDANT = 3  //!< no ACK; 2-3x jittered rebroadcast instead
};

const char* CaTblAckPolicyName(CaTblAckPolicy p);

/** ACK policy for a message type (§2.8 table). */
CaTblAckPolicy CaTblAckPolicyFor(CaTblMsgType t);

/** True if this message is flooded (dedup + hop_limit decrement + jitter, §2.6)
 *  rather than being a directed point-to-point exchange. */
bool CaTblIsFlooded(CaTblMsgType t);

/** True if this message carries position-fix data, which is what scopes the
 *  confidence-weighted routing metric (§3.3). The document is explicit that the
 *  metric applies to position-fix traffic ONLY, not to routine gossip -- a
 *  reviewer will otherwise read it as a general-purpose routing claim, which is
 *  much larger and separately well-studied. */
bool CaTblCarriesPositionFix(CaTblMsgType t);

/** Identity classes (§3.1). */
enum CaTblIdentityClass : uint8_t
{
    CA_TBL_ID_UNASSIGNED = 0,
    CA_TBL_ID_RESCUER = 1,
    CA_TBL_ID_TARGET = 2,
    CA_TBL_ID_ANCHOR = 3
};

// ---------------------------------------------------------------------------
// Payload sizes
// ---------------------------------------------------------------------------

/**
 * \brief Wire size of one table entry (design doc §2.1 of the switching section:
 *        E = { pos(x,y), Sigma(2x2), timestamp, seq_no, hop_count, source_type,
 *        velocity_estimate }).
 *
 * Field widths are a design choice this code makes explicit rather than leaving
 * implicit, because payload bytes drive ToA which drives the duty-cycle budget --
 * these numbers are load-bearing for the airtime claim, not cosmetic:
 *
 *   node_id            2   (§3.1: 16-bit provisioned)
 *   pos x, y           8   (2 x int32, centimetre resolution over +/-21 000 km)
 *   Sigma              6   (3 x uint16: var_x, var_y, cov_xy, quantised)
 *   timestamp          4   (uint32, seconds)
 *   seq_no             2   (uint16, wraps -- supersession is modular)
 *   hop_count          1
 *   source_type        1   (tier + identity class packed)
 *   velocity_estimate  4   (2 x int16, cm/s)
 *   -----------------------
 *                     28 bytes
 */
static const uint32_t CA_TBL_TABLE_ENTRY_BYTES = 28;

/** One byte for target_manifest_id, charged only when proxy answering is enabled
 *  (Network §4). A byte that is not transmitted must not be billed, and a byte
 *  that is must be -- so this is added to the entry size by the caller rather
 *  than folded into the constant above. */
static const uint32_t CA_TBL_MANIFEST_ID_BYTES = 1;

/**
 * \brief The record the constant above sizes, now with contents.
 *
 * This is what the network knows ABOUT a node, as distinct from CaTblNeighbour, which
 * is what a node has personally HEARD. Keeping the two apart is load-bearing: the
 * neighbour table drives the poll set and must stay strictly one-hop, or a node burns a
 * full receive timeout every cycle calling peers it cannot reach. The shared table has
 * no such restriction, because nothing is ever polled from it -- it is knowledge, not a
 * work list.
 */
struct CaTblTableEntry
{
    uint16_t nodeId;
    double x;           //!< reported position estimate
    double y;
    double varX;        //!< the reporter's OWN uncertainty, travelling with the fix
    double varY;
    double covXY;
    double timestampS;
    uint16_t seqNo;     //!< wraps; supersession is modular
    uint8_t hopCount;
    uint8_t sourceType; //!< tier in the low nibble, identity class in the high
    double vx;          //!< velocity estimate
    double vy;
    //! Network Layer §1. Which missing person this entry is about, 0 for none.
    //! Identity CLASS already rides in sourceType's high nibble; the manifest id
    //! needs its own byte, and a proxy responder cannot match without it.
    uint8_t targetManifestId;
    bool valid;

    CaTblTableEntry();
};

//! Modular supersession: true if \p a is newer than \p b on a wrapping counter.
bool CaTblSeqNewer(uint16_t a, uint16_t b);

/**
 * \brief Covariance Intersection (Julier & Uhlmann 1997) over two table entries.
 *
 * WHY NOT A KALMAN UPDATE, which is the thing everyone reaches for first. A Kalman
 * fuse assumes the two estimates are INDEPENDENT. Two copies of the same entry that
 * reached this node by different mesh paths are not: they share an upstream relay,
 * and which one cannot be identified from the entry. Fusing them as independent
 * returns a covariance SMALLER THAN EITHER INPUT -- an estimate more confident than
 * either thing it was built from, which is the classic double-counting failure and
 * is exactly how a mesh talks itself into a wrong position with high confidence.
 *
 * CI is conservative BY CONSTRUCTION under any unknown correlation. The price is
 * real and should be stated rather than hidden: the result is never as tight as a
 * correct independent fuse would have been. That is the trade -- never overconfident,
 * sometimes needlessly humble -- and for a system whose whole premise is that a
 * confidence number drives an escalation decision, it is the right side to err on.
 *
 * omega is chosen to minimise trace(P_fused) by a bounded scan. trace rather than
 * det because trace is the quantity the confidence score already consumes, so the
 * fuse optimises the number the policy actually reads.
 *
 * \return false if either covariance is singular or non-positive, in which case
 *         nothing is written to \p out and the caller must fall back. A fuse that
 *         cannot be computed must not silently return one of its inputs.
 */
/**
 * \brief Age classes for a held entry (spec sect 23).
 *
 * The code already inflated variance with age, which is the right thing to do to a
 * NUMBER. What it could not do is answer a question about a DECISION -- may this entry
 * be proxied, may it steer a searcher, may it be shown to an operator as a position --
 * because those are threshold questions and a continuous inflation has no threshold in
 * it. Four classes, and each names an action rather than a feeling:
 *
 *   FRESH    usable as a position without qualification
 *   AGING    usable, and must be shown with its age
 *   STALE    may steer a search, must NOT be reported as a position
 *   EXPIRED  may not leave this node at all
 *
 * The boundaries are OURS, not cited, and are recorded as such: the spec fixes only
 * the proxy bound at 120 s. Anything downstream that treats them as measured is wrong.
 */
enum CaTblFreshness : uint8_t
{
    CA_TBL_FRESH = 0,
    CA_TBL_AGING = 1,
    CA_TBL_STALE = 2,
    CA_TBL_EXPIRED = 3
};

//! Age boundaries in seconds. Defaults are ours; see the enum's note.
struct CaTblFreshnessBounds
{
    double freshS = 30.0;
    double agingS = 120.0; //!< the spec's proxy staleness bound
    double staleS = 600.0;
};

//! Classify an entry by age. \p ageS below zero (a clock that went backwards) is
//! EXPIRED rather than FRESH: an entry from the future is not evidence.
CaTblFreshness CaTblClassifyFreshness(double ageS, const CaTblFreshnessBounds& b);

//! Human-readable class name, for traces and operator output.
const char* CaTblFreshnessName(CaTblFreshness f);

bool CaTblCovarianceIntersect(const CaTblTableEntry& a,
                              const CaTblTableEntry& b,
                              CaTblTableEntry& out);


/** Digest carries { node_id, seq_no } per known peer: 4 bytes each. This is the
 *  whole point of the digest -- 4 bytes to say "I have version N of peer X"
 *  versus 28 to send the entry itself. */
static const uint32_t CA_TBL_DIGEST_ENTRY_BYTES = 4;

/** One line of a digest: "I hold version seqNo of nodeId". Exactly the 4 bytes the
 *  constant above has always charged for -- 2 for the id, 2 for the sequence. */
struct CaTblDigestItem
{
    uint16_t nodeId = 0;
    uint16_t seqNo = 0;
};

/** Cap on how many summary lines one digest carries.
 *
 *  A digest is only cheap while it is a SUMMARY. At SF12/125 kHz a 50-byte frame is
 *  already 1.5-2 s of airtime, so an unbounded summary would spend more of the duty
 *  budget than the entries it is trying to avoid sending. Twelve keeps a digest inside
 *  one small frame; a node holding more than twelve peers sends the twelve it has heard
 *  from most recently, which is the set a responder is most likely to be able to act on.
 */
static const uint32_t CA_TBL_DIGEST_MAX_ITEMS = 12;

/** Network sect 6. One held UWB slot, as a GRANT_GOSSIP frame carries it: the same three
 *  fields an ESC_GRANT already carries for its slot -- target_id(2), slot_start_ms(2),
 *  slot_len_ms(2) -- so 6 bytes on the wire. The issuing anchor is the frame's origin_id
 *  and is not repeated per item. Times are held as absolute seconds here because this is
 *  one simulation clock; on hardware they would be offsets from the frame's own time. */
struct CaTblGrantItem
{
    uint16_t targetId = 0;
    double startS = 0.0;
    double endS = 0.0;
};

static const uint32_t CA_TBL_GRANT_ITEM_BYTES = 6;

/** The uniform MAC header: packet_id(4) origin_id(2) hop_limit(1) msg_type(1) tier(1). */
static const uint32_t CA_TBL_MAC_HEADER_BYTES = 9;

/**
 * \brief Payload bytes for a message type.
 *
 * \param t        message type
 * \param nEntries table entries (ENTRY_PUSH / RESPONSE), digest entries (DIGEST),
 *                 or manifest ids (QUERY). Ignored for fixed-size types.
 */
uint32_t CaTblPayloadBytes(CaTblMsgType t, uint32_t nEntries);

// ---------------------------------------------------------------------------
// Time on air
// ---------------------------------------------------------------------------

/**
 * \brief LoRa time-on-air, Semtech AN1200.13 / SX1276 datasheet. Borrowed, unmodified.
 *
 *   T_symbol  = 2^SF / BW
 *   T_preamble = (n_preamble + 4.25) * T_symbol
 *   n_payload  = 8 + max( ceil[(8*PL - 4*SF + 28 + 16*CRC - 20*H)
 *                              / (4*(SF - 2*DE))] * (CR + 4), 0 )
 *   T_payload = n_payload * T_symbol
 *   T_packet  = T_preamble + T_payload
 *
 * \param payloadBytes PL, MAC header included -- pass
 *                     CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(...)
 * \param sf           spreading factor 5..12
 * \param bandwidthHz  BW
 * \param cr           coding rate index 1..4 (4/5 .. 4/8)
 * \param explicitHeader true => H = 0
 * \param crcOn        CRC field
 * \param lowDataRateOpt DE; the datasheet mandates it for T_symbol > 16 ms,
 *                       which is SF11/SF12 at 125 kHz
 *
 * The DE term divides (SF - 2*DE), so it must not be set at low SF: at SF5 with
 * DE=1 the denominator is 4*(5-2)=12, which silently inflates the symbol count.
 * CaTblRecommendedLowDataRateOpt() applies the datasheet rule instead of leaving
 * it to the caller.
 */
Time CaTblTimeOnAir(uint32_t payloadBytes,
                    uint8_t sf,
                    double bandwidthHz,
                    uint8_t cr = 1,
                    bool explicitHeader = true,
                    bool crcOn = true,
                    bool lowDataRateOpt = false,
                    uint32_t nPreamble = 8);

/** Datasheet rule: enable DE when T_symbol > 16 ms. */
bool CaTblRecommendedLowDataRateOpt(uint8_t sf, double bandwidthHz);

/** T_symbol = 2^SF / BW. */
Time CaTblSymbolTime(uint8_t sf, double bandwidthHz);

// ---------------------------------------------------------------------------
// The frame
// ---------------------------------------------------------------------------

/**
 * \brief The uniform 9-byte CA-TBL MAC header (§2.7).
 *
 * Uniformity is deliberate and is the reason the dedup/flood logic in §2.6 can be
 * message-type-agnostic: every frame exposes packet_id and hop_limit at the same
 * offsets regardless of payload, so a relay can forward a message type it does
 * not understand. That property is worth preserving -- do not add per-type
 * fields to this header; put them in the payload.
 */
class CaTblHeader : public Header
{
  public:
    CaTblHeader();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    void SetPacketId(uint32_t id) { m_packetId = id; }
    uint32_t GetPacketId() const { return m_packetId; }

    void SetOriginId(uint16_t id) { m_originId = id; }
    uint16_t GetOriginId() const { return m_originId; }

    void SetHopLimit(uint8_t h) { m_hopLimit = h; }
    uint8_t GetHopLimit() const { return m_hopLimit; }

    /** Decrement for forwarding. Returns false if the frame is expired and must
     *  be dropped rather than rebroadcast. */
    bool DecrementHopLimit();

    void SetMsgType(CaTblMsgType t) { m_msgType = t; }
    CaTblMsgType GetMsgType() const { return m_msgType; }

    /** Tier the frame was sent on. 0 = 868 MHz, 1 = 2.4 GHz, 2 = UWB, matching
     *  CaTblTier in ca-tbl-energy-model.h. */
    void SetTier(uint8_t tier) { m_tier = tier; }
    uint8_t GetTier() const { return m_tier; }

    /** Hops already traversed = initial hop limit - current. Needed because the
     *  PCS hop-provenance term is a function of how far a fix has travelled, and
     *  a relay cannot know that from hop_limit alone. */
    void SetInitialHopLimit(uint8_t h) { m_initialHopLimit = h; }

    /**
     * \brief Which target identity a QUERY is scoped to. C3.
     *
     * Meaningful only on CA_TBL_MSG_QUERY. A responder answers only when this
     * matches its own CaTblMacConfig::targetManifestId, which is what makes the
     * search identity-scoped rather than a broadcast every target answers.
     *
     * CARRIED BUT NOT SERIALISED, exactly like m_initialHopLimit below, and for
     * the same reason: it costs no wire bytes here because GetSerializedSize()
     * is the fixed 9-byte MAC header and CaTblPayloadBytes(QUERY, nEntries)
     * ALREADY charges a query for carrying manifest ids. On real hardware the
     * id occupies those payload bytes, which are already billed; in simulation
     * it rides here rather than through a serialiser that would re-derive the
     * same size.
     */
    void SetQueryManifestId(uint8_t m) { m_queryManifestId = m; }
    void SetSelfEntry(const CaTblTableEntry& e)
    {
        m_selfEntry = e;
        m_hasSelfEntry = true;
    }
    const CaTblTableEntry& GetSelfEntry() const { return m_selfEntry; }
    bool HasSelfEntry() const { return m_hasSelfEntry; }

    /**
     * The anti-entropy summary a DIGEST is supposed to carry: {node_id, seq_no} per
     * known peer, 4 bytes each.
     *
     * THE AIRTIME WAS ALREADY BEING BILLED FOR THIS. CA_TBL_DIGEST_ENTRY_BYTES has
     * existed since S4 and CaTblPayloadBytes() charges nEntries * 4 for a DIGEST, with
     * the comment explaining the whole point -- "4 bytes to say I have version N of
     * peer X versus 28 to send the entry itself". The frame paid for the summary and
     * carried nothing, so the responder could not tell what the asker was missing and
     * fell back to pushing its own entry regardless.
     *
     * Without this, a digest exchange is a request for one specific thing dressed as a
     * request for everything. With it, the responder can answer what was actually
     * asked, which is the mechanism anti-entropy is.
     */
    void SetDigest(const std::vector<CaTblDigestItem>& d) { m_digest = d; }
    const std::vector<CaTblDigestItem>& GetDigest() const { return m_digest; }

    /** Network sect 6. The slots a GRANT_GOSSIP frame reports its origin as holding.
     *  Carried, not serialised, like the digest: CaTblPayloadBytes(GRANT_GOSSIP, n)
     *  bills the 6 bytes per item, so writing them again would double-count airtime. */
    void SetGrants(const std::vector<CaTblGrantItem>& g) { m_grants = g; }
    const std::vector<CaTblGrantItem>& GetGrants() const { return m_grants; }

    uint8_t GetQueryManifestId() const { return m_queryManifestId; }

    /**
     * \brief C4. The two arbitration terms an anchor cannot observe for itself.
     *
     * U(r) = l1*T_deg + l2*T_batt + l3*T_stale + l4*T_query. The anchor can see
     * T_stale (when it last granted this target) and T_query (the frame's access
     * category). It cannot see T_deg, which is the slope of the requester's own
     * PCS history, or T_batt, which is its remaining charge. Those are private to
     * the requester and must travel with the request, or the arbiter is scoring a
     * different function under the same name.
     *
     * Both are quantised to a byte, 0..255 mapping [0,1]. Carried and not
     * serialised, like m_queryManifestId: GetSerializedSize() is the fixed 9-byte
     * MAC header and CaTblPayloadBytes already sizes ESC_REQUEST, so the billed
     * airtime does not change.
     */
    void SetReqDegradation(uint8_t d) { m_reqDegradation = d; }
    uint8_t GetReqDegradation() const { return m_reqDegradation; }
    void SetReqBattery(uint8_t b) { m_reqBattery = b; }
    uint8_t GetReqBattery() const { return m_reqBattery; }

    /**
     * \brief MC-1 (--handshakeFixes). What a reply answers, what a GRANT says, and
     * which tier a request asks for.
     *
     * CaTblPayloadBytes already bills an ACK for "acked packet_id(4)", a GRANT for
     * "granted(1) ... queue_pos(1)" and a request for its terms, and until MC-1 none of
     * them was carried: an ACK cleared its receiver's OLDEST pending frame, whatever it
     * acknowledged, and every GRANT meant granted. Carried, not serialised, like the C4
     * terms above, because the bytes are already billed. All of them keep their defaults
     * unless the MAC runs with handshakeFixes, and the event payload prints them only
     * when they differ from those defaults -- so only under the flag.
     */
    void SetAckedPacketId(uint32_t id) { m_ackedPacketId = id; }
    uint32_t GetAckedPacketId() const { return m_ackedPacketId; }
    void SetGrantVerdict(bool granted, uint8_t queuePos)
    {
        m_hasGrantVerdict = true;
        m_granted = granted;
        m_queuePos = queuePos;
    }
    bool HasGrantVerdict() const { return m_hasGrantVerdict; }
    bool GetGranted() const { return m_granted; }
    uint8_t GetQueuePos() const { return m_queuePos; }
    //! 0xFF: not stated, which is every request outside --handshakeFixes.
    void SetReqTargetTier(uint8_t t) { m_reqTargetTier = t; }
    uint8_t GetReqTargetTier() const { return m_reqTargetTier; }

    /**
     * \brief MC-2 (--triggeredUpdates). Why a TRIGGERED_UPDATE was sent: 1 = a large
     * correction, 2 = the last gasp before a flat battery silences the radio. 0 = not
     * stated, which is every frame outside the flag, including StartHandshake's
     * RECEDING push. CaTblPayloadBytes already bills "a reason code" byte on every
     * TRIGGERED_UPDATE, so it is carried, not serialised.
     */
    void SetTriggerReason(uint8_t r) { m_triggerReason = r; }
    uint8_t GetTriggerReason() const { return m_triggerReason; }

    /**
     * \brief NW-8 (--reversePathHop). A RESPONSE names the QUERY it answers and the node
     * that asked, so each hop of the reverse path can look up ITS OWN cached previous hop
     * and the requester can tell that the answer reached it; forceFlood marks the
     * fallback copy that floods although RESPONSE is not a flooded type. Carried, not
     * serialised; the MAC bills their eight bytes (query_id 4, requester 2, answerer 2)
     * under the flag. answererId is the node whose answer this is (the target, or the proxy
     * answering for it), unchanged by forwarding: a lost ACK makes the sender retransmit
     * under a fresh packet id, which dedup cannot catch, so (query, answerer) is what a
     * forwarder and the requester deduplicate on. 0 / 0 / 0 / false outside the flag.
     */
    void SetRespRoute(uint32_t queryId, uint16_t requesterId, uint16_t answererId,
                      bool forceFlood)
    {
        m_respQueryId = queryId;
        m_respRequesterId = requesterId;
        m_respAnswererId = answererId;
        m_forceFlood = forceFlood;
    }
    uint16_t GetRespAnswererId() const { return m_respAnswererId; }
    uint32_t GetRespQueryId() const { return m_respQueryId; }
    uint16_t GetRespRequesterId() const { return m_respRequesterId; }
    bool GetForceFlood() const { return m_forceFlood; }
    uint8_t GetHopsTraversed() const;

    CaTblAccessCategory GetAccessCategory() const { return CaTblAccessCategoryFor(m_msgType); }
    CaTblAckPolicy GetAckPolicy() const { return CaTblAckPolicyFor(m_msgType); }

  private:
    uint32_t m_packetId;
    uint16_t m_originId;
    uint8_t m_hopLimit;
    CaTblMsgType m_msgType;
    uint8_t m_tier;

    // Not on the wire: the sender's initial hop limit, carried locally so
    // GetHopsTraversed() works within one simulation. On real hardware this would
    // have to be an explicit payload field -- flagged so the report does not
    // claim a 9-byte header that secretly needs a tenth byte.
    uint8_t m_initialHopLimit;
    //! The sender's own entry, riding an ENTRY_PUSH. CARRIED, NOT SERIALISED --
    //! CaTblPayloadBytes already charges for it, so writing the bytes again would
    //! double-count the airtime this design exists to measure.
    CaTblTableEntry m_selfEntry;
    std::vector<CaTblDigestItem> m_digest;
    std::vector<CaTblGrantItem> m_grants; //!< sect 6, carried not serialised -- see above
    bool m_hasSelfEntry;
    uint8_t m_queryManifestId = 0; //!< C3, carried not serialised -- see above
    uint8_t m_reqDegradation = 0;  //!< C4, quantised T_deg input
    uint8_t m_reqBattery = 0;      //!< C4, quantised T_batt input
    uint32_t m_ackedPacketId = 0;  //!< MC-1, carried not serialised -- see above
    bool m_hasGrantVerdict = false; //!< MC-1
    bool m_granted = true;          //!< MC-1
    uint8_t m_queuePos = 0;         //!< MC-1; no queue exists until NW-5
    uint8_t m_reqTargetTier = 0xFF; //!< MC-1
    uint8_t m_triggerReason = 0;    //!< MC-2, carried not serialised -- see above
    uint32_t m_respQueryId = 0;     //!< NW-8, carried not serialised -- see above
    uint16_t m_respRequesterId = 0; //!< NW-8
    uint16_t m_respAnswererId = 0;  //!< NW-8
    bool m_forceFlood = false;      //!< NW-8
};

} // namespace ns3

#endif // CA_TBL_HEADER_H
