#ifndef LORA_DISTRIBUTED_PHY_ENGINE_H
#define LORA_DISTRIBUTED_PHY_ENGINE_H

#include "ns3/core-module.h"
#include "ns3/lora-physics-parameters.h"
#include <map>
#include <vector>

namespace ns3 {

/**
 * @brief Represents the partial observation state from a single gateway in the distributed network
 */
struct DistributedPhyGatewayState {
    uint32_t gatewayId;
    double symbolLikelihood;
    double timingJitterNs;
    double residualCfoHz;
    bool hasCollision;
    double rawSnrDb;
};

/**
 * @brief Results from the Distributed Cooperative Gateway Fusion execution
 */
struct DistributedPhyResult {
    double distributedDecodeGainDb;
    double sharedPllConfidence;
    double cooperativeSymbolConfidence;
    bool distributedSicTriggered;
};

/**
 * @ingroup lora-physics-engine
 * @brief Emulates a true distributed cooperative PHY network where gateways exchange
 * hypotheses, timing estimates, and partial symbol likelihoods to improve decodes.
 */
class DistributedPhyEngine {
public:
    DistributedPhyEngine(const DistributedPhyConfigParams& params);
    virtual ~DistributedPhyEngine() = default;

    /**
     * @brief Registers a gateway's partial observation for a given physical transmission.
     */
    void RegisterGatewayObservation(uint64_t packetUid, const DistributedPhyGatewayState& state);
    
    /**
     * @brief Computes fused symbol confidence, timing, and distributed SIC capability.
     */
    DistributedPhyResult EvaluateDistributedFusion(uint64_t packetUid);

private:
    DistributedPhyConfigParams m_params;
    std::map<uint64_t, std::vector<DistributedPhyGatewayState>> m_packetStates;
};

} // namespace ns3

#endif // LORA_DISTRIBUTED_PHY_ENGINE_H
