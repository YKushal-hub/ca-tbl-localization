/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Step 4 of the backpack SHS pipeline: gyro-integration heading estimator with
 * RF-aided theta_0 initialization and drift correction.
 */

#ifndef LORA_HEADING_SUBMODULE_H
#define LORA_HEADING_SUBMODULE_H

#include "lora-shs-parameters.h"
#include "lora-step-length-submodule.h"

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Gyro-integration heading estimator (Step 4), with both theta_0 and ongoing
 * drift correction derived from trusted radio fixes.
 *
 * Integrates theta(t) = theta_0 + integral(w_z - b_g_hat) dt. The sign
 * convention is the standard mathematical one -- theta = 0 along +x, positive
 * counterclockwise -- which matches the MPU-6050's native right-hand-rule gyro
 * Z output directly, so no sign flip is applied. See doc/imu-shs-design.md
 * Section 4 for the convention and Section 6 Flag 4 for the validation gap in
 * substituting LoRa for the UWB/WiFi aiding source the pattern was published
 * with.
 *
 * @see HeadingParams
 */
class HeadingSubmodule
{
  public:
    /**
     * Constructor.
     *
     * @param params Heading estimator parameters.
     */
    explicit HeadingSubmodule(const HeadingParams& params);

    /**
     * Seed theta_0 directly from a known deployment orientation, instead of
     * waiting to derive it from a pair of trusted fixes.
     *
     * The fix-pair derivation can only run once two fixes bracket a stretch the
     * wearer walked approximately straight. Where the node is deployed in a
     * known orientation that wait is unnecessary, and avoiding it removes the
     * cold-start window entirely: on a route whose every fix interval contains
     * a turn, the derivation never fires at all and the heading stays on
     * HeadingParams::theta0Fallback indefinitely (design doc Flag 5).
     *
     * Call once, before the first Step. Marks the heading initialized, so the
     * fix-pair derivation will not later overwrite the seeded value; ongoing
     * drift correction still applies as normal.
     *
     * @param theta Known heading at deployment, rad, standard mathematical
     *              convention (0 along +x, increasing counterclockwise).
     */
    void SetInitialHeading(double theta);

    /**
     * Integrate the heading forward by one gyro sample tick.
     *
     * @param dt Tick duration, s.
     * @param gzMeasured Raw gyro Z-axis reading, rad/s.
     * @param gzBiasEstimate Running gyro bias estimate, rad/s (0.0 if none).
     */
    void Step(double dt, double gzMeasured, double gzBiasEstimate);

    /**
     * Get the current heading estimate.
     *
     * @return Heading in radians, standard mathematical convention.
     */
    double GetTheta() const
    {
        return m_theta;
    }

    /**
     * Report whether theta_0 has been derived from real fixes yet.
     *
     * @return True once theta_0 has been initialized, false while the
     *         provisional HeadingParams::theta0Fallback is still in force.
     */
    bool HasBeenInitialized() const
    {
        return m_initialized;
    }

    /**
     * Handle a new trusted radio fix. This covers both jobs:
     * if this is the second trusted fix and theta_0 is not yet set, theta_0 is
     * derived from the direction implied by the first two fixes; otherwise, if
     * the path since the previous fix looks like a straight-line stretch, the
     * accumulated heading is corrected toward the implied direction.
     *
     * @param timestamp Time the fix refers to, s.
     * @param x Fix position x, m.
     * @param y Fix position y, m.
     * @param isClean CLEAN vs SUSPECT flag; SUSPECT fixes are ignored entirely.
     */
    void OnTrustedFix(double timestamp, double x, double y, bool isClean);

  private:
    /**
     * Decide whether the stretch since the previous fix can be treated as a
     * straight line, and therefore whether the discrepancy between the implied
     * and integrated headings is drift worth correcting rather than a real turn.
     *
     * @param thetaBeforeCorrection The integrated heading estimate, rad.
     * @param thetaImplied The heading implied by the last two fixes, rad.
     * @return True if the stretch should be used for drift correction.
     */
    bool IsStraightLineStretch(double thetaBeforeCorrection, double thetaImplied) const;

    HeadingParams m_params;     //!< Heading estimator parameters
    double m_theta = 0.0;       //!< Current heading estimate, rad
    bool m_initialized = false; //!< True once theta_0 has been set from real fixes

    bool m_haveFirstFix = false; //!< True once the first trusted fix has arrived
    double m_firstFixT = 0.0;    //!< Timestamp of the first trusted fix, s
    double m_firstFixX = 0.0;    //!< Position x of the first trusted fix, m
    double m_firstFixY = 0.0;    //!< Position y of the first trusted fix, m
    /// Accumulated heading when the candidate theta_0 pair's first fix arrived, rad
    double m_thetaAtFirstFix = 0.0;

    bool m_havePrevFix = false;    //!< True once a previous trusted fix is stored
    double m_prevFixT = 0.0;       //!< Timestamp of the previous trusted fix, s
    double m_prevFixX = 0.0;       //!< Position x of the previous trusted fix, m
    double m_prevFixY = 0.0;       //!< Position y of the previous trusted fix, m
    double m_thetaAtPrevFix = 0.0; //!< Accumulated heading at the time of the previous fix, rad
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_HEADING_SUBMODULE_H
