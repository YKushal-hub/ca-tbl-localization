#include "lora-mobility-submodule.h"
#include <cmath>

namespace ns3 {

double ComputeDopplerPenaltyDb(
    double velocityMps, uint8_t spreadingFactor, double bandwidthHz,
    double frequencyHz, const MobilityParams& params,
    LoRaDistributionSampler& sampler)
{
    if (!params.enabled || !params.dopplerDegradationEnabled) {
        return 0.0;
    }

    if (velocityMps <= 0.0) {
        return 0.0;
    }

    // 1. Compute Doppler shift: f_D = v * f / c
    double c = 299792458.0; // speed of light in m/s
    double f_D = velocityMps * frequencyHz / c;

    // 2. Compute maximum tolerable Doppler shift for LoRa SF:
    // f_D_max = BW / (2 * 2^SF) (standard LoRa coherence limitation)
    double numSymbols = std::pow(2.0, spreadingFactor);
    double f_D_max = bandwidthHz / (2.0 * numSymbols);

    // 3. If f_D <= f_D_max, no penalty is applied
    if (f_D <= f_D_max) {
        return 0.0;
    }

    // 4. If f_D > f_D_max, apply an SNR penalty proportional to the excess Doppler
    double excessDoppler = f_D - f_D_max;
    double penaltyPerHz = sampler.Sample(params.dopplerSNRPenaltyDbPerHz);
    double penalty = penaltyPerHz * excessDoppler;

    if (penalty < 0.0) {
        penalty = 0.0; // Constraint 6
    }
    return penalty;
}

} // namespace ns3
