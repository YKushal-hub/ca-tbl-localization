/*
 * Unit suite for Network Layer §2/§3 -- routing table, ETX baseline, and confidence-
 * weighted path selection.
 *
 * WHAT THIS SUITE IS ACTUALLY FOR. The claim being made is narrow: substituting
 * localization-uncertainty terms into the routing cost function for POSITION-FIX TRAFFIC
 * ONLY. Every check below tests a boundary of that claim rather than the happy path --
 * that gossip is not affected, that slack is respected, that the metric can be observed to
 * diverge from its baseline, and that it falls back rather than refusing to route.
 *
 * A metric that never diverges from ETX cannot be responsible for any difference in the
 * results, whatever its cost function says. That is why Divergences() is tested and not
 * merely exposed.
 */

#include "ns3/ca-tbl-routing.h"
#include "ns3/core-module.h"

#include <iostream>

using namespace ns3;

static int g_pass = 0;
static int g_fail = 0;

static void
Check(bool ok, const char* what)
{
    if (ok)
    {
        ++g_pass;
    }
    else
    {
        ++g_fail;
        std::cout << "  FAIL: " << what << "\n";
    }
}

static CaTblRoute
MakeRoute(uint16_t dest, uint16_t nextHop, uint8_t hops, double etx, double nlos,
          double rssiVar, double jitter, uint16_t seq, double t)
{
    CaTblRoute r;
    r.nodeId = dest;
    r.nextHopId = nextHop;
    r.hopCount = hops;
    r.etx = etx;
    r.sumNlos = nlos;
    r.sumRssiVar = rssiVar;
    r.sumJitter = jitter;
    r.seqNo = seq;
    r.lastUpdatedS = t;
    r.valid = true;
    return r;
}

int
main(int argc, char** argv)
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);
    std::cout << "CA-TBL Network Layer 2/3 routing suite\n";

    // ---- table mechanics --------------------------------------------------------------
    {
        CaTblRoutingTable t;
        t.Update(MakeRoute(5, 10, 1, 1.0, 0.1, 0.1, 0.1, 1, 0.0));
        t.Update(MakeRoute(5, 11, 2, 2.0, 0.2, 0.2, 0.2, 1, 0.0));
        Check(t.Size() == 2, "two routes to one destination are BOTH kept");
        Check(t.RoutesTo(5).size() == 2, "both retrievable");

        // Supersession is modular, like the position table's.
        t.Update(MakeRoute(5, 10, 3, 9.0, 0.9, 0.9, 0.9, 2, 1.0));
        Check(t.RoutesTo(5).size() == 2, "a newer seq replaces, does not duplicate");

        t.Update(MakeRoute(5, 10, 4, 99.0, 9.9, 9.9, 9.9, 1, 2.0));
        std::vector<CaTblRoute> rs = t.RoutesTo(5);
        double worst = 0.0;
        for (size_t i = 0; i < rs.size(); ++i)
        {
            worst = std::max(worst, rs[i].sumNlos);
        }
        Check(worst == 0.9, "an OLDER seq is refused (negative fixture)");

        t.Expire(100.0, 10.0);
        Check(t.Size() == 0, "stale routes age out -- a stale route names a dead next hop");
    }

    // ---- ETX baseline -----------------------------------------------------------------
    {
        CaTblRoutingTable t;
        // Route via 10: short and reliable. Via 11: same hops, worse delivery.
        t.Update(MakeRoute(5, 10, 2, 2.0, 5.0, 5.0, 5.0, 1, 0.0));
        t.Update(MakeRoute(5, 11, 2, 8.0, 0.1, 0.1, 0.1, 1, 0.0));
        CaTblRoute got;
        Check(t.SelectRoute(5, CA_TBL_ROUTE_ETX, CA_TBL_TRAFFIC_POSITION_FIX, got),
              "ETX selects");
        Check(got.nextHopId == 10, "ETX picks the cheapest link, ignoring confidence");

        // Same table, confidence metric: the dirty-but-reliable path loses.
        Check(t.SelectRoute(5, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got),
              "confidence selects");
        Check(got.nextHopId == 11, "confidence picks the cleaner path at equal hops");
        Check(t.Divergences() >= 1, "the divergence is counted, not merely possible");
    }

    // ---- shadow scoring: route on ETX, count what the confidence metric would do ------
    //
    // L.1.1 routes position fixes on ETX by default and scores the confidence metric in the
    // SAME call, through etxOut. Two calls would count each selection twice, so these pin
    // the one-call contract: ETX's pick comes back beside the confidence pick, the counters
    // move once per call, and the out-parameter is optional.
    {
        CaTblRoutingTable t;
        t.Update(MakeRoute(5, 10, 2, 2.0, 5.0, 5.0, 5.0, 1, 0.0));
        t.Update(MakeRoute(5, 11, 2, 8.0, 0.1, 0.1, 0.1, 1, 0.0));
        CaTblRoute conf;
        CaTblRoute etx;
        const bool ok =
            t.SelectRoute(5, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, conf, &etx);
        Check(ok && conf.nextHopId == 11 && etx.nextHopId == 10,
              "one call returns BOTH picks: confidence 11, and ETX's own 10 beside it");
        Check(t.Selections() == 1 && t.Divergences() == 1,
              "that call is ONE selection and ONE divergence, not two of each");

        CaTblRoute plain;
        CaTblRoute etx2;
        t.SelectRoute(5, CA_TBL_ROUTE_ETX, CA_TBL_TRAFFIC_POSITION_FIX, plain, &etx2);
        Check(plain.nextHopId == 10 && etx2.nextHopId == 10 && t.Divergences() == 1,
              "under ETX the two picks coincide and no divergence is counted");
    }

    // ---- an advertisement is not a selection ------------------------------------------
    //
    // A relay advertising its route says which path a frame handed to it would take.
    // PeekRoute() must name the SAME next hop SelectRoute() would, under either metric, and
    // move neither counter -- otherwise every advertisement would be counted as a routing
    // decision and Divergences() would stop meaning "a frame went a different way".
    {
        CaTblRoutingTable t;
        t.Update(MakeRoute(5, 10, 2, 2.0, 5.0, 5.0, 5.0, 1, 0.0));
        t.Update(MakeRoute(5, 11, 2, 8.0, 0.1, 0.1, 0.1, 1, 0.0));
        CaTblRoute pc;
        CaTblRoute pe;
        const bool okC = t.PeekRoute(5, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, pc);
        const bool okE = t.PeekRoute(5, CA_TBL_ROUTE_ETX, CA_TBL_TRAFFIC_POSITION_FIX, pe);
        Check(okC && okE && pc.nextHopId == 11 && pe.nextHopId == 10 &&
                  t.Selections() == 0 && t.Divergences() == 0,
              "PeekRoute names each metric's pick and counts nothing (negative fixture)");
        CaTblRoute sc;
        t.SelectRoute(5, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, sc);
        Check(sc.nextHopId == pc.nextHopId && t.Selections() == 1,
              "and SelectRoute then routes exactly where the advertisement said");
        CaTblRoute untouched = MakeRoute(1, 2, 3, 4.0, 0.0, 0.0, 0.0, 1, 0.0);
        Check(!t.PeekRoute(99, CA_TBL_ROUTE_ETX, CA_TBL_TRAFFIC_POSITION_FIX, untouched) &&
                  untouched.nextHopId == 2,
              "no route: PeekRoute returns false and leaves out untouched");
    }

    // ---- the traffic split IS the claim ----------------------------------------------
    {
        CaTblRoutingTable t;
        t.Update(MakeRoute(5, 10, 2, 2.0, 5.0, 5.0, 5.0, 1, 0.0));
        t.Update(MakeRoute(5, 11, 2, 8.0, 0.1, 0.1, 0.1, 1, 0.0));
        CaTblRoute got;
        // Gossip carries no localization payload, so it must keep plain ETX even when the
        // confidence metric is selected. Routing it otherwise would silently widen this
        // into a general-purpose routing claim.
        Check(t.SelectRoute(5, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_GOSSIP, got),
              "gossip selects");
        Check(got.nextHopId == 10,
              "GOSSIP stays on ETX even under the confidence metric (negative fixture)");
    }

    // ---- slack is capped, and the cap is load-bearing ---------------------------------
    {
        CaTblRoutingTable t;
        // A pristine path that is three hops longer. With slack 1 it must be refused,
        // however clean -- each extra hop adds another relay's uncertainty to the fix.
        t.Update(MakeRoute(5, 10, 1, 1.0, 9.0, 9.0, 9.0, 1, 0.0));
        t.Update(MakeRoute(5, 12, 4, 4.0, 0.0, 0.0, 0.0, 1, 0.0));
        CaTblRoute got;
        t.SelectRoute(5, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got);
        Check(got.nextHopId == 10,
              "a cleaner path beyond the slack cap is REFUSED (negative fixture)");

        CaTblRoutingWeights w;
        w.slack = 3;
        t.SetWeights(w);
        t.SelectRoute(5, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got);
        Check(got.nextHopId == 12, "raising slack admits it, so the cap was what bound");
    }

    // ---- sums along the chain, not averages -------------------------------------------
    {
        CaTblRoutingTable t;
        // Equal per-hop exposure, different lengths. The long path really is worse, and
        // an averaging metric would call them identical -- the exact failure the hop-aware
        // idea exists to correct.
        t.Update(MakeRoute(7, 20, 1, 1.0, 1.0, 1.0, 1.0, 1, 0.0));
        t.Update(MakeRoute(7, 21, 2, 1.0, 2.0, 2.0, 2.0, 1, 0.0));
        CaTblRoute got;
        CaTblRoutingWeights w;
        w.slack = 2;
        t.SetWeights(w);
        t.SelectRoute(7, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got);
        Check(got.nextHopId == 20,
              "accumulated exposure beats equal per-hop exposure (negative fixture)");
    }

    // ---- no route, and the availability guard ----------------------------------------
    {
        CaTblRoutingTable t;
        CaTblRoute got;
        Check(!t.SelectRoute(99, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got),
              "no route to an unknown destination (negative fixture)");

        // One viable route only: the confidence metric must still return it rather than
        // refuse. Turning a quality problem into an availability one would be the worse
        // failure in a search system.
        t.Update(MakeRoute(9, 30, 1, 1.0, 100.0, 100.0, 100.0, 1, 0.0));
        Check(t.SelectRoute(9, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got),
              "a filthy sole route is still routed, not refused");
        Check(got.nextHopId == 30, "and it is the one that exists");
    }

    // ---- weights actually reach the cost ----------------------------------------------
    {
        CaTblRoutingTable t;
        // Path A: all its exposure in NLOS. Path B: all of it in jitter. Which wins must
        // depend on the weights, or the weights are decorative.
        t.Update(MakeRoute(3, 40, 1, 1.0, 4.0, 0.0, 0.0, 1, 0.0));
        t.Update(MakeRoute(3, 41, 1, 1.0, 0.0, 0.0, 4.0, 1, 0.0));
        CaTblRoute got;
        CaTblRoutingWeights w;
        w.alphaNlos = 10.0;
        w.gammaJitter = 1.0;
        t.SetWeights(w);
        t.SelectRoute(3, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got);
        Check(got.nextHopId == 41, "heavy NLOS weight rejects the NLOS-exposed path");

        w.alphaNlos = 1.0;
        w.gammaJitter = 10.0;
        t.SetWeights(w);
        t.SelectRoute(3, CA_TBL_ROUTE_CONFIDENCE, CA_TBL_TRAFFIC_POSITION_FIX, got);
        Check(got.nextHopId == 40, "heavy jitter weight rejects the jittery path");
    }

    // ---- ETX cold start ---------------------------------------------------------------
    {
        CaTblRoutingTable t;
        // No delivery-ratio estimate observed yet. Falling back to hop count is what makes
        // the baseline usable from the first cycle instead of only after convergence.
        t.Update(MakeRoute(4, 50, 3, 0.0, 0.0, 0.0, 0.0, 1, 0.0));
        t.Update(MakeRoute(4, 51, 1, 0.0, 0.0, 0.0, 0.0, 1, 0.0));
        CaTblRoute got;
        t.SelectRoute(4, CA_TBL_ROUTE_ETX, CA_TBL_TRAFFIC_POSITION_FIX, got);
        Check(got.nextHopId == 51, "ETX falls back to hop count before any estimate exists");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
