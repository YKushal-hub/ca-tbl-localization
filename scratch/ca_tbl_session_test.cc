/*
 * Unit suite for the Transport/Application layer: session lifecycle, result provenance
 * rendering, and the SEARCH_AND_LOCALIZE / BATCH_SEARCH orchestration.
 *
 * WHY A SEPARATE SUITE RATHER THAN A SCENARIO GATE. Every transition here is defined by
 * time and by message arrival order, and both are things a scenario supplies incidentally
 * rather than deliberately. A staleness transition, in particular, is defined by the
 * ABSENCE of a result -- a defect whose signature is absence cannot be caught by scanning
 * data that is present, so it has to be driven directly.
 *
 * NEGATIVE FIXTURES ARE INCLUDED AND EXPECTED TO PASS BY FAILING: each is a case the layer
 * must refuse. A suite that only demonstrates the happy path proves the code runs, not
 * that it discriminates.
 */

#include "ns3/ca-tbl-session.h"
#include "ns3/core-module.h"

#include <iostream>
#include <vector>

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

static CaTblResult
MakeResult(uint8_t id, CaTblTier tier, bool suspect, double t)
{
    CaTblResult r;
    r.targetManifestId = id;
    r.x = 100.0;
    r.y = 200.0;
    r.varX = 400.0;
    r.varY = 400.0;
    r.covXY = 0.0;
    r.tierSource = tier;
    r.pcs = 0.5;
    r.hopCountAtFix = 1;
    r.suspect = suspect;
    r.timestampS = t;
    return r;
}

int
main(int argc, char** argv)
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    std::cout << "CA-TBL Transport/Application layer suite\n";

    // ---- Application §3: the rendering rule ------------------------------------------
    {
        Check(CaTblRenderFor(MakeResult(1, CA_TBL_TIER_UWB, false, 0.0)) ==
                  CA_TBL_RENDER_HIGH,
              "UWB clean renders HIGH");
        Check(CaTblRenderFor(MakeResult(1, CA_TBL_TIER_2P4, false, 0.0)) ==
                  CA_TBL_RENDER_MEDIUM,
              "2.4 GHz clean renders MEDIUM");
        Check(CaTblRenderFor(MakeResult(1, CA_TBL_TIER_868, false, 0.0)) ==
                  CA_TBL_RENDER_COARSE,
              "868 clean renders COARSE");

        // THE ONE THAT MATTERS. A sub-metre UWB fix that failed the integrity gate must
        // NOT inherit the most confident marker on the screen. This is the case the
        // switching-point integrity check exists to surface, and the only case where the
        // render rule disagrees with the tier.
        Check(CaTblRenderFor(MakeResult(1, CA_TBL_TIER_UWB, true, 0.0)) ==
                  CA_TBL_RENDER_VERIFY,
              "SUSPECT overrides UWB (negative fixture)");
        Check(CaTblRenderFor(MakeResult(1, CA_TBL_TIER_868, true, 0.0)) ==
                  CA_TBL_RENDER_VERIFY,
              "SUSPECT overrides 868 (negative fixture)");
    }

    // ---- Transport §2: the lifecycle -------------------------------------------------
    {
        CaTblSessionManager mgr;
        mgr.SetStalenessBound(30.0);
        mgr.SetSessionTimeout(120.0);

        const uint32_t sid = mgr.SearchAndLocalize(7, 100, CA_TBL_PRIORITY_ROUTINE, 0.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_SEARCHING,
              "opens in SEARCHING");

        mgr.OnResult(MakeResult(7, CA_TBL_TIER_868, false, 10.0), 10.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_LOCATED_COARSE,
              "868 result -> LOCATED_COARSE");
        Check(mgr.Sessions().at(sid).TimeToCoarse(7) == 10.0, "time-to-coarse recorded");
        Check(mgr.Sessions().at(sid).TimeToPrecise(7) < 0.0,
              "time-to-precise still never (negative means never)");

        mgr.OnResult(MakeResult(7, CA_TBL_TIER_UWB, false, 20.0), 20.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_LOCATED_PRECISE,
              "UWB result -> LOCATED_PRECISE");
        Check(mgr.Sessions().at(sid).TimeToPrecise(7) == 20.0, "time-to-precise recorded");

        mgr.OnResult(MakeResult(7, CA_TBL_TIER_UWB, false, 25.0), 25.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_TRACKING,
              "held open past first precise -> TRACKING");

        // Staleness is the absence of results, so it is driven by time alone.
        mgr.Tick(60.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_STALE,
              "no fix within the bound -> STALE");

        // RECOVERS, does not reopen. A second "first fix" for a target that was never
        // actually lost would be a false metric.
        mgr.OnResult(MakeResult(7, CA_TBL_TIER_868, false, 70.0), 70.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_TRACKING,
              "fresh fix -> TRACKING, not SEARCHING");
        Check(mgr.Sessions().at(sid).TimeToCoarse(7) == 10.0,
              "recovery does NOT restamp time-to-coarse (negative fixture)");
        Check(mgr.Sessions().at(sid).Recoveries() == 1, "recovery counted");

        mgr.Tick(300.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_CLOSED,
              "past the session timeout -> CLOSED");
    }

    // ---- a suspect fix must not advance the session ----------------------------------
    {
        CaTblSessionManager mgr;
        const uint32_t sid = mgr.SearchAndLocalize(9, 100, CA_TBL_PRIORITY_URGENT, 0.0);
        mgr.OnResult(MakeResult(9, CA_TBL_TIER_UWB, true, 5.0), 5.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_SEARCHING,
              "SUSPECT fix does not advance the session (negative fixture)");
        Check(mgr.Sessions().at(sid).TimeToPrecise(9) < 0.0,
              "SUSPECT fix stamps no time-to-precise (negative fixture)");
        Check(mgr.Sessions().at(sid).ResultsSuspect() == 1, "suspect counted");
    }

    // ---- a result for a target this session did not ask about ------------------------
    {
        CaTblSessionManager mgr;
        const uint32_t sid = mgr.SearchAndLocalize(3, 100, CA_TBL_PRIORITY_ROUTINE, 0.0);
        mgr.OnResult(MakeResult(4, CA_TBL_TIER_UWB, false, 5.0), 5.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_SEARCHING,
              "unrelated target ignored (negative fixture)");
        Check(mgr.Sessions().at(sid).ResultsAccepted() == 0,
              "unrelated target not counted (negative fixture)");
    }

    // ---- an unanswered search must eventually close ----------------------------------
    {
        CaTblSessionManager mgr;
        mgr.SetSessionTimeout(100.0);
        const uint32_t sid = mgr.SearchAndLocalize(5, 100, CA_TBL_PRIORITY_ROUTINE, 0.0);
        mgr.Tick(50.0);
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_SEARCHING,
              "still searching before the timeout");
        mgr.Tick(150.0);
        // Without ageing an unanswered session against its OPEN time it would sit in
        // SEARCHING forever -- the one outcome an operator most needs told.
        Check(mgr.Sessions().at(sid).State() == CA_TBL_SESSION_CLOSED,
              "a search nobody answered closes (negative fixture)");
    }

    // ---- Application §5: batch is ONE session over N targets --------------------------
    {
        CaTblSessionManager mgr;
        std::vector<uint8_t> team;
        team.push_back(2);
        team.push_back(3);
        team.push_back(4);
        const uint32_t sid = mgr.BatchSearch(team, 100, CA_TBL_PRIORITY_URGENT, 0.0);
        Check(mgr.OpenCount() == 1, "a five-person search is ONE session, not N");

        mgr.OnResult(MakeResult(2, CA_TBL_TIER_UWB, false, 10.0), 10.0);
        // ONE target precise: a spread needs two points, and reporting a spread of
        // zero here would claim the batch finished together when four fifths of it
        // has not been found at all.
        Check(mgr.BatchPrecisionSpread(sid) < 0.0,
              "spread undefined with one target precise (negative fixture)");

        mgr.OnResult(MakeResult(3, CA_TBL_TIER_UWB, false, 25.0), 25.0);
        Check(mgr.Sessions().at(sid).TimeToPrecise(4) < 0.0, "third target still never");

        mgr.OnResult(MakeResult(4, CA_TBL_TIER_UWB, false, 40.0), 40.0);
        // The number the spec calls a headline result: the spread between first and last
        // target to reach precision is the direct evidence that arbitration ORDERS
        // targets rather than serving them first-come-first-served.
        Check(mgr.BatchPrecisionSpread(sid) == 30.0,
              "batch precision spread = last - first");
    }

    // ---- one flood can answer two outstanding searches --------------------------------
    {
        CaTblSessionManager mgr;
        const uint32_t a = mgr.SearchAndLocalize(8, 100, CA_TBL_PRIORITY_ROUTINE, 0.0);
        const uint32_t b = mgr.SearchAndLocalize(8, 101, CA_TBL_PRIORITY_URGENT, 0.0);
        mgr.OnResult(MakeResult(8, CA_TBL_TIER_868, false, 5.0), 5.0);
        Check(mgr.Sessions().at(a).State() == CA_TBL_SESSION_LOCATED_COARSE &&
                  mgr.Sessions().at(b).State() == CA_TBL_SESSION_LOCATED_COARSE,
              "a shared answer reaches every session that asked");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
