/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Step 0 of the backpack SHS pipeline: MPU-6050 sensor measurement model.
 */

#ifndef LORA_IMU_SENSOR_SUBMODULE_H
#define LORA_IMU_SENSOR_SUBMODULE_H

#include "lora-shs-parameters.h"

#include "ns3/random-variable-stream.h"

namespace ns3
{
namespace lorawan
{

/**
 * @ingroup lorawan
 *
 * Raw sensor state for one IMU tick.
 *
 * No strapdown integration happens here. SHS does not double-integrate
 * acceleration into position -- that is the whole point of using SHS rather
 * than strapdown + ZUPT at a backpack mount, where no true zero-velocity
 * instant exists. This submodule's only job is to produce realistic raw
 * accelerometer and gyroscope samples.
 *
 * @see ImuSensorSubmodule
 */
struct ImuSensorState
{
    double trueAx = 0.0; //!< Ground-truth anterior-posterior acceleration, m/s^2 (gravity-excluded)
    /**
     * Ground-truth vertical acceleration, m/s^2, INCLUDING gravity.
     *
     * A real MPU-6050 reads ~+9.81 m/s^2 on its vertical axis at rest. This
     * channel previously excluded gravity entirely, which made trunk lean a
     * no-op: with no gravity there is nothing for a tilt to project.
     */
    double trueAy = 0.0;
    /**
     * Ground-truth lateral (mediolateral) acceleration, m/s^2.
     *
     * Carries the two turn-synchronous mechanisms that a two-axis model could
     * not represent at all: centripetal acceleration a_c = v * omega_z, and the
     * component of gravity that trunk lean rotates out of vertical,
     * g * sin(theta_lean). Both act along the same physical direction during a
     * turn and sum.
     */
    double trueAz = 0.0;
    double trueOmegaZ = 0.0; //!< Ground-truth yaw rate, rad/s

    /**
     * Instantaneous trunk lean angle, rad. Exposed for logging and diagnostics
     * only -- nothing downstream consumes it. The lean's effect reaches the
     * pipeline through trueAy and trueAz, not through this field.
     */
    double trueThetaLean = 0.0;

    double biasAx = 0.0; //!< Slowly-varying accelerometer x bias (Gauss-Markov), m/s^2
    double biasAy = 0.0; //!< Slowly-varying accelerometer y bias (Gauss-Markov), m/s^2
    double biasAz = 0.0; //!< Slowly-varying accelerometer z bias (Gauss-Markov), m/s^2
    double biasGz = 0.0; //!< Slowly-varying gyroscope z bias (Gauss-Markov), rad/s

    double measAx = 0.0; //!< Accelerometer x measurement, Eq. (3.11), m/s^2
    double measAy = 0.0; //!< Accelerometer y measurement, Eq. (3.11), m/s^2
    double measAz = 0.0; //!< Accelerometer z measurement, Eq. (3.11), m/s^2
    double measGz = 0.0; //!< Gyroscope z measurement, Eq. (3.12), rad/s
};

/**
 * @ingroup lorawan
 *
 * MPU-6050 accelerometer and gyroscope measurement model (Step 0).
 *
 * Implements the two measurement equations
 * \verbatim
 *   a~ = a + b_a + n_a   (Eq. 3.11)
 *   w~ = w + b_g + n_g   (Eq. 3.12)
 * \endverbatim
 * with first-order Gauss-Markov bias evolution
 * \verbatim
 *   b_k = b_(k-1) * exp(-dt/tau) + w,  w ~ N(0, sigma^2 * (1 - exp(-2*dt/tau)))
 * \endverbatim
 * and white noise discretized as sigma_discrete = density / sqrt(dt).
 *
 * @see ImuSensorParams
 */
class ImuSensorSubmodule
{
  public:
    /**
     * Constructor.
     *
     * @param params Sensor model parameters.
     * @param normalRv Normal random variable stream used to draw both the bias
     *                 driving noise and the white measurement noise. Sharing a
     *                 single stream keeps the whole sensor model reproducible
     *                 from one ns-3 RNG substream.
     */
    ImuSensorSubmodule(const ImuSensorParams& params, Ptr<NormalRandomVariable> normalRv);

    /**
     * Advance the sensor model by one sample tick.
     *
     * @param s State to advance, updated in place.
     * @param dt Tick duration, s (nominally 1/sampleRateHz).
     * @param trueAx Ground-truth anterior-posterior acceleration, m/s^2.
     * @param trueAy Ground-truth vertical acceleration INCLUDING gravity, m/s^2.
     * @param trueAz Ground-truth lateral acceleration, m/s^2.
     * @param trueOmegaZ Ground-truth yaw rate, rad/s.
     * @param trueThetaLean Instantaneous trunk lean, rad. Recorded for logging
     *                      only; its physical effect must already be present in
     *                      trueAy and trueAz.
     */
    void Step(ImuSensorState& s,
              double dt,
              double trueAx,
              double trueAy,
              double trueAz,
              double trueOmegaZ,
              double trueThetaLean = 0.0);

  private:
    /**
     * Advance the Gauss-Markov accelerometer and gyroscope biases by one tick.
     *
     * @param s State to advance, updated in place.
     * @param dt Tick duration, s.
     */
    void StepBiases(ImuSensorState& s, double dt);

    /**
     * Form the noisy measurements from the ground truth and current biases.
     *
     * @param s State to update, updated in place.
     * @param dt Tick duration, s, used to discretize the noise densities.
     */
    void SampleMeasurements(ImuSensorState& s, double dt);

    /**
     * Draw one truncated normal sample.
     *
     * @param sigma Standard deviation. A non-positive value returns 0.
     * @param boundSigmas Truncation bound, expressed in multiples of sigma.
     * @return A zero-mean sample bounded to +/- boundSigmas * sigma.
     */
    double SampleNormal(double sigma, double boundSigmas) const;

    ImuSensorParams m_params;             //!< Sensor model parameters
    Ptr<NormalRandomVariable> m_normalRv; //!< Normal RNG stream for noise and bias driving terms
};

} // namespace lorawan
} // namespace ns3

#endif // LORA_IMU_SENSOR_SUBMODULE_H
