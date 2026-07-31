#ifndef LORA_NODE_POSITION_UNCERTAINTY_SUBMODULE_H
#define LORA_NODE_POSITION_UNCERTAINTY_SUBMODULE_H

#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/core-module.h"

namespace ns3 {

class NodePositionUncertaintySubmodule {
public:
    NodePositionUncertaintySubmodule(const NodePositionUncertaintyParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler);
    void Compute(LinkPhysicsState& state, double trueRange);

private:
    NodePositionUncertaintyParams m_params;
    EnvironmentClass m_env;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_NODE_POSITION_UNCERTAINTY_SUBMODULE_H
