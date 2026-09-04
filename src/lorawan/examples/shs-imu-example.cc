/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * End-to-end demonstration of the backpack-mounted IMU Step-and-Heading System
 * (SHS), the ns-3 port of the standalone SHS validation harness.
 *
 * Scenario: a field node walks a synthetic straight line at ~1.2 m/s with an
 * MPU-6050 in its backpack. Sensor noise and Gauss-Markov bias are injected per
 * the modelled datasheet/Allan-variance figures, and periodic noisy "trusted
 * radio fixes" arrive every 10 s, feeding both the online k_w calibration
 * (Step 3) and the heading initialization and drift correction (Step 4). The
 * final dead-reckoned position is compared against ground truth.
 *
 * The residual heading error this reports is expected, not a bug: the RF-aided
 * drift-correction pattern is a principled adaptation of a UWB/WiFi-validated
 * method to LoRa, and is flagged in the design as unvalidated at LoRa's
 * accuracy and update-rate profile.
 */

#include "ns3/command-line.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/lora-shs-engine.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"

#include <cmath>
#include <iostream>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("ShsImuExample");

namespace
{

/// Standard gravity, m/s^2.
constexpr double kG = 9.80665;

/**
 * Smoothstep, 3t^2 - 2t^3, clamped to [0, 1] over [edge0, edge1].
 *
 * @param x Input value.
 * @param edge0 Lower edge.
 * @param edge1 Upper edge.
 * @return The smoothstepped value in [0, 1].
 */
double
SmoothStep(double x, double edge0, double edge1)
{
    if (edge1 <= edge0)
    {
        return (x >= edge1) ? 1.0 : 0.0;
    }
    double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

} // namespace

int
main(int argc, char* argv[])
{
    double duration = 120.0;      // s of simulated walking
    double speed = 1.2;           // m/s
    double trueHeading = 0.35;    // rad, an arbitrary non-trivial direction
    double cadenceHz = 1.0;       // steps/s
    double bounceAmplitude = 6.0; // m/s^2, peak vertical gait acceleration
    double apAmplitude = 3.0;     // m/s^2, peak anterior-posterior gait acceleration
    double fixIntervalS = 10.0;   // s between trusted radio fixes
    double fixNoiseSigmaM = 3.0;  // m, 1-sigma radio fix position noise
    double loopSideM = 0.0;       // m, >0 selects the closed square-loop route
    double turnRateDegPerS = 45.0; // deg/s yaw rate used during the loop's turns
    uint32_t loopCount = 1;        // how many times to walk the square loop
    // Staircase route: legs of zigzagSideM joined by ALTERNATING +/-90 degree
    // turns. Unlike the closed loop this keeps turning while still accumulating
    // net displacement, which is the realistic case for a traverse with course
    // changes -- and the case that separates a turning effect from the
    // geometric bound a closed loop imposes.
    double zigzagSideM = 0.0;
    // Random-turn route: turns occur at random INTERVALS and through random
    // ANGLES, rather than on a fixed schedule at a fixed 90 degrees. Distance
    // between turns is exponentially distributed (a Poisson process in
    // distance); turn angle is uniform over +/- randomTurnMaxDeg, either
    // direction. Both distributions are modelling choices, not literature
    // values -- no source was found giving turn-interval or turn-angle
    // distributions for a walking search pattern.
    bool randomTurns = false;
    double randomTurnMeanLegM = 100.0; // mean distance between turns, m
    double randomTurnMaxDeg = 90.0;    // max |turn angle|, deg
    // Turn-injection model. Both magnitudes below are ESTIMATES written for this
    // project, not values taken from any paper -- see the design doc's Flag 11.
    // What the literature supports is that these mechanisms are real and
    // measurable at a trunk mount (Fino et al. 2020 for centripetal
    // acceleration, PMC4325939 for leaning into the turn), NOT their magnitudes.
    double leanMaxDeg = 10.0;      // deg, representative trunk lean -- UNSOURCED ESTIMATE
    double leanRiseS = 0.75;       // s, lean onset/offset time -- UNSOURCED ESTIMATE
    bool turnInjection = true;     // inject centripetal + lean during turns
    bool wrongAxisWiring = false;  // feed lateral as vertical, to quantify a bad axis choice
    bool gyroBiasObserver = true;  // run the ZARU gyro bias observer
    double restEveryS = 0.0;      // s of walking between rest stops; 0 disables rests
    double restForS = 30.0;       // s spent stationary at each rest stop
    bool knownInitialState = true; // node deployed from a known position and orientation
    // ORACLE FLAG -- DEFAULT OFF. When true, the ground-truth heading is injected
    // into the estimator at the end of the calibration walk, which silently erases
    // whatever gyro drift accumulated during that walk. That is not something the
    // real system can do, so it must stay off for any reported result. Retained
    // only to quantify what the injection is worth (measured: 8.9 m over a 1 h
    // traverse, 160.9 -> 169.8 m, n=12 -- i.e. small, and within run-to-run
    // dispersion, but still an oracle).
    bool oracleHeadingAtCalibEnd = false;
    double calibrationWalkM = 0.0; // m, >0 prepends a straight k_w calibration walk
    bool snapPositionToFix = false; // let a clean fix overwrite the dead-reckoned position
    uint32_t rngRun = 1;

    CommandLine cmd(__FILE__);
    cmd.AddValue("duration", "Simulated walking duration, s", duration);
    cmd.AddValue("speed", "Walking speed, m/s", speed);
    cmd.AddValue("trueHeading", "Ground-truth heading, rad", trueHeading);
    cmd.AddValue("cadenceHz", "Step cadence, Hz", cadenceHz);
    cmd.AddValue("bounceAmplitude", "Peak vertical gait acceleration, m/s^2", bounceAmplitude);
    cmd.AddValue("apAmplitude",
                 "Peak anterior-posterior gait acceleration, m/s^2",
                 apAmplitude);
    cmd.AddValue("fixIntervalS", "Interval between trusted radio fixes, s", fixIntervalS);
    cmd.AddValue("fixNoiseSigmaM", "1-sigma radio fix position noise, m", fixNoiseSigmaM);
    cmd.AddValue("loopSideM",
                 "Side length of a closed square loop, m. 0 walks a straight line instead",
                 loopSideM);
    cmd.AddValue("turnRateDegPerS", "Yaw rate during the loop's turns, deg/s", turnRateDegPerS);
    cmd.AddValue("randomTurns",
                 "Turns at random intervals through random angles, instead of a fixed schedule. "
                 "Interval ~ Exponential(randomTurnMeanLegM); angle ~ Uniform(+/-randomTurnMaxDeg)",
                 randomTurns);
    cmd.AddValue("randomTurnMeanLegM",
                 "Mean distance between random turns, m (exponentially distributed)",
                 randomTurnMeanLegM);
    cmd.AddValue("randomTurnMaxDeg",
                 "Maximum |turn angle| for random turns, deg (uniform, either direction)",
                 randomTurnMaxDeg);
    cmd.AddValue("zigzagSideM",
                 "Leg length of a STAIRCASE route, m. Legs joined by alternating +/-90 degree "
                 "turns, so the route both turns and translates. 0 disables. Runs for --duration",
                 zigzagSideM);
    cmd.AddValue("loopCount",
                 "How many times to walk the square loop. >1 gives a route with many "
                 "turns, for measuring sustained performance rather than a single circuit",
                 loopCount);
    cmd.AddValue("turnInjection",
                 "Inject centripetal acceleration and trunk-lean gravity projection during "
                 "turns. Disable to reproduce the pre-turn-model behaviour",
                 turnInjection);
    cmd.AddValue("leanMaxDeg",
                 "Peak trunk lean angle during a turn, deg. UNSOURCED ESTIMATE",
                 leanMaxDeg);
    cmd.AddValue("leanRiseS",
                 "Trunk lean onset/offset time, s. UNSOURCED ESTIMATE",
                 leanRiseS);
    cmd.AddValue("wrongAxisWiring",
                 "Deliberately feed the LATERAL axis to the step detector as if it were "
                 "vertical, to quantify what a wrong axis assignment costs",
                 wrongAxisWiring);
    cmd.AddValue("gyroBiasObserver",
                 "Enable the ZARU gyro bias observer. Disable to measure what it is worth",
                 gyroBiasObserver);
    cmd.AddValue("restEveryS",
                 "Seconds of walking between rest stops. 0 disables rests entirely",
                 restEveryS);
    cmd.AddValue("restForS", "Seconds spent stationary at each rest stop", restForS);
    cmd.AddValue("knownInitialState",
                 "Seed the known deployment position and heading instead of bootstrapping "
                 "them from radio fixes",
                 knownInitialState);
    cmd.AddValue("oracleHeadingAtCalibEnd",
                 "ORACLE, DEFAULT OFF: inject the ground-truth heading at the end of the "
                 "calibration walk. Erases drift accumulated during the walk. Not achievable "
                 "by the real system -- diagnostic use only, never for reported results",
                 oracleHeadingAtCalibEnd);
    cmd.AddValue("calibrationWalkM",
                 "Length of a straight calibration walk prepended to the route, m. 0 disables. "
                 "Calibrates k_w before the route proper begins",
                 calibrationWalkM);
    cmd.AddValue("snapPositionToFix",
                 "Let a clean radio fix overwrite the dead-reckoned position. Bounds error to one fix interval, but makes end-of-run error a function of fix spacing rather than of pipeline accuracy -- read the worst-instant figure instead",
                 snapPositionToFix);
    cmd.AddValue("rngRun", "ns-3 RNG run number", rngRun);
    cmd.Parse(argc, argv);

    // The square loop walks four equal straight sides joined by four 90 degree
    // turns, taken at a constant yaw rate while still walking forward. The
    // corners are therefore rounded rather than sharp, which is both more
    // realistic and still exactly closed: four displacement vectors each
    // rotated 90 degrees from the last sum to zero. Size the run to the route
    // so the loop actually closes, since a partial loop has no meaningful
    // return position error.
    const bool loopMode = (loopSideM > 0.0);
    const bool zigzagMode = (zigzagSideM > 0.0);
    const bool randomMode = randomTurns;
    const double turnRateRadPerS = turnRateDegPerS * M_PI / 180.0;
    if (loopMode)
    {
        double straightTime = 4.0 * loopCount * (loopSideM / speed);
        double turnTime = 4.0 * loopCount * ((M_PI / 2.0) / turnRateRadPerS);
        duration = straightTime + turnTime;
    }

    // A calibration walk is a straight stretch prepended to the route, with a
    // trusted fix at each end. That fix pair brackets a known distance over a
    // stretch with no turning, which is exactly what the online k_w calibration
    // needs, so stride length is calibrated before the route proper starts
    // rather than being estimated with the kwFallback placeholder throughout.
    // Rests consume wall-clock time without advancing the route, so the run has
    // to be lengthened by the resting fraction or the route simply does not
    // finish -- and a partial square loop has no meaningful return position
    // error.
    if (restEveryS > 0.0 && restForS > 0.0)
    {
        duration *= (restEveryS + restForS) / restEveryS;
    }

    const double calibrationTimeS = (calibrationWalkM > 0.0) ? (calibrationWalkM / speed) : 0.0;
    duration += calibrationTimeS;

    RngSeedManager::SetRun(rngRun);

    ShsEngineParams params;
    params.gyroBias.enabled = gyroBiasObserver;
    params.snapPositionToTrustedFix = snapPositionToFix;
    auto sensorRv = CreateObject<NormalRandomVariable>();
    auto fixRv = CreateObject<NormalRandomVariable>();
    // Seeded from the same RngRun, so a given seed reproduces the same route.
    auto legRv = CreateObject<ExponentialRandomVariable>();
    legRv->SetAttribute("Mean", DoubleValue(randomTurnMeanLegM));
    auto angleRv = CreateObject<UniformRandomVariable>();
    angleRv->SetAttribute("Min", DoubleValue(-randomTurnMaxDeg * M_PI / 180.0));
    angleRv->SetAttribute("Max", DoubleValue(randomTurnMaxDeg * M_PI / 180.0));
    ShsEngine engine(params, sensorRv);

    // The field node is deployed from a known location in a known orientation,
    // so seed both rather than making the engine bootstrap them from radio
    // fixes. Without this the heading can never initialize on a route whose
    // every fix interval contains a turn.
    if (knownInitialState)
    {
        engine.SetInitialState(0.0, 0.0, trueHeading);
    }

    const double dt = 1.0 / params.sensor.sampleRateHz;
    const auto ticks = static_cast<uint32_t>(duration / dt);
    const double gaitPeriod = 1.0 / cadenceHz;

    double trueX = 0.0;
    double trueY = 0.0;
    double lastFixTime = -1e9;

    // Ground-truth route state. In straight-line mode the heading never changes
    // and trueOmegaZ stays 0, exactly as before.
    double curHeading = trueHeading;
    double pathLength = 0.0;
    double segmentDistance = 0.0;
    double turnAccumulated = 0.0;
    double turnSign = 1.0;      // flips each turn in staircase mode
    double turnTargetRad = M_PI / 2.0; // magnitude of the turn in progress
    double nextLegM = randomMode ? legRv->GetValue() : 0.0;
    uint32_t randomTurnCount = 0;
    uint32_t turnsRemaining = 4 * loopCount;
    bool turning = false;
    bool calibrationFixDone = false;
    double maxError = 0.0;
    double turnStartT = -1e9;
    const double turnDurationS = (M_PI / 2.0) / turnRateRadPerS;
    const double leanMaxRad = leanMaxDeg * M_PI / 180.0;

    for (uint32_t i = 0; i < ticks; ++i)
    {
        double t = i * dt;

        // Ground truth: constant speed along the current heading. The heading
        // is fixed in straight-line mode and rotates through the loop's turns
        // otherwise. trueOmegaZ is the yaw rate the gyro genuinely experiences.
        // The calibration walk is deliberately straight: no turning, and the
        // loop route does not begin until it is over.
        bool inCalibrationWalk = (t < calibrationTimeS);

        // Rest stops. The wearer is stationary: no forward motion, no gait
        // acceleration, no yaw. These are what the ZARU gyro bias observer
        // needs -- at a backpack there is no zero-velocity instant during
        // walking, so a genuine rest is the only time the zero-rate assumption
        // holds. Rest time does not count toward the route, so the walked path
        // is unchanged by adding them.
        bool resting = false;
        if (restEveryS > 0.0 && !inCalibrationWalk)
        {
            double sinceRouteStart = t - calibrationTimeS;
            double cycle = std::fmod(sinceRouteStart, restEveryS + restForS);
            resting = (cycle >= restEveryS);
        }

        double trueOmegaZ = 0.0;
        if ((loopMode || zigzagMode || randomMode) && !inCalibrationWalk && !resting)
        {
            double legLength = randomMode ? nextLegM : (zigzagMode ? zigzagSideM : loopSideM);
            bool turnsLeft = randomMode || zigzagMode || (turnsRemaining > 0);
            if (turning)
            {
                trueOmegaZ = turnRateRadPerS * turnSign;
                turnAccumulated += trueOmegaZ * dt;
                if (std::fabs(turnAccumulated) >= turnTargetRad)
                {
                    // Trim the final tick so the turn lands on exactly 90
                    // degrees; overshooting would leave a closed loop unclosed
                    // and would bias the staircase's advance direction.
                    trueOmegaZ -= (turnAccumulated - turnSign * turnTargetRad) / dt;
                    turnAccumulated = 0.0;
                    turning = false;
                    segmentDistance = 0.0;
                    // Staircase alternates direction, so the route advances
                    // along a diagonal instead of closing.
                    if (zigzagMode)
                    {
                        turnSign = -turnSign;
                    }
                    if (randomMode)
                    {
                        nextLegM = legRv->GetValue();
                    }
                }
            }
            else if (turnsLeft && segmentDistance >= legLength)
            {
                if (!zigzagMode)
                {
                    --turnsRemaining;
                }
                turning = true;
                turnStartT = t;
                if (randomMode)
                {
                    // Draw this turn's angle; sign gives the direction, and the
                    // magnitude sets how long the turn lasts at the fixed rate.
                    double drawn = angleRv->GetValue();
                    turnSign = (drawn < 0.0) ? -1.0 : 1.0;
                    turnTargetRad = std::fabs(drawn);
                    ++randomTurnCount;
                }
                trueOmegaZ = turnRateRadPerS * turnSign;
                turnAccumulated += trueOmegaZ * dt;
            }
        }

        double effectiveSpeed = resting ? 0.0 : speed;
        curHeading += trueOmegaZ * dt;
        trueX += effectiveSpeed * std::cos(curHeading) * dt;
        trueY += effectiveSpeed * std::sin(curHeading) * dt;
        pathLength += effectiveSpeed * dt;
        if (!turning && !inCalibrationWalk && !resting)
        {
            segmentDistance += effectiveSpeed * dt;
        }

        // Synthetic vertical bounce: a half-sine over the first 40% of each
        // gait cycle (the swing phase), flat through stance. This gives the
        // step detector a realistic valley-peak-valley shape to lock onto.
        double cyclePhase = std::fmod(t, gaitPeriod) / gaitPeriod;
        double verticalBounce =
            (cyclePhase < 0.4) ? bounceAmplitude * std::sin(M_PI * cyclePhase / 0.4) : 0.0;

        // Anterior-posterior gait acceleration: one full cycle of forward
        // acceleration at push-off and deceleration at heel strike, smaller in
        // amplitude than the vertical bounce. Previously this channel was fed a
        // hard 0.0, which meant the detector's AP input was pure sensor noise
        // and the Iluz method's second channel could not be exercised at all.
        double apAccel = apAmplitude * std::sin(2.0 * M_PI * cyclePhase);
        if (resting)
        {
            verticalBounce = 0.0;
            apAccel = 0.0;
        }

        // ---- Turn injection -------------------------------------------------
        //
        // Two turn-synchronous mechanisms that a two-axis, gravity-free sensor
        // model could not represent at all. Both are ordinary physics; neither
        // is taken from a paper. What the literature contributes is evidence
        // that they are real and measurable at a trunk mount: Fino et al. (2020)
        // measured centripetal acceleration at a lumbar mount and found straight
        // walking tightly distributed about 0 while turning is not; PMC4325939
        // observed trunk inclination toward the inside of the turn. Neither
        // supplies the magnitudes used here.
        //
        //   centripetal : a_c = v * omega_z
        //   lean        : a_lean = g * sin(theta_lean), with the vertical
        //                 channel correspondingly reading g * cos(theta_lean)
        //
        // Both act along the same physical direction during a turn and sum on
        // the lateral axis. The lean's onset and offset are what put this
        // transient inside the 0.5-3 Hz gait passband -- a static tilt would be
        // removed by the band-pass, a changing one is not.
        double thetaLean = 0.0;
        double lateralAccel = 0.0;
        if (turnInjection && turning && !resting)
        {
            double sinceTurnStart = t - turnStartT;
            double rise = SmoothStep(sinceTurnStart, 0.0, leanRiseS);
            double fall = SmoothStep(sinceTurnStart, turnDurationS - leanRiseS, turnDurationS);
            thetaLean = leanMaxRad * (rise - fall);
            lateralAccel = effectiveSpeed * trueOmegaZ + kG * std::sin(thetaLean);
        }

        // Gravity now sits on the vertical channel, as a real MPU-6050 reports
        // it. Lean rotates part of it away, which is why the lean angle has to
        // reach the vertical channel and not only the lateral one.
        double verticalTotal = kG * std::cos(thetaLean) + verticalBounce;

        // Per the engine's placeholder axis assignment, trueAy is the vertical
        // channel and trueAx the anterior-posterior one. Confirm this against
        // the real mounting orientation before trusting the output.
        // Axis wiring. Normally the engine receives (AP, vertical, lateral).
        // With wrongAxisWiring the lateral and vertical arguments are swapped,
        // which is exactly the failure a wrong physical axis assignment would
        // produce: the step detector's primary input becomes the axis carrying
        // the turn transients instead of the gait bounce.
        if (wrongAxisWiring)
        {
            engine.Tick(t, dt, apAccel, lateralAccel, verticalTotal, trueOmegaZ, thetaLean);
        }
        else
        {
            engine.Tick(t, dt, apAccel, verticalTotal, lateralAccel, trueOmegaZ, thetaLean);
        }

        // Track the WORST error seen at any instant, not just the final one.
        //
        // Final-position error alone is misleading on any route that returns
        // toward its start. The dead-reckoned path is close to a scaled copy of
        // the true path, so a step-length scale error contracts the whole route
        // and shows up at the end only in proportion to net displacement --
        // which a closed loop drives to zero. A square loop therefore reports
        // near-perfect final error while having been tens of metres out at its
        // farthest point. The running maximum below is immune to that, and to
        // the matching blindness RPE has to a constant heading offset.
        // Fire one extra fix the moment the calibration walk ends, so the pair
        // bracketing it is guaranteed regardless of the periodic fix schedule.
        bool calibrationWalkJustEnded =
            (calibrationTimeS > 0.0 && !calibrationFixDone && t >= calibrationTimeS);
        if (calibrationWalkJustEnded)
        {
            calibrationFixDone = true;
        }

        if (calibrationWalkJustEnded || t - lastFixTime >= fixIntervalS)
        {
            lastFixTime = t;
            double variance = fixNoiseSigmaM * fixNoiseSigmaM;
            double bound = 4.0 * fixNoiseSigmaM;
            double fixX = trueX + fixRv->GetValue(0.0, variance, bound);
            double fixY = trueY + fixRv->GetValue(0.0, variance, bound);
            engine.OnTrustedFix(t, fixX, fixY, true);

            // End of the calibration walk: re-seed position from that fix.
            //
            // This is deployment initialization, NOT the per-fix snapping that
            // is now off by default. During the calibration walk k_w is still
            // the fallback, so the walk is dead-reckoned with a stride constant
            // known to be wrong -- with a 100 m walk and the 0.7 fallback
            // against a true ~0.455, that alone injects a systematic 14.4% of
            // path error which would otherwise be carried for the whole route
            // and swamp everything being measured after it. The walk exists to
            // produce k_w, not to navigate; its endpoint is a known location by
            // procedure, so the position is re-established there once and then
            // left alone for the rest of the run.
            if (calibrationWalkJustEnded)
            {
                // Position is re-established from the RADIO FIX, which the real
                // system does have. Heading is left to the estimator: whatever
                // the gyro has integrated, drift included, is carried forward.
                // Re-seed heading to the KNOWN deployment heading, not to
                // whatever the fix pair implied. The calibration walk is
                // straight along the known heading by procedure, so the true
                // heading at its end is still trueHeading. Letting the drift
                // correction snap heading to the fix-implied direction instead
                // discards that knowledge and injects the fix noise: two 3 m
                // fixes 200 m apart imply a direction uncertain by
                // atan(3*sqrt(2)/200) ~= 0.021 rad, which then persists for the
                // whole route. Measured contribution: up to 0.031 rad, worth
                // 154 m over a 5 km traverse.
                engine.SetInitialState(fixX,
                                       fixY,
                                       oracleHeadingAtCalibEnd ? trueHeading : engine.GetTheta());
            }
        }

        // Track the WORST error seen at any instant, not just the final one.
        //
        // Final-position error alone is misleading on any route that returns
        // toward its start. The dead-reckoned path is close to a scaled copy of
        // the true path, so a step-length scale error contracts the whole route
        // and shows up at the end only in proportion to net displacement --
        // which a closed loop drives to zero. A square loop therefore reports
        // near-perfect final error while having been tens of metres out at its
        // farthest point. The running maximum below is immune to that, and to
        // the matching blindness RPE has to a constant heading offset.
        //
        // Sampled AFTER the fix block on purpose: the calibration walk is
        // dead-reckoned on the fallback k_w and accrues ~112 m of error that is
        // zeroed by the re-seed at its end. Sampling before the re-seed would
        // record that transient as the run's worst error, which it is not --
        // it belongs to deployment, not to the route being measured.
        if (t >= calibrationTimeS)
        {
            double ex = engine.GetX() - trueX;
            double ey = engine.GetY() - trueY;
            maxError = std::max(maxError, std::sqrt(ex * ex + ey * ey));
        }
    }

    double trueDistance = std::sqrt(trueX * trueX + trueY * trueY);
    double errX = engine.GetX() - trueX;
    double errY = engine.GetY() - trueY;
    double finalErr = std::sqrt(errX * errX + errY * errY);

    std::cout << "=== SHS Pipeline (backpack IMU) ===\n";
    if (oracleHeadingAtCalibEnd)
    {
        std::cout << "*** WARNING: ORACLE ACTIVE -- ground-truth heading injected at the "
                     "calibration-walk end. Results are NOT achievable by the real system "
                     "and must not be reported. ***\n";
    }
    std::printf("Route: %s\n",
                randomMode
                    ? "random turns (exponential intervals, uniform angles; turns AND net "
                      "displacement)"
                    : zigzagMode
                    ? "staircase (legs joined by alternating +/-90 deg turns; turns AND net "
                      "displacement)"
                    : loopMode ? "closed square loop (four sides, four 90 deg turns)"
                               : "straight line (no turns -- gyro sees bias and noise only)");
    std::printf("Simulated time: %.0f s\n", duration);
    std::printf("Path length walked: %.2f m\n", pathLength);
    std::printf("True final position: (%.2f, %.2f), distance from origin: %.2f m\n",
                trueX,
                trueY,
                trueDistance);
    std::printf("Estimated final position: (%.2f, %.2f)\n", engine.GetX(), engine.GetY());

    // Report the error against PATH LENGTH, not against displacement from the
    // origin. On a closed loop the displacement is ~0, so the old denominator
    // would divide by nothing; and even on a straight route, error as a
    // fraction of distance is only comparable to the pedestrian-dead-reckoning
    // literature when the denominator is the distance actually walked. Note
    // that with radio fixes enabled the position is snapped at every fix, so
    // this figure reflects one fix interval of dead reckoning, NOT unaided
    // performance -- disable fixes to measure that.
    std::printf("Final position error: %.2f m (%.2f%% of the %.0f m walked)\n",
                finalErr,
                100.0 * finalErr / std::max(pathLength, 1e-6),
                pathLength);
    if (loopMode)
    {
        std::printf("Return position error (RPE): %.2f%% of path length\n",
                    100.0 * finalErr / std::max(pathLength, 1e-6));
    }
    std::printf("Worst position error at any instant: %.2f m (%.2f%% of path)\n",
                maxError,
                100.0 * maxError / std::max(pathLength, 1e-6));
    std::printf("Detected step count: %u (expected ~%.0f for %.0f s at %.1f Hz cadence)\n",
                engine.GetDetectedStepCount(),
                duration * cadenceHz,
                duration,
                cadenceHz);
    std::printf("Final k_w: %.4f (fallback was %.4f)\n",
                engine.GetCurrentKw(),
                params.stepLength.kwFallback);
    if (randomMode)
    {
        std::printf("Ground-truth random turns executed: %u\n", randomTurnCount);
    }
    std::printf("Turns detected (yaw >90 deg in 0.5-10 s): %u, %.0f s spent turning\n",
                engine.GetTurnCount(),
                engine.GetTurningTimeS());
    std::printf("ZARU gyro bias estimate: %.3e rad/s from %u update(s) "
                "(true bias instability %.3e rad/s)\n",
                engine.GetGyroBiasEstimate(),
                engine.GetGyroBiasUpdateCount(),
                params.sensor.GyroBiasInstabilityRadS());
    // Compare headings modulo 2*pi. On the square loop the ground-truth heading
    // accumulates a full turn, and the estimate is only re-wrapped when a fix
    // snaps it, so a raw subtraction can differ by 2*pi and read as nonsense.
    double headingDiff = engine.GetTheta() - curHeading;
    while (headingDiff > M_PI)
    {
        headingDiff -= 2.0 * M_PI;
    }
    while (headingDiff < -M_PI)
    {
        headingDiff += 2.0 * M_PI;
    }
    std::printf("Final theta estimate: %.4f rad (true heading: %.4f rad, diff: %.4f rad)\n",
                engine.GetTheta(),
                curHeading,
                headingDiff);

    return 0;
}
