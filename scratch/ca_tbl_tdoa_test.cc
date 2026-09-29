/*
 * Unit suite for tier-1 TDoA (Physical sect 1.1; TECH_DEBT J.2, L.2.1).
 *
 * WHAT IS UNDER TEST: the arrival-time bound, the hyperbolic solver (Chan's closed form,
 * Foy's iteration, the refusals), the covariance it hands the filter, the filter's 2x2
 * position update, and the policy hand-off that fuses the fix inside Step().
 *
 * HOW THE EXPECTED VALUES ARE MADE. Never by calling the code under test a second time.
 * The covariance is recomputed here from the geometry with the FULL difference covariance
 * inverted by Gauss-Jordan elimination, not by the Sherman-Morrison form the solver uses;
 * the filter update is checked against the information form (P^-1 + R^-1)^-1, not the
 * gain form the filter uses. A wrong formula that agreed with itself would pass a
 * self-comparison and measure nothing (the same reason lora-ranging-submodule.cc and the
 * scenario each carry their own CRLB).
 *
 * THE NEGATIVE FIXTURES are the refusals -- too few anchors, collinear or coincident
 * anchors, a non-finite stamp, a zero sigma, a covariance that is not positive definite,
 * a cycle the integrity check rejects -- each beside a positive control that differs from
 * it in the one respect the refusal is about. And one fixture that shows a check CAN
 * fail: the covariance with the reference anchor's shared error dropped is required to
 * DIFFER from the solver's, or check 11 could not tell the two apart.
 *
 * Random draws use a local mt19937_64 with a fixed seed and an explicit Box-Muller, so
 * the numbers do not depend on the library's normal_distribution and touch no ns-3 stream.
 */

#include "ns3/ca-tbl-policy.h"
#include "ns3/ca-tbl-tdoa.h"
#include "ns3/core-module.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

static const double kC = 299792458.0;

struct Pt
{
    double x;
    double y;
};

//! Stamps for a node at \p p emitting at \p t0, each anchor with its own sigma.
static std::vector<CaTblTdoaObs>
MakeObs(const std::vector<Pt>& a, const std::vector<double>& sigS, Pt p, double t0)
{
    std::vector<CaTblTdoaObs> o;
    for (size_t k = 0; k < a.size(); ++k)
    {
        CaTblTdoaObs s;
        s.anchorId = static_cast<uint16_t>(101 + k);
        s.x = a[k].x;
        s.y = a[k].y;
        s.tArrS = t0 + std::hypot(p.x - a[k].x, p.y - a[k].y) / kC;
        s.sigmaS = sigS[k];
        o.push_back(s);
    }
    return o;
}

//! General n x n inverse by Gauss-Jordan with partial pivoting. False when singular.
static bool
InvertN(std::vector<double> A, size_t n, std::vector<double>& Ai)
{
    Ai.assign(n * n, 0.0);
    for (size_t i = 0; i < n; ++i)
    {
        Ai[i * n + i] = 1.0;
    }
    for (size_t c = 0; c < n; ++c)
    {
        size_t piv = c;
        for (size_t r = c + 1; r < n; ++r)
        {
            if (std::fabs(A[r * n + c]) > std::fabs(A[piv * n + c]))
            {
                piv = r;
            }
        }
        if (std::fabs(A[piv * n + c]) < 1e-300)
        {
            return false;
        }
        for (size_t k = 0; k < n; ++k)
        {
            std::swap(A[c * n + k], A[piv * n + k]);
            std::swap(Ai[c * n + k], Ai[piv * n + k]);
        }
        const double d = A[c * n + c];
        for (size_t k = 0; k < n; ++k)
        {
            A[c * n + k] /= d;
            Ai[c * n + k] /= d;
        }
        for (size_t r = 0; r < n; ++r)
        {
            if (r == c)
            {
                continue;
            }
            const double f = A[r * n + c];
            for (size_t k = 0; k < n; ++k)
            {
                A[r * n + k] -= f * A[c * n + k];
                Ai[r * n + k] -= f * Ai[c * n + k];
            }
        }
    }
    return true;
}

/**
 * (J' Q^-1 J)^-1 at \p p, reference \p ref, from first principles. \p shared = false
 * drops the reference anchor's common error from Q -- the omission the solver's own
 * comment warns against -- so the suite can show the two are distinguishable.
 */
static void
ReferenceCov(const std::vector<Pt>& a,
             const std::vector<double>& sigS,
             size_t ref,
             Pt p,
             bool shared,
             double& rxx,
             double& rxy,
             double& ryy)
{
    std::vector<size_t> idx;
    for (size_t k = 0; k < a.size(); ++k)
    {
        if (k != ref)
        {
            idx.push_back(k);
        }
    }
    const size_t n = idx.size();
    std::vector<double> Q(n * n, 0.0);
    const double sr = kC * sigS[ref];
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            const double si = kC * sigS[idx[i]];
            Q[i * n + j] = (shared ? sr * sr : 0.0) + ((i == j) ? si * si : 0.0);
        }
    }
    std::vector<double> W;
    InvertN(Q, n, W);
    const double rr = std::hypot(p.x - a[ref].x, p.y - a[ref].y);
    std::vector<double> J(2 * n);
    for (size_t i = 0; i < n; ++i)
    {
        const double rk = std::hypot(p.x - a[idx[i]].x, p.y - a[idx[i]].y);
        J[2 * i] = (p.x - a[idx[i]].x) / rk - (p.x - a[ref].x) / rr;
        J[2 * i + 1] = (p.y - a[idx[i]].y) / rk - (p.y - a[ref].y) / rr;
    }
    double f00 = 0.0;
    double f01 = 0.0;
    double f11 = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            f00 += J[2 * i] * W[i * n + j] * J[2 * j];
            f01 += J[2 * i] * W[i * n + j] * J[2 * j + 1];
            f11 += J[2 * i + 1] * W[i * n + j] * J[2 * j + 1];
        }
    }
    const double det = f00 * f11 - f01 * f01;
    rxx = f11 / det;
    rxy = -f01 / det;
    ryy = f00 / det;
}

static bool
Near(double a, double b, double relTol)
{
    return std::fabs(a - b) <= relTol * std::max(std::fabs(a), std::fabs(b));
}

//! A standard normal from two 53-bit uniforms, Box-Muller, cosine branch only.
static double
Gauss(std::mt19937_64& g)
{
    const double u1 = (static_cast<double>(g() >> 11) + 1.0) * (1.0 / 9007199254740993.0);
    const double u2 = static_cast<double>(g() >> 11) * (1.0 / 9007199254740992.0);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
}

static std::vector<CaTblRangeMeasurement>
Ranges(const std::vector<Pt>& a, Pt p, double sigmaM)
{
    std::vector<CaTblRangeMeasurement> m;
    for (size_t k = 0; k < a.size(); ++k)
    {
        CaTblRangeMeasurement r;
        r.anchorX = a[k].x;
        r.anchorY = a[k].y;
        r.rangeM = std::hypot(p.x - a[k].x, p.y - a[k].y);
        r.nlos = false;
        r.nlosPropagation = false;
        r.hiddenChannelActive = false;
        r.sigmaM = sigmaM;
        m.push_back(r);
    }
    return m;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    std::cout << "CA-TBL tier-1 TDoA suite (Physical sect 1.1)\n\n";

    // ---------------------------------------------------------------- the bound --
    {
        // By hand: beta_rms = 125e3/sqrt(12) = 36084.39 Hz; E/N0 = 2^7 = 128 at 0 dB
        // in-band; sigma_t = 1 / (2 pi 36084.39 sqrt(256)) = 275.66 ns.
        const double s0 = CaTblTdoaArrivalSigmaS(125e3, 7, 0.0);
        Check(std::fabs(s0 - 275.66e-9) < 0.05e-9,
              "SF7/125 kHz at 0 dB in-band SNR gives the hand-computed 275.66 ns");
        Check(Near(CaTblTdoaArrivalSigmaS(125e3, 7, 10.0), s0 / std::sqrt(10.0), 1e-12) &&
                  Near(CaTblTdoaArrivalSigmaS(125e3, 8, 0.0), s0 / std::sqrt(2.0), 1e-12) &&
                  Near(CaTblTdoaArrivalSigmaS(250e3, 7, 0.0), s0 / 2.0, 1e-12),
              "the bound scales as 1/sqrt(SNR), 1/sqrt(2^SF) and 1/BW");
        Check(std::isinf(CaTblTdoaArrivalSigmaS(0.0, 7, 0.0)) &&
                  std::isinf(CaTblTdoaArrivalSigmaS(125e3, 7,
                                                    std::numeric_limits<double>::quiet_NaN())),
              "NEGATIVE: zero bandwidth or a NaN SNR gives +inf, not a number");
    }

    const std::vector<Pt> sq = {{0, 0}, {2000, 0}, {0, 2000}, {2000, 2000}};
    const Pt truth = {700, 1300};
    const std::vector<double> sig4 = {60e-9, 40e-9, 80e-9, 50e-9}; // ref = index 1

    // ------------------------------------------------------ noise-free geometry --
    {
        const auto obs = MakeObs(sq, sig4, truth, 0.0);
        double cx = 0.0;
        double cy = 0.0;
        const bool chan = CaTblTdoaChan(obs, 1, cx, cy);
        Check(chan && std::hypot(cx - truth.x, cy - truth.y) < 1e-4,
              "Chan's closed form recovers the truth from exact stamps, four anchors");

        const CaTblTdoaResult r = CaTblSolveTdoa(obs, 1e5, -1e5);
        Check(r.verdict == CA_TBL_TDOA_OK && r.seededBy == CA_TBL_TDOA_SEED_CHAN &&
                  std::hypot(r.x - truth.x, r.y - truth.y) < 0.01,
              "four anchors: Chan seeds, Foy refines, a seed 140 km off is never needed");
        Check(r.refAnchorId == 102, "the reference is the best-timed anchor (40 ns, id 102)");

        const auto late = MakeObs(sq, sig4, truth, 1234.5678);
        const CaTblTdoaResult rl = CaTblSolveTdoa(late, 1e5, -1e5);
        Check(rl.verdict == CA_TBL_TDOA_OK &&
                  std::hypot(rl.x - r.x, rl.y - r.y) < 1e-3,
              "an unknown emission instant (t0 = 1234.5678 s) cancels in the differences");
    }

    // --------------------------------------------------------- three anchors --
    {
        const std::vector<Pt> tri = {{0, 0}, {2000, 0}, {0, 2000}};
        const auto obs = MakeObs(tri, {50e-9, 50e-9, 50e-9}, truth, 0.0);
        const CaTblTdoaResult r = CaTblSolveTdoa(obs, 650.0, 1250.0);
        Check(r.verdict == CA_TBL_TDOA_OK && r.seededBy == CA_TBL_TDOA_SEED_ROOT &&
                  !r.chanOk && std::hypot(r.x - truth.x, r.y - truth.y) < 1e-3 &&
                  r.chi2 < 1e-9,
              "exactly three anchors: no Chan, solved in closed form, square system, chi2 ~ 0");

        std::vector<CaTblTdoaObs> two(obs.begin(), obs.begin() + 2);
        Check(CaTblSolveTdoa(two, 650.0, 1250.0).verdict == CA_TBL_TDOA_TOO_FEW,
              "NEGATIVE: two anchors are TOO_FEW -- one difference cannot fix two unknowns");
    }

    // ------------------------------------------ three anchors, TWO exact roots --
    {
        // Found by a separate scan and verified by substituting the second point into
        // the range differences directly (error 2.7e-12 m): both points fit the stamps.
        const std::vector<Pt> tri = {{0, 0}, {1000, 0}, {0, 1000}};
        const Pt t2 = {-100.0, 1200.0};
        const Pt mirror = {-10.607, 993.1323};
        const std::vector<double> s3 = {50e-9, 50e-9, 50e-9};
        const auto obs = MakeObs(tri, s3, t2, 0.0);

        double rx[2];
        double ry[2];
        const int nr = CaTblTdoaThreeAnchorRoots(obs, 0, rx, ry);
        bool bothExact = (nr == 2);
        bool hasTruth = false;
        bool hasMirror = false;
        for (int i = 0; i < nr && i < 2; ++i)
        {
            const double r0 = std::hypot(rx[i] - tri[0].x, ry[i] - tri[0].y);
            for (size_t k = 1; k < 3; ++k)
            {
                const double dk = std::hypot(rx[i] - tri[k].x, ry[i] - tri[k].y) - r0;
                const double dt = std::hypot(t2.x - tri[k].x, t2.y - tri[k].y) -
                                  std::hypot(t2.x - tri[0].x, t2.y - tri[0].y);
                bothExact = bothExact && std::fabs(dk - dt) < 1e-6;
            }
            hasTruth = hasTruth || std::hypot(rx[i] - t2.x, ry[i] - t2.y) < 1e-4;
            hasMirror = hasMirror || std::hypot(rx[i] - mirror.x, ry[i] - mirror.y) < 1e-3;
        }
        Check(bothExact && hasTruth && hasMirror,
              "three anchors: BOTH exact roots found, the truth and its mirror 225 m away");

        auto hintsAt = [&](Pt p, double sig) {
            std::vector<CaTblTdoaRangeHint> h;
            for (size_t k = 0; k < tri.size(); ++k)
            {
                CaTblTdoaRangeHint e;
                e.x = tri[k].x;
                e.y = tri[k].y;
                e.rangeM = std::hypot(p.x - tri[k].x, p.y - tri[k].y);
                e.sigmaM = sig;
                h.push_back(e);
            }
            return h;
        };
        const Pt mid = {0.5 * (t2.x + mirror.x), 0.5 * (t2.y + mirror.y)};
        const CaTblTdoaResult rt =
            CaTblSolveTdoa(obs, mid.x, mid.y, CaTblTdoaConfig(), hintsAt(t2, 20.0));
        const CaTblTdoaResult rm =
            CaTblSolveTdoa(obs, mid.x, mid.y, CaTblTdoaConfig(), hintsAt(mirror, 20.0));
        Check(rt.verdict == CA_TBL_TDOA_OK && rt.nRoots == 2 &&
                  std::hypot(rt.x - t2.x, rt.y - t2.y) < 1e-3 &&
                  rm.verdict == CA_TBL_TDOA_OK &&
                  std::hypot(rm.x - mirror.x, rm.y - mirror.y) < 1e-3,
              "the RSSI ranges choose the root: ranges at the truth pick it, ranges at the "
              "mirror pick the mirror, from the same seed halfway between");

        const CaTblTdoaResult ra =
            CaTblSolveTdoa(obs, t2.x, t2.y, CaTblTdoaConfig(), hintsAt(t2, 5000.0));
        Check(ra.verdict == CA_TBL_TDOA_AMBIGUOUS && ra.tieChi2Other - ra.tieChi2Chosen < 4.0,
              "NEGATIVE: ranges too coarse to separate the roots (sigma 5 km) are AMBIGUOUS, "
              "even with the seed ON the truth");

        const CaTblTdoaResult rs = CaTblSolveTdoa(obs, t2.x + 10.0, t2.y + 10.0);
        const CaTblTdoaResult rq = CaTblSolveTdoa(obs, mid.x, mid.y);
        Check(rs.verdict == CA_TBL_TDOA_OK && std::hypot(rs.x - t2.x, rs.y - t2.y) < 1e-3 &&
                  rq.verdict == CA_TBL_TDOA_AMBIGUOUS,
              "no ranges: a seed 14 m from one root picks it; a seed equidistant is AMBIGUOUS");

        // |r_1 - r_ref| can never exceed the 1000 m baseline: no point has these stamps.
        auto bad = obs;
        bad[1].tArrS = bad[0].tArrS + 1500.0 / kC;
        bad[2].tArrS = bad[0].tArrS;
        Check(CaTblSolveTdoa(bad, 500.0, 500.0).verdict == CA_TBL_TDOA_NO_ROOT,
              "NEGATIVE: a difference longer than its baseline is NO_ROOT, not a nearby guess");
    }

    // ------------------------------------------------------ the runaway guard --
    {
        const std::vector<Pt> tight = {{0, 0}, {200, 0}, {0, 200}, {200, 200}};
        const Pt far = {1500.0, 800.0}; // 1565 m from the centroid; extent 141 m
        const std::vector<double> s = {50e-9, 50e-9, 50e-9, 50e-9};
        const auto obs = MakeObs(tight, s, far, 0.0);
        CaTblTdoaConfig wide;
        wide.maxExtentFactor = 100.0;
        const CaTblTdoaResult rd = CaTblSolveTdoa(obs, 100.0, 100.0);
        const CaTblTdoaResult rw = CaTblSolveTdoa(obs, 100.0, 100.0, wide);
        Check(rd.verdict == CA_TBL_TDOA_DIVERGED && rw.verdict == CA_TBL_TDOA_OK &&
                  std::hypot(rw.x - far.x, rw.y - far.y) < 0.1,
              "a solution beyond 10 anchor extents is DIVERGED; the same with the bound at "
              "100 extents is OK -- the guard, and only the guard, refuses it");

        auto imp = MakeObs(sq, sig4, truth, 0.0);
        imp[1].tArrS = imp[0].tArrS + 2600.0 / kC; // 2000 m baseline
        imp[2].tArrS = imp[0].tArrS - 2600.0 / kC;
        const CaTblTdoaResult ri = CaTblSolveTdoa(imp, 1000.0, 1000.0);
        std::cout << "        impossible four-anchor stamps -> "
                  << CaTblTdoaVerdictName(ri.verdict) << " at (" << ri.x << ", " << ri.y
                  << ")\n";
        Check(ri.verdict != CA_TBL_TDOA_OK,
              "NEGATIVE: four anchors with differences longer than their baselines give no fix");
    }

    // ------------------------------------------------------ degenerate geometry --
    {
        const std::vector<Pt> line = {{0, 0}, {1000, 0}, {2000, 0}, {3000, 0}};
        const Pt off = {1500, 800};
        const std::vector<double> s = {50e-9, 50e-9, 50e-9, 50e-9};
        Check(CaTblSolveTdoa(MakeObs(line, s, off, 0.0), 1500.0, 700.0).verdict ==
                  CA_TBL_TDOA_COLLINEAR,
              "NEGATIVE: four collinear anchors are refused -- the side of the line is unknowable");

        const std::vector<Pt> lifted = {{0, 0}, {1000, 0}, {2000, 0}, {3000, 90}};
        const CaTblTdoaResult r = CaTblSolveTdoa(MakeObs(lifted, s, off, 0.0), 1500.0, 700.0);
        Check(r.verdict == CA_TBL_TDOA_OK && std::hypot(r.x - off.x, r.y - off.y) < 0.1,
              "positive control: the same line with one anchor 90 m off it (2.2% spread) solves");

        const std::vector<Pt> same = {{500, 500}, {500, 500}, {500, 500}, {500, 500}};
        Check(CaTblSolveTdoa(MakeObs(same, s, off, 0.0), 1500.0, 700.0).verdict ==
                  CA_TBL_TDOA_COLLINEAR,
              "NEGATIVE: coincident anchors are refused, not solved near the origin");
    }

    // ------------------------------------------------------------- bad input --
    {
        auto obs = MakeObs(sq, sig4, truth, 0.0);
        obs[2].sigmaS = 0.0;
        const bool zeroSig = CaTblSolveTdoa(obs, 0.0, 0.0).verdict == CA_TBL_TDOA_BAD_INPUT;
        obs = MakeObs(sq, sig4, truth, 0.0);
        obs[0].tArrS = std::numeric_limits<double>::quiet_NaN();
        const bool nanStamp = CaTblSolveTdoa(obs, 0.0, 0.0).verdict == CA_TBL_TDOA_BAD_INPUT;
        obs = MakeObs(sq, sig4, truth, 0.0);
        const bool nanSeed =
            CaTblSolveTdoa(obs, std::numeric_limits<double>::quiet_NaN(), 0.0).verdict ==
            CA_TBL_TDOA_BAD_INPUT;
        Check(zeroSig && nanStamp && nanSeed,
              "NEGATIVE: a zero sigma, a NaN stamp or a NaN seed is BAD_INPUT, never a fix");
    }

    // ------------------------------------------------------------- covariance --
    {
        const auto obs = MakeObs(sq, sig4, truth, 0.0);
        const CaTblTdoaResult r = CaTblSolveTdoa(obs, 0.0, 0.0);
        double exx;
        double exy;
        double eyy;
        ReferenceCov(sq, sig4, 1, truth, true, exx, exy, eyy);
        Check(Near(r.rxx, exx, 1e-4) && Near(r.ryy, eyy, 1e-4) &&
                  std::fabs(r.rxy - exy) <= 1e-4 * std::sqrt(exx * eyy),
              "covariance = (J' Q^-1 J)^-1 with the full Q, rebuilt by Gauss-Jordan");

        double dxx;
        double dxy;
        double dyy;
        ReferenceCov(sq, sig4, 1, truth, false, dxx, dxy, dyy);
        Check(!Near(dxx + dyy, exx + eyy, 0.05),
              "NEGATIVE: dropping the reference's shared error moves the trace by > 5%, "
              "so the check above can see that omission");
    }

    // --------------------------------- Monte Carlo: is the covariance honest? --
    {
        // Five anchors, 30 ns each (9 m of range difference). If the solver's weighting
        // or its covariance were wrong, the spread of its answers would not match what
        // it claims, and the weighted residual would not average M - 3 = 2.
        const std::vector<Pt> five = {{0, 0}, {2000, 0}, {0, 2000}, {2000, 2000}, {1000, -600}};
        const std::vector<double> s5 = {30e-9, 30e-9, 30e-9, 30e-9, 30e-9};
        std::mt19937_64 g(20260929ULL);
        const int N = 3000;
        int ok = 0;
        double mx = 0.0;
        double my = 0.0;
        double sxx = 0.0;
        double syy = 0.0;
        double sxy = 0.0;
        double chiSum = 0.0;
        double claimedTr = 0.0;
        for (int i = 0; i < N; ++i)
        {
            auto obs = MakeObs(five, s5, truth, 0.0);
            for (auto& o : obs)
            {
                o.tArrS += o.sigmaS * Gauss(g);
            }
            const CaTblTdoaResult r = CaTblSolveTdoa(obs, truth.x + 50.0, truth.y - 50.0);
            if (r.verdict != CA_TBL_TDOA_OK)
            {
                continue;
            }
            ++ok;
            const double ex = r.x - truth.x;
            const double ey = r.y - truth.y;
            mx += ex;
            my += ey;
            sxx += ex * ex;
            syy += ey * ey;
            sxy += ex * ey;
            chiSum += r.chi2;
            claimedTr += r.rxx + r.ryy;
        }
        const double n = static_cast<double>(ok);
        const double empTr = (sxx + syy) / n;
        const double ratio = empTr / (claimedTr / n);
        const double chiMean = chiSum / n;
        std::cout << "        MC: " << ok << "/" << N << " OK, empirical/claimed trace "
                  << ratio << ", mean chi2 " << chiMean << ", bias (" << mx / n << ", "
                  << my / n << ") m\n";
        Check(ok == N, "every noisy five-anchor broadcast solves");
        Check(ratio > 0.9 && ratio < 1.1,
              "the claimed covariance matches the spread of 3000 solves within 10%");
        Check(chiMean > 1.85 && chiMean < 2.15,
              "the weighted residual averages M - 3 = 2: the correlated weighting is right");
    }

    // ----------------------------------------------------- the filter update --
    {
        CaTblEkf e;
        e.Init(0.0, 0.0, 0.0, 0.0, 100.0, 1.0);
        const bool took = e.FusePositionCov(10.0, 0.0, 100.0, 0.0, 100.0);
        Check(took && std::fabs(e.X() - 5.0) < 1e-12 && std::fabs(e.Y()) < 1e-12 &&
                  std::fabs(e.Pxx() - 50.0) < 1e-9 && std::fabs(e.Pyy() - 50.0) < 1e-9,
              "equal P and R: gain 1/2 on each axis, variance halved (scalar Kalman)");

        // An elongated, rotated R against the information form, computed here.
        CaTblEkf f;
        f.Init(0.0, 0.0, 0.0, 0.0, 100.0, 1.0);
        const double rxx = 50.0;
        const double rxy = 30.0;
        const double ryy = 80.0;
        f.FusePositionCov(10.0, -20.0, rxx, rxy, ryy);
        const double rd = rxx * ryy - rxy * rxy;
        const double i00 = 1.0 / 100.0 + ryy / rd;
        const double i01 = -rxy / rd;
        const double i11 = 1.0 / 100.0 + rxx / rd;
        const double idet = i00 * i11 - i01 * i01;
        const double pxx = i11 / idet;
        const double pxy = -i01 / idet;
        const double pyy = i00 / idet;
        // x = Ppost R^-1 z, since the prior mean is 0.
        const double wx = (ryy * 10.0 - rxy * -20.0) / rd;
        const double wy = (-rxy * 10.0 + rxx * -20.0) / rd;
        const double xx = pxx * wx + pxy * wy;
        const double yy = pxy * wx + pyy * wy;
        Check(std::fabs(f.Pxx() - pxx) < 1e-9 && std::fabs(f.Pxy() - pxy) < 1e-9 &&
                  std::fabs(f.Pyy() - pyy) < 1e-9 && std::fabs(f.X() - xx) < 1e-9 &&
                  std::fabs(f.Y() - yy) < 1e-9,
              "a rotated R: posterior equals the information form (P^-1 + R^-1)^-1");

        CaTblEkf h;
        h.Init(3.0, 4.0, 0.0, 0.0, 100.0, 1.0);
        const bool nonPd = h.FusePositionCov(50.0, 50.0, 1.0, 2.0, 1.0);
        const bool nan =
            h.FusePositionCov(std::numeric_limits<double>::quiet_NaN(), 0.0, 1.0, 0.0, 1.0);
        Check(!nonPd && !nan && h.X() == 3.0 && h.Y() == 4.0 && h.Pxx() == 100.0,
              "NEGATIVE: an R that is not positive definite, or a NaN, is refused and "
              "leaves the filter untouched");
    }

    // ------------------------------------------------------- policy hand-off --
    {
        const std::vector<Pt> a3 = {{0, 0}, {1000, 0}, {0, 1000}};
        const Pt p = {500, 500};
        const auto good = Ranges(a3, p, 20.0);
        CaTblPolicyConfig cfg;

        CaTblPolicy plain;
        plain.Configure(cfg);
        plain.Initialize(p.x, p.y, 100.0, 5.0);
        CaTblPolicy aided;
        aided.Configure(cfg);
        aided.Initialize(p.x, p.y, 100.0, 5.0);

        const CaTblDecision d0 = plain.Step(1.0, good, 0, 3, 3, 1u);
        aided.SetAuxPositionFix(p.x, p.y, 25.0, 0.0, 25.0);
        const CaTblDecision d1 = aided.Step(1.0, good, 0, 3, 3, 1u);
        Check(!plain.AuxFusedLast() && plain.GetAuxFusedCount() == 0,
              "nothing queued: nothing fused, the count stays 0");
        Check(aided.AuxFusedLast() && aided.GetAuxFusedCount() == 1 &&
                  d1.traceSigma < d0.traceSigma,
              "queued: fused in the same Step, and the decision's trace(P) already contains it");

        aided.SetAuxPositionFix(p.x, p.y, 25.0, 0.0, 25.0);
        std::vector<CaTblRangeMeasurement> two(good.begin(), good.begin() + 2);
        aided.Step(1.0, two, 0, 2, 3, 1u);
        const bool noFix = !aided.AuxFusedLast() && aided.GetAuxFusedCount() == 1;
        aided.Step(1.0, good, 0, 3, 3, 1u);
        Check(noFix && !aided.AuxFusedLast() && aided.GetAuxFusedCount() == 1,
              "NEGATIVE: a no-fix cycle fuses nothing, and the fix does not outlive its cycle");

        // A cycle the Switching-Point Integrity Check rejects: every link LOS, the
        // ranges placing the node 3.5 km from its prior.
        const auto far = Ranges(a3, {3000, 3000}, 20.0);
        CaTblPolicy rej;
        rej.Configure(cfg);
        rej.Initialize(p.x, p.y, 100.0, 5.0);
        rej.SetAuxPositionFix(3000.0, 3000.0, 25.0, 0.0, 25.0);
        const CaTblDecision dr = rej.Step(1.0, far, 0, 3, 3, 1u);
        Check(dr.suspect && dr.measurementRejected && !rej.AuxFusedLast() &&
                  rej.GetAuxFusedCount() == 0,
              "NEGATIVE: a cycle the integrity check rejects fuses neither half of the fix");

        CaTblPolicyConfig bounded = cfg;
        bounded.suspectRecovery = CA_TBL_RECOVERY_BOUNDED;
        bounded.suspectRecoveryN = 1;
        CaTblPolicy force;
        force.Configure(bounded);
        force.Initialize(p.x, p.y, 100.0, 5.0);
        force.SetAuxPositionFix(3000.0, 3000.0, 25.0, 0.0, 25.0);
        const CaTblDecision df = force.Step(1.0, far, 0, 3, 3, 1u);
        Check(df.suspect && force.AuxFusedLast() && force.GetAuxFusedCount() == 1,
              "positive control: the same cycle force-accepted (BOUNDED, N=1) fuses both");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
