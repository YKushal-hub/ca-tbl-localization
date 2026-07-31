#ifndef LORA_COOPERATIVE_PHY_ENGINE_H
#define LORA_COOPERATIVE_PHY_ENGINE_H

#include "ns3/object.h"
#include "ns3/nstime.h"
#include "../model/lora-physics-parameters.h"
#include "../model/submodules/lora-hardware-submodule.h"
#include <vector>
#include <memory>

namespace ns3 {

/**
 * @brief Output metrics from cooperative multi-gateway reception
 */
struct CooperativePhyEvaluationResult {
    double combinedSnrDb;
    uint32_t gatewayDiversityOrder;
    bool cooperativeDecodeSuccess;
    double fusedFineTimestampNs;
};

/**
 * @brief Represents a single gateway's partial reception observation for combining
 */
struct GatewayReceptionCandidate {
    uint32_t gatewayId;
    double snrDb;
    double rxPowerDbm;
    bool preambleLocked;
    bool payloadCrcPassed;
    TimestampObservation timestamp;
};

/**
 * @brief Engine evaluating multi-gateway cooperative combining, MRC, and TDoA fusion
 */
class LoRaCooperativePhyEngine : public Object {
public:
    static TypeId GetTypeId();
    LoRaCooperativePhyEngine();
    virtual ~LoRaCooperativePhyEngine() override = default;

    void Initialize(const CooperativePhyConfigParams& params);

    /**
     * @brief Register a gateway's partial candidate reception for a specific packet
     */
    void RegisterGatewayCandidate(uint64_t packetUid, const GatewayReceptionCandidate& candidate);

    /**
     * @brief Execute multi-gateway combining across all registered candidates for a packet
     */
    CooperativePhyEvaluationResult EvaluateCooperativeReception(uint64_t packetUid, uint8_t spreadingFactor);

    void ClearPacketCandidates(uint64_t packetUid);

private:
    CooperativePhyConfigParams m_params;
    std::map<uint64_t, std::vector<GatewayReceptionCandidate>> m_candidates;
};

} // namespace ns3

#endif // LORA_COOPERATIVE_PHY_ENGINE_H
