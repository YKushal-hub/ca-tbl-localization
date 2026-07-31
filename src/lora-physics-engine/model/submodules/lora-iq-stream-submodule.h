#ifndef LORA_IQ_STREAM_SUBMODULE_H
#define LORA_IQ_STREAM_SUBMODULE_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include <vector>
#include <complex>

namespace ns3 {

/**
 * @brief Represents a sparse snapshot sample of continuous IQ stream
 */
struct IQSnapshotSample {
    Time timestamp;
    std::complex<double> complexEnvelope;
    double instantaneousPhaseRad;
    double instantaneousFrequencyErrorHz;
};

/**
 * @brief Output metrics from sparse continuous-time IQ stream evolution
 */
struct IQStreamResult {
    double iqPhaseContinuity;
    double coherentEnergyRatio;
    double instantaneousFrequencyErrorHz;
    std::vector<IQSnapshotSample> sparseSnapshots;
};

/**
 * @brief Submodule for lightweight continuous-time IQ state evolution
 */
class IQStreamSubmodule {
public:
    IQStreamSubmodule(const IQStreamConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~IQStreamSubmodule() = default;

    /**
     * @brief Evolve sparse continuous-time IQ stream across preamble, sync, and payload chirps
     */
    IQStreamResult EvolveIQStream(double snrDb, double cfoHz, double dopplerShiftHz, 
                                  uint8_t spreadingFactor, double bandwidthHz, 
                                  double packetDurationSec, double collisionOverlapFraction);

private:
    IQStreamConfigParams m_params;
    LoRaDistributionSampler m_sampler;
};

} // namespace ns3

#endif // LORA_IQ_STREAM_SUBMODULE_H
