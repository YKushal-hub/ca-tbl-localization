/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-shs-engine.h"

namespace ns3
{
namespace lorawan
{

ShsEngine::ShsEngine(const ShsEngineParams& params, Ptr<NormalRandomVariable> normalRv)
    : m_params(params),
      m_sensor(params.sensor, normalRv),
      m_stepDetector(params.stepDetector, params.sensor.sampleRateHz),
      m_stepLength(params.stepLength),
      m_heading(params.heading),
      m_gyroBias(params.gyroBias),
      m_turnDetector(params.turnDetector)
{
}

void
ShsEngine::SetInitialState(double x, double y, double theta)
{
    m_position.SetPosition(x, y);
    m_heading.SetInitialHeading(theta);
}

void
ShsEngine::Tick(double t,
                double dt,
                double trueAx,
                double trueAy,
                double trueAz,
                double trueOmegaZ,
                double trueThetaLean)
{
    // Step 0: sensor model.
    m_sensor.Step(m_sensorState, dt, trueAx, trueAy, trueAz, trueOmegaZ, trueThetaLean);

    // ZARU gyro bias observer. Driven off the step detector's walking state
    // from the previous evaluated window, which is the stillness signal it
    // needs; windows are evaluated once per second, so the classification lags
    // by at most that, which is immaterial against the dwell requirement.
    m_gyroBias.Update(dt, m_sensorState.measGz, m_stepDetector.IsWalking());

    // Step 4: heading integration runs every tick, independent of step
    // detection, now with the observed bias subtracted.
    m_heading.Step(dt, m_sensorState.measGz, m_gyroBias.GetBiasEstimate());

    // Turn detection from the integrated heading (PMC5591331: >90 deg over
    // 0.5-10 s). Exposed for diagnostics and as the hook for turn-adaptive
    // processing; it does not alter detection yet, deliberately -- see the
    // TurnDetector class comment.
    m_turnDetector.Update(t, m_heading.GetTheta());

    // Steps 1-2: step detection and valley-peak-valley extraction.
    //
    // AXIS ASSIGNMENT. The sensor model now carries three accelerometer axes,
    // so this wiring is an explicit decision rather than the two-axis default
    // it used to be:
    //
    //   measAy -> vertical channel            (drives peak finding, and the
    //                                          Weinberg a_max/a_min handoff)
    //   measAx -> anterior-posterior channel  (contributes to the walking gate)
    //   measAz -> lateral, DELIBERATELY NOT FED to the detector
    //
    // The lateral axis is excluded because the Iluz method specifies vertical
    // plus anterior-posterior, and lateral is neither. It is also where the
    // turn-synchronous signals now live (centripetal acceleration and the
    // gravity component that trunk lean rotates out of vertical), so feeding it
    // in would inject a large non-gait transient at every turn.
    //
    // STILL A PLACEHOLDER as to which PHYSICAL MPU-6050 axis is which. That
    // depends on the real backpack mounting orientation and remains unresolved.
    // The consequence is now sharper than before: if the axis physically facing
    // laterally is the one wired here as vertical, every turn dumps ~1-3 m/s^2
    // of non-gait signal straight into the step detector's primary input. The
    // LateralAxisTurnResponseTest in the test suite quantifies that case.
    std::vector<StepEvent> newSteps =
        m_stepDetector.Step(t, m_sensorState.measAy, m_sensorState.measAx);

    for (const auto& step : newSteps)
    {
        // Step 3: step length. The heading is buffered alongside the step
        // because calibration sums the bracketed steps as vectors, and must
        // use the same heading the position update below applies.
        double stepLength = m_stepLength.EstimateStepLength(step);
        double theta = m_heading.GetTheta();
        m_stepLength.BufferStep(step, theta);

        // Step 5: position update, using the heading at the time of this step.
        m_position.ApplyStep(stepLength, theta);
        ++m_detectedStepCount;
    }
}

void
ShsEngine::OnTrustedFix(double timestamp, double x, double y, bool isClean)
{
    m_stepLength.OnTrustedFix(TrustedRadioFix{timestamp, x, y, isClean});
    m_heading.OnTrustedFix(timestamp, x, y, isClean);
    if (isClean && m_params.snapPositionToTrustedFix)
    {
        // Opt-in only. Snapping resets the dead-reckoned position at every fix,
        // which makes the SHS position error unmeasurable and discards the
        // estimate even when it beats a noisy fix. See
        // ShsEngineParams::snapPositionToTrustedFix. The cold-start motivation
        // this once had is now served by SetInitialState, which seeds the known
        // deployment position directly.
        m_position.SetPosition(x, y);
    }
}

} // namespace lorawan
} // namespace ns3
