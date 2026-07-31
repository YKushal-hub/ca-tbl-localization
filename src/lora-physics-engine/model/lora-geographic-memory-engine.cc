#include "lora-geographic-memory-engine.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaGeographicMemoryEngine");

NS_OBJECT_ENSURE_REGISTERED(LoRaGeographicMemoryEngine);

TypeId LoRaGeographicMemoryEngine::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LoRaGeographicMemoryEngine")
        .SetParent<Object>()
        .SetGroupName("LoRaPhysics");
    return tid;
}

LoRaGeographicMemoryEngine::LoRaGeographicMemoryEngine()
{
    NS_LOG_FUNCTION(this);
}

void LoRaGeographicMemoryEngine::Initialize(const GeographicMemoryConfigParams& params)
{
    NS_LOG_FUNCTION(this);
    m_params = params;
    m_memoryMap.clear();
}

GeographicCellKey LoRaGeographicMemoryEngine::GetCellKey(Vector pos) const
{
    double s = std::max(5.0, m_params.memoryCellSizeM);
    int32_t x = static_cast<int32_t>(std::floor(pos.x / s));
    int32_t y = static_cast<int32_t>(std::floor(pos.y / s));
    return {x, y};
}

GeographicMemoryResult LoRaGeographicMemoryEngine::QueryGeographicMemory(Vector pos)
{
    GeographicMemoryResult res;
    res.rfMemoryRegionId = 0;
    res.learnedPathLossOffsetDb = 0.0;
    res.learnedShadowingOffsetDb = 0.0;
    res.learnedReflectionGainDb = 0.0;
    res.learnedDopplerSpreadHz = 0.0;
    res.memoryHit = false;

    if (!m_params.enabled) return res;

    GeographicCellKey k = GetCellKey(pos);
    auto it = m_memoryMap.find(k);
    if (it != m_memoryMap.end()) {
        res.memoryHit = true;
        res.rfMemoryRegionId = it->second.regionSemanticId;
        res.learnedPathLossOffsetDb = it->second.meanPathLossDb;
        res.learnedShadowingOffsetDb = it->second.meanShadowingDb;
        res.learnedReflectionGainDb = it->second.reflectionHotspotGainDb;
        res.learnedDopplerSpreadHz = it->second.dopplerSpreadHz;
    }

    return res;
}

void LoRaGeographicMemoryEngine::RegisterObservation(Vector pos, uint32_t regionId, double measuredPlDb, double measuredShadDb, double reflectionGainDb, double dopplerHz)
{
    if (!m_params.enabled) return;

    GeographicCellKey k = GetCellKey(pos);
    auto it = m_memoryMap.find(k);
    if (it != m_memoryMap.end()) {
        double alpha = 0.2; // Exponential moving average learning rate
        it->second.meanPathLossDb = (1.0 - alpha) * it->second.meanPathLossDb + alpha * measuredPlDb;
        it->second.meanShadowingDb = (1.0 - alpha) * it->second.meanShadowingDb + alpha * measuredShadDb;
        it->second.reflectionHotspotGainDb = std::max(it->second.reflectionHotspotGainDb, reflectionGainDb);
        it->second.dopplerSpreadHz = (1.0 - alpha) * it->second.dopplerSpreadHz + alpha * dopplerHz;
        it->second.visitCount++;
        it->second.lastObserved = Simulator::Now();
    } else {
        GeographicCellFingerprint fp;
        fp.regionSemanticId = regionId;
        fp.meanPathLossDb = measuredPlDb;
        fp.meanShadowingDb = measuredShadDb;
        fp.reflectionHotspotGainDb = reflectionGainDb;
        fp.dopplerSpreadHz = dopplerHz;
        fp.visitCount = 1;
        fp.lastObserved = Simulator::Now();
        m_memoryMap[k] = fp;
    }
}

} // namespace ns3
