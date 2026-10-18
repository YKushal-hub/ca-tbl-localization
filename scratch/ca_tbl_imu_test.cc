/*
 * CA-TBL DC-1 unit suite: the synthetic IMU dead-reckoner and the controlled prediction
 * (docs/SPEC_CONFORMANCE_PLAN.md DC-1).
 *
 *   - zero-error parameters reproduce the true displacement exactly, with zero covariance;
 *   - the heading error's variance grows as N^2 t (angle random walk) and as sigma_b^2 t^2
 *     (bias), across 10^4 Monte-Carlo runs on distinct streams;
 *   - the reckoner takes no automatic stream index, so nothing else's draws move;
 *   - the accumulated covariance matches its formula, its increments are PSD, and a reset
 *     restarts it;
 *   - PredictWithControl() equals Predict() in mean and position block when the velocity
 *     block is zero, u = v dt and Qu = the constant-velocity Q, and it snapshots the prior.
 */

#include "ns3/ca-tbl-imu-ekf.h"
#include "ns3/ca-tbl-imu-model.h"
#include "ns3/ca-tbl-policy.h"
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
    std::cout << "CA-TBL DC-1 synthetic IMU suite\n";

    // --- zero error -----------------------------------------------------------------
    {
        CaTblImuParams z;
        z.gyroNoiseDensityRadSqrtS = 0.0;
        z.gyroBiasSigmaRadS = 0.0;
        z.scaleSigma = 0.0;
        CaTblImuDeadReckoner rk;
        rk.Configure(z, 12345);
        bool exact = true;
        bool zeroQ = true;
        for (int k = 0; k < 50; ++k)
        {
            double ux, uy, q[3];
            const double dx = 1.3 * std::cos(0.1 * k), dy = 1.3 * std::sin(0.1 * k);
            rk.Step(dx, dy, 2.5, ux, uy, q);
            exact = exact && ux == dx && uy == dy;
            zeroQ = zeroQ && q[0] == 0.0 && q[1] == 0.0 && q[2] == 0.0;
        }
        Check(exact, "zero-error parameters give u = Delta exactly");
        Check(zeroQ, "zero-error parameters give Q_u = 0 on every moving step");
    }

    // --- heading-error growth, Monte Carlo ---------------------------------------------
    {
        const int runs = 10000;
        const int steps = 40;
        const double dt = 2.5;
        CaTblImuParams arw;
        arw.gyroBiasSigmaRadS = 0.0;
        arw.scaleSigma = 0.0;
        CaTblImuParams bias;
        bias.gyroNoiseDensityRadSqrtS = 0.0;
        bias.scaleSigma = 0.0;
        double sa = 0.0, sb = 0.0, sa20 = 0.0, sb20 = 0.0;
        for (int r = 0; r < runs; ++r)
        {
            CaTblImuDeadReckoner a, b;
            a.Configure(arw, 900000 + r);
            b.Configure(bias, 700000 + r);
            for (int k = 0; k < steps; ++k)
            {
                double ux, uy, q[3];
                a.Step(1.0, 0.0, dt, ux, uy, q);
                b.Step(1.0, 0.0, dt, ux, uy, q);
                if (k == 19)
                {
                    sa20 += a.Heading() * a.Heading();
                    sb20 += b.Heading() * b.Heading();
                }
            }
            sa += a.Heading() * a.Heading();
            sb += b.Heading() * b.Heading();
        }
        const double t = steps * dt;
        const double wantA = arw.gyroNoiseDensityRadSqrtS * arw.gyroNoiseDensityRadSqrtS * t;
        const double wantB = bias.gyroBiasSigmaRadS * bias.gyroBiasSigmaRadS * t * t;
        const double gotA = sa / runs, gotB = sb / runs;
        std::cout << "  ----  ARW var " << gotA << " vs " << wantA << "; bias var " << gotB
                  << " vs " << wantB << "; ratio t=50 s to t=100 s: ARW " << (sa20 / sa)
                  << " (0.5), bias " << (sb20 / sb) << " (0.25)\n";
        Check(std::fabs(gotA / wantA - 1.0) < 0.05,
              "angle random walk: var(psi) = N^2 t to 5% over 10^4 runs");
        Check(std::fabs(gotB / wantB - 1.0) < 0.05,
              "gyro bias: var(psi) = sigma_b^2 t^2 to 5% over 10^4 runs");
        Check(std::fabs(sa20 / sa - 0.5) < 0.03 && std::fabs(sb20 / sb - 0.25) < 0.02,
              "the variance grows linearly (ARW) and quadratically (bias) in t");
    }

    // --- the dedicated stream takes no automatic index -----------------------------------
    {
        const uint64_t before = RngSeedManager::GetNextStreamIndex();
        CaTblImuDeadReckoner rk;
        rk.Configure(CaTblImuParams(), 4242);
        double ux, uy, q[3];
        rk.Step(1.0, 1.0, 2.5, ux, uy, q);
        const uint64_t after = RngSeedManager::GetNextStreamIndex();
        Check(after == before + 1,
              "configuring and stepping the reckoner takes no automatic stream index");
    }

    // --- the accumulated covariance -------------------------------------------------------
    {
        CaTblImuParams p;
        CaTblImuDeadReckoner rk;
        rk.Configure(p, 777);
        double ux, uy, q[3];
        double acc[3] = {0.0, 0.0, 0.0};
        bool psd = true, sums = true;
        for (int k = 0; k < 30; ++k)
        {
            rk.Step(1.0, 0.5, 2.5, ux, uy, q);
            const double det = q[0] * q[2] - q[1] * q[1];
            psd = psd && q[0] >= -1e-12 && q[2] >= -1e-12 && det >= -1e-9 * (q[0] + q[2] + 1e-30);
            for (int i = 0; i < 3; ++i)
            {
                acc[i] += q[i];
            }
        }
        const double* s = rk.SigmaAcc();
        const double t = rk.ElapsedS(), Dx = rk.AccX(), Dy = rk.AccY();
        const double vpsi = p.gyroBiasSigmaRadS * p.gyroBiasSigmaRadS * t * t +
                            p.gyroNoiseDensityRadSqrtS * p.gyroNoiseDensityRadSqrtS * t;
        const double ss = p.scaleSigma * p.scaleSigma;
        const double want[3] = {vpsi * Dy * Dy + ss * Dx * Dx, -vpsi * Dx * Dy + ss * Dx * Dy,
                                vpsi * Dx * Dx + ss * Dy * Dy};
        bool formula = true;
        for (int i = 0; i < 3; ++i)
        {
            formula = formula && std::fabs(s[i] - want[i]) <= 1e-9 * std::fabs(want[i]) + 1e-12;
            sums = sums && std::fabs(acc[i] - s[i]) <= 1e-6 * std::fabs(s[i]) + 1e-9;
        }
        Check(formula, "Sigma_acc = var_psi(t) |D|^2 p p^T + sigma_s^2 D D^T");
        Check(psd, "every increment Q_u is positive semi-definite");
        Check(sums, "on a straight walk the increments sum to Sigma_acc");
        rk.ResetAccumulation();
        Check(rk.SigmaAcc()[0] == 0.0 && rk.AccX() == 0.0 && rk.Heading() != 0.0,
              "a reset restarts the position error and keeps the heading error");
        rk.Step(0.0, 0.0, 2.5, ux, uy, q);
        Check(q[0] == p.dwellSigmaM * p.dwellSigmaM && q[1] == 0.0 && ux == 0.0,
              "a dwelling step gives Q_u = sigma_w^2 I and no displacement");
    }

    // --- PredictWithControl against Predict ------------------------------------------------
    {
        CaTblEkf a, b;
        a.Init(10.0, -5.0, 1.2, -0.4, 400.0, 0.0); // velocity block zero
        b.Init(10.0, -5.0, 1.2, -0.4, 400.0, 0.0);
        const double dt = 2.5;
        a.Predict(dt);
        const double q = 1.0 * 1.0 * dt; // the CV Q's position term, qPos = 1
        b.PredictWithControl(dt, 1.2 * dt, -0.4 * dt, q, 0.0, q);
        Check(std::fabs(a.X() - b.X()) < 1e-12 && std::fabs(a.Y() - b.Y()) < 1e-12 &&
                  std::fabs(a.Pxx() - b.Pxx()) < 1e-9 && std::fabs(a.Pyy() - b.Pyy()) < 1e-9 &&
                  std::fabs(a.Pxy() - b.Pxy()) < 1e-12,
              "PredictWithControl = Predict in mean and position block (zero velocity block)");
        Check(b.PriorX() == b.X() && b.PriorY() == b.Y(),
              "PredictWithControl snapshots the prior the integrity check reads");
    }

    // --- N.33 CONSIDER: a fix of fewer than 3 ranges is a Schmidt update -------------------
    // One or two ranges cannot observe theta, b or s; under mode 8 they must not move those
    // states or their variances, while position still moves. Three ranges are not considered,
    // and mode 0 lets one range move theta -- the defect the mode exists for.
    {
        auto primed = [](int mode) {
            CaTblImuEkf f;
            f.Init(0.0, 0.0, 25.0, 0.01, 1e-6, 1e-4);
            f.SetRobust(mode);
            for (int k = 0; k < 20; ++k)
            {
                f.Predict(2.5, 3.0, 0.3, 1e-4, 1e-5, 1e-10); // builds pos-theta covariance
            }
            return f;
        };
        auto ranges = [](const CaTblImuEkf& f, int n, std::vector<CaTblRangeMeasurement>& m,
                         std::vector<double>& v) {
            const double ax[3] = {200.0, -150.0, 40.0}, ay[3] = {30.0, 120.0, -180.0};
            for (int i = 0; i < n; ++i)
            {
                CaTblRangeMeasurement r;
                r.anchorX = f.X() + ax[i];
                r.anchorY = f.Y() + ay[i];
                r.rangeM = std::sqrt(ax[i] * ax[i] + ay[i] * ay[i]) + 7.0; // a 7 m innovation
                m.push_back(r);
                v.push_back(9.0);
            }
        };
        for (int n = 1; n <= 3; ++n)
        {
            for (int mode : {0, 8})
            {
                CaTblImuEkf f = primed(mode);
                std::vector<CaTblRangeMeasurement> m;
                std::vector<double> v;
                ranges(f, n, m, v);
                const double x0 = f.X(), y0 = f.Y(), t0 = f.HeadingOffset(), b0 = f.GyroBias(),
                             s0 = f.StrideScale(), p22 = f.Cov(2, 2), p33 = f.Cov(3, 3),
                             p44 = f.Cov(4, 4);
                f.Update(m, v);
                const bool posMoved = std::hypot(f.X() - x0, f.Y() - y0) > 1e-3;
                const bool sensorsFixed = f.HeadingOffset() == t0 && f.GyroBias() == b0 &&
                                          f.StrideScale() == s0 && f.Cov(2, 2) == p22 &&
                                          f.Cov(3, 3) == p33 && f.Cov(4, 4) == p44;
                bool psd = true;
                for (int i = 0; i < CaTblImuEkf::N; ++i)
                {
                    psd = psd && f.Cov(i, i) >= 0.0;
                    for (int j = 0; j < CaTblImuEkf::N; ++j)
                    {
                        psd = psd && f.Cov(i, j) == f.Cov(j, i);
                    }
                }
                const bool considered = (mode == 8 && n < 3);
                std::string what = "N.33 mode " + std::to_string(mode) + ", " +
                                   std::to_string(n) + " range(s): position moves, " +
                                   (considered ? "theta/b/s and their variances unchanged"
                                               : "theta moves through its covariance") +
                                   ", P symmetric with a non-negative diagonal";
                Check(posMoved && psd &&
                          (considered ? sensorsFixed : std::fabs(f.HeadingOffset() - t0) > 1e-9) &&
                          f.RobustConsidered() == (considered ? 1u : 0u),
                      what.c_str());
            }
        }
    }

    // --- --imuStrideSigmaPct: per-step stride noise, its spread and its stated variance agree --------
    {
        CaTblImuParams p;
        p.gyroNoiseDensityRadSqrtS = 0.0;
        p.gyroBiasSigmaRadS = 0.0;
        p.scaleSigma = 0.0;
        p.strideSigmaRel = 0.05;
        CaTblImuDeadReckoner rk;
        rk.Configure(p, 4242);
        double se = 0.0, sq = 0.0;
        const int n = 20000;
        const double d = 3.0;
        for (int k = 0; k < n; ++k)
        {
            double ux, uy, q[3];
            rk.Step(d, 0.0, 2.5, ux, uy, q);
            se += (ux - d) * (ux - d);
            sq += q[0];
            rk.ResetAccumulation();
        }
        const double want = 0.05 * 0.05 * 0.7 * d; // sigma_rel^2 l |d|
        Check(std::fabs(se / n / want - 1.0) < 0.05,
              "stride noise: per-cycle along-track variance = sigma_rel^2 l |d| within 5 % (20 000 cycles)");
        Check(std::fabs(sq / n / want - 1.0) < 1e-9, "stride noise: Q_u's along-track increment states exactly that variance");
        CaTblImuParams off;
        off.gyroNoiseDensityRadSqrtS = 0.0;
        off.gyroBiasSigmaRadS = 0.0;
        off.scaleSigma = 0.0;
        CaTblImuDeadReckoner r0;
        r0.Configure(off, 4242);
        double ux, uy, q[3];
        r0.Step(d, 0.0, 2.5, ux, uy, q);
        Check(ux == d && uy == 0.0 && q[0] == 0.0, "stride noise off: the stride is exact and Q_u is zero, as before");
    }

    // --- --imuMagSigmaDeg: the magnetometer reads psi with its stated noise; ekf5 fuses it as a theta reading --
    {
        CaTblImuParams p;
        p.gyroBiasSigmaRadS = 0.01; // a large bias, so psi wanders far
        p.magSigmaRad = 5.0 * M_PI / 180.0;
        p.magGmSigmaRad = 3.0 * M_PI / 180.0;
        p.magGmTauS = 120.0;
        CaTblImuDeadReckoner rk;
        rk.Configure(p, 99);
        double se = 0.0;
        const int n = 40000;
        for (int k = 0; k < n; ++k)
        {
            double ux, uy, q[3];
            rk.Step(3.0, 0.0, 2.5, ux, uy, q);
            const double e = rk.MagTheta() - rk.Heading();
            se += e * e;
        }
        Check(std::fabs(se / n / rk.MagVar() - 1.0) < 0.10,
              "magnetometer: reading - psi has the stated variance (white + Gauss-Markov) within 10 % over 40 000 cycles");
        CaTblImuEkf f;
        f.Init(0.0, 0.0, 25.0, 0.5, 1e-6, 1e-4);
        const double p0 = f.Cov(2, 2);
        f.FuseHeading(0.3, 0.01);
        Check(f.Cov(2, 2) < p0 && std::fabs(f.HeadingOffset() - 0.3 * 0.5 / 0.51) < 1e-12,
              "magnetometer: ekf5 FuseHeading is the scalar update on theta (gain P/(P+r), variance shrinks)");
    }

    // --- --ekf5FinderConsider: a constant finder offset is not averaged away ------------------------------
    // A static tag at the origin ranges, 60 times, one finder whose reported position is off by a constant
    // N(0, 4 I) offset, and two surveyed anchors; ToF sigma 0.3 m. Naive: the offset variance folded into
    // every range's R, fused as independent. Consider: R is the ToF part only, the offset a consider parameter.
    {
        auto run = [](bool consider, double& meanNees) {
            uint64_t z = 1234567891234567ull;
            auto u01 = [&z]() {
                z ^= z << 13;
                z ^= z >> 7;
                z ^= z << 17;
                return ((z >> 11) + 0.5) / 9007199254740992.0;
            };
            auto gauss = [&u01]() { return std::sqrt(-2.0 * std::log(u01())) * std::cos(2.0 * M_PI * u01()); };
            double sn = 0.0;
            const int nmc = 300;
            for (int k = 0; k < nmc; ++k)
            {
                CaTblImuEkf f;
                f.Init(0.0, 0.0, 100.0, 1e-6, 1e-12, 1e-8);
                f.SetFinderConsider(consider, 4.0);
                const double ox = 2.0 * gauss(), oy = 2.0 * gauss();
                const double fx = 40.0, fy = 10.0; // the finder's TRUE position
                for (int c = 0; c < 60; ++c)
                {
                    std::vector<CaTblRangeMeasurement> m(3);
                    std::vector<double> v(3);
                    m[0].anchorX = fx + ox; // reported, offset
                    m[0].anchorY = fy + oy;
                    m[0].anchorId = 501;
                    m[0].sourceKind = CA_TBL_SRC_FINDER;
                    m[0].rangeM = std::hypot(fx, fy) + 0.3 * gauss();
                    v[0] = consider ? 0.09 : 0.09 + 4.0;
                    const double gx[2] = {-60.0, 5.0}, gy[2] = {20.0, -70.0};
                    for (int a = 0; a < 2; ++a)
                    {
                        m[1 + a].anchorX = gx[a];
                        m[1 + a].anchorY = gy[a];
                        m[1 + a].anchorId = 101 + a;
                        m[1 + a].sourceKind = CA_TBL_SRC_SURVEYED_GATEWAY;
                        m[1 + a].rangeM = std::hypot(gx[a], gy[a]) + 25.0 * gauss();
                        v[1 + a] = 625.0;
                    }
                    f.Update(m, v);
                }
                const double ex = f.X(), ey = f.Y();
                const double det = f.Pxx() * f.Pyy() - f.Pxy() * f.Pxy();
                sn += (f.Pyy() * ex * ex - 2.0 * f.Pxy() * ex * ey + f.Pxx() * ey * ey) / det;
            }
            meanNees = sn / nmc;
        };
        double nNaive, nCons;
        run(false, nNaive);
        run(true, nCons);
        std::cout << "    finder consider: mean position NEES naive " << nNaive << ", consider " << nCons << "\n";
        Check(nCons > 1.0 && nCons < 3.5, "finder consider: position NEES within [1, 3.5] (chi^2_2 mean 2) with a constant 2 m offset");
        Check(nNaive > 2.0 * nCons, "finder consider: the naive fusion is more than twice as overconfident (the defect)");
    }

    // --- --ekf5RelayGate: a relayed range far outside the prior is not fused; the filter's own range is --------
    {
        auto one = [](uint8_t kind, bool gate, double& moved, uint64_t& gated) {
            CaTblImuEkf f;
            f.Init(0.0, 0.0, 25.0, 1e-4, 1e-10, 1e-6);
            f.SetRelayGate(gate);
            std::vector<CaTblRangeMeasurement> m(1);
            std::vector<double> v(1, 9.0);
            m[0].anchorX = 100.0;
            m[0].anchorY = 0.0;
            m[0].anchorId = 3;
            m[0].sourceKind = kind;
            m[0].rangeM = 160.0; // 60 m off a 5 m prior: NIS ~ 3600 / 34
            f.Update(m, v);
            moved = std::hypot(f.X(), f.Y());
            gated = f.RelayGated();
        };
        double mv;
        uint64_t g;
        one(CA_TBL_SRC_RELAY, true, mv, g);
        Check(mv == 0.0 && g == 1, "relay gate: a relayed range with NIS far above 10.83 is not fused, and is counted");
        one(CA_TBL_SRC_FINDER, true, mv, g);
        Check(mv > 1.0 && g == 0, "relay gate: the filter's own range with the same innovation is fused (no self-lock-out)");
        one(CA_TBL_SRC_RELAY, false, mv, g);
        Check(mv > 1.0 && g == 0, "relay gate off: the relayed range is fused as before");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
