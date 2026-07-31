#include "lora-symbol-sync-submodule.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaSymbolSyncSubmodule");

SymbolSyncSubmodule::SymbolSyncSubmodule(const SymbolSyncConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler)
{
    NS_LOG_FUNCTION(this);
}

SymbolSyncResult SymbolSyncSubmodule::ProcessTimingRecovery(double initialTimingOffsetNs, double snrDb, uint8_t spreadingFactor, double bandwidthHz)
{
    NS_LOG_FUNCTION(this << initialTimingOffsetNs << snrDb << (int)spreadingFactor << bandwidthHz);

    SymbolSyncResult res;
    if (!m_params.enabled) {
        res.timingLockQuality = 1.0;
        res.timingJitterNs = 0.0;
        res.preambleAcquisitionScore = 1.0;
        res.residualTimingOffsetNs = 0.0;
        return res;
    }

    double symbolDurationNs = 1e9 * std::pow(2.0, spreadingFactor) / bandwidthHz;
    double snrLinear = std::max(0.1, std::pow(10.0, snrDb / 10.0));

    // Early-Late gate tracking loop reduces initial offset
    double loopTrackingEfficiency = std::clamp(1.0 - 0.1 / snrLinear, 0.2, 0.95);
    res.residualTimingOffsetNs = initialTimingOffsetNs * (1.0 - loopTrackingEfficiency);

    // Timing jitter inversely proportional to SNR
    double baseJitterNs = 100.0 / std::sqrt(snrLinear);
    res.timingJitterNs = std::abs(m_sampler.Sample(DistributionParam::Normal(0.0, baseJitterNs, -5.0 * baseJitterNs, 5.0 * baseJitterNs)));

    res.residualTimingOffsetNs += res.timingJitterNs;

    double timingDriftFraction = std::abs(res.residualTimingOffsetNs) / symbolDurationNs;

    res.timingLockQuality = std::clamp(1.0 - (timingDriftFraction / m_params.timingGateFraction), 0.0, 1.0);
    res.preambleAcquisitionScore = std::clamp(res.timingLockQuality * (snrLinear / (snrLinear + 1.0)), 0.0, 1.0);

    return res;
}

} // namespace ns3
