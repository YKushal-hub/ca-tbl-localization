#include "lora-pll-submodule.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaPllSubmodule");

PllSubmodule::PllSubmodule(const PllConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler)
{
    NS_LOG_FUNCTION(this);
}

PllResult PllSubmodule::ProcessPllLoop(double initialCfoHz, double fadingLossDb, double preambleDurationSec)
{
    NS_LOG_FUNCTION(this << initialCfoHz << fadingLossDb << preambleDurationSec);

    PllResult res;
    if (!m_params.enabled) {
        res.pllLocked = true;
        res.pllPhaseErrorRad = 0.0;
        res.pllFrequencyErrorHz = 0.0;
        res.cycleSlipOccurred = false;
        return res;
    }

    double zeta = m_params.dampingFactor;
    double bl = m_params.loopBandwidthHz;
    double wn = 2.0 * M_PI * bl / (zeta + 1.0 / (4.0 * zeta));

    // Exponential settling over preamble duration
    double settlingFactor = std::exp(-zeta * wn * preambleDurationSec);
    double residualCfo = initialCfoHz * settlingFactor;

    // Fading loss introduces phase jitter
    double phaseJitter = m_sampler.Sample(DistributionParam::Normal(0.0, 0.05 + 0.02 * fadingLossDb, -M_PI, M_PI));
    res.pllPhaseErrorRad = std::fmod(std::abs(phaseJitter + (residualCfo * preambleDurationSec * 2.0 * M_PI)), M_PI);

    res.pllFrequencyErrorHz = residualCfo + m_sampler.Sample(DistributionParam::Normal(0.0, 0.5 * fadingLossDb, -50.0, 50.0));

    // Cycle slip condition: severe fading notch
    res.cycleSlipOccurred = false;
    if (fadingLossDb > 18.0 && m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0)) < 0.6) {
        res.cycleSlipOccurred = true;
        res.pllPhaseErrorRad = M_PI; // Maximum phase inversion during cycle slip
        res.pllFrequencyErrorHz += 100.0; // Frequency jump
    }

    // Lock verification condition
    double maxAllowableFreqErrHz = 150.0;
    double maxAllowablePhaseErrRad = 1.5;
    if (std::abs(res.pllFrequencyErrorHz) < maxAllowableFreqErrHz && res.pllPhaseErrorRad < maxAllowablePhaseErrRad && !res.cycleSlipOccurred) {
        res.pllLocked = true;
    } else {
        res.pllLocked = false;
    }

    return res;
}

} // namespace ns3
