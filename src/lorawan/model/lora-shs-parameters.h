/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Backpack-mounted IMU / Step-and-Heading System (SHS) parameter set.
 *
 * See doc/imu-shs-design.md for the full citation list. This header only
 * carries the numeric values and a one-line pointer to the source; the
 * reasoning is not repeated here.
 */

#ifndef LORA_SHS_PARAMETERS_H
#define LORA_SHS_PARAMETERS_H

#include <cmath>
#include <cstdint>

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Step 0 parameters: MPU-6050 accelerometer and gyroscope measurement model.
 *
 * Sources: TDK/InvenSense PS-MPU-6000A-00 Rev 3.4 (accelerometer noise
 * density); Rudyk et al., Sensors 20(17):4841, 2020 (gyroscope noise density,
 * bias instability and Gauss-Markov time constant). The accelerometer bias
 * instability and its time constant are FLAGGED ESTIMATES -- no MPU-6050
 * specific measurement was found for either (design doc Section 6, Flag 1).
 *
 * @see ImuSensorSubmodule
 */
struct ImuSensorParams
{
    double accelNoiseDensity = 3.9227e-3;  //!< Accel noise density, m/s^2/sqrt(Hz) (400 ug/sqrt(Hz))
    double accelBiasInstability = 1.471e-3; //!< Accel bias instability, m/s^2 (~150 ug) -- ESTIMATE
    double accelBiasTimeConstantS = 300.0;  //!< Accel bias Gauss-Markov time constant, s -- ESTIMATE

    double gyroNoiseDensityRadS = 1.6682e-4;        //!< Gyro noise density, rad/s/sqrt(Hz)
    double gyroBiasInstabilityDegPerHr = 8.816;     //!< Gyro bias instability, deg/h (worst axis, Z)
    double gyroBiasTimeConstantS = 80.0;            //!< Gyro bias Gauss-Markov time constant, s

    double sampleRateHz = 100.0; //!< IMU output data rate, Hz

    /**
     * Accelerometer full-scale range (AFS_SEL), g. Readings saturate here.
     *
     * Set to +/-6g on the evidence rather than by judgement: Rowlands et al.
     * (PMC4180224) measured waist, thigh and ankle acceleration across gait
     * velocities up to 4.8 m/s and found the waist stayed within +/-6g for 99%
     * of cases. A trunk/backpack mount sees the same envelope as the waist, so
     * +/-6g covers essentially all walking while giving twice the resolution of
     * the +/-16g the part also offers.
     *
     * This replaces an earlier +/-8g, which was an engineering decision with no
     * paper behind it (design doc Flag 3). Note the trade: the 1% of waist
     * samples outside +/-6g in Rowlands now saturate. Those correspond to
     * running and impact, not walking, so for a walking-speed SAR field node
     * the resolution is worth more than the headroom -- revisit if the wearer
     * is expected to run.
     */
    double accelFullScaleG = 6.0;

    /**
     * Gyroscope full-scale range (FS_SEL), deg/s. Readings saturate here.
     *
     * STILL UNCITED (design doc Flag 2). No study was found reporting peak
     * trunk yaw rate in a form usable for choosing a full-scale range. Retained
     * at +/-500 deg/s by the original reasoning that trunk rotation is gentler
     * than foot swing. For context, chest-mounted gyroscopes during walking are
     * reported around 40-60 deg/s, so this is roughly 10x margin -- likely more
     * than needed, and a candidate for reduction to +/-250 deg/s (doubling
     * resolution) if trunk yaw rate is ever measured directly.
     */
    double gyroFullScaleDegPerS = 500.0;

    /**
     * Accelerometer saturation limit in m/s^2.
     *
     * @return The full-scale range converted from g.
     */
    double AccelFullScaleMps2() const
    {
        return accelFullScaleG * 9.80665;
    }

    /**
     * Gyroscope saturation limit in rad/s.
     *
     * @return The full-scale range converted from deg/s.
     */
    double GyroFullScaleRadS() const
    {
        return gyroFullScaleDegPerS * M_PI / 180.0;
    }

    /**
     * Gyroscope bias instability converted from deg/h to rad/s.
     *
     * @return The bias instability in rad/s.
     */
    double GyroBiasInstabilityRadS() const
    {
        return gyroBiasInstabilityDegPerHr * (M_PI / 180.0) / 3600.0;
    }
};

/**
 * @ingroup lorawan
 *
 * Step 1 parameters: Iluz walk/step detection.
 *
 * Source: Iluz, T. et al., J. NeuroEngineering and Rehabilitation, 11, 48,
 * 2014. Selected for this sensor placement and population per Mico-Amigo et
 * al., Mobilise-D consortium, J. NeuroEngineering and Rehabilitation, 20, 78,
 * 2023.
 *
 * @see StepDetectorSubmodule
 */
struct StepDetectorParams
{
    double bandpassLowHz = 0.5;       //!< Band-pass lower cutoff, Hz
    double bandpassHighHz = 3.0;      //!< Band-pass upper cutoff, Hz
    double windowLengthS = 5.0;       //!< Sliding window length, s
    double templateFreqHz = 2.0;      //!< Matched-filter sinusoidal template frequency, Hz
    uint32_t minStepsPerWindow = 2;   //!< Lower bound of the gait classification rule
    uint32_t maxStepsPerWindow = 15;  //!< Upper bound of the gait classification rule

    /**
     * Peak-finding sensitivity, as a fraction of the MEAN magnitude of all
     * candidate peaks in the window. A peak is kept only if its magnitude
     * exceeds peakRelativeThreshold * mean(all candidate peak magnitudes).
     *
     * Source: Bertoli, M. et al., "Validation of a Step Detection Algorithm
     * during Straight Walking and Turning in Patients with Parkinson's Disease
     * and Older Adults Using an Inertial Measurement Unit at the Lower Back,"
     * PMC5591331 -- which keeps peaks of "magnitude >40% of the mean of all
     * peaks detected". Same sensor location class as this project (lower
     * back / trunk), and validated explicitly across straight AND turning
     * walking, which is why its threshold form is preferred here.
     *
     * This REPLACES an earlier peakProminenceThreshold = 0.05, which was an
     * absolute compare against an unnormalized correlation sum whose scale runs
     * 1-100. That test never rejected anything: it was the reason a stationary
     * wearer produced ~215 phantom "steps" per 120 s. The walking-energy gate
     * (walkingRmsThresholdMps2) closed that hole; this makes the peak test
     * itself meaningful rather than merely inert, and scales correctly across
     * slow and fast walking where a fixed number cannot.
     */
    double peakRelativeThreshold = 0.40;

    /**
     * Minimum RMS of the band-passed acceleration, over one window, for that
     * window to be considered walking at all. Windows below this are rejected
     * before the matched filter runs, so no steps can be emitted.
     *
     * Computed over the COMBINED vertical and anterior-posterior channels,
     * which is where Iluz's use of both channels enters this implementation.
     *
     * Without this gate the detector fires on the MPU-6050 noise floor alone: a
     * stationary wearer produced ~215 "steps" per 120 s and ~95 m of phantom
     * displacement, because peakProminenceThreshold is absolute and the noise
     * correlation sum (~0.33) sits well above it (0.05).
     *
     * Not individually cited -- no study was found that states a band-passed
     * RMS threshold for backpack-mounted walk detection; the literature
     * describes the method and leaves this constant to calibration. The
     * multi-criteria approach (amplitude gate plus peak distance, prominence
     * and periodicity constraints) follows standard step-counting practice.
     *
     * Default derived by measurement against this project's own sensor model
     * (scratch/shs-noise-floor.cc), combined-channel RMS:
     * \verbatim
     *   standing still (noise only) : 0.0147 m/s^2
     *   sub-walking motion (0.1)    : 0.0407 m/s^2
     *   faint motion (0.5)          : 0.1899 m/s^2
     *   light walking (2.0)         : 0.7578 m/s^2
     *   normal walking (6.0)        : 2.2723 m/s^2
     * \endverbatim
     * 0.21 sits ~14x above the noise floor and ~3.6x below light walking, while
     * still rejecting motion too faint to be walking. The ~155x separation
     * between standing and walking means the exact value is not critical;
     * re-run the harness and revisit if the sensor part, sample rate, band-pass
     * topology, or the set of channels feeding the gate changes.
     */
    double walkingRmsThresholdMps2 = 0.21;
};

/**
 * @ingroup lorawan
 *
 * Step 3 parameters: Weinberg step-length model.
 *
 * Source: Weinberg, H., "Using the ADXL202 in Pedestrian Dead-Reckoning
 * Applications," Analog Devices AN-602, 2002 (formula only -- k_w itself is
 * not a literature constant, it is calibrated online).
 *
 * @see StepLengthSubmodule
 */
struct StepLengthParams
{
    /**
     * Fallback k_w used before the first online calibration event. PROVISIONAL
     * PLACEHOLDER (design doc Section 6, Flag 5, cold-start item) -- not
     * sourced from a specific paper. Replace with a deliberately chosen
     * population-average value before relying on this for real output.
     */
    double kwFallback = 0.7;

    /**
     * Exponential moving average weight applied to each new online k_w
     * estimate. Not cited -- an implementation choice; 0 < alpha <= 1, larger
     * means faster adaptation, smaller means more stability against noisy
     * single-fix calibration events.
     */
    double kwEmaAlpha = 0.3;

    /**
     * Physiological bounds on the step length a calibration may imply, in
     * metres. chord / (buffered steps) is an implied mean step length, and a
     * stretch implying a step outside this range is refused rather than fitted.
     *
     * This exists because k_w = chord / |sum q_i u_i| has no upper bound: the
     * numerator is a radio chord and the denominator is a gait quantity that does
     * not grow when that chord is mostly fix error. Where fixes are sparse the
     * chord is dominated by noise and the fitted gait runs away -- measured at
     * 1.9e6 m of SHS position error on an arm holding 7 fixes in 1440 cycles,
     * against 13 m on an arm that never calibrated at all.
     *
     * A bound on step length is a claim about legs, not a fitted constant, which
     * is why the guard is placed here and not on k_w.
     *
     * BOTH ZERO DISABLES THE TEST, which is the default so that results predating
     * it reproduce byte for byte.
     */
    double minPlausibleStepM = 0.0;
    double maxPlausibleStepM = 0.0;

    /**
     * Minimum straightness required of a bracketed stretch before it may be
     * used to calibrate k_w, in [0, 1].
     *
     * Straightness is |sum of q_i * u_i| / |sum of q_i| -- 1 for a straight
     * line, 0 for a path returning to its start. Calibration divides the
     * measured chord by the vector-sum magnitude, so as straightness falls the
     * estimate amplifies both radio-fix noise and heading error without bound.
     * Below this floor the stretch is skipped rather than calibrated.
     *
     * Not cited -- an implementation choice. 0.2 admits a stretch containing
     * roughly a single right-angle turn while rejecting near-closed sub-loops.
     */
    double minCalibrationStraightness = 0.2;
};

/**
 * @ingroup lorawan
 *
 * Step 4 parameters: heading estimator.
 *
 * No fixed numeric constants beyond what is already in ImuSensorParams (the
 * gyro bias figures are reused). Drift-correction pattern sources: PMC7472062;
 * "Tightly Coupling Fusion of UWB Ranging and IMU Pedestrian Dead Reckoning
 * for Indoor Localization," IEEE Access, 2021. The LoRa substitution for
 * UWB/WiFi is UNVALIDATED (design doc Section 6, Flag 4).
 *
 * @see HeadingSubmodule
 */
struct HeadingParams
{
    /**
     * A pair of consecutive radio fixes is treated as a "straight-line
     * stretch" usable for drift correction if the implied direction does not
     * deviate from the current heading estimate by more than this, in radians.
     * Implementation parameter, not individually cited.
     */
    double straightLineToleranceRad = 0.17; // ~10 degrees

    /**
     * Provisional theta_0 fallback used before the first two trusted fixes
     * arrive (design doc Section 6, Flag 5, cold-start item). Only reached when
     * the deployment orientation was not seeded via SetInitialHeading.
     */
    double theta0Fallback = 0.0;

    /**
     * Minimum separation between two fixes, m, before the direction they imply
     * may be used to correct or initialize the heading.
     *
     * A fix pair implies a direction with angular uncertainty of roughly
     * atan(sigma_fix / baseline), so over a short baseline the implied
     * direction is mostly fix noise. Correcting to it then makes the heading
     * WORSE than leaving it to the gyro. Measured with 3 m fixes: a 20 m
     * baseline produced up to 0.36 rad of heading error on a dead-straight
     * route whose unaided drift over the same period was 0.006 rad -- a
     * correction 60x worse than the drift it was supposed to remove.
     *
     * Set this to several times the 1-sigma radio fix noise. The default of
     * 20 m assumes ~3 m fixes, giving atan(3/20) ~= 0.15 rad of implied-
     * direction uncertainty, just inside straightLineToleranceRad. Raise it if
     * the RF layer is noisier. Not cited -- an implementation parameter, but
     * the sizing rule above is the thing to preserve, not the number.
     */
    double minCorrectionBaselineM = 20.0;
};

/**
 * @ingroup lorawan
 *
 * Parameters for yaw-based turn detection.
 *
 * Source: Bertoli et al., PMC5591331, which defines a turn as "a change of the
 * yaw angle (i.e., around the vertical axis) with a magnitude >90 degrees and a
 * duration between 0.5 and 10 s", validated at a lower-back mount across
 * straight and turning walking. Both the magnitude and the duration bounds are
 * taken directly from that definition.
 *
 * @see TurnDetector
 */
struct TurnDetectorParams
{
    /**
     * Yaw change that qualifies as a turn, rad. 90 degrees per PMC5591331.
     */
    double turnMagnitudeRad = M_PI / 2.0;

    /// Shortest interval over which the yaw change may accumulate, s. **[CITED]**
    double minTurnDurationS = 0.5;

    /// Longest interval over which the yaw change may accumulate, s. **[CITED]**
    double maxTurnDurationS = 10.0;

    /**
     * Heading-history decimation period, s. Implementation detail, not cited:
     * storing every tick at 100 Hz would mean rescanning 1000 samples per tick
     * over the 10 s window. Any value well below minTurnDurationS leaves the
     * decision unchanged.
     */
    double historySamplePeriodS = 0.1;

    /**
     * Hysteresis: once turning, the yaw change must fall below this fraction of
     * turnMagnitudeRad before the turn is considered over.
     *
     * Implementation detail, not cited. Necessary because a bare threshold
     * chatters whenever the actual turn magnitude sits near the threshold --
     * which it does exactly for the square-loop route, whose turns are 90
     * degrees against a 90 degree threshold. Without hysteresis that route
     * reported 132 turns for 24 real ones. Real-world turns are not so precisely
     * sized, which is presumably why the source paper did not need this.
     */
    double turnExitFraction = 0.75;
};

/**
 * @ingroup lorawan
 *
 * Parameters for the ZARU gyroscope bias observer.
 *
 * Sources: the Zero Angular Rate Update technique is standard in pedestrian
 * inertial navigation -- when the sensor is stationary the gyroscope output is
 * its own error, so it can be averaged and subtracted. See e.g. "Enhanced
 * Heuristic Drift Elimination with Adaptive Zero-Velocity Detection and Heading
 * Correction Algorithms for Pedestrian Navigation" (PMC7070454) and the ZARU
 * treatment in the foot-mounted PDR literature. The numeric values below are
 * NOT from those papers: they are derived from this project's own sensor model,
 * because the published work applies ZARU at foot-mounted stance phases rather
 * than at a trunk mount during rest periods.
 *
 * @see GyroBiasObserver
 */
struct GyroBiasObserverParams
{
    /**
     * Whether the observer runs at all. When false the bias estimate stays at
     * exactly 0.0 and heading integration is unaided, which is both the
     * pre-observer behaviour and the control case for measuring what the
     * observer is worth.
     */
    bool enabled = true;

    /**
     * Maximum |gyro reading| for a sample to count as stationary, rad/s.
     *
     * At 100 Hz the modelled per-sample gyro noise standard deviation is
     * ~1.67e-3 rad/s, so 0.05 rad/s (2.9 deg/s) is roughly 30 sigma: it admits
     * every genuinely still sample while rejecting a wearer turning on the spot.
     */
    double stillnessRateGateRadS = 0.05;

    /**
     * Stationary dwell required before an estimate is adopted, s.
     *
     * Averaging over 5 s at 100 Hz cuts the noise by sqrt(500) ~= 22, to
     * ~7.5e-5 rad/s. That is still above the ~4.3e-5 rad/s bias instability, so
     * a single dwell cannot resolve the bias by itself; convergence comes from
     * the EMA across successive rest periods.
     */
    double dwellS = 5.0;

    /**
     * Exponential moving average weight applied to each adopted estimate.
     *
     * 0.2 gives an effective averaging length of ~5 dwells, i.e. ~25 s of
     * accumulated stillness, which brings the estimate below the bias
     * instability. Not cited -- an implementation choice.
     */
    double emaAlpha = 0.2;
};

/**
 * @ingroup lorawan
 *
 * Aggregate parameter set for the whole SHS pipeline (Steps 0-5).
 *
 * @see ShsEngine
 */
struct ShsEngineParams
{
    ImuSensorParams sensor;         //!< Step 0 parameters
    StepDetectorParams stepDetector; //!< Steps 1-2 parameters
    StepLengthParams stepLength;    //!< Step 3 parameters
    HeadingParams heading;          //!< Step 4 parameters
    GyroBiasObserverParams gyroBias; //!< ZARU gyro bias observer parameters
    TurnDetectorParams turnDetector; //!< Yaw-based turn detection parameters

    /**
     * Whether a clean trusted fix overwrites the dead-reckoned position.
     *
     * DEFAULT IS FALSE, deliberately. Snapping makes the SHS position error
     * unmeasurable: the estimate is reset at every fix, so the reported error
     * only ever reflects one fix interval of dead reckoning no matter how far
     * the wearer walked, and expressing it as a percentage of total distance
     * makes long runs look arbitrarily good (a 60 minute run reported "0.0%"
     * while carrying the same ~1 m error as a 2 minute one). It also discards
     * the SHS estimate outright even when it is better than a noisy LoRa fix.
     *
     * With this false, fixes still drive k_w calibration and heading drift
     * correction -- the aiding that has a defensible basis -- while position
     * accumulates honestly and end-to-end error means what it appears to mean.
     *
     * Set true only to reproduce the previous behaviour. The principled
     * replacement is a filtered position update that weights the fix against
     * the dead-reckoned estimate rather than replacing it, which is part of the
     * EKF redesign the design doc's Section 7 already calls for.
     */
    bool snapPositionToTrustedFix = false;
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_SHS_PARAMETERS_H
