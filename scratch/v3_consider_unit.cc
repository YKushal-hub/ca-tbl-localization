// Gate G141 unit level (master plan v3, V3 --rangeBiasStates). Monte Carlo of CaTblEkf with correlated range
// errors: a static node ranged K times from 3 anchors whose errors are a constant bias per anchor (fully
// correlated, the static-node case) plus a small white part. With the consider states the 2-D NEES of the
// final estimate should average ~2; without them the filter averages the bias away and NEES grows ~K-fold.
// A moving case checks decorrelation: the node walks 20 correlation lengths between ranges, so the biases are
// renewed and the filter should again be consistent. "G141U PASS|FAIL <label> <detail>"; exit 1 on a FAIL.
// --break=off runs the consider case with the states disabled, which must turn CONSIST red (rule 6).
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
    std::printf("G141U %s %-9s %s\n", ok ? "PASS" : "FAIL", label, why);
    g_bad += ok ? 0 : 1;
}

static double RunCase(bool consider, bool moving, int K, int trials, uint32_t seed)
{
    std::mt19937_64 rng(seed);
    std::normal_distribution<double> nz(0.0, 1.0);
    const double ax[3] = {0.0, 1000.0, 0.0}, ay[3] = {0.0, 0.0, 1000.0};
    const double sigma = 30.0, white = 0.05, corrM = 30.0;
    double neesSum = 0.0;
    for (int tr = 0; tr < trials; ++tr)
    {
        CaTblEkf ekf;
        ekf.SetProcessNoise(1e-6, 1e-6);
        if (consider)
        {
            ekf.EnableConsider(corrM, white);
        }
        double tx = 400.0, ty = 300.0;
        ekf.Init(tx + 80.0 * nz(rng), ty + 80.0 * nz(rng), 0.0, 0.0, 80.0 * 80.0, 1e-6);
        double bias[3];
        for (int a = 0; a < 3; ++a)
        {
            bias[a] = std::sqrt(1.0 - white) * sigma * nz(rng);
        }
        for (int k = 0; k < K; ++k)
        {
            if (moving && k > 0)
            {
                // move 20 correlation lengths in x and back: biases renew; the filter is told via velocity.
                const double step = 20.0 * corrM;
                // drive the EKF with a control equal to the true displacement and negligible noise
                const double dxm = (k % 2) ? step : -step;
                tx += dxm;
                ekf.PredictWithControl(1.0, dxm, 0.0, 1e-6, 0.0, 1e-6);
                for (int a = 0; a < 3; ++a)
                {
                    bias[a] = std::sqrt(1.0 - white) * sigma * nz(rng);
                }
            }
            else if (k > 0)
            {
                ekf.Predict(1.0);
            }
            std::vector<CaTblRangeMeasurement> meas;
            std::vector<double> var;
            for (int a = 0; a < 3; ++a)
            {
                CaTblRangeMeasurement m;
                m.anchorX = ax[a];
                m.anchorY = ay[a];
                m.rangeM = std::hypot(tx - ax[a], ty - ay[a]) + bias[a] + std::sqrt(white) * sigma * nz(rng);
                m.sigmaM = sigma;
                m.anchorId = static_cast<uint16_t>(101 + a);
                m.tier = 0;
                meas.push_back(m);
                var.push_back(sigma * sigma);
            }
            ekf.Update(meas, var);
        }
        const double ex = ekf.X() - tx, ey = ekf.Y() - ty;
        const double pxx = ekf.Pxx(), pxy = ekf.Pxy(), pyy = ekf.Pyy();
        const double det = pxx * pyy - pxy * pxy;
        neesSum += (pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det;
    }
    return neesSum / trials;
}

int main(int argc, char** argv)
{
    std::string brk;
    CommandLine cmd;
    cmd.AddValue("break", "fixture mode", brk);
    cmd.Parse(argc, argv);
    char buf[256];
    const double withC = RunCase(brk != "off", false, 30, 3000, 11);
    const double without = RunCase(false, false, 30, 3000, 11);
    std::snprintf(buf, sizeof buf, "static node, 30 updates, fully correlated: mean NEES consider %.2f (want 1.6-2.5), plain %.2f", withC, without);
    Check(withC >= 1.6 && withC <= 2.5, "CONSIST", buf);
    std::snprintf(buf, sizeof buf, "plain filter overconfident: mean NEES %.2f (want > 8)", without);
    Check(without > 8.0, "CONTROL", buf);
    const double mv = RunCase(true, true, 30, 3000, 12);
    std::snprintf(buf, sizeof buf, "moving node, biases renewed each update: mean NEES consider %.2f (want 1.6-2.5)", mv);
    Check(mv >= 1.6 && mv <= 2.5, "DECORR", buf);
    return g_bad ? 1 : 0;
}
