#include "lora-distributed-phy-engine.h"
#include "ns3/log.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaDistributedPhyEngine");

DistributedPhyEngine::DistributedPhyEngine(const DistributedPhyConfigParams& params)
    : m_params(params)
{
    NS_LOG_FUNCTION(this);
}

void DistributedPhyEngine::RegisterGatewayObservation(uint64_t packetUid, const DistributedPhyGatewayState& state)
{
    NS_LOG_FUNCTION(this << packetUid << state.gatewayId);
    if (!m_params.enabled) return;
    m_packetStates[packetUid].push_back(state);
}

DistributedPhyResult DistributedPhyEngine::EvaluateDistributedFusion(uint64_t packetUid)
{
    NS_LOG_FUNCTION(this << packetUid);
    DistributedPhyResult res;
    res.distributedDecodeGainDb = 0.0;
    res.sharedPllConfidence = 0.0;
    res.cooperativeSymbolConfidence = 0.0;
    res.distributedSicTriggered = false;

    if (!m_params.enabled || m_packetStates.find(packetUid) == m_packetStates.end()) {
        return res;
    }

    auto& observations = m_packetStates[packetUid];
    if (observations.size() < 2) {
        if (observations.size() == 1) {
            res.sharedPllConfidence = std::max(0.0, 1.0 - std::abs(observations[0].residualCfoHz) / 200.0);
            res.cooperativeSymbolConfidence = observations[0].symbolLikelihood;
        }
        return res;
    }

    // 1. Cooperative Symbol Fusion
    double sumLikelihood = 0.0;
    double maxSnrDb = -200.0;
    double bestSnrLikelihood = 0.0;
    
    // 3. Shared Timing Recovery & 4. Cooperative CFO Estimation
    double sumCfoWeights = 0.0;
    double weightedCfo = 0.0;
    double minJitter = 10000.0;

    for (const auto& obs : observations) {
        sumLikelihood += obs.symbolLikelihood;
        
        if (obs.rawSnrDb > maxSnrDb) {
            maxSnrDb = obs.rawSnrDb;
            bestSnrLikelihood = obs.symbolLikelihood;
        }

        double w = std::pow(10.0, obs.rawSnrDb / 10.0);
        sumCfoWeights += w;
        weightedCfo += obs.residualCfoHz * w;
        minJitter = std::min(minJitter, obs.timingJitterNs);
        
        // 2. Distributed SIC
        // If one gateway has a collision but another doesn't, we can trigger distributed SIC
        if (obs.hasCollision) {
            res.distributedSicTriggered = true;
        }
    }

    if (res.distributedSicTriggered) {
        // If at least one gateway was clean, the collision is bypassed at the network level
        bool cleanExists = false;
        for (const auto& obs : observations) {
            if (!obs.hasCollision && obs.symbolLikelihood > 0.5) {
                cleanExists = true;
                break;
            }
        }
        res.distributedSicTriggered = cleanExists;
    }

    double avgLikelihood = sumLikelihood / observations.size();
    // Fusion gain: combining multiple weak symbols yields a stronger probability
    res.cooperativeSymbolConfidence = std::min(1.0, avgLikelihood * (1.0 + 0.1 * observations.size()));
    
    // Calculate effective decode gain in dB derived from confidence boost
    if (res.cooperativeSymbolConfidence > bestSnrLikelihood && bestSnrLikelihood > 0.01) {
        res.distributedDecodeGainDb = 10.0 * std::log10(res.cooperativeSymbolConfidence / bestSnrLikelihood);
    }

    double fusedCfo = (sumCfoWeights > 0.0) ? (weightedCfo / sumCfoWeights) : 0.0;
    res.sharedPllConfidence = std::max(0.0, 1.0 - std::abs(fusedCfo) / 200.0);

    return res;
}

} // namespace ns3
