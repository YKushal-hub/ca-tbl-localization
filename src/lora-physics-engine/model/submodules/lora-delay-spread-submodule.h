#ifndef LORA_DELAY_SPREAD_SUBMODULE_H
#define LORA_DELAY_SPREAD_SUBMODULE_H

#include "ns3/vector.h"
#include "../../model/lora-physics-parameters.h"
#include "../../model/lora-distribution-sampler.h"
#include <vector>

namespace ns3 {

/**
 * @brief Represents an individual multipath tap in a TDL model
 */
struct MultipathTap {
    double excessDelayNs;
    double relativePowerDb;
    double phaseRad;
};

/**
 * @brief Output container for frequency-selective fading calculations
 */
struct DelaySpreadResult {
    double rmsDelaySpreadNs;
    double coherenceBandwidthHz;
    double frequencySelectivePenaltyDb;
    std::vector<MultipathTap> taps;
};

/**
 * @brief Submodule for generating tapped delay line (TDL) multipath profiles
 * and evaluating frequency-selective notch penalties for LoRa bandwidths.
 * Feature 7: Dynamic delay spread sampling from LOS/NLOS distributions.
 */
class DelaySpreadSubmodule {
public:
    DelaySpreadSubmodule();
    
    DelaySpreadResult ComputeDelaySpread(
        EnvironmentClass env, double distM, double carrierFreqHz, double signalBandwidthHz,
        double correlatedDelaySpreadNs, LoRaDistributionSampler& sampler);
    
    // Feature 7: Dynamic delay spread with environment-aware sampling
    DelaySpreadResult ComputeDelaySpreadDynamic(
        LinkPhysicsState& state, EnvironmentClass env, double distM, double carrierFreqHz, double signalBandwidthHz,
        const DelaySpreadParams& params, LoRaDistributionSampler& sampler);
};

} // namespace ns3

#endif // LORA_DELAY_SPREAD_SUBMODULE_H
