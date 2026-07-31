#include "lora-distribution-sampler.h"
#include "ns3/double.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

LoRaDistributionSampler::LoRaDistributionSampler(uint32_t seed) {
    m_uniform = CreateObject<UniformRandomVariable>();
    m_normal = CreateObject<NormalRandomVariable>();
    m_exponential = CreateObject<ExponentialRandomVariable>();
    m_lognormalUnderlying = CreateObject<NormalRandomVariable>();
    m_gamma = CreateObject<GammaRandomVariable>();
    
    // In ns-3, stream allocation is normally done by assigning streams to 
    // variables using AssignStreams or letting them use the default stream manager.
    // The underlying ns-3 RNG will respect --RngSeed and --RngRun automatically.
}

double LoRaDistributionSampler::Sample(const DistributionParam& param) {
    switch (param.type) {
        case DistributionParam::DETERMINISTIC:
            return param.mean;
        case DistributionParam::UNIFORM:
            return SampleUniform(param.min, param.max);
        case DistributionParam::NORMAL:
            return SampleNormal(param.mean, param.stddev, param.min, param.max);
        case DistributionParam::LOGNORMAL:
            return SampleLogNormal(param.mean, param.stddev, param.min, param.max);
        case DistributionParam::EXPONENTIAL:
            return SampleExponential(param.lambda, param.min, param.max);
        case DistributionParam::NAKAGAMI:
            return SampleNakagami(param.shape, param.mean, param.min, param.max);
    }
    return param.mean; // Fallback
}

double LoRaDistributionSampler::SamplePersistent(const DistributionParam& param,
                                                 uint64_t nodeId,
                                                 const std::string& paramName) {
    auto key = std::make_pair(nodeId, paramName);
    auto it = m_persistentCache.find(key);
    if (it != m_persistentCache.end()) {
        return it->second;
    }
    
    double val = Sample(param);
    m_persistentCache[key] = val;
    return val;
}

void LoRaDistributionSampler::ClearNodeCache(uint64_t nodeId) {
    for (auto it = m_persistentCache.begin(); it != m_persistentCache.end();) {
        if (it->first.first == nodeId) {
            it = m_persistentCache.erase(it);
        } else {
            ++it;
        }
    }
}

void LoRaDistributionSampler::ClearAllCaches() {
    m_persistentCache.clear();
}

double LoRaDistributionSampler::SampleUniform(double min, double max) {
    m_uniform->SetAttribute("Min", DoubleValue(min));
    m_uniform->SetAttribute("Max", DoubleValue(max));
    return m_uniform->GetValue();
}

double LoRaDistributionSampler::SampleNormal(double mean, double stddev, double min, double max) {
    m_normal->SetAttribute("Mean", DoubleValue(mean));
    // Variance is stddev^2 in ns-3 NormalRandomVariable
    m_normal->SetAttribute("Variance", DoubleValue(stddev * stddev));
    return Clamp(m_normal->GetValue(), min, max);
}

double LoRaDistributionSampler::SampleLogNormal(double mean, double stddev, double min, double max) {
    m_lognormalUnderlying->SetAttribute("Mean", DoubleValue(mean));
    m_lognormalUnderlying->SetAttribute("Variance", DoubleValue(stddev * stddev));
    double val = m_lognormalUnderlying->GetValue();
    return Clamp(std::exp(val), min, max);
}

double LoRaDistributionSampler::SampleExponential(double lambda, double min, double max) {
    // Mean is 1/lambda in ns-3 ExponentialRandomVariable
    if (lambda <= 0.0) return Clamp(0.0, min, max);
    m_exponential->SetAttribute("Mean", DoubleValue(1.0 / lambda));
    return Clamp(m_exponential->GetValue(), min, max);
}

double LoRaDistributionSampler::SampleNakagami(double shape, double mean, double min, double max) {
    // A Nakagami-m random variable with shape m and spread Omega can be generated
    // by taking the square root of a Gamma variable with shape m and scale Omega/m
    if (shape <= 0.0 || mean <= 0.0) return Clamp(0.0, min, max);
    m_gamma->SetAttribute("Alpha", DoubleValue(shape));
    m_gamma->SetAttribute("Beta", DoubleValue(mean / shape));
    double y = m_gamma->GetValue();
    return Clamp(std::sqrt(y), min, max);
}

double LoRaDistributionSampler::Clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

} // namespace ns3
