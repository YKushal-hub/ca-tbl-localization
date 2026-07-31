#ifndef LORA_DSP_CHAIN_SUBMODULE_H
#define LORA_DSP_CHAIN_SUBMODULE_H

#include "ns3/object.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include "lora-iq-stream-submodule.h"

namespace ns3 {

/**
 * @brief Output metrics from lightweight SDR DSP receiver pipeline
 */
struct DspChainResult {
    double fftPeakConfidence;
    double symbolLikelihood;
    double crcConfidence;
    bool agcSaturationClipping;
    double effectiveSnrPostDspDb;
};

/**
 * @brief Submodule executing a lightweight continuous-time SDR DSP pipeline
 */
class DspChainSubmodule {
public:
    DspChainSubmodule(const DspChainConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~DspChainSubmodule() = default;

    /**
     * @brief Process incoming sparse IQ stream through the SDR receiver DSP stages
     */
    DspChainResult ProcessDspPipeline(const IQStreamResult& iqStream, double rxPowerDbm, 
                                      double internalSnrDb, uint8_t spreadingFactor, 
                                      double bandwidthHz, bool wasCollision);

private:
    DspChainConfigParams m_params;
    LoRaDistributionSampler m_sampler;
};

} // namespace ns3

#endif // LORA_DSP_CHAIN_SUBMODULE_H
