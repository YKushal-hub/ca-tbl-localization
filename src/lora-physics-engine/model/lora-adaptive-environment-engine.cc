#include "lora-adaptive-environment-engine.h"
#include "ns3/log.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaAdaptiveEnvironmentEngine");

AdaptiveEnvironmentEngine::AdaptiveEnvironmentEngine(const AdaptiveEnvironmentConfigParams& params)
    : m_params(params), m_learningConfidence(params.enabled ? 0.05 : 0.0), m_currentShadowingVariance(8.0), m_currentLosProbability(0.5)
{
    NS_LOG_FUNCTION(this);
}

AdaptiveEnvironmentResult AdaptiveEnvironmentEngine::EvaluateAndAdapt(bool packetReceived, double actualSnr, double expectedSnr)
{
    NS_LOG_FUNCTION(this << packetReceived << actualSnr << expectedSnr);

    AdaptiveEnvironmentResult res;
    res.environmentLearningConfidence = m_learningConfidence;
    res.updatedShadowingVariance = m_currentShadowingVariance;
    res.updatedLosProbability = m_currentLosProbability;

    if (!m_params.enabled) {
        return res;
    }

    // 1. Bayesian Parameter Updates
    double error = actualSnr - expectedSnr;
    
    // Update shadowing variance estimate based on squared error (simplistic Kalman-like gain)
    double observedVariance = error * error;
    m_currentShadowingVariance = (m_params.memoryDecayFactor * m_currentShadowingVariance) + 
                                 ((1.0 - m_params.memoryDecayFactor) * observedVariance);

    // Update LOS probability based on packet success
    // Success slightly increases LOS confidence, failure drops it
    double losTarget = packetReceived ? 1.0 : 0.0;
    m_currentLosProbability = m_currentLosProbability + m_params.learningRate * (losTarget - m_currentLosProbability);

    // 3. Environment Confidence Scores
    // Confidence grows as variance stabilizes and shrinks when large surprising errors occur
    double surprise = std::abs(error) / std::sqrt(std::max(1.0, m_currentShadowingVariance));
    if (surprise > 3.0) {
        m_learningConfidence *= 0.5; // Huge surprise drastically cuts statistical confidence
    } else {
        m_learningConfidence = std::min(1.0, m_learningConfidence + 0.05); // Gradual confidence convergence
    }

    res.environmentLearningConfidence = m_learningConfidence;
    res.updatedShadowingVariance = m_currentShadowingVariance;
    res.updatedLosProbability = std::clamp(m_currentLosProbability, 0.0, 1.0);

    return res;
}

} // namespace ns3
