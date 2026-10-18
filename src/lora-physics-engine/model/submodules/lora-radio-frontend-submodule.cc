#include "lora-radio-frontend-submodule.h"
#include "ns3/log.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

static bool g_agcPerReceiver = false;

void RadioFrontendSubmodule::SetAgcPerReceiver(bool on)
{
    g_agcPerReceiver = on;
}


NS_LOG_COMPONENT_DEFINE("LoRaRadioFrontendSubmodule");

RadioFrontendSubmodule::RadioFrontendSubmodule(const RadioFrontendConfigParams& params, LoRaDistributionSampler& sampler)
    : m_params(params), m_sampler(sampler), m_lastAgcTriggerTime(Seconds(0)), m_lastAgcAttenuationDb(0.0)
{
    NS_LOG_FUNCTION(this);
}

RadioFrontendResult RadioFrontendSubmodule::ProcessAnalogFrontend(
    double initialCfoHz, double ambientTemperatureCelsius,
    Time simulationTime, double rxPowerDbm, 
    double strongestInterfererPowerDbm)
{
    // B6: select the AGC state -- the engine's single one (default) or this receiver's own.
    Time& agcTime = g_agcPerReceiver ? m_agcPerRx[m_rxId].first : m_lastAgcTriggerTime;
    double& agcAtt = g_agcPerReceiver ? m_agcPerRx[m_rxId].second : m_lastAgcAttenuationDb;
    NS_LOG_FUNCTION(this << initialCfoHz << ambientTemperatureCelsius << simulationTime.GetSeconds() << rxPowerDbm << strongestInterfererPowerDbm);

    RadioFrontendResult res;
    res.oscillatorDriftHz = 0.0;
    res.thermalDriftHz = 0.0;
    res.phaseNoisePsd = 0.0;
    res.spurResponseDb = -200.0;
    res.agcRecoveryTimeMs = 0.0;
    res.frontendDesensitizationDb = 0.0;

    if (!m_params.enabled) {
        return res;
    }

    // 1. Oscillator Aging
    // Slowly drifts CFO over simulation time (minutes/hours/years)
    double yearsElapsed = simulationTime.GetSeconds() / (3600.0 * 24.0 * 365.0);
    res.oscillatorDriftHz = yearsElapsed * m_params.oscillatorAgingDriftHzPerYear;

    // 2. Thermal Drift
    // Oscillator stability alters with temperature variation from nominal 25C
    double deltaTemp = ambientTemperatureCelsius - 25.0;
    res.thermalDriftHz = deltaTemp * m_params.thermalDriftHzPerCelsius;

    // 3. PLL Noise Spectrum (1/f phase noise PSD approximation)
    // Assumes a standard phase noise shape at a baseline offset
    res.phaseNoisePsd = -90.0 - m_sampler.Sample(DistributionParam::Uniform(0.0, 10.0));

    // 4. Mixer Spurs & Intermodulation
    // Strong interferers create spurious mirror responses
    if (strongestInterfererPowerDbm > -50.0) {
        double spurMagnitude = strongestInterfererPowerDbm - 40.0; // Simplistic IMD3 spur proxy
        res.spurResponseDb = spurMagnitude + m_sampler.Sample(DistributionParam::Normal(0.0, 3.0, -10.0, 10.0));
    }

    // 5. AGC Hysteresis and Frontend Desensitization
    double timeSinceLastAgcMs = (simulationTime - agcTime).GetMilliSeconds();
    
    if (strongestInterfererPowerDbm > -40.0 || rxPowerDbm > -40.0) {
        // Front-end saturation triggered (Smooth nonlinear compression)
        double maxPower = std::max(strongestInterfererPowerDbm, rxPowerDbm);
        double attenuationDb = (maxPower + 40.0); // 1 dB attenuation for every dB over -40 dBm
        
        agcAtt = std::min(50.0, attenuationDb);
        agcTime = simulationTime;
        res.agcRecoveryTimeMs = m_params.agcRecoveryTimeMs * agcAtt; // Dynamic recovery time
        res.frontendDesensitizationDb = agcAtt;
    } else {
        // Recovery phase: time constants govern gain recovery
        if (timeSinceLastAgcMs > 0.0 && agcAtt > 0.0) {
            double decayRate = agcAtt / m_params.agcRecoveryTimeMs;
            double remainingAttenuation = agcAtt - (timeSinceLastAgcMs * decayRate);
            
            if (remainingAttenuation > 0.0) {
                res.frontendDesensitizationDb = remainingAttenuation;
                res.agcRecoveryTimeMs = remainingAttenuation / decayRate;
            } else {
                agcAtt = 0.0;
            }
        }
    }

    return res;
}

} // namespace ns3
