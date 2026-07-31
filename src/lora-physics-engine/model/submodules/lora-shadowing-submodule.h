#ifndef LORA_SHADOWING_SUBMODULE_H
#define LORA_SHADOWING_SUBMODULE_H

#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include <map>
#include <utility>

namespace ns3 {

/**
 * @brief Computes correlated large-scale log-normal shadowing loss
 * Implements Gudmundson's spatial correlation model:
 * S_2 = rho * S_1 + sqrt(1 - rho^2) * W
 * where rho = exp(-d / d_corr)
 */
class ShadowingSubmodule {
public:
    ShadowingSubmodule(const ShadowingParams& params, 
                        LoRaDistributionSampler& sampler);
    
    double GetShadowingDb(LinkPhysicsState& state, uint32_t txNodeId, uint32_t rxNodeId,
                           Vector txPos, Vector rxPos, Time currentTime);

private:
    struct ShadowingEntry {
        double valueDb;
        Vector txPosition;
        Vector rxPosition;
        Time lastUpdate;
    };
    
    std::map<std::pair<uint32_t, uint32_t>, ShadowingEntry> m_cache;
    const ShadowingParams& m_params;
    LoRaDistributionSampler& m_sampler;
    
    double ComputeSpatialCorrelation(Vector pos1, Vector pos2,
                                      double correlationDistanceM);
};

} // namespace ns3

#endif // LORA_SHADOWING_SUBMODULE_H
