#include "lora-floor-attenuation-submodule.h"

namespace ns3 {

FloorAttenuationSubmodule::FloorAttenuationSubmodule(const FloorAttenuationParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void FloorAttenuationSubmodule::Compute(LinkPhysicsState& state) {
    if (!m_params.enabled || !state.isIndoor) {
        state.floorAttenuationDb = 0.0;
        return;
    }

    // Determine number of floors randomly based on environment
    // For office, maybe 1-5 floors difference. For bunker, 1-2.
    int nFloors = 0;
    if (m_env == ENV_INDOOR_OFFICE) {
        double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        nFloors = (int)(u * 6.0); // 0 to 5
    } else if (m_env == ENV_CONCRETE_BUNKER) {
        double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        nFloors = (int)(u * 3.0); // 0 to 2
    } else if (m_env == ENV_INDUSTRIAL) {
        nFloors = 0; // Usually large open halls, no intermediate floors
    } else {
        // Default
        double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        nFloors = (int)(u * 2.0); // 0 or 1
    }

    if (nFloors > 0) {
        double lossPerFloor = m_sampler.Sample(m_params.lossPerFloorDb);
        state.floorAttenuationDb = lossPerFloor * nFloors;
    } else {
        state.floorAttenuationDb = 0.0;
    }
}

} // namespace ns3
