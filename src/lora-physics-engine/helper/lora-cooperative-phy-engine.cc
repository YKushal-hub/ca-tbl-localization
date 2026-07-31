#include "lora-cooperative-phy-engine.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaCooperativePhyEngine");

NS_OBJECT_ENSURE_REGISTERED(LoRaCooperativePhyEngine);

TypeId LoRaCooperativePhyEngine::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LoRaCooperativePhyEngine")
        .SetParent<Object>()
        .SetGroupName("LoRaPhysics");
    return tid;
}

LoRaCooperativePhyEngine::LoRaCooperativePhyEngine()
{
    NS_LOG_FUNCTION(this);
}

void LoRaCooperativePhyEngine::Initialize(const CooperativePhyConfigParams& params)
{
    NS_LOG_FUNCTION(this);
    m_params = params;
    m_candidates.clear();
}

void LoRaCooperativePhyEngine::RegisterGatewayCandidate(uint64_t packetUid, const GatewayReceptionCandidate& candidate)
{
    if (!m_params.enabled) return;
    m_candidates[packetUid].push_back(candidate);
}

CooperativePhyEvaluationResult LoRaCooperativePhyEngine::EvaluateCooperativeReception(uint64_t packetUid, uint8_t spreadingFactor)
{
    NS_LOG_FUNCTION(this << packetUid << (int)spreadingFactor);

    CooperativePhyEvaluationResult res;
    res.combinedSnrDb = -100.0;
    res.gatewayDiversityOrder = 0;
    res.cooperativeDecodeSuccess = false;
    res.fusedFineTimestampNs = 0.0;

    if (!m_params.enabled) return res;

    auto it = m_candidates.find(packetUid);
    if (it == m_candidates.end() || it->second.empty()) {
        return res;
    }

    const auto& cands = it->second;
    res.gatewayDiversityOrder = static_cast<uint32_t>(cands.size());

    double maxSnrDb = -100.0;
    double snrLinearSum = 0.0;
    double weightedTimestampSum = 0.0;
    double weightSum = 0.0;
    bool anyDecoded = false;

    for (const auto& c : cands) {
        if (c.snrDb > maxSnrDb) {
            maxSnrDb = c.snrDb;
        }

        double lin = std::pow(10.0, c.snrDb / 10.0);
        snrLinearSum += lin;

        if (lin > 1e-9) {
            weightedTimestampSum += c.timestamp.measuredArrivalTimeNs * lin;
            weightSum += lin;
        }

        if (c.payloadCrcPassed) {
            anyDecoded = true;
        }
    }

    // Maximal Ratio Combining (MRC) or Selection Combining
    if (m_params.maximalRatioCombining && snrLinearSum > 1e-9) {
        res.combinedSnrDb = 10.0 * std::log10(snrLinearSum);
    } else {
        res.combinedSnrDb = maxSnrDb;
    }

    if (weightSum > 1e-9) {
        res.fusedFineTimestampNs = weightedTimestampSum / weightSum;
    }

    // Cooperative decode evaluation
    double reqSnrDb = -7.5 - 2.5 * (spreadingFactor - 7);
    if (anyDecoded || res.combinedSnrDb >= reqSnrDb) {
        res.cooperativeDecodeSuccess = true;
    }

    return res;
}

void LoRaCooperativePhyEngine::ClearPacketCandidates(uint64_t packetUid)
{
    m_candidates.erase(packetUid);
}

} // namespace ns3
