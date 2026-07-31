#include "lora-iq-impairment-submodule.h"

namespace ns3 {

IQImpairmentSubmodule::IQImpairmentSubmodule(const IQImpairmentParams& params, LoRaDistributionSampler& sampler)
    : m_params(params),
      m_sampler(sampler)
{
}

IQResult IQImpairmentSubmodule::ComputeIQImpairments(double rxPowerDbm, double baseSnrDb) {
    IQResult res;
    if (!m_params.enabled) {
        res.evmPercent = 0.0;
        res.imageRejectionDb = 60.0;
        res.adcClippingOccurred = false;
        res.snrDegradationDb = 0.0;
        return res;
    }
    
    double ampImbalanceDb = m_sampler.Sample(m_params.iqAmplitudeImbalanceDb);
    double phaseMismatchDeg = m_sampler.Sample(m_params.iqPhaseMismatchDeg);
    double adcBits = m_sampler.Sample(m_params.adcEffectiveBits);
    double clippingThreshDbm = m_sampler.Sample(m_params.adcClippingThresholdDbm);
    
    double g = std::pow(10.0, ampImbalanceDb / 20.0);
    double phi = phaseMismatchDeg * M_PI / 180.0;
    
    // Image rejection calculation
    double num = 1.0 + g * g + 2.0 * g * std::cos(phi);
    double den = 1.0 + g * g - 2.0 * g * std::cos(phi) + 1e-12;
    res.imageRejectionDb = std::clamp(10.0 * std::log10(num / den), 0.0, 60.0);
    
    // EVM calculation
    double evmIq = std::sqrt(1.0 + g * g - 2.0 * g * std::cos(phi)) * 100.0;
    double snrQuant = 6.02 * adcBits + 1.76;
    double evmNoise = std::pow(10.0, -std::clamp(baseSnrDb, -20.0, 50.0) / 20.0) * 100.0 + 
                      std::pow(10.0, -snrQuant / 20.0) * 100.0;
                      
    double evmTotal = std::sqrt(evmIq * evmIq + evmNoise * evmNoise);
    
    res.adcClippingOccurred = (rxPowerDbm >= clippingThreshDbm);
    if (res.adcClippingOccurred) {
        evmTotal += 35.0; // severe clipping distortion
    }
    
    res.evmPercent = std::clamp(evmTotal, 0.0, 100.0);
    res.snrDegradationDb = 20.0 * std::log10(1.0 + res.evmPercent / 100.0) + (res.adcClippingOccurred ? 15.0 : 0.0);
    
    return res;
}

} // namespace ns3
