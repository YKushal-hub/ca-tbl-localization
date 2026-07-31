#include "lora-soil-attenuation-submodule.h"

namespace ns3 {

SoilAttenuationSubmodule::SoilAttenuationSubmodule(const SoilAttenuationParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void SoilAttenuationSubmodule::Compute(LinkPhysicsState& state) {
    // Only WUSN (Wireless Underground Sensor Networks) scenarios have soil attenuation
    // We can proxy this by checking if the node is "buried"
    // Let's assume nodes are buried 10% of the time in agricultural/rural environments if this feature is on
    
    if (!m_params.enabled || (m_env != ENV_RURAL_OPEN && m_env != ENV_RURAL_FORESTED)) {
        state.soilAttenuationDb = 0.0;
        return;
    }

    double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
    if (u < 0.1) {
        double depthM = m_sampler.Sample(m_params.burialDepthM);
        double moisture = m_sampler.Sample(m_params.soilMoisturePercent);
        
        // Simple empirical model: Attenuation is highly dependent on volumetric water content (VWC)
        // Typical LoRa soil attenuation at 868 MHz: ~15-30 dB/meter for dry soil, up to 100+ dB/meter for wet
        double attenPerMeterDb = 15.0 + 2.5 * moisture;
        
        state.soilAttenuationDb = depthM * attenPerMeterDb;
    } else {
        state.soilAttenuationDb = 0.0;
    }
}

} // namespace ns3
