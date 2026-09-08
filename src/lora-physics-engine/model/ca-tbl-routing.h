/*
 * CA-TBL Network Layer §2/§3 -- routing table, ETX baseline, and confidence-weighted path
 * selection for position-fix traffic.
 *
 * THIS IS THE CLAIMED CONTRIBUTION, SO ITS SCOPE MUST BE STATED NARROWLY.
 *
 * Link-quality-aware routing is old, standard MANET material -- ETX and ETT are decades
 * old and are BORROWED here unmodified. What is contributed is substituting
 * localization-uncertainty terms into the routing cost function SPECIFICALLY WHEN RELAYING
 * POSITION DATA: letting the confidence model reach down into path selection. That is a
 * much smaller claim than "confidence-weighted routing", and writing it any larger invites
 * a reviewer to answer it with the general weighted-routing literature.
 *
 * THE TRAFFIC SPLIT IS PART OF THE CLAIM, NOT AN OPTIMISATION. Routine digest and
 * discovery gossip keeps plain ETX, because there is no localization payload to protect
 * and no uncertainty to minimise. Only ENTRY_PUSH and TRIGGERED_UPDATE -- the messages
 * that actually carry a fix -- are routed on the confidence metric. Omitting this split
 * would be claiming a general-purpose routing contribution, which is a different and
 * well-studied problem.
 *
 * SLACK IS CAPPED AND THE CAP IS LOAD-BEARING. Unbounded slack lets the router chase a
 * cleaner path through arbitrarily many hops; each extra hop costs latency, airtime, and
 * -- the part that makes it self-defeating -- another relay's own uncertainty added to the
 * fix being carried. A confidence metric that degrades confidence to satisfy itself is
 * worse than the hop-count metric it replaced.
 */

#ifndef CA_TBL_ROUTING_H
#define CA_TBL_ROUTING_H

#include <cstdint>
#include <map>
#include <vector>

namespace ns3
{

/**
 * \brief One routing table row (Network §2).
 *
 * pathConfidencePenalty IS NOT PCS. PCS scores how much a POSITION FIX is believed.
 * This scores how much a RELAY CHAIN degrades whatever it carries -- accumulated NLOS and
 * variance exposure along the specific path this entry took to arrive, whether or not the
 * entry is itself a fix. Conflating them was the first thing that had to be kept apart:
 * a perfectly confident fix can arrive over an appalling path, and a hopeless fix can
 * arrive over a clean one.
 */
struct CaTblRoute
{
    uint16_t nodeId;      //!< destination
    uint16_t nextHopId;   //!< neighbour to hand it to
    uint8_t hopCount;
    double pcs;           //!< confidence in the position this route last carried
    double pathConfidencePenalty; //!< accumulated NLOS/variance exposure along the chain
    double lastUpdatedS;
    uint16_t seqNo;

    //! Per-hop terms behind the penalty, retained so the cost can be recomputed when the
    //! weights change without re-walking the mesh. Sums along the chain, not averages:
    //! a five-hop path with mild exposure per hop really is worse than a one-hop path
    //! with the same per-hop figure, and an average would hide exactly that.
    double sumNlos;
    double sumRssiVar;
    double sumJitter;

    //! Link-delivery ratio product along the chain, for the ETX baseline. ETX is the
    //! EXPECTED TRANSMISSION COUNT, so it is 1/(df*dr) per hop and additive along a path.
    double etx;

    bool valid;

    CaTblRoute();
};

/** Which cost function a route request should be scored under (Network §3). */
enum CaTblRouteMetric : uint8_t
{
    //! Standard MANET baseline: fewest expected transmissions. Borrowed, unmodified.
    //! The spec's execution plan says to build and validate this FIRST, so that the
    //! confidence metric has something to be measured against rather than asserted over.
    CA_TBL_ROUTE_ETX = 0,
    //! The contributed metric: minimise accumulated localization uncertainty along the
    //! path, among routes not more than `slack` hops longer than the shortest.
    CA_TBL_ROUTE_CONFIDENCE = 1
};

/** Traffic class, because the metric depends on it (Network §3 scope note). */
enum CaTblTrafficClass : uint8_t
{
    //! Digest, discovery, keepalive. No localization payload, so plain ETX.
    CA_TBL_TRAFFIC_GOSSIP = 0,
    //! ENTRY_PUSH, TRIGGERED_UPDATE -- carries a fix, so it gets the confidence metric.
    CA_TBL_TRAFFIC_POSITION_FIX = 1
};

/** Weights for the confidence cost. Ours, not cited -- see the note in the .cc. */
struct CaTblRoutingWeights
{
    double alphaNlos = 1.0;
    double betaRssiVar = 1.0;
    double gammaJitter = 1.0;
    //! Extra hops tolerated in exchange for a cleaner path. The spec says start at 1 and
    //! tune empirically; it must never be unbounded (see the header note).
    uint8_t slack = 1;
};

/**
 * \brief Routing table with both metrics (Network §2/§3).
 *
 * Holds every known route to a destination rather than one, because choosing between
 * routes is the entire mechanism: a table that already collapsed to a single next hop has
 * made the decision this class exists to make.
 */
class CaTblRoutingTable
{
  public:
    CaTblRoutingTable();

    void SetWeights(const CaTblRoutingWeights& w) { m_w = w; }
    const CaTblRoutingWeights& Weights() const { return m_w; }

    /** Install or supersede a route. Supersession is modular on seqNo, the same rule the
     *  position table uses -- a wrapping counter compared with plain `>` would freeze the
     *  table the first time a node wrapped. */
    void Update(const CaTblRoute& r);

    /** Age out rows not refreshed within \p ttlS. A stale route is worse than no route:
     *  it advertises a next hop that may no longer exist, and the sender has no way to
     *  discover that except by losing the frame. */
    void Expire(double nowS, double ttlS);

    /** Cost of a single route under \p metric. Public so a test can score a route
     *  without going through selection. */
    double Cost(const CaTblRoute& r, CaTblRouteMetric metric) const;

    /**
     * \brief Choose a next hop for \p dest (Network §3).
     *
     * \param traffic  GOSSIP is always routed on ETX regardless of \p metric -- the split
     *                 is part of the claim, not a tunable.
     * \param out      the chosen route, untouched if none is viable
     * \return false when no valid route exists
     */
    bool SelectRoute(uint16_t dest,
                     CaTblRouteMetric metric,
                     CaTblTrafficClass traffic,
                     CaTblRoute& out) const;

    /** Every known route to \p dest, for tests and for the A/B comparison. */
    std::vector<CaTblRoute> RoutesTo(uint16_t dest) const;

    size_t Size() const { return m_routes.size(); }

    //! A/B accounting: how often the confidence metric picked a DIFFERENT next hop than
    //! ETX would have. A metric that never diverges from its baseline cannot be
    //! responsible for any difference in the results, whatever its cost function says --
    //! this is the counter that distinguishes "enabled" from "doing something".
    uint64_t Divergences() const { return m_divergences; }
    uint64_t Selections() const { return m_selections; }

  private:
    //! Keyed by (dest, nextHop): several routes to one destination is the normal case and
    //! is the whole point.
    std::map<std::pair<uint16_t, uint16_t>, CaTblRoute> m_routes;
    CaTblRoutingWeights m_w;
    mutable uint64_t m_divergences;
    mutable uint64_t m_selections;
};

} // namespace ns3

#endif // CA_TBL_ROUTING_H
