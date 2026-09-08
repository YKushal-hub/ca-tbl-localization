#include "ca-tbl-grant-coord.h"

namespace ns3
{

CaTblGrantCoordinator::CaTblGrantCoordinator(uint16_t selfAnchorId)
    : m_selfId(selfAnchorId)
{
}

void
CaTblGrantCoordinator::NoteOwnGrant(uint16_t targetId, double startS, double endS)
{
    CaTblGrantRecord g;
    g.anchorId = m_selfId;
    g.targetId = targetId;
    g.startS = startS;
    g.endS = endS;
    m_grants.push_back(g);
}

void
CaTblGrantCoordinator::NoteGossipedGrant(const CaTblGrantRecord& g)
{
    // A gossiped copy of our own grant is not new information. Without this an anchor
    // hearing its own grant reflected back would find a "conflict" with itself and
    // defer to nobody.
    if (g.anchorId == m_selfId)
    {
        return;
    }
    m_grants.push_back(g);
}

void
CaTblGrantCoordinator::Expire(double nowS)
{
    std::vector<CaTblGrantRecord> keep;
    keep.reserve(m_grants.size());
    for (size_t i = 0; i < m_grants.size(); ++i)
    {
        if (m_grants[i].endS > nowS)
        {
            keep.push_back(m_grants[i]);
        }
    }
    m_grants.swap(keep);
}

CaTblGrantVerdict
CaTblGrantCoordinator::Evaluate(uint16_t targetId, double startS, double endS) const
{
    bool conflictWithLower = false;
    bool conflictWithHigher = false;

    for (size_t i = 0; i < m_grants.size(); ++i)
    {
        const CaTblGrantRecord& g = m_grants[i];
        if (g.targetId != targetId || !g.OverlapsInTime(startS, endS))
        {
            continue;
        }
        if (g.anchorId == m_selfId)
        {
            // Already holding this window ourselves. Re-issuing would double-charge the
            // slot and tell the node the same thing twice.
            return CA_TBL_GRANT_SELF_REPEAT;
        }
        if (g.anchorId < m_selfId)
        {
            conflictWithLower = true;
        }
        else
        {
            conflictWithHigher = true;
        }
    }

    // THE RULE, and it is the whole mechanism: the LOWER id defers. So this anchor
    // yields when it knows of an overlapping grant from a HIGHER id, and proceeds when
    // the conflict is only with lower ids -- because those anchors, applying the same
    // rule against us, are deferring.
    //
    // Both directions are counted, and both are informative. Zero of each means the
    // anchors never contended and this mechanism was never exercised at all, which is
    // a thing a demo must be able to say about itself.
    if (conflictWithHigher)
    {
        ++m_defers;
        return CA_TBL_GRANT_DEFER_LOWER_ID;
    }
    if (conflictWithLower)
    {
        ++m_wins;
        return CA_TBL_GRANT_WIN_HIGHER_ID;
    }
    return CA_TBL_GRANT_OK;
}

const char*
CaTblGrantCoordinator::VerdictName(CaTblGrantVerdict v)
{
    switch (v)
    {
    case CA_TBL_GRANT_OK:
        return "OK";
    case CA_TBL_GRANT_DEFER_LOWER_ID:
        return "DEFER";
    case CA_TBL_GRANT_WIN_HIGHER_ID:
        return "WIN";
    default:
        return "SELF_REPEAT";
    }
}

} // namespace ns3
