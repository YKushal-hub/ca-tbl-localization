/*
 * Plan A, A2v: the error-state filter (CaTblEkf::EnableErrorStates, --imuErrorStates) on a
 * fixed deterministic stream, printed to full precision for comparison with the Python
 * reference (analysis/accuracy_replay/make_ie_vectors.py, AugEkf). No randomness: the stream
 * is closed-form in the cycle index, and make_ie_vectors.py builds the same one.
 *
 * Output: one line per state (x, y, psi, b, s) and the position block (pxx, pxy, pyy).
 * Pass/fail is decided by scripts/ie_gate.py, which runs both and compares (gate G101).
 */
#include "ns3/ca-tbl-policy.h"
#include "ns3/core-module.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace ns3;

int
main(int argc, char* argv[])
{
    CaTblEkf f;
    f.SetProcessNoise(1.0, 0.5);
    f.EnableErrorStates({4.27e-5, 0.0266, 1.6682e-4, 0.05, 1e-6});
    f.Init(100.0, 50.0, 0.0, 0.0, 100.0, 25.0);
    const double dt = 2.5;
    for (int k = 0; k < 200; ++k)
    {
        const double ux = 1.0 * std::cos(0.01 * k) + 0.3;
        const double uy = 0.8 * std::sin(0.013 * k);
        f.PredictWithControl(dt, ux, uy, 0.0, 0.0, 0.0);
        if (k % 3 == 0)
        {
            std::vector<CaTblRangeMeasurement> m(2);
            m[0].anchorX = 0.0;
            m[0].anchorY = 0.0;
            m[0].rangeM = 300.0 + 2.0 * std::sin(0.1 * k);
            m[1].anchorX = 500.0;
            m[1].anchorY = 0.0;
            m[1].rangeM = 260.0 + 3.0 * std::cos(0.07 * k);
            f.Update(m, {9.0, 16.0});
        }
        if (k % 7 == 0)
        {
            f.FusePositionCov(100.0 + 0.5 * k, 50.0 - 0.2 * k, 25.0, 3.0, 16.0);
        }
    }
    std::printf("x %.12e\ny %.12e\npsi %.12e\nb %.12e\ns %.12e\npxx %.12e\npxy %.12e\npyy %.12e\n",
                f.X(), f.Y(), f.IePsi(), f.IeBias(), f.IeScale(), f.Pxx(), f.Pxy(), f.Pyy());
    return 0;
}
