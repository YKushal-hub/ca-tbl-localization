#ifndef LORA_IQ_IMPAIRMENT_SUBMODULE_H
#define LORA_IQ_IMPAIRMENT_SUBMODULE_H

#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

/**
 * @brief Output telemetry from RF frontend IQ hardware imperfections
 */
struct IQResult {
    double evmPercent;
    double imageRejectionDb;
    bool adcClippingOccurred;
    double snrDegradationDb;
};

/**
 * @brief Emulates I/Q amplitude imbalance, phase skew, DC offset, and ADC saturation
 */
class IQImpairmentSubmodule {
public:
    IQImpairmentSubmodule(const IQImpairmentParams& params, LoRaDistributionSampler& sampler);
    
    IQResult ComputeIQImpairments(double rxPowerDbm, double baseSnrDb);

private:
    IQImpairmentParams m_params;
    LoRaDistributionSampler& m_sampler;
};

} // namespace ns3

#endif // LORA_IQ_IMPAIRMENT_SUBMODULE_H
