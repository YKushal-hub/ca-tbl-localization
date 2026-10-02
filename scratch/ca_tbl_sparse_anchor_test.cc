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

#include "ns3/ca-tbl-policy.h"
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

    // --- DC-11: Cases A/B inside CaTblPolicy::Step(), after the prediction ------
    {
        // Two gateways 1000 m apart, the node at (500, 300); the mirror root is (500, -300).
        auto two = [](uint8_t k0, uint8_t k1) {
            std::vector<CaTblRangeMeasurement> m(2);
            const double gx[2] = {0.0, 1000.0};
            const uint8_t kind[2] = {k0, k1};
            for (int i = 0; i < 2; ++i)
            {
                m[i].anchorX = gx[i];
                m[i].anchorY = 0.0;
                m[i].rangeM = std::hypot(500.0 - gx[i], 300.0);
                m[i].sigmaM = 5.0;
                m[i].sourceKind = kind[i];
                m[i].anchorId = static_cast<uint16_t>(101 + i);
            }
            return m;
        };
        auto run = [&](bool inStep, double priorY, uint8_t k0, uint8_t k1, uint64_t* noFix,
                       double* ex, double* ey) {
            CaTblPolicyConfig cfg;
            cfg.sparseInStep = inStep;
            cfg.sparseMarginM = 1.0;
            CaTblPolicy p;
            p.Configure(cfg);
            p.Initialize(500.0, priorY, 60.0, 0.0);
            CaTblDecision d = p.Step(1.0, two(k0, k1), 0, 2, 2, 7u);
            *noFix = p.GetNoFixCount();
            *ex = p.EstX();
            *ey = p.EstY();
            return d;
        };
        const uint8_t GW = CA_TBL_SRC_SURVEYED_GATEWAY;
        uint64_t nf = 0;
        double ex = 0.0, ey = 0.0;

        CaTblDecision off = run(false, 250.0, GW, GW, &nf, &ex, &ey);
        Check(!off.hadFix && off.fixKind == CA_TBL_FIX_NONE && off.sparseVerdict == 255 &&
                  nf == 1 && ex == 500.0 && ey == 250.0,
              "DC-11 off: two ranges coast and count as a no-fix, as before");

        CaTblDecision a = run(true, 250.0, GW, GW, &nf, &ex, &ey);
        Check(a.hadFix && a.fixKind == CA_TBL_FIX_CASE_A && a.sparseVerdict == CA_TBL_SPARSE_OK &&
                  nf == 0,
              "DC-11: two surveyed gateways solve as Case A and are a fix, not a no-fix");
        Check(a.positionFuses == 1 && a.rangeUpdates == 0,
              "DC-11: the pair is fused once as a position and never also as ranges");
        Check(std::fabs(a.sparseX - 500.0) < 1e-6 && std::fabs(a.sparseY - 300.0) < 1e-6 &&
                  a.sparsePriorX == 500.0 && a.sparsePriorY == 250.0 && ey > 250.0 && ey < 300.0,
              "DC-11: the propagated prior chose the true root, and the estimate moved toward it");

        CaTblDecision b = run(true, 250.0, GW, CA_TBL_SRC_FINDER, &nf, &ex, &ey);
        Check(b.fixKind == CA_TBL_FIX_CASE_B,
              "DC-11: a gateway and a finder solve as Case B -- provenance decides");
        CaTblDecision u = run(true, 250.0, CA_TBL_SRC_UNSTATED, CA_TBL_SRC_UNSTATED, &nf, &ex, &ey);
        Check(u.fixKind == CA_TBL_FIX_CASE_B,
              "DC-11: unlabelled ranges are never taken for surveyed gateways");

        CaTblDecision amb = run(true, 0.0, GW, GW, &nf, &ex, &ey);
        Check(amb.sparseVerdict == CA_TBL_SPARSE_AMBIGUOUS && !amb.hadFix &&
                  amb.fixKind == CA_TBL_FIX_NONE && nf == 1 && amb.positionFuses == 0,
              "DC-11: a prior equidistant from both roots refuses, and the cycle coasts");

        // Three ranges take the ordinary path: FULL, range updates, no sparse attempt.
        CaTblPolicyConfig cfg;
        cfg.sparseInStep = true;
        CaTblPolicy p;
        p.Configure(cfg);
        p.Initialize(500.0, 250.0, 60.0, 0.0);
        std::vector<CaTblRangeMeasurement> m3 = two(GW, GW);
        CaTblRangeMeasurement third = m3[0];
        third.anchorX = 500.0;
        third.anchorY = 1000.0;
        third.rangeM = 700.0;
        m3.push_back(third);
        CaTblDecision full = p.Step(1.0, m3, 0, 3, 3, 7u);
        Check(full.fixKind == CA_TBL_FIX_FULL && full.sparseVerdict == 255 &&
                  full.rangeUpdates >= 1 && full.positionFuses == 0,
              "DC-11: three ranges take the ordinary update path");
    }

    // --- DC-2: Case B against the IMU-propagated prior -----------------------------------
    {
        // A gateway at (0, 0) and a finder at (1000, 0); the node is truly at (500, 300), the
        // mirror root at (500, -300). The previous estimate is (500, -50), still. The walker
        // went 150 m north: the constant-velocity prior stays at (500, -50) and picks the
        // wrong root; the IMU, which saw the 150 m, carries the prior to (500, 100).
        std::vector<CaTblRangeMeasurement> m(2);
        const double gx[2] = {0.0, 1000.0};
        const uint8_t kind[2] = {CA_TBL_SRC_SURVEYED_GATEWAY, CA_TBL_SRC_FINDER};
        for (int i = 0; i < 2; ++i)
        {
            m[i].anchorX = gx[i];
            m[i].anchorY = 0.0;
            m[i].rangeM = std::hypot(500.0 - gx[i], 300.0);
            m[i].sigmaM = 5.0;
            m[i].sourceKind = kind[i];
        }
        CaTblPolicyConfig cfg;
        cfg.sparseInStep = true;
        CaTblPolicy cv, imu;
        cv.Configure(cfg);
        imu.Configure(cfg);
        cv.Initialize(500.0, -50.0, 60.0, 0.0);
        imu.Initialize(500.0, -50.0, 60.0, 0.0);
        imu.SetImuControl(0.0, 150.0, 25.0, 0.0, 25.0);
        CaTblDecision dc = cv.Step(1.0, m, 0, 2, 2, 7u);
        CaTblDecision di = imu.Step(1.0, m, 0, 2, 2, 7u);
        Check(dc.fixKind == CA_TBL_FIX_CASE_B && dc.sparsePriorSource == 0 &&
                  std::fabs(dc.sparseY + 300.0) < 1e-6,
              "DC-2: the constant-velocity prior picks the mirror root (labelled CV)");
        Check(di.fixKind == CA_TBL_FIX_CASE_B && di.sparsePriorSource == 1 &&
                  std::fabs(di.sparseY - 300.0) < 1e-6 && di.sparsePriorY == 100.0,
              "DC-2: the IMU-propagated prior picks the true root (labelled IMU)");
    }

    // --- DC-4: Case D, one range on the propagated state --------------------------------
    {
        // One gateway due east of the prior at 300 m; the node is truly at 310 m.
        auto one = [](double range) {
            std::vector<CaTblRangeMeasurement> m(1);
            m[0].anchorX = 300.0;
            m[0].anchorY = 0.0;
            m[0].rangeM = range;
            m[0].sigmaM = 5.0;
            m[0].sourceKind = CA_TBL_SRC_SURVEYED_GATEWAY;
            return m;
        };
        auto mk = [](bool caseD, CaTblPolicy& p) {
            CaTblPolicyConfig cfg;
            cfg.sparseInStep = true;
            cfg.caseD = caseD;
            cfg.nDeescalate = 1;
            cfg.mode = CA_TBL_MODE_FULL;
            p.Configure(cfg);
            p.Initialize(0.0, 0.0, 50.0, 0.0);
            // Start on 2.4 GHz, so a demotion is possible and its absence means something.
            p.AbortTierChange(CA_TBL_TIER_2P4);
        };
        CaTblPolicy off, on, wild;
        mk(false, off);
        mk(true, on);
        mk(true, wild);
        const double pyy0 = 2500.0;
        CaTblDecision d0 = off.Step(1.0, one(310.0), 0, 1, 1, 7u);
        CaTblDecision d1 = on.Step(1.0, one(310.0), 0, 1, 1, 7u);
        CaTblDecision d2 = wild.Step(1.0, one(5000.0), 0, 1, 1, 7u);
        Check(d0.fixKind == CA_TBL_FIX_NONE && d0.caseDNis < 0.0 && !d0.caseDApplied,
              "DC-4 off: a single range is discarded, as before");
        Check(d1.fixKind == CA_TBL_FIX_CASE_D && !d1.hadFix && d1.caseDApplied &&
                  d1.rangeUpdates == 1,
              "DC-4: one plausible range is applied as a scalar update; not a position fix");
        // The range runs along x, so only Pxx shrinks; Pyy keeps the prediction's value.
        Check(on.Pxx() < off.Pxx() && std::fabs(on.Pyy() - off.Pyy()) < 1e-9 && on.Pyy() > pyy0,
              "DC-4: the update shrinks P only along the range direction");
        Check(d2.fixKind == CA_TBL_FIX_CASE_D && !d2.caseDApplied && d2.caseDNis > 6.63 &&
                  wild.GetCaseDRefused() == 1 && wild.Pxx() == off.Pxx(),
              "DC-4: a wild range fails NIS <= 6.63 and is refused, P untouched");
        Check(d0.deescalated,
              "DC-4 off: the discarded single range counts toward demotion (nDeescalate 1)");
        // Its PCS is low (one range leaves P wide), so the confidence model wants to
        // escalate, and it may: the cycle went on up to UWB or stayed, never down.
        Check(d1.pcs > 0.0 && d1.wantedEscalation && !d1.deescalated &&
                  d1.nextTier != CA_TBL_TIER_868,
              "DC-4: the Case D cycle reaches the decision (PCS computed) and is not demoted");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
