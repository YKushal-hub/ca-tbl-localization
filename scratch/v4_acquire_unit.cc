// Gate G142 unit level (master plan v3, V4 --acquireParticles). Two anchors give two mirror roots; the particle
// filter must keep both and refuse to hand over; a third anchor must resolve it, with the handover at the true
// root. --break=force skips the unimodality test, which must hand over between or at the wrong root (red).
// "G142U PASS|FAIL <label> <detail>"; exit 1 on a FAIL.
#include "ns3/core-module.h"
#include "ns3/ca-tbl-policy.h"
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
using namespace ns3;

static int g_bad = 0;
static void Check(bool ok, const char* label, const char* why)
{
    std::printf("G142U %s %-9s %s\n", ok ? "PASS" : "FAIL", label, why);
    g_bad += ok ? 0 : 1;
}

int main(int argc, char** argv)
{
    std::string brk;
    CommandLine cmd;
    cmd.AddValue("break", "fixture mode", brk);
    cmd.Parse(argc, argv);
    int heldBimodal = 0, resolvedOk = 0, trials = 40;
    double worstErr = 0.0;
    std::mt19937_64 rng(5);
    std::normal_distribution<double> nz(0.0, 1.0);
    for (int tr = 0; tr < trials; ++tr)
    {
        CaTblPolicyConfig c;
        c.acquireParticles = true;
        c.acquireSeed = 100 + tr;
        c.acquireSpeedMps = 0.5;
        CaTblPolicy p;
        p.Configure(c);
        p.SetAcquireSalt(7);
        p.PfTestForceUnimodal(brk == "force");
        p.PfTestInit(500.0, 0.0, 1500.0);
        const double tx = 500.0, ty = (tr % 2) ? 400.0 : -400.0; // the true root alternates sides
        const double ax[3] = {0.0, 1000.0, 500.0}, ay[3] = {0.0, 0.0, 1400.0};
        bool handedEarly = false;
        double mx = 0, my = 0, cxx = 0, cxy = 0, cyy = 0;
        for (int k = 0; k < 15; ++k) // two anchors only: ambiguous by construction
        {
            std::vector<CaTblRangeMeasurement> meas;
            std::vector<double> var;
            for (int a = 0; a < 2; ++a)
            {
                CaTblRangeMeasurement m;
                m.anchorX = ax[a];
                m.anchorY = ay[a];
                m.rangeM = std::hypot(tx - ax[a], ty - ay[a]) + 3.0 * nz(rng);
                m.tier = 1;
                meas.push_back(m);
                var.push_back(9.0);
            }
            if (p.PfTestStep(2.5, meas, var, mx, my, cxx, cxy, cyy))
            {
                handedEarly = true;
                break;
            }
        }
        if (!handedEarly)
        {
            ++heldBimodal;
        }
        bool done = handedEarly;
        for (int k = 0; k < 30 && !done; ++k) // the third anchor arrives
        {
            std::vector<CaTblRangeMeasurement> meas;
            std::vector<double> var;
            for (int a = 0; a < 3; ++a)
            {
                CaTblRangeMeasurement m;
                m.anchorX = ax[a];
                m.anchorY = ay[a];
                m.rangeM = std::hypot(tx - ax[a], ty - ay[a]) + 3.0 * nz(rng);
                m.tier = 1;
                meas.push_back(m);
                var.push_back(9.0);
            }
            done = p.PfTestStep(2.5, meas, var, mx, my, cxx, cxy, cyy);
        }
        const double err = std::hypot(mx - tx, my - ty);
        worstErr = std::max(worstErr, err);
        const double l1 = 0.5 * (cxx + cyy) + std::sqrt(0.25 * (cxx - cyy) * (cxx - cyy) + cxy * cxy);
        // The right root (the mirror is 800 m away), with an error the handed-over covariance accounts for.
        if (done && !handedEarly && err < 200.0 && err <= 3.0 * std::sqrt(l1))
        {
            ++resolvedOk;
        }
    }
    char buf[200];
    std::snprintf(buf, sizeof buf, "two anchors, 15 cycles: no handover in %d/%d trials (want all)", heldBimodal, trials);
    Check(heldBimodal == trials, "HOLDS", buf);
    std::snprintf(buf, sizeof buf, "third anchor: handover at the true root (< 200 m, within 3 SD of the handed-over covariance) in %d/%d trials, worst %.1f m", resolvedOk, trials, worstErr);
    Check(resolvedOk >= trials - 1, "RESOLVES", buf);
    return g_bad ? 1 : 0;
}
