#ifndef LORA_PLL_SUBMODULE_H
#define LORA_PLL_SUBMODULE_H

#include "ns3/object.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Output metrics from second-order digital PLL model
 */
struct PllResult {
    bool pllLocked;
    double pllPhaseErrorRad;
    double pllFrequencyErrorHz;
    bool cycleSlipOccurred;
};

/**
 * @brief Submodule modeling a second-order digital phase-locked loop (PLL)
 */
class PllSubmodule {
public:
    PllSubmodule(const PllConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~PllSubmodule() = default;

    /**
     * @brief Process carrier frequency recovery and phase tracking loop
     */
    PllResult ProcessPllLoop(double initialCfoHz, double fadingLossDb, double preambleDurationSec);

private:
    PllConfigParams m_params;
    LoRaDistributionSampler m_sampler;
};

} // namespace ns3

#endif // LORA_PLL_SUBMODULE_H
