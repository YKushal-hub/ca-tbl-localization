/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-step-length-submodule.h"

#include <algorithm>
#include <cmath>

namespace ns3
{
namespace lorawan
{

StepLengthSubmodule::StepLengthSubmodule(const StepLengthParams& params)
    : m_params(params),
      m_kw(params.kwFallback)
{
}

// Weinberg's formula: s_w = k_w * (a_max - a_min)^(1/4). See
// doc/imu-shs-design.md Section 4 for the citation (Weinberg, Analog Devices
// AN-602, 2002).
double
StepLengthSubmodule::EstimateStepLength(const StepEvent& step) const
{
    double diff = std::max(0.0, step.aMax - step.aMin);
    return m_kw * std::pow(diff, 0.25);
}

void
StepLengthSubmodule::BufferStep(const StepEvent& step, double theta)
{
    m_bufferedSteps.push_back(BufferedStep{step, theta});
}

// Online calibration: whenever two consecutive trusted fixes bracket a set of
// detected steps, back-solve k_w from the real distance covered. Pattern
// source: GIScience & Remote Sensing, 2024, DOI 10.1080/10095020.2024.2338225.
//
// The displacement between the two fixes is the VECTOR sum of the steps taken
// between them:
//
//     d = sum_i ( s_i * u_i ) = k_w * sum_i ( q_i * u_i )
//
// where q_i = (a_max - a_min)^(1/4) and u_i is the unit heading of step i, so
//
//     k_w = |d| / | sum_i ( q_i * u_i ) |
//
// Summing q_i as a SCALAR instead -- i.e. dividing the chord by the total path
// length -- is only correct when every step pointed the same way. When the
// wearer turns inside a fix interval the chord is shorter than the path walked,
// and the scalar form biases k_w LOW in proportion to how much turning
// happened: measured at -2.7% over a 10 s interval, -10.4% over 30 s and -29.4%
// over 60 s on a square loop, the last costing 36 m of position error. The
// vector form below is exact for any path shape and reduces to the scalar form
// on a straight line.
void
StepLengthSubmodule::OnTrustedFix(const TrustedRadioFix& fix)
{
    if (!fix.isClean)
    {
        // Never calibrate against a SUSPECT fix. Keep the last known-good fix
        // as the bracket start so a later CLEAN fix can still pair with it;
        // steps keep accumulating in the meantime via BufferStep, which the
        // caller is expected to go on calling regardless.
        return;
    }

    if (m_havePrevFix && !m_bufferedSteps.empty())
    {
        double dx = fix.x - m_prevFix.x;
        double dy = fix.y - m_prevFix.y;
        double chord = std::sqrt(dx * dx + dy * dy);

        double sumQuarticDiffs = 0.0; // scalar sum, i.e. path length / k_w
        double vecX = 0.0;
        double vecY = 0.0;
        for (const auto& b : m_bufferedSteps)
        {
            double diff = std::max(0.0, b.step.aMax - b.step.aMin);
            double q = std::pow(diff, 0.25);
            sumQuarticDiffs += q;
            vecX += q * std::cos(b.theta);
            vecY += q * std::sin(b.theta);
        }
        double vecMagnitude = std::sqrt(vecX * vecX + vecY * vecY);

        // Straightness of the bracketed stretch, in [0, 1]: 1 is a straight
        // line, 0 a path that returns to where it started. As it approaches 0
        // the chord carries almost no information about scale -- k_w = chord /
        // vecMagnitude amplifies both fix noise and heading error without
        // bound -- so refuse to calibrate rather than accept a wild estimate.
        bool straightEnough =
            sumQuarticDiffs > 1e-9 &&
            (vecMagnitude / sumQuarticDiffs) >= m_params.minCalibrationStraightness;

        if (straightEnough && vecMagnitude > 1e-9)
        {
            double kwEstimate = chord / vecMagnitude;
            if (!m_hasCalibrated)
            {
                m_kw = kwEstimate; // first calibration: adopt directly, no EMA history yet
                m_hasCalibrated = true;
            }
            else
            {
                m_kw = m_params.kwEmaAlpha * kwEstimate + (1.0 - m_params.kwEmaAlpha) * m_kw;
            }
        }
    }

    m_prevFix = fix;
    m_havePrevFix = true;
    m_bufferedSteps.clear();
}

} // namespace lorawan
} // namespace ns3
