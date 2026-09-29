/*
 * CA-TBL Physical sect 1.1 -- tier-1 TDoA solver. See ca-tbl-tdoa.h for the design.
 */

#include "ca-tbl-tdoa.h"

#include <algorithm>
#include <cmath>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ns3
{

namespace
{

// Speed of light in vacuum, m/s -- exact by the SI definition. Propagation in air is
// slower by ~3 parts in 10^4, i.e. 0.3 m per km of path, a common-mode term that mostly
// cancels in a difference and is far below tier-1 timing noise. Not modelled.
constexpr double kC = 299792458.0;

/**
 * W = Q^-1 for Q = diag(s_k^2) + s_ref^2 * 1 1', all in metres.
 *
 * Every range difference d_k = r_k - r_ref carries the reference anchor's error, so the
 * differences are correlated through it. Sherman-Morrison inverts the diagonal-plus-rank-1
 * form exactly without a general matrix inverse:
 *   W = D^-1 - (D^-1 1)(D^-1 1)' / (1/s_ref^2 + 1' D^-1 1).
 */
void
BuildWeight(const std::vector<double>& sig, double sigRef, std::vector<double>& W)
{
    const size_t n = sig.size();
    std::vector<double> w(n, 0.0);
    double sumW = 0.0;
    for (size_t k = 0; k < n; ++k)
    {
        w[k] = 1.0 / (sig[k] * sig[k]);
        sumW += w[k];
    }
    const double denom = 1.0 / (sigRef * sigRef) + sumW;
    W.assign(n * n, 0.0);
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            W[i * n + j] = ((i == j) ? w[i] : 0.0) - w[i] * w[j] / denom;
        }
    }
}

/**
 * 3x3 inverse by cofactors. Refuses when |det| is negligible against the Hadamard bound
 * (the product of the row norms), which is scale-free: a singular matrix of large entries
 * and a well-conditioned matrix of small ones are told apart by shape, not magnitude.
 */
bool
Inv3(const double A[3][3], double Ai[3][3])
{
    const double c00 = A[1][1] * A[2][2] - A[1][2] * A[2][1];
    const double c01 = A[1][2] * A[2][0] - A[1][0] * A[2][2];
    const double c02 = A[1][0] * A[2][1] - A[1][1] * A[2][0];
    const double det = A[0][0] * c00 + A[0][1] * c01 + A[0][2] * c02;
    double bound = 1.0;
    for (int i = 0; i < 3; ++i)
    {
        bound *= std::sqrt(A[i][0] * A[i][0] + A[i][1] * A[i][1] + A[i][2] * A[i][2]);
    }
    if (!std::isfinite(det) || !(bound > 0.0) || !(std::fabs(det) > 1e-13 * bound))
    {
        return false;
    }
    Ai[0][0] = c00 / det;
    Ai[1][0] = c01 / det;
    Ai[2][0] = c02 / det;
    Ai[0][1] = (A[0][2] * A[2][1] - A[0][1] * A[2][2]) / det;
    Ai[1][1] = (A[0][0] * A[2][2] - A[0][2] * A[2][0]) / det;
    Ai[2][1] = (A[0][1] * A[2][0] - A[0][0] * A[2][1]) / det;
    Ai[0][2] = (A[0][1] * A[1][2] - A[0][2] * A[1][1]) / det;
    Ai[1][2] = (A[0][2] * A[1][0] - A[0][0] * A[1][2]) / det;
    Ai[2][2] = (A[0][0] * A[1][1] - A[0][1] * A[1][0]) / det;
    return true;
}

//! Range differences and their weighting for one broadcast, reference-relative.
struct Differences
{
    std::vector<double> ax;  //!< anchor positions, absolute, non-reference anchors
    std::vector<double> ay;
    std::vector<double> d;   //!< c * (t_k - t_ref), m
    std::vector<double> W;   //!< Q^-1, n x n, m^-2
    double refX = 0.0;
    double refY = 0.0;
};

void
BuildDifferences(const std::vector<CaTblTdoaObs>& obs, size_t ref, Differences& D)
{
    std::vector<double> sig;
    for (size_t k = 0; k < obs.size(); ++k)
    {
        if (k == ref)
        {
            continue;
        }
        D.ax.push_back(obs[k].x);
        D.ay.push_back(obs[k].y);
        // Subtract the stamps BEFORE scaling: both sit at simulation time, and the
        // difference is what carries the information.
        D.d.push_back(kC * (obs[k].tArrS - obs[ref].tArrS));
        sig.push_back(kC * obs[k].sigmaS);
    }
    D.refX = obs[ref].x;
    D.refY = obs[ref].y;
    BuildWeight(sig, kC * obs[ref].sigmaS, D.W);
}

/**
 * Weighted residual cost at (x, y); fills the residual and, if asked, the Jacobian
 * (n x 2, row-major) of the MODEL h_k = |p - a_k| - |p - a_ref|.
 */
double
Evaluate(const Differences& D, double x, double y, std::vector<double>& r, std::vector<double>* J)
{
    const size_t n = D.d.size();
    r.assign(n, 0.0);
    if (J != nullptr)
    {
        J->assign(2 * n, 0.0);
    }
    const double ex = x - D.refX;
    const double ey = y - D.refY;
    const double rr = std::sqrt(ex * ex + ey * ey);
    // A point standing ON an anchor has no defined bearing to it; its gradient term is
    // taken as zero rather than as a division by zero.
    const double urx = (rr > 1e-9) ? ex / rr : 0.0;
    const double ury = (rr > 1e-9) ? ey / rr : 0.0;
    for (size_t k = 0; k < n; ++k)
    {
        const double kx = x - D.ax[k];
        const double ky = y - D.ay[k];
        const double rk = std::sqrt(kx * kx + ky * ky);
        r[k] = D.d[k] - (rk - rr);
        if (J != nullptr)
        {
            (*J)[2 * k] = ((rk > 1e-9) ? kx / rk : 0.0) - urx;
            (*J)[2 * k + 1] = ((rk > 1e-9) ? ky / rk : 0.0) - ury;
        }
    }
    double cost = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        double wi = 0.0;
        for (size_t j = 0; j < n; ++j)
        {
            wi += D.W[i * n + j] * r[j];
        }
        cost += r[i] * wi;
    }
    return cost;
}

//! J' W J (2x2, symmetric: a b; b c) and J' W r.
void
NormalEquations(const Differences& D,
                const std::vector<double>& J,
                const std::vector<double>& r,
                double& a,
                double& b,
                double& c,
                double& g0,
                double& g1)
{
    const size_t n = D.d.size();
    a = b = c = g0 = g1 = 0.0;
    for (size_t i = 0; i < n; ++i)
    {
        for (size_t j = 0; j < n; ++j)
        {
            const double w = D.W[i * n + j];
            a += J[2 * i] * w * J[2 * j];
            b += J[2 * i] * w * J[2 * j + 1];
            c += J[2 * i + 1] * w * J[2 * j + 1];
            g0 += J[2 * i] * w * r[j];
            g1 += J[2 * i + 1] * w * r[j];
        }
    }
}

/**
 * Foy's Taylor-series least squares from (x, y). Returns true on convergence and leaves
 * the solution in (x, y). Step halving keeps each accepted step a descent step: plain
 * Gauss-Newton on hyperbolae overshoots across an asymptote from a poor seed.
 */
bool
Foy(const Differences& D, const CaTblTdoaConfig& cfg, double& x, double& y, uint32_t& iterations)
{
    std::vector<double> r;
    std::vector<double> J;
    std::vector<double> rTry;
    iterations = 0;
    for (uint32_t it = 0; it < cfg.maxIterations; ++it)
    {
        ++iterations;
        const double cost0 = Evaluate(D, x, y, r, &J);
        if (!std::isfinite(cost0))
        {
            return false;
        }
        double a;
        double b;
        double c;
        double g0;
        double g1;
        NormalEquations(D, J, r, a, b, c, g0, g1);
        const double det = a * c - b * b;
        if (!(det > 0.0) || !std::isfinite(det))
        {
            return false;
        }
        const double sx = (c * g0 - b * g1) / det;
        const double sy = (a * g1 - b * g0) / det;
        double lambda = 1.0;
        bool descended = false;
        for (int h = 0; h < 30; ++h)
        {
            const double cTry = Evaluate(D, x + lambda * sx, y + lambda * sy, rTry, nullptr);
            if (std::isfinite(cTry) && cTry <= cost0)
            {
                descended = true;
                break;
            }
            lambda *= 0.5;
        }
        if (!descended)
        {
            // No step along the Gauss-Newton direction lowers the cost: this IS the
            // minimum to working precision.
            return true;
        }
        x += lambda * sx;
        y += lambda * sy;
        if (!std::isfinite(x) || !std::isfinite(y))
        {
            return false;
        }
        if (std::sqrt(sx * sx + sy * sy) * lambda < cfg.convergedStepM)
        {
            return true;
        }
    }
    return false;
}

} // namespace

const char*
CaTblTdoaVerdictName(CaTblTdoaVerdict v)
{
    switch (v)
    {
    case CA_TBL_TDOA_OK:
        return "OK";
    case CA_TBL_TDOA_TOO_FEW:
        return "TOO_FEW";
    case CA_TBL_TDOA_COLLINEAR:
        return "COLLINEAR";
    case CA_TBL_TDOA_SINGULAR:
        return "SINGULAR";
    case CA_TBL_TDOA_NO_CONVERGE:
        return "NO_CONVERGE";
    case CA_TBL_TDOA_BAD_INPUT:
        return "BAD_INPUT";
    case CA_TBL_TDOA_NO_ROOT:
        return "NO_ROOT";
    case CA_TBL_TDOA_AMBIGUOUS:
        return "AMBIGUOUS";
    case CA_TBL_TDOA_DIVERGED:
        return "DIVERGED";
    }
    return "UNKNOWN";
}

const char*
CaTblTdoaSeedName(CaTblTdoaSeed s)
{
    switch (s)
    {
    case CA_TBL_TDOA_SEED_NONE:
        return "none";
    case CA_TBL_TDOA_SEED_CHAN:
        return "chan";
    case CA_TBL_TDOA_SEED_CALLER:
        return "rssi";
    case CA_TBL_TDOA_SEED_ROOT:
        return "root";
    }
    return "unknown";
}

double
CaTblTdoaArrivalSigmaS(double bandwidthHz, uint8_t spreadingFactor, double snrDbInBand)
{
    if (!(bandwidthHz > 0.0) || !std::isfinite(snrDbInBand))
    {
        return std::numeric_limits<double>::infinity();
    }
    const double dechirpGainDb =
        10.0 * std::log10(std::pow(2.0, static_cast<double>(spreadingFactor)));
    const double ebn0 = std::pow(10.0, (snrDbInBand + dechirpGainDb) / 10.0);
    if (!(ebn0 > 0.0) || !std::isfinite(ebn0))
    {
        return std::numeric_limits<double>::infinity();
    }
    const double betaRms = bandwidthHz / std::sqrt(12.0);
    return 1.0 / (2.0 * M_PI * betaRms * std::sqrt(2.0 * ebn0));
}

bool
CaTblTdoaChan(const std::vector<CaTblTdoaObs>& obs, size_t ref, double& outX, double& outY)
{
    if (obs.size() < 4 || ref >= obs.size())
    {
        return false;
    }
    Differences D;
    BuildDifferences(obs, ref, D);
    const size_t n = D.d.size();

    // Work relative to the reference anchor, so K_ref = 0 and every coordinate below is
    // a baseline: z = (x, y, R_ref), row k:  -(x_k, y_k, d_k) . z = (d_k^2 - K_k) / 2.
    std::vector<double> gx(n);
    std::vector<double> gy(n);
    std::vector<double> h(n);
    for (size_t k = 0; k < n; ++k)
    {
        gx[k] = D.ax[k] - D.refX;
        gy[k] = D.ay[k] - D.refY;
        h[k] = 0.5 * (D.d[k] * D.d[k] - (gx[k] * gx[k] + gy[k] * gy[k]));
    }

    // Stage 1, twice. Psi = B Q B, B = diag(true distance to anchor k): unknown on the
    // first pass (B = I, the far-field form), taken from pass 1 on the second.
    std::vector<double> dist(n, 1.0);
    double z[3] = {0.0, 0.0, 0.0};
    double A[3][3];
    double Ai[3][3];
    for (int pass = 0; pass < 2; ++pass)
    {
        double b[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                A[i][j] = 0.0;
            }
        }
        for (size_t i = 0; i < n; ++i)
        {
            const double Gi[3] = {-gx[i], -gy[i], -D.d[i]};
            for (size_t j = 0; j < n; ++j)
            {
                const double p = D.W[i * n + j] / (dist[i] * dist[j]);
                const double Gj[3] = {-gx[j], -gy[j], -D.d[j]};
                for (int u = 0; u < 3; ++u)
                {
                    for (int v = 0; v < 3; ++v)
                    {
                        A[u][v] += Gi[u] * p * Gj[v];
                    }
                    b[u] += Gi[u] * p * h[j];
                }
            }
        }
        if (!Inv3(A, Ai))
        {
            return false;
        }
        for (int u = 0; u < 3; ++u)
        {
            z[u] = Ai[u][0] * b[0] + Ai[u][1] * b[1] + Ai[u][2] * b[2];
        }
        if (!std::isfinite(z[0]) || !std::isfinite(z[1]) || !std::isfinite(z[2]))
        {
            return false;
        }
        for (size_t k = 0; k < n; ++k)
        {
            const double ex = z[0] - gx[k];
            const double ey = z[1] - gy[k];
            dist[k] = std::max(1e-6, std::sqrt(ex * ex + ey * ey));
        }
    }

    // Stage 2: the three stage-1 unknowns are not independent -- R_ref^2 = x^2 + y^2 in
    // reference-relative coordinates. Estimate (x^2, y^2) from h' = (z0^2, z1^2, z2^2)
    // with G' = [1 0; 0 1; 1 1] and Psi' = 4 B' cov(z) B', B' = diag(z). cov(z) is A^-1
    // from the second pass, so Psi'^-1 = B'^-1 A B'^-1 / 4 needs no further inverse.
    outX = D.refX + z[0];
    outY = D.refY + z[1];
    const double tiny = 1e-9;
    if (std::fabs(z[0]) < tiny || std::fabs(z[1]) < tiny || std::fabs(z[2]) < tiny)
    {
        return true; // B' singular: stage 1 stands
    }
    double P[3][3];
    for (int u = 0; u < 3; ++u)
    {
        for (int v = 0; v < 3; ++v)
        {
            P[u][v] = 0.25 * A[u][v] / (z[u] * z[v]);
        }
    }
    const double hp[3] = {z[0] * z[0], z[1] * z[1], z[2] * z[2]};
    const double Gp[3][2] = {{1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}};
    double a2[2][2] = {{0.0, 0.0}, {0.0, 0.0}};
    double b2[2] = {0.0, 0.0};
    for (int u = 0; u < 3; ++u)
    {
        for (int v = 0; v < 3; ++v)
        {
            for (int s = 0; s < 2; ++s)
            {
                for (int q = 0; q < 2; ++q)
                {
                    a2[s][q] += Gp[u][s] * P[u][v] * Gp[v][q];
                }
                b2[s] += Gp[u][s] * P[u][v] * hp[v];
            }
        }
    }
    const double det2 = a2[0][0] * a2[1][1] - a2[0][1] * a2[1][0];
    if (!(std::fabs(det2) > 0.0) || !std::isfinite(det2))
    {
        return true;
    }
    const double zx2 = (a2[1][1] * b2[0] - a2[0][1] * b2[1]) / det2;
    const double zy2 = (a2[0][0] * b2[1] - a2[1][0] * b2[0]) / det2;
    if (!(zx2 >= 0.0) || !(zy2 >= 0.0))
    {
        return true; // a negative square: the constraint cannot be imposed; stage 1 stands
    }
    outX = D.refX + ((z[0] < 0.0) ? -std::sqrt(zx2) : std::sqrt(zx2));
    outY = D.refY + ((z[1] < 0.0) ? -std::sqrt(zy2) : std::sqrt(zy2));
    return true;
}

int
CaTblTdoaThreeAnchorRoots(const std::vector<CaTblTdoaObs>& obs,
                          size_t ref,
                          double outX[2],
                          double outY[2])
{
    if (obs.size() != 3 || ref >= 3)
    {
        return 0;
    }
    Differences D;
    BuildDifferences(obs, ref, D);

    // Reference at the origin: g_k = a_k - a_ref, R = |p|. From |p - g_k| = R + d_k,
    //   g_k . p = (|g_k|^2 - d_k^2) / 2 - d_k R,   i.e.  G p = e - d R,
    // so p = a + b R with a = G^-1 e and b = -G^-1 d, and |p| = R is a quadratic in R.
    const double g0x = D.ax[0] - D.refX;
    const double g0y = D.ay[0] - D.refY;
    const double g1x = D.ax[1] - D.refX;
    const double g1y = D.ay[1] - D.refY;
    const double det = g0x * g1y - g0y * g1x;
    const double scale = (g0x * g0x + g0y * g0y) * (g1x * g1x + g1y * g1y);
    if (!(det * det > 1e-20 * scale))
    {
        return -1; // parallel baselines: the anchors are collinear
    }
    const double e0 = 0.5 * (g0x * g0x + g0y * g0y - D.d[0] * D.d[0]);
    const double e1 = 0.5 * (g1x * g1x + g1y * g1y - D.d[1] * D.d[1]);
    const double ax = (g1y * e0 - g0y * e1) / det;
    const double ay = (-g1x * e0 + g0x * e1) / det;
    const double bx = -(g1y * D.d[0] - g0y * D.d[1]) / det;
    const double by = -(-g1x * D.d[0] + g0x * D.d[1]) / det;

    const double qa = bx * bx + by * by - 1.0;
    const double qb = 2.0 * (ax * bx + ay * by);
    const double qc = ax * ax + ay * ay;
    double cand[2];
    int nc = 0;
    if (std::fabs(qa) < 1e-12)
    {
        // |b| = 1: the quadratic degenerates to a line with one root.
        if (std::fabs(qb) > 0.0)
        {
            cand[nc++] = -qc / qb;
        }
    }
    else
    {
        const double disc = qb * qb - 4.0 * qa * qc;
        if (disc >= 0.0)
        {
            const double sq = std::sqrt(disc);
            // The numerically stable pair: never subtract two nearly equal numbers.
            const double qq = -0.5 * (qb + ((qb < 0.0) ? -sq : sq));
            cand[nc++] = qq / qa;
            if (qq != 0.0)
            {
                cand[nc++] = qc / qq;
            }
        }
    }

    // A root is real only if every range it implies is non-negative: squaring
    // |p - g_k| = R + d_k admitted the solutions of |p - g_k| = -(R + d_k) too.
    const double tol = 1e-6;
    int n = 0;
    for (int i = 0; i < nc; ++i)
    {
        const double R = cand[i];
        if (!std::isfinite(R) || R < -tol || R + D.d[0] < -tol || R + D.d[1] < -tol)
        {
            continue;
        }
        const double px = ax + bx * R;
        const double py = ay + by * R;
        if (n == 1 && std::hypot(px - (outX[0] - D.refX), py - (outY[0] - D.refY)) < 1e-6)
        {
            continue; // a double root, counted once
        }
        outX[n] = D.refX + px;
        outY[n] = D.refY + py;
        ++n;
    }
    return n;
}

CaTblTdoaResult
CaTblSolveTdoa(const std::vector<CaTblTdoaObs>& obs,
               double seedX,
               double seedY,
               const CaTblTdoaConfig& cfg,
               const std::vector<CaTblTdoaRangeHint>& hints)
{
    CaTblTdoaResult res;
    res.nAnchors = static_cast<uint32_t>(obs.size());

    for (size_t k = 0; k < obs.size(); ++k)
    {
        if (!std::isfinite(obs[k].x) || !std::isfinite(obs[k].y) ||
            !std::isfinite(obs[k].tArrS) || !std::isfinite(obs[k].sigmaS) ||
            !(obs[k].sigmaS > 0.0))
        {
            res.verdict = CA_TBL_TDOA_BAD_INPUT;
            return res;
        }
    }
    if (!std::isfinite(seedX) || !std::isfinite(seedY))
    {
        res.verdict = CA_TBL_TDOA_BAD_INPUT;
        return res;
    }
    if (obs.size() < 3)
    {
        res.verdict = CA_TBL_TDOA_TOO_FEW;
        return res;
    }

    // The reference is the best-timed anchor: its error enters EVERY difference, so it
    // should be the smallest one available.
    size_t ref = 0;
    for (size_t k = 1; k < obs.size(); ++k)
    {
        if (obs[k].sigmaS < obs[ref].sigmaS)
        {
            ref = k;
        }
    }
    res.refAnchorId = obs[ref].anchorId;

    // Geometry first. Collinear anchors leave a mirror ambiguity about their line that no
    // number of timestamps resolves, and coincident anchors measure no difference at all.
    double mx = 0.0;
    double my = 0.0;
    double extent = 0.0;
    {
        for (size_t k = 0; k < obs.size(); ++k)
        {
            mx += obs[k].x;
            my += obs[k].y;
        }
        mx /= static_cast<double>(obs.size());
        my /= static_cast<double>(obs.size());
        for (size_t k = 0; k < obs.size(); ++k)
        {
            extent = std::max(extent, std::hypot(obs[k].x - mx, obs[k].y - my));
        }
        double sxx = 0.0;
        double sxy = 0.0;
        double syy = 0.0;
        for (size_t k = 0; k < obs.size(); ++k)
        {
            const double ux = obs[k].x - mx;
            const double uy = obs[k].y - my;
            sxx += ux * ux;
            sxy += ux * uy;
            syy += uy * uy;
        }
        const double tr = sxx + syy;
        const double det = sxx * syy - sxy * sxy;
        const double disc = std::sqrt(std::max(0.0, 0.25 * tr * tr - det));
        const double l1 = 0.5 * tr + disc;
        const double l2 = std::max(0.0, 0.5 * tr - disc);
        if (!(l1 > 0.0) || std::sqrt(l2 / l1) < cfg.minSpreadRatio)
        {
            res.verdict = CA_TBL_TDOA_COLLINEAR;
            return res;
        }
    }

    Differences D;
    BuildDifferences(obs, ref, D);

    // An iterate this far from the anchors has run out along an asymptote: the
    // cost flattens there, the steps stop lowering it, and Foy() reports the stall
    // as convergence. Measured before this guard: two "OK" fixes 1e17 m away.
    const double farM = cfg.maxExtentFactor * extent;
    auto diverged = [&](double px, double py) {
        return !(std::hypot(px - mx, py - my) <= farM);
    };

    double x = seedX;
    double y = seedY;
    uint32_t its = 0;
    bool ok = false;

    if (obs.size() == 3)
    {
        // Square system: find EVERY exact root, and let evidence -- not the seed's
        // basin -- choose between two. See the header, step 2.
        double rx[2];
        double ry[2];
        const int nr = CaTblTdoaThreeAnchorRoots(obs, ref, rx, ry);
        if (nr < 0)
        {
            res.verdict = CA_TBL_TDOA_COLLINEAR;
            return res;
        }
        res.nRoots = static_cast<uint32_t>(nr);
        if (nr == 0)
        {
            res.verdict = CA_TBL_TDOA_NO_ROOT;
            return res;
        }
        int pick = 0;
        if (nr == 2)
        {
            double c[2] = {0.0, 0.0};
            uint32_t usable = 0;
            for (size_t h = 0; h < hints.size(); ++h)
            {
                const CaTblTdoaRangeHint& hh = hints[h];
                if (!(hh.sigmaM > 0.0) || !std::isfinite(hh.sigmaM) ||
                    !std::isfinite(hh.rangeM) || !std::isfinite(hh.x) || !std::isfinite(hh.y))
                {
                    continue;
                }
                ++usable;
                for (int i = 0; i < 2; ++i)
                {
                    const double e = (std::hypot(rx[i] - hh.x, ry[i] - hh.y) - hh.rangeM) /
                                     hh.sigmaM;
                    c[i] += e * e;
                }
            }
            if (usable > 0)
            {
                pick = (c[0] <= c[1]) ? 0 : 1;
                res.tieChi2Chosen = c[pick];
                res.tieChi2Other = c[1 - pick];
                if (!(std::fabs(c[0] - c[1]) >= cfg.minTieChi2Gap))
                {
                    res.x = rx[pick];
                    res.y = ry[pick];
                    res.verdict = CA_TBL_TDOA_AMBIGUOUS;
                    return res;
                }
            }
            else
            {
                const double d0 = std::hypot(rx[0] - seedX, ry[0] - seedY);
                const double d1 = std::hypot(rx[1] - seedX, ry[1] - seedY);
                pick = (d0 <= d1) ? 0 : 1;
                if (!(std::max(d0, d1) >= cfg.minTieSeedRatio * std::min(d0, d1)))
                {
                    res.x = rx[pick];
                    res.y = ry[pick];
                    res.verdict = CA_TBL_TDOA_AMBIGUOUS;
                    return res;
                }
            }
        }
        x = rx[pick];
        y = ry[pick];
        res.seededBy = CA_TBL_TDOA_SEED_ROOT;
        // An exact root is already the answer; this only polishes rounding, and
        // leaves the iteration count as evidence that it did nothing.
        ok = Foy(D, cfg, x, y, its);
        res.iterations = its;
    }
    else
    {
        res.seededBy = CA_TBL_TDOA_SEED_CALLER;
        double cx = 0.0;
        double cy = 0.0;
        if (CaTblTdoaChan(obs, ref, cx, cy))
        {
            res.chanOk = true;
            res.chanX = cx;
            res.chanY = cy;
            x = cx;
            y = cy;
            res.seededBy = CA_TBL_TDOA_SEED_CHAN;
        }
        ok = Foy(D, cfg, x, y, its);
        res.iterations = its;
        if ((!ok || diverged(x, y)) && res.seededBy == CA_TBL_TDOA_SEED_CHAN)
        {
            // A closed form fitted to noisy differences can land outside the basin the
            // iteration needs. The caller's seed is independent evidence; try it once.
            // Its answer replaces Chan's only if it is better: converged and in reach,
            // or converged at all when Chan's attempt did not converge.
            double x2 = seedX;
            double y2 = seedY;
            uint32_t its2 = 0;
            const bool ok2 = Foy(D, cfg, x2, y2, its2);
            res.iterations += its2;
            if ((ok2 && !diverged(x2, y2)) || !ok)
            {
                x = x2;
                y = y2;
                ok = ok2;
                res.seededBy = CA_TBL_TDOA_SEED_CALLER;
            }
        }
    }
    res.x = x;
    res.y = y;
    if (!ok)
    {
        res.verdict = CA_TBL_TDOA_NO_CONVERGE;
        return res;
    }
    if (diverged(x, y))
    {
        res.verdict = CA_TBL_TDOA_DIVERGED;
        return res;
    }

    // Covariance at the solution: (J' Q^-1 J)^-1.
    std::vector<double> r;
    std::vector<double> J;
    res.chi2 = Evaluate(D, x, y, r, &J);
    double a;
    double b;
    double c;
    double g0;
    double g1;
    NormalEquations(D, J, r, a, b, c, g0, g1);
    const double det = a * c - b * b;
    const double tr = a + c;
    const double disc = std::sqrt(std::max(0.0, 0.25 * tr * tr - det));
    const double lMax = 0.5 * tr + disc;
    const double lMin = 0.5 * tr - disc;
    if (!(det > 0.0) || !std::isfinite(det) || !(lMin > 0.0) || lMax / lMin > cfg.maxCondition)
    {
        res.verdict = CA_TBL_TDOA_SINGULAR;
        return res;
    }
    res.rxx = c / det;
    res.rxy = -b / det;
    res.ryy = a / det;
    res.verdict = CA_TBL_TDOA_OK;
    return res;
}

} // namespace ns3
