#ifndef LORA_CSS_PHY_SUBMODULE_H
#define LORA_CSS_PHY_SUBMODULE_H

#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

/**
 * @brief Output telemetry and degradation results from the CSS PHY dechirp process
 */
struct CSSPHYResult {
    double fftBinOffset;
    double timingSyncErrorNs;
    double demodSuccessProbability;
    bool preambleLocked;
    bool syncCorruptionOccurred;
    bool payloadDemodulated;
    double cfoPenaltyDb;
    double timingPenaltyDb;
    double orthogonalityLeakageDb;
};

/**
 * @brief Emulates SDR-grade chirp spread spectrum (CSS) dechirp and FFT bin sensitivity
 */
class CSSPHYSubmodule {
public:
    CSSPHYSubmodule();
    
    CSSPHYResult ComputeDemodulation(
        double snrDb, double cfoHz, double timingErrorNs,
        double delaySpreadNs, double collisionOverlapFraction,
        uint8_t spreadingFactor, double bandwidthHz,
        bool syncCorrupted, LoRaDistributionSampler& sampler);
        
    static double GetSensitivityThreshold(uint8_t spreadingFactor);
    static double GetSnrThreshold(uint8_t spreadingFactor);
};

} // namespace ns3

#endif // LORA_CSS_PHY_SUBMODULE_H
