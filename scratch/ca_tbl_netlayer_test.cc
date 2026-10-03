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
#include "ns3/ca-tbl-header.h"
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

    // ---- sect 6: the double grant that ACTUALLY happens first ---------------
    // Two anchors hear one ESC_REQUEST in the same instant and both grant it, because
    // neither yet knows of the other. Only gossip can settle it afterwards, and it must
    // settle it the same way from both sides with nothing further exchanged.
    {
        CaTblGrantCoordinator low(101);
        CaTblGrantCoordinator high(107);
        low.NoteOwnGrant(500, 10.0, 20.0);
        high.NoteOwnGrant(500, 10.0, 20.0);

        CaTblGrantRecord fromHigh; fromHigh.anchorId = 107; fromHigh.targetId = 500;
        fromHigh.startS = 10.0; fromHigh.endS = 20.0;
        CaTblGrantRecord fromLow = fromHigh; fromLow.anchorId = 101;

        const CaTblGrantVerdict vLow = low.Reconcile(fromHigh);
        const CaTblGrantVerdict vHigh = high.Reconcile(fromLow);
        Check(vLow == CA_TBL_GRANT_DEFER_LOWER_ID && vHigh == CA_TBL_GRANT_WIN_HIGHER_ID,
              "crossing gossip settles a same-instant double grant: lower DEFERS, higher WINS");
        Check(low.OwnActiveGrants(10.0).empty() && high.OwnActiveGrants(10.0).size() == 1,
              "the deferring anchor YIELDS its grant and the winner keeps its own");
        Check(low.LastConflictAnchor() == 107 && high.LastConflictAnchor() == 101,
              "each side records which anchor it contested with");
        Check(low.Evaluate(500, 12.0, 22.0) == CA_TBL_GRANT_DEFER_LOWER_ID,
              "after yielding, a new request from that node in the window is refused");
        Check(high.Reconcile(fromLow) == CA_TBL_GRANT_OK && high.Wins() == 1,
              "a repeated gossip is not re-counted -- counters track contests, not chatter");
    }

    // ---- sect 6: gossip with no overlapping grant of ours is knowledge only ------
    {
        CaTblGrantCoordinator a(101);
        CaTblGrantRecord g; g.anchorId = 107; g.targetId = 500; g.startS = 0.0; g.endS = 10.0;
        Check(a.Reconcile(g) == CA_TBL_GRANT_OK && a.Size() == 1 && a.Defers() == 0,
              "an uncontested gossiped grant is stored, and contests nothing");
    }

    // ---- sect 6: the frame that carries it, on the spec's terms --------------
    {
        Check(CaTblAccessCategoryFor(CA_TBL_MSG_GRANT_GOSSIP) == CA_TBL_AC_LOW &&
                  !CaTblIsFlooded(CA_TBL_MSG_GRANT_GOSSIP) &&
                  CaTblAckPolicyFor(CA_TBL_MSG_GRANT_GOSSIP) == CA_TBL_ACK_NONE,
              "GRANT_GOSSIP is low-priority CW, one hop (never flooded), unacknowledged");
        Check(CaTblPayloadBytes(CA_TBL_MSG_GRANT_GOSSIP, 0) == 1 &&
                  CaTblPayloadBytes(CA_TBL_MSG_GRANT_GOSSIP, 3) == 1 + 3 * 6,
              "GRANT_GOSSIP is billed 1 byte of count plus 6 per held slot");
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

    // ---- NW-6: the queue gossips, and the tie-break extends to it -------------
    {
        CaTblGrantCoordinator low(101);
        CaTblGrantCoordinator high(107);
        CaTblQueueRecord q;
        q.targetId = 2;
        q.uQ = 100;
        q.queuedSinceS = 5.0;
        low.SetOwnQueue(std::vector<CaTblQueueRecord>(1, q));
        high.SetOwnQueue(std::vector<CaTblQueueRecord>(1, q));
        const uint32_t dLow = low.NoteGossipedQueue(107, high.OwnQueue(), 6.0);
        const uint32_t dHigh = high.NoteGossipedQueue(101, low.OwnQueue(), 6.0);
        Check(dLow == 1 && dHigh == 1 && low.DoubleQueued() == 1,
              "a request both anchors queue is seen as double-queued by both");
        Check(low.QueueYields(2, 6.5, 30.0) && !high.QueueYields(2, 6.5, 30.0),
              "the lower id gives the queued request up, the higher keeps it");
        Check(!low.QueueYields(3, 6.5, 30.0), "another target is not affected");
        Check(!low.QueueYields(2, 40.0, 30.0), "a heard queue older than the bound stops counting");
        // A later gossip without the item replaces the earlier one: the request is gone.
        low.NoteGossipedQueue(107, std::vector<CaTblQueueRecord>(), 7.0);
        Check(!low.QueueYields(2, 7.5, 30.0),
              "a gossip states the whole queue, so an item it no longer carries stops counting");
        // A higher-id anchor's active grant for the target also makes the lower yield.
        CaTblGrantRecord g;
        g.anchorId = 107;
        g.targetId = 2;
        g.startS = 7.0;
        g.endS = 17.0;
        low.NoteGossipedGrant(g);
        Check(low.QueueYields(2, 8.0, 30.0) && !low.QueueYields(2, 20.0, 30.0),
              "a higher id's live grant for the target also wins over a queued request here");
        Check(low.NoteGossipedQueue(101, low.OwnQueue(), 8.0) == 0,
              "an anchor's own queue echoed back is ignored");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
