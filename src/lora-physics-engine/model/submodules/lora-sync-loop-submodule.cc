#include "lora-sync-loop-submodule.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

SyncLoopSubmodule::SyncLoopSubmodule() {}

SyncLoopResult SyncLoopSubmodule::ComputeSynchronizationLoop(
    uint8_t spreadingFactor, double bandwidthHz,
    double snrDb, double initialCfoHz,
    double initialTimingErrorNs, uint32_t preambleLengthSymbols)
{
    SyncLoopResult res;
    
    // Effective loop bandwidth and gain parameters modulated by SNR
    double snrLinear = std::pow(10.0, std::clamp(snrDb, -25.0, 50.0) / 10.0);
    double loopAlpha = 0.3 * (snrLinear / (snrLinear + 0.5)); // FLL/PLL loop gain
    double timingBeta = 0.25 * (snrLinear / (snrLinear + 0.8)); // Timing loop gain
    
    // For very weak preambles (low SNR), loop gain drops significantly, slowing convergence
    if (snrDb < -15.0) {
        loopAlpha *= 0.2;
        timingBeta *= 0.2;
    }
    
    uint32_t nPre = (preambleLengthSymbols > 0 ? preambleLengthSymbols : 8);
    
    // CFO tracking loop evolution over preamble symbols
    double cfo = initialCfoHz;
    double timing = initialTimingErrorNs;
    double agc = 0.0; // AGC convergence starts at 0
    double confidence = 0.0;
    
    for (uint32_t m = 0; m < nPre; ++m) {
        // CFO correction step
        cfo = cfo * (1.0 - loopAlpha) + (snrDb < 0.0 ? (10.0 / (snrLinear + 1e-3)) * std::sin((double)m) : 0.0);
        // Timing correction step
        timing = timing * (1.0 - timingBeta) + (snrDb < 0.0 ? (50.0 / (snrLinear + 1e-3)) * std::cos((double)m) : 0.0);
        // AGC settling step
        double agcStep = 0.4 * (1.0 - (snrDb < -10.0 ? 0.5 : 0.0));
        agc = agc + (1.0 - agc) * agcStep;
        // Sync confidence accumulates
        confidence += 0.15 * (snrDb > -20.0 ? 1.0 : 0.2);
    }
    
    res.residualCfoHz = cfo;
    res.residualTimingErrorNs = timing;
    res.agcConvergenceFactor = std::clamp(agc, 0.0, 1.0);
    res.syncConfidence = std::clamp(confidence, 0.0, 1.0);
    
    return res;
}

} // namespace ns3
