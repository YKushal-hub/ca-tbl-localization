#ifndef LORA_DISTRIBUTION_SAMPLER_H
#define LORA_DISTRIBUTION_SAMPLER_H

#include "ns3/random-variable-stream.h"
#include "ns3/ptr.h"
#include "lora-physics-parameters.h"
#include <map>
#include <string>
#include <utility>

namespace ns3 {

/**
 * @brief Thread-safe (for single-threaded ns-3) random distribution sampler.
 * Integrates with ns-3's RandomVariableStream architecture for repeatable runs.
 */
class LoRaDistributionSampler {
public:
    LoRaDistributionSampler(uint32_t seed = 42);
    
    // Sample once from a distribution, clamped to [min, max]
    double Sample(const DistributionParam& param);
    
    // Sample and cache a persistent value per nodeId
    // Returns the same value every call for the same (nodeId, paramName)
    double SamplePersistent(const DistributionParam& param,
                             uint64_t nodeId,
                             const std::string& paramName);
    
    // Clear the persistent cache for a specific node
    void ClearNodeCache(uint64_t nodeId);
    
    // Clear all persistent caches
    void ClearAllCaches();

private:
    Ptr<UniformRandomVariable> m_uniform;
    Ptr<NormalRandomVariable> m_normal;
    Ptr<ExponentialRandomVariable> m_exponential;
    Ptr<NormalRandomVariable> m_lognormalUnderlying;
    Ptr<GammaRandomVariable> m_gamma; // Used for Nakagami generation
    
    // Persistent cache: key = (nodeId, paramName) -> cached value
    std::map<std::pair<uint64_t, std::string>, double> m_persistentCache;
    
    double SampleNormal(double mean, double stddev, double min, double max);
    double SampleLogNormal(double mean, double stddev, double min, double max);
    double SampleUniform(double min, double max);
    double SampleExponential(double lambda, double min, double max);
    double SampleNakagami(double shape, double mean, double min, double max);
    double Clamp(double value, double min, double max);
};

} // namespace ns3

#endif // LORA_DISTRIBUTION_SAMPLER_H
