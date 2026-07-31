#ifndef LORA_CORRELATION_ENGINE_H
#define LORA_CORRELATION_ENGINE_H

#include "ns3/nstime.h"
#include "ns3/vector.h"
#include "lora-physics-parameters.h"
#include "lora-distribution-sampler.h"
#include <map>

namespace ns3 {

/**
 * @brief Physically coupled impairment states across a LoRa transmission link.
 * Removes artificial hard loss caps by generating correlated latent variables.
 */
struct CorrelatedChannelState {
    bool los;
    double obstructionFactor;      // [0.0, 1.0] where 1.0 is extreme obstruction
    double foliageDensity;         // [0.0, 1.0]
    double delaySpreadNs;
    double angularSpreadDeg;
    double shadowingBiasDb;        // Latent shadowing shift in dB
    double fadingSeverity;         // [0.0, 1.0] where 1.0 is severe Rayleigh/Nakagami
    double coherenceTimeScale;     // Multiplier for temporal coherence
    double terrainBlockageFactor;  // [0.0, 1.0]
    double rainWetnessFactor;      // [0.0, 1.0]
    
    Time lastUpdateTime;
    
    // Gauss-Markov state tracking for smooth temporal evolution
    double gmShadowingState;
    double gmFadingState;
    double gmDelaySpreadState;
    double gmDopplerState;
};

/**
 * @brief Manages link-level channel state correlations and Gauss-Markov temporal evolution.
 */
class LoRaCorrelationEngine {
public:
    LoRaCorrelationEngine();
    
    CorrelatedChannelState GetCorrelatedState(
        uint64_t linkId, Vector txPos, Vector rxPos,
        EnvironmentClass env, WeatherState weather,
        Time currentTime, LoRaDistributionSampler& sampler);
        
private:
    std::map<uint64_t, CorrelatedChannelState> m_channelStates;
};

} // namespace ns3

#endif // LORA_CORRELATION_ENGINE_H
