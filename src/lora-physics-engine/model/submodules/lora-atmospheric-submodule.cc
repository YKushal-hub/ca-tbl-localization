#include "lora-atmospheric-submodule.h"
#include <cmath>

namespace ns3 {

AtmosphericSubmodule::AtmosphericSubmodule(const AtmosphericParams& params,
                                           LoRaDistributionSampler& sampler)
    : m_currentState(params.initialWeatherState),
      m_lastWeatherUpdate(Time(0)),
      m_currentRainRateMmPerHr(0.0),
      m_currentFogAttenuationDbPerKm(0.0),
      m_currentTemperature(17.0),
      m_currentWaterVaporDensityGPerM3(7.5),
      m_currentAtmosphericPressureHPa(1013.0),
      m_params(params),
      m_sampler(sampler)
{
}

void AtmosphericSubmodule::UpdateWeatherState(Time currentTime)
{
    if (!m_params.enabled) {
        m_currentRainRateMmPerHr = 0.0;
        m_currentFogAttenuationDbPerKm = 0.0;
        m_currentTemperature = 17.0; // Reference room temperature in C
        m_currentWaterVaporDensityGPerM3 = 7.5;
        m_currentAtmosphericPressureHPa = 1013.0;
        return;
    }

    bool isFirst = (m_lastWeatherUpdate == Time(0));
    bool isExpired = (currentTime - m_lastWeatherUpdate).GetSeconds() >= m_params.weatherUpdateIntervalS;
    // --rngKeyed: weather is a function of time alone -- one state per update epoch
    // floor(t / interval), drawn on its own keyed sub-sequence -- instead of being re-drawn
    // inside whichever link evaluation happens to arrive first (TECH_DEBT 1.1, plan v3 V1).
    int64_t epoch = -1;
    if (LoRaDistributionSampler::KeyedMode())
    {
        const double iv = std::max(1e-9, m_params.weatherUpdateIntervalS);
        epoch = static_cast<int64_t>(std::floor(currentTime.GetSeconds() / iv));
        isFirst = (m_keyedWeatherEpoch < 0);
        isExpired = (epoch != m_keyedWeatherEpoch);
        if (isFirst || isExpired)
        {
            m_keyedWeatherEpoch = epoch;
            m_sampler.BeginSubContext(0x5745415448455200ULL, static_cast<uint64_t>(epoch));
        }
    }

    if (isFirst || isExpired) {
        m_currentRainRateMmPerHr = m_sampler.Sample(m_params.rainRateMmPerHr);
        m_currentFogAttenuationDbPerKm = m_sampler.Sample(m_params.fogAttenuationDbPerKm);
        m_currentTemperature = m_sampler.Sample(m_params.temperatureCelsius);
        m_currentWaterVaporDensityGPerM3 = m_sampler.Sample(m_params.waterVaporDensityGPerM3);
        
        // Feature 3: Sample atmospheric pressure
        if (m_params.atmosphericPressure.enableTroposphericRefraction) {
            m_currentAtmosphericPressureHPa = m_sampler.Sample(m_params.atmosphericPressure.atmosphericPressureHPa);
        } else {
            m_currentAtmosphericPressureHPa = 1013.0;  // Standard pressure
        }
        
        // Feature 4: Sample snow parameters
        if (m_params.snow.enabled) {
            m_currentSnowfallRateMmHr = m_sampler.Sample(m_params.snow.snowfallRateMmHrEquiv);
            m_currentSnowTemperatureCelsius = m_sampler.Sample(m_params.snow.snowTemperatureCelsius);
        } else {
            m_currentSnowfallRateMmHr = 0.0;
            m_currentSnowTemperatureCelsius = 0.0;
        }
        
        m_lastWeatherUpdate = currentTime;
        if (epoch >= 0)
        {
            m_sampler.EndSubContext();
        }
    }
}

double AtmosphericSubmodule::ComputeAtmosphericLossDb(double distanceKm, Time currentTime)
{
    UpdateWeatherState(currentTime);

    if (!m_params.enabled) {
        return 0.0;
    }

    // 1. Rain specific attenuation (ITU-R P.838-3 specific attenuation model, 2005)
    // gamma_R = k * R^alpha where k=0.000065, alpha=0.7 for 868 MHz (horiz. pol.)
    double k = m_params.rainAttenuationK;
    double alpha = m_params.rainAttenuationAlpha;
    double gamma_R = k * std::pow(m_currentRainRateMmPerHr, alpha);
    double rain_loss = gamma_R * distanceKm;

    // 2. Specific water vapor density absorption (approximate empirical model)
    // gamma_wv = 0.05 * waterVapor / 7.5 dB/km
    double gamma_wv = 0.05 * (m_currentWaterVaporDensityGPerM3 / 7.5);
    double wv_loss = gamma_wv * distanceKm;

    // 3. Fog specific attenuation (scaled down by 1e-3 for 868 MHz European ISM band)
    double fog_loss = m_currentFogAttenuationDbPerKm * distanceKm * 1e-3;

    // 4. Snow/ice precipitation attenuation (Mätzler 1987), kept in the atmospheric path.
    double snow_loss = 0.0;
    if (m_params.snow.enabled) {
        snow_loss = ComputeSnowAttenuationDb(
            m_currentSnowfallRateMmHr,
            m_currentSnowTemperatureCelsius,
            distanceKm);
    }

    // 5. Total Atmospheric Loss
    double totalLoss = rain_loss + wv_loss + fog_loss + snow_loss;
    if (totalLoss < 0.0) {
        totalLoss = 0.0;
    }
    return totalLoss;
}

double AtmosphericSubmodule::ComputeThermalNoiseShiftDb(Time currentTime)
{
    UpdateWeatherState(currentTime);

    if (!m_params.enabled) {
        return 0.0;
    }

    // shift = 10 * log10((T + 273.15) / 290.15) relative to standard reference 290.15K (17 deg C)
    double T_kelvin = m_currentTemperature + 273.15;
    double T_ref = 290.15; // standard ITU noise reference temperature
    double shift = 10.0 * std::log10(T_kelvin / T_ref);
    return shift; // Can be positive or negative
}

// Feature 3: Tropospheric refractivity computation (ITU-R P.453-14)
// N = (n-1) * 1e6 where n is refractive index
// ITU-R P.453-14 formula: N = 77.6 * P/T + 3.73e5 * e/T^2
// P = pressure in hPa, T = absolute temperature in K, e = water vapor partial pressure in hPa
double AtmosphericSubmodule::ComputeRefractivityN(Time currentTime)
{
    UpdateWeatherState(currentTime);
    
    if (!m_params.atmosphericPressure.enableTroposphericRefraction) {
        return 0.0;  // No refraction effect
    }
    
    // Sample atmospheric pressure if enabled
    double pressureHPa = m_sampler.Sample(m_params.atmosphericPressure.atmosphericPressureHPa);
    m_currentAtmosphericPressureHPa = pressureHPa;
    
    double T_kelvin = m_currentTemperature + 273.15;
    
    // Compute water vapor partial pressure (simplified Magnus approximation)
    // RH = 50% nominal for mid-latitude; e_s = 6.112 * exp((17.67 * T) / (T + 243.5)) hPa
    double e_s = 6.112 * std::exp((17.67 * m_currentTemperature) / (m_currentTemperature + 243.5));
    double relativeHumidity = 0.50;  // Default 50% RH
    double e = relativeHumidity * e_s;
    
    // ITU-R P.453-14 refractivity formula: N = 77.6 * P/T + 3.73e5 * e/T^2
    // Note: water vapor term has T^2 in denominator (not T)
    double N = (77.6 * pressureHPa / T_kelvin) + (3.73e5 * e / (T_kelvin * T_kelvin));
    
    return N;
}

// Feature 3: Effective earth radius k-factor computation
// k-factor = 4/3 for standard refraction; varies with refractivity gradient
// k = (1 + (radius / dn/dh)^-1)^-1
double AtmosphericSubmodule::ComputeEffectiveEarthRadiusKFactor(Time currentTime)
{
    double N = ComputeRefractivityN(currentTime);
    
    if (!m_params.atmosphericPressure.enableTroposphericRefraction || N == 0.0) {
        return 1.333;  // Standard k-factor (4/3)
    }
    
    // Simplified model: k-factor inversely proportional to refractivity gradient
    // Standard N ~ 315 N-units at sea level; dn/dh ~ -39 N-units/km typical
    // k = 1 + (1 / (-(dn/dh) * radius))
    double dn_dh = -39.0;  // Typical atmospheric refractivity gradient N-units/km
    double earthRadiusKm = 6371.0;
    
    double k = 1.0 + 1.0 / ((-dn_dh) * earthRadiusKm);
    
    // Clamp k-factor to reasonable bounds (0.5 to 10)
    if (k < 0.5) k = 0.5;
    if (k > 10.0) k = 10.0;
    
    return k;
}

// Feature 8: Humidity dielectric effect (ITU-R P.676-12)
// Humid air has different dielectric constant than dry air, affecting propagation velocity and reflection
double AtmosphericSubmodule::ComputeHumidityDielectricEffect(Time currentTime)
{
    if (!m_params.humidityDielectric.enableHumidityDielectric) {
        return 0.0;  // No humidity dielectric effect
    }
    
    UpdateWeatherState(currentTime);
    
    // Dielectric constant of dry air: ~1.000264
    // Dielectric constant of water vapor relative to air varies with humidity and temperature
    // Complex permittivity model (simplified):
    // Phase velocity in humid air is reduced compared to free space
    // This causes phase shift and slight path delay variation
    
    double T_celsius = m_currentTemperature;
    double T_kelvin = T_celsius + 273.15;
    
    // Compute saturation vapor pressure (Magnus approximation)
    double e_s = 6.112 * std::exp((17.67 * T_celsius) / (T_celsius + 243.5));
    
    // Relative humidity estimated from water vapor density
    // Water vapor density = (RH * e_s * M) / (R * T)
    // where M = 18.015 g/mol (H2O), R = 8.314 J/(mol·K)
    // Simplified: RH ~ (vapor_density / 7.5) for mid-latitude 20°C standard
    double estimatedRH = std::min(1.0, m_currentWaterVaporDensityGPerM3 / 7.5);
    
    // Dielectric effect on refractive index (simplified ITU-R model)
    // Additional refractivity due to humidity: 4810 * e / T (already in refractivity)
    // Phase shift coefficient (empirical): 0.5 rad per 50% RH increase
    double humidityPhaseShiftRad = 0.5 * (estimatedRH - 0.5) * 2.0;  // [-0.5, +0.5] rad range
    
    return humidityPhaseShiftRad;
}

// Feature 4: Snow/ice precipitation attenuation (Mätzler 1987)
// Wet snow: L_wet_snow = 0.0078 * R_snow^0.72 dB/km
// Dry snow: L_dry_snow = 0.0003 * R_snow^1.6 dB/km
// Reference: Mätzler et al. 1987; scitepress 111026
double AtmosphericSubmodule::ComputeSnowAttenuationDb(double snowfallRateMmHr, 
                                                       double snowTemperatureCelsius, 
                                                       double distanceKm)
{
    if (snowfallRateMmHr <= 0.0 || distanceKm <= 0.0) {
        return 0.0;
    }
    
    // Mätzler formula: attenuation depends on snow type (wet vs dry) based on temperature
    // Wet snow (-5 to 0°C): L_wet = 0.0078 * R^0.72 dB/km
    // Dry snow (< -10°C): L_dry = 0.0003 * R^1.6 dB/km
    // Mixed (-10 to -5°C): interpolate between the two
    
    double specificAttenuationDbPerKm = 0.0;
    
    if (snowTemperatureCelsius >= -5.0) {
        // Wet snow regime
        specificAttenuationDbPerKm = 0.0078 * std::pow(snowfallRateMmHr, 0.72);
    } else if (snowTemperatureCelsius < -10.0) {
        // Dry snow regime
        specificAttenuationDbPerKm = 0.0003 * std::pow(snowfallRateMmHr, 1.6);
    } else {
        // Mixed regime: linear interpolation between -10°C (dry) and -5°C (wet)
        double alpha = (snowTemperatureCelsius + 10.0) / 5.0;  // [0, 1]
        double wet = 0.0078 * std::pow(snowfallRateMmHr, 0.72);
        double dry = 0.0003 * std::pow(snowfallRateMmHr, 1.6);
        specificAttenuationDbPerKm = (1.0 - alpha) * dry + alpha * wet;
    }
    
    double totalAttenuationDb = specificAttenuationDbPerKm * distanceKm;
    return totalAttenuationDb;
}

} // namespace ns3
