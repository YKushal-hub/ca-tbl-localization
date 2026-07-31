#include "lora-rf-nonlinearity-submodule.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaRFNonlinearitySubmodule");

RFNonlinearitySubmodule::RFNonlinearitySubmodule(const RFNonlinearityConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler)
{
    NS_LOG_FUNCTION(this);
}

RFNonlinearityResult RFNonlinearitySubmodule::ProcessNonlinearities(double txPowerDbm, double rxPowerDbm, double strongestInterfererDbm)
{
    NS_LOG_FUNCTION(this << txPowerDbm << rxPowerDbm << strongestInterfererDbm);

    RFNonlinearityResult res;
    res.paCompressionDb = 0.0;
    res.lnaSaturationOccurred = false;
    res.phaseNoiseVariance = 0.01;
    res.intermodulationPenaltyDb = 0.0;
    res.effectiveTxPowerDbm = txPowerDbm;
    res.effectiveRxPowerDbm = rxPowerDbm;

    if (!m_params.enabled) return res;

    // 1. Transmitter PA Compression
    double paThresh = m_sampler.Sample(m_params.paCompressionThresholdDbm);
    if (txPowerDbm > paThresh) {
        double excess = txPowerDbm - paThresh;
        // Nonlinear gain compression: 1 dB compression point dynamics
        res.paCompressionDb = std::min(5.0, 0.4 * excess + 0.05 * excess * excess);
        res.effectiveTxPowerDbm -= res.paCompressionDb;
    }

    // 2. LNA Saturation and Desensitization under strong interferers
    double lnaThresh = m_sampler.Sample(m_params.lnaSaturationThresholdDbm);
    if (strongestInterfererDbm > lnaThresh) {
        res.lnaSaturationOccurred = true;
        double desensDb = (strongestInterfererDbm - lnaThresh) * 0.5;
        res.effectiveRxPowerDbm -= std::min(20.0, desensDb);
    }

    // 3. 3rd-Order Intermodulation Distortion (IIP3)
    double iip3 = m_sampler.Sample(m_params.iip3Dbm);
    if (strongestInterfererDbm > (iip3 - 10.0)) {
        double delta = strongestInterfererDbm - (iip3 - 10.0);
        res.intermodulationPenaltyDb = std::min(12.0, 0.3 * std::pow(delta, 1.5));
    }

    // 4. Phase Noise spectrum widening
    res.phaseNoiseVariance = 0.02 + 0.01 * res.paCompressionDb + 0.005 * res.intermodulationPenaltyDb;

    return res;
}

} // namespace ns3
