#ifndef LORA_TEMPORAL_BLOCKAGE_SUBMODULE_H
#define LORA_TEMPORAL_BLOCKAGE_SUBMODULE_H

#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/core-module.h"

namespace ns3 {

class TemporalBlockageSubmodule {
public:
    TemporalBlockageSubmodule(const TemporalBlockageParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler);
    void Compute(LinkPhysicsState& state);

private:
    TemporalBlockageParams m_params;
    EnvironmentClass m_env;
    LoRaDistributionSampler& m_sampler;
    std::map<uint32_t, bool> m_blockageState;
};

} // namespace ns3

#endif // LORA_TEMPORAL_BLOCKAGE_SUBMODULE_H
