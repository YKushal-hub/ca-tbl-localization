#ifndef LORA_SCALABILITY_ENGINE_H
#define LORA_SCALABILITY_ENGINE_H

#include "ns3/object.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "lora-physics-parameters.h"
#include <map>
#include <vector>
#include <memory>

namespace ns3 {

/**
 * @brief Output metrics from city-scale scalability engine
 */
struct ScalabilityEvaluationResult {
    uint32_t activeFidelityLevel;
    double batchProcessingGain;
    bool cacheHit;
    double cachedPathLossDb;
};

/**
 * @brief Spatial tile key for spatial environment caching
 */
struct SpatialTileKey {
    int32_t tileX;
    int32_t tileY;

    bool operator<(const SpatialTileKey& o) const {
        if (tileX != o.tileX) return tileX < o.tileX;
        return tileY < o.tileY;
    }
};

/**
 * @brief Structure representing cached environmental properties within a tile
 */
struct SpatialTileCacheEntry {
    double terrainElevationM;
    double foliageAttenuationDb;
    double obstructionFactor;
    Time lastUpdated;
};

/**
 * @brief Structure representing an evaluation job for batch processing in SoA layout
 */
struct BatchEvaluationJob {
    uint32_t txNodeId;
    uint32_t rxNodeId;
    Vector txPos;
    Vector rxPos;
    double txPowerDbm;
    uint8_t spreadingFactor;
    double distanceM;
};

/**
 * @brief City-scale simulation scalability engine with spatial tile caching and batch evaluation
 */
class LoRaScalabilityEngine : public Object {
public:
    static TypeId GetTypeId();
    LoRaScalabilityEngine();
    virtual ~LoRaScalabilityEngine() override = default;

    void Initialize(const ScalabilityConfigParams& params);

    /**
     * @brief Check if a channel update is required based on sparse scheduling rules
     */
    bool RequiresChannelUpdate(uint64_t linkId, Time currentTime, double distanceM, double velocityMps);

    /**
     * @brief Determine hierarchical simulation fidelity resolution for an active link
     */
    ScalabilityEvaluationResult EvaluateScalabilityFidelity(uint64_t linkId, Vector txPos, Vector rxPos, 
                                                            uint32_t activeLinkCount, SimulationFidelityMode currentMode);

    /**
     * @brief Access spatial tile cache for environmental properties
     */
    bool GetCachedTileProperties(Vector pos, SpatialTileCacheEntry& entry);
    void UpdateCachedTileProperties(Vector pos, const SpatialTileCacheEntry& entry);

    /**
     * @brief Submit a packet evaluation job to the batch processing queue
     */
    void SubmitBatchJob(const BatchEvaluationJob& job);
    size_t GetBatchQueueSize() const { return m_batchQueue.size(); }
    void ClearBatchQueue() { m_batchQueue.clear(); }

private:
    SpatialTileKey GetTileKey(Vector pos) const;

    ScalabilityConfigParams m_params;
    std::map<uint64_t, Time> m_lastLinkUpdate;
    std::map<SpatialTileKey, SpatialTileCacheEntry> m_spatialTileCache;
    std::vector<BatchEvaluationJob> m_batchQueue;
};

} // namespace ns3

#endif // LORA_SCALABILITY_ENGINE_H
