/*
 * Unit suite for Network sect 5 (relay chain composition) and sect 6 (grant coordination).
 *
 * Both mechanisms exist to prevent a SILENT wrong answer, so both are tested by trying
 * to produce that wrong answer:
 *
 *   sect 6 -- two anchors granting one node's radio for overlapping windows, each
 *             believing it holds the slot. The check is that they reach OPPOSITE
 *             decisions from the same facts without communicating further.
 *   sect 5 -- a depth-2 relay passing on a covariance that understates its own error by
 *             a known amount. The check is that the amount comes back.
 */

#include "ns3/ca-tbl-grant-coord.h"
#include "ns3/ca-tbl-relay-admission.h"
#include "ns3/core-module.h"

#include <cmath>
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
    std::cout << "CA-TBL Network sect 5/6 suite\n\n";

    // ---- sect 6: the tie-break, from both sides at once ---------------------
    {
        CaTblGrantCoordinator low(101);
        CaTblGrantCoordinator high(107);

        CaTblGrantRecord byHigh;
        byHigh.anchorId = 107; byHigh.targetId = 500; byHigh.startS = 10.0; byHigh.endS = 20.0;
        CaTblGrantRecord byLow;
        byLow.anchorId = 101; byLow.targetId = 500; byLow.startS = 12.0; byLow.endS = 22.0;

        low.NoteGossipedGrant(byHigh);
        high.NoteGossipedGrant(byLow);

        const CaTblGrantVerdict vLow = low.Evaluate(500, 12.0, 22.0);
        const CaTblGrantVerdict vHigh = high.Evaluate(500, 10.0, 20.0);

        Check(vLow == CA_TBL_GRANT_DEFER_LOWER_ID, "the lower-id anchor defers");
        Check(vHigh == CA_TBL_GRANT_WIN_HIGHER_ID, "the higher-id anchor proceeds");
        Check(!CaTblGrantCoordinator::Permits(vLow) && CaTblGrantCoordinator::Permits(vHigh),
              "exactly one of two contending anchors is permitted -- the slot is not double-granted");
        Check(low.Defers() == 1 && high.Wins() == 1,
              "both sides count the contention, so a demo can prove it was exercised");
    }

    // ---- sect 6: different targets are not a conflict ------------------------
    {
        CaTblGrantCoordinator a(101);
        CaTblGrantRecord g; g.anchorId = 107; g.targetId = 501; g.startS = 0.0; g.endS = 100.0;
        a.NoteGossipedGrant(g);
        Check(a.Evaluate(500, 0.0, 100.0) == CA_TBL_GRANT_OK,
              "an overlapping window for a DIFFERENT target is not a conflict");
    }

    // ---- sect 6: touching windows are a handover, not a collision -----------
    {
        CaTblGrantCoordinator a(101);
        CaTblGrantRecord g; g.anchorId = 107; g.targetId = 500; g.startS = 0.0; g.endS = 10.0;
        a.NoteGossipedGrant(g);
        Check(a.Evaluate(500, 10.0, 20.0) == CA_TBL_GRANT_OK,
              "a window starting exactly as another ends is a handover, not an overlap");
    }

    // ---- sect 6: an anchor must not conflict with itself --------------------
    {
        CaTblGrantCoordinator a(101);
        CaTblGrantRecord echo; echo.anchorId = 101; echo.targetId = 500; echo.startS = 0.0; echo.endS = 10.0;
        a.NoteGossipedGrant(echo);   // our own grant reflected back by gossip
        Check(a.Size() == 0, "an anchor ignores its own grant echoed back to it");
        a.NoteOwnGrant(500, 0.0, 10.0);
        Check(a.Evaluate(500, 5.0, 15.0) == CA_TBL_GRANT_SELF_REPEAT,
              "re-granting a window it already holds is SELF_REPEAT, not a fresh grant");
    }

    // ---- sect 6: expiry, so the tie-break cannot depend on uptime -----------
    {
        CaTblGrantCoordinator a(101);
        CaTblGrantRecord g; g.anchorId = 107; g.targetId = 500; g.startS = 0.0; g.endS = 10.0;
        a.NoteGossipedGrant(g);
        a.Expire(50.0);
        Check(a.Size() == 0 && a.Evaluate(500, 0.0, 10.0) == CA_TBL_GRANT_OK,
              "an ended grant stops constraining, so the answer does not depend on uptime");
    }

    // ---- sect 5: depth 1 inherits nothing and must not be inflated ----------
    {
        const double P[2][2] = {{4.0, 0.0}, {0.0, 9.0}};
        double out[2][2];
        Check(CaTblComposeRelayChain(P, 0.0, 1.0, 0.0, out) &&
                  std::fabs(out[0][0] - 4.0) < 1e-9 && std::fabs(out[1][1] - 9.0) < 1e-9,
              "depth 1 passes P_B through unchanged -- no double counting");
    }

    // ---- sect 5: depth 2 adds the inherited term, DIRECTIONALLY -------------
    {
        const double P[2][2] = {{4.0, 0.0}, {0.0, 9.0}};
        double out[2][2];
        const bool ok = CaTblComposeRelayChain(P, 25.0, 1.0, 0.0, out);
        Check(ok && std::fabs(out[0][0] - 29.0) < 1e-9,
              "depth 2 adds the inherited variance along the inherited direction");
        Check(ok && std::fabs(out[1][1] - 9.0) < 1e-9,
              "and leaves the perpendicular axis alone -- the inflation is not isotropic");
    }

    // ---- sect 5: the composed covariance actually changes admission ---------
    {
        const double P[2][2] = {{4.0, 0.0}, {0.0, 4.0}};
        double out[2][2];
        CaTblComposeRelayChain(P, 100.0, 1.0, 0.0, out);
        const double before = CaTblProjectRelayCovariance(P, 1.0, 0.0);
        const double after = CaTblProjectRelayCovariance(out, 1.0, 0.0);
        Check(after > before * 10.0,
              "the correction is large enough to change an admission decision, not cosmetic");
    }

    // ---- sect 5: NEGATIVE -- a degenerate relay covariance is refused -------
    {
        const double bad[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
        double out[2][2] = {{-1.0, -1.0}, {-1.0, -1.0}};
        Check(!CaTblComposeRelayChain(bad, 25.0, 1.0, 0.0, out) && out[0][0] < 0.0,
              "a degenerate P_B is REFUSED and outP is left untouched, not passed through");
    }

    // ---- sect 5: a zero direction falls back rather than dropping the term --
    {
        const double P[2][2] = {{4.0, 0.0}, {0.0, 4.0}};
        double out[2][2];
        const bool ok = CaTblComposeRelayChain(P, 16.0, 0.0, 0.0, out);
        Check(ok && out[0][0] > 4.0 && out[1][1] > 4.0,
              "with no direction the inherited variance is applied isotropically, never dropped");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
