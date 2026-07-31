#ifndef LORA_TUNNEL_SUBMODULE_H
#define LORA_TUNNEL_SUBMODULE_H

#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/core-module.h"

namespace ns3 {

class TunnelSubmodule {
public:
    TunnelSubmodule(const TunnelParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler);
    void Compute(LinkPhysicsState& state, double distanceM);

private:
    TunnelParams m_params;
    EnvironmentClass m_env;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_TUNNEL_SUBMODULE_H
