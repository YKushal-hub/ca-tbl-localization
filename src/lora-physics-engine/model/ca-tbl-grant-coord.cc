#include "ca-tbl-grant-coord.h"

#include <algorithm>

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

bool
CaTblGrantCoordinator::NoteGossipedGrant(const CaTblGrantRecord& g)
{
    // A gossiped copy of our own grant is not new information. Without this an anchor
    // hearing its own grant reflected back would find a "conflict" with itself and
    // defer to nobody.
    if (g.anchorId == m_selfId)
    {
        return false;
    }
    for (size_t i = 0; i < m_grants.size(); ++i)
    {
        const CaTblGrantRecord& h = m_grants[i];
        if (h.anchorId == g.anchorId && h.targetId == g.targetId && h.startS == g.startS &&
            h.endS == g.endS)
        {
            return false;
        }
    }
    m_grants.push_back(g);
    return true;
}

CaTblGrantVerdict
CaTblGrantCoordinator::Reconcile(const CaTblGrantRecord& heard)
{
    // Only NEW information is reconciled. A repeat of a record already held was
    // settled when it first arrived; counting it again would inflate the contest
    // counters by however often the neighbour happens to gossip.
    if (!NoteGossipedGrant(heard))
    {
        return CA_TBL_GRANT_OK;
    }

    bool contested = false;
    for (size_t i = 0; i < m_grants.size(); ++i)
    {
        const CaTblGrantRecord& g = m_grants[i];
        if (g.anchorId == m_selfId && g.targetId == heard.targetId &&
            g.OverlapsInTime(heard.startS, heard.endS))
        {
            contested = true;
            break;
        }
    }
    if (!contested)
    {
        return CA_TBL_GRANT_OK;
    }

    m_lastConflict = heard.anchorId;
    if (heard.anchorId > m_selfId)
    {
        // THE RULE, unchanged from Evaluate(): the lower id defers. Our overlapping
        // grants for this target are dropped -- yielded -- so we stop gossiping them and
        // stop treating the slot as ours.
        std::vector<CaTblGrantRecord> keep;
        keep.reserve(m_grants.size());
        for (size_t i = 0; i < m_grants.size(); ++i)
        {
            const CaTblGrantRecord& g = m_grants[i];
            if (g.anchorId == m_selfId && g.targetId == heard.targetId &&
                g.OverlapsInTime(heard.startS, heard.endS))
            {
                continue;
            }
            keep.push_back(g);
        }
        m_grants.swap(keep);
        ++m_defers;
        return CA_TBL_GRANT_DEFER_LOWER_ID;
    }
    ++m_wins;
    return CA_TBL_GRANT_WIN_HIGHER_ID;
}

std::vector<CaTblGrantRecord>
CaTblGrantCoordinator::OwnActiveGrants(double nowS) const
{
    std::vector<CaTblGrantRecord> out;
    for (size_t i = 0; i < m_grants.size(); ++i)
    {
        if (m_grants[i].anchorId == m_selfId && m_grants[i].endS > nowS)
        {
            out.push_back(m_grants[i]);
        }
    }
    return out;
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
    uint16_t higherId = 0;
    uint16_t lowerId = 0;

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
            lowerId = g.anchorId;
        }
        else
        {
            conflictWithHigher = true;
            higherId = std::max(higherId, g.anchorId);
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
        m_lastConflict = higherId;
        return CA_TBL_GRANT_DEFER_LOWER_ID;
    }
    if (conflictWithLower)
    {
        ++m_wins;
        m_lastConflict = lowerId;
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
