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
    CA_TBL_MSG_ACK = 9               //!< explicit ACK; only ever for ESC_REQUEST/GRANT (§2.8)
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

/** Digest carries { node_id, seq_no } per known peer: 4 bytes each. This is the
 *  whole point of the digest -- 4 bytes to say "I have version N of peer X"
 *  versus 28 to send the entry itself. */
static const uint32_t CA_TBL_DIGEST_ENTRY_BYTES = 4;

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
    uint8_t m_queryManifestId = 0; //!< C3, carried not serialised -- see above
    uint8_t m_reqDegradation = 0;  //!< C4, quantised T_deg input
    uint8_t m_reqBattery = 0;      //!< C4, quantised T_batt input
};

} // namespace ns3

#endif // CA_TBL_HEADER_H
