#ifndef LORA_FADING_SUBMODULE_H
#define LORA_FADING_SUBMODULE_H

#include "ns3/nstime.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Computes small-scale fading loss (Rayleigh, Rician, Nakagami-m)
 * Updates fading value after coherence time has elapsed
 * Rayleigh: r = sqrt(X^2 + Y^2), X, Y ~ N(0, 1/sqrt(2))
 * Rician: r = sqrt((X + nu)^2 + Y^2), nu = sqrt(K/(K+1)), sigma = sqrt(1/(2*(K+1)))
 * Nakagami: r = sqrt(Y), Y ~ Gamma(m, 1/m)
 */
double ComputeFadingDb(
    LinkPhysicsState& state, Time currentTime,
    const FadingParams& params, LoRaDistributionSampler& sampler,
    uint64_t linkId, Time& lastFadingUpdate, double& lastFadingValue);

void ResetFadingState();

} // namespace ns3

#endif // LORA_FADING_SUBMODULE_H
