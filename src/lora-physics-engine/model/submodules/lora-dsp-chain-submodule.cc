#include "lora-dsp-chain-submodule.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaDspChainSubmodule");

DspChainSubmodule::DspChainSubmodule(const DspChainConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler)
{
    NS_LOG_FUNCTION(this);
}

DspChainResult DspChainSubmodule::ProcessDspPipeline(const IQStreamResult& iqStream, double rxPowerDbm, 
                                                     double internalSnrDb, uint8_t spreadingFactor, 
                                                     double bandwidthHz, bool wasCollision)
{
    NS_LOG_FUNCTION(this << rxPowerDbm << internalSnrDb << (int)spreadingFactor << bandwidthHz << wasCollision);

    DspChainResult res;
    if (!m_params.enabled) {
        res.fftPeakConfidence = 1.0;
        res.symbolLikelihood = 1.0;
        res.crcConfidence = 1.0;
        res.agcSaturationClipping = false;
        res.effectiveSnrPostDspDb = internalSnrDb;
        return res;
    }

    // 1. AGC Saturation Stage
    double agcPenaltyDb = 0.0;
    res.agcSaturationClipping = false;
    if (rxPowerDbm > -20.0) {
        // High input power risks ADC clipping and frontend saturation
        res.agcSaturationClipping = true;
        double overdriveDb = rxPowerDbm - (-20.0);
        agcPenaltyDb = std::min(15.0, overdriveDb * 0.8);
    } else if (rxPowerDbm < m_params.agcThresholdDbm) {
        // Weak signals experience slow AGC convergence
        double deficitDb = m_params.agcThresholdDbm - rxPowerDbm;
        agcPenaltyDb = std::min(5.0, deficitDb * 0.3);
    }

    // 2. Matched Filter & Dechirp Processing Gain
    double dspProcessingGainDb = 10.0 * std::log10(std::pow(2.0, spreadingFactor));
    double effectiveSnrDb = internalSnrDb + dspProcessingGainDb - agcPenaltyDb;

    if (wasCollision) {
        effectiveSnrDb -= 6.0; // Collision noise floor elevation in matched filter
    }

    res.effectiveSnrPostDspDb = effectiveSnrDb;

    // 3. FFT Peak Estimator & Confidence
    // Side-lobe contamination and peak ambiguity based on IQ coherence and residual frequency error
    double freqDriftPenalty = std::min(1.0, std::abs(iqStream.instantaneousFrequencyErrorHz) / (bandwidthHz / std::pow(2.0, spreadingFactor)));
    double dominantPeakRatio = iqStream.iqPhaseContinuity * (1.0 - 0.4 * freqDriftPenalty);
    if (wasCollision) { dominantPeakRatio *= 0.7; }
    
    res.fftPeakConfidence = std::clamp(dominantPeakRatio * iqStream.coherentEnergyRatio, 0.0, 1.0);

    // 4. Soft Symbol Likelihood (Soft Decoder output instead of hard decision)
    double snrLinear = std::pow(10.0, effectiveSnrDb / 10.0);
    double likelihood = snrLinear / (snrLinear + 2.0); // Soft sigmoid mapping
    res.symbolLikelihood = std::clamp(likelihood * res.fftPeakConfidence, 0.0, 1.0);

    // 5. CRC Confidence Estimation
    // Evaluated over aggregate symbol reliabilities
    double crcDecayExponent = 1.5; // Moderate exponent to reflect CRC robustness
    res.crcConfidence = std::clamp(std::pow(res.symbolLikelihood, crcDecayExponent), 0.0, 1.0);

    return res;
}

} // namespace ns3
