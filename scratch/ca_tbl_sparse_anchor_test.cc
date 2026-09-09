/*
 * Unit suite for sect 7 sparse-anchor geometry.
 *
 * The happy path is the least interesting thing here. Case A's danger is that it
 * ALWAYS has two geometrically valid answers, so every check below is aimed at the
 * disambiguation and at the refusals: that the prior picks the right intersection,
 * that a prior which cannot separate them is refused rather than guessed, and that a
 * degenerate geometry is refused rather than returned as a position near the origin.
 *
 * A solver that never refuses is not a solver, it is a random number generator with
 * good manners.
 */

#include "ns3/ca-tbl-sparse-anchor.h"
#include "ns3/core-module.h"

#include <cmath>
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
        std::cout << "  ok    " << what << "\n";
    }
    else
    {
        ++g_fail;
        std::cout << "  FAIL  " << what << "\n";
    }
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    std::cout << "CA-TBL sect 7 sparse-anchor suite\n\n";

    // Truth at (0, 30). Anchors on the x axis at -40 and +40.
    const double ax1 = -40.0, ay1 = 0.0;
    const double ax2 = 40.0, ay2 = 0.0;
    const double tx = 0.0, ty = 30.0;
    const double d1 = std::sqrt((tx - ax1) * (tx - ax1) + (ty - ay1) * (ty - ay1));
    const double d2 = std::sqrt((tx - ax2) * (tx - ax2) + (ty - ay2) * (ty - ay2));

    // --- Case A picks the candidate the prior supports -----------------------
    {
        CaTblSparseResult r = CaTblSolveCaseA(ax1, ay1, d1, ax2, ay2, d2,
                                              0.0, 25.0, 100.0, 0.0, 100.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_OK && r.solvedBy == CA_TBL_CASE_A,
              "Case A solves with a prior on the correct side");
        Check(std::fabs(r.x - tx) < 1e-6 && std::fabs(r.y - ty) < 1e-6,
              "Case A recovers the true position exactly from clean ranges");
    }

    // --- and the MIRROR prior picks the mirror point, which is the real test --
    {
        CaTblSparseResult r = CaTblSolveCaseA(ax1, ay1, d1, ax2, ay2, d2,
                                              0.0, -25.0, 100.0, 0.0, 100.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_OK && std::fabs(r.y + ty) < 1e-6,
              "a mirrored prior selects the mirrored intersection, not the true one");
    }

    // --- NEGATIVE: a prior equidistant from both cannot choose ---------------
    {
        CaTblSparseResult r = CaTblSolveCaseA(ax1, ay1, d1, ax2, ay2, d2,
                                              0.0, 0.0, 100.0, 0.0, 100.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_AMBIGUOUS && r.solvedBy == CA_TBL_CASE_NONE,
              "a prior on the baseline is AMBIGUOUS and returns no position");
    }

    // --- NEGATIVE: coincident anchors are degenerate -------------------------
    {
        CaTblSparseResult r = CaTblSolveCaseA(10.0, 10.0, 5.0, 10.0, 10.0, 5.0,
                                              0.0, 0.0, 100.0, 0.0, 100.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_DEGENERATE,
              "coincident anchors are DEGENERATE, not a fix at the midpoint");
    }

    // --- NEGATIVE: circles that miss by more than their own scale ------------
    {
        CaTblSparseResult r = CaTblSolveCaseA(-40.0, 0.0, 1.0, 40.0, 0.0, 1.0,
                                              0.0, 0.0, 100.0, 0.0, 100.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_DEGENERATE,
              "ranges that cannot reach each other are DEGENERATE");
    }

    // --- a small shortfall is pulled to the radical line, not refused --------
    {
        CaTblSparseResult r = CaTblSolveCaseA(ax1, ay1, d1 * 0.97, ax2, ay2, d2 * 0.97,
                                              0.0, 25.0, 100.0, 0.0, 100.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_OK,
              "a 3% range shortfall still solves rather than refusing");
    }

    // --- NEGATIVE: a singular prior cannot disambiguate ----------------------
    {
        CaTblSparseResult r = CaTblSolveCaseA(ax1, ay1, d1, ax2, ay2, d2,
                                              0.0, 25.0, 0.0, 0.0, 0.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_NO_PRIOR,
              "a singular prior covariance yields NO_PRIOR, not a guess");
    }

    // --- Case B reports its provenance separately ----------------------------
    {
        CaTblSparseResult r = CaTblSolveCaseB(ax1, ay1, d1, ax2, ay2, d2,
                                              0.0, 25.0, 100.0, 0.0, 100.0, 0.5);
        Check(r.verdict == CA_TBL_SPARSE_OK && r.solvedBy == CA_TBL_CASE_B,
              "Case B solves and labels itself B, so peer-derived evidence is traceable");
    }

    // --- Case C always refuses ----------------------------------------------
    {
        CaTblSparseResult r = CaTblSolveCaseC();
        Check(r.solvedBy == CA_TBL_CASE_NONE,
              "Case C returns no absolute position, by construction");
    }

    // --- the margin threshold actually gates ---------------------------------
    {
        CaTblSparseResult loose = CaTblSolveCaseA(ax1, ay1, d1, ax2, ay2, d2,
                                                  0.0, 1.0, 100.0, 0.0, 100.0, 0.001);
        CaTblSparseResult tight = CaTblSolveCaseA(ax1, ay1, d1, ax2, ay2, d2,
                                                  0.0, 1.0, 100.0, 0.0, 100.0, 5.0);
        Check(loose.verdict == CA_TBL_SPARSE_OK && tight.verdict == CA_TBL_SPARSE_AMBIGUOUS,
              "the same geometry is accepted at a loose margin and refused at a tight one");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
