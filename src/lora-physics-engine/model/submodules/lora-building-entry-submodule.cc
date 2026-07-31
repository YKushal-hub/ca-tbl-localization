#include "lora-building-entry-submodule.h"

namespace ns3 {

BuildingEntrySubmodule::BuildingEntrySubmodule(const BuildingEntryParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void BuildingEntrySubmodule::Compute(LinkPhysicsState& state) {
    if (!m_params.enabled) {
        state.isIndoor = false;
        state.buildingEntryDb = 0.0;
        return;
    }

    if (m_env == ENV_INDOOR_OFFICE || m_env == ENV_CONCRETE_BUNKER || m_env == ENV_INDUSTRIAL) {
        state.isIndoor = true;
    } else {
        state.isIndoor = false;
    }

    if (state.isIndoor) {
        // ITU-R P.2109-2 typical building entry loss is ~12-15 dB lognormal
        state.buildingEntryDb = m_sampler.Sample(DistributionParam::Normal(12.0, 4.0, 0.0, 30.0));
    } else {
        state.buildingEntryDb = 0.0;
    }
}

} // namespace ns3
