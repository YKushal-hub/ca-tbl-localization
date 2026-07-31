#ifndef LORA_MOBILITY_SUBMODULE_H
#define LORA_MOBILITY_SUBMODULE_H

#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Computes SNR degradation penalty due to Doppler shift under high mobility
 * f_D = velocity * frequencyHz / c
 * f_D_max = bandwidthHz / (2 * 2^SF)
 * If f_D > f_D_max: penalty = dopplerSNRPenaltyDbPerHz * (f_D - f_D_max)
 */
double ComputeDopplerPenaltyDb(
    double velocityMps, uint8_t spreadingFactor, double bandwidthHz,
    double frequencyHz, const MobilityParams& params,
    LoRaDistributionSampler& sampler);

} // namespace ns3

#endif // LORA_MOBILITY_SUBMODULE_H
