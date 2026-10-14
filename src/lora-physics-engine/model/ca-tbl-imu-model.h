#ifndef CA_TBL_IMU_MODEL_H
#define CA_TBL_IMU_MODEL_H

/**
 * \file ca-tbl-imu-model.h
 * \brief CA-TBL C3 -- a body-worn MEMS IMU and the pedestrian dead-reckoning (PDR)
 *        increments it produces, as an error model driven by the true trajectory.
 *
 * Why this exists
 * ---------------
 * The specification puts an inertial dead-reckoning prior in the EKF predict step
 * (ch10 §10.3.1). Until this file the predict step was constant-velocity only, so
 * every no-fix cycle "coasted" on an extrapolation of past radio fixes, and the
 * integrity check's prior was built from the same radio history it was meant to be
 * independent of (docs/CONTEXT_PACK.md, predict-step row).
 *
 * The IMU in the prior study's Python pipeline (scratch/multimodal_generator.py) is
 * NOT a starting point. It modelled one error term of five, its gyro term was not
 * the integral it claimed, its "EKF" took the realised error as its variance, and
 * its only check correlated a function with its own argument. See
 * docs/COMPLETION_PLAN.md §1.2. Nothing here is ported from it.
 *
 * Why PDR and not strapdown
 * -------------------------
 * Double-integrating a consumer accelerometer is not a navigation method at the
 * timescales this scenario needs. The BMI270 zero-g offset is ±20 mg (below): a
 * residual 20 mg bias alone is 0.5 * 0.196 m/s^2 * t^2 = 1411 m at t = 120 s. A
 * pedestrian tag therefore uses step-and-heading: the accelerometer detects steps
 * and a stride model converts them to distance, the gyro integrates heading. The
 * accelerometer bias then does not integrate twice, and the error budget is set by
 * the gyro (heading) and the stride model (distance). That is the model below.
 *
 * Scope limits -- stated so a reader does not have to find them
 * ------------------------------------------------------------
 *  - Planar. The node moves in 2-D at fixed height (Tourist::Position z = 1.5), so
 *    only yaw is modelled. Tilt and its leakage into yaw are not.
 *  - No magnetometer. Absolute heading is not observed by the IMU; it is an initial
 *    alignment error that the filter must learn from radio fixes.
 *  - No zero-velocity or zero-rate updates. The scenario's dwell is NOT stationary
 *    -- Tourist mills about at 0.3 x walking speed -- so there is no standstill to
 *    exploit without changing the mobility model, which would move every arm.
 *  - Step detection is assumed to count every step; its errors are folded into the
 *    per-step stride error. Missed/extra steps are not modelled separately.
 *  - MCU energy for running PDR is not charged, matching the energy model's
 *    existing "radio energy only" scope (ca-tbl-energy-model.h).
 *
 * The time base is the decision cycle. The sensor is not sampled at its ODR here;
 * instead each error process is advanced over a cycle with its exact discrete-time
 * statistics (white rate noise integrates to sigma^2 = N^2 dt over a cycle; a
 * first-order Gauss-Markov bias uses exp(-dt/tau)). The trajectory itself only
 * exists at cycle resolution (Tourist::Step(dt)), so a finer sensor clock would be
 * integrating a path that is piecewise-linear at dt anyway.
 *
 * Randomness
 * ----------
 * The model owns a self-contained generator (SplitMix64-seeded xoshiro256**, with a
 * Box-Muller normal) and DOES NOT create any ns-3 RandomVariableStream. Creating
 * one consumes an automatic stream index even if SetStream() is called afterwards
 * (random-variable-stream.cc, SetStream(-1) during attribute construction), which
 * would shift every automatically-assigned engine stream created later -- i.e. it
 * would change the channel realisation when the IMU is switched on, confounding the
 * treatment with a different radio world. A private generator cannot do that.
 *
 * The draw count is fixed per Step() (three normals), independent of parameter
 * values, so zeroing one term leaves every other term's realisation unchanged.
 * That property is what makes the term-by-term validation in
 * scratch/ca_tbl_imu_validator.cc meaningful.
 */

#include <cstdint>

namespace ns3
{

/**
 * \brief IMU and stride-model parameters.
 *
 * Every field states its source. CITED fields come from the Bosch BMI270 data sheet,
 * BST-BMI270-DS000-04, revision 1.2, June 2020, tables "Output signal gyroscope" and
 * "Electrical specification" (pp. 8-13). Where the data sheet gives a typical
 * +/- value, it is used here as a ONE-SIGMA spread across parts -- that reading is
 * OURS; data sheets do not define "typ" statistically. OURS fields are choices,
 * labelled as such, and are the first things to sweep.
 *
 * The same struct is used twice: once for the TRUTH (what the simulated sensor
 * actually does, in CaTblImuModel) and once for the FILTER (what the EKF assumes,
 * in CaTblPolicyConfig). They default to the same values -- the filter is told the
 * data-sheet spec, not the realised error -- and can be set apart to test a
 * mis-specified filter.
 */
struct CaTblImuParams
{
    //! Gyro rate noise density, deg/s/sqrt(Hz). CITED: 0.007, performance mode,
    //! "Output Noise nG,nd". Integrated over a cycle it is angle random walk:
    //! heading sigma = N * sqrt(t).
    double gyroNoiseDpsRtHz;
    //! One-sigma residual gyro bias at the start of the run, deg/s, AFTER the tag's
    //! own static offset calibration. OURS, derived: the data sheet gives zero-rate
    //! offset +/-0.5 deg/s uncalibrated and its temperature coefficient TCO_G
    //! +/-0.015 deg/s/K. A calibration at power-on removes the first; what it cannot
    //! remove is drift from a temperature change after calibration. 10 K (a tag
    //! carried from a vehicle into a cold site) gives 0.15 deg/s. The 10 K is ours.
    double gyroBiasInitDps;
    //! Bias instability as a first-order Gauss-Markov sigma, deg/s. OURS: the
    //! BMI270 data sheet does not specify bias instability. 0.0014 deg/s = 5 deg/h,
    //! an order-of-magnitude figure for consumer MEMS. Small against gyroBiasInitDps
    //! by design; it exists so the filter's bias state has something to track.
    double gyroBiasGmDps;
    //! Gauss-Markov correlation time, s. OURS.
    double gyroBiasTauS;
    //! One-sigma gyro scale-factor error, fraction. CITED: +/-0.4 % after the
    //! component re-trim (CRT), "SG_err_CRT"; +/-2 % without CRT.
    double gyroScaleErr;
    //! One-sigma initial heading alignment error, degrees. OURS. With no
    //! magnetometer the tag's absolute heading at start is not measured; this is the
    //! error of whatever coarse alignment it begins with (e.g. a compass bearing).
    double headingInitDeg;
    //! Nominal stride length, m. OURS. Used only to convert distance into a step
    //! count for the per-step error; 0.75 m at 1.5 m/s is two steps per second.
    double strideLengthM;
    //! One-sigma per-run stride-model scale error, fraction. OURS. A step-length
    //! model calibrated on one gait and walked on another (load, slope, snow) is
    //! wrong by a roughly constant factor for the whole run.
    double strideScaleErr;
    //! One-sigma per-step length error, fraction of strideLengthM. OURS.
    double strideStepErr;
    //! Supply current with accelerometer and gyroscope running, mA. CITED: 0.970
    //! mA, "IDD A+G Performance Mode", VDD = 1.8 V, ODR max. The energy line charges
    //! it at the radio rail voltage (an LDO passes current, not power) -- OURS.
    double currentMa;

    CaTblImuParams();
};

/** What the IMU hands the filter each cycle. */
struct CaTblImuIncrement
{
    double distM;         //!< PDR distance travelled this cycle (stride model)
    double rawHeadingRad; //!< gyro-integrated heading at the end of the cycle,
                          //!< in the IMU's own (misaligned, drifting) frame
    bool valid;
    CaTblImuIncrement();
};

/**
 * \brief Simulated sensor. Driven by the TRUE per-cycle displacement; returns what
 *        a PDR tag would measure. The filter never sees anything else from here.
 */
class CaTblImuModel
{
  public:
    CaTblImuModel();

    /** Seed the private generator and draw the per-run constants (turn-on bias,
     *  gyro scale factor, stride scale, alignment error). Call once per arm. */
    void Configure(const CaTblImuParams& p, uint64_t seed);

    /** Advance one cycle given the TRUE displacement (dx, dy) over \p dt. */
    CaTblImuIncrement Step(double dxTrue, double dyTrue, double dt);

    // ---- truth-side diagnostics. For traces and validation ONLY; passing any of
    // these to the filter would make it an oracle, which is the defect this file
    // exists to replace.
    double TrueHeadingErrRad() const; //!< raw heading minus true heading, wrapped
    double TrueBiasRadS() const;      //!< gyro bias that will apply to the next cycle
    double TrueGyroScale() const { return m_sg; }
    double TrueStrideScale() const { return m_sd; }
    double TrueAlignErrRad() const { return m_align; }
    double TrueHeadingRad() const { return m_psiTruePrev; }
    double RawHeadingRad() const { return m_rawHeading; }
    uint64_t Steps() const { return m_steps; }

    /** Wrap to (-pi, pi]. */
    static double Wrap(double a);

  private:
    double Normal();
    uint64_t NextU64();

    CaTblImuParams m_p;
    uint64_t m_s[4];

    // per-run constants
    double m_b0;    //!< rad/s
    double m_sg;    //!< gyro scale error
    double m_sd;    //!< stride scale error
    double m_align; //!< rad

    // state
    double m_bGm; //!< rad/s
    double m_rawHeading;
    double m_psiTruePrev;
    bool m_started;
    uint64_t m_steps;
};

} // namespace ns3

#endif // CA_TBL_IMU_MODEL_H
