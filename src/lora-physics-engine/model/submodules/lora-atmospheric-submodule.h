#ifndef LORA_ATMOSPHERIC_SUBMODULE_H
#define LORA_ATMOSPHERIC_SUBMODULE_H

#include "ns3/nstime.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Computes weather-related atmospheric absorption and thermal noise floor shift
 * Specific Rain Attenuation (ITU-R P.838-3): gamma_R = k * R^alpha
 * Specific Water Vapor: gamma_wv = 0.05 * waterVapor / 7.5
 * Noise Shift: 10 * log10((T + 273.15) / 290.15)
 * Feature 3: Atmospheric pressure and tropospheric refraction (ITU-R P.453-14)
 */
class AtmosphericSubmodule {
public:
    AtmosphericSubmodule(const AtmosphericParams& params,
                          LoRaDistributionSampler& sampler);
    
    double ComputeAtmosphericLossDb(double distanceKm, Time currentTime);
    double ComputeThermalNoiseShiftDb(Time currentTime);
    void UpdateWeatherState(Time currentTime);
    
    // Feature 3: Atmospheric refractivity and k-factor calculations
    double ComputeRefractivityN(Time currentTime);
    double ComputeEffectiveEarthRadiusKFactor(Time currentTime);
    
    // Feature 4: Snow precipitation model
    double ComputeSnowAttenuationDb(double snowfallRateMmHr, double snowTemperatureCelsius, double distanceKm);
    
    // Feature 8: Humidity dielectric effect on wave propagation
    double ComputeHumidityDielectricEffect(Time currentTime);
    
    WeatherState GetCurrentState() const {
        return m_currentState;
    }
    
    double GetCurrentTemperature() const {
        return m_currentTemperature;
    }
    
    double GetCurrentRainRate() const {
        return m_currentRainRateMmPerHr;
    }

private:
    WeatherState m_currentState;
    Time m_lastWeatherUpdate;
    double m_currentRainRateMmPerHr;
    double m_currentFogAttenuationDbPerKm;
    double m_currentTemperature;
    double m_currentWaterVaporDensityGPerM3;
    double m_currentAtmosphericPressureHPa;  // Feature 3
    double m_currentSnowfallRateMmHr;  // Feature 4
    double m_currentSnowTemperatureCelsius;  // Feature 4
    
    const AtmosphericParams& m_params;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_ATMOSPHERIC_SUBMODULE_H
