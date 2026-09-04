/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Unit tests for the backpack-mounted IMU Step-and-Heading System (SHS)
 * pipeline, following the per-submodule test plan in the design handoff.
 *
 * This file includes testing for the following components:
 * - ImuSensorSubmodule       (Step 0)
 * - StepDetectorSubmodule    (Steps 1-2)
 * - StepLengthSubmodule      (Step 3)
 * - HeadingSubmodule         (Step 4)
 * - PositionUpdateSubmodule  (Step 5)
 * - ShsEngine                (end-to-end)
 */

#include "ns3/log.h"
#include "ns3/lora-gyro-bias-observer.h"
#include "ns3/lora-heading-submodule.h"
#include "ns3/lora-imu-sensor-submodule.h"
#include "ns3/lora-position-update-submodule.h"
#include "ns3/lora-shs-engine.h"
#include "ns3/lora-shs-parameters.h"
#include "ns3/lora-step-detector-submodule.h"
#include "ns3/lora-step-length-submodule.h"
#include "ns3/lora-turn-detector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/test.h"

#include <cmath>
#include <vector>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("LorawanShsTestSuite");

namespace
{

/**
 * Synthesize the vertical acceleration of one gait cycle, matching the profile
 * used by the reference validation harness: a half-sine bounce during the first
 * 40% of the cycle, flat for the rest.
 *
 * @param t Time, s.
 * @param cadenceHz Step cadence, Hz.
 * @param amplitude Peak vertical acceleration, m/s^2.
 * @return The synthetic vertical acceleration at time t, m/s^2.
 */
double
SyntheticVerticalAccel(double t, double cadenceHz, double amplitude)
{
    double period = 1.0 / cadenceHz;
    double cyclePhase = std::fmod(t, period) / period;
    if (cyclePhase < 0.4)
    {
        return amplitude * std::sin(M_PI * cyclePhase / 0.4);
    }
    return 0.0;
}

} // namespace

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests the MPU-6050 sensor model (Step 0): that the measurement equations
 * reproduce the ground truth in the mean, and that the Gauss-Markov bias
 * process stays within the stationary bound implied by its bias instability.
 */
class ImuSensorSubmoduleTest : public TestCase
{
  public:
    ImuSensorSubmoduleTest();           //!< Default constructor
    ~ImuSensorSubmoduleTest() override; //!< Destructor

  private:
    void DoRun() override;
};

ImuSensorSubmoduleTest::ImuSensorSubmoduleTest()
    : TestCase("Verify that the IMU sensor model is unbiased and its Gauss-Markov bias is bounded")
{
}

ImuSensorSubmoduleTest::~ImuSensorSubmoduleTest()
{
}

void
ImuSensorSubmoduleTest::DoRun()
{
    ImuSensorParams params;
    auto rv = CreateObject<NormalRandomVariable>();
    ImuSensorSubmodule sensor(params, rv);
    ImuSensorState state;

    const double dt = 1.0 / params.sampleRateHz;
    const uint32_t n = 60000; // 600 s at 100 Hz
    const double trueAx = 0.5;
    const double trueAy = -0.25;
    const double trueAz = 0.15;
    const double trueOmegaZ = 0.1;

    double sumAx = 0.0;
    double sumAy = 0.0;
    double sumAz = 0.0;
    double sumGz = 0.0;
    double maxAbsBiasA = 0.0;
    double maxAbsBiasG = 0.0;

    for (uint32_t i = 0; i < n; ++i)
    {
        sensor.Step(state, dt, trueAx, trueAy, trueAz, trueOmegaZ);
        sumAx += state.measAx;
        sumAy += state.measAy;
        sumAz += state.measAz;
        sumGz += state.measGz;
        maxAbsBiasA = std::max(maxAbsBiasA, std::abs(state.biasAx));
        maxAbsBiasG = std::max(maxAbsBiasG, std::abs(state.biasGz));
    }

    // Both the white noise and the Gauss-Markov bias are zero-mean, so the
    // sample means must converge back onto the ground truth. Two independent
    // terms set the tolerance:
    //
    //  - white noise: the sample mean of the discretized noise has standard
    //    deviation density / sqrt(T), where T is the total integration time;
    //  - Gauss-Markov bias: its time-average does NOT average out nearly as
    //    fast, because the process is correlated over tau. For T >> tau the
    //    sample mean has standard deviation sigma_b * sqrt(2 * tau / T). With
    //    the accelerometer's tau of 300 s this term dominates outright, which
    //    is exactly why it cannot be left out of the bound.
    //
    // Each term is taken at 5 sigma so the test is not flaky across RNG runs.
    const double totalTimeS = n * dt;
    double accelTol = 5.0 * params.accelNoiseDensity / std::sqrt(totalTimeS) +
                      5.0 * params.accelBiasInstability *
                          std::sqrt(2.0 * params.accelBiasTimeConstantS / totalTimeS);
    double gyroTol = 5.0 * params.gyroNoiseDensityRadS / std::sqrt(totalTimeS) +
                     5.0 * params.GyroBiasInstabilityRadS() *
                         std::sqrt(2.0 * params.gyroBiasTimeConstantS / totalTimeS);
    NS_TEST_ASSERT_MSG_EQ_TOL(sumAx / n, trueAx, accelTol, "Accel x mean drifted off ground truth");
    NS_TEST_ASSERT_MSG_EQ_TOL(sumAy / n, trueAy, accelTol, "Accel y mean drifted off ground truth");
    NS_TEST_ASSERT_MSG_EQ_TOL(sumAz / n, trueAz, accelTol, "Accel z mean drifted off ground truth");
    NS_TEST_ASSERT_MSG_EQ_TOL(sumGz / n, trueOmegaZ, gyroTol, "Gyro z mean drifted off ground truth");

    // A first-order Gauss-Markov process driven to a stationary standard
    // deviation equal to the bias instability should not wander far beyond a
    // handful of sigma over a 10-minute run.
    NS_TEST_ASSERT_MSG_LT(maxAbsBiasA,
                          6.0 * params.accelBiasInstability,
                          "Accel Gauss-Markov bias exceeded its stationary bound");
    NS_TEST_ASSERT_MSG_LT(maxAbsBiasG,
                          6.0 * params.GyroBiasInstabilityRadS(),
                          "Gyro Gauss-Markov bias exceeded its stationary bound");
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests the step detector (Steps 1-2) in both directions: that a clean
 * synthetic gait signal with a known cadence yields approximately the right
 * step count, and that a non-gait signal yields (near) none. The negative case
 * directly exercises the strap-bounce concern flagged in the design.
 */
class StepDetectorSubmoduleTest : public TestCase
{
  public:
    StepDetectorSubmoduleTest();           //!< Default constructor
    ~StepDetectorSubmoduleTest() override; //!< Destructor

  private:
    void DoRun() override;
};

StepDetectorSubmoduleTest::StepDetectorSubmoduleTest()
    : TestCase("Verify step detection on synthetic gait and rejection of non-gait signals")
{
}

StepDetectorSubmoduleTest::~StepDetectorSubmoduleTest()
{
}

void
StepDetectorSubmoduleTest::DoRun()
{
    ShsEngineParams params;
    const double dt = 1.0 / params.sensor.sampleRateHz;
    const double duration = 60.0;
    const double cadenceHz = 1.0;
    const auto ticks = static_cast<uint32_t>(duration / dt);

    // Positive case: clean synthetic gait.
    {
        StepDetectorSubmodule detector(params.stepDetector, params.sensor.sampleRateHz);
        uint32_t detected = 0;
        std::vector<double> stepTimes;
        for (uint32_t i = 0; i < ticks; ++i)
        {
            double t = i * dt;
            double vert = SyntheticVerticalAccel(t, cadenceHz, 6.0);
            for (const auto& s : detector.Step(t, vert, 0.0))
            {
                ++detected;
                stepTimes.push_back(s.timestamp);
            }
        }

        // The detector needs one full 5 s window before it can emit anything,
        // so the achievable count is bounded by (duration - window) * cadence.
        auto expected = static_cast<uint32_t>((duration - params.stepDetector.windowLengthS) *
                                              cadenceHz);
        NS_TEST_ASSERT_MSG_GT(detected,
                              static_cast<uint32_t>(0.8 * expected),
                              "Detected far fewer steps than the known synthetic cadence");
        NS_TEST_ASSERT_MSG_LT(detected,
                              static_cast<uint32_t>(1.2 * expected + 2),
                              "Detected far more steps than the known synthetic cadence");

        // Successive step timestamps must respect the physical inter-step floor.
        for (size_t i = 1; i < stepTimes.size(); ++i)
        {
            NS_TEST_ASSERT_MSG_GT_OR_EQ(stepTimes[i] - stepTimes[i - 1],
                                        0.25,
                                        "Two steps emitted closer than the inter-step floor");
        }
    }

    // Negative case, part 1: a perfectly flat signal has no rhythmic content
    // at all, so nothing may be classified as gait.
    {
        StepDetectorSubmodule detector(params.stepDetector, params.sensor.sampleRateHz);
        uint32_t detected = 0;
        for (uint32_t i = 0; i < ticks; ++i)
        {
            detected += detector.Step(i * dt, 0.0, 0.0).size();
        }
        NS_TEST_ASSERT_MSG_EQ(detected, 0, "Steps detected in a perfectly flat signal");
    }

    // Negative case, part 2: a stationary wearer, i.e. nothing but the
    // modelled MPU-6050 noise floor on the vertical channel. A wearer who is
    // standing still must produce no steps at all.
    //
    // This previously stood as a characterization test pinning a known defect:
    // peakProminenceThreshold is an ABSOLUTE threshold on an UNNORMALIZED
    // correlation sum, and correlating ~50 samples of noise with standard
    // deviation 0.039 m/s^2 against the template accumulates to roughly 0.33,
    // several times the 0.05 threshold. The detector therefore fired on the
    // noise floor at roughly the rate it fires on real walking (~215 steps per
    // 120 s, ~95 m of phantom displacement).
    //
    // StepDetectorParams::walkingRmsThresholdMps2 now rejects quiet windows on
    // band-passed signal energy before the matched filter runs, which closes
    // this. The assertion is correspondingly tightened to zero.
    {
        StepDetectorSubmodule detector(params.stepDetector, params.sensor.sampleRateHz);
        auto rv = CreateObject<NormalRandomVariable>();
        double sigma = params.sensor.accelNoiseDensity / std::sqrt(dt);
        uint32_t detected = 0;
        for (uint32_t i = 0; i < ticks; ++i)
        {
            double noise = rv->GetValue(0.0, sigma * sigma, 6.0 * sigma);
            detected += detector.Step(i * dt, noise, 0.0).size();
        }
        NS_TEST_ASSERT_MSG_EQ(detected,
                              0,
                              "Steps detected from the stationary sensor noise floor alone");
    }

    // Negative case, part 3: motion far too faint to be walking must also be
    // rejected. This guards the gate from being weakened to the point where it
    // only catches a perfectly silent input.
    {
        StepDetectorSubmodule detector(params.stepDetector, params.sensor.sampleRateHz);
        auto rv = CreateObject<NormalRandomVariable>();
        double sigma = params.sensor.accelNoiseDensity / std::sqrt(dt);
        uint32_t detected = 0;
        for (uint32_t i = 0; i < ticks; ++i)
        {
            double t = i * dt;
            double faint = SyntheticVerticalAccel(t, 2.0, 0.1);
            double noise = rv->GetValue(0.0, sigma * sigma, 6.0 * sigma);
            detected += detector.Step(t, faint + noise, 0.0).size();
        }
        NS_TEST_ASSERT_MSG_EQ(detected, 0, "Steps detected from sub-walking-amplitude motion");
    }

    // Positive case, part 2: the gate must not cost real detections at the low
    // end of genuine walking. Light walking has to still be picked up.
    {
        StepDetectorSubmodule detector(params.stepDetector, params.sensor.sampleRateHz);
        auto rv = CreateObject<NormalRandomVariable>();
        double sigma = params.sensor.accelNoiseDensity / std::sqrt(dt);
        uint32_t detected = 0;
        for (uint32_t i = 0; i < ticks; ++i)
        {
            double t = i * dt;
            double vert = SyntheticVerticalAccel(t, cadenceHz, 2.0);
            double noise = rv->GetValue(0.0, sigma * sigma, 6.0 * sigma);
            detected += detector.Step(t, vert + noise, 0.0).size();
        }
        auto expected =
            static_cast<uint32_t>((duration - params.stepDetector.windowLengthS) * cadenceHz);
        NS_TEST_ASSERT_MSG_GT(detected,
                              static_cast<uint32_t>(0.8 * expected),
                              "Walking-energy gate suppressed genuine light walking");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests the Weinberg step-length model (Step 3): the formula itself, the use of
 * the fallback k_w before calibration, convergence of the online calibration to
 * the value implied by a known bracketed distance, and rejection of SUSPECT
 * fixes.
 */
class StepLengthSubmoduleTest : public TestCase
{
  public:
    StepLengthSubmoduleTest();           //!< Default constructor
    ~StepLengthSubmoduleTest() override; //!< Destructor

  private:
    void DoRun() override;
};

StepLengthSubmoduleTest::StepLengthSubmoduleTest()
    : TestCase("Verify the Weinberg step-length formula and its online k_w calibration")
{
}

StepLengthSubmoduleTest::~StepLengthSubmoduleTest()
{
}

void
StepLengthSubmoduleTest::DoRun()
{
    StepLengthParams params;
    StepLengthSubmodule stepLength(params);

    // Before any calibration the fallback k_w must be in force, and the
    // formula must be exactly s_w = k_w * (a_max - a_min)^(1/4).
    NS_TEST_ASSERT_MSG_EQ(stepLength.HasBeenCalibrated(),
                          false,
                          "Reported as calibrated before any fix arrived");
    NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.GetCurrentKw(),
                              params.kwFallback,
                              1e-12,
                              "Fallback k_w not in force before calibration");

    StepEvent probe{0.0, 16.0, 0.0}; // (16 - 0)^(1/4) == 2
    NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.EstimateStepLength(probe),
                              2.0 * params.kwFallback,
                              1e-12,
                              "Weinberg formula did not evaluate as expected");

    // A negative (a_max - a_min) must clamp to zero rather than produce NaN.
    StepEvent inverted{0.0, 1.0, 4.0};
    NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.EstimateStepLength(inverted),
                              0.0,
                              1e-12,
                              "Inverted peak/valley pair did not clamp to zero step length");

    // Calibration: 10 identical steps bracketed by two fixes 12 m apart. Each
    // step contributes (16)^(1/4) == 2, so the implied k_w is 12 / (10*2) = 0.6.
    const uint32_t nSteps = 10;
    const double bracketDistance = 12.0;
    const double expectedKw = bracketDistance / (nSteps * 2.0);

    stepLength.OnTrustedFix(TrustedRadioFix{0.0, 0.0, 0.0, true});
    for (uint32_t i = 0; i < nSteps; ++i)
    {
        stepLength.BufferStep(StepEvent{static_cast<double>(i), 16.0, 0.0}, 0.0);
    }
    stepLength.OnTrustedFix(TrustedRadioFix{10.0, bracketDistance, 0.0, true});

    NS_TEST_ASSERT_MSG_EQ(stepLength.HasBeenCalibrated(),
                          true,
                          "First calibration event did not fire");
    NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.GetCurrentKw(),
                              expectedKw,
                              1e-9,
                              "First calibration did not adopt the implied k_w directly");

    // Repeated calibrations that stay geometrically consistent must keep k_w
    // pinned to the same value rather than drifting. Each round covers the same
    // 12 m over the same 10 steps, so every estimate is 0.6 and the EMA has
    // nothing to move toward.
    for (uint32_t round = 0; round < 20; ++round)
    {
        for (uint32_t i = 0; i < nSteps; ++i)
        {
            stepLength.BufferStep(StepEvent{static_cast<double>(i), 16.0, 0.0}, 0.0);
        }
        stepLength.OnTrustedFix(
            TrustedRadioFix{20.0 + round, bracketDistance * (round + 2), 0.0, true});
    }
    NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.GetCurrentKw(),
                              expectedKw,
                              1e-9,
                              "Repeated consistent calibrations did not keep k_w converged");

    // The EMA must actually track a changed step-length regime: halving the
    // distance covered per bracket should pull k_w toward half its value,
    // monotonically and without overshoot.
    const double newExpectedKw = expectedKw / 2.0;
    double previousKw = stepLength.GetCurrentKw();
    for (uint32_t round = 0; round < 40; ++round)
    {
        for (uint32_t i = 0; i < nSteps; ++i)
        {
            stepLength.BufferStep(StepEvent{static_cast<double>(i), 16.0, 0.0}, 0.0);
        }
        // Advance only half as far as before over the same number of steps.
        stepLength.OnTrustedFix(TrustedRadioFix{100.0 + round,
                                                bracketDistance * 21 +
                                                    (bracketDistance / 2.0) * (round + 1),
                                                0.0,
                                                true});
        double kw = stepLength.GetCurrentKw();
        NS_TEST_ASSERT_MSG_LT(kw, previousKw + 1e-12, "k_w did not decrease monotonically");
        NS_TEST_ASSERT_MSG_GT_OR_EQ(kw, newExpectedKw - 1e-12, "k_w overshot the new regime");
        previousKw = kw;
    }
    NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.GetCurrentKw(),
                              newExpectedKw,
                              1e-6,
                              "k_w did not converge on the new step-length regime");

    // A SUSPECT fix must neither calibrate nor disturb the current k_w. This is
    // checked on a fresh instance so the steps it deliberately strands in the
    // buffer cannot contaminate the convergence checks above: the submodule
    // intentionally keeps buffered steps across a SUSPECT fix so that the next
    // CLEAN fix still brackets the whole span since the last good fix.
    {
        StepLengthSubmodule suspectCase(params);
        suspectCase.OnTrustedFix(TrustedRadioFix{0.0, 0.0, 0.0, true});
        for (uint32_t i = 0; i < nSteps; ++i)
        {
            suspectCase.BufferStep(StepEvent{static_cast<double>(i), 16.0, 0.0}, 0.0);
        }
        suspectCase.OnTrustedFix(TrustedRadioFix{10.0, 1000.0, 0.0, false});
        NS_TEST_ASSERT_MSG_EQ(suspectCase.HasBeenCalibrated(),
                              false,
                              "A SUSPECT fix triggered a calibration event");
        NS_TEST_ASSERT_MSG_EQ_TOL(suspectCase.GetCurrentKw(),
                                  params.kwFallback,
                                  1e-12,
                                  "A SUSPECT fix was allowed to change k_w");

        // The stranded steps must still be bracketed by the next CLEAN fix:
        // 10 steps over 12 m implies k_w = 0.6 as before.
        suspectCase.OnTrustedFix(TrustedRadioFix{20.0, bracketDistance, 0.0, true});
        NS_TEST_ASSERT_MSG_EQ_TOL(suspectCase.GetCurrentKw(),
                                  expectedKw,
                                  1e-9,
                                  "Steps stranded by a SUSPECT fix were not bracketed by the next "
                                  "CLEAN fix");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests the heading estimator (Step 4): plain gyro integration, theta_0
 * initialization from the first two trusted fixes, drift correction on a
 * straight-line stretch, and refusal to correct through a real turn.
 */
class HeadingSubmoduleTest : public TestCase
{
  public:
    HeadingSubmoduleTest();           //!< Default constructor
    ~HeadingSubmoduleTest() override; //!< Destructor

  private:
    void DoRun() override;
};

HeadingSubmoduleTest::HeadingSubmoduleTest()
    : TestCase("Verify gyro heading integration, theta_0 initialization and drift correction")
{
}

HeadingSubmoduleTest::~HeadingSubmoduleTest()
{
}

void
HeadingSubmoduleTest::DoRun()
{
    HeadingParams params;

    // Pure integration: a constant 0.1 rad/s yaw rate for 10 s is 1.0 rad.
    {
        HeadingSubmodule heading(params);
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  params.theta0Fallback,
                                  1e-12,
                                  "Initial heading is not the configured fallback");
        for (uint32_t i = 0; i < 1000; ++i)
        {
            heading.Step(0.01, 0.1, 0.0);
        }
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  1.0,
                                  1e-9,
                                  "Gyro integration did not accumulate the expected heading");

        // A bias estimate equal to the measured rate must freeze the heading.
        double thetaBefore = heading.GetTheta();
        for (uint32_t i = 0; i < 1000; ++i)
        {
            heading.Step(0.01, 0.1, 0.1);
        }
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  thetaBefore,
                                  1e-9,
                                  "Bias compensation did not cancel the measured yaw rate");
    }

    // theta_0 initialization from the first two trusted fixes: two fixes on the
    // +y axis imply a heading of pi/2.
    {
        HeadingSubmodule heading(params);
        heading.OnTrustedFix(0.0, 0.0, 0.0, true);
        NS_TEST_ASSERT_MSG_EQ(heading.HasBeenInitialized(),
                              false,
                              "Heading reported as initialized after only one fix");
        heading.OnTrustedFix(10.0, 0.0, 30.0, true);
        NS_TEST_ASSERT_MSG_EQ(heading.HasBeenInitialized(),
                              true,
                              "Heading not initialized after the second trusted fix");
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  M_PI / 2.0,
                                  1e-9,
                                  "theta_0 does not match the direction implied by the first two "
                                  "fixes");
    }

    // A SUSPECT fix must be ignored entirely, including for initialization.
    {
        HeadingSubmodule heading(params);
        heading.OnTrustedFix(0.0, 0.0, 0.0, false);
        heading.OnTrustedFix(10.0, 0.0, 30.0, false);
        NS_TEST_ASSERT_MSG_EQ(heading.HasBeenInitialized(),
                              false,
                              "SUSPECT fixes were used to initialize theta_0");
    }

    // Drift correction: after initialization along +x, inject a small amount of
    // drift and confirm that a straight-line fix pair snaps it back.
    {
        HeadingSubmodule heading(params);
        heading.OnTrustedFix(0.0, 0.0, 0.0, true);
        heading.OnTrustedFix(10.0, 30.0, 0.0, true); // theta_0 = 0
        for (uint32_t i = 0; i < 1000; ++i)
        {
            heading.Step(0.01, 0.02, 0.0); // accumulate 0.2 rad of drift
        }
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  0.2,
                                  1e-9,
                                  "Drift did not accumulate as expected");
        heading.OnTrustedFix(20.0, 60.0, 0.0, true); // still travelling along +x
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  0.0,
                                  1e-9,
                                  "Straight-line stretch did not correct accumulated drift");
    }

    // Real turn: a fix pair implying a 90-degree change is far outside the
    // straight-line tolerance, so the heading must be left alone rather than
    // snapped (correcting through a real turn would be wrong, not imprecise).
    {
        HeadingSubmodule heading(params);
        heading.OnTrustedFix(0.0, 0.0, 0.0, true);
        heading.OnTrustedFix(10.0, 30.0, 0.0, true); // theta_0 = 0
        for (uint32_t i = 0; i < 1000; ++i)
        {
            heading.Step(0.01, M_PI / 2.0 / 10.0, 0.0); // genuinely turn 90 degrees
        }
        double thetaAfterTurn = heading.GetTheta();
        heading.OnTrustedFix(20.0, 30.0, 30.0, true); // fixes now imply +y
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  thetaAfterTurn,
                                  1e-9,
                                  "Heading was snap-corrected through a genuine turn");
    }

    // Known deployment orientation: seeding theta_0 directly must take effect
    // immediately and must NOT be overwritten by the later fix-pair derivation,
    // which is only a bootstrap for when the orientation is unknown.
    {
        HeadingSubmodule heading(params);
        const double deployed = 0.35;
        heading.SetInitialHeading(deployed);
        NS_TEST_ASSERT_MSG_EQ(heading.HasBeenInitialized(),
                              true,
                              "Seeding a known deployment heading did not initialize theta_0");
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  deployed,
                                  1e-12,
                                  "Seeded theta_0 is not the value supplied");

        // Two fixes implying +y, i.e. pi/2. That is 1.22 rad away from the
        // seeded heading, well outside the straight-line tolerance, so ongoing
        // drift correction must reject it. If theta still moves to pi/2 the
        // only thing that could have done it is the theta_0 derivation, which
        // must stay disabled once a known orientation has been seeded.
        heading.OnTrustedFix(0.0, 0.0, 0.0, true);
        heading.OnTrustedFix(10.0, 0.0, 30.0, true);
        NS_TEST_ASSERT_MSG_EQ_TOL(heading.GetTheta(),
                                  deployed,
                                  1e-9,
                                  "Fix-pair derivation overwrote a seeded deployment heading");
    }

    // theta_0 derivation must REFUSE a stretch containing a real turn. Deriving
    // across one seeds a direction the wearer never travelled, and the error
    // then persists through every subsequent integration.
    {
        HeadingSubmodule heading(params);
        heading.OnTrustedFix(0.0, 0.0, 0.0, true);
        for (uint32_t i = 0; i < 1000; ++i)
        {
            heading.Step(0.01, M_PI / 2.0 / 10.0, 0.0); // turn 90 degrees between fixes
        }
        heading.OnTrustedFix(10.0, 30.0, 30.0, true);
        NS_TEST_ASSERT_MSG_EQ(heading.HasBeenInitialized(),
                              false,
                              "theta_0 was derived across a stretch containing a 90 degree turn");

        // The candidate pair must slide forward, so a later straight stretch
        // can still initialize rather than the node being stuck forever.
        heading.OnTrustedFix(20.0, 60.0, 30.0, true); // straight along +x from the last fix
        NS_TEST_ASSERT_MSG_EQ(heading.HasBeenInitialized(),
                              true,
                              "theta_0 never initialized from a later straight stretch");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests the position update (Step 5), principally the angle convention: theta=0
 * must move purely along +x and theta=pi/2 purely along +y.
 */
class PositionUpdateSubmoduleTest : public TestCase
{
  public:
    PositionUpdateSubmoduleTest();           //!< Default constructor
    ~PositionUpdateSubmoduleTest() override; //!< Destructor

  private:
    void DoRun() override;
};

PositionUpdateSubmoduleTest::PositionUpdateSubmoduleTest()
    : TestCase("Verify the SHS position update formula and its angle convention")
{
}

PositionUpdateSubmoduleTest::~PositionUpdateSubmoduleTest()
{
}

void
PositionUpdateSubmoduleTest::DoRun()
{
    PositionUpdateSubmodule position;
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetX(), 0.0, 1e-12, "Initial x is not zero");
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetY(), 0.0, 1e-12, "Initial y is not zero");

    position.ApplyStep(1.0, 0.0);
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetX(), 1.0, 1e-12, "theta=0 did not move purely along +x");
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetY(), 0.0, 1e-12, "theta=0 introduced spurious y motion");

    position.ApplyStep(1.0, M_PI / 2.0);
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetX(),
                              1.0,
                              1e-12,
                              "theta=pi/2 introduced spurious x motion");
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetY(),
                              1.0,
                              1e-12,
                              "theta=pi/2 did not move purely along +y");

    // An explicit fix snap must override the dead-reckoned position.
    position.SetPosition(-5.0, 7.5);
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetX(), -5.0, 1e-12, "SetPosition did not set x");
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetY(), 7.5, 1e-12, "SetPosition did not set y");

    // Walking a closed square must return to the starting point.
    position.SetPosition(0.0, 0.0);
    for (uint32_t i = 0; i < 4; ++i)
    {
        position.ApplyStep(3.0, i * M_PI / 2.0);
    }
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetX(), 0.0, 1e-9, "Closed square did not return x to start");
    NS_TEST_ASSERT_MSG_EQ_TOL(position.GetY(), 0.0, 1e-9, "Closed square did not return y to start");
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * End-to-end test of the whole engine on a synthetic straight-line walk with
 * periodic noisy radio fixes: confirms that steps are detected, that k_w
 * calibrates away from its fallback, that theta_0 initializes, and that the
 * final position error stays within a loose regression bound.
 */
class ShsEngineEndToEndTest : public TestCase
{
  public:
    ShsEngineEndToEndTest();           //!< Default constructor
    ~ShsEngineEndToEndTest() override; //!< Destructor

  private:
    void DoRun() override;
};

ShsEngineEndToEndTest::ShsEngineEndToEndTest()
    : TestCase("Verify the full SHS pipeline end to end on a synthetic walk")
{
}

ShsEngineEndToEndTest::~ShsEngineEndToEndTest()
{
}

void
ShsEngineEndToEndTest::DoRun()
{
    ShsEngineParams params;
    auto rv = CreateObject<NormalRandomVariable>();
    ShsEngine engine(params, rv);

    auto fixRv = CreateObject<NormalRandomVariable>();

    const double dt = 1.0 / params.sensor.sampleRateHz;
    const double duration = 120.0;
    const double cadenceHz = 1.0;
    const double speed = 1.2;         // m/s
    const double trueHeading = 0.35;  // rad
    const double fixIntervalS = 10.0;
    const double fixNoiseSigmaM = 3.0;

    // The field node is deployed from a known position in a known orientation,
    // so seed both. Without this the heading starts on its fallback, 0.35 rad
    // away from the truth, and that error is carried for the whole run -- which
    // used to be masked by the position snapping that is now off by default.
    engine.SetInitialState(0.0, 0.0, trueHeading);

    double trueX = 0.0;
    double trueY = 0.0;
    double lastFixTime = -1e9;
    const auto ticks = static_cast<uint32_t>(duration / dt);

    for (uint32_t i = 0; i < ticks; ++i)
    {
        double t = i * dt;

        // Ground truth: constant-velocity straight-line walk, with a synthetic
        // gait-cycle bounce on the channel the engine treats as vertical and a
        // matching anterior-posterior component on the other, since the walking
        // gate now draws on both channels.
        trueX += speed * std::cos(trueHeading) * dt;
        trueY += speed * std::sin(trueHeading) * dt;
        double verticalBounce = SyntheticVerticalAccel(t, cadenceHz, 6.0);
        double cyclePhase = std::fmod(t, 1.0 / cadenceHz) * cadenceHz;
        double apAccel = 3.0 * std::sin(2.0 * M_PI * cyclePhase);

        // Per the engine's placeholder axis assignment, trueAy is the vertical
        // channel and trueAx the anterior-posterior one.
        engine.Tick(t, dt, apAccel, verticalBounce, 0.0, 0.0);

        if (t - lastFixTime >= fixIntervalS)
        {
            lastFixTime = t;
            double fixX = trueX + fixRv->GetValue(0.0, fixNoiseSigmaM * fixNoiseSigmaM, 4.0 * fixNoiseSigmaM);
            double fixY = trueY + fixRv->GetValue(0.0, fixNoiseSigmaM * fixNoiseSigmaM, 4.0 * fixNoiseSigmaM);
            engine.OnTrustedFix(t, fixX, fixY, true);
        }
    }

    // Steps must be detected at roughly the synthetic cadence, allowing for the
    // 5 s window warm-up at the start.
    auto expectedSteps =
        static_cast<uint32_t>((duration - params.stepDetector.windowLengthS) * cadenceHz);
    NS_TEST_ASSERT_MSG_GT(engine.GetDetectedStepCount(),
                          static_cast<uint32_t>(0.8 * expectedSteps),
                          "End-to-end run detected far fewer steps than expected");
    NS_TEST_ASSERT_MSG_LT(engine.GetDetectedStepCount(),
                          static_cast<uint32_t>(1.2 * expectedSteps + 2),
                          "End-to-end run detected far more steps than expected");

    // k_w must have calibrated away from its provisional fallback.
    NS_TEST_ASSERT_MSG_GT(std::abs(engine.GetCurrentKw() - params.stepLength.kwFallback),
                          1e-6,
                          "k_w never moved off its fallback value");

    // Position error is bounded loosely: this is a regression guard on the
    // pipeline staying connected, not an accuracy claim. The design flags the
    // RF-aided heading correction as an unvalidated adaptation, so the residual
    // heading error here is expected rather than a defect.
    double errX = engine.GetX() - trueX;
    double errY = engine.GetY() - trueY;
    double finalErr = std::sqrt(errX * errX + errY * errY);
    double trueDistance = std::sqrt(trueX * trueX + trueY * trueY);
    NS_TEST_ASSERT_MSG_LT(finalErr,
                          0.25 * trueDistance,
                          "End-to-end position error exceeded the regression bound");
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Drives the whole ShsEngine around a CLOSED SQUARE LOOP, so that heading
 * integration, step detection and position dead reckoning are all exercised
 * THROUGH GENUINE TURNS with the sensor noise and bias model live.
 *
 * HeadingSubmoduleTest already covers turning at the submodule level, but only
 * with clean synthetic yaw rates fed straight into HeadingSubmodule::Step. This
 * case closes the end-to-end gap: nothing before it ever routed a non-zero
 * trueOmegaZ through ImuSensorSubmodule and out into a position estimate.
 */
class ShsEngineTurningTest : public TestCase
{
  public:
    ShsEngineTurningTest();           //!< Default constructor
    ~ShsEngineTurningTest() override; //!< Destructor

  private:
    void DoRun() override;
};

ShsEngineTurningTest::ShsEngineTurningTest()
    : TestCase("Verify the SHS pipeline end-to-end around a closed loop with real turns")
{
}

ShsEngineTurningTest::~ShsEngineTurningTest()
{
}

void
ShsEngineTurningTest::DoRun()
{
    ShsEngineParams params;
    const double dt = 1.0 / params.sensor.sampleRateHz;
    const double speed = 1.4;
    const double cadenceHz = 2.0;
    const double sideM = 50.0;
    const double turnRate = 45.0 * M_PI / 180.0;
    const double fixIntervalS = 10.0;

    // Four sides plus four 90 degree turns, walked at constant speed.
    const double duration = 4.0 * (sideM / speed) + 4.0 * ((M_PI / 2.0) / turnRate);
    const auto ticks = static_cast<uint32_t>(duration / dt);

    auto sensorRv = CreateObject<NormalRandomVariable>();
    ShsEngine engine(params, sensorRv);

    double trueX = 0.0;
    double trueY = 0.0;
    double curHeading = 0.0;
    double pathLength = 0.0;
    double segmentDistance = 0.0;
    double turnAccumulated = 0.0;
    uint32_t turnsRemaining = 4;
    bool turning = false;
    double lastFixTime = -1e9;
    bool sawNonZeroYaw = false;

    for (uint32_t i = 0; i < ticks; ++i)
    {
        double t = i * dt;

        double trueOmegaZ = 0.0;
        if (turning)
        {
            trueOmegaZ = turnRate;
            turnAccumulated += trueOmegaZ * dt;
            if (turnAccumulated >= M_PI / 2.0)
            {
                trueOmegaZ -= (turnAccumulated - M_PI / 2.0) / dt;
                turnAccumulated = 0.0;
                turning = false;
                segmentDistance = 0.0;
            }
        }
        else if (turnsRemaining > 0 && segmentDistance >= sideM)
        {
            --turnsRemaining;
            turning = true;
            trueOmegaZ = turnRate;
            turnAccumulated += trueOmegaZ * dt;
        }

        if (std::abs(trueOmegaZ) > 1e-9)
        {
            sawNonZeroYaw = true;
        }

        curHeading += trueOmegaZ * dt;
        trueX += speed * std::cos(curHeading) * dt;
        trueY += speed * std::sin(curHeading) * dt;
        pathLength += speed * dt;
        if (!turning)
        {
            segmentDistance += speed * dt;
        }

        engine.Tick(t, dt, 0.0, SyntheticVerticalAccel(t, cadenceHz, 6.0), 0.0, trueOmegaZ);

        if (t - lastFixTime >= fixIntervalS)
        {
            lastFixTime = t;
            engine.OnTrustedFix(t, trueX, trueY, true);
        }
    }

    // The route must actually have turned, otherwise this case is silently
    // testing the same straight line every other case already covers.
    NS_TEST_ASSERT_MSG_EQ(sawNonZeroYaw, true, "Route never applied a non-zero yaw rate");

    // Four 90 degree turns bring the walker back to the start.
    double closure = std::sqrt(trueX * trueX + trueY * trueY);
    NS_TEST_ASSERT_MSG_LT(closure, 0.5, "Ground-truth square loop did not close on itself");

    // Steps must still be detected while turning, not just on the straight
    // sections: the walking-energy gate must not reject a turning window.
    auto expectedSteps = static_cast<uint32_t>((duration - params.stepDetector.windowLengthS) *
                                               cadenceHz);
    NS_TEST_ASSERT_MSG_GT(engine.GetDetectedStepCount(),
                          static_cast<uint32_t>(0.8 * expectedSteps),
                          "Step detection fell away on a route containing turns");

    // Return position error, the metric the pedestrian-dead-reckoning
    // literature reports for closed-loop routes. Bounded loosely as a
    // regression guard, not as an accuracy claim.
    //
    // NOTE: RPE on a CLOSED loop is blind to a constant heading offset, since
    // rotating a closed route about its start point still ends at that point.
    // It bounds accumulated drift, not absolute heading accuracy.
    double errX = engine.GetX() - trueX;
    double errY = engine.GetY() - trueY;
    double rpe = std::sqrt(errX * errX + errY * errY) / pathLength;
    NS_TEST_ASSERT_MSG_LT(rpe, 0.05, "Return position error exceeded the regression bound");
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Verifies that online k_w calibration is correct across a stretch containing a
 * genuine turn, i.e. that it sums the bracketed steps as VECTORS.
 *
 * Dividing the chord between two fixes by the scalar sum of step features
 * silently assumes every step pointed the same way. It does not when the wearer
 * turns inside the interval: the chord is shorter than the path walked, so k_w
 * comes out low and every subsequent step is estimated short. Measured on the
 * square loop used by ShsEngineTurningTest, against a straight route of
 * identical length whose k_w settles at ~0.446, the scalar form gave:
 * \verbatim
 *   fix interval 10 s : k_w 0.434  (-2.7%)
 *   fix interval 30 s : k_w 0.400  (-10.4%)
 *   fix interval 60 s : k_w 0.315  (-29.4%), final position error 36 m vs 1.4 m
 * \endverbatim
 *
 * The vector form recovers k_w exactly regardless of path shape, which is what
 * the first case below asserts. The second case covers the degenerate end: a
 * stretch that nearly returns to its start carries almost no scale information,
 * and must be skipped rather than calibrated on.
 */
class StepLengthTurningCalibrationTest : public TestCase
{
  public:
    StepLengthTurningCalibrationTest();           //!< Default constructor
    ~StepLengthTurningCalibrationTest() override; //!< Destructor

  private:
    void DoRun() override;
};

StepLengthTurningCalibrationTest::StepLengthTurningCalibrationTest()
    : TestCase("Verify online k_w calibration across a stretch containing a real turn")
{
}

StepLengthTurningCalibrationTest::~StepLengthTurningCalibrationTest()
{
}

void
StepLengthTurningCalibrationTest::DoRun()
{
    StepLengthParams params;

    // Feed steps whose quartic features are known, so the expected k_w is exact.
    const double aMax = 6.0;
    const double aMin = 0.0;
    const double q = std::pow(aMax - aMin, 0.25);

    // Right-angle stretch: 10 steps along +x then 10 along +y. The wearer walks
    // 20 steps of path but ends only sqrt(2) * 10 steps from where they began,
    // so the chord understates the path by a factor of sqrt(2)/2.
    {
        StepLengthSubmodule stepLength(params);
        const uint32_t perLeg = 10;
        const double legM = 30.0;
        const double chordM = std::sqrt(2.0) * legM;

        stepLength.OnTrustedFix(TrustedRadioFix{0.0, 0.0, 0.0, true});
        for (uint32_t i = 0; i < perLeg; ++i)
        {
            stepLength.BufferStep(StepEvent{0.1 * i, aMax, aMin}, 0.0);
        }
        for (uint32_t i = 0; i < perLeg; ++i)
        {
            stepLength.BufferStep(StepEvent{0.1 * (perLeg + i), aMax, aMin}, M_PI / 2.0);
        }
        stepLength.OnTrustedFix(TrustedRadioFix{10.0, legM, legM, true});

        // Vector sum magnitude is q * sqrt(10^2 + 10^2), so k_w = chord / that.
        // This is identically the value implied by the true path length,
        // 2 * legM / (20 * q) -- the whole point of the vector form.
        double kwFromPath = (2.0 * legM) / (2.0 * perLeg * q);
        NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.GetCurrentKw(),
                                  kwFromPath,
                                  1e-9,
                                  "k_w across a right-angle turn does not match the value implied "
                                  "by the distance actually walked");
    }

    // A straight stretch must still calibrate exactly as before: the vector form
    // has to reduce to the scalar form when every step shares a heading.
    {
        StepLengthSubmodule stepLength(params);
        const uint32_t steps = 20;
        const double distanceM = 14.0;

        stepLength.OnTrustedFix(TrustedRadioFix{0.0, 0.0, 0.0, true});
        for (uint32_t i = 0; i < steps; ++i)
        {
            stepLength.BufferStep(StepEvent{0.1 * i, aMax, aMin}, 0.0);
        }
        stepLength.OnTrustedFix(TrustedRadioFix{10.0, distanceM, 0.0, true});

        NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.GetCurrentKw(),
                                  distanceM / (steps * q),
                                  1e-9,
                                  "Vector-form calibration does not reduce to the scalar form on a "
                                  "straight stretch");
    }

    // Degenerate stretch: the wearer walks out and back, returning to the start.
    // The chord is ~0 while the path is long, so no scale information exists.
    // Calibration must be skipped, leaving k_w at its fallback.
    {
        StepLengthSubmodule stepLength(params);
        const uint32_t perLeg = 10;

        stepLength.OnTrustedFix(TrustedRadioFix{0.0, 0.0, 0.0, true});
        for (uint32_t i = 0; i < perLeg; ++i)
        {
            stepLength.BufferStep(StepEvent{0.1 * i, aMax, aMin}, 0.0);
        }
        for (uint32_t i = 0; i < perLeg; ++i)
        {
            stepLength.BufferStep(StepEvent{0.1 * (perLeg + i), aMax, aMin}, M_PI);
        }
        stepLength.OnTrustedFix(TrustedRadioFix{10.0, 0.0, 0.0, true});

        NS_TEST_ASSERT_MSG_EQ(stepLength.HasBeenCalibrated(),
                              false,
                              "Calibrated on an out-and-back stretch carrying no scale information");
        NS_TEST_ASSERT_MSG_EQ_TOL(stepLength.GetCurrentKw(),
                                  params.kwFallback,
                                  1e-12,
                                  "k_w moved off its fallback despite calibration being skipped");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests the ZARU gyroscope bias observer: that it converges on a known bias
 * across stationary dwells, and that it refuses every condition under which the
 * zero-rate assumption does not hold.
 */
class GyroBiasObserverTest : public TestCase
{
  public:
    GyroBiasObserverTest();           //!< Default constructor
    ~GyroBiasObserverTest() override; //!< Destructor

  private:
    void DoRun() override;
};

GyroBiasObserverTest::GyroBiasObserverTest()
    : TestCase("Verify ZARU gyro bias estimation and its refusal conditions")
{
}

GyroBiasObserverTest::~GyroBiasObserverTest()
{
}

void
GyroBiasObserverTest::DoRun()
{
    GyroBiasObserverParams params;
    const double dt = 0.01;
    const double knownBias = 3.0e-4; // rad/s, well inside the stillness gate

    // Stationary, noiseless: the observer must converge on the bias itself.
    {
        GyroBiasObserver observer(params);
        NS_TEST_ASSERT_MSG_EQ(observer.HasEstimate(), false, "Reported an estimate before any dwell");
        NS_TEST_ASSERT_MSG_EQ_TOL(observer.GetBiasEstimate(),
                                  0.0,
                                  1e-15,
                                  "Initial bias estimate is not exactly zero");

        // 60 s of stillness: many dwells, so the EMA has fully settled.
        for (uint32_t i = 0; i < 6000; ++i)
        {
            observer.Update(dt, knownBias, false);
        }
        NS_TEST_ASSERT_MSG_EQ(observer.HasEstimate(), true, "No estimate after prolonged stillness");
        NS_TEST_ASSERT_MSG_GT(observer.GetUpdateCount(), 1, "Only one dwell completed in 60 s");
        NS_TEST_ASSERT_MSG_EQ_TOL(observer.GetBiasEstimate(),
                                  knownBias,
                                  1e-9,
                                  "Observer did not converge on the known bias");
    }

    // Walking: the true yaw rate is unknown, so nothing may be accumulated no
    // matter how long it goes on.
    {
        GyroBiasObserver observer(params);
        for (uint32_t i = 0; i < 6000; ++i)
        {
            observer.Update(dt, knownBias, true);
        }
        NS_TEST_ASSERT_MSG_EQ(observer.HasEstimate(), false, "Estimated a bias while walking");
    }

    // Stationary but turning on the spot: the zero-rate assumption fails, so
    // this must be rejected too. "Not walking" is not "not rotating".
    {
        GyroBiasObserver observer(params);
        double turnRate = 2.0 * params.stillnessRateGateRadS;
        for (uint32_t i = 0; i < 6000; ++i)
        {
            observer.Update(dt, turnRate, false);
        }
        NS_TEST_ASSERT_MSG_EQ(observer.HasEstimate(),
                              false,
                              "Estimated a bias while stationary but rotating");
    }

    // A dwell interrupted before it completes must not produce an estimate,
    // and must not carry its partial accumulation across the interruption.
    {
        GyroBiasObserver observer(params);
        auto ticksJustUnderDwell = static_cast<uint32_t>((params.dwellS / dt) - 10);
        for (uint32_t cycle = 0; cycle < 5; ++cycle)
        {
            for (uint32_t i = 0; i < ticksJustUnderDwell; ++i)
            {
                observer.Update(dt, knownBias, false);
            }
            observer.Update(dt, knownBias, true); // interrupted by walking
        }
        NS_TEST_ASSERT_MSG_EQ(observer.HasEstimate(),
                              false,
                              "Adopted an estimate from repeatedly interrupted partial dwells");
    }

    // Disabled: the estimate must stay exactly zero so heading integration is
    // unaided, which is the control case for measuring the observer's value.
    {
        GyroBiasObserverParams off = params;
        off.enabled = false;
        GyroBiasObserver observer(off);
        for (uint32_t i = 0; i < 6000; ++i)
        {
            observer.Update(dt, knownBias, false);
        }
        NS_TEST_ASSERT_MSG_EQ(observer.HasEstimate(), false, "Disabled observer produced an estimate");
        NS_TEST_ASSERT_MSG_EQ_TOL(observer.GetBiasEstimate(),
                                  0.0,
                                  1e-15,
                                  "Disabled observer returned a non-zero bias");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests that the accelerometer and gyroscope saturate at their configured
 * full-scale ranges (AFS_SEL / FS_SEL), which a real MPU-6050 cannot exceed.
 */
class SensorSaturationTest : public TestCase
{
  public:
    SensorSaturationTest();           //!< Default constructor
    ~SensorSaturationTest() override; //!< Destructor

  private:
    void DoRun() override;
};

SensorSaturationTest::SensorSaturationTest()
    : TestCase("Verify accelerometer and gyroscope saturate at their full-scale ranges")
{
}

SensorSaturationTest::~SensorSaturationTest()
{
}

void
SensorSaturationTest::DoRun()
{
    ImuSensorParams params;
    auto rv = CreateObject<NormalRandomVariable>();
    ImuSensorSubmodule sensor(params, rv);
    ImuSensorState state;
    const double dt = 1.0 / params.sampleRateHz;

    double accelLimit = params.AccelFullScaleMps2();
    double gyroLimit = params.GyroFullScaleRadS();

    // +/-6g at 9.80665 m/s^2 per g.
    NS_TEST_ASSERT_MSG_EQ_TOL(accelLimit, 58.8399, 1e-3, "Accel full scale is not 6 g");

    // Drive far beyond both ranges, in both directions.
    for (uint32_t i = 0; i < 100; ++i)
    {
        sensor.Step(state, dt, 10.0 * accelLimit, -10.0 * accelLimit, 10.0 * accelLimit,
                    10.0 * gyroLimit);
        NS_TEST_ASSERT_MSG_EQ_TOL(state.measAx, accelLimit, 1e-9, "Accel x exceeded full scale");
        NS_TEST_ASSERT_MSG_EQ_TOL(state.measAy, -accelLimit, 1e-9, "Accel y exceeded full scale");
        NS_TEST_ASSERT_MSG_EQ_TOL(state.measAz, accelLimit, 1e-9, "Accel z exceeded full scale");
        NS_TEST_ASSERT_MSG_EQ_TOL(state.measGz, gyroLimit, 1e-9, "Gyro z exceeded full scale");
    }

    // Ordinary walking must be nowhere near saturation, otherwise the range
    // choice would be corrupting the very signal it is sized for.
    for (uint32_t i = 0; i < 1000; ++i)
    {
        sensor.Step(state, dt, 3.0, 6.0, 1.1, 0.8);
        NS_TEST_ASSERT_MSG_LT(std::abs(state.measAy), accelLimit, "Walking saturated the accel");
        NS_TEST_ASSERT_MSG_LT(std::abs(state.measGz), gyroLimit, "Turning saturated the gyro");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Pins the pipeline's response to the lateral accelerometer axis, which is where
 * the turn-synchronous signals live: centripetal acceleration a_c = v * omega_z,
 * and the gravity component that trunk lean rotates out of vertical,
 * g * sin(theta_lean).
 *
 * Two properties are asserted, and together they are the whole reason the turn
 * model was built:
 *
 *  1. With the CORRECT axis wiring the lateral axis is not fed to the step
 *     detector at all, so injecting a large lateral signal must leave step
 *     detection bit-identical. Turning is structurally free for SHS -- not
 *     because turning is easy, but because the turn signals land on an axis the
 *     detector does not read.
 *
 *  2. With a WRONG axis wiring -- the lateral axis mistaken for vertical, which
 *     is exactly what the unresolved mounting-orientation placeholder risks --
 *     step detection degrades badly. This is the failure mode a two-axis sensor
 *     model was structurally incapable of showing.
 */
class LateralAxisTurnResponseTest : public TestCase
{
  public:
    LateralAxisTurnResponseTest();           //!< Default constructor
    ~LateralAxisTurnResponseTest() override; //!< Destructor

  private:
    void DoRun() override;
};

LateralAxisTurnResponseTest::LateralAxisTurnResponseTest()
    : TestCase("Verify the pipeline ignores the lateral axis, and degrades if it is miswired")
{
}

LateralAxisTurnResponseTest::~LateralAxisTurnResponseTest()
{
}

void
LateralAxisTurnResponseTest::DoRun()
{
    ShsEngineParams params;
    const double dt = 1.0 / params.sensor.sampleRateHz;
    const double duration = 60.0;
    const double cadenceHz = 2.0;
    const auto ticks = static_cast<uint32_t>(duration / dt);
    const double g = 9.80665;

    // Turn-realistic lateral magnitude: centripetal at 1.4 m/s and 45 deg/s is
    // 1.10 m/s^2, plus gravity projected by a 10 degree lean at 1.70 m/s^2.
    const double lateralDuringTurn = 1.10 + g * std::sin(10.0 * M_PI / 180.0);

    // Baseline: no lateral signal at all.
    uint32_t stepsNoLateral = 0;
    {
        RngSeedManager::SetRun(1);
        auto rv = CreateObject<NormalRandomVariable>();
        ShsEngine engine(params, rv);
        for (uint32_t i = 0; i < ticks; ++i)
        {
            double t = i * dt;
            double vert = g + SyntheticVerticalAccel(t, cadenceHz, 6.0);
            engine.Tick(t, dt, 0.0, vert, 0.0, 0.0);
        }
        stepsNoLateral = engine.GetDetectedStepCount();
        NS_TEST_ASSERT_MSG_GT(stepsNoLateral, 100, "Baseline detected implausibly few steps");
    }

    // Correct wiring, large lateral signal present. Step detection must be
    // completely unaffected: the detector never reads this axis.
    {
        RngSeedManager::SetRun(1);
        auto rv = CreateObject<NormalRandomVariable>();
        ShsEngine engine(params, rv);
        for (uint32_t i = 0; i < ticks; ++i)
        {
            double t = i * dt;
            double vert = g + SyntheticVerticalAccel(t, cadenceHz, 6.0);
            engine.Tick(t, dt, 0.0, vert, lateralDuringTurn, 0.0);
        }
        NS_TEST_ASSERT_MSG_EQ(engine.GetDetectedStepCount(),
                              stepsNoLateral,
                              "Lateral-axis signal changed step detection despite not being wired "
                              "to the detector");
    }

    // Wrong wiring: the lateral axis presented as vertical. The detector's
    // primary input becomes a near-constant offset with no gait bounce on it,
    // so detection must collapse. If this ever starts passing, the axis wiring
    // in ShsEngine::Tick has changed and the mounting-orientation flag needs
    // revisiting.
    {
        RngSeedManager::SetRun(1);
        auto rv = CreateObject<NormalRandomVariable>();
        ShsEngine engine(params, rv);
        for (uint32_t i = 0; i < ticks; ++i)
        {
            double t = i * dt;
            double vert = g + SyntheticVerticalAccel(t, cadenceHz, 6.0);
            // Vertical and lateral swapped, as a wrong mounting assumption would.
            engine.Tick(t, dt, 0.0, lateralDuringTurn, vert, 0.0);
        }
        NS_TEST_ASSERT_MSG_LT(engine.GetDetectedStepCount(),
                              stepsNoLateral / 2,
                              "A wrong vertical/lateral axis assignment did not degrade step "
                              "detection, which means this test no longer exercises the failure "
                              "mode it was written for");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Tests yaw-based turn detection against the rule from Bertoli et al.
 * (PMC5591331): a yaw change exceeding 90 degrees over an interval between
 * 0.5 and 10 s.
 */
class TurnDetectorTest : public TestCase
{
  public:
    TurnDetectorTest();           //!< Default constructor
    ~TurnDetectorTest() override; //!< Destructor

  private:
    void DoRun() override;
};

TurnDetectorTest::TurnDetectorTest()
    : TestCase("Verify yaw-based turn detection and its rejection conditions")
{
}

TurnDetectorTest::~TurnDetectorTest()
{
}

void
TurnDetectorTest::DoRun()
{
    TurnDetectorParams params;
    const double dt = 0.01;

    // Straight walking: heading never changes, so nothing may be reported.
    {
        TurnDetector det(params);
        for (uint32_t i = 0; i < 6000; ++i)
        {
            det.Update(i * dt, 0.35);
        }
        NS_TEST_ASSERT_MSG_EQ(det.IsTurning(), false, "Reported a turn on a constant heading");
        NS_TEST_ASSERT_MSG_EQ(det.GetTurnCount(), 0, "Counted a turn on a constant heading");
    }

    // A clean 120 degree turn taken over 3 s, comfortably inside the duration
    // window and above the magnitude threshold, must be detected exactly once.
    {
        TurnDetector det(params);
        double theta = 0.0;
        double rate = (120.0 * M_PI / 180.0) / 3.0;
        uint32_t i = 0;
        for (; i < 300; ++i) // 3 s straight first
        {
            det.Update(i * dt, theta);
        }
        for (uint32_t k = 0; k < 300; ++k, ++i) // 3 s turning
        {
            theta += rate * dt;
            det.Update(i * dt, theta);
        }
        NS_TEST_ASSERT_MSG_EQ(det.IsTurning(), true, "Missed a clean 120 degree turn");
        for (uint32_t k = 0; k < 1500; ++k, ++i) // 15 s straight, turn leaves the window
        {
            det.Update(i * dt, theta);
        }
        NS_TEST_ASSERT_MSG_EQ(det.IsTurning(),
                              false,
                              "Turn state persisted after the turn left the lookback window");
        NS_TEST_ASSERT_MSG_EQ(det.GetTurnCount(),
                              1,
                              "One physical turn was not counted exactly once -- if this reports "
                              "many, the hysteresis in TurnDetector has regressed");
    }

    // A yaw change below the magnitude threshold is not a turn, however long it
    // takes. 45 degrees over 3 s must be rejected.
    {
        TurnDetector det(params);
        double theta = 0.0;
        double rate = (45.0 * M_PI / 180.0) / 3.0;
        for (uint32_t i = 0; i < 300; ++i)
        {
            theta += rate * dt;
            det.Update(i * dt, theta);
        }
        NS_TEST_ASSERT_MSG_EQ(det.IsTurning(), false, "A 45 degree change was reported as a turn");
    }

    // A yaw change spread over longer than maxTurnDurationS is not a turn
    // either: 120 degrees taken over 30 s is course-following, not a turn.
    {
        TurnDetector det(params);
        double theta = 0.0;
        double rate = (120.0 * M_PI / 180.0) / 30.0;
        for (uint32_t i = 0; i < 3000; ++i)
        {
            theta += rate * dt;
            det.Update(i * dt, theta);
        }
        NS_TEST_ASSERT_MSG_EQ(det.IsTurning(),
                              false,
                              "A gradual 120 degree change over 30 s was reported as a turn");
    }
}

/**
 * @ingroup lorawan
 * @ingroup tests
 *
 * Test suite for the backpack-mounted IMU Step-and-Heading System.
 */
class LorawanShsTestSuite : public TestSuite
{
  public:
    LorawanShsTestSuite(); //!< Default constructor
};

LorawanShsTestSuite::LorawanShsTestSuite()
    : TestSuite("lorawan-shs", Type::UNIT)
{
    LogComponentEnable("LorawanShsTestSuite", LOG_LEVEL_INFO);

    AddTestCase(new ImuSensorSubmoduleTest, TestCase::Duration::QUICK);
    AddTestCase(new StepDetectorSubmoduleTest, TestCase::Duration::QUICK);
    AddTestCase(new StepLengthSubmoduleTest, TestCase::Duration::QUICK);
    AddTestCase(new HeadingSubmoduleTest, TestCase::Duration::QUICK);
    AddTestCase(new PositionUpdateSubmoduleTest, TestCase::Duration::QUICK);
    AddTestCase(new ShsEngineEndToEndTest, TestCase::Duration::QUICK);
    AddTestCase(new ShsEngineTurningTest, TestCase::Duration::QUICK);
    AddTestCase(new StepLengthTurningCalibrationTest, TestCase::Duration::QUICK);
    AddTestCase(new GyroBiasObserverTest, TestCase::Duration::QUICK);
    AddTestCase(new SensorSaturationTest, TestCase::Duration::QUICK);
    AddTestCase(new LateralAxisTurnResponseTest, TestCase::Duration::QUICK);
    AddTestCase(new TurnDetectorTest, TestCase::Duration::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static LorawanShsTestSuite lorawanShsTestSuite;
