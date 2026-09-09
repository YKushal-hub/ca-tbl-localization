#include "ca-tbl-routing.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ns3
{

CaTblRoute::CaTblRoute()
    : nodeId(0), nextHopId(0), hopCount(0), pcs(0.0), pathConfidencePenalty(0.0),
      lastUpdatedS(0.0), seqNo(0), sumNlos(0.0), sumRssiVar(0.0), sumJitter(0.0),
      etx(0.0), valid(false)
{
}

//! Modular supersession, same rule as the position table: a wrapping counter compared with
//! a plain `>` would freeze the table the first time a node wrapped.
static bool
SeqNewer(uint16_t a, uint16_t b)
{
    return static_cast<uint16_t>(a - b) != 0 && static_cast<uint16_t>(a - b) < 0x8000;
}

CaTblRoutingTable::CaTblRoutingTable()
    : m_divergences(0), m_selections(0)
{
}

void
CaTblRoutingTable::Update(const CaTblRoute& r)
{
    if (!r.valid)
    {
        return;
    }
    const std::pair<uint16_t, uint16_t> key(r.nodeId, r.nextHopId);
    std::map<std::pair<uint16_t, uint16_t>, CaTblRoute>::iterator it = m_routes.find(key);
    if (it == m_routes.end() || SeqNewer(r.seqNo, it->second.seqNo))
    {
        m_routes[key] = r;
    }
}

void
CaTblRoutingTable::Expire(double nowS, double ttlS)
{
    // A stale route is worse than no route: it names a next hop that may no longer exist,
    // and the sender cannot discover that except by losing the frame.
    std::map<std::pair<uint16_t, uint16_t>, CaTblRoute>::iterator it = m_routes.begin();
    while (it != m_routes.end())
    {
        if (nowS - it->second.lastUpdatedS > ttlS)
        {
            m_routes.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

double
CaTblRoutingTable::Cost(const CaTblRoute& r, CaTblRouteMetric metric) const
{
    if (metric == CA_TBL_ROUTE_ETX)
    {
        // Borrowed, unmodified. Fall back to hop count when no delivery-ratio estimate
        // has been observed yet, which is the standard cold-start behaviour and is what
        // makes the baseline usable from the first cycle instead of after convergence.
        return (r.etx > 0.0) ? r.etx : static_cast<double>(r.hopCount);
    }

    // THE CONTRIBUTED METRIC. Sums along the chain rather than averaging: a five-hop path
    // with mild per-hop exposure really is worse than a one-hop path with the same per-hop
    // figure, and an average would hide precisely that -- which is the failure the whole
    // hop-aware idea exists to correct.
    return m_w.alphaNlos * r.sumNlos + m_w.betaRssiVar * r.sumRssiVar +
           m_w.gammaJitter * r.sumJitter;
}

std::vector<CaTblRoute>
CaTblRoutingTable::RoutesTo(uint16_t dest) const
{
    std::vector<CaTblRoute> out;
    for (std::map<std::pair<uint16_t, uint16_t>, CaTblRoute>::const_iterator it =
             m_routes.begin();
         it != m_routes.end(); ++it)
    {
        if (it->first.first == dest && it->second.valid)
        {
            out.push_back(it->second);
        }
    }
    return out;
}

bool
CaTblRoutingTable::SelectRoute(uint16_t dest,
                               CaTblRouteMetric metric,
                               CaTblTrafficClass traffic,
                               CaTblRoute& out) const
{
    const std::vector<CaTblRoute> cand = RoutesTo(dest);
    if (cand.empty())
    {
        return false;
    }

    // THE SPLIT IS PART OF THE CLAIM, NOT A TUNABLE. Gossip carries no localization
    // payload, so there is no uncertainty to minimise and no reason to spend extra hops.
    // Routing it on the confidence metric would make this a general-purpose routing
    // contribution, which is a different and well-studied claim.
    const CaTblRouteMetric effective =
        (traffic == CA_TBL_TRAFFIC_GOSSIP) ? CA_TBL_ROUTE_ETX : metric;

    // ETX's own choice, always computed: it is both the baseline answer and the reference
    // the divergence counter needs. Without it "the confidence metric is enabled" and
    // "the confidence metric changed something" are indistinguishable.
    size_t bestEtx = 0;
    double bestEtxCost = std::numeric_limits<double>::infinity();
    uint8_t minHops = 255;
    for (size_t i = 0; i < cand.size(); ++i)
    {
        const double c = Cost(cand[i], CA_TBL_ROUTE_ETX);
        if (c < bestEtxCost)
        {
            bestEtxCost = c;
            bestEtx = i;
        }
        minHops = std::min(minHops, cand[i].hopCount);
    }

    size_t chosen = bestEtx;
    if (effective == CA_TBL_ROUTE_CONFIDENCE)
    {
        // SLACK BOUNDS THE SEARCH, AND THE BOUND IS LOAD-BEARING. Unbounded, the router
        // would chase a cleaner path through arbitrarily many hops -- and each extra hop
        // adds another relay's own uncertainty to the fix being carried, so a metric that
        // ignored the cap would degrade the very quantity it is minimising.
        const int hopCap = static_cast<int>(minHops) + static_cast<int>(m_w.slack);
        double best = std::numeric_limits<double>::infinity();
        bool found = false;
        for (size_t i = 0; i < cand.size(); ++i)
        {
            if (static_cast<int>(cand[i].hopCount) > hopCap)
            {
                continue;
            }
            const double c = Cost(cand[i], CA_TBL_ROUTE_CONFIDENCE);
            if (!found || c < best)
            {
                best = c;
                chosen = i;
                found = true;
            }
        }
        // If the cap admitted nothing -- impossible while minHops came from this same
        // set, but cheap to be explicit about -- fall back to the baseline rather than
        // failing to route. Refusing to send a fix because no path was clean enough would
        // convert a quality problem into an availability one.
        if (!found)
        {
            chosen = bestEtx;
        }
    }

    ++m_selections;
    if (cand[chosen].nextHopId != cand[bestEtx].nextHopId)
    {
        ++m_divergences;
    }
    out = cand[chosen];
    return true;
}

} // namespace ns3
