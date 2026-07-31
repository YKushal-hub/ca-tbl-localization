#ifndef LORA_CROSSLAYER_ENGINE_H
#define LORA_CROSSLAYER_ENGINE_H

#include <map>
#include <vector>
#include <string>
#include <cstdint>

namespace ns3 {

/**
 * @brief Output summary of physical layer link metrics for MAC/routing adaptation
 */
struct PhyLinkMetricSummary {
    double averageSnrDb;
    double collisionRate;
    double delaySpreadNs;
    double fadingVarianceDb;
    double syncFailureRate;
    uint32_t observationCount;
};

/**
 * @brief Implements cross-layer MAC/PHY coupling: PHY-aware ADR, collision-aware MAC backoffs,
 * and relay routing quality scoring based on long-term physical layer observations.
 */
class LoRaCrossLayerEngine {
public:
    LoRaCrossLayerEngine();
    
    void RecordPhyObservation(uint64_t linkId, double snrDb, bool collision, double delaySpreadNs, double fadingDb, bool syncFailed);
    
    PhyLinkMetricSummary GetLinkMetricSummary(uint64_t linkId) const;
    
    uint8_t GetAdaptedSpreadingFactor(uint64_t linkId, uint8_t currentSf) const;
    
    double GetRetransmissionProbability(uint64_t linkId) const;
    
    double GetRoutingPenaltyScore(uint64_t linkId) const;
    
    double GetChannelRandomizationOffsetMs(uint64_t linkId) const;
    
private:
    struct LinkObservationHistory {
        double snrSum;
        double snrSqSum;
        uint32_t collisions;
        double delaySpreadSum;
        double fadingSum;
        double fadingSqSum;
        uint32_t syncFailures;
        uint32_t totalPackets;
        
        LinkObservationHistory()
            : snrSum(0), snrSqSum(0), collisions(0),
              delaySpreadSum(0), fadingSum(0), fadingSqSum(0),
              syncFailures(0), totalPackets(0) {}
    };
    
    std::map<uint64_t, LinkObservationHistory> m_history;
};

} // namespace ns3

#endif // LORA_CROSSLAYER_ENGINE_H
