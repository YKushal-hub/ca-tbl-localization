/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Orchestrator for the backpack-mounted IMU Step-and-Heading System (SHS),
 * wiring Steps 0-5 together.
 */

#ifndef LORA_SHS_ENGINE_H
#define LORA_SHS_ENGINE_H

#include "lora-gyro-bias-observer.h"
#include "lora-heading-submodule.h"
#include "lora-imu-sensor-submodule.h"
#include "lora-position-update-submodule.h"
#include "lora-shs-parameters.h"
#include "lora-step-detector-submodule.h"
#include "lora-step-length-submodule.h"
#include "lora-turn-detector.h"

#include "ns3/random-variable-stream.h"

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Backpack-mounted IMU Step-and-Heading System engine.
 *
 * Ties together the five pipeline stages: the MPU-6050 sensor model (Step 0),
 * Iluz step detection with Ho et al. valley-peak-valley extraction (Steps 1-2),
 * the Weinberg step-length model with online k_w calibration (Step 3), the
 * gyro-integration heading estimator with RF-aided drift correction (Step 4),
 * and the per-step position update (Step 5).
 *
 * The IMU is assumed to be trunk-mounted (in or on a backpack), not
 * foot-mounted. ZUPT is therefore unavailable, which is precisely why this is
 * an SHS rather than a strapdown-plus-ZUPT design. See doc/imu-shs-design.md
 * Section 1.
 *
 * This engine produces a position estimate and stops there. It is deliberately
 * not wired into the EKF prediction step or the Switching-Point Integrity
 * Check: those were derived against a strapdown prior whose error model does
 * not apply to an SHS prior, and that redesign has not been started (design doc
 * Section 7).
 *
 * @see ShsEngineParams
 */
class ShsEngine
{
  public:
    /**
     * Constructor.
     *
     * @param params Parameters for all five pipeline stages.
     * @param normalRv Normal random variable stream driving the sensor model.
     */
    ShsEngine(const ShsEngineParams& params, Ptr<NormalRandomVariable> normalRv);

    /**
     * Seed the known deployment state: exact starting position and the
     * orientation the node was deployed in.
     *
     * Without this the engine must bootstrap from radio fixes alone, which
     * leaves a cold-start window it may never escape: theta_0 is only derived
     * once two fixes bracket a stretch walked approximately straight, so on a
     * route whose every fix interval contains a turn the heading stays on its
     * fallback for the whole run (design doc Flag 5, measured at ~0.35 rad and
     * 15 m of position error on a square loop at 60 s fix intervals).
     *
     * Note this seeds position and heading only. Stride length still has to be
     * calibrated: k_w remains at StepLengthParams::kwFallback until a fix pair
     * brackets a known distance, so a straight calibration walk at deployment
     * is still what removes the last of the cold start.
     *
     * Call once, before the first Tick.
     *
     * @param x Known starting position x, m.
     * @param y Known starting position y, m.
     * @param theta Known deployment heading, rad, standard mathematical
     *              convention (0 along +x, increasing counterclockwise).
     */
    void SetInitialState(double x, double y, double theta);

    /**
     * Advance the pipeline by one IMU sample tick.
     *
     * @param t Simulation time of this tick, s.
     * @param dt Tick duration, s (nominally 1/sampleRateHz).
     * @param trueAx Ground-truth anterior-posterior acceleration, m/s^2.
     * @param trueAy Ground-truth vertical acceleration INCLUDING gravity, m/s^2.
     * @param trueAz Ground-truth lateral acceleration, m/s^2. During a turn this
     *               carries centripetal acceleration plus the gravity component
     *               trunk lean rotates out of vertical.
     * @param trueOmegaZ Ground-truth yaw rate, rad/s.
     * @param trueThetaLean Instantaneous trunk lean, rad, for logging only.
     */
    void Tick(double t,
              double dt,
              double trueAx,
              double trueAy,
              double trueAz,
              double trueOmegaZ,
              double trueThetaLean = 0.0);

    /**
     * Handle a fresh radio fix from the RF side of the system.
     *
     * @param timestamp Time the fix refers to, s.
     * @param x Fix position x, m.
     * @param y Fix position y, m.
     * @param isClean CLEAN vs SUSPECT flag from the Switching-Point Integrity Check.
     */
    void OnTrustedFix(double timestamp, double x, double y, bool isClean);

    /**
     * Get the current estimated position x.
     *
     * @return Position x, m.
     */
    double GetX() const
    {
        return m_position.GetX();
    }

    /**
     * Get the current estimated position y.
     *
     * @return Position y, m.
     */
    double GetY() const
    {
        return m_position.GetY();
    }

    /**
     * Report whether the wearer is currently turning, per the yaw-based rule.
     *
     * @return True while a qualifying yaw change is in progress.
     */
    bool IsTurning() const
    {
        return m_turnDetector.IsTurning();
    }

    /**
     * Get the number of turns detected so far.
     *
     * @return The detected-turn count.
     */
    uint32_t GetTurnCount() const
    {
        return m_turnDetector.GetTurnCount();
    }

    /**
     * Get the total time spent turning, s.
     *
     * @return Accumulated turning duration.
     */
    double GetTurningTimeS() const
    {
        return m_turnDetector.GetTurningTimeS();
    }

    /**
     * Get the current gyro bias estimate from the ZARU observer.
     *
     * @return The estimated gyro Z bias, rad/s, or 0.0 if none adopted yet.
     */
    double GetGyroBiasEstimate() const
    {
        return m_gyroBias.GetBiasEstimate();
    }

    /**
     * Get the number of ZARU bias estimates adopted so far.
     *
     * @return The adopted-estimate count.
     */
    uint32_t GetGyroBiasUpdateCount() const
    {
        return m_gyroBias.GetUpdateCount();
    }

    /**
     * Get the current heading estimate.
     *
     * @return Heading, rad.
     */
    double GetTheta() const
    {
        return m_heading.GetTheta();
    }

    /**
     * Get the Weinberg constant currently in use.
     *
     * @return The current k_w.
     */
    double GetCurrentKw() const
    {
        return m_stepLength.GetCurrentKw();
    }

    /**
     * Get the number of steps detected so far.
     *
     * @return The cumulative detected step count.
     */
    uint32_t GetDetectedStepCount() const
    {
        return m_detectedStepCount;
    }

  private:
    ShsEngineParams m_params; //!< Parameters for all five stages

    ImuSensorSubmodule m_sensor;            //!< Step 0
    StepDetectorSubmodule m_stepDetector;   //!< Steps 1-2
    StepLengthSubmodule m_stepLength;       //!< Step 3
    HeadingSubmodule m_heading;             //!< Step 4
    PositionUpdateSubmodule m_position;     //!< Step 5
    GyroBiasObserver m_gyroBias;            //!< ZARU gyro bias observer, aids Step 4
    TurnDetector m_turnDetector;            //!< Yaw-based turn detection, diagnostics

    ImuSensorState m_sensorState;        //!< Running sensor model state
    uint32_t m_detectedStepCount = 0;    //!< Cumulative count of accepted steps
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_SHS_ENGINE_H
