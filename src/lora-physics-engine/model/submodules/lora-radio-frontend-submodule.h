#ifndef LORA_RADIO_FRONTEND_SUBMODULE_H
#define LORA_RADIO_FRONTEND_SUBMODULE_H

#include "ns3/core-module.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Results from the Advanced Analog RF Frontend execution
 */
struct RadioFrontendResult {
    double oscillatorDriftHz;
    double thermalDriftHz;
    double phaseNoisePsd;
    double spurResponseDb;
    double agcRecoveryTimeMs;
    double frontendDesensitizationDb;
};

/**
 * @ingroup lora-physics-engine
 * @brief Emulates true analog RF hardware internal dynamics including thermal drift,
 * AGC hysteresis, memory effects, mixer spurs, and phase noise spectrum.
 */
class RadioFrontendSubmodule {
public:
    RadioFrontendSubmodule(const RadioFrontendConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~RadioFrontendSubmodule() = default;

    /**
     * @brief Computes continuous analog frontend response and hardware impairments.
     */
    RadioFrontendResult ProcessAnalogFrontend(
        double initialCfoHz, double ambientTemperatureCelsius,
        Time simulationTime, double rxPowerDbm, 
        double strongestInterfererPowerDbm);

private:
    RadioFrontendConfigParams m_params;
    LoRaDistributionSampler& m_sampler;
    
    // Memory state for AGC hysteresis and memory effects
    Time m_lastAgcTriggerTime;
    double m_lastAgcAttenuationDb;
};

} // namespace ns3

#endif // LORA_RADIO_FRONTEND_SUBMODULE_H
