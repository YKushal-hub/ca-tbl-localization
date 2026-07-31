#include "lora-spectrum-occupancy-submodule.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaSpectrumOccupancySubmodule");

SpectrumOccupancySubmodule::SpectrumOccupancySubmodule(const SpectrumOccupancyConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler)
{
    NS_LOG_FUNCTION(this);
}

SpectrumOccupancyResult SpectrumOccupancySubmodule::EvaluateSpectrumEnvironment(Time simulationTime, double bandwidthHz)
{
    NS_LOG_FUNCTION(this << simulationTime.GetSeconds() << bandwidthHz);

    SpectrumOccupancyResult res;
    res.spectrumOccupancyDbm = m_params.backgroundIsmNoiseFloorDbm;
    res.impulsiveNoiseEvents = 0;
    res.coloredNoiseVariance = 0.0;

    if (!m_params.enabled) {
        return res;
    }

    // 1. Time-Varying Spectrum Maps (Periodic diurnal variations)
    // Simulates macro-scale ISM occupancy evolution
    double hourOfDay = std::fmod(simulationTime.GetSeconds() / 3600.0, 24.0);
    // Peak ISM traffic around 8 PM (20.0), minimum around 4 AM
    double diurnalFactor = std::cos((hourOfDay - 20.0) * M_PI / 12.0);
    res.spectrumOccupancyDbm += diurnalFactor * 5.0;

    // 3. Colored Noise
    // True RF noise is not perfectly flat AWGN, variance represents PSD slope
    res.coloredNoiseVariance = m_sampler.Sample(DistributionParam::LogNormal(0.0, 0.5, 0.1, 5.0));
    res.spectrumOccupancyDbm += res.coloredNoiseVariance;

    // 2. Burst Interference
    // Simulates short impulsive noise events (WiFi beacons, BLE bursts, Microwave pulses)
    if (m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0)) < m_params.impulsiveNoiseProbability) {
        res.impulsiveNoiseEvents = (uint32_t)m_sampler.Sample(DistributionParam::Uniform(1.0, 5.0));
        
        // Impulsive events drastically spike the instantaneous effective noise floor
        res.spectrumOccupancyDbm += 10.0 + m_sampler.Sample(DistributionParam::Normal(5.0, 2.0, 0.0, 20.0));
    }

    return res;
}

} // namespace ns3
