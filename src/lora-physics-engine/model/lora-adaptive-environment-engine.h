#ifndef LORA_ADAPTIVE_ENVIRONMENT_ENGINE_H
#define LORA_ADAPTIVE_ENVIRONMENT_ENGINE_H

#include "ns3/core-module.h"
#include "ns3/lora-physics-parameters.h"

namespace ns3 {

/**
 * @brief Results from the Adaptive Environment Learning engine
 */
struct AdaptiveEnvironmentResult {
    double environmentLearningConfidence;
    double updatedShadowingVariance;
    double updatedLosProbability;
};

/**
 * @ingroup lora-physics-engine
 * @brief Emulates an online self-learning RF digital twin that dynamically adjusts 
 * underlying propagation statistics (Bayesian updates) based on continuous observation feedback.
 */
class AdaptiveEnvironmentEngine {
public:
    AdaptiveEnvironmentEngine(const AdaptiveEnvironmentConfigParams& params);
    virtual ~AdaptiveEnvironmentEngine() = default;

    /**
     * @brief Ingests the latest packet outcome to continuously refine the environment model.
     */
    AdaptiveEnvironmentResult EvaluateAndAdapt(bool packetReceived, double actualSnr, double expectedSnr);

private:
    AdaptiveEnvironmentConfigParams m_params;
    
    double m_learningConfidence;
    double m_currentShadowingVariance;
    double m_currentLosProbability;
};

} // namespace ns3

#endif // LORA_ADAPTIVE_ENVIRONMENT_ENGINE_H
