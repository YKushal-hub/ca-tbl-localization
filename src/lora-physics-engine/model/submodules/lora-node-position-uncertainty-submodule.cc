#include "lora-node-position-uncertainty-submodule.h"

namespace ns3 {

NodePositionUncertaintySubmodule::NodePositionUncertaintySubmodule(const NodePositionUncertaintyParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void NodePositionUncertaintySubmodule::Compute(LinkPhysicsState& state, double trueRange) {
    state.trueRangeM = trueRange;
    
    if (!m_params.enabled) {
        state.estimatedRangeM = trueRange;
        state.rangeErrorM = 0.0;
        return;
    }

    double errX = 0.0;
    double errY = 0.0;
    if (m_params.horizontalErrorM.type == DistributionParam::DETERMINISTIC) {
        double r = m_params.horizontalErrorM.mean;
        double theta = m_sampler.Sample(DistributionParam::Uniform(0, 2*M_PI));
        errX = r * std::cos(theta);
        errY = r * std::sin(theta);
    } else {
        double sigma = m_params.horizontalErrorM.mean;
        if (m_params.horizontalErrorM.type == DistributionParam::NORMAL) {
            sigma = m_params.horizontalErrorM.stddev;
        }
        if (sigma == 0) sigma = m_params.horizontalErrorM.mean;

        // 2D Gaussian offset
        errX = m_sampler.Sample(DistributionParam::Normal(0, sigma, -3*sigma, 3*sigma));
        errY = m_sampler.Sample(DistributionParam::Normal(0, sigma, -3*sigma, 3*sigma));
    }
    
    double estX = state.txPos.x + errX;
    double estY = state.txPos.y + errY;
    state.positionErrorM = std::sqrt(errX*errX + errY*errY);
    
    double dx = estX - state.rxPos.x;
    double dy = estY - state.rxPos.y;
    double dz = state.txPos.z - state.rxPos.z;
    double estRange = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    state.estimatedRangeM = std::max(1.0, estRange);
    state.rangeErrorM = state.estimatedRangeM - state.trueRangeM;
    
    double n = 2.0;
    if (m_env == ENV_URBAN_MODERATE || m_env == ENV_URBAN_DENSE || m_env == ENV_INDUSTRIAL) n = 3.5;
    else if (m_env == ENV_SUBURBAN) n = 2.75;
    else if (m_env == ENV_RURAL_OPEN) n = 2.2;
    
    state.propagationLossBiasDb = 10.0 * n * std::log10(state.estimatedRangeM / std::max(1.0, state.trueRangeM));
}

} // namespace ns3
