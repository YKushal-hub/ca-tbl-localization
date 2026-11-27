// DD 59 --ekf5WindowThetaWalk: the sliding-window smoother with heading as a state per cycle. In its own file so that the
// smoother it varies (ca-tbl-imu-ekf.cc) compiles to the same arithmetic as before: the switch off is byte-identical.
#include "ca-tbl-imu-ekf.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace ns3
{

namespace
{
// In-place Cholesky of a symmetric positive-definite n x n matrix (row-major, lower triangle used).
bool
CholDense(std::vector<double>& A, int n)
{
    for (int j = 0; j < n; ++j)
    {
        double d = A[j * n + j];
        for (int k = 0; k < j; ++k)
        {
            d -= A[j * n + k] * A[j * n + k];
        }
        if (!(d > 0.0) || !std::isfinite(d))
        {
            return false;
        }
        const double l = std::sqrt(d);
        A[j * n + j] = l;
        for (int i = j + 1; i < n; ++i)
        {
            double v = A[i * n + j];
            for (int k = 0; k < j; ++k)
            {
                v -= A[i * n + k] * A[j * n + k];
            }
            A[i * n + j] = v / l;
        }
    }
    return true;
}

void
CholSolve(const std::vector<double>& L, int n, std::vector<double>& b)
{
    for (int i = 0; i < n; ++i)
    {
        double v = b[i];
        for (int k = 0; k < i; ++k)
        {
            v -= L[i * n + k] * b[k];
        }
        b[i] = v / L[i * n + i];
    }
    for (int i = n - 1; i >= 0; --i)
    {
        double v = b[i];
        for (int k = i + 1; k < n; ++k)
        {
            v -= L[k * n + i] * b[k];
        }
        b[i] = v / L[i * n + i];
    }
}
} // namespace

void
CaTblImuEkf::WinMarginaliseFrontWalk()
{
    // DD 59: as WinMarginaliseFront, but heading is a state per cycle with the angle random walk between cycles. Local
    // variables: p0, theta0 (eliminated), p1, theta1, b, s, then the finder offsets.
    const WinCycle& c0 = m_win[0];
    const WinCycle& c1 = m_win[1];
    std::vector<int> fl;
    auto fpos = [&fl](int f) {
        auto it = std::find(fl.begin(), fl.end(), f);
        if (it != fl.end())
        {
            return static_cast<int>(it - fl.begin());
        }
        fl.push_back(f);
        return static_cast<int>(fl.size()) - 1;
    };
    for (const auto& r : c0.ranges)
    {
        if (r.finder >= 0)
        {
            fpos(r.finder);
        }
    }
    if (m_winPriorOn)
    {
        for (int f : m_winPriorF)
        {
            fpos(f);
        }
    }
    const int nl = 8 + 2 * static_cast<int>(fl.size());
    std::vector<double> v(nl, 0.0);
    v[0] = c0.x;
    v[1] = c0.y;
    v[2] = c0.th;
    v[3] = c1.x;
    v[4] = c1.y;
    v[5] = c1.th;
    v[6] = m_winB;
    v[7] = m_winS;
    for (size_t i = 0; i < fl.size(); ++i)
    {
        auto it = m_winOff.find(fl[i]);
        if (it != m_winOff.end())
        {
            v[8 + 2 * i] = it->second[0];
            v[9 + 2 * i] = it->second[1];
        }
    }
    std::vector<double> H(static_cast<size_t>(nl) * nl, 0.0), g(nl, 0.0);
    auto add = [&](double res, const int* id, const double* jac, int m, double var) {
        for (int a = 0; a < m; ++a)
        {
            g[id[a]] += jac[a] * res / var;
            for (int b = 0; b < m; ++b)
            {
                H[static_cast<size_t>(id[a]) * nl + id[b]] += jac[a] * jac[b] / var;
            }
        }
    };
    for (const auto& r : c0.ranges)
    {
        const int cf = r.finder >= 0 ? 8 + 2 * fpos(r.finder) : -1;
        const double dx = v[0] - r.ax - (cf >= 0 ? v[cf] : 0.0);
        const double dy = v[1] - r.ay - (cf >= 0 ? v[cf + 1] : 0.0);
        const double rho = std::max(std::hypot(dx, dy), 1e-6);
        const int id[4] = {0, 1, cf, cf + 1};
        const double jac[4] = {dx / rho, dy / rho, -dx / rho, -dy / rho};
        add(rho - r.r, id, jac, cf >= 0 ? 4 : 2, r.var);
    }
    if (c0.hasMag && c0.magR > 0.0)
    {
        const double nu = v[2] - c0.magZ;
        const int id[1] = {2};
        const double jac[1] = {1.0};
        add(std::atan2(std::sin(nu), std::cos(nu)), id, jac, 1, c0.magR);
    }
    for (const auto& f : c0.fixes)
    {
        const double l00 = std::sqrt(f[2]);
        const double l10 = f[3] / l00;
        const double l11 = std::sqrt(std::max(f[4] - l10 * l10, 1e-12));
        const double w0 = (v[0] - f[0]) / l00, w1 = (v[1] - f[1] - l10 * w0) / l11;
        const int id[2] = {0, 1};
        const double j0[2] = {1.0 / l00, 0.0};
        const double j1[2] = {-l10 / (l00 * l11), 1.0 / l11};
        add(w0, id, j0, 2, 1.0);
        add(w1, id, j1, 2, 1.0);
    }
    {
        const int id[3] = {5, 2, 6};
        const double jac[3] = {1.0, -1.0, -c1.dt};
        add(v[5] - v[2] - v[6] * c1.dt, id, jac, 3, std::max(c1.qTheta2, 1e-12));
    }
    if (c1.dwell)
    {
        for (int ax = 0; ax < 2; ++ax)
        {
            const int id[2] = {3 + ax, ax};
            const double jac[2] = {1.0, -1.0};
            add(v[3 + ax] - v[ax], id, jac, 2, std::max(c1.dwellVar, 1e-4));
        }
    }
    else
    {
        const double phi = c1.raw - v[5];
        const double ops = std::max(1.0 + v[7], 0.1);
        const double D = c1.dist / ops;
        const double dDds = -c1.dist / (ops * ops);
        const double pred[2] = {D * std::cos(phi), D * std::sin(phi)};
        const double dth[2] = {D * std::sin(phi), -D * std::cos(phi)};
        const double ds[2] = {dDds * std::cos(phi), dDds * std::sin(phi)};
        for (int ax = 0; ax < 2; ++ax)
        {
            const int id[4] = {3 + ax, ax, 5, 7};
            const double jac[4] = {1.0, -1.0, -dth[ax], -ds[ax]};
            add(v[3 + ax] - v[ax] - pred[ax], id, jac, 4, c1.qStep2 + 0.01);
        }
    }
    if (m_winPriorOn)
    {
        const int kp = static_cast<int>(m_winPriorMu.size());
        std::vector<int> map = {0, 1, 2, 6, 7};
        for (int f : m_winPriorF)
        {
            map.push_back(8 + 2 * fpos(f));
            map.push_back(9 + 2 * fpos(f));
        }
        for (int i = 0; i < kp; ++i)
        {
            double Ld = 0.0;
            for (int j = 0; j < kp; ++j)
            {
                const double l = m_winPriorL[static_cast<size_t>(i) * kp + j];
                Ld += l * (v[map[j]] - m_winPriorMu[j]);
                H[static_cast<size_t>(map[i]) * nl + map[j]] += l;
            }
            g[map[i]] += Ld + m_winPriorBeta[i];
        }
    }
    // Eliminate p0x, p0y, theta0 one at a time (Schur complement).
    for (int e = 0; e < 3; ++e)
    {
        const double d = H[static_cast<size_t>(e) * nl + e];
        if (!(d > 0.0) || !std::isfinite(d))
        {
            m_winPriorOn = false;
            return;
        }
        for (int i = e + 1; i < nl; ++i)
        {
            const double f = H[static_cast<size_t>(i) * nl + e] / d;
            g[i] -= f * g[e];
            for (int j = e + 1; j < nl; ++j)
            {
                H[static_cast<size_t>(i) * nl + j] -= f * H[static_cast<size_t>(e) * nl + j];
            }
        }
    }
    const int k = nl - 3;
    m_winPriorMu.assign(v.begin() + 3, v.end());
    m_winPriorBeta.assign(g.begin() + 3, g.end());
    m_winPriorL.assign(static_cast<size_t>(k) * k, 0.0);
    for (int i = 0; i < k; ++i)
    {
        for (int j = 0; j < k; ++j)
        {
            m_winPriorL[static_cast<size_t>(i) * k + j] = 0.5 * (H[static_cast<size_t>(i + 3) * nl + j + 3] +
                                                              H[static_cast<size_t>(j + 3) * nl + i + 3]);
        }
    }
    m_winPriorF = fl;
    m_winPriorOn = true;
}

void
CaTblImuEkf::WinSolveWalk()
{
    const int W = static_cast<int>(m_win.size());
    if (W < std::max(2, m_winMin))
    {
        return;
    }
    // Unknowns: p_0..p_{W-1} (2W), theta at the window's first cycle, b, s, then 2 per finder under consider.
    std::map<int, int> fidx;
    for (const auto& c : m_win)
    {
        for (const auto& r : c.ranges)
        {
            if (r.finder >= 0 && !fidx.count(r.finder))
            {
                const int k = static_cast<int>(fidx.size());
                fidx[r.finder] = k;
            }
        }
    }
    if (m_winPriorOn)
    {
        for (int f : m_winPriorF)
        {
            if (!fidx.count(f))
            {
                const int k = static_cast<int>(fidx.size());
                fidx[f] = k;
            }
        }
    }
    const int F = static_cast<int>(fidx.size());
    const int iT = 2 * W, iB = iT + 1, iS = iT + 2, iC = iT + 3;
    const int nw = iC + 2 * F; // DD 59 appends theta_1..theta_{W-1}; iT is theta_0
    const int n = nw + (true ? W - 1 : 0);
    auto thIdx = [&](int k) { return true && k > 0 ? nw + k - 1 : iT; };
    std::vector<double> tau(W, 0.0);
    for (int k = 1; k < W; ++k)
    {
        tau[k] = tau[k - 1] + m_win[k].dt;
    }
    std::vector<double> x(n, 0.0);
    for (int k = 0; k < W; ++k)
    {
        x[2 * k] = m_win[k].x;
        x[2 * k + 1] = m_win[k].y;
    }
    // Sensor states from the filter as it stands (theta at the newest cycle, rolled back to the window's first).
    x[iB] = m_x[3];
    x[iS] = m_x[4];
    x[iT] = m_x[2] - m_x[3] * tau[W - 1];
    if (m_winHaveSol)
    {
        x[iB] = m_winB;
        x[iS] = m_winS;
        x[iT] = m_winTheta0;
        for (const auto& kv : fidx)
        {
            auto it = m_winOff.find(kv.first);
            if (it != m_winOff.end())
            {
                x[iC + 2 * kv.second] = it->second[0];
                x[iC + 2 * kv.second + 1] = it->second[1];
            }
        }
    }
    if (true) // DD 59
    {
        for (int k = 0; k < W; ++k)
        {
            x[thIdx(k)] = m_win[k].th;
        }
    }
    // The marginalisation prior's variables, as indices into x.
    std::vector<int> pmap;
    if (m_winPriorOn)
    {
        pmap = {0, 1, iT, iB, iS};
        for (int f : m_winPriorF)
        {
            pmap.push_back(iC + 2 * fidx[f]);
            pmap.push_back(iC + 2 * fidx[f] + 1);
        }
    }

    // Accumulates whitened residuals r and their gradients into H = J'J, g = J'r and the cost.
    std::vector<double> H(static_cast<size_t>(n) * n), g(n);
    double cost = 0.0;
    int nres = 0;
    auto add = [&](double res, const int* id, const double* jac, int m, double var) {
        const double w = 1.0 / var;
        cost += res * res * w;
        ++nres;
        for (int a = 0; a < m; ++a)
        {
            g[id[a]] += jac[a] * res * w;
            for (int b = 0; b < m; ++b)
            {
                H[static_cast<size_t>(id[a]) * n + id[b]] += jac[a] * jac[b] * w;
            }
        }
    };
    auto build = [&](const std::vector<double>& v) {
        std::fill(H.begin(), H.end(), 0.0);
        std::fill(g.begin(), g.end(), 0.0);
        cost = 0.0;
        nres = 0;
        const double th0 = v[iT], b = v[iB], s = v[iS];
        for (int k = 0; k < W; ++k)
        {
            const WinCycle& c = m_win[k];
            const double thk = true ? v[thIdx(k)] : th0 + b * tau[k];
            if (true && k > 0) // DD 59: theta_k = theta_{k-1} + b dt + angle random walk
            {
                const int id[3] = {thIdx(k), thIdx(k - 1), iB};
                const double jac[3] = {1.0, -1.0, -c.dt};
                add(thk - v[thIdx(k - 1)] - b * c.dt, id, jac, 3, std::max(c.qTheta2, 1e-12));
            }
            if (k > 0)
            {
                if (c.dwell)
                {
                    const double var = std::max(c.dwellVar, 1e-4);
                    for (int ax = 0; ax < 2; ++ax)
                    {
                        const int id[2] = {2 * k + ax, 2 * (k - 1) + ax};
                        const double jac[2] = {1.0, -1.0};
                        add(v[id[0]] - v[id[1]], id, jac, 2, var);
                    }
                }
                else
                {
                    const double phi = c.raw - thk;
                    const double ops = std::max(1.0 + s, 0.1);
                    const double D = c.dist / ops;
                    const double dDds = -c.dist / (ops * ops);
                    // Stride noise along track, heading noise across it, and a 0.1 m floor (OURS).
                    const double var = c.qStep2 + (true ? 0.0 : D * D * c.qTheta2) + 0.01;
                    const double pred[2] = {D * std::cos(phi), D * std::sin(phi)};
                    const double dth[2] = {D * std::sin(phi), -D * std::cos(phi)}; // d pred / d theta_k
                    const double ds[2] = {dDds * std::cos(phi), dDds * std::sin(phi)};
                    for (int ax = 0; ax < 2; ++ax)
                    {
                        const int id[5] = {2 * k + ax, 2 * (k - 1) + ax, thIdx(k), iB, iS};
                        const double jac[5] = {1.0, -1.0, -dth[ax], true ? 0.0 : -dth[ax] * tau[k], -ds[ax]};
                        add(v[id[0]] - v[id[1]] - pred[ax], id, jac, 5, var);
                    }
                }
            }
            for (const auto& r : c.ranges)
            {
                double qx = r.ax, qy = r.ay;
                int cf = -1;
                if (r.finder >= 0)
                {
                    cf = iC + 2 * fidx[r.finder];
                    qx += v[cf];
                    qy += v[cf + 1];
                }
                const double dx = v[2 * k] - qx, dy = v[2 * k + 1] - qy;
                const double rho = std::max(std::hypot(dx, dy), 1e-6);
                const double ux = dx / rho, uy = dy / rho;
                if (cf >= 0)
                {
                    const int id[4] = {2 * k, 2 * k + 1, cf, cf + 1};
                    const double jac[4] = {ux, uy, -ux, -uy};
                    add(rho - r.r, id, jac, 4, r.var);
                }
                else
                {
                    const int id[2] = {2 * k, 2 * k + 1};
                    const double jac[2] = {ux, uy};
                    add(rho - r.r, id, jac, 2, r.var);
                }
            }
            if (c.hasMag && c.magR > 0.0)
            {
                double nu = thk - c.magZ;
                nu = std::atan2(std::sin(nu), std::cos(nu));
                const int id[2] = {thIdx(k), iB};
                const double jac[2] = {1.0, true ? 0.0 : tau[k]};
                add(nu, id, jac, 2, c.magR);
            }
            for (const auto& f : c.fixes)
            {
                // A 2-D fix with covariance R: whiten by R's Cholesky factor (lower, l00 l10 l11).
                const double l00 = std::sqrt(f[2]);
                const double l10 = f[3] / l00;
                const double l11 = std::sqrt(std::max(f[4] - l10 * l10, 1e-12));
                const double ex = v[2 * k] - f[0], ey = v[2 * k + 1] - f[1];
                const double w0 = ex / l00, w1 = (ey - l10 * w0) / l11;
                const int id[2] = {2 * k, 2 * k + 1};
                const double j0[2] = {1.0 / l00, 0.0};
                const double j1[2] = {-l10 / (l00 * l11), 1.0 / l11};
                add(w0, id, j0, 2, 1.0);
                add(w1, id, j1, 2, 1.0);
            }
        }
        if (m_winPriorOn)
        {
            const int kp = static_cast<int>(pmap.size());
            std::vector<double> dd(kp);
            for (int i = 0; i < kp; ++i)
            {
                dd[i] = v[pmap[i]] - m_winPriorMu[i];
            }
            for (int i = 0; i < kp; ++i)
            {
                double Ld = 0.0;
                for (int j = 0; j < kp; ++j)
                {
                    const double l = m_winPriorL[static_cast<size_t>(i) * kp + j];
                    Ld += l * dd[j];
                    H[static_cast<size_t>(pmap[i]) * n + pmap[j]] += l;
                }
                g[pmap[i]] += Ld + m_winPriorBeta[i];
                cost += dd[i] * Ld + 2.0 * m_winPriorBeta[i] * dd[i];
            }
        }
        if (m_winBiasSigma > 0.0)
        {
            const int id[1] = {iB};
            const double jac[1] = {1.0};
            add(b, id, jac, 1, m_winBiasSigma * m_winBiasSigma);
        }
        if (m_winScaleSigma > 0.0)
        {
            const int id[1] = {iS};
            const double jac[1] = {1.0};
            add(s, id, jac, 1, m_winScaleSigma * m_winScaleSigma);
        }
        for (int f = 0; f < F; ++f)
        {
            for (int ax = 0; ax < 2; ++ax)
            {
                const int id[1] = {iC + 2 * f + ax};
                const double jac[1] = {1.0};
                add(v[id[0]], id, jac, 1, m_consVar > 0.0 ? m_consVar : 4.0);
            }
        }
    };

    // Levenberg-Marquardt on the dense normal equations. Window of 60 cycles: about 130 unknowns.
    double lambda = 1e-3;
    build(x);
    double c0 = cost;
    bool ok = std::isfinite(c0);
    for (int it = 0; ok && it < 20; ++it)
    {
        std::vector<double> A = H;
        for (int i = 0; i < n; ++i)
        {
            A[static_cast<size_t>(i) * n + i] += lambda * std::max(A[static_cast<size_t>(i) * n + i], 1e-9);
        }
        std::vector<double> dx(g);
        if (!CholDense(A, n))
        {
            lambda *= 10.0;
            continue;
        }
        CholSolve(A, n, dx);
        std::vector<double> xn(x);
        double maxStep = 0.0;
        for (int i = 0; i < n; ++i)
        {
            xn[i] -= dx[i];
            maxStep = std::max(maxStep, std::fabs(dx[i]));
        }
        std::vector<double> Hs = H, gs = g;
        build(xn);
        if (std::isfinite(cost) && cost < c0)
        {
            x.swap(xn);
            c0 = cost;
            lambda = std::max(lambda / 10.0, 1e-9);
            if (maxStep < 1e-3)
            {
                break;
            }
        }
        else
        {
            H.swap(Hs);
            g.swap(gs);
            lambda *= 10.0;
            if (lambda > 1e8)
            {
                break;
            }
        }
    }
    build(x); // H at the solution, undamped: its inverse is the window's covariance
    std::vector<double> L = H;
    if (!ok || !CholDense(L, n))
    {
        ++m_winFails;
        return;
    }
    // Marginal covariance of (p_last, theta0, b, s): columns of H^-1 for those five unknowns.
    const int sel[5] = {2 * (W - 1), 2 * (W - 1) + 1, thIdx(W - 1), iB, iS};
    double S[5][5];
    for (int a = 0; a < 5; ++a)
    {
        std::vector<double> e(n, 0.0);
        e[sel[a]] = 1.0;
        CholSolve(L, n, e);
        for (int b2 = 0; b2 < 5; ++b2)
        {
            S[b2][a] = e[sel[b2]];
        }
    }
    // State = A (p_last, theta0, b, s): theta_last = theta0 + b T.
    const double T = tau[W - 1];
    double A[5][5] = {};
    A[0][0] = A[1][1] = A[3][3] = A[4][4] = 1.0;
    A[2][2] = 1.0;
    A[2][3] = true ? 0.0 : T; // DD 59: theta_last is a state
    double AS[5][5] = {};
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            for (int k = 0; k < 5; ++k)
            {
                AS[i][j] += A[i][k] * S[k][j];
            }
        }
    }
    double P[5][5] = {};
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            for (int k = 0; k < 5; ++k)
            {
                P[i][j] += AS[i][k] * A[j][k];
            }
        }
    }
    const double xs[5] = {x[2 * (W - 1)], x[2 * (W - 1) + 1], true ? x[thIdx(W - 1)] : x[iT] + x[iB] * T, x[iB], x[iS]};
    for (int i = 0; i < 5; ++i)
    {
        if (!std::isfinite(xs[i]) || !(P[i][i] > 0.0) || !std::isfinite(P[i][i]))
        {
            ++m_winFails;
            return;
        }
    }
    for (int i = 0; i < N; ++i)
    {
        m_x[i] = xs[i];
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = P[i][j];
        }
    }
    Symmetrise();
    // The finder offsets now live in the window; the filter's consider cross-covariances restart from zero.
    for (auto& kv : m_cons)
    {
        for (int i = 0; i < N; ++i)
        {
            kv.second.X[i][0] = kv.second.X[i][1] = 0.0;
        }
    }
    for (int k = 0; k < W; ++k)
    {
        m_win[k].x = x[2 * k];
        m_win[k].y = x[2 * k + 1];
        if (true)
        {
            m_win[k].th = x[thIdx(k)];
        }
        m_win[k].solved = true;
    }
    m_winHaveSol = true;
    m_winTheta0 = x[iT];
    m_winB = x[iB];
    m_winS = x[iS];
    for (const auto& kv : fidx)
    {
        m_winOff[kv.first] = {x[iC + 2 * kv.second], x[iC + 2 * kv.second + 1]};
    }
    ++m_winSolves;
    m_winLastRms = nres > 0 ? std::sqrt(c0 / nres) : 0.0;
}

} // namespace ns3
