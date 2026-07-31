#include "lora-css-phy-submodule.h"

namespace ns3 {

CSSPHYSubmodule::CSSPHYSubmodule() {}

double CSSPHYSubmodule::GetSensitivityThreshold(uint8_t spreadingFactor) {
    switch (spreadingFactor) {
        case 7:  return -123.0;
        case 8:  return -126.0;
        case 9:  return -129.0;
        case 10: return -132.0;
        case 11: return -134.5;
        case 12: return -137.0;
        default: return -123.0;
    }
}

double CSSPHYSubmodule::GetSnrThreshold(uint8_t spreadingFactor) {
    switch (spreadingFactor) {
        case 7:  return -7.5;
        case 8:  return -10.0;
        case 9:  return -12.5;
        case 10: return -15.0;
        case 11: return -17.5;
        case 12: return -20.0;
        default: return -7.5;
    }
}

CSSPHYResult CSSPHYSubmodule::ComputeDemodulation(
    double snrDb, double cfoHz, double timingErrorNs,
    double delaySpreadNs, double collisionOverlapFraction,
    uint8_t spreadingFactor, double bandwidthHz,
    bool syncCorrupted, LoRaDistributionSampler& sampler)
{
    CSSPHYResult res;
    res.syncCorruptionOccurred = syncCorrupted;
    res.timingSyncErrorNs = timingErrorNs;
    
    double effBw = (bandwidthHz > 0.0 ? bandwidthHz : 125000.0);
    double m = std::pow(2.0, spreadingFactor);
    double binSpacing = effBw / m;
    
    res.fftBinOffset = cfoHz / binSpacing;
    
    // 1. CFO-induced FFT bin drift penalty
    if (std::abs(res.fftBinOffset) > 0.25) {
        res.cfoPenaltyDb = 10.0 * std::log10(1.0 + 8.0 * (std::abs(res.fftBinOffset) - 0.25));
        res.cfoPenaltyDb = std::clamp(res.cfoPenaltyDb, 0.0, 15.0);
    } else {
        res.cfoPenaltyDb = 0.0;
    }
    
    // 2. Timing offset sensitivity
    double symbolDurationNs = 1e9 * m / effBw;
    double timingFraction = std::abs(timingErrorNs) / symbolDurationNs;
    res.timingPenaltyDb = std::clamp(15.0 * timingFraction, 0.0, 10.0);
    
    // 3. Chirp orthogonality leakage (CFO-induced SF leakage, asynchronous overlap, delay spread inter-symbol interference)
    res.orthogonalityLeakageDb = std::clamp(4.0 * collisionOverlapFraction + 2.0 * std::abs(res.fftBinOffset) + 0.005 * delaySpreadNs, 0.0, 12.0);
    
    // 4. Dechirp failure probability
    double effectiveSnrDb = snrDb - res.cfoPenaltyDb - res.timingPenaltyDb - res.orthogonalityLeakageDb;
    double snrReq = GetSnrThreshold(spreadingFactor);
    
    res.preambleLocked = (effectiveSnrDb >= (snrReq - 3.0)) && (!syncCorrupted);
    
    if (res.preambleLocked) {
        double z = (effectiveSnrDb - snrReq + 1.5) / (2.0 * std::sqrt(2.0));
        res.demodSuccessProbability = std::clamp(0.5 * (1.0 + std::erf(z)), 0.0, 1.0);
    } else {
        res.demodSuccessProbability = 0.0;
    }
    
    res.payloadDemodulated = (sampler.Sample(DistributionParam::Uniform(0.0, 1.0)) <= res.demodSuccessProbability);
    
    return res;
}

} // namespace ns3
