#include "lora-temporal-blockage-submodule.h"

namespace ns3 {

TemporalBlockageSubmodule::TemporalBlockageSubmodule(const TemporalBlockageParams& params, EnvironmentClass env, LoRaDistributionSampler& sampler)
    : m_params(params), m_env(env), m_sampler(sampler) {
}

void TemporalBlockageSubmodule::Compute(LinkPhysicsState& state) {
    if (!m_params.enabled) {
        state.temporalBlockageDb = 0.0;
        return;
    }

    if (m_env == ENV_INDUSTRIAL) {
        bool currentlyBlocked = false;
        if (m_blockageState.count(state.txNodeId) > 0) {
            currentlyBlocked = m_blockageState[state.txNodeId];
        }
        // Markov chain yielding ~0.0625 stationary probability
        // P(Clear -> Block) = 0.01, P(Block -> Block) = 0.85
        double pToBlock = currentlyBlocked ? 0.85 : 0.01;
        double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
        currentlyBlocked = (u < pToBlock);
        m_blockageState[state.txNodeId] = currentlyBlocked;
        
        if (currentlyBlocked) {
            state.temporalBlockageDb = m_sampler.Sample(m_params.blockageLossDb);
        } else {
            state.temporalBlockageDb = 0.0;
        }
        return;
    }

    // Blockage probabilities vary by environment
    double pBlockage = 0.0;
    if (m_env == ENV_URBAN_DENSE) {
        pBlockage = 0.2; // High probability of moving vehicles/machinery
    } else if (m_env == ENV_URBAN_MODERATE || m_env == ENV_INDOOR_OFFICE) {
        pBlockage = 0.1;
    } else {
        pBlockage = 0.02; // Rural/Open
    }

    double u = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
    if (u < pBlockage) {
        state.temporalBlockageDb = m_sampler.Sample(m_params.blockageLossDb);
    } else {
        state.temporalBlockageDb = 0.0;
    }
}

} // namespace ns3
