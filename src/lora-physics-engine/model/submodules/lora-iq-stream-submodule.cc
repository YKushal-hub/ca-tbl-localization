#include "lora-iq-stream-submodule.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaIQStreamSubmodule");

IQStreamSubmodule::IQStreamSubmodule(const IQStreamConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler)
{
    NS_LOG_FUNCTION(this);
}

IQStreamResult IQStreamSubmodule::EvolveIQStream(double snrDb, double cfoHz, double dopplerShiftHz, 
                                                 uint8_t spreadingFactor, double bandwidthHz, 
                                                 double packetDurationSec, double collisionOverlapFraction)
{
    NS_LOG_FUNCTION(this << snrDb << cfoHz << dopplerShiftHz << (int)spreadingFactor << bandwidthHz << packetDurationSec << collisionOverlapFraction);

    IQStreamResult res;
    if (!m_params.enabled) {
        res.iqPhaseContinuity = 1.0;
        res.coherentEnergyRatio = 1.0;
        res.instantaneousFrequencyErrorHz = cfoHz;
        return res;
    }

    double symbolDurationSec = std::pow(2.0, spreadingFactor) / bandwidthHz;
    uint32_t totalSymbols = std::max((uint32_t)1, (uint32_t)(packetDurationSec / symbolDurationSec));
    
    // Choose sparse snapshot resolution: 4 snapshots per symbol up to a maximum of 128 total snapshots
    uint32_t numSnapshots = std::min((uint32_t)128, totalSymbols * 4);
    if (numSnapshots < 4) numSnapshots = 4;
    
    double dt = packetDurationSec / numSnapshots;
    double currentPhase = 0.0;
    double signalAmplitude = std::sqrt(std::pow(10.0, snrDb / 10.0));
    double phaseJitterStd = 0.05 / std::max(0.1, signalAmplitude); // higher phase noise at low SNR
    
    double totalEnergy = 0.0;
    double coherentEnergy = 0.0;
    double phaseDiscontinuityCount = 0.0;

    res.sparseSnapshots.reserve(numSnapshots);

    for (uint32_t i = 0; i < numSnapshots; ++i) {
        double t = i * dt;
        double jitter = m_sampler.Sample(DistributionParam::Normal(0.0, phaseJitterStd, -M_PI/2, M_PI/2));
        
        // Continuous CFO accumulation: phi(t+dt) = phi(t) + 2*pi*(f_cfo + f_doppler)*dt
        double dPhi = 2.0 * M_PI * (cfoHz + dopplerShiftHz) * dt + jitter;
        currentPhase += dPhi;
        // Keep phase in [-pi, pi]
        currentPhase = std::fmod(currentPhase + M_PI, 2.0 * M_PI) - M_PI;

        double instFreqErr = (cfoHz + dopplerShiftHz) + jitter / (2.0 * M_PI * dt);
        
        // Complex envelope snapshot
        std::complex<double> noise(
            m_sampler.Sample(DistributionParam::Normal(0.0, 0.707, -3.0, 3.0)),
            m_sampler.Sample(DistributionParam::Normal(0.0, 0.707, -3.0, 3.0))
        );
        std::complex<double> signal = std::polar(signalAmplitude, currentPhase);
        std::complex<double> envelope = signal + noise;

        IQSnapshotSample s;
        s.timestamp = Seconds(t);
        s.complexEnvelope = envelope;
        s.instantaneousPhaseRad = currentPhase;
        s.instantaneousFrequencyErrorHz = instFreqErr;
        res.sparseSnapshots.push_back(s);

        // Coherent symbol energy integration
        double sampleEnergy = std::norm(envelope);
        totalEnergy += sampleEnergy;
        
        // Overlapping collision section degrades coherent phase alignment
        double isOverlapping = ((double)i / numSnapshots) < collisionOverlapFraction ? 1.0 : 0.0;
        if (isOverlapping > 0.5) {
            coherentEnergy += sampleEnergy * 0.2; // Severely degraded coherence during collision
            phaseDiscontinuityCount += 1.0;
        } else {
            coherentEnergy += sampleEnergy * std::cos(jitter);
        }
    }

    res.instantaneousFrequencyErrorHz = cfoHz + dopplerShiftHz;
    res.coherentEnergyRatio = (totalEnergy > 1e-9) ? std::clamp(coherentEnergy / totalEnergy, 0.0, 1.0) : 1.0;
    res.iqPhaseContinuity = std::clamp(1.0 - (phaseDiscontinuityCount / numSnapshots) - phaseJitterStd, 0.0, 1.0);

    return res;
}

} // namespace ns3
