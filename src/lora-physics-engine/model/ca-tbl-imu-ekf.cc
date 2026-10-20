#include "ca-tbl-imu-ekf.h"

#include "ca-tbl-policy.h" // CaTblRangeMeasurement

#include <algorithm>
#include <cmath>

namespace ns3
{

// ---------------------------------------------------------------------------
// C3: IMU-driven EKF, state [x, y, theta, b, s]
// ---------------------------------------------------------------------------

CaTblImuEkf::CaTblImuEkf()
{
    Init(0.0, 0.0, 1e4, 0.0, 0.0, 0.0);
}

void
CaTblImuEkf::Init(double x,
                  double y,
                  double posVar,
                  double thetaVar,
                  double biasVar,
                  double strideVar)
{
    for (int i = 0; i < N; ++i)
    {
        m_x[i] = 0.0;
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = 0.0;
        }
    }
    m_x[0] = x;
    m_x[1] = y;
    m_P[0][0] = posVar;
    m_P[1][1] = posVar;
    m_P[2][2] = thetaVar;
    m_P[3][3] = biasVar;
    m_P[4][4] = strideVar;
    WinClear(); // --ekf5Window
    SnapshotPrior();
}

void
CaTblImuEkf::SnapshotPrior()
{
    if (m_consOn)
    {
        m_consPrior = m_cons;
    }
    for (int i = 0; i < N; ++i)
    {
        m_xPrior[i] = m_x[i];
        for (int j = 0; j < N; ++j)
        {
            m_PPrior[i][j] = m_P[i][j];
        }
    }
}

void
CaTblImuEkf::Symmetrise()
{
    for (int i = 0; i < N; ++i)
    {
        for (int j = i + 1; j < N; ++j)
        {
            const double avg = 0.5 * (m_P[i][j] + m_P[j][i]);
            m_P[i][j] = avg;
            m_P[j][i] = avg;
        }
    }
}

void
CaTblImuEkf::Predict(double dt,
                     double distM,
                     double rawHeadingRad,
                     double qStep2,
                     double qTheta2,
                     double qBias2)
{
    if (m_winW > 0)
    {
        WinOpen(dt, distM, rawHeadingRad, qStep2, qTheta2, false, 0.0); // --ekf5Window
    }
    const double theta = m_x[2] + m_x[3] * dt;
    const double phi = rawHeadingRad - theta;
    const double c = std::cos(phi);
    const double s = std::sin(phi);
    const double onePlusS = 1.0 + m_x[4];
    // Guard only against a pathological estimate; s is a few percent in practice.
    const double denom = (onePlusS > 0.1) ? onePlusS : 0.1;
    const double D = distM / denom;
    const double dDds = -distM / (denom * denom);

    m_x[0] += D * c;
    m_x[1] += D * s;
    m_x[2] = theta;
    m_vx = (dt > 0.0) ? D * c / dt : 0.0; // Vx()/Vy(): velocity is not a state here
    m_vy = (dt > 0.0) ? D * s / dt : 0.0;
    m_pathX += D * c;
    m_pathY += D * s;
    // m_x[3] (bias) and m_x[4] (stride scale) are constant under the prediction.

    // F = d(x', y', theta', b', s') / d(x, y, theta, b, s). phi depends on theta and
    // on b through theta' = theta + b dt, hence the dt in column 3.
    double F[N][N] = {};
    for (int i = 0; i < N; ++i)
    {
        F[i][i] = 1.0;
    }
    F[0][2] = D * s;
    F[0][3] = D * s * dt;
    F[0][4] = dDds * c;
    F[1][2] = -D * c;
    F[1][3] = -D * c * dt;
    F[1][4] = dDds * s;
    F[2][3] = dt;

    double FP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            double acc = 0.0;
            for (int k = 0; k < N; ++k)
            {
                acc += F[i][k] * m_P[k][j];
            }
            FP[i][j] = acc;
        }
    }
    ConsAfterTransition(F); // --ekf5FinderConsider; a no-op unless on
    double newP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            double acc = 0.0;
            for (int k = 0; k < N; ++k)
            {
                acc += FP[i][k] * F[j][k];
            }
            newP[i][j] = acc;
        }
    }

    // Q = qTheta2 g g^T + qStep2 u u^T + qBias2 e_b e_b^T. Heading noise added to
    // theta' moves the position through phi: d x'/d theta' = D sin(phi), so its
    // shaping vector is g = [D s, -D c, 1, 0, 0], not the theta axis alone. Adding it
    // to theta only would leave the position block unaware of heading noise until
    // the NEXT cycle -- a one-cycle lag in the very covariance the integrity check
    // reads.
    const double g[N] = {D * s, -D * c, 1.0, 0.0, 0.0};
    const double u[N] = {c, s, 0.0, 0.0, 0.0};
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            newP[i][j] += qTheta2 * g[i] * g[j] + qStep2 * u[i] * u[j];
        }
    }
    newP[3][3] += qBias2;

    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = newP[i][j];
        }
    }
    Symmetrise();
    if (m_mm)
    {
        MomentMatch();
    }
    if (m_winW > 0 && !m_win.empty())
    {
        m_win.back().x = m_x[0];
        m_win.back().y = m_x[1];
    }
    SnapshotPrior();
}

void
CaTblImuEkf::ResetCoast()
{
    m_pathX = m_pathY = 0.0;
    m_shiftX = m_shiftY = 0.0;
    m_varApplied = 0.0;
}

void
CaTblImuEkf::MomentMatch()
{
    const double L = std::hypot(m_pathX, m_pathY);
    if (L < 1e-9)
    {
        return;
    }
    const double ux = m_pathX / L;
    const double uy = m_pathY / L;
    const double s2 = std::max(0.0, m_P[2][2]);
    // Target totals for the whole path since the last fix, at the current heading variance.
    const double shrink = L * (1.0 - std::exp(-0.5 * s2));
    const double tx = -shrink * ux;
    const double ty = -shrink * uy;
    const double varAlong =
        L * L * std::max(0.0, 0.5 * (1.0 + std::exp(-2.0 * s2)) - std::exp(-s2));
    // Apply only the increments; earlier cycles' corrections already travel in x and P.
    if (m_mmShift)
    {
        m_x[0] += tx - m_shiftX;
        m_x[1] += ty - m_shiftY;
        m_shiftX = tx;
        m_shiftY = ty;
    }
    const double dv = varAlong - m_varApplied;
    if (dv > 0.0)
    {
        m_P[0][0] += dv * ux * ux;
        m_P[0][1] += dv * ux * uy;
        m_P[1][0] += dv * ux * uy;
        m_P[1][1] += dv * uy * uy;
        m_varApplied = varAlong;
    }
}

void
CaTblImuEkf::ProcessNoise(const CaTblImuSensorParams& p,
                          double dt,
                          double distM,
                          double turnRad,
                          double& qStep2,
                          double& qTheta2,
                          double& qBias2)
{
    const double deg = M_PI / 180.0;
    const double n = p.gyroNoiseDpsRtHz * deg;
    qTheta2 = n * n * dt + (p.gyroScaleErr * turnRad) * (p.gyroScaleErr * turnRad);
    qStep2 = p.strideStepErr * p.strideStepErr * p.strideLengthM * distM;
    qBias2 = 0.0;
    if (p.gyroBiasTauS > 0.0)
    {
        const double g = p.gyroBiasGmDps * deg;
        qBias2 = g * g * (1.0 - std::exp(-2.0 * dt / p.gyroBiasTauS));
    }
}

namespace
{
// Lower-triangular Cholesky factor of a symmetric positive semi-definite 5x5 matrix;
// tiny negative pivots from round-off are clamped to zero.
void
Cholesky5(const double A[5][5], double L[5][5])
{
    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            L[i][j] = 0.0;
        }
    }
    for (int j = 0; j < 5; ++j)
    {
        double d = A[j][j];
        for (int k = 0; k < j; ++k)
        {
            d -= L[j][k] * L[j][k];
        }
        L[j][j] = d > 0.0 ? std::sqrt(d) : 0.0;
        for (int i = j + 1; i < 5; ++i)
        {
            double v = A[i][j];
            for (int k = 0; k < j; ++k)
            {
                v -= L[i][k] * L[j][k];
            }
            L[i][j] = L[j][j] > 0.0 ? v / L[j][j] : 0.0;
        }
    }
}
} // namespace

void
CaTblImuEkf::PredictUnscented(double dt,
                              double distM,
                              double rawHeadingRad,
                              double qStep2,
                              double qTheta2,
                              double qBias2)
{
    if (m_winW > 0)
    {
        WinOpen(dt, distM, rawHeadingRad, qStep2, qTheta2, false, 0.0); // --ekf5Window
    }
    double scaled[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            scaled[i][j] = N * m_P[i][j];
        }
    }
    double L[N][N];
    Cholesky5(scaled, L);

    auto f = [&](const double in[N], double out[N]) {
        const double theta = in[2] + in[3] * dt;
        const double phi = rawHeadingRad - theta;
        const double onePlusS = 1.0 + in[4];
        const double D = distM / (onePlusS > 0.1 ? onePlusS : 0.1);
        out[0] = in[0] + D * std::cos(phi);
        out[1] = in[1] + D * std::sin(phi);
        out[2] = theta;
        out[3] = in[3];
        out[4] = in[4];
    };

    double pts[2 * N][N];
    for (int k = 0; k < N; ++k)
    {
        double plus[N], minus[N];
        for (int i = 0; i < N; ++i)
        {
            plus[i] = m_x[i] + L[i][k];
            minus[i] = m_x[i] - L[i][k];
        }
        f(plus, pts[2 * k]);
        f(minus, pts[2 * k + 1]);
    }
    double mean[N] = {};
    for (int p = 0; p < 2 * N; ++p)
    {
        for (int i = 0; i < N; ++i)
        {
            mean[i] += pts[p][i] / (2.0 * N);
        }
    }
    double newP[N][N] = {};
    for (int p = 0; p < 2 * N; ++p)
    {
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                newP[i][j] += (pts[p][i] - mean[i]) * (pts[p][j] - mean[j]) / (2.0 * N);
            }
        }
    }
    // Process noise, shaped as in Predict(), at the mean heading.
    const double phiM = rawHeadingRad - mean[2];
    const double c = std::cos(phiM);
    const double s = std::sin(phiM);
    const double onePlusS = 1.0 + mean[4];
    const double Dm = distM / (onePlusS > 0.1 ? onePlusS : 0.1);
    const double g[N] = {Dm * s, -Dm * c, 1.0, 0.0, 0.0};
    const double u[N] = {c, s, 0.0, 0.0, 0.0};
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            newP[i][j] += qTheta2 * g[i] * g[j] + qStep2 * u[i] * u[j];
        }
    }
    newP[3][3] += qBias2;
    for (int i = 0; i < N; ++i)
    {
        m_x[i] = mean[i];
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = newP[i][j];
        }
    }
    Symmetrise();
    SnapshotPrior();
}

void
CaTblImuEkf::FuseHeading(double z, double r)
{
    FuseHeadingBody(z, r);
    ApplyBounds();
}

void
CaTblImuEkf::FuseHeadingBody(double z, double r)
{
    if (!(r > 0.0) || !std::isfinite(z))
    {
        return;
    }
    const double S = m_P[2][2] + r;
    if (!(S > 0.0))
    {
        return;
    }
    if (m_winW > 0 && !m_win.empty())
    {
        m_win.back().hasMag = true; // --ekf5Window
        m_win.back().magZ = z;
        m_win.back().magR = r;
    }
    double nu = z - m_x[2];
    nu = std::atan2(std::sin(nu), std::cos(nu));
    double K[N];
    for (int i = 0; i < N; ++i)
    {
        K[i] = m_P[i][2] / S;
    }
    if (m_consOn)
    {
        const double H[N] = {0.0, 0.0, 1.0, 0.0, 0.0};
        ConsAfterGain(K, 1, H);
    }
    for (int i = 0; i < N; ++i)
    {
        m_x[i] += K[i] * nu;
    }
    double newP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            newP[i][j] = m_P[i][j] - K[i] * m_P[2][j];
        }
    }
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = newP[i][j];
        }
    }
    Symmetrise();
}

void
CaTblImuEkf::ReinitPosition(double x, double y, double posVar)
{
    WinClear(); // --ekf5Window: the path before a reset is not this position's
    for (auto& kv : m_cons)
    {
        kv.second.X[0][0] = kv.second.X[0][1] = kv.second.X[1][0] = kv.second.X[1][1] = 0.0;
    }
    m_x[0] = x;
    m_x[1] = y;
    for (int j = 0; j < N; ++j)
    {
        m_P[0][j] = 0.0;
        m_P[j][0] = 0.0;
        m_P[1][j] = 0.0;
        m_P[j][1] = 0.0;
    }
    m_P[0][0] = posVar;
    m_P[1][1] = posVar;
    SnapshotPrior();
}

void
CaTblImuEkf::SetPositionCov(double x, double y, double pxx, double pxy, double pyy)
{
    WinClear(); // --ekf5Window: the path before a reset is not this position's
    for (auto& kv : m_cons)
    {
        kv.second.X[0][0] = kv.second.X[0][1] = kv.second.X[1][0] = kv.second.X[1][1] = 0.0;
    }
    m_x[0] = x;
    m_x[1] = y;
    for (int j = 0; j < N; ++j)
    {
        m_P[0][j] = 0.0;
        m_P[j][0] = 0.0;
        m_P[1][j] = 0.0;
        m_P[j][1] = 0.0;
    }
    m_P[0][0] = pxx;
    m_P[0][1] = pxy;
    m_P[1][0] = pxy;
    m_P[1][1] = pyy;
    SnapshotPrior();
}

void
CaTblImuEkf::Update(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    if (m_iterMax > 0 && m_robust == 0 && !meas.empty() && !varM2.empty())
    {
        UpdateIterated(meas, varM2); // --ekf5Iterate; off by default
    }
    else
    {
        UpdateBody(meas, varM2);
    }
    ApplyBounds(); // --ekf5StateBounds; a no-op unless on
    if (m_winW > 0 && !m_win.empty() && !meas.empty())
    {
        if (!m_win.back().solved)
        {
            m_win.back().x = m_x[0];
            m_win.back().y = m_x[1];
        }
        WinSolve(); // --ekf5Window
    }
}

void
CaTblImuEkf::UpdateIterated(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    // IEKF, Gauss-Newton form: x_{i+1} = x0 + K_i (z - h(x_i) - H_i (x0 - x_i)), K_i from H_i at x_i, and the
    // posterior covariance from the last pass's H. Each pass restarts from the prior (the whole object,
    // counters and finder cross-covariances included), so the state and every counter read as ONE update:
    // the last pass's.
    const CaTblImuEkf start = *this;
    double lx = m_x[0];
    double ly = m_x[1];
    int passes = 0;
    bool converged = false;
    while (passes < m_iterMax)
    {
        if (passes > 0)
        {
            *this = start;
        }
        m_linOn = true;
        m_linX = lx;
        m_linY = ly;
        UpdateBody(meas, varM2);
        m_linOn = false;
        ++passes;
        const double step = std::hypot(m_x[0] - lx, m_x[1] - ly);
        lx = m_x[0];
        ly = m_x[1];
        if (!std::isfinite(step))
        {
            break; // counted as not converged
        }
        if (step < m_iterTol)
        {
            converged = true;
            break;
        }
    }
    m_iterUpdates = start.m_iterUpdates + 1;
    m_iterPasses = start.m_iterPasses + passes;
    m_iterUnconv = start.m_iterUnconv + (converged ? 0 : 1);
}

void
CaTblImuEkf::UpdateBody(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    if (m_robust != 0)
    {
        UpdateRobust(meas, varM2); // N.33; mode 0 below is the original, untouched
        return;
    }
    if (m_consOn)
    {
        UpdateConsider(meas, varM2); // --ekf5FinderConsider; off by default
        return;
    }
    if (!meas.empty())
    {
        ResetCoast(); // a fix re-anchors the path; the coherent correction starts again
    }
    // The same sequential scalar update as CaTblEkf::Update, over five states. A range
    // depends on position only, so H = [dx/r, dy/r, 0, 0, 0]; theta, b and s are
    // corrected through their covariance with position, which is how they are learned.
    const size_t n = std::min(meas.size(), varM2.size());
    for (size_t k = 0; k < n; ++k)
    {
        double dx, dy;
        LinDelta(meas[k].anchorX, meas[k].anchorY, dx, dy); // --ekf5Iterate's fixed point, else the state
        const double rLin = std::sqrt(dx * dx + dy * dy);
        const double rSafe = std::max(rLin, 1e-6);
        const double H0 = dx / rSafe;
        const double H1 = dy / rSafe;
        const double rPred = LinPred(rLin, H0, H1);

        double PH[N];
        for (int i = 0; i < N; ++i)
        {
            PH[i] = m_P[i][0] * H0 + m_P[i][1] * H1;
        }
        const double S = H0 * PH[0] + H1 * PH[1] + varM2[k];
        if (!(S > 0.0) || !std::isfinite(S))
        {
            continue;
        }
        double K[N];
        for (int i = 0; i < N; ++i)
        {
            K[i] = PH[i] / S;
        }
        const double innovation = meas[k].rangeM - rPred;
        if (m_relayGate && meas[k].sourceKind == CA_TBL_SRC_RELAY)
        {
            ++m_relaySeen;
            if (innovation * innovation / S > 10.83)
            {
                ++m_relayGated;
                continue;
            }
        }
        NoteNis(innovation * innovation / S); // --relayNisInflate; a no-op unless enabled
        if (m_winW > 0 && !m_win.empty())
        {
            m_win.back().ranges.push_back({meas[k].anchorX, meas[k].anchorY, meas[k].rangeM, varM2[k], -1}); // --ekf5Window
        }
        for (int i = 0; i < N; ++i)
        {
            m_x[i] += K[i] * innovation;
        }
        double newP[N][N];
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                newP[i][j] = m_P[i][j] - K[i] * (H0 * m_P[0][j] + H1 * m_P[1][j]);
            }
        }
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                m_P[i][j] = newP[i][j];
            }
        }
        Symmetrise();
    }
}

void
CaTblImuEkf::RevertToPrior()
{
    if (m_consOn)
    {
        m_cons = m_consPrior;
    }
    for (int i = 0; i < N; ++i)
    {
        m_x[i] = m_xPrior[i];
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = m_PPrior[i][j];
        }
    }
}

void
CaTblImuEkf::InflatePos(double factor)
{
    m_P[0][0] *= factor;
    m_P[1][1] *= factor;
}

double
CaTblImuEkf::MahalanobisFromPrior(double zx, double zy) const
{
    // Same statistic, same documented mis-specification, as CaTblEkf's: kept so the
    // two filters answer the integrity question identically under mahalUseR=false.
    const double dx = zx - m_xPrior[0];
    const double dy = zy - m_xPrior[1];
    const double a = m_PPrior[0][0];
    const double b = m_PPrior[0][1];
    const double c = m_PPrior[1][0];
    const double d = m_PPrior[1][1];
    const double det = a * d - b * c;
    if (std::fabs(det) < 1e-12)
    {
        return 0.0;
    }
    return dx * ((d / det) * dx + (-b / det) * dy) + dy * ((-c / det) * dx + (a / det) * dy);
}

double
CaTblImuEkf::MahalanobisFromPriorWithR(double zx, double zy, double rxx, double rxy, double ryy) const
{
    const double dx = zx - m_xPrior[0];
    const double dy = zy - m_xPrior[1];
    const double a = m_PPrior[0][0] + rxx;
    const double b = m_PPrior[0][1] + rxy;
    const double c = m_PPrior[1][0] + rxy;
    const double d = m_PPrior[1][1] + ryy;
    const double det = a * d - b * c;
    if (std::fabs(det) < 1e-12)
    {
        return 0.0;
    }
    return dx * ((d / det) * dx + (-b / det) * dy) + dy * ((-c / det) * dx + (a / det) * dy);
}

// ---------------------------------------------------------------------------
// Position observations, as CaTblEkf's, over five states
// ---------------------------------------------------------------------------

void
CaTblImuEkf::FuseAxis(int axis, double z, double varM2)
{
    double PH[N];
    for (int i = 0; i < N; ++i)
    {
        PH[i] = m_P[i][axis];
    }
    const double S = PH[axis] + varM2;
    if (!(S > 0.0) || !std::isfinite(S))
    {
        return;
    }
    double K[N];
    for (int i = 0; i < N; ++i)
    {
        K[i] = PH[i] / S;
    }
    const double innovation = z - m_x[axis];
    for (int i = 0; i < N; ++i)
    {
        m_x[i] += K[i] * innovation;
    }
    if (m_consOn)
    {
        double H[N] = {0.0, 0.0, 0.0, 0.0, 0.0};
        H[axis] = 1.0;
        ConsAfterGain(K, 1, H);
    }
    double newP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            newP[i][j] = m_P[i][j] - K[i] * m_P[axis][j];
        }
    }
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = newP[i][j];
        }
    }
    Symmetrise();
}

void
CaTblImuEkf::FusePosition(double zx, double zy, double varM2)
{
    FusePositionBody(zx, zy, varM2);
    ApplyBounds();
}

void
CaTblImuEkf::FusePositionBody(double zx, double zy, double varM2)
{
    if (m_robust != 0)
    {
        FusePositionCovRobust(zx, zy, varM2, 0.0, varM2); // N.33
        return;
    }
    if (!(varM2 > 0.0) || !std::isfinite(varM2) || !std::isfinite(zx) || !std::isfinite(zy))
    {
        return;
    }
    if (m_winW > 0 && !m_win.empty())
    {
        m_win.back().fixes.push_back({zx, zy, varM2, 0.0, varM2}); // --ekf5Window
    }
    ResetCoast();
    FuseAxis(0, zx, varM2);
    FuseAxis(1, zy, varM2);
}

double
CaTblImuEkf::PositionNis(double zx, double zy, double rxx, double rxy, double ryy) const
{
    if (!std::isfinite(zx) || !std::isfinite(zy) || !std::isfinite(rxx) || !std::isfinite(rxy) ||
        !std::isfinite(ryy) || !(rxx > 0.0) || !(ryy > 0.0) || !(rxx * ryy - rxy * rxy > 0.0))
    {
        return -1.0;
    }
    const double s00 = m_P[0][0] + rxx;
    const double s01 = 0.5 * (m_P[0][1] + m_P[1][0]) + rxy;
    const double s11 = m_P[1][1] + ryy;
    const double det = s00 * s11 - s01 * s01;
    if (!(det > 0.0) || !std::isfinite(det))
    {
        return -1.0;
    }
    const double v0 = zx - m_x[0];
    const double v1 = zy - m_x[1];
    return (s11 * v0 * v0 - 2.0 * s01 * v0 * v1 + s00 * v1 * v1) / det;
}

bool
CaTblImuEkf::FusePositionCov(double zx, double zy, double rxx, double rxy, double ryy)
{
    const bool ok = FusePositionCovBody(zx, zy, rxx, rxy, ryy);
    ApplyBounds();
    return ok;
}

bool
CaTblImuEkf::FusePositionCovBody(double zx, double zy, double rxx, double rxy, double ryy)
{
    if (m_robust != 0)
    {
        return FusePositionCovRobust(zx, zy, rxx, rxy, ryy); // N.33
    }
    if (!std::isfinite(zx) || !std::isfinite(zy) || !std::isfinite(rxx) || !std::isfinite(rxy) ||
        !std::isfinite(ryy) || !(rxx > 0.0) || !(ryy > 0.0) || !(rxx * ryy - rxy * rxy > 0.0))
    {
        return false;
    }
    if (m_winW > 0 && !m_win.empty())
    {
        m_win.back().fixes.push_back({zx, zy, rxx, rxy, ryy}); // --ekf5Window
    }
    const double pxy = 0.5 * (m_P[0][1] + m_P[1][0]);
    const double s00 = m_P[0][0] + rxx;
    const double s01 = pxy + rxy;
    const double s11 = m_P[1][1] + ryy;
    const double det = s00 * s11 - s01 * s01;
    if (!(det > 0.0) || !std::isfinite(det))
    {
        return false;
    }
    ResetCoast();
    const double i00 = s11 / det;
    const double i01 = -s01 / det;
    const double i11 = s00 / det;
    double K[N][2];
    for (int i = 0; i < N; ++i)
    {
        K[i][0] = m_P[i][0] * i00 + m_P[i][1] * i01;
        K[i][1] = m_P[i][0] * i01 + m_P[i][1] * i11;
    }
    const double v0 = zx - m_x[0];
    const double v1 = zy - m_x[1];
    for (int i = 0; i < N; ++i)
    {
        m_x[i] += K[i][0] * v0 + K[i][1] * v1;
    }
    if (m_consOn)
    {
        const double K0[N] = {K[0][0], K[1][0], K[2][0], K[3][0], K[4][0]};
        const double K1[N] = {K[0][1], K[1][1], K[2][1], K[3][1], K[4][1]};
        const double H0[N] = {1.0, 0.0, 0.0, 0.0, 0.0};
        const double H1[N] = {0.0, 1.0, 0.0, 0.0, 0.0};
        // A 2-D update with gain [K0 K1] and H = [e_x; e_y]: X_b -= K0 (e_x X_b) + K1 (e_y X_b), both from
        // the pre-update X_b -- applied as one step.
        for (auto& kv : m_cons)
        {
            double r0[2], r1[2];
            for (int c = 0; c < 2; ++c)
            {
                r0[c] = kv.second.X[0][c];
                r1[c] = kv.second.X[1][c];
            }
            for (int i = 0; i < N; ++i)
            {
                for (int c = 0; c < 2; ++c)
                {
                    kv.second.X[i][c] -= K0[i] * r0[c] + K1[i] * r1[c];
                }
            }
        }
        (void)H0;
        (void)H1;
    }
    double newP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            newP[i][j] = m_P[i][j] - K[i][0] * m_P[0][j] - K[i][1] * m_P[1][j];
        }
    }
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            m_P[i][j] = newP[i][j];
        }
    }
    Symmetrise();
    return true;
}

void
CaTblImuEkf::PredictDwell(double dt, double qTheta2, double qBias2, double posVar2)
{
    if (m_winW > 0)
    {
        WinOpen(dt, 0.0, 0.0, 0.0, qTheta2, true, posVar2); // --ekf5Window
        m_win.back().x = m_x[0];
        m_win.back().y = m_x[1];
    }
    // theta' = theta + b dt is the only state that moves: F = I except d theta' / d b = dt.
    // P' = F P F^T, written out for that single off-diagonal.
    m_x[2] += m_x[3] * dt;
    if (m_consOn)
    {
        double F[N][N] = {};
        for (int i = 0; i < N; ++i)
        {
            F[i][i] = 1.0;
        }
        F[2][3] = dt;
        ConsAfterTransition(F);
    }
    double P[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            P[i][j] = m_P[i][j];
        }
    }
    for (int j = 0; j < N; ++j)
    {
        P[2][j] = m_P[2][j] + dt * m_P[3][j]; // row 2 of F P
    }
    for (int i = 0; i < N; ++i)
    {
        m_P[i][2] = P[i][2] + dt * P[i][3]; // column 2 of (F P) F^T
        m_P[2][i] = m_P[i][2];
    }
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            if (i != 2 && j != 2)
            {
                m_P[i][j] = P[i][j];
            }
        }
    }
    m_P[2][2] += qTheta2;
    m_P[3][3] += qBias2;
    m_P[0][0] += posVar2;
    m_P[1][1] += posVar2;
    m_vx = 0.0;
    m_vy = 0.0;
    Symmetrise();
    SnapshotPrior();
}

// ---------------------------------------------------------------------------
// N.33: the robust update path. Entered only when SetRobust() set a mode.
// ---------------------------------------------------------------------------

namespace
{
const double kGate1 = 10.83;   // chi^2, 1 dof, 0.999
const double kGate2 = 13.82;   // chi^2, 2 dof, 0.999
const double kProtect1 = 6.63; // chi^2, 1 dof, 0.99: mean range NIS of a consistent fix
const double kProtect2 = 9.21; // chi^2, 2 dof, 0.99
const uint32_t kRecoverAfter = 3;             // consecutive majority-gated fixes. OURS
const double kRecoverPosVar = 50.0 * 50.0;    // m^2 added to each position axis. OURS
const double kRecoverThetaVar = 1.5708 * 1.5708; // rad^2 added to theta: (pi/2)^2. OURS
} // namespace

void
CaTblImuEkf::JosephScalar(const double H[5], const double K[5], double r)
{
    // P <- (I - K H) P (I - K H)^T + r K K^T: valid for ANY gain, which PROTECT's is not optimal.
    double A[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            A[i][j] = (i == j ? 1.0 : 0.0) - K[i] * H[j];
        }
    }
    double AP[N][N];
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            double v = 0.0;
            for (int k = 0; k < N; ++k)
            {
                v += A[i][k] * m_P[k][j];
            }
            AP[i][j] = v;
        }
    }
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            double v = 0.0;
            for (int k = 0; k < N; ++k)
            {
                v += AP[i][k] * A[j][k];
            }
            m_P[i][j] = v + r * K[i] * K[j];
        }
    }
    Symmetrise();
}

bool
CaTblImuEkf::NoteFixVerdict(bool majorityOut)
{
    // Returns true when this fix should re-open the filter (RECOVER) and be taken ungated.
    if (!(m_robust & ROBUST_RECOVER))
    {
        return false;
    }
    m_badStreak = majorityOut ? m_badStreak + 1 : 0;
    if (m_badStreak < kRecoverAfter)
    {
        return false;
    }
    m_badStreak = 0;
    ++m_nRecoveries;
    m_P[0][0] += kRecoverPosVar;
    m_P[1][1] += kRecoverPosVar;
    m_P[2][2] += kRecoverThetaVar;
    Symmetrise();
    return true;
}

void
CaTblImuEkf::UpdateRobust(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    const size_t n = std::min(meas.size(), varM2.size());
    if (n == 0)
    {
        return;
    }
    ResetCoast();
    // The fix judged as a whole, against the state before any of it is applied.
    double nisSum = 0.0;
    size_t nOut = 0;
    for (size_t k = 0; k < n; ++k)
    {
        const double dx = m_x[0] - meas[k].anchorX;
        const double dy = m_x[1] - meas[k].anchorY;
        const double r = std::max(std::sqrt(dx * dx + dy * dy), 1e-6);
        const double H0 = dx / r, H1 = dy / r;
        const double S = H0 * (m_P[0][0] * H0 + m_P[0][1] * H1) + H1 * (m_P[1][0] * H0 + m_P[1][1] * H1) + varM2[k];
        const double v = meas[k].rangeM - r;
        const double nis = (S > 0.0) ? v * v / S : 0.0;
        nisSum += nis;
        nOut += (nis > kGate1) ? 1 : 0;
    }
    const bool reopen = NoteFixVerdict(n >= 2 && 2 * nOut > n);
    const bool gate = (m_robust & ROBUST_GATE) && !reopen;
    const bool protect = (m_robust & ROBUST_PROTECT) && !reopen && (nisSum / n > kProtect1);
    if (protect)
    {
        ++m_nProtected;
    }
    const bool consider = (m_robust & ROBUST_CONSIDER) && n < 3;
    if (consider)
    {
        ++m_nConsidered;
    }
    for (size_t k = 0; k < n; ++k)
    {
        const double dx = m_x[0] - meas[k].anchorX;
        const double dy = m_x[1] - meas[k].anchorY;
        const double r = std::max(std::sqrt(dx * dx + dy * dy), 1e-6);
        const double H[N] = {dx / r, dy / r, 0.0, 0.0, 0.0};
        double PH[N];
        for (int i = 0; i < N; ++i)
        {
            PH[i] = m_P[i][0] * H[0] + m_P[i][1] * H[1];
        }
        const double S = H[0] * PH[0] + H[1] * PH[1] + varM2[k];
        if (!(S > 0.0) || !std::isfinite(S))
        {
            continue;
        }
        const double v = meas[k].rangeM - r;
        NoteNis(v * v / S); // --relayNisInflate; a no-op unless enabled
        if (gate && v * v / S > kGate1)
        {
            ++m_nGated;
            continue;
        }
        double K[N];
        for (int i = 0; i < N; ++i)
        {
            K[i] = PH[i] / S;
        }
        if (protect || consider)
        {
            K[2] = K[3] = K[4] = 0.0; // position only: the sensor states keep what they knew
        }
        for (int i = 0; i < N; ++i)
        {
            m_x[i] += K[i] * v;
        }
        JosephScalar(H, K, varM2[k]);
    }
}

bool
CaTblImuEkf::FusePositionCovRobust(double zx, double zy, double rxx, double rxy, double ryy)
{
    const double nis = PositionNis(zx, zy, rxx, rxy, ryy);
    if (nis < 0.0)
    {
        return false;
    }
    ResetCoast();
    const bool reopen = NoteFixVerdict(nis > kGate2);
    if ((m_robust & ROBUST_GATE) && !reopen && nis > kGate2)
    {
        ++m_nGated;
        return false;
    }
    const bool protect = (m_robust & ROBUST_PROTECT) && !reopen && nis > kProtect2;
    if (protect)
    {
        ++m_nProtected;
    }
    // Decorrelate R (rotate into its eigenbasis) so two scalar Joseph updates are exact.
    const double tr = rxx + ryy, det = rxx * ryy - rxy * rxy;
    const double disc = std::sqrt(std::max(0.0, tr * tr / 4.0 - det));
    const double l1 = tr / 2.0 + disc, l2 = tr / 2.0 - disc;
    double e1x = 1.0, e1y = 0.0;
    if (std::fabs(rxy) > 1e-12)
    {
        e1x = l1 - ryy;
        e1y = rxy;
        const double nn = std::sqrt(e1x * e1x + e1y * e1y);
        e1x /= nn;
        e1y /= nn;
    }
    else if (ryy > rxx)
    {
        e1x = 0.0;
        e1y = 1.0;
    }
    const double ev[2][2] = {{e1x, e1y}, {-e1y, e1x}};
    const double lam[2] = {l1, l2};
    for (int a = 0; a < 2; ++a)
    {
        const double H[N] = {ev[a][0], ev[a][1], 0.0, 0.0, 0.0};
        double PH[N];
        for (int i = 0; i < N; ++i)
        {
            PH[i] = m_P[i][0] * H[0] + m_P[i][1] * H[1];
        }
        const double S = H[0] * PH[0] + H[1] * PH[1] + lam[a];
        if (!(S > 0.0) || !std::isfinite(S))
        {
            continue;
        }
        const double v = (ev[a][0] * zx + ev[a][1] * zy) - (ev[a][0] * m_x[0] + ev[a][1] * m_x[1]);
        double K[N];
        for (int i = 0; i < N; ++i)
        {
            K[i] = PH[i] / S;
        }
        if (protect)
        {
            K[2] = K[3] = K[4] = 0.0;
        }
        for (int i = 0; i < N; ++i)
        {
            m_x[i] += K[i] * v;
        }
        JosephScalar(H, K, lam[a]);
    }
    return true;
}

// ---------------------------------------------------------------------------
// --ekf5FinderConsider: finder GNSS offsets as Schmidt consider parameters.
// ---------------------------------------------------------------------------

void
CaTblImuEkf::ConsAfterTransition(const double F[N][N])
{
    if (!m_consOn)
    {
        return;
    }
    for (auto& kv : m_cons)
    {
        double Y[N][2];
        for (int i = 0; i < N; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                double acc = 0.0;
                for (int k = 0; k < N; ++k)
                {
                    acc += F[i][k] * kv.second.X[k][c];
                }
                Y[i][c] = acc;
            }
        }
        for (int i = 0; i < N; ++i)
        {
            kv.second.X[i][0] = Y[i][0];
            kv.second.X[i][1] = Y[i][1];
        }
    }
}

void
CaTblImuEkf::ConsAfterGain(const double* K, int nK, const double* H)
{
    // A scalar update of the state alone (H on the state, none on any offset): X_b <- (I - K H) X_b.
    (void)nK;
    for (auto& kv : m_cons)
    {
        double hx[2] = {0.0, 0.0};
        for (int c = 0; c < 2; ++c)
        {
            for (int k = 0; k < N; ++k)
            {
                hx[c] += H[k] * kv.second.X[k][c];
            }
        }
        for (int i = 0; i < N; ++i)
        {
            kv.second.X[i][0] -= K[i] * hx[0];
            kv.second.X[i][1] -= K[i] * hx[1];
        }
    }
}

void
CaTblImuEkf::UpdateConsider(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    if (!meas.empty())
    {
        ResetCoast();
    }
    const size_t n = std::min(meas.size(), varM2.size());
    for (size_t k = 0; k < n; ++k)
    {
        const CaTblRangeMeasurement& mm = meas[k];
        double dx, dy;
        LinDelta(mm.anchorX, mm.anchorY, dx, dy); // --ekf5Iterate's fixed point, else the state
        const double rLin = std::sqrt(dx * dx + dy * dy);
        const double rSafe = std::max(rLin, 1e-6);
        const double H[N] = {dx / rSafe, dy / rSafe, 0.0, 0.0, 0.0};
        const double rPred = LinPred(rLin, H[0], H[1]);
        const bool finder = (mm.sourceKind == CA_TBL_SRC_FINDER);
        ConsAnchor* ca = nullptr;
        if (finder)
        {
            ca = &m_cons[mm.anchorId]; // a new finder enters uncorrelated with the state
        }
        // Ha = d r / d (anchor offset) = -h. PH = P H', XH = X Ha'.
        double PH[N], XH[N];
        for (int i = 0; i < N; ++i)
        {
            PH[i] = m_P[i][0] * H[0] + m_P[i][1] * H[1];
            XH[i] = ca ? -(ca->X[i][0] * H[0] + ca->X[i][1] * H[1]) : 0.0;
        }
        const double haCha = ca ? m_consVar : 0.0; // Ha C Ha' with C = var I and |Ha| = 1
        const double S = H[0] * PH[0] + H[1] * PH[1] + 2.0 * (H[0] * XH[0] + H[1] * XH[1]) + haCha + varM2[k];
        if (!(S > 0.0) || !std::isfinite(S))
        {
            continue;
        }
        double K[N];
        for (int i = 0; i < N; ++i)
        {
            K[i] = (PH[i] + XH[i]) / S;
        }
        const double innovation = mm.rangeM - rPred;
        if (m_relayGate && mm.sourceKind == CA_TBL_SRC_RELAY)
        {
            ++m_relaySeen;
            if (innovation * innovation / S > 10.83)
            {
                ++m_relayGated;
                continue;
            }
        }
        NoteNis(innovation * innovation / S);
        if (m_winW > 0 && !m_win.empty())
        {
            m_win.back().ranges.push_back(
                {mm.anchorX, mm.anchorY, mm.rangeM, varM2[k], finder ? static_cast<int>(mm.anchorId) : -1}); // --ekf5Window
        }
        // Cross-covariances first, from pre-update values. For the ranged finder:
        // X_a <- X_a - K (H X_a + Ha C) = X_a - K (H X_a - var h). Every other finder: X_b <- X_b - K (H X_b).
        for (auto& kv : m_cons)
        {
            double hx[2] = {0.0, 0.0};
            for (int c = 0; c < 2; ++c)
            {
                hx[c] = H[0] * kv.second.X[0][c] + H[1] * kv.second.X[1][c];
            }
            if (ca && &kv.second == ca)
            {
                hx[0] -= m_consVar * H[0];
                hx[1] -= m_consVar * H[1];
            }
            for (int i = 0; i < N; ++i)
            {
                kv.second.X[i][0] -= K[i] * hx[0];
                kv.second.X[i][1] -= K[i] * hx[1];
            }
        }
        for (int i = 0; i < N; ++i)
        {
            m_x[i] += K[i] * innovation;
        }
        // Schmidt: P <- P - K (H P + Ha X') = P - K (PH + XH)'.
        double newP[N][N];
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                newP[i][j] = m_P[i][j] - K[i] * (PH[j] + XH[j]);
            }
        }
        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                m_P[i][j] = newP[i][j];
            }
        }
        Symmetrise();
        if (finder)
        {
            ++m_consN;
        }
    }
}

void
CaTblImuEkf::ApplyBounds()
{
    if (!(m_bndK > 0.0))
    {
        return;
    }
    const double bb = m_bndK * m_bndB, bs = m_bndK * m_bndS;
    if (bb > 0.0 && std::fabs(m_x[3]) > bb)
    {
        m_x[3] = std::copysign(bb, m_x[3]);
        ++m_bndN;
    }
    if (bs > 0.0 && std::fabs(m_x[4]) > bs)
    {
        m_x[4] = std::copysign(bs, m_x[4]);
        ++m_bndN;
    }
}


// ---------------------------------------------------------------------------------------------------------------------
// --ekf5Window (TECH_DEBT N.62): the sliding-window smoother.

void
CaTblImuEkf::WinOpen(double dt, double dist, double raw, double qStep2, double qTheta2, bool dwell, double dwellVar)
{
    WinCycle c;
    c.dt = dt;
    c.dist = dist;
    c.raw = raw;
    c.qStep2 = qStep2;
    c.qTheta2 = qTheta2;
    c.dwell = dwell;
    c.dwellVar = dwellVar;
    c.x = m_x[0];
    c.y = m_x[1];
    m_win.push_back(c);
    while (static_cast<int>(m_win.size()) > m_winW)
    {
        if (m_winHaveSol && m_win.size() >= 2)
        {
            WinMarginaliseFront();
        }
        m_win.pop_front();
        if (m_winHaveSol && !m_win.empty())
        {
            m_winTheta0 += m_winB * m_win.front().dt; // theta referenced to the new first cycle
        }
    }
}

void
CaTblImuEkf::WinMarginaliseFront()
{
    // Local variables: p0 (dropped), p1, theta0, b, s, then the offsets of the finders the dropped cycle ranged and
    // the old prior's. Factors: everything that touches p0 -- its ranges, magnetometer, fixes, the old prior, and the
    // odometry p0 -> p1. Linearised at the last solution.
    const WinCycle& c0 = m_win[0];
    const WinCycle& c1 = m_win[1];
    std::vector<int> fl;
    auto fpos = [&fl](int f) {
        for (size_t i = 0; i < fl.size(); ++i)
        {
            if (fl[i] == f)
            {
                return static_cast<int>(i);
            }
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
    const int nl = 7 + 2 * static_cast<int>(fl.size());
    std::vector<double> v(nl, 0.0);
    v[0] = c0.x;
    v[1] = c0.y;
    v[2] = c1.x;
    v[3] = c1.y;
    v[4] = m_winTheta0;
    v[5] = m_winB;
    v[6] = m_winS;
    for (size_t i = 0; i < fl.size(); ++i)
    {
        auto it = m_winOff.find(fl[i]);
        v[7 + 2 * i] = it != m_winOff.end() ? it->second[0] : 0.0;
        v[8 + 2 * i] = it != m_winOff.end() ? it->second[1] : 0.0;
    }
    std::vector<double> H(static_cast<size_t>(nl) * nl, 0.0), g(nl, 0.0);
    auto add = [&](double res, const int* id, const double* jac, int m, double var) {
        const double w = 1.0 / var;
        for (int a = 0; a < m; ++a)
        {
            g[id[a]] += jac[a] * res * w;
            for (int b = 0; b < m; ++b)
            {
                H[static_cast<size_t>(id[a]) * nl + id[b]] += jac[a] * jac[b] * w;
            }
        }
    };
    for (const auto& r : c0.ranges)
    {
        double qx = r.ax, qy = r.ay;
        int cf = -1;
        if (r.finder >= 0)
        {
            cf = 7 + 2 * fpos(r.finder);
            qx += v[cf];
            qy += v[cf + 1];
        }
        const double dx = v[0] - qx, dy = v[1] - qy;
        const double rho = std::max(std::hypot(dx, dy), 1e-6);
        const double ux = dx / rho, uy = dy / rho;
        if (cf >= 0)
        {
            const int id[4] = {0, 1, cf, cf + 1};
            const double jac[4] = {ux, uy, -ux, -uy};
            add(rho - r.r, id, jac, 4, r.var);
        }
        else
        {
            const int id[2] = {0, 1};
            const double jac[2] = {ux, uy};
            add(rho - r.r, id, jac, 2, r.var);
        }
    }
    if (c0.hasMag && c0.magR > 0.0)
    {
        double nu = v[4] - c0.magZ;
        nu = std::atan2(std::sin(nu), std::cos(nu));
        const int id[1] = {4};
        const double jac[1] = {1.0};
        add(nu, id, jac, 1, c0.magR);
    }
    for (const auto& f : c0.fixes)
    {
        const double l00 = std::sqrt(f[2]);
        const double l10 = f[3] / l00;
        const double l11 = std::sqrt(std::max(f[4] - l10 * l10, 1e-12));
        const double ex = v[0] - f[0], ey = v[1] - f[1];
        const double w0 = ex / l00, w1 = (ey - l10 * w0) / l11;
        const int id[2] = {0, 1};
        const double j0[2] = {1.0 / l00, 0.0};
        const double j1[2] = {-l10 / (l00 * l11), 1.0 / l11};
        add(w0, id, j0, 2, 1.0);
        add(w1, id, j1, 2, 1.0);
    }
    // Odometry p0 -> p1 with cycle 1's increment; theta at cycle 1 = theta0 + b dt1.
    {
        const double dt1 = c1.dt;
        if (c1.dwell)
        {
            const double var = std::max(c1.dwellVar, 1e-4);
            for (int ax = 0; ax < 2; ++ax)
            {
                const int id[2] = {2 + ax, ax};
                const double jac[2] = {1.0, -1.0};
                add(v[2 + ax] - v[ax], id, jac, 2, var);
            }
        }
        else
        {
            const double th1 = v[4] + v[5] * dt1;
            const double phi = c1.raw - th1;
            const double ops = std::max(1.0 + v[6], 0.1);
            const double D = c1.dist / ops;
            const double dDds = -c1.dist / (ops * ops);
            const double var = c1.qStep2 + D * D * c1.qTheta2 + 0.01;
            const double pred[2] = {D * std::cos(phi), D * std::sin(phi)};
            const double dth[2] = {D * std::sin(phi), -D * std::cos(phi)};
            const double ds[2] = {dDds * std::cos(phi), dDds * std::sin(phi)};
            for (int ax = 0; ax < 2; ++ax)
            {
                const int id[5] = {2 + ax, ax, 4, 5, 6};
                const double jac[5] = {1.0, -1.0, -dth[ax], -dth[ax] * dt1, -ds[ax]};
                add(v[2 + ax] - v[ax] - pred[ax], id, jac, 5, var);
            }
        }
    }
    // The old prior, on (p0, theta0, b, s, its finders).
    if (m_winPriorOn)
    {
        const int kp = static_cast<int>(m_winPriorMu.size());
        std::vector<int> map(kp);
        map[0] = 0;
        map[1] = 1;
        map[2] = 4;
        map[3] = 5;
        map[4] = 6;
        for (size_t i = 0; i < m_winPriorF.size(); ++i)
        {
            const int fi = 7 + 2 * fpos(m_winPriorF[i]);
            map[5 + 2 * i] = fi;
            map[6 + 2 * i] = fi + 1;
        }
        std::vector<double> d(kp);
        for (int i = 0; i < kp; ++i)
        {
            d[i] = v[map[i]] - m_winPriorMu[i];
        }
        for (int i = 0; i < kp; ++i)
        {
            double Ld = 0.0;
            for (int j = 0; j < kp; ++j)
            {
                Ld += m_winPriorL[static_cast<size_t>(i) * kp + j] * d[j];
                H[static_cast<size_t>(map[i]) * nl + map[j]] += m_winPriorL[static_cast<size_t>(i) * kp + j];
            }
            g[map[i]] += Ld + m_winPriorBeta[i];
        }
    }
    // Schur complement: marginalise p0 (indices 0, 1) out of (H, g).
    const double a00 = H[0], a01 = H[1], a10 = H[nl], a11 = H[nl + 1];
    const double det = a00 * a11 - a01 * a10;
    if (!(det > 0.0) || !std::isfinite(det))
    {
        m_winPriorOn = false;
        return;
    }
    const double i00 = a11 / det, i01 = -a01 / det, i10 = -a10 / det, i11 = a00 / det;
    const int k = nl - 2;
    std::vector<double> L(static_cast<size_t>(k) * k), beta(k);
    for (int i = 0; i < k; ++i)
    {
        const double hi0 = H[static_cast<size_t>(i + 2) * nl + 0], hi1 = H[static_cast<size_t>(i + 2) * nl + 1];
        const double t0 = hi0 * i00 + hi1 * i10, t1 = hi0 * i01 + hi1 * i11; // H_rm H_mm^-1, row i
        for (int j = 0; j < k; ++j)
        {
            const double h0j = H[0 * static_cast<size_t>(nl) + j + 2], h1j = H[1 * static_cast<size_t>(nl) + j + 2];
            L[static_cast<size_t>(i) * k + j] = H[static_cast<size_t>(i + 2) * nl + j + 2] - (t0 * h0j + t1 * h1j);
        }
        beta[i] = g[i + 2] - (t0 * g[0] + t1 * g[1]);
    }
    // Reparametrise theta at cycle 0 to theta at cycle 1: theta0 = theta1 - b dt1, so d_old = T d_new with
    // T = I except T[theta][b] = -dt1 (indices within the reduced vector: p1 0-1, theta 2, b 3, s 4).
    const double dt1 = c1.dt;
    std::vector<double> L2(L), beta2(beta);
    for (int i = 0; i < k; ++i)
    {
        for (int j = 0; j < k; ++j)
        {
            // (T' L T)_{ij}: column b picks up -dt1 * column theta, row b picks up -dt1 * row theta.
            double vv = L[static_cast<size_t>(i) * k + j];
            if (j == 3)
            {
                vv -= dt1 * L[static_cast<size_t>(i) * k + 2];
            }
            L2[static_cast<size_t>(i) * k + j] = vv;
        }
    }
    std::vector<double> L3(L2);
    for (int j = 0; j < k; ++j)
    {
        L3[static_cast<size_t>(3) * k + j] = L2[static_cast<size_t>(3) * k + j] - dt1 * L2[static_cast<size_t>(2) * k + j];
    }
    beta2[3] = beta[3] - dt1 * beta[2];
    m_winPriorMu.assign(k, 0.0);
    m_winPriorMu[0] = v[2];
    m_winPriorMu[1] = v[3];
    m_winPriorMu[2] = v[4] + v[5] * dt1;
    m_winPriorMu[3] = v[5];
    m_winPriorMu[4] = v[6];
    for (size_t i = 0; i < fl.size(); ++i)
    {
        m_winPriorMu[5 + 2 * i] = v[7 + 2 * i];
        m_winPriorMu[6 + 2 * i] = v[8 + 2 * i];
    }
    m_winPriorF = fl;
    m_winPriorL = L3;
    m_winPriorBeta = beta2;
    m_winPriorOn = true;
}

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
CaTblImuEkf::WinSolve()
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
    const int n = iC + 2 * F;
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
            const double thk = th0 + b * tau[k];
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
                    const double var = c.qStep2 + D * D * c.qTheta2 + 0.01;
                    const double pred[2] = {D * std::cos(phi), D * std::sin(phi)};
                    const double dth[2] = {D * std::sin(phi), -D * std::cos(phi)}; // d pred / d theta_k
                    const double ds[2] = {dDds * std::cos(phi), dDds * std::sin(phi)};
                    for (int ax = 0; ax < 2; ++ax)
                    {
                        const int id[5] = {2 * k + ax, 2 * (k - 1) + ax, iT, iB, iS};
                        const double jac[5] = {1.0, -1.0, -dth[ax], -dth[ax] * tau[k], -ds[ax]};
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
                const int id[2] = {iT, iB};
                const double jac[2] = {1.0, tau[k]};
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
    const int sel[5] = {2 * (W - 1), 2 * (W - 1) + 1, iT, iB, iS};
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
    A[2][3] = T;
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
    const double xs[5] = {x[2 * (W - 1)], x[2 * (W - 1) + 1], x[iT] + x[iB] * T, x[iB], x[iS]};
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
