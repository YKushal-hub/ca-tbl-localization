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
    SnapshotPrior();
}

void
CaTblImuEkf::SnapshotPrior()
{
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
    if (!(r > 0.0) || !std::isfinite(z))
    {
        return;
    }
    const double S = m_P[2][2] + r;
    if (!(S > 0.0))
    {
        return;
    }
    double nu = z - m_x[2];
    nu = std::atan2(std::sin(nu), std::cos(nu));
    double K[N];
    for (int i = 0; i < N; ++i)
    {
        K[i] = m_P[i][2] / S;
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
    if (m_robust != 0)
    {
        UpdateRobust(meas, varM2); // N.33; mode 0 below is the original, untouched
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
        const double dx = m_x[0] - meas[k].anchorX;
        const double dy = m_x[1] - meas[k].anchorY;
        const double rPred = std::sqrt(dx * dx + dy * dy);
        const double rSafe = std::max(rPred, 1e-6);
        const double H0 = dx / rSafe;
        const double H1 = dy / rSafe;

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
        NoteNis(innovation * innovation / S); // --relayNisInflate; a no-op unless enabled
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
    if (m_robust != 0)
    {
        FusePositionCovRobust(zx, zy, varM2, 0.0, varM2); // N.33
        return;
    }
    if (!(varM2 > 0.0) || !std::isfinite(varM2) || !std::isfinite(zx) || !std::isfinite(zy))
    {
        return;
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
    if (m_robust != 0)
    {
        return FusePositionCovRobust(zx, zy, rxx, rxy, ryy); // N.33
    }
    if (!std::isfinite(zx) || !std::isfinite(zy) || !std::isfinite(rxx) || !std::isfinite(rxy) ||
        !std::isfinite(ryy) || !(rxx > 0.0) || !(ryy > 0.0) || !(rxx * ryy - rxy * rxy > 0.0))
    {
        return false;
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
    // theta' = theta + b dt is the only state that moves: F = I except d theta' / d b = dt.
    // P' = F P F^T, written out for that single off-diagonal.
    m_x[2] += m_x[3] * dt;
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

} // namespace ns3
