/*
 * Unit suite for CAL-01 LOOCV weight fitting.
 *
 * The dangerous failure here is a function that ALWAYS returns weights. Every check below
 * is aimed at a refusal: too few samples, a term that cannot be identified, and -- the
 * one that matters -- a fit that does not beat the single term the score is a re-encoding
 * of. K.8 says that last case is what real data is expected to produce, so it must be a
 * reported verdict and not an exception.
 */

#include "ns3/ca-tbl-loocv.h"
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

static CaTblCalSample
S(double a, double b, double c, double e)
{
    CaTblCalSample s;
    s.term1 = a; s.term2 = b; s.term3 = c; s.realisedErrorM = e;
    return s;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    std::cout << "CA-TBL CAL-01 LOOCV suite\n\n";

    // --- NEGATIVE: fewer samples than parameters ----------------------------
    {
        std::vector<CaTblCalSample> v{S(1, 0, 0, 1), S(0, 1, 0, 2)};
        CaTblCalResult r = CaTblCalibrateLoocv(v);
        Check(r.verdict == CA_TBL_CAL_TOO_FEW_SAMPLES,
              "fewer samples than free parameters is refused, not fitted");
    }

    // --- NEGATIVE: a constant term is unidentifiable -------------------------
    {
        std::vector<CaTblCalSample> v;
        for (int i = 0; i < 8; ++i)
        {
            v.push_back(S(0.1 * i, 0.5, 0.2 * i, 3.0 + i));  // term2 never varies
        }
        CaTblCalResult r = CaTblCalibrateLoocv(v);
        Check(r.verdict == CA_TBL_CAL_DEGENERATE,
              "a term that never varies is DEGENERATE -- its weight cannot be identified");
    }

    // --- a genuine three-term signal is fitted and beats the control ---------
    {
        std::vector<CaTblCalSample> v;
        for (int i = 0; i < 20; ++i)
        {
            const double a = 0.05 * i;
            const double b = 1.0 - 0.03 * i;
            const double c = (i % 5) * 0.17;
            // Error genuinely depends on all three, so three weights should win.
            v.push_back(S(a, b, c, 2.0 * a + 5.0 * b + 3.0 * c));
        }
        CaTblCalResult r = CaTblCalibrateLoocv(v);
        Check(r.verdict == CA_TBL_CAL_OK,
              "a genuine three-term dependence fits and beats the one-term control");
        Check(r.loocvRmse < r.controlRmse,
              "and the LOOCV error is actually lower, not merely reported");
        Check(std::fabs(r.w1 + r.w2 + r.w3 - 3.0) < 1e-6,
              "weights are normalised to sum to 3, the scale the thresholds were set against");
    }

    // --- THE CASE K.8 EXPECTS: the extra terms are noise ---------------------
    {
        std::vector<CaTblCalSample> v;
        for (int i = 0; i < 20; ++i)
        {
            const double a = 0.05 * (i + 1);
            const double b = ((i * 37) % 11) * 0.01;   // unrelated to the error
            const double c = ((i * 53) % 7) * 0.01;    // also unrelated
            v.push_back(S(a, b, c, 4.0 * a));          // error depends on term 1 ALONE
        }
        CaTblCalResult r = CaTblCalibrateLoocv(v);
        Check(r.verdict == CA_TBL_CAL_NO_IMPROVEMENT,
              "when only term 1 carries signal, three weights do NOT earn their parameters");
        Check(r.controlRmse >= 0.0 && r.loocvRmse >= 0.0,
              "and both errors are still reported, so the margin is inspectable");
    }

    // --- the acceptance test can be switched off, and says so ----------------
    {
        std::vector<CaTblCalSample> v;
        for (int i = 0; i < 20; ++i)
        {
            const double a = 0.05 * (i + 1);
            v.push_back(S(a, ((i * 37) % 11) * 0.01, ((i * 53) % 7) * 0.01, 4.0 * a));
        }
        CaTblCalResult r = CaTblCalibrateLoocv(v, false);
        Check(r.verdict == CA_TBL_CAL_OK,
              "requireImprovement=false returns the fit regardless -- for inspection, not for use");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
