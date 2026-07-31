#ifndef LORA_WALL_MATERIAL_SUBMODULE_H
#define LORA_WALL_MATERIAL_SUBMODULE_H

#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/core-module.h"

namespace ns3 {

class WallMaterialSubmodule {
public:
    WallMaterialSubmodule(const WallMaterialParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler);
    void Compute(LinkPhysicsState& state);

private:
    WallMaterialParams m_params;
    EnvironmentClass m_env;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_WALL_MATERIAL_SUBMODULE_H
