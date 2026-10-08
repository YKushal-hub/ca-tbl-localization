#ifndef LORA_FOLIAGE_SUBMODULE_H
#define LORA_FOLIAGE_SUBMODULE_H

#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include <map>

namespace ns3 {

/**
 * @brief Computes vegetation and foliage attenuation loss (ITU-R P.833-10)
 * A_veg = A_max * (1 - exp(-depth * gamma / A_max))
 * If winter: specific attenuation gamma is scaled down
 * If wet foliage: additional loss is added to the total
 * Feature 5: Wind-induced dynamic foliage variation (Ornstein-Uhlenbeck process)
 */
double ComputeFoliageLossDb(
    Vector txPos, Vector rxPos,
    const FoliageParams& params, LoRaDistributionSampler& sampler,
    double rainRateMmHr, bool isWinterSeason);

// Feature 5: Wind-induced dynamic foliage variation result
struct FoliageVariationResult {
    double staticLossDb;      // Base foliage loss (unchanged)
    double dynamicLossDb;     // Wind-induced variation component (OU process)
    double coherenceFactorDb; // Link coherence (spatial correlation of wind effects)
};

// Feature 5: Compute dynamic wind-induced foliage loss variation (Cheffena & Ekman, EURASIP JWCN 2009; values OURS)
// Uses Ornstein-Uhlenbeck process: new = rho*old + sqrt(1-rho²)*σ*N(0,1)
// where rho = exp(-dt / T_wind) and T_wind = depth_m / (wind_speed_mps + 1)
FoliageVariationResult ComputeDynamicFoliageLossDb(
    Vector txPos, Vector rxPos,
    const FoliageParams& params,
    const WindVariationParams& windParams,
    LoRaDistributionSampler& sampler,
    double rainRateMmHr, bool isWinterSeason,
    Time currentTime);

} // namespace ns3

#endif // LORA_FOLIAGE_SUBMODULE_H
