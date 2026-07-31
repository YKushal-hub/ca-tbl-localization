#ifndef LORA_PROPAGATION_SUBMODULE_H
#define LORA_PROPAGATION_SUBMODULE_H

#include "ns3/vector.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Computes large-scale path loss with distance and height corrections
 * Friis reference at 1m: 20*log10(4*pi*d0*f/c)
 * Log-distance: FSPL_d0 + 10*n*log10(d/d0)
 * Hata correction: -13.82 * log10(hb) for gateway heights > 30m
 */
double ComputePathLossDb(
    Vector txPosition, Vector rxPosition,
    double txHeightM, double rxHeightM,
    const PropagationParams& params,
    LoRaDistributionSampler& sampler);

} // namespace ns3

#endif // LORA_PROPAGATION_SUBMODULE_H
