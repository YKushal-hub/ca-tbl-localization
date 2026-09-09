/*
 * CA-TBL Network sect 6 -- multi-anchor grant coordination.
 *
 * THE PROBLEM IS REAL AND IT IS NARROW. A SAR area has several anchors, each running its
 * own U(r) arbitration over its own request queue. Two anchors that can both hear the
 * same field node will both decide to grant it a UWB slot, and neither knows the other
 * did. The node is then told twice to occupy one radio for two overlapping windows, and
 * whichever grant it honours, the other anchor has spent a slot on a target that is not
 * listening to it.
 *
 * DELIBERATELY NOT A CONSENSUS PROTOCOL, and the spec says so explicitly: "a
 * deterministic tie-break, not a consensus protocol -- CPS-appropriate given anchors are
 * trusted infrastructure, not adversarial". Paxos over a bounded set of mains-powered
 * gateways in a search area would be over-engineering, and it would spend airtime the
 * duty budget exists to protect.
 *
 * THE RULE: on an overlap for the same target, the LOWER node id defers. Lower rather
 * than higher is arbitrary, but it must be WRITTEN DOWN and identical everywhere, because
 * the entire property this buys is that two anchors reach the same answer without talking
 * about it. A rule that is merely "consistent" is not enough -- it has to be the same
 * rule, and that makes it a specification detail rather than an implementation choice.
 *
 * WHAT THIS DOES NOT SOLVE, and it is worth being clear because the mechanism looks
 * stronger than it is: coordination is only as good as the gossip that feeds it. An
 * anchor that has not heard another's grant will grant anyway. This bounds double-grants
 * to the gossip interval; it does not eliminate them, and a demo that shows it working
 * should show that too.
 */

#ifndef CA_TBL_GRANT_COORD_H
#define CA_TBL_GRANT_COORD_H

#include <cstdint>
#include <vector>

namespace ns3
{

//! One UWB slot grant, either issued here or learned from a neighbouring anchor.
struct CaTblGrantRecord
{
    uint16_t anchorId = 0;  //!< which anchor issued it
    uint16_t targetId = 0;  //!< which field node it was issued to
    double startS = 0.0;
    double endS = 0.0;

    //! Half-open [start, end). Two grants that merely touch do not overlap -- a slot
    //! ending exactly as another begins is a handover, not a collision.
    bool OverlapsInTime(double s, double e) const { return startS < e && s < endS; }
};

//! Why a grant was allowed or refused, so the demo can show the reason and not just
//! the outcome.
enum CaTblGrantVerdict : uint8_t
{
    CA_TBL_GRANT_OK = 0,          //!< no conflict known
    CA_TBL_GRANT_DEFER_LOWER_ID,  //!< a lower-id anchor yields to a higher-id one
    CA_TBL_GRANT_WIN_HIGHER_ID,   //!< conflict exists and this anchor is the higher id
    CA_TBL_GRANT_SELF_REPEAT      //!< this anchor already holds an overlapping grant here
};

/**
 * \brief Tracks grants this anchor issued and grants it learned by gossip.
 *
 * Bounded by expiry rather than by count: a grant that has ended carries no information
 * and keeping it would make the tie-break depend on how long the process has been
 * running. `Expire()` must be called; nothing self-cleans on a timer here because the
 * caller owns the clock.
 */
class CaTblGrantCoordinator
{
  public:
    explicit CaTblGrantCoordinator(uint16_t selfAnchorId);

    //! Record a grant issued by this anchor.
    void NoteOwnGrant(uint16_t targetId, double startS, double endS);

    //! Record a grant heard from another anchor's gossip.
    void NoteGossipedGrant(const CaTblGrantRecord& g);

    //! Drop everything that ended before \p nowS.
    void Expire(double nowS);

    /**
     * \brief May this anchor grant \p targetId the window [startS, endS)?
     *
     * Returns the verdict rather than a bare bool so a caller can distinguish "no
     * conflict" from "conflict, and I won" -- the second is the case worth logging,
     * because it means a slot was contested and the tie-break did work.
     */
    CaTblGrantVerdict Evaluate(uint16_t targetId, double startS, double endS) const;

    //! Convenience: true only for verdicts that permit the grant.
    static bool Permits(CaTblGrantVerdict v)
    {
        return v == CA_TBL_GRANT_OK || v == CA_TBL_GRANT_WIN_HIGHER_ID;
    }

    static const char* VerdictName(CaTblGrantVerdict v);

    size_t Size() const { return m_grants.size(); }
    uint16_t SelfId() const { return m_selfId; }

    //! Contested windows this anchor won, and deferred. Counted so a demo can show
    //! that coordination happened at all -- zero of both means the anchors never
    //! actually contended and the mechanism was never exercised.
    uint64_t Wins() const { return m_wins; }
    uint64_t Defers() const { return m_defers; }

  private:
    uint16_t m_selfId;
    std::vector<CaTblGrantRecord> m_grants;
    mutable uint64_t m_wins = 0;
    mutable uint64_t m_defers = 0;
};

} // namespace ns3

#endif // CA_TBL_GRANT_COORD_H
