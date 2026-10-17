#ifndef LORA_FOLIAGE_SUBMODULE_H
#define LORA_FOLIAGE_SUBMODULE_H

#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include <map>

namespace ns3 {

//! B3f --foliageDepthClamp: foliage evaluations whose drawn depth was clamped to the link length.
extern uint64_t g_foliageDepthClampFirings;


/**
 * @brief Computes vegetation and foliage attenuation loss (ITU-R P.833-10)
 * A_veg = A_max * (1 - exp(-depth * gamma / A_max))
 * If winter: specific attenuation gamma is scaled down
 * If wet foliage: additional loss is added to the total
 * Feature 5: Wind-induced dynamic foliage variation (Ornstein-Uhlenbeck process)
 */
/**
 * C4 --foliageP833 (scope 2026-10-17). ITU-R P.833-10 (09/2021) Annex 1 section 2.1, terrestrial path with one
 * terminal in woodland, mixed coniferous-deciduous forest near St. Petersburg (mean tree height 16 m):
 *  - specific attenuation gamma (dB/m): Table 1 (105.9, 466.475, 949.0, 1852.2, 2117.5 MHz -> 0.04, 0.12, 0.17,
 *    0.30, 0.34), log-log interpolated; outside the table the end segment's log-log slope is extended;
 *  - maximum attenuation Am (dB): eq. (2), Am = A1 f^alpha with A1 = 1.37 dB, alpha = 0.42 (f in MHz), the same
 *    St. Petersburg forest-park measurements (105.9-2117.5 MHz).
 * The Recommendation's own caveat applies: "values ... should be viewed as only typical".
 */
double P833GammaDbPerM(double fMHz);
double P833AmDb(double fMHz);
//! Sets the gamma and Am means of a forest FoliageParams (light, dense, tropical) to the P.833 values at fMHz,
//! keeping the relative spreads and bounds. Other vegetation types are left unchanged. Returns true if it set them.
bool ApplyP833Forest(FoliageParams& f, double fMHz);

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
