#ifndef LORA_MICRODSP_SUBMODULE_H
#define LORA_MICRODSP_SUBMODULE_H

#include "ns3/core-module.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include <vector>
#include <complex>

namespace ns3 {

/**
 * @brief Results from the Hybrid SDR MicroDSP execution
 */
struct MicroDspResult {
    bool microDspActive;
    double microDspFftConfidence;
    double microDspSymbolAgreement;
    double microDspRuntimeUs;
};

/**
 * @ingroup lora-physics-engine
 * @brief Emulates hybrid micro-sample SDR windows only during critical PHY events.
 * 
 * Replaces full continuous-time heavy simulation with event-triggered micro-windows
 * during collisions, synchronization acquisition, or severe fading.
 */
class MicroDspSubmodule {
public:
    MicroDspSubmodule(const MicroDspConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~MicroDspSubmodule() = default;

    /**
     * @brief Evaluates if MicroDSP should activate, and if so, runs windowed FFT simulation.
     */
    MicroDspResult EvaluateMicroDspWindow(
        double snrDb, double collisionOverlapFraction,
        bool pllLocked, double cfoInstabilityHz,
        bool sicAmbiguity, double symbolicFftConfidence);

private:
    MicroDspConfigParams m_params;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_MICRODSP_SUBMODULE_H
