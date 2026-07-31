#ifndef LORA_SYMBOL_SYNC_SUBMODULE_H
#define LORA_SYMBOL_SYNC_SUBMODULE_H

#include "ns3/object.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Output metrics from symbol timing recovery dynamics
 */
struct SymbolSyncResult {
    double timingLockQuality;
    double timingJitterNs;
    double preambleAcquisitionScore;
    double residualTimingOffsetNs;
};

/**
 * @brief Submodule for true symbol timing recovery and early-late gate tracking
 */
class SymbolSyncSubmodule {
public:
    SymbolSyncSubmodule(const SymbolSyncConfigParams& params, LoRaDistributionSampler& sampler);
    virtual ~SymbolSyncSubmodule() = default;

    /**
     * @brief Process symbol timing recovery loop
     */
    SymbolSyncResult ProcessTimingRecovery(double initialTimingOffsetNs, double snrDb, uint8_t spreadingFactor, double bandwidthHz);

private:
    SymbolSyncConfigParams m_params;
    LoRaDistributionSampler m_sampler;
};

} // namespace ns3

#endif // LORA_SYMBOL_SYNC_SUBMODULE_H
