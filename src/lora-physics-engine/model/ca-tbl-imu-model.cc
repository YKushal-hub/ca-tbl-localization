#include "ca-tbl-imu-model.h"

#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/object-factory.h"

#include <cmath>

namespace ns3
{

void
CaTblProjectPsd2(double m[3])
{
    const double a = m[0], b = m[1], c = m[2];
    const double tr = a + c;
    const double disc = std::sqrt(std::max(0.0, (a - c) * (a - c) / 4.0 + b * b));
    const double l1 = tr / 2.0 + disc;
    const double l2 = tr / 2.0 - disc;
    if (l2 >= 0.0)
    {
        return;
    }
    if (l1 <= 0.0)
    {
        m[0] = m[1] = m[2] = 0.0;
        return;
    }
    // Keep the positive eigenvalue's component: l1 v v^T, v its unit eigenvector.
    double vx, vy;
    if (std::fabs(b) > 1e-300)
    {
        vx = l1 - c;
        vy = b;
    }
    else
    {
        vx = (a >= c) ? 1.0 : 0.0;
        vy = (a >= c) ? 0.0 : 1.0;
    }
    const double n = std::sqrt(vx * vx + vy * vy);
    vx /= n;
    vy /= n;
    m[0] = l1 * vx * vx;
    m[1] = l1 * vx * vy;
    m[2] = l1 * vy * vy;
}

void
CaTblImuDeadReckoner::Configure(const CaTblImuParams& p, int64_t stream)
{
    m_p = p;
    // The stream is a CONSTRUCTION attribute (plan sect 0 rule 5): CreateObject followed by
    // SetStream would first take the next automatic stream index, and so displace every
    // random variable created after it.
    m_rv = CreateObjectWithAttributes<NormalRandomVariable>("Stream", IntegerValue(stream),
                                                            "Mean", DoubleValue(0.0),
                                                            "Variance", DoubleValue(1.0));
    m_bias = m_p.gyroBiasSigmaRadS * m_rv->GetValue();
    m_scale = m_p.scaleSigma * m_rv->GetValue();
    m_psi = 0.0;
    m_t = 0.0;
    ResetAccumulation();
}

void
CaTblImuDeadReckoner::ResetAccumulation()
{
    m_dx = 0.0;
    m_dy = 0.0;
    m_sigAcc[0] = m_sigAcc[1] = m_sigAcc[2] = 0.0;
}

void
CaTblImuDeadReckoner::Step(double dx, double dy, double dt, double& ux, double& uy, double qu[3])
{
    m_t += dt;
    m_psi += m_bias * dt + m_p.gyroNoiseDensityRadSqrtS * std::sqrt(dt) * m_rv->GetValue();
    const double c = std::cos(m_psi), s = std::sin(m_psi);
    ux = (1.0 + m_scale) * (c * dx - s * dy);
    uy = (1.0 + m_scale) * (s * dx + c * dy);

    const double step = std::sqrt(dx * dx + dy * dy);
    if (!(step > 0.0))
    {
        // Dwelling: no direction to project onto, and the accumulated error is unchanged.
        const double w = m_p.dwellSigmaM * m_p.dwellSigmaM;
        qu[0] = w;
        qu[1] = 0.0;
        qu[2] = w;
        return;
    }
    m_dx += dx;
    m_dy += dy;
    const double prev[3] = {m_sigAcc[0], m_sigAcc[1], m_sigAcc[2]};
    const double varPsi = m_p.gyroBiasSigmaRadS * m_p.gyroBiasSigmaRadS * m_t * m_t +
                          m_p.gyroNoiseDensityRadSqrtS * m_p.gyroNoiseDensityRadSqrtS * m_t;
    const double ss = m_p.scaleSigma * m_p.scaleSigma;
    // var_psi |D|^2 p p^T with p = (-Dy, Dx)/|D| is var_psi (Dy^2, -Dx Dy, Dx^2).
    m_sigAcc[0] = varPsi * m_dy * m_dy + ss * m_dx * m_dx;
    m_sigAcc[1] = -varPsi * m_dx * m_dy + ss * m_dx * m_dy;
    m_sigAcc[2] = varPsi * m_dx * m_dx + ss * m_dy * m_dy;
    qu[0] = m_sigAcc[0] - prev[0];
    qu[1] = m_sigAcc[1] - prev[1];
    qu[2] = m_sigAcc[2] - prev[2];
    CaTblProjectPsd2(qu);
}

} // namespace ns3
