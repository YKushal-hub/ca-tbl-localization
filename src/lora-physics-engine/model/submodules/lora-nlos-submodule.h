#ifndef LORA_NLOS_SUBMODULE_H
#define LORA_NLOS_SUBMODULE_H

#include "ns3/vector.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Computes Non-Line-of-Sight (NLOS) path obstruction and penetration losses
 * P_NLOS(d) = 1 - exp(-d / scale)
 * Penalty = nlosAttenuation + diffractionLoss * (u / P_NLOS)
 * Note: Snow weather state is sampled by AtmosphericSubmodule; NLOS applies weather scaling
 */
double ComputeNLOSPenaltyDb(
    Vector txPos, Vector rxPos,
    const NLOSParams& params, LoRaDistributionSampler& sampler);

} // namespace ns3

#endif // LORA_NLOS_SUBMODULE_H
