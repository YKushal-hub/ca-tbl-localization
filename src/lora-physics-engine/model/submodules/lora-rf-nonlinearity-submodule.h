#ifndef LORA_RF_NONLINEARITY_SUBMODULE_H
#define LORA_RF_NONLINEARITY_SUBMODULE_H

#include "ns3/object.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Output metrics from nonlinear RF frontend distortion
 */
struct RFNonlinearityResult {
    double paCompressionDb;
    bool lnaSaturationOccurred;
    double phaseNoiseVariance;
    double intermodulationPenaltyDb;
    double effectiveTxPowerDbm;
    double effectiveRxPowerDbm;
};

/**
 * @brief Submodule modeling PA compression, LNA saturation, and intermodulation distortion
 */
class RFNonlinearitySubmodule {
public:
    RFNonlinearitySubmodule(const RFNonlinearityConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~RFNonlinearitySubmodule() = default;

    /**
     * @brief Process nonlinear RF frontend impairments at transmitter and receiver
     */
    RFNonlinearityResult ProcessNonlinearities(double txPowerDbm, double rxPowerDbm, double strongestInterfererDbm);

private:
    RFNonlinearityConfigParams m_params;
    LoRaDistributionSampler m_sampler;
};

} // namespace ns3

#endif // LORA_RF_NONLINEARITY_SUBMODULE_H
