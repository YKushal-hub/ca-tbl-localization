#include "lora-hardware-submodule.h"
#include <cmath>

namespace ns3 {

HardwareSubmodule::HardwareSubmodule(const HardwareParams& params,
                                     LoRaDistributionSampler& sampler)
    : m_params(params),
      m_sampler(sampler)
{
}

void HardwareSubmodule::InitializeGateway(uint32_t gatewayId)
{
    if (m_gatewayCFO.count(gatewayId) == 0) {
        // Sample persistent per-gateway offsets once
        m_gatewayCFO[gatewayId] = m_sampler.SamplePersistent(
            m_params.carrierFrequencyOffsetHz, gatewayId, "gw_cfo");
        
        m_gatewayRssiOffset[gatewayId] = m_sampler.SamplePersistent(
            m_params.rssiHardwareVariationDb, gatewayId, "gw_rssi_offset");
        
        m_gatewaySensitivityOffset[gatewayId] = m_sampler.SamplePersistent(
            m_params.sensitivityVariationDb, gatewayId, "gw_sens_offset");
        
        m_gatewayTimestampJitter[gatewayId] = m_sampler.SamplePersistent(
            m_params.timestampJitterNs, gatewayId, "gw_ts_jitter");
        if (m_sampler.SamplePersistent(DistributionParam::Uniform(0.0, 1.0), gatewayId, "gw_ts_sign") > 0.5) {
            m_gatewayTimestampJitter[gatewayId] = -m_gatewayTimestampJitter[gatewayId];
        }
    }
}

void HardwareSubmodule::ResetGatewayJitter(const HardwareParams& params)
{
    m_gatewayTimestampJitter.clear();
    for (auto const& pair : m_gatewayCFO) {
        uint32_t gatewayId = pair.first;
        m_gatewayTimestampJitter[gatewayId] = m_sampler.SamplePersistent(
            params.timestampJitterNs, gatewayId, "gw_ts_jitter_fine");
        if (m_sampler.SamplePersistent(DistributionParam::Uniform(0.0, 1.0), gatewayId, "gw_ts_sign_fine") > 0.5) {
            m_gatewayTimestampJitter[gatewayId] = -m_gatewayTimestampJitter[gatewayId];
        }
    }
}

void HardwareSubmodule::InitializeNode(uint32_t nodeId)
{
    if (m_nodeTxPowerOffset.count(nodeId) == 0) {
        // Sample persistent per-node TX power deviation once
        m_nodeTxPowerOffset[nodeId] = m_sampler.SamplePersistent(
            m_params.txPowerAccuracyDb, nodeId, "node_tx_offset");
    }
}

double HardwareSubmodule::ApplyTxPowerVariation(double nominalTxPowerDbm, uint32_t nodeId)
{
    if (!m_params.enabled) {
        return nominalTxPowerDbm;
    }
    
    if (m_params.perNodeVariationEnabled) {
        InitializeNode(nodeId);
        return nominalTxPowerDbm + m_nodeTxPowerOffset[nodeId];
    }
    
    return nominalTxPowerDbm;
}

double HardwareSubmodule::ApplyRxSensitivityVariation(double nominalSensitivityDbm, uint32_t gatewayId)
{
    if (!m_params.enabled) {
        return nominalSensitivityDbm;
    }
    
    InitializeGateway(gatewayId);
    return nominalSensitivityDbm + m_gatewaySensitivityOffset[gatewayId];
}

double HardwareSubmodule::ApplyRSSIErrors(double trueRssiDbm, uint32_t gatewayId)
{
    if (!m_params.enabled) {
        return trueRssiDbm;
    }

    InitializeGateway(gatewayId);
    double biasedRSSI = trueRssiDbm + m_gatewayRssiOffset[gatewayId];

    // Quantize RSSI to SX1276 step resolution (typically 1.0 dB steps)
    double resolution = m_params.rssiResolutionDb;
    if (resolution <= 0.0) {
        resolution = 1.0;
    }
    double quantizedRSSI = std::round(biasedRSSI / resolution) * resolution;
    return quantizedRSSI;
}

TimestampObservation HardwareSubmodule::GenerateTimestampObservation(
    uint32_t nodeId, uint32_t gatewayId,
    Time trueArrival, double trueRssiDbm, double snrDb,
    uint8_t sf, uint8_t relayHops)
{
    InitializeGateway(gatewayId);
    
    double trueArrivalNs = trueArrival.GetNanoSeconds();
    double measuredArrivalNs = trueArrivalNs;
    double measuredRssiDbm = ApplyRSSIErrors(trueRssiDbm, gatewayId);
    double phaseNoise = 0.0;
    
    if (m_params.enabled) {
        // Sample dynamic per-packet jitter
        double perPacketJitter = m_sampler.Sample(m_params.timestampJitterNs);
        if (m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0)) > 0.5) {
            perPacketJitter = -perPacketJitter;
        }
        
        // 50% of the jitter is persistent per gateway, 50% is packet-level variance
        double gwJitter = m_gatewayTimestampJitter[gatewayId];
        double totalJitter = 0.5 * gwJitter + 0.5 * perPacketJitter;
        
        measuredArrivalNs = trueArrivalNs + totalJitter;
        
        // Apply hardware clock quantization steps
        double quantization = m_sampler.Sample(m_params.timestampQuantizationNs);
        if (quantization > 0.0) {
            measuredArrivalNs = std::round(measuredArrivalNs / quantization) * quantization;
        }

        phaseNoise = m_sampler.Sample(m_params.phaseNoiseRad);
    }

    TimestampObservation obs;
    obs.nodeId = nodeId;
    obs.gatewayId = gatewayId;
    obs.trueArrivalTimeNs = trueArrivalNs;
    obs.measuredArrivalTimeNs = measuredArrivalNs;
    obs.trueRssiDbm = trueRssiDbm;
    obs.measuredRssiDbm = measuredRssiDbm;
    obs.snrDb = snrDb;
    obs.spreadingFactor = sf;
    obs.relayHops = relayHops;
    obs.captureOccurred = false; // default, main engine will override if captured
    obs.cfoHz = m_gatewayCFO[gatewayId];
    obs.phaseNoiseRad = phaseNoise;

    return obs;
}

// Feature 6: Temperature-dependent CFO computation (coefficient OURS; Robyns 2017 WiSec shows none)
// CFO = coefficient * (T - T_ref) for oscillator frequency temperature dependency
// SX1272 typical coefficient: 1.5 Hz/°C at 868 MHz (can range 0.5-3.0 Hz/°C)
double HardwareSubmodule::ComputeTemperatureCFO(double temperatureCelsius,
                                                 const TemperatureCfoParams& params)
{
    // Sample temperature coefficient if not already known
    double temperatureCoefficientHzPerC = m_sampler.Sample(params.temperatureCoefficientHzPerC);
    
    // Compute temperature deviation from reference
    double temperatureDeviationC = temperatureCelsius - params.referenceTemperatureC;
    
    // CFO = coefficient * ΔT
    double cfoHz = temperatureCoefficientHzPerC * temperatureDeviationC;
    
    // Add thermal jitter component (temperature-induced PLL jitter)
    double jitterSensitivity = m_sampler.Sample(params.jitterTempSensitivity);
    double thermalJitterHz = jitterSensitivity * std::abs(temperatureDeviationC);
    
    // Total temperature effect on CFO
    double totalCFOHz = cfoHz + thermalJitterHz;
    
    return totalCFOHz;
}

} // namespace ns3
