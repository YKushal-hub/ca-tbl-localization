/*
 * Unit suite for sect 24-27 node lifecycle.
 *
 * The whole module exists to notice an ABSENCE, so the suite is built around rule 9:
 * a check for a defect whose signature is absence cannot be a scan of present data.
 * Every meaningful case here is therefore driven by Tick() WITHOUT contact, and the
 * hardest one -- reappearance -- is tested for being distinguishable from a first
 * sighting, which is the thing a boolean "seen" flag would get wrong.
 */

#include "ns3/ca-tbl-node-lifecycle.h"
#include "ns3/core-module.h"

#include <iostream>

using namespace ns3;

static int g_pass = 0;
static int g_fail = 0;

static void
Check(bool ok, const char* what)
{
    if (ok) { ++g_pass; std::cout << "  ok    " << what << "\n"; }
    else    { ++g_fail; std::cout << "  FAIL  " << what << "\n"; }
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    std::cout << "CA-TBL sect 24-27 node lifecycle suite\n\n";

    CaTblLifecycleBounds b;   // 60 / 300 / 30

    // ---- unseen is not the same as lost -------------------------------------
    {
        CaTblNodeLifecycle lc(b);
        Check(lc.StateOf(500) == CA_TBL_NODE_UNSEEN,
              "a node never heard from is UNSEEN, not LOST");
        lc.Tick(10000.0);
        Check(lc.StateOf(500) == CA_TBL_NODE_UNSEEN,
              "time alone does not turn UNSEEN into LOST -- absence of a record is not a loss");
    }

    // ---- the full arc: appear, degrade, lose --------------------------------
    {
        CaTblNodeLifecycle lc(b);
        Check(lc.NoteContact(500, 0.0) == CA_TBL_NODE_TRACKED, "first contact gives TRACKED");
        lc.Tick(30.0);
        Check(lc.StateOf(500) == CA_TBL_NODE_TRACKED, "still TRACKED inside the degrade bound");
        lc.Tick(90.0);
        Check(lc.StateOf(500) == CA_TBL_NODE_DEGRADED, "past 60 s of silence it is DEGRADED");
        lc.Tick(400.0);
        Check(lc.StateOf(500) == CA_TBL_NODE_LOST, "past 300 s of silence it is LOST");
    }

    // ---- THE HARD CASE: reappearance is not a first sighting -----------------
    {
        CaTblNodeLifecycle lc(b);
        lc.NoteContact(500, 0.0);
        lc.Tick(400.0);
        Check(lc.StateOf(500) == CA_TBL_NODE_LOST, "lost after silence");

        const CaTblNodeState back = lc.NoteContact(500, 410.0);
        Check(back == CA_TBL_NODE_REACQUIRED,
              "contact after loss gives REACQUIRED, distinguishable from a first sighting");

        CaTblNodeRecord r;
        lc.Get(500, r);
        Check(r.reacquisitions == 1 && r.lossEvents == 1 && r.everLost,
              "the loss and the recovery are both recorded, not just the current state");
        Check(r.firstHeardS == 0.0,
              "firstHeardS still points at the original sighting, not the reacquisition");
    }

    // ---- REACQUIRED is an announcement with a lifetime -----------------------
    {
        CaTblNodeLifecycle lc(b);
        lc.NoteContact(500, 0.0);
        lc.Tick(400.0);
        lc.NoteContact(500, 410.0);
        lc.NoteContact(500, 415.0);
        Check(lc.StateOf(500) == CA_TBL_NODE_REACQUIRED,
              "further contact inside the hold does not cancel the announcement early");
        lc.Tick(445.0);
        Check(lc.StateOf(500) == CA_TBL_NODE_TRACKED,
              "after the hold it settles to TRACKED -- REACQUIRED is not a resting state");
    }

    // ---- loss is counted as a TRANSITION, not as a condition ----------------
    {
        CaTblNodeLifecycle lc(b);
        lc.NoteContact(500, 0.0);
        for (double t = 400.0; t < 2000.0; t += 50.0)
        {
            lc.Tick(t);   // sits in LOST for a long time, ticked many times
        }
        CaTblNodeRecord r;
        lc.Get(500, r);
        Check(r.lossEvents == 1,
              "a node lost once counts once however often Tick() is called");
    }

    // ---- two losses are two events ------------------------------------------
    {
        CaTblNodeLifecycle lc(b);
        lc.NoteContact(500, 0.0);
        lc.Tick(400.0);
        lc.NoteContact(500, 410.0);
        lc.Tick(450.0);              // settle out of REACQUIRED
        lc.Tick(900.0);              // lost again
        CaTblNodeRecord r;
        lc.Get(500, r);
        Check(r.state == CA_TBL_NODE_LOST && r.lossEvents == 2 && r.reacquisitions == 1,
              "a second disappearance is a second loss event");
    }

    // ---- summary counts, which is what a demo would print --------------------
    {
        CaTblNodeLifecycle lc(b);
        lc.NoteContact(500, 0.0);
        lc.NoteContact(501, 0.0);
        lc.NoteContact(502, 0.0);
        lc.Tick(90.0);
        lc.NoteContact(500, 90.0);
        lc.Tick(95.0);
        Check(lc.CountIn(CA_TBL_NODE_TRACKED) == 1 && lc.CountIn(CA_TBL_NODE_DEGRADED) == 2,
              "per-state counts separate the node still in contact from the two that are not");
        Check(lc.TotalLossEvents() == 0,
              "degraded is not lost -- the loss counter stays honest about the difference");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
