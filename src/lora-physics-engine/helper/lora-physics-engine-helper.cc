#include "lora-physics-engine-helper.h"
#include "ns3/log.h"
#include <iostream>
#include <iomanip>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRaPhysicsEngineHelper");

LoRaPhysicsEngineHelper::LoRaPhysicsEngineHelper()
    : m_loggingEnabled(false)
{
    m_engine = CreateObject<LoRaPhysicsEngine>();
    LoRaPhysicsEngine::SetInstance(m_engine);
}

LoRaPhysicsEngineHelper::~LoRaPhysicsEngineHelper()
{
    CloseResultLogging();
}

void LoRaPhysicsEngineHelper::Initialize(const LoRaPhysicsConfig& config)
{
    m_engine->Initialize(config);
}

void LoRaPhysicsEngineHelper::Initialize(const std::string& yamlPath)
{
    m_engine->LoadConfig(yamlPath);
}

Ptr<LoRaPhysicsEngine> LoRaPhysicsEngineHelper::GetEngine() const
{
    return m_engine;
}

void LoRaPhysicsEngineHelper::EnableResultLogging(const std::string& csvPath)
{
    CloseResultLogging();
    
    m_csvPath = csvPath;
    m_csvFile.open(csvPath);
    if (!m_csvFile.is_open()) {
        NS_LOG_ERROR("LoRaPhysicsEngineHelper: Failed to open CSV log file at " << csvPath);
        m_loggingEnabled = false;
        return;
    }
    
    // Write detailed comprehensive CSV telemetry header
    m_csvFile << "txNodeId,rxGatewayId,spreadingFactor,distanceM,pathLossDb,shadowingDb,fadingDb,nlosDb,"
              << "atmosphericDb,foliageDb,dopplerDb,terrainDb,interferenceDb,thermalNoiseShiftDb,"
              << "hwTxVariationDb,rxPowerDbm,noiseFloorDbm,finalSNRDb,packetReceived,captureEventOccurred,"
              << "trueArrivalTimeNs,measuredArrivalTimeNs,cfoHz,phaseNoiseRad,relayHops,"
              << "tx_power_dbm,measured_rssi_dbm,true_rssi_dbm,snr_db,raw_snr_db,sensitivity_dbm,sensitivity_margin_db,"
              << "collision_occurred,overlap_probability,"
              << "delay_spread_ns,coherence_bandwidth_hz,fresnel_clearance_ratio,terrain_los_blocked,"
              << "collision_overlap_fraction,capture_margin_db,antenna_gain_db,doppler_phase_error_rad,"
              << "interferer_count,strongest_interferer_dbm,correlated_obstruction_factor,"
              << "demod_success_probability,fft_bin_offset,timing_sync_error_ns,evm_percent,"
              << "iq_image_rejection_db,adc_clipping_occurred,active_cluster_count,strongest_cluster_power_db,"
              << "cluster_turnover_rate,symbol_collision_fraction,preamble_locked,payload_crc_success,"
              << "sync_corruption_occurred,multi_edge_loss_db,dominant_edge_height_m,terrain_reflection_count,"
              << "hidden_channel_state,doppler_spectrum_width_hz,"
              << "symbol_error_probability,packet_crc_probability,soft_symbol_margin,"
              << "residual_cfo_hz,residual_timing_error_ns,sync_confidence,agc_convergence,"
              << "sic_recovered,fidelity_level,cluster_spatial_stability,environment_region,phase_coherence,"
              << "iq_phase_continuity,coherent_energy_ratio,instantaneous_frequency_error_hz,"
              << "fft_peak_confidence,symbol_likelihood,crc_confidence,pll_locked,pll_phase_error_rad,"
              << "cycle_slip_occurred,timing_lock_quality,timing_jitter_ns,fidelity_scheduler_state,"
              << "batch_processing_gain,phase_noise_variance,combined_snr_db,gateway_diversity_order,"
              << "cooperative_decode_success,rf_memory_region_id\n";
              
    m_loggingEnabled = true;
}

void LoRaPhysicsEngineHelper::CloseResultLogging()
{
    if (m_loggingEnabled && m_csvFile.is_open()) {
        m_csvFile.flush();
        m_csvFile.close();
    }
    m_loggingEnabled = false;
}

PhysicsResult LoRaPhysicsEngineHelper::Process(
    uint32_t txNodeId, uint32_t rxGatewayId,
    Vector txPosition, Vector rxPosition,
    double txPowerDbm, uint8_t spreadingFactor,
    double bandwidthHz, uint8_t relayHops,
    Time currentTime)
{
    PhysicsResult res = m_engine->ProcessTransmission(
        txNodeId, rxGatewayId, txPosition, rxPosition,
        txPowerDbm, spreadingFactor, bandwidthHz, relayHops, currentTime);
        
    if (m_loggingEnabled && m_csvFile.is_open()) {
        double dist = CalculateDistance(txPosition, rxPosition);
        
        m_csvFile << txNodeId << ","
                  << rxGatewayId << ","
                  << (int)spreadingFactor << ","
                  << std::fixed << std::setprecision(3) << dist << ","
                  << res.pathLossDb << ","
                  << res.shadowingDb << ","
                  << res.fadingDb << ","
                  << res.nlosDb << ","
                  << res.atmosphericDb << ","
                  << res.foliageDb << ","
                  << res.dopplerPenaltyDb << ","
                  << res.terrainDb << ","
                  << res.interferencePenaltyDb << ","
                  << res.thermalNoiseShiftDb << ","
                  << res.hwTxVariationDb << ","
                  << res.rxPowerDbm << ","
                  << res.noiseFloorDbm << ","
                  << res.finalSNRDb << ","
                  << (res.packetReceived ? 1 : 0) << ","
                  << (res.captureEventOccurred ? 1 : 0) << ","
                  << std::setprecision(1) << res.timestamp.trueArrivalTimeNs << ","
                  << res.timestamp.measuredArrivalTimeNs << ","
                  << std::setprecision(4) << res.timestamp.cfoHz << ","
                  << res.timestamp.phaseNoiseRad << ","
                  << (int)relayHops << ","
                  << std::setprecision(2) << res.effectiveTxPowerDbm << ","
                  << res.measuredRssiDbm << ","
                  << res.trueRssiDbm << ","
                  << res.snr_db << ","
                  << res.raw_snr_db << ","
                  << res.effectiveSensitivityDbm << ","
                  << res.sensitivityMarginDb << ","
                  << (res.collisionOccurred ? 1 : 0) << ","
                  << std::setprecision(4) << res.interferenceOverlapProbability << ","
                  << std::setprecision(2) << res.delaySpreadNs << ","
                  << std::setprecision(1) << res.coherenceBandwidthHz << ","
                  << std::setprecision(4) << res.fresnelClearanceRatio << ","
                  << (res.terrainLosBlocked ? 1 : 0) << ","
                  << std::setprecision(4) << res.collisionOverlapFraction << ","
                  << std::setprecision(2) << res.captureMarginDb << ","
                  << res.antennaGainDb << ","
                  << std::setprecision(5) << res.dopplerPhaseErrorRad << ","
                  << res.interfererCount << ","
                  << std::setprecision(2) << res.strongestInterfererDbm << ","
                  << std::setprecision(4) << res.correlatedObstructionFactor << ","
                  << std::setprecision(4) << res.demodSuccessProbability << ","
                  << res.fftBinOffset << ","
                  << std::setprecision(1) << res.timingSyncErrorNs << ","
                  << std::setprecision(2) << res.evmPercent << ","
                  << res.iqImageRejectionDb << ","
                  << (res.adcClippingOccurred ? 1 : 0) << ","
                  << res.activeClusterCount << ","
                  << res.strongestClusterPowerDb << ","
                  << std::setprecision(4) << res.clusterTurnoverRate << ","
                  << res.symbolCollisionFraction << ","
                  << (res.preambleLocked ? 1 : 0) << ","
                  << (res.payloadCrcSuccess ? 1 : 0) << ","
                  << (res.syncCorruptionOccurred ? 1 : 0) << ","
                  << std::setprecision(2) << res.multiEdgeLossDb << ","
                  << res.dominantEdgeHeightM << ","
                  << res.terrainReflectionCount << ","
                  << res.hiddenChannelState << ","
                  << std::setprecision(2) << res.dopplerSpectrumWidthHz << ","
                  << std::setprecision(6) << res.symbolErrorProbability << ","
                  << res.packetCrcProbability << ","
                  << std::setprecision(2) << res.softSymbolMarginDb << ","
                  << std::setprecision(3) << res.residualCfoHz << ","
                  << std::setprecision(1) << res.residualTimingErrorNs << ","
                  << std::setprecision(3) << res.syncConfidence << ","
                  << res.agcConvergence << ","
                  << (res.sicRecovered ? 1 : 0) << ","
                  << res.fidelityLevel << ","
                  << std::setprecision(4) << res.clusterSpatialStability << ","
                  << res.environmentRegion << ","
                  << std::setprecision(4) << res.phaseCoherence << ","
                  << std::setprecision(4) << res.iqPhaseContinuity << ","
                  << res.coherentEnergyRatio << ","
                  << std::setprecision(2) << res.instantaneousFrequencyErrorHz << ","
                  << std::setprecision(4) << res.fftPeakConfidence << ","
                  << res.symbolLikelihood << ","
                  << res.crcConfidence << ","
                  << (res.pllLocked ? 1 : 0) << ","
                  << std::setprecision(4) << res.pllPhaseErrorRad << ","
                  << (res.cycleSlipOccurred ? 1 : 0) << ","
                  << std::setprecision(4) << res.timingLockQuality << ","
                  << std::setprecision(2) << res.timingJitterNs << ","
                  << res.fidelitySchedulerState << ","
                  << std::setprecision(3) << res.batchProcessingGain << ","
                  << std::setprecision(5) << res.phaseNoiseVariance << ","
                  << std::setprecision(2) << res.combinedSnrDb << ","
                  << res.gatewayDiversityOrder << ","
                  << (res.cooperativeDecodeSuccess ? 1 : 0) << ","
                  << res.rfMemoryRegionId << "\n";
    }
    
    return res;
}

} // namespace ns3
