#include "lora-crosslayer-engine.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

LoRaCrossLayerEngine::LoRaCrossLayerEngine() {}

void LoRaCrossLayerEngine::RecordPhyObservation(uint64_t linkId, double snrDb, bool collision, double delaySpreadNs, double fadingDb, bool syncFailed) {
    LinkObservationHistory& h = m_history[linkId];
    h.snrSum += snrDb;
    h.snrSqSum += snrDb * snrDb;
    if (collision) h.collisions++;
    h.delaySpreadSum += delaySpreadNs;
    h.fadingSum += fadingDb;
    h.fadingSqSum += fadingDb * fadingDb;
    if (syncFailed) h.syncFailures++;
    h.totalPackets++;
}

PhyLinkMetricSummary LoRaCrossLayerEngine::GetLinkMetricSummary(uint64_t linkId) const {
    PhyLinkMetricSummary s;
    if (m_history.count(linkId) == 0) {
        s.averageSnrDb = 10.0;
        s.collisionRate = 0.0;
        s.delaySpreadNs = 10.0;
        s.fadingVarianceDb = 1.0;
        s.syncFailureRate = 0.0;
        s.observationCount = 0;
        return s;
    }
    
    const LinkObservationHistory& h = m_history.at(linkId);
    if (h.totalPackets == 0) {
        s.averageSnrDb = 10.0;
        s.collisionRate = 0.0;
        s.delaySpreadNs = 10.0;
        s.fadingVarianceDb = 1.0;
        s.syncFailureRate = 0.0;
        s.observationCount = 0;
        return s;
    }
    
    double n = (double)h.totalPackets;
    s.averageSnrDb = h.snrSum / n;
    s.collisionRate = (double)h.collisions / n;
    s.delaySpreadNs = h.delaySpreadSum / n;
    
    double meanFading = h.fadingSum / n;
    s.fadingVarianceDb = std::max(0.1, (h.fadingSqSum / n) - (meanFading * meanFading));
    s.syncFailureRate = (double)h.syncFailures / n;
    s.observationCount = h.totalPackets;
    
    return s;
}

uint8_t LoRaCrossLayerEngine::GetAdaptedSpreadingFactor(uint64_t linkId, uint8_t currentSf) const {
    PhyLinkMetricSummary s = GetLinkMetricSummary(linkId);
    if (s.observationCount < 5) return currentSf;
    
    // High fading variance or high sync failures -> increase SF aggressively for robustness
    if (s.fadingVarianceDb > 15.0 || s.syncFailureRate > 0.25) {
        return std::clamp((int)currentSf + 1, 7, 12);
    }
    // Very clean, stable link with high SNR -> reduce SF gradually for data rate
    if (s.averageSnrDb > 15.0 && s.collisionRate < 0.05 && s.syncFailureRate < 0.02) {
        return std::clamp((int)currentSf - 1, 7, 12);
    }
    return currentSf;
}

double LoRaCrossLayerEngine::GetRetransmissionProbability(uint64_t linkId) const {
    PhyLinkMetricSummary s = GetLinkMetricSummary(linkId);
    if (s.observationCount == 0) return 0.05;
    
    double p = s.collisionRate + s.syncFailureRate + (s.averageSnrDb < -15.0 ? 0.3 : 0.0);
    return std::clamp(p, 0.01, 0.85);
}

double LoRaCrossLayerEngine::GetRoutingPenaltyScore(uint64_t linkId) const {
    PhyLinkMetricSummary s = GetLinkMetricSummary(linkId);
    if (s.observationCount == 0) return 100.0;
    
    // Lower score is better. Penalty incorporates collision rate, sync instability, and delay spread
    double score = 10.0 * (s.collisionRate * 5.0 + s.syncFailureRate * 8.0) + s.delaySpreadNs * 0.05 + s.fadingVarianceDb * 0.5 - s.averageSnrDb;
    return std::max(1.0, score);
}

double LoRaCrossLayerEngine::GetChannelRandomizationOffsetMs(uint64_t linkId) const {
    PhyLinkMetricSummary s = GetLinkMetricSummary(linkId);
    // If collision rate is persistent (>10%), introduce a random backoff timing offset
    if (s.collisionRate > 0.10) {
        return 10.0 + 100.0 * s.collisionRate;
    }
    return 0.0;
}

} // namespace ns3
