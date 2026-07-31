#include "lora-shadowing-submodule.h"
#include <cmath>

namespace ns3 {

ShadowingSubmodule::ShadowingSubmodule(const ShadowingParams& params, 
                                       LoRaDistributionSampler& sampler)
    : m_params(params),
      m_sampler(sampler)
{
}

double ShadowingSubmodule::GetShadowingDb(LinkPhysicsState& state, uint32_t txNodeId, uint32_t rxNodeId,
                                          Vector txPos, Vector rxPos, Time currentTime)
{
    if (!m_params.enabled) {
        state.shadowingDb = 0.0;
        return 0.0;
    }

    auto key = std::make_pair(txNodeId, rxNodeId);
    auto it = m_cache.find(key);
    
    bool needNewSample = false;
    if (it == m_cache.end()) {
        needNewSample = true;
    } else {
        if (m_params.updateIntervalS > 0.0) {
            double elapsed = (currentTime - it->second.lastUpdate).GetSeconds();
            if (elapsed >= m_params.updateIntervalS) {
                needNewSample = true;
            }
        }
        
        // Feature: Spatial correlation requires updating shadowing as the node moves
        if (!needNewSample && m_params.spatialCorrelationEnabled) {
            double txDistMoved = ComputeSpatialCorrelation(txPos, it->second.txPosition, 0.0);
            double rxDistMoved = ComputeSpatialCorrelation(rxPos, it->second.rxPosition, 0.0);
            if (txDistMoved > 0.1 || rxDistMoved > 0.1) { // Node moved more than 10cm
                needNewSample = true;
            }
        }
    }

    // Determine current effective standard deviation based on NLOS state (Coupling 2)
    double stddev = m_sampler.Sample(m_params.shadowingStddevDb);
    if (state.isNLOS) {
        stddev *= m_params.nlosToLosSigmaRatio;
    }
    if (state.pendingTerrainDb > 3.0) {
        stddev *= 1.25; // Coupling 5: Terrain variance inflation
    }
    state.shadowingStdEff = stddev;

    if (needNewSample) {
        // Zero-mean log-normal shadowing in dB: sample standard normal and scale
        double W = m_sampler.Sample(DistributionParam::Normal(0.0, stddev, -4.0 * stddev, 4.0 * stddev));
        double finalShadowing = W;
        
        if (m_params.spatialCorrelationEnabled && it != m_cache.end()) {
            double corrDist = m_sampler.Sample(m_params.correlationDistanceM);
            double txDist = ComputeSpatialCorrelation(txPos, it->second.txPosition, corrDist);
            double rxDist = ComputeSpatialCorrelation(rxPos, it->second.rxPosition, corrDist);
            double dist = std::max(txDist, rxDist);
            double rho = std::exp(-dist / corrDist);
            double prev_val = it->second.valueDb;
            
            // Coupling 5: Terrain gates correlation
            if (state.pendingTerrainDb > 3.0) {
                rho = 0.0;
            }
            
            // Gudmundson's model: S_2 = rho * S_1 + sqrt(1 - rho^2) * W
            if (rho > 0.0 && rho < 1.0) {
                finalShadowing = rho * prev_val + std::sqrt(1.0 - rho * rho) * W;
            } else if (rho >= 1.0) {
                finalShadowing = prev_val;
            }
        }
        
        ShadowingEntry entry;
        entry.valueDb = finalShadowing;
        entry.txPosition = txPos;
        entry.rxPosition = rxPos;
        entry.lastUpdate = currentTime;
        
        m_cache[key] = entry;
        state.shadowingDb = finalShadowing;
        return finalShadowing;
    }

    state.shadowingDb = it->second.valueDb;
    return it->second.valueDb;
}

double ShadowingSubmodule::ComputeSpatialCorrelation(Vector pos1, Vector pos2,
                                                     double correlationDistanceM)
{
    double dx = pos1.x - pos2.x;
    double dy = pos1.y - pos2.y;
    double dz = pos1.z - pos2.z;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

} // namespace ns3
