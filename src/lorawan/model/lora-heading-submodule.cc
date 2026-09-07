/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-heading-submodule.h"

#include <algorithm>
#include <cmath>

namespace ns3
{
namespace lorawan
{

/**
 * Wrap an angle to the (-pi, pi] interval.
 *
 * @param angle The angle to wrap, rad.
 * @return The equivalent angle in (-pi, pi].
 */
/**
 * Heuristic multiplier on the configured straight-line tolerance -- an
 * implementation choice (design doc Flag 4: this whole RF-aided correction
 * pattern is unvalidated at LoRa's accuracy and update-rate profile). Applied
 * both to the rotation-since-last-fix guard and to the implied-direction
 * agreement check in OnTrustedFix.
 */
static const double kToleranceMultiplier = 3.0;

/**
 * Wrap an angle to the (-pi, pi] interval.
 *
 * @param angle The angle to wrap, rad.
 * @return The equivalent angle in (-pi, pi].
 */
static double
WrapToPi(double angle)
{
    while (angle > M_PI)
    {
        angle -= 2.0 * M_PI;
    }
    while (angle < -M_PI)
    {
        angle += 2.0 * M_PI;
    }
    return angle;
}

HeadingSubmodule::HeadingSubmodule(const HeadingParams& params)
    : m_params(params),
      m_theta(params.theta0Fallback)
{
}

void
HeadingSubmodule::SetInitialHeading(double theta)
{
    m_theta = theta;
    m_initialized = true;
    m_thetaAtFirstFix = theta;
    m_thetaAtPrevFix = theta;
}

// theta(t) = theta_0 + integral(w_z - b_g_hat) dt. Sign convention: positive
// gyro Z = counterclockwise = increasing theta, matching the MPU-6050's native
// right-hand-rule output directly -- no flip. See doc/imu-shs-design.md.
void
HeadingSubmodule::Step(double dt, double gzMeasured, double gzBiasEstimate)
{
    m_theta += (gzMeasured - gzBiasEstimate) * dt;
}

bool
HeadingSubmodule::IsStraightLineStretch(double thetaBeforeCorrection, double thetaImplied) const
{
    double diff = std::fabs(WrapToPi(thetaImplied - thetaBeforeCorrection));
    // If the observed discrepancy is small, treat it as pure gyro drift and
    // correct; if it is large, assume the wearer actually turned during the
    // interval and leave theta alone, since correcting through a real turn
    // would be wrong rather than merely imprecise. See kToleranceMultiplier.
    return diff <= (m_params.straightLineToleranceRad * kToleranceMultiplier);
}

void
HeadingSubmodule::OnTrustedFix(double timestamp, double x, double y, bool isClean)
{
    if (!isClean)
    {
        return; // never calibrate or correct against a SUSPECT fix
    }

    if (!m_haveFirstFix)
    {
        m_haveFirstFix = true;
        m_firstFixT = timestamp;
        m_firstFixX = x;
        m_firstFixY = y;
        m_thetaAtFirstFix = m_theta;
        m_havePrevFix = true;
        m_prevFixT = timestamp;
        m_prevFixX = x;
        m_prevFixY = y;
        m_thetaAtPrevFix = m_theta;
        return;
    }

    if (!m_initialized)
    {
        // Derive theta_0 from the direction implied by a pair of fixes, which
        // is only valid if the stretch between them was approximately straight
        // (design doc Section 4, theta_0 item). That assumption has to be
        // CHECKED, not assumed: the implied direction is the average heading
        // over the interval, so initializing across a stretch containing a turn
        // seeds theta_0 with a direction the wearer never travelled, and the
        // error persists through every subsequent integration. Measured on a
        // square loop at 60 s fix intervals, initializing across one 90 degree
        // turn cost ~1.0 rad of heading error for the rest of the run.
        //
        // Rotation is measured by the gyro since the candidate pair's first
        // fix, the same quantity the drift-correction guard below uses.
        double rotationSinceFirstFix = std::fabs(WrapToPi(m_theta - m_thetaAtFirstFix));
        bool straightEnough =
            rotationSinceFirstFix <= (m_params.straightLineToleranceRad * kToleranceMultiplier);

        double dx = x - m_firstFixX;
        double dy = y - m_firstFixY;
        double baseline = std::sqrt(dx * dx + dy * dy);
        if (straightEnough && baseline >= m_params.minCorrectionBaselineM)
        {
            double thetaImplied = std::atan2(dy, dx);
            m_theta = thetaImplied; // overwrite the arbitrary theta_0 fallback
            m_initialized = true;
        }
        else
        {
            // Slide the candidate pair forward: this fix becomes the start of
            // the next attempt, so a later straight stretch can still
            // initialize. Until one arrives theta stays at the fallback, which
            // is the cold-start window the design already flags (Flag 5).
            m_firstFixT = timestamp;
            m_firstFixX = x;
            m_firstFixY = y;
            m_thetaAtFirstFix = m_theta;
        }
        m_prevFixT = timestamp;
        m_prevFixX = x;
        m_prevFixY = y;
        m_thetaAtPrevFix = m_theta;
        return;
    }

    // Subsequent fixes: attempt ongoing drift correction.
    //
    // Two independent guards must both pass, because either alone is fooled.
    //
    // 1. Rotation actually integrated since the previous fix. The direction
    //    implied by a fix pair is the AVERAGE heading over the interval, not
    //    the instantaneous heading now, so it may only be used as a correction
    //    when the wearer barely turned. Comparing the chord against the current
    //    heading (guard 2) cannot establish this on its own: after a turn the
    //    chord can coincidentally fall close to the new heading and be accepted,
    //    snapping theta to a direction that was never true. Measured on a square
    //    loop at 60 s fix intervals, that mis-acceptance cost ~1.0 rad of
    //    heading error.
    //
    // 2. Agreement between the implied direction and the current heading, which
    //    catches a bad fix that guard 1 would wave through.
    double rotationSincePrevFix = std::fabs(WrapToPi(m_theta - m_thetaAtPrevFix));
    bool barelyTurned =
        rotationSincePrevFix <= (m_params.straightLineToleranceRad * kToleranceMultiplier);

    // 3. A baseline long enough that the implied direction is signal rather
    //    than fix noise. Over a short baseline atan(sigma_fix / baseline) swamps
    //    the drift being corrected, so snapping to it is actively harmful. See
    //    HeadingParams::minCorrectionBaselineM.
    double dx = x - m_prevFixX;
    double dy = y - m_prevFixY;
    double baseline = std::sqrt(dx * dx + dy * dy);
    if (barelyTurned && baseline >= m_params.minCorrectionBaselineM)
    {
        double thetaImplied = std::atan2(dy, dx);
        if (IsStraightLineStretch(m_theta, thetaImplied))
        {
            m_theta = thetaImplied; // hard snap-correction, see design doc Flag 4
        }
        // else: treat this as a real turn and leave m_theta uncorrected
    }

    m_prevFixT = timestamp;
    m_prevFixX = x;
    m_prevFixY = y;
    m_thetaAtPrevFix = m_theta;
}

} // namespace lorawan
} // namespace ns3
