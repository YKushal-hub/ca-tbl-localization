#include "lora-wall-material-submodule.h"

namespace ns3 {

WallMaterialSubmodule::WallMaterialSubmodule(const WallMaterialParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void WallMaterialSubmodule::Compute(LinkPhysicsState& state) {
    if (!m_params.enabled || !state.isIndoor) {
        state.wallPenetrationDb = 0.0;
        return;
    }

    // Determine wall material based on environment
    double loss = 0.0;
    if (m_env == ENV_CONCRETE_BUNKER) {
        loss = m_sampler.Sample(m_params.concreteLossDb);
    } else if (m_env == ENV_INDUSTRIAL) {
        loss = m_sampler.Sample(m_params.metalLossDb);
    } else if (m_env == ENV_INDOOR_OFFICE) {
        // 50% glass, 50% wood/plaster
        double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        if (u < 0.5) {
            loss = m_sampler.Sample(m_params.glassLossDb);
        } else {
            loss = m_sampler.Sample(m_params.woodLossDb);
        }
    } else {
        // Default indoor mix
        double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        if (u < 0.33) {
            loss = m_sampler.Sample(m_params.woodLossDb);
        } else if (u < 0.66) {
            loss = m_sampler.Sample(m_params.glassLossDb);
        } else {
            loss = m_sampler.Sample(m_params.concreteLossDb);
        }
    }

    state.wallPenetrationDb = loss;
}

} // namespace ns3
