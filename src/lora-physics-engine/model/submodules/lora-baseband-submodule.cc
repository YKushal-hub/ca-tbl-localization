#include "lora-baseband-submodule.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

BasebandSubmodule::BasebandSubmodule() {}

void BasebandSubmodule::ResetPhaseState(uint64_t linkId) {
    m_linkChirpPhase[linkId] = 0.0;
}

BasebandResult BasebandSubmodule::ComputeBasebandEvolution(
    uint8_t spreadingFactor, double bandwidthHz,
    double snrDb, double residualCfoHz,
    double residualTimingNs, double dopplerShiftHz,
    double delaySpreadNs, uint32_t payloadBytes,
    uint64_t linkId, Time currentTime)
{
    BasebandResult res;
    res.residualCfoHz = residualCfoHz;
    res.residualTimingErrorNs = residualTimingNs;
    
    double effBw = (bandwidthHz > 0.0 ? bandwidthHz : 125000.0);
    double m = std::pow(2.0, spreadingFactor);
    double tSymS = m / effBw;
    
    // 1. Symbol Phase Continuity
    if (m_linkChirpPhase.count(linkId) == 0) {
        m_linkChirpPhase[linkId] = 0.0;
    }
    double currentPhase = m_linkChirpPhase[linkId];
    
    double totalFreqOffset = residualCfoHz + dopplerShiftHz;
    double phaseDriftPerSym = totalFreqOffset * 2.0 * M_PI * tSymS;
    
    // Approximate symbol count: 8 preamble + 4.25 sync/SF header + payload
    double numSymbols = 12.25 + std::max(1.0, (double)payloadBytes * 8.0 / spreadingFactor);
    
    double finalPhase = currentPhase + phaseDriftPerSym * numSymbols;
    finalPhase = std::fmod(finalPhase, 2.0 * M_PI);
    if (finalPhase < 0.0) finalPhase += 2.0 * M_PI;
    m_linkChirpPhase[linkId] = finalPhase;
    res.finalChirpPhaseRad = finalPhase;
    
    // Chirp frequency drift over packet duration
    res.chirpFrequencyDriftHz = totalFreqOffset * (1.0 + 1e-4 * numSymbols);
    
    // 2. FFT Peak Tracking & Spectral Spreading / Leakage
    double binSpacing = effBw / m;
    double fractionalBinShift = totalFreqOffset / binSpacing;
    double timingFraction = std::abs(residualTimingNs) / (tSymS * 1e9);
    
    // Dirichlet kernel (sinc) leakage from fractional FFT bin shift
    double deltaK = std::abs(fractionalBinShift) - std::floor(std::abs(fractionalBinShift));
    if (deltaK > 0.5) deltaK = 1.0 - deltaK;
    double sincLeakage = (deltaK > 1e-4 ? (1.0 - std::pow(std::sin(M_PI * deltaK) / (M_PI * deltaK), 2.0)) : 0.0);
    
    // Timing offset leakage
    double timingLeakage = std::clamp(timingFraction * 1.5, 0.0, 0.8);
    res.symbolEnergyLeakageRatio = std::clamp(sincLeakage + timingLeakage + delaySpreadNs * 1e-4, 0.0, 0.95);
    
    // Dechirp Coherence Score [0, 1]
    res.dechirpCoherenceScore = std::clamp(1.0 - res.symbolEnergyLeakageRatio - std::abs(dopplerShiftHz)/binSpacing, 0.0, 1.0);
    
    // 3. Symbol Error Probability (SEP) & Packet CRC Probability
    // LoRa non-coherent M-FSK demodulation performance erfc approximation
    double effSnrLinear = std::pow(10.0, snrDb / 10.0) * res.dechirpCoherenceScore;
    double gammaSym = effSnrLinear * spreadingFactor;
    
    double arg = std::sqrt(std::max(0.0, gammaSym / 2.0));
    double erfcVal = std::erfc(std::clamp(arg, 0.0, 10.0));
    double sep = std::clamp(((m - 1.0) / 2.0) * erfcVal, 0.0, 1.0);
    if (std::isnan(sep)) sep = 1e-6;
    res.symbolErrorProbability = sep;
    
    res.packetCrcProbability = std::pow(1.0 - sep, numSymbols);
    if (res.packetCrcProbability < 1e-9) res.packetCrcProbability = 0.0;
    
    // 4. Soft Demodulation Margin
    double snrThreshold = -7.5 - 2.5 * (spreadingFactor - 7);
    res.softSymbolMarginDb = snrDb - snrThreshold;
    
    return res;
}

} // namespace ns3
