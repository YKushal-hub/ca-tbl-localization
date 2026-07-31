#ifndef LORA_BASEBAND_SUBMODULE_H
#define LORA_BASEBAND_SUBMODULE_H

#include <map>
#include <vector>
#include "ns3/nstime.h"

namespace ns3 {

/**
 * @brief Output result from symbol-domain baseband chirp evolution
 */
struct BasebandResult {
    double finalChirpPhaseRad;
    double chirpFrequencyDriftHz;
    double dechirpCoherenceScore; // [0, 1]
    double symbolEnergyLeakageRatio;
    double symbolErrorProbability;
    double packetCrcProbability;
    double softSymbolMarginDb;
    double residualCfoHz;
    double residualTimingErrorNs;
};

/**
 * @brief Implements symbol-domain baseband chirp evolution, tracking phase continuity,
 * FFT peak migration, Dirichlet kernel spectral leakage, and soft demodulation margins.
 */
class BasebandSubmodule {
public:
    BasebandSubmodule();
    
    BasebandResult ComputeBasebandEvolution(
        uint8_t spreadingFactor, double bandwidthHz,
        double snrDb, double residualCfoHz,
        double residualTimingNs, double dopplerShiftHz,
        double delaySpreadNs, uint32_t payloadBytes,
        uint64_t linkId, Time currentTime);
        
    void ResetPhaseState(uint64_t linkId);
    
private:
    std::map<uint64_t, double> m_linkChirpPhase;
};

} // namespace ns3

#endif // LORA_BASEBAND_SUBMODULE_H
