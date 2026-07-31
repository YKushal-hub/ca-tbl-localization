#include "lora-foliage-submodule.h"
#include <cmath>
#include <map>

namespace ns3 {

// Static cache for per-link dynamic foliage variation (survives across multiple calls for same link)
static std::map<std::pair<uint32_t, uint32_t>, std::pair<double, Time>> g_foliageDynamicCache;

double ComputeFoliageLossDb(
    Vector txPos, Vector rxPos,
    const FoliageParams& params, LoRaDistributionSampler& sampler,
    double rainRateMmHr, bool isWinterSeason)
{
    if (!params.enabled || params.type == VEG_NONE) {
        return 0.0;
    }

    // 1. Sample vegetation depth, specific attenuation, and max attenuation
    double depth = sampler.Sample(params.vegetationDepthM);
    double gamma = sampler.Sample(params.specificAttenuationDbPerM);
    double A_max = sampler.Sample(params.maxAttenuationDb);

    if (depth <= 0.0 || gamma <= 0.0 || A_max <= 0.0) {
        return 0.0;
    }

    // 2. Apply winter seasonal multiplier if active
    if (isWinterSeason) {
        double multiplier = sampler.Sample(params.seasonalMultiplier);
        gamma *= multiplier;
    }

    // 3. ITU-R P.833-10 foliage model:
    // A_veg = A_max * (1 - exp(-depth * gamma / A_max))
    double A_veg = A_max * (1.0 - std::exp(-depth * gamma / A_max));

    // 4. Add wet foliage attenuation proportionally to rain rate
    if (rainRateMmHr > 0.01) {
        double wetAdd = sampler.Sample(params.wetFoliageAddDb);
        // Empirical scaling: full wetAdd at ~10 mm/hr, partial at drizzle
        double multiplier = std::min(1.0, rainRateMmHr / 10.0);
        A_veg += (wetAdd * multiplier);
    }

    if (A_veg < 0.0) {
        A_veg = 0.0; // Constraint 6
    }
    return A_veg;
}

// Feature 5: Dynamic wind-induced foliage variation with per-link caching
// Uses Ornstein-Uhlenbeck process for time-correlated stochastic variation
// Reference: Meng et al. 2009 IEEE TAP
FoliageVariationResult ComputeDynamicFoliageLossDb(
    Vector txPos, Vector rxPos,
    const FoliageParams& params,
    const WindVariationParams& windParams,
    LoRaDistributionSampler& sampler,
    double rainRateMmHr, bool isWinterSeason,
    Time currentTime)
{
    FoliageVariationResult result;
    
    // Base static foliage loss (unchanged)
    result.staticLossDb = ComputeFoliageLossDb(txPos, rxPos, params, sampler, rainRateMmHr, isWinterSeason);
    result.dynamicLossDb = 0.0;
    result.coherenceFactorDb = 0.0;
    
    if (!windParams.windVariationEnabled) {
        return result;  // Wind variation disabled, return static only
    }
    
    // Feature 5: Wind-induced dynamic variation using Ornstein-Uhlenbeck process
    // Meng et al. 2009: foliage loss varies with wind-induced branch motion
    
    double windSpeed = sampler.Sample(windParams.windSpeedMps);
    double windAmplitude = sampler.Sample(windParams.windAmplitudeDb);
    
    // Vegetation depth affects coherence time
    double depth = sampler.Sample(params.vegetationDepthM);
    if (depth <= 0.0) depth = 10.0;  // Default 10m if invalid
    
    // Coherence time: T_wind = depth_m / (wind_speed_mps + 1)
    // Faster wind through denser vegetation → faster channel variation
    double coherenceTimeSec = depth / (windSpeed + 1.0);
    if (coherenceTimeSec < 0.5) coherenceTimeSec = 0.5;   // Min 0.5 sec
    if (coherenceTimeSec > 30.0) coherenceTimeSec = 30.0; // Max 30 sec
    
    // Create per-link identifier
    uint32_t linkHash = (uint32_t)(txPos.x + txPos.y + rxPos.x + rxPos.y) % 0xFFFFFFFF;
    auto cacheKey = std::make_pair(linkHash, 0U);
    
    // Check if cached value is still valid
    bool needNewSample = true;
    double previousDynamic = 0.0;
    Time previousTime = currentTime;
    
    if (g_foliageDynamicCache.count(cacheKey) > 0) {
        auto cachedEntry = g_foliageDynamicCache[cacheKey];
        previousDynamic = cachedEntry.first;
        previousTime = cachedEntry.second;
        double elapsedS = (currentTime - cachedEntry.second).GetSeconds();
        if (elapsedS < coherenceTimeSec) {
            needNewSample = false;
        }
    }
    
    double newDynamic = 0.0;
    
    if (needNewSample) {
        // Ornstein-Uhlenbeck process: new = rho * old + sqrt(1-rho²) * σ * N(0,1)
        // where rho = exp(-dt / T_wind)
        double dt = (currentTime - previousTime).GetSeconds();
        if (dt < 0.0) dt = 0.0;
        
        double rho = std::exp(-dt / coherenceTimeSec);
        double innovation = sampler.Sample(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0));
        double sqrtTerm = std::sqrt(std::max(0.0, 1.0 - rho * rho));
        
        // New value combines previous value with innovation scaled by wind amplitude
        newDynamic = rho * previousDynamic + sqrtTerm * windAmplitude * innovation;
        
        // Clamp to reasonable bounds (typically ±5 dB variation)
        if (newDynamic > 5.0) newDynamic = 5.0;
        if (newDynamic < -5.0) newDynamic = -5.0;
        
        g_foliageDynamicCache[cacheKey] = std::make_pair(newDynamic, currentTime);
    } else {
        newDynamic = previousDynamic;
    }
    
    result.dynamicLossDb = newDynamic;
    
    // Spatial coherence factor: neighboring links see correlated wind effects
    // Decorrelation distance ~100m for wind-induced foliage variation
    double distM = std::sqrt((txPos.x - rxPos.x) * (txPos.x - rxPos.x) +
                             (txPos.y - rxPos.y) * (txPos.y - rxPos.y));
    double decorrelationDistM = 100.0;
    double coherenceFactor = std::exp(-distM / decorrelationDistM);
    result.coherenceFactorDb = 20.0 * std::log10(coherenceFactor + 0.001);
    
    return result;
}

} // namespace ns3
