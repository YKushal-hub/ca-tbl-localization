#ifndef LORA_GEOGRAPHIC_MEMORY_ENGINE_H
#define LORA_GEOGRAPHIC_MEMORY_ENGINE_H

#include "ns3/object.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "lora-physics-parameters.h"
#include <map>

namespace ns3 {

/**
 * @brief Output metrics from persistent geographic RF memory evaluation
 */
struct GeographicMemoryResult {
    uint32_t rfMemoryRegionId;
    double learnedPathLossOffsetDb;
    double learnedShadowingOffsetDb;
    double learnedReflectionGainDb;
    double learnedDopplerSpreadHz;
    bool memoryHit;
};

/**
 * @brief Geographic cell coordinate key for persistent RF memory
 */
struct GeographicCellKey {
    int32_t cellX;
    int32_t cellY;

    bool operator<(const GeographicCellKey& o) const {
        if (cellX != o.cellX) return cellX < o.cellX;
        return cellY < o.cellY;
    }
};

/**
 * @brief Persistent RF fingerprint statistics for a geographic cell
 */
struct GeographicCellFingerprint {
    uint32_t regionSemanticId;
    double meanPathLossDb;
    double meanShadowingDb;
    double reflectionHotspotGainDb;
    double dopplerSpreadHz;
    uint32_t visitCount;
    Time lastObserved;
};

/**
 * @brief Distributed spatial environment memory engine maintaining long-term persistent RF fingerprints
 */
class LoRaGeographicMemoryEngine : public Object {
public:
    static TypeId GetTypeId();
    LoRaGeographicMemoryEngine();
    virtual ~LoRaGeographicMemoryEngine() override = default;

    void Initialize(const GeographicMemoryConfigParams& params);

    /**
     * @brief Query persistent geographic RF memory for specific spatial coordinates
     */
    GeographicMemoryResult QueryGeographicMemory(Vector pos);

    /**
     * @brief Register a new RF propagation observation into the persistent geographic memory
     */
    void RegisterObservation(Vector pos, uint32_t regionId, double measuredPlDb, double measuredShadDb, double reflectionGainDb, double dopplerHz);

private:
    GeographicCellKey GetCellKey(Vector pos) const;

    GeographicMemoryConfigParams m_params;
    std::map<GeographicCellKey, GeographicCellFingerprint> m_memoryMap;
};

} // namespace ns3

#endif // LORA_GEOGRAPHIC_MEMORY_ENGINE_H
