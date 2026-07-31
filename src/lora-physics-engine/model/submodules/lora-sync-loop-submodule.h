#ifndef LORA_SYNC_LOOP_SUBMODULE_H
#define LORA_SYNC_LOOP_SUBMODULE_H

#include <vector>
#include <map>
#include <cstdint>

namespace ns3 {

/**
 * @brief Output result from receiver synchronization loops
 */
struct SyncLoopResult {
    double residualCfoHz;
    double residualTimingErrorNs;
    double agcConvergenceFactor; // [0, 1]
    double syncConfidence; // [0, 1]
};

/**
 * @brief Emulates LoRa receiver synchronization loops: CFO tracking, timing recovery, AGC settling, and preamble detection.
 */
class SyncLoopSubmodule {
public:
    SyncLoopSubmodule();
    
    SyncLoopResult ComputeSynchronizationLoop(
        uint8_t spreadingFactor, double bandwidthHz,
        double snrDb, double initialCfoHz,
        double initialTimingErrorNs, uint32_t preambleLengthSymbols);
};

} // namespace ns3

#endif // LORA_SYNC_LOOP_SUBMODULE_H
