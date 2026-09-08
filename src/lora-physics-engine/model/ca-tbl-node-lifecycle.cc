#include "ca-tbl-node-lifecycle.h"

namespace ns3
{

CaTblNodeState
CaTblNodeLifecycle::NoteContact(uint16_t nodeId, double nowS)
{
    CaTblNodeRecord& r = m_nodes[nodeId];
    const bool firstEver = (r.lastHeardS < 0.0);

    if (firstEver)
    {
        r.nodeId = nodeId;
        r.firstHeardS = nowS;
    }

    // REAPPEARANCE IS ITS OWN EVENT. Coming back after LOST is not the same as being
    // seen for the first time, and collapsing them reports a fresh discovery for
    // someone already found and lost once.
    const bool wasLost = (r.state == CA_TBL_NODE_LOST);
    if (wasLost)
    {
        r.state = CA_TBL_NODE_REACQUIRED;
        ++r.reacquisitions;
        r.stateSinceS = nowS;
    }
    else if (r.state != CA_TBL_NODE_REACQUIRED)
    {
        // Holding REACQUIRED here is deliberate: further contact during the hold window
        // must not cancel the announcement before a consumer has had a chance to see it.
        if (r.state != CA_TBL_NODE_TRACKED)
        {
            r.stateSinceS = nowS;
        }
        r.state = CA_TBL_NODE_TRACKED;
    }

    r.lastHeardS = nowS;
    return r.state;
}

void
CaTblNodeLifecycle::Tick(double nowS)
{
    for (std::map<uint16_t, CaTblNodeRecord>::iterator it = m_nodes.begin();
         it != m_nodes.end(); ++it)
    {
        CaTblNodeRecord& r = it->second;
        if (r.lastHeardS < 0.0)
        {
            continue;
        }
        const double age = nowS - r.lastHeardS;

        // REACQUIRED settles to TRACKED once the hold expires, provided contact is still
        // recent. It is an announcement with a lifetime, not a place to live.
        if (r.state == CA_TBL_NODE_REACQUIRED)
        {
            if (nowS - r.stateSinceS >= m_bounds.reacquiredHoldS)
            {
                r.state = (age > m_bounds.lostS)      ? CA_TBL_NODE_LOST
                          : (age > m_bounds.degradedS) ? CA_TBL_NODE_DEGRADED
                                                       : CA_TBL_NODE_TRACKED;
                r.stateSinceS = nowS;
                if (r.state == CA_TBL_NODE_LOST)
                {
                    ++r.lossEvents;
                    r.everLost = true;
                }
            }
            continue;
        }

        CaTblNodeState want = CA_TBL_NODE_TRACKED;
        if (age > m_bounds.lostS)
        {
            want = CA_TBL_NODE_LOST;
        }
        else if (age > m_bounds.degradedS)
        {
            want = CA_TBL_NODE_DEGRADED;
        }

        if (want != r.state)
        {
            // Count the TRANSITION into loss, not the condition. A node sitting in LOST
            // for an hour has been lost once, and counting the condition would make the
            // number a function of how often Tick() happened to be called.
            if (want == CA_TBL_NODE_LOST)
            {
                ++r.lossEvents;
                r.everLost = true;
            }
            r.state = want;
            r.stateSinceS = nowS;
        }
    }
}

CaTblNodeState
CaTblNodeLifecycle::StateOf(uint16_t nodeId) const
{
    std::map<uint16_t, CaTblNodeRecord>::const_iterator it = m_nodes.find(nodeId);
    return (it == m_nodes.end()) ? CA_TBL_NODE_UNSEEN : it->second.state;
}

bool
CaTblNodeLifecycle::Get(uint16_t nodeId, CaTblNodeRecord& out) const
{
    std::map<uint16_t, CaTblNodeRecord>::const_iterator it = m_nodes.find(nodeId);
    if (it == m_nodes.end())
    {
        return false;
    }
    out = it->second;
    return true;
}

uint32_t
CaTblNodeLifecycle::CountIn(CaTblNodeState s) const
{
    uint32_t n = 0;
    for (std::map<uint16_t, CaTblNodeRecord>::const_iterator it = m_nodes.begin();
         it != m_nodes.end(); ++it)
    {
        if (it->second.state == s) { ++n; }
    }
    return n;
}

uint32_t
CaTblNodeLifecycle::TotalLossEvents() const
{
    uint32_t n = 0;
    for (std::map<uint16_t, CaTblNodeRecord>::const_iterator it = m_nodes.begin();
         it != m_nodes.end(); ++it)
    {
        n += it->second.lossEvents;
    }
    return n;
}

uint32_t
CaTblNodeLifecycle::TotalReacquisitions() const
{
    uint32_t n = 0;
    for (std::map<uint16_t, CaTblNodeRecord>::const_iterator it = m_nodes.begin();
         it != m_nodes.end(); ++it)
    {
        n += it->second.reacquisitions;
    }
    return n;
}

const char*
CaTblNodeLifecycle::StateName(CaTblNodeState s)
{
    switch (s)
    {
    case CA_TBL_NODE_UNSEEN:     return "UNSEEN";
    case CA_TBL_NODE_TRACKED:    return "TRACKED";
    case CA_TBL_NODE_DEGRADED:   return "DEGRADED";
    case CA_TBL_NODE_LOST:       return "LOST";
    default:                     return "REACQUIRED";
    }
}

} // namespace ns3
