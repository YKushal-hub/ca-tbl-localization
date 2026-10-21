#ifndef LORA_INTERFERENCE_SUBMODULE_H
#define LORA_INTERFERENCE_SUBMODULE_H

#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"
#include <vector>
#include <list>
#include <deque>

namespace ns3 {

/**
 * @brief Represents an active physical LoRa transmission in the air
 */
struct ActiveTransmission {
    uint32_t txNodeId;
    Time startTime;
    Time endTime;
    uint8_t spreadingFactor;
    double bandwidthHz;
    uint8_t codingRateDenom;
    double rxPowerDbmAtGateway;
    Vector txPosition;
    double ownLinkDistM = 0.0; //!< T29: the length of the interferer's own link (0: unknown)
    uint32_t rxGatewayId;
    bool isCanceled; // Successive Interference Cancellation (SIC) state
};

/**
 * @brief Manages external co-channel/adjacent-channel interferers, active LoRa packet registry,
 * partial overlap collisions, and dynamic preamble capture logic.
 */
class InterferenceSubmodule {
public:
    InterferenceSubmodule();

    void RegisterActiveTransmission(
        uint32_t txNodeId, uint32_t rxGatewayId,
        Time startTime, Time airtime,
        uint8_t sf, double bw, uint8_t cr,
        double txPowerDbm, Vector txPosition, double ownLinkDistM = 0.0);

    void NotifySuccessfulDecode(uint32_t txNodeId, uint32_t rxGatewayId, double decodedPowerDbm);

    /**
     * TECH_DEBT M.1(b), --relayRoundInSlice. A journal of every registration, so a later evaluation stamped at an
     * EARLIER instant (the relays' round, evaluated after the MAC slice that already purged the frames overlapping
     * it) can put back the transmissions that were in the air then. Off (default): nothing is recorded, and the
     * registry behaves exactly as before. The journal keeps 10 s of registrations.
     */
    void SetJournal(bool on) { m_journalOn = on; }
    //! Re-registers every journalled transmission overlapping [from, to) that is no longer in the registry;
    //! returns how many. Entries still present are not duplicated.
    uint32_t ReplayJournal(Time from, Time to);

    double ComputeInterferencePenaltyDb(
        uint32_t txNodeId, uint8_t rxSpreadingFactor, double bandwidthHz,
        Time desiredStartTime, Time desiredAirtime,
        Vector txPosition, Vector rxPosition, Time currentTime,
        const InterferenceParams& params,
        LoRaDistributionSampler& sampler,
        double desiredSignalPowerDbm,
        double noiseFloorDbm);
    
    bool CheckCaptureEffect(
        double desiredPowerDbm, double interfererPowerDbm,
        uint8_t desiredSF, uint8_t interfererSF,
        const InterferenceParams& params);

    bool WasCollision() const { return m_wasCollision; }
    bool WasCaptured() const { return m_wasCaptured; }
    double GetStrongestInterfererPowerDbm() const { return m_strongestInterfererPowerDbm; }
    uint8_t GetStrongestInterfererSF() const { return m_strongestInterfererSF; }
    double GetOverlapProbability() const { return m_overlapProbability; }
    double GetCollisionOverlapFraction() const { return m_collisionOverlapFraction; }
    uint32_t GetInterfererCount() const { return m_interfererCount; }
    double GetCaptureMarginDb() const { return m_captureMarginDb; }
    bool GetSicRecovered() const { return m_sicRecovered; }

private:
    struct ExternalInterferer {
        Vector position;
        double txPowerDbm;
        double dutyCycle;
        bool currentlyActive;
        Time nextStateChange;
        uint8_t spreadingFactor;
    };
    
    std::vector<ExternalInterferer> m_interferers;
    std::list<ActiveTransmission> m_activeRegistry;
    bool m_journalOn = false;                 //!< M.1(b)
    std::deque<ActiveTransmission> m_journal; //!< M.1(b)
    
    bool m_interfererMapInitialized;
    bool m_wasCollision;
    bool m_wasCaptured;
    double m_strongestInterfererPowerDbm;
    uint8_t m_strongestInterfererSF;
    double m_overlapProbability;
    double m_collisionOverlapFraction;
    uint32_t m_interfererCount;
    double m_captureMarginDb;
    bool m_sicRecovered;
    
    void InitializeInterfererMap(Vector centerPosition, 
                                  double areaRadiusM,
                                  const InterferenceParams& params,
                                  LoRaDistributionSampler& sampler);
};

} // namespace ns3

#endif // LORA_INTERFERENCE_SUBMODULE_H
