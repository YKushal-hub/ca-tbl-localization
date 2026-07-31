#include "lora-tunnel-submodule.h"

namespace ns3 {

TunnelSubmodule::TunnelSubmodule(const TunnelParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void TunnelSubmodule::Compute(LinkPhysicsState& state, double distanceM) {
    if (!m_params.enabled || m_env != ENV_TUNNEL_CONFINED) {
        state.tunnelLossDb = 0.0;
        return;
    }

    // Tunnel waveguide effect is already modeled by PLE=1.3 in PropagationSubmodule.
    // Adding it here again causes double counting and massive loss.
    state.tunnelLossDb = 0.0;
}

} // namespace ns3
