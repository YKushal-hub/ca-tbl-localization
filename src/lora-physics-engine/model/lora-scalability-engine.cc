#include "lora-scalability-engine.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaScalabilityEngine");

NS_OBJECT_ENSURE_REGISTERED(LoRaScalabilityEngine);

TypeId LoRaScalabilityEngine::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LoRaScalabilityEngine")
        .SetParent<Object>()
        .SetGroupName("LoRaPhysics");
    return tid;
}

LoRaScalabilityEngine::LoRaScalabilityEngine()
{
    NS_LOG_FUNCTION(this);
}

void LoRaScalabilityEngine::Initialize(const ScalabilityConfigParams& params)
{
    NS_LOG_FUNCTION(this);
    m_params = params;
    m_lastLinkUpdate.clear();
    m_spatialTileCache.clear();
    m_batchQueue.clear();
}

SpatialTileKey LoRaScalabilityEngine::GetTileKey(Vector pos) const
{
    double res = std::max(5.0, m_params.spatialTileResolutionM);
    int32_t x = static_cast<int32_t>(std::floor(pos.x / res));
    int32_t y = static_cast<int32_t>(std::floor(pos.y / res));
    return {x, y};
}

bool LoRaScalabilityEngine::RequiresChannelUpdate(uint64_t linkId, Time currentTime, double distanceM, double velocityMps)
{
    if (!m_params.enabled) return true;

    auto it = m_lastLinkUpdate.find(linkId);
    if (it == m_lastLinkUpdate.end()) {
        m_lastLinkUpdate[linkId] = currentTime;
        return true;
    }

    Time elapsed = currentTime - it->second;
    double dtSec = elapsed.GetSeconds();

    // Sparse update scheduling rules
    double coherenceTimeSec = (velocityMps > 1.0) ? 0.2 : 2.0;
    double movementM = velocityMps * dtSec;

    if (dtSec >= coherenceTimeSec || movementM >= 1.0) {
        m_lastLinkUpdate[linkId] = currentTime;
        return true;
    }

    return false; // Channel conditions remain coherent, skip redundant update
}

ScalabilityEvaluationResult LoRaScalabilityEngine::EvaluateScalabilityFidelity(uint64_t linkId, Vector txPos, Vector rxPos, 
                                                                               uint32_t activeLinkCount, SimulationFidelityMode currentMode)
{
    NS_LOG_FUNCTION(this << linkId << activeLinkCount << (int)currentMode);

    ScalabilityEvaluationResult res;
    res.activeFidelityLevel = static_cast<uint32_t>(currentMode);
    res.batchProcessingGain = 1.0;
    res.cacheHit = false;
    res.cachedPathLossDb = 0.0;

    if (!m_params.enabled) return res;

    double dx = rxPos.x - txPos.x;
    double dy = rxPos.y - txPos.y;
    double dz = rxPos.z - txPos.z;
    double dist = std::sqrt(dx*dx + dy*dy + dz*dz);

    // WARNING: Dynamic / Adaptive Fidelity Scaling (auto-scaling) changes the active
    // simulation fidelity mode of individual links based on computational load or link status.
    // This dynamically alters the underlying physical channel and baseband models used,
    // which makes the resulting packet telemetry outputs of different links or scenarios
    // non-comparable unless their fidelity levels are identical.
    
    // Hierarchical Channel Resolution
    // High node count or extreme long distance triggers dynamic fidelity reduction
    if (activeLinkCount > m_params.activeLinkThreshold || dist > 4000.0) {
        if (res.activeFidelityLevel == FIDELITY_SDR_GRADE_REALISM) {
            res.activeFidelityLevel = FIDELITY_ADVANCED_TDL_COLLISIONS;
        } else if (res.activeFidelityLevel == FIDELITY_ADVANCED_TDL_COLLISIONS && dist > 6000.0) {
            res.activeFidelityLevel = FIDELITY_CORRELATED_PHY;
        }
    }

    // Check spatial tile cache for static environmental obstructions
    SpatialTileKey tileTx = GetTileKey(txPos);
    auto it = m_spatialTileCache.find(tileTx);
    if (it != m_spatialTileCache.end() && (Simulator::Now() - it->second.lastUpdated).GetSeconds() < 60.0) {
        res.cacheHit = true;
        res.cachedPathLossDb = it->second.obstructionFactor * 5.0 + it->second.foliageAttenuationDb;
    }

    if (!m_batchQueue.empty()) {
        res.batchProcessingGain = 1.0 + 0.2 * std::log2(1.0 + m_batchQueue.size());
    }

    return res;
}

bool LoRaScalabilityEngine::GetCachedTileProperties(Vector pos, SpatialTileCacheEntry& entry)
{
    if (!m_params.enabled) return false;
    SpatialTileKey k = GetTileKey(pos);
    auto it = m_spatialTileCache.find(k);
    if (it != m_spatialTileCache.end()) {
        entry = it->second;
        return true;
    }
    return false;
}

void LoRaScalabilityEngine::UpdateCachedTileProperties(Vector pos, const SpatialTileCacheEntry& entry)
{
    if (!m_params.enabled) return;
    SpatialTileKey k = GetTileKey(pos);
    m_spatialTileCache[k] = entry;
}

void LoRaScalabilityEngine::SubmitBatchJob(const BatchEvaluationJob& job)
{
    if (!m_params.enabled) return;
    m_batchQueue.push_back(job);
}

} // namespace ns3
