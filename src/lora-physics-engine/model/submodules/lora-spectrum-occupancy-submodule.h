#ifndef LORA_SPECTRUM_OCCUPANCY_SUBMODULE_H
#define LORA_SPECTRUM_OCCUPANCY_SUBMODULE_H

#include "ns3/core-module.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Results from the Spectrum Occupancy evaluation
 */
struct SpectrumOccupancyResult {
    double spectrumOccupancyDbm;
    uint32_t impulsiveNoiseEvents;
    double coloredNoiseVariance;
};

/**
 * @ingroup lora-physics-engine
 * @brief Emulates full spectrum occupancy realism including colored noise floors,
 * external ISM/WiFi harmonics, and impulsive RF burst interference.
 */
class SpectrumOccupancySubmodule {
public:
    SpectrumOccupancySubmodule(const SpectrumOccupancyConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~SpectrumOccupancySubmodule() = default;

    /**
     * @brief Computes dynamic spectrum environment noise and impulsive events.
     */
    SpectrumOccupancyResult EvaluateSpectrumEnvironment(Time simulationTime, double bandwidthHz);

private:
    SpectrumOccupancyConfigParams m_params;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_SPECTRUM_OCCUPANCY_SUBMODULE_H
