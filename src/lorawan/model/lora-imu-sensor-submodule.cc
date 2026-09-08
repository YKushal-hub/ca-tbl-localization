/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "lora-imu-sensor-submodule.h"

#include <algorithm>
#include <cmath>

namespace ns3
{
namespace lorawan
{

ImuSensorSubmodule::ImuSensorSubmodule(const ImuSensorParams& params,
                                       Ptr<NormalRandomVariable> normalRv)
    : m_params(params),
      m_normalRv(normalRv)
{
}

double
ImuSensorSubmodule::SampleNormal(double sigma, double boundSigmas) const
{
    if (sigma <= 0.0)
    {
        return 0.0;
    }
    // NormalRandomVariable takes a variance and a bound that is symmetric
    // about the mean, which matches the +/- N-sigma truncation used here.
    return m_normalRv->GetValue(0.0, sigma * sigma, boundSigmas * sigma);
}

// First-order Gauss-Markov bias evolution. See doc/imu-shs-design.md Section 4
// for the closed form and its citation.
void
ImuSensorSubmodule::StepBiases(ImuSensorState& s, double dt)
{
    const auto& p = m_params;

    double phiA = std::exp(-dt / p.accelBiasTimeConstantS);
    double qA = p.accelBiasInstability * p.accelBiasInstability * (1.0 - phiA * phiA);
    double sigA = std::sqrt(std::max(qA, 0.0));
    // All three accelerometer axes share the same noise density and bias
    // instability. There is no reason to expect the MPU-6050's third axis
    // behaves differently from the other two, and no per-axis accelerometer
    // figures were found -- Rudyk et al. measured the gyroscope per axis, not
    // the accelerometer.
    s.biasAx = s.biasAx * phiA + SampleNormal(sigA, 4.0);
    s.biasAy = s.biasAy * phiA + SampleNormal(sigA, 4.0);
    s.biasAz = s.biasAz * phiA + SampleNormal(sigA, 4.0);

    double gInst = p.GyroBiasInstabilityRadS();
    double phiG = std::exp(-dt / p.gyroBiasTimeConstantS);
    double qG = gInst * gInst * (1.0 - phiG * phiG);
    double sigG = std::sqrt(std::max(qG, 0.0));
    s.biasGz = s.biasGz * phiG + SampleNormal(sigG, 4.0);
}

// Eq. (3.11)-(3.12). White noise scaled to the sample rate using the standard
// ARW/VRW discretization: sigma_discrete = density / sqrt(dt).
void
ImuSensorSubmodule::SampleMeasurements(ImuSensorState& s, double dt)
{
    const auto& p = m_params;
    double sigA = p.accelNoiseDensity / std::sqrt(dt);
    double sigG = p.gyroNoiseDensityRadS / std::sqrt(dt);

    s.measAx = s.trueAx + s.biasAx + SampleNormal(sigA, 6.0);
    s.measAy = s.trueAy + s.biasAy + SampleNormal(sigA, 6.0);
    s.measAz = s.trueAz + s.biasAz + SampleNormal(sigA, 6.0);
    s.measGz = s.trueOmegaZ + s.biasGz + SampleNormal(sigG, 6.0);

    // Saturate at the configured full-scale ranges. A real MPU-6050 cannot
    // report beyond AFS_SEL / FS_SEL, and silently letting the model exceed
    // them would understate the error of any manoeuvre that clips -- which is
    // exactly the case the range choice is supposed to be justified against.
    double accelLimit = p.AccelFullScaleMps2();
    double gyroLimit = p.GyroFullScaleRadS();
    s.measAx = std::clamp(s.measAx, -accelLimit, accelLimit);
    s.measAy = std::clamp(s.measAy, -accelLimit, accelLimit);
    s.measAz = std::clamp(s.measAz, -accelLimit, accelLimit);
    s.measGz = std::clamp(s.measGz, -gyroLimit, gyroLimit);
}

void
ImuSensorSubmodule::Step(ImuSensorState& s,
                         double dt,
                         double trueAx,
                         double trueAy,
                         double trueAz,
                         double trueOmegaZ,
                         double trueThetaLean)
{
    s.trueAx = trueAx;
    s.trueAy = trueAy;
    s.trueAz = trueAz;
    s.trueOmegaZ = trueOmegaZ;
    s.trueThetaLean = trueThetaLean;
    StepBiases(s, dt);
    SampleMeasurements(s, dt);
}

} // namespace lorawan
} // namespace ns3
