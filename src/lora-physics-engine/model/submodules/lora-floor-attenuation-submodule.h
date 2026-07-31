#ifndef LORA_FLOOR_ATTENUATION_SUBMODULE_H
#define LORA_FLOOR_ATTENUATION_SUBMODULE_H

#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/core-module.h"

namespace ns3 {

class FloorAttenuationSubmodule {
public:
    FloorAttenuationSubmodule(const FloorAttenuationParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler);
    void Compute(LinkPhysicsState& state);

private:
    FloorAttenuationParams m_params;
    EnvironmentClass m_env;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_FLOOR_ATTENUATION_SUBMODULE_H
