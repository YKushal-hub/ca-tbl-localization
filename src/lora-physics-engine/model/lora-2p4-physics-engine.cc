#include "lora-2p4-physics-engine.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("LoRa2p4PhysicsEngine");

NS_OBJECT_ENSURE_REGISTERED(LoRa2p4PhysicsEngine);

static Ptr<LoRa2p4PhysicsEngine> g_physicsEngineInstance = nullptr;

Ptr<LoRa2p4PhysicsEngine> LoRa2p4PhysicsEngine::GetInstance() {
    return g_physicsEngineInstance;
}

void LoRa2p4PhysicsEngine::SetInstance(Ptr<LoRa2p4PhysicsEngine> instance) {
    g_physicsEngineInstance = instance;
}

TypeId LoRa2p4PhysicsEngine::GetTypeId() {
    static TypeId tid = TypeId("ns3::LoRa2p4PhysicsEngine")
        .SetParent<Object>()
        .SetGroupName("Lora")
        .AddConstructor<LoRa2p4PhysicsEngine>();
    return tid;
}

LoRa2p4PhysicsEngine::LoRa2p4PhysicsEngine()
    : m_initialized(false)
{
    NS_LOG_FUNCTION(this);
}

LoRa2p4PhysicsEngine::~LoRa2p4PhysicsEngine() {
    NS_LOG_FUNCTION(this);
}

void LoRa2p4PhysicsEngine::Initialize(const LoRa2p4PhysicsConfig& config) {
    NS_LOG_FUNCTION(this);
    m_config = config;
    
    // Seed and instantiate the distribution sampler
    m_sampler = LoRaDistributionSampler(config.randomSeed);
    
    // Seed and instantiate the fading sampler
    if (config.fading.useIsolatedSeed) {
        m_fadingSampler = LoRaDistributionSampler(config.fading.isolatedSeed);
    } else {
        m_fadingSampler = LoRaDistributionSampler(config.randomSeed);
    }
    
    m_correlation = std::make_unique<LoRaCorrelationEngine>();
    m_spatial = std::make_unique<LoRaSpatialFieldEngine>();
    m_shadowing = std::make_unique<ShadowingSubmodule>(m_config.shadowing, m_sampler);
    m_interference = std::make_unique<InterferenceSubmodule>();
    m_hardware = std::make_unique<HardwareSubmodule>(m_config.hardware, m_sampler);
    m_atmospheric = std::make_unique<AtmosphericSubmodule>(m_config.atmospheric, m_sampler);
    m_delaySpread = std::make_unique<DelaySpreadSubmodule>();
    m_antenna = std::make_unique<AntennaSubmodule>();
    m_terrain = std::make_unique<TerrainSubmodule>();
    m_cssPhy = std::make_unique<CSSPHYSubmodule>();
    m_iqImpairment = std::make_unique<IQImpairmentSubmodule>(m_config.iqImpairment, m_sampler);
    m_baseband = std::make_unique<BasebandSubmodule>();
    m_syncLoop = std::make_unique<SyncLoopSubmodule>();
    m_region = std::make_unique<SemanticRegionSubmodule>();
    m_crossLayer = std::make_shared<LoRaCrossLayerEngine>();
    m_validator = std::make_unique<LoRaSdrValidator>();
    
    // Tier 2 Submodules
    m_buildingEntry = std::make_unique<BuildingEntrySubmodule>(m_config.buildingEntry, m_config.environment, m_sampler);
    m_wallMaterial = std::make_unique<WallMaterialSubmodule>(m_config.wallMaterial, m_config.environment, m_sampler);
    m_floorAttenuation = std::make_unique<FloorAttenuationSubmodule>(m_config.floorAttenuation, m_config.environment, m_sampler);
    m_nodePositionUncertainty = std::make_unique<NodePositionUncertaintySubmodule>(m_config.nodePositionUncertainty, m_config.environment, m_sampler);
    m_temporalBlockage = std::make_unique<TemporalBlockageSubmodule>(m_config.temporalBlockage, m_config.environment, m_sampler);
    m_bodyShadowing = std::make_unique<BodyShadowingSubmodule>(m_config.bodyShadowing, m_config.environment, m_sampler);
    
    // Tier 3 Submodules
    m_tunnel = std::make_unique<TunnelSubmodule>(m_config.tunnel, m_config.environment, m_sampler);
    m_twoRayGround = std::make_unique<TwoRayGroundSubmodule>(m_config.twoRayGround, m_config.environment);
    m_soilAttenuation = std::make_unique<SoilAttenuationSubmodule>(m_config.soilAttenuation, m_config.environment, m_sampler);
    
    // Stage 5 Emulation Framework Submodules
    m_iqStream = std::make_unique<IQStreamSubmodule>(m_config.iqStream, m_sampler);
    m_dspChain = std::make_unique<DspChainSubmodule>(m_config.dspChain, m_sampler);
    m_pll = std::make_unique<PllSubmodule>(m_config.pll, m_sampler);
    m_symbolSync = std::make_unique<SymbolSyncSubmodule>(m_config.symbolSync, m_sampler);
    m_rfNonlinearity = std::make_unique<RFNonlinearitySubmodule>(m_config.rfNonlinearity, m_sampler);
    
    m_scalability = std::make_unique<LoRaScalabilityEngine>();
    m_scalability->Initialize(m_config.scalability);
    
    m_geographicMemory = std::make_unique<LoRaGeographicMemoryEngine>();
    m_geographicMemory->Initialize(m_config.geographicMemory);
    
    m_validationSuite = std::make_unique<LoRaValidationSuite>();
    m_validationSuite->Initialize(m_config.advancedValidation);
    
    m_cooperativePhy = std::make_unique<LoRaCooperativePhyEngine>();
    m_cooperativePhy->Initialize(m_config.cooperativePhy);
    
    // Stage 6 Scale and Realism Submodules
    m_microDsp = std::make_unique<MicroDspSubmodule>(m_config.microDsp, m_sampler);
    m_radioFrontend = std::make_unique<RadioFrontendSubmodule>(m_config.radioFrontend, m_sampler);
    m_spectrumOccupancy = std::make_unique<SpectrumOccupancySubmodule>(m_config.spectrumOccupancy, m_sampler);
    m_distributedPhy = std::make_unique<DistributedPhyEngine>(m_config.distributedPhy);
    m_globalValidation = std::make_unique<GlobalValidationEngine>(m_config.globalValidation);
    m_distributedScalability = std::make_unique<DistributedScalabilityEngine>(m_config.distributedScalability);
    m_adaptiveEnvironment = std::make_unique<AdaptiveEnvironmentEngine>(m_config.adaptiveEnvironment);
    
    m_fadingStates.clear();
    ResetFadingState();
    m_linkHysteresisActive.clear();
    m_hiddenChannelStates.clear();
    m_lastStateTransition.clear();
    m_initialized = true;
}

void LoRa2p4PhysicsEngine::LoadConfig(const std::string& yamlPath) {
    LoRa2p4PhysicsConfig config = LoRa2p4PhysicsConfig::FromYAML(yamlPath);
    Initialize(config);
}

uint64_t LoRa2p4PhysicsEngine::MakeLinkId(uint32_t txId, uint32_t rxId) {
    return (static_cast<uint64_t>(txId) << 32) | rxId;
}

bool LoRa2p4PhysicsEngine::PassesReceiverSensitivity(double rxPowerDbm, double effectiveSensitivityDbm) {
    if (!m_config.receiver.softThresholdEnabled) {
        return rxPowerDbm >= effectiveSensitivityDbm;
    }

    double widthDb = std::max(1e-6, m_config.receiver.softThresholdWidthDb);
    double marginDb = rxPowerDbm - effectiveSensitivityDbm;
    double passProbability = 1.0 / (1.0 + std::exp(-marginDb / widthDb));
    double draw = m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0));
    return draw < passProbability;
}

double LoRa2p4PhysicsEngine::GetSensitivityThreshold(uint8_t sf, uint8_t codingRateDenom) {
    // D1 (DEFECTS.md). This table is EIGHT entries based at SF5 -- see the
    // declaration comment, "index 0=SF5, 7=SF12". The 868 table is six entries
    // based at SF7 and its accessor's `sf - 7` is correct there; this accessor
    // was copied without re-basing, so SF7 read index 0 (the SF5 cell) and
    // indices 6 and 7 were unreachable. An eight-entry LoRa SF table cannot be
    // SF7-based: SF7..SF14 does not exist. Corroborated by the SX1280 datasheet,
    // which puts SF7 near -106 dBm, not the -100 dBm sitting at index 0.
    // CORRECTION 2026-10-15: -106 dBm is the data sheet's SF7 value at 1625 kHz. At the
    // 800 kHz tier 2 actually runs at it is -112 dBm, so this index fix landed on the wrong
    // bandwidth row (D-8, gate G19, --t2SensitivityRowKHz).
    int sfIndex = std::min(7, std::max(0, static_cast<int>(sf) - 5));
    int crIndex = std::min(3, std::max(0, static_cast<int>(codingRateDenom) - 5));
    return m_config.receiver.sensitivityDbm[sfIndex] + m_config.receiver.codingRateSensitivityOffsetDb[crIndex];
}

void LoRa2p4PhysicsEngine::RegisterGateway(uint32_t gatewayId, Vector position, bool fineTimestamp) {
    GatewayState gw;
    gw.position = position;
    gw.fineTimestamp = fineTimestamp;
    m_gatewayStates[gatewayId] = gw;
}

void LoRa2p4PhysicsEngine::RegisterNode(uint32_t nodeId, Vector initialPosition) {
    NodeState ns;
    ns.position = initialPosition;
    ns.velocityMps = 0.0;
    ns.headingRad = 0.0;
    ns.lastPositionUpdate = Simulator::Now();
    m_nodeStates[nodeId] = ns;
}

void LoRa2p4PhysicsEngine::UpdateNodePosition(uint32_t nodeId, Vector newPosition) {
    Time now = Simulator::Now();
    if (m_nodeStates.count(nodeId) == 0) {
        RegisterNode(nodeId, newPosition);
        return;
    }
    
    NodeState& ns = m_nodeStates[nodeId];
    Time elapsed = now - ns.lastPositionUpdate;
    double dt = elapsed.GetSeconds();
    
    if (dt > 1e-4) {
        double dx = newPosition.x - ns.position.x;
        double dy = newPosition.y - ns.position.y;
        double dz = newPosition.z - ns.position.z;
        double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        ns.velocityMps = dist / dt;
        ns.headingRad = std::atan2(dy, dx);
    }
    
    ns.position = newPosition;
    ns.lastPositionUpdate = now;
}

void LoRa2p4PhysicsEngine::SetNodeVelocity(uint32_t nodeId, double velocityMps, double headingRad) {
    if (m_nodeStates.count(nodeId) == 0) {
        RegisterNode(nodeId, Vector(0,0,0));
    }
    m_nodeStates[nodeId].velocityMps = velocityMps;
    m_nodeStates[nodeId].headingRad = headingRad;
}

WeatherState LoRa2p4PhysicsEngine::GetCurrentWeather() const {
    if (m_atmospheric) {
        return m_atmospheric->GetCurrentState();
    }
    return m_config.initialWeather;
}

void LoRa2p4PhysicsEngine::SetWeatherState(WeatherState state) {
    m_config.initialWeather = state;
    if (m_atmospheric) {
        // Re-initialize atmospheric model with new weather state
        m_config.atmospheric = AtmosphericParams::DefaultsFor(state);
        m_atmospheric = std::make_unique<AtmosphericSubmodule>(m_config.atmospheric, m_sampler);
    }
}

EngineStatistics2p4 LoRa2p4PhysicsEngine::GetStatistics() const {
    return m_stats;
}

void LoRa2p4PhysicsEngine::ResetStatistics() {
    m_stats = EngineStatistics2p4();
}

PhysicsResult2p4 LoRa2p4PhysicsEngine::ProcessTransmission(
    uint32_t txNodeId, uint32_t rxGatewayId,
    Vector txPosition, Vector rxPosition,
    double txPowerDbm, uint8_t spreadingFactor,
    double bandwidthHz, uint8_t relayHops,
    Time currentTime, double nodeOrientationRadians) {
    NS_LOG_FUNCTION(this << txNodeId << rxGatewayId << txPowerDbm << (int)spreadingFactor);
    PhysicsResult2p4 result;
    LinkPhysicsState state;
    state.txNodeId = txNodeId;
    state.nodeOrientationRadians = nodeOrientationRadians;
    state.txPos = txPosition;
    state.rxPos = rxPosition;
    // P4: Propagate tunnel flag from config so result.isTunnel is correctly set
    state.isTunnel = m_config.tunnel.enabled;
    
    // Default Stage 3 SDR Telemetry Initialization
    result.demodSuccessProbability = 1.0;
    result.fftBinOffset = 0.0;
    result.timingSyncErrorNs = 0.0;
    result.evmPercent = 0.0;
    result.iqImageRejectionDb = 60.0;
    result.adcClippingOccurred = false;
    result.activeClusterCount = 3;
    result.strongestClusterPowerDb = -100.0;
    result.clusterTurnoverRate = 0.0;
    result.symbolCollisionFraction = 0.0;
    result.preambleLocked = true;
    result.payloadCrcSuccess = true;
    result.syncCorruptionOccurred = false;
    result.multiEdgeLossDb = 0.0;
    result.dominantEdgeHeightM = 0.0;
    result.terrainReflectionCount = 0;
    result.hiddenChannelState = 0;
    result.dopplerSpectrumWidthHz = 0.0;
    
    // Stage 5 Final Advanced Emulation Framework Telemetry Initialization
    result.iqPhaseContinuity = 1.0;
    result.coherentEnergyRatio = 1.0;
    result.instantaneousFrequencyErrorHz = 0.0;
    result.fftPeakConfidence = 1.0;
    result.symbolLikelihood = 1.0;
    result.crcConfidence = 1.0;
    result.pllLocked = true;
    result.pllPhaseErrorRad = 0.0;
    result.cycleSlipOccurred = false;
    result.timingLockQuality = 1.0;
    result.timingJitterNs = 0.0;
    result.fidelitySchedulerState = FIDELITY_SDR_GRADE_REALISM;
    result.batchProcessingGain = 1.0;
    result.phaseNoiseVariance = 0.0;
    result.combinedSnrDb = -100.0;
    result.gatewayDiversityOrder = 1;
    result.cooperativeDecodeSuccess = true;
    result.rfMemoryRegionId = 0;
    
    // Stage 6 Radio Frontend defaults (zeroed so hardware.enabled=false paths are deterministic)
    result.oscillatorDriftHz = 0.0;
    result.thermalDriftHz = 0.0;
    result.phaseNoisePsd = 0.0;
    result.spurResponseDb = 0.0;
    result.agcRecoveryTimeMs = 0.0;
    result.frontendDesensitizationDb = 0.0;
    if (!m_initialized) {
        NS_LOG_WARN("LoRa2p4PhysicsEngine not initialized! Initializing with suburban defaults.");
        Initialize(LoRa2p4PhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false));
    }
    
    // Ensure gateway and node registries are updated
    if (m_nodeStates.count(txNodeId) == 0) {
        RegisterNode(txNodeId, txPosition);
    }
    if (m_gatewayStates.count(rxGatewayId) == 0) {
        RegisterGateway(rxGatewayId, rxPosition, false);
    }

    double dist_m = CalculateDistance(txPosition, rxPosition);
    if (dist_m < 1.0) dist_m = 1.0;
    
    uint64_t linkId = MakeLinkId(txNodeId, rxGatewayId);
    double velocity = m_nodeStates[txNodeId].velocityMps;
    
    // Stage 3: Spatial Consistency Maps
    SpatialFieldState spatialState = m_spatial->GetSpatialState(txPosition, m_config.environment, m_config.randomSeed);
    
    // Stage 3: Hidden Markov Channel States (Hysteresis)
    uint32_t activeMarkovState = 0;
    double dtState = 0.0;
    if (m_config.nlos.enabled) {
        if (m_hiddenChannelStates.count(linkId) == 0) {
            m_hiddenChannelStates[linkId] = 0; // default LOS
            m_lastStateTransition[linkId] = currentTime;
        }
        Time lastTrans = m_lastStateTransition[linkId];
        dtState = std::max(0.0, (currentTime - lastTrans).GetSeconds());
        double transitionProb = dtState * (velocity * 0.05 + 0.01);
        if (m_sampler.Sample(DistributionParam::Uniform(0.0, 1.0)) < transitionProb) {
            uint32_t newState = (uint32_t)m_sampler.Sample(DistributionParam::Uniform(0.0, 3.99));
            m_hiddenChannelStates[linkId] = newState;
            m_lastStateTransition[linkId] = currentTime;
        }
        activeMarkovState = m_hiddenChannelStates[linkId];
        result.hiddenChannelState = activeMarkovState;
    } else {
        activeMarkovState = 0;
        result.hiddenChannelState = 0;
    }

    WeatherState currentWeather = m_atmospheric ? m_atmospheric->GetCurrentState() : m_config.initialWeather;
    CorrelatedChannelState corrState = m_correlation->GetCorrelatedState(
        linkId, txPosition, rxPosition, m_config.environment, currentWeather, currentTime, m_sampler);
    result.correlatedObstructionFactor = corrState.obstructionFactor;
    
    // 1. Gateway Antenna Gain (3GPP directional sector model)
    Vector gwOrientation(1.0, 0.0, 0.0);    // 6. 3D Antenna Radiation Pattern Integration
    double antGain = m_antenna->ComputeAntennaGainDb(m_config.antenna, txPosition, rxPosition, gwOrientation);
    result.antennaGainDb = 0.0;
    
    // 2. TX power accuracy variation and antenna gain coupling
    double effectiveTxPower = m_hardware->ApplyTxPowerVariation(txPowerDbm, txNodeId) + antGain;
    result.hwTxVariationDb = (txPowerDbm + antGain) - effectiveTxPower;
    result.effectiveTxPowerDbm = effectiveTxPower;
    
    // 1. Non-Line-of-Sight (NLOS) blockage penalty (Moved up for DAG Coupling)
    double rawNlosDb = 0.0;
    if (m_config.nlos.enabled) {
        rawNlosDb = ComputeNLOSPenaltyDb(txPosition, rxPosition, m_config.nlos, m_sampler);
        if (m_config.fidelityMode == FIDELITY_SDR_GRADE_REALISM && m_config.environment != ENV_TUNNEL_CONFINED) {
            if (activeMarkovState == 1) rawNlosDb += 6.0;        // Partial obstruction
            else if (activeMarkovState == 2) rawNlosDb += 18.0;   // Deep NLOS
        }
    }
    result.nlosDb = std::max(0.0, rawNlosDb); // Remove 55.0 cap per Fix E3
    result.wasNLOS = (result.nlosDb > 0.01);
    
    // Update LinkPhysicsState
    state.isNLOS = result.wasNLOS;
    state.nlosPenaltyDb = result.nlosDb;

    // --- Tier 2 Submodules Execution ---
    // Execute NodePositionUncertainty (uses dist_m)
    m_nodePositionUncertainty->Compute(state, dist_m);
    
    // Execute Indoor Triad
    m_buildingEntry->Compute(state);
    m_wallMaterial->Compute(state);
    m_floorAttenuation->Compute(state);
    
    // Execute Blockage and Shadowing
    m_temporalBlockage->Compute(state);
    m_bodyShadowing->Compute(state);
    
    // --- Tier 3 Submodules Execution ---
    m_tunnel->Compute(state, dist_m);
    m_soilAttenuation->Compute(state);

    double scaleDist = m_sampler.Sample(m_config.nlos.nlosScaleDistanceM);
    result.nlosProbability = m_config.nlos.enabled ? (1.0 - std::exp(-dist_m / (scaleDist > 0.0 ? scaleDist : 1.0))) : 0.0;
    
    // 2. Large-scale Path Loss
    double txHeight = (txPosition.z > 0.0) ? txPosition.z : m_config.propagation.nodeHeightM.mean;
    double rxHeight = (rxPosition.z > 0.0) ? rxPosition.z : m_config.terrain.gatewayHeightOptionsM[0];
    
    m_twoRayGround->Compute(state, dist_m, txHeight, rxHeight, m_config.propagation.frequencyHz);
    if (state.isTwoRay) {
        result.pathLossDb = state.propagationLossDb;
    } else {
        result.pathLossDb = ComputePathLossDb(txPosition, rxPosition, txHeight, rxHeight, m_config.propagation, m_sampler);
        state.propagationLossDb = result.pathLossDb;
    }
    
    // 10. Procedural Terrain & geometric LOS ray intersection testing
    TerrainResult terrRes = m_terrain->ComputeTerrainPenaltyDb(
        txPosition, rxPosition, txHeight, rxHeight, m_config.terrain, m_config.randomSeed, m_sampler);
        
    result.terrainDb = std::clamp(terrRes.terrainPenaltyDb, 0.0, 30.0);
    result.fresnelClearanceRatio = terrRes.fresnelClearanceRatio;
    result.terrainLosBlocked = terrRes.terrainLosBlocked;
    state.terrainLosBlocked = terrRes.terrainLosBlocked;
    state.pendingTerrainDb = result.terrainDb;
    
    if (m_config.fidelityMode == FIDELITY_SDR_GRADE_REALISM) {
        result.dominantEdgeHeightM = 15.0 + terrRes.terrainPenaltyDb * 1.2;
        result.multiEdgeLossDb = 3.5 + terrRes.terrainPenaltyDb * 0.8;
        if (m_config.nlos.enabled && activeMarkovState == 3) {
            result.terrainReflectionCount = 2 + (uint32_t)m_sampler.Sample(DistributionParam::Uniform(0.0, 3.0));
        }
    }
    
    // 3. Large-scale Shadowing (Coupling 2: Variance increased by NLOS, Coupling 5: Terrain gates correlation)
    double rawShadowingDb = 0.0;
    if (m_config.shadowing.enabled) {
        rawShadowingDb = m_shadowing->GetShadowingDb(
            state,
            txNodeId,
            rxGatewayId,
            txPosition,
            rxPosition,
            currentTime);
    }
    double clamp_max = 3.0 * state.shadowingStdEff;
    double clamp_min = -2.0 * state.shadowingStdEff;
    if (!m_config.shadowing.enabled) {
        clamp_max = 3.0 * m_config.shadowing.shadowingStddevDb.mean;
        clamp_min = -2.0 * m_config.shadowing.shadowingStddevDb.mean;
    }
    result.shadowingDb = std::clamp(rawShadowingDb, clamp_min, clamp_max);
    
    // 4. Small-scale Multipath Fading (Coupling 1: K-factor defined by NLOS)
    FadingParams effFading = m_config.fading;
    if (effFading.model != FADING_NONE) {
        if (state.isNLOS) {
            double kNlosMean = m_config.fading.kNlosMean;
            double kSampled = std::max(0.0, m_sampler.Sample(DistributionParam::Normal(kNlosMean, 1.0, -10.0, 10.0)));
            effFading.model = FADING_RICIAN;
            effFading.ricianKFactor = DistributionParam::Fixed(kSampled);
        } else {
            double kSampled = std::max(0.0, m_sampler.Sample(m_config.fading.ricianKFactor));
            effFading.model = FADING_RICIAN;
            effFading.ricianKFactor = DistributionParam::Fixed(kSampled);
        }
        state.kFactor = effFading.ricianKFactor.mean;
    }
    FadingState& fs = m_fadingStates[linkId];
    double rawFadingDb = ComputeFadingDb(state, currentTime, effFading, m_fadingSampler, linkId, fs.lastUpdateTime, fs.lastFadingDb);
    result.sampledKFactor = state.kFactor;
    result.fadingDb = std::max(rawFadingDb, -20.0); // Remove hard upper cap per Fix E1
    
    // [CA-TBL Phase S0b] Physics-derived ranging error -- PART 1 OF 2.
    //
    // The TIER_2P4 sigma is now CRLB-derived from bandwidth and SNR, so the call
    // itself has moved to Step 15, after internalSnrDb is final. What must still
    // happen HERE is the zeroing of state.propagationLossBiasDb, and that is not
    // cosmetic bookkeeping:
    //
    //   Sub.21 (m_nodePositionUncertainty->Compute) runs unconditionally in the
    //   Tier-2 group, in BOTH ranging modes, and writes a nonzero
    //   state.propagationLossBiasDb. Step 14 adds that field into additional_loss.
    //   The old in-place TIER_2P4 branch zeroed it as a side effect before Step 14
    //   read it. Moving the call without preserving that zeroing would leak
    //   Sub.21's RSSI-inversion bias into the 2.4 GHz link budget and silently
    //   change every result on this tier.
    //
    // RANGING_SYNTHETIC_GPS_DENIED is untouched: Sub.21's output stays final, so
    // ml_dataset_generator.cc remains bit-for-bit unchanged.
    if (m_config.rangingModelMode == RANGING_PHYSICS_DERIVED) {
        state.propagationLossBiasDb = 0.0;
    }

    // 5. Tapped Delay Line Multipath (Coupling 3: Delay Spread scaled by NLOS)
    double effBw = (bandwidthHz > 0.0 ? bandwidthHz : 125000.0);
    DelaySpreadResult dsRes = m_delaySpread->ComputeDelaySpreadDynamic(
        state, m_config.environment, dist_m, m_config.propagation.frequencyHz, effBw, m_config.delaySpread, m_sampler);
        
    if (m_config.fidelityMode == FIDELITY_SDR_GRADE_REALISM && m_config.nlos.enabled && activeMarkovState == 3) {
        dsRes.rmsDelaySpreadNs *= 1.8; // Canyon multipath delay expansion
        state.delaySpreadNs = dsRes.rmsDelaySpreadNs;
    }
    result.delaySpreadNs = dsRes.rmsDelaySpreadNs;
    result.coherenceBandwidthHz = dsRes.coherenceBandwidthHz;
    double multipathPenaltyDb = dsRes.frequencySelectivePenaltyDb;
    
    // Mode 0 Bypass
    if (m_config.fidelityMode == FIDELITY_ULTRA_FAST_STATISTICAL) {
        multipathPenaltyDb = 0.0;
    }
    
    // 8. Atmospheric absorption and thermal noise floor shift
    double distanceKm = dist_m / 1000.0;
    result.atmosphericDb = m_atmospheric->ComputeAtmosphericLossDb(distanceKm, currentTime);
    double tempJitter = m_sampler.Sample(DistributionParam::Normal(0.0, 2.0, -5.0, 5.0));
    double effectiveTemp = m_atmospheric->GetCurrentTemperature() + tempJitter;
    result.thermalNoiseShiftDb = 10.0 * std::log10((effectiveTemp + 273.15) / 290.0);
    result.weatherAtTime = m_atmospheric->GetCurrentState();
    
    // 9. Foliage obstruction loss (modulated by correlated foliage density + spatial map)
    FoliageParams effFoliage = m_config.foliage;
    effFoliage.maxAttenuationDb.mean *= corrState.foliageDensity;
    if (m_config.fidelityMode == FIDELITY_SDR_GRADE_REALISM) {
        effFoliage.maxAttenuationDb.mean *= (0.5 + 0.5 * spatialState.foliageDensityField);
    }
    double rawFoliageDb = 0.0;
    result.foliageStaticDb = 0.0;
    result.foliageDynamicDb = 0.0;
    
    // Coupling 4: Rain saturates foliage (extract rain rate)
    double rainRateMmHr = m_atmospheric ? m_atmospheric->GetCurrentRainRate() : 0.0;
    
    if (m_config.windVariation.windVariationEnabled) {
        FoliageVariationResult foliageResult = ComputeDynamicFoliageLossDb(
            txPosition, rxPosition, effFoliage, m_config.windVariation, m_sampler, rainRateMmHr, false, currentTime);
        result.foliageStaticDb = foliageResult.staticLossDb;
        result.foliageDynamicDb = foliageResult.dynamicLossDb;
        rawFoliageDb = foliageResult.staticLossDb + foliageResult.dynamicLossDb;
    } else {
        rawFoliageDb = ComputeFoliageLossDb(txPosition, rxPosition, effFoliage, m_sampler, rainRateMmHr, false);
        result.foliageStaticDb = rawFoliageDb;
    }
    result.foliageDb = std::clamp(rawFoliageDb, 0.0, 35.0);
    if (m_config.fidelityMode == FIDELITY_ULTRA_FAST_STATISTICAL) {
        // Retain macro terrain loss even in statistical mode for range validity
        // result.terrainDb = 0.0; 
    }
    
    // 11. Doppler phase rotation & spectrum shaping under mobility
    double dopplerShiftHz = (velocity * m_config.propagation.frequencyHz / 299792458.0);
    result.dopplerPhaseErrorRad = dopplerShiftHz * 2.0 * M_PI * 0.015; // Phase drift over ~15ms preamble
    result.dopplerPenaltyDb = ComputeDopplerPenaltyDb(velocity, spreadingFactor, effBw, m_config.propagation.frequencyHz, m_config.mobility, m_sampler);
    result.dopplerPenaltyDb = 0.0;
    result.dopplerSpectrumWidthHz = 2.0 * dopplerShiftHz;
    
    // 12. Gateway Noise Floor
    double kTBdBm = -174.0;
    double noiseFigureDb = 6.0;
    result.noiseFloorDbm = kTBdBm + 10.0 * std::log10(effBw) + noiseFigureDb + result.thermalNoiseShiftDb;
    
    double preliminaryRxPowerDbm = effectiveTxPower - result.pathLossDb;
    
    // Stage 3: Dynamic Cluster Tracking
    if (m_config.fidelityMode >= FIDELITY_ADVANCED_TDL_COLLISIONS) {
        uint32_t baseClusters = (m_config.environment == ENV_URBAN_DENSE ? 8 : (m_config.environment == ENV_SUBURBAN ? 5 : 3));
        double lambdaTurnover = (velocity * 0.15 + 0.05);
        result.clusterTurnoverRate = lambdaTurnover;
        int clusterDelta = (int)m_sampler.Sample(DistributionParam::Normal(0.0, lambdaTurnover * dtState, -3.0, 3.0));
        result.activeClusterCount = std::clamp((int)baseClusters + clusterDelta, 1, 15);
        result.strongestClusterPowerDb = preliminaryRxPowerDbm - 1.5;
    }
    
    // 13. Active Packet Interference & Collision Overlap Tracking
    Time airtime = Time::FromDouble(std::pow(2, spreadingFactor) / effBw * 20.0, Time::S); // Nominal airtime approximation
    m_interference->RegisterActiveTransmission(
        txNodeId, rxGatewayId, currentTime, airtime, spreadingFactor, effBw, m_config.hardware.codingRateDenom, preliminaryRxPowerDbm, txPosition);
        
    double interferenceDb = m_interference->ComputeInterferencePenaltyDb(
        txNodeId, spreadingFactor, effBw, currentTime, airtime, txPosition, rxPosition, currentTime,
        m_config.interference, m_sampler, preliminaryRxPowerDbm, result.noiseFloorDbm);
        
    bool wasCollision = m_interference->WasCollision();
    bool wasCaptured = m_interference->WasCaptured();
        
    result.interferencePenaltyDb = interferenceDb;
    result.interferenceOverlapProbability = m_interference->GetOverlapProbability();
    result.collisionOverlapFraction = m_interference->GetCollisionOverlapFraction();
    result.interfererCount = m_interference->GetInterfererCount();
    result.strongestInterfererDbm = m_interference->GetStrongestInterfererPowerDbm();
    result.captureMarginDb = m_interference->GetCaptureMarginDb();
    
    // Stage 3: Symbol-Level Collision Model
    result.symbolCollisionFraction = result.collisionOverlapFraction;
    result.syncCorruptionOccurred = (result.collisionOverlapFraction > 0.08 && !wasCaptured);
    
    // 14. Total loss accumulation with bounded loss caps (including TDL multipath penalty)
    // Feature: Oliveira 2017 thermal sensitivity target (~0.075 dB/°C)
    double ambientTemp = m_config.atmospheric.temperatureCelsius.mean;
    double thermalEfficiencyLossDb = 0.0; // Handled by Johnson-Nyquist in atmospheric submodule
    
    double positiveShadowing = std::max(0.0, result.shadowingDb);
    double negativeShadowingGain = std::min(0.0, result.shadowingDb);
    
    // Sum Tier 2 losses
    double tier2LossDb = state.buildingEntryDb + state.wallPenetrationDb + state.floorAttenuationDb + 
                         state.temporalBlockageDb + state.bodyShadowingDb;
                         
    // Sum Tier 3 losses
    state.soilAttenuationDb = 0.0;
    double tier3LossDb = state.tunnelLossDb + state.soilAttenuationDb;
                         
    multipathPenaltyDb = 0.0;
    
    double additional_loss = positiveShadowing + result.fadingDb + result.nlosDb + 
                             result.foliageDb + result.terrainDb + result.dopplerPenaltyDb + 
                             result.interferencePenaltyDb + multipathPenaltyDb + thermalEfficiencyLossDb + tier2LossDb + tier3LossDb + result.atmosphericDb + state.propagationLossBiasDb;
    double totalLoss = result.pathLossDb + additional_loss + negativeShadowingGain;
    totalLoss += result.hwTxVariationDb; // Add TX hardware variance to loss
    totalLoss -= result.antennaGainDb;   // Antenna gain reduces total loss
    result.totalLossDb = totalLoss;
    result.rxPowerDbm = txPowerDbm - totalLoss;
    
    if (txNodeId == 10 && rxGatewayId == 5) {
        std::cout << "\n[ENGINE DEBUG] Node 9 GW 4 hidden losses:"
                  << "\n  dopplerPenaltyDb=" << result.dopplerPenaltyDb
                  << "\n  interferencePenaltyDb=" << result.interferencePenaltyDb
                  << "\n  multipathPenaltyDb=" << multipathPenaltyDb
                  << "\n  thermalEfficiencyLossDb=" << thermalEfficiencyLossDb
                  << "\n  tier2LossDb=" << tier2LossDb
                  << "\n    buildingEntryDb=" << state.buildingEntryDb
                  << "\n    wallPenetrationDb=" << state.wallPenetrationDb
                  << "\n    floorAttenuationDb=" << state.floorAttenuationDb
                  << "\n    temporalBlockageDb=" << state.temporalBlockageDb
                  << "\n    bodyShadowingDb=" << state.bodyShadowingDb
                  << "\n  tier3LossDb=" << tier3LossDb << std::endl;
    }
    
    // 15. Internal high-precision SNR
    double internalSnrDb = result.rxPowerDbm - result.noiseFloorDbm;
    
    // Stage 3: IQ Hardware Impairments
    IQResult iqRes = m_iqImpairment->ComputeIQImpairments(result.rxPowerDbm, internalSnrDb);
    result.evmPercent = iqRes.evmPercent;
    result.iqImageRejectionDb = iqRes.imageRejectionDb;
    result.adcClippingOccurred = iqRes.adcClippingOccurred;
    
    result.rxPowerDbm -= iqRes.snrDegradationDb; // Degrade rx power by EVM and clipping noise
    internalSnrDb = result.rxPowerDbm - result.noiseFloorDbm;

    // [CA-TBL Phase S0b] Physics-derived ranging error -- PART 2 OF 2.
    //
    // Called here rather than at Step 4.5 because sigma is now
    //   sigma = max(CRLB(effBw, internalSnrDb + 10*log10(2^SF)), hardware floor)
    // and internalSnrDb only becomes final on the line above, once the IQ
    // impairment degradation has been applied. Ranging that claims a precision its
    // bandwidth and SNR cannot support is what made tier 2 report a flat 0.24 m at
    // every distance and dominate every architectural comparison non-physically.
    //
    // Nothing between Step 4.5 and here reads state.rangeErrorM,
    // state.estimatedRangeM or state.positionErrorM -- they are consumed only by
    // the epilogue that copies them into result -- so moving the call is safe for
    // those three fields. state.propagationLossBiasDb is the exception and is
    // handled at Step 4.5; see PART 1.
    if (m_config.rangingModelMode == RANGING_PHYSICS_DERIVED) {
        ComputeRangingErrorM(state, RANGING_TIER_2P4, m_config.ranging,
                              m_config.propagation, m_config.propagation.frequencyHz,
                              effBw, internalSnrDb, spreadingFactor, m_sampler);
    }

    // 16. Calibrated receiver sensitivity threshold & Link Hysteresis (Issue #9)
    double rxSensitivity = GetSensitivityThreshold(spreadingFactor, m_config.hardware.codingRateDenom);
    double effectiveSensitivity = m_hardware->ApplyRxSensitivityVariation(rxSensitivity, rxGatewayId);
    
    if (m_linkHysteresisActive.count(linkId) > 0 && m_linkHysteresisActive[linkId]) {
        effectiveSensitivity -= m_config.receiver.linkHysteresisMarginDb;
    }
    
    result.sensitivityDbm = rxSensitivity;
    result.effectiveSensitivityDbm = effectiveSensitivity;
    result.sensitivityMarginDb = result.rxPowerDbm - effectiveSensitivity;
    
    // 17. Core physical layer collision & capture decision logic
    result.collisionOccurred = wasCollision;
    result.captureSucceeded = wasCaptured;
    result.captureEventOccurred = (wasCollision && wasCaptured);
    
    // 18. Telemetry reported SNR unclamped (actual computed value)
    // Reference: Physical layer SNR must reflect actual received power vs noise floor
    result.rawSNRDb = internalSnrDb;  // Unclamped raw SNR value
    result.finalSNRDb = internalSnrDb;  // Removed artificial -30 dB bound per validation requirements
    result.snr_db = internalSnrDb;
    result.raw_snr_db = internalSnrDb;
    
    // Enforce physical causality: PLL cannot lock before preamble lock is established
    // Reference: LoRa modulation sequence — preamble correlation is the trigger for PLL acquisition
    if (!result.preambleLocked) {
        result.pllLocked = false;
        result.payloadCrcSuccess = false;
        result.syncCorruptionOccurred = true;
    }
    
    // Enforce CRC dependency: both preamble and PLL must be locked for successful decode
    if (!result.pllLocked) {
        result.payloadCrcSuccess = false;
    }
    
    // 19. Generate realistic receiver timing and RSSI observation
    Time trueArrival = currentTime + Seconds(dist_m / 299792458.0);
    result.timestamp = m_hardware->GenerateTimestampObservation(
        txNodeId, rxGatewayId, trueArrival, result.rxPowerDbm, result.finalSNRDb, spreadingFactor, relayHops);
    result.timestamp.captureOccurred = result.captureEventOccurred;
    result.trueRssiDbm = result.timestamp.trueRssiDbm;
    result.measuredRssiDbm = result.timestamp.measuredRssiDbm;
    
    // Stage 3: True CSS PHY Dechirp & Synchronization Model
    double timingErr = m_sampler.Sample(DistributionParam::Normal(0.0, 400.0, -4000.0, 4000.0));
    
    double phyCfoHz = result.timestamp.cfoHz;
    double phyTimingErr = timingErr;
    if (m_config.fidelityMode == FIDELITY_ULTRA_FAST_STATISTICAL) {
        phyCfoHz = 0.0;
        phyTimingErr = 0.0;
    }
    
    CSSPHYResult cssRes = m_cssPhy->ComputeDemodulation(
        internalSnrDb, phyCfoHz, phyTimingErr, result.delaySpreadNs, result.collisionOverlapFraction,
        spreadingFactor, effBw, result.syncCorruptionOccurred, m_sampler);
        
    result.demodSuccessProbability = cssRes.demodSuccessProbability;
    result.fftBinOffset = cssRes.fftBinOffset;
    result.timingSyncErrorNs = cssRes.timingSyncErrorNs;
    result.preambleLocked = cssRes.preambleLocked;
    result.payloadCrcSuccess = cssRes.payloadDemodulated;
    result.syncCorruptionOccurred = cssRes.syncCorruptionOccurred;
    
    SyncLoopResult syncRes = m_syncLoop->ComputeSynchronizationLoop(
        spreadingFactor, effBw, internalSnrDb, result.timestamp.cfoHz, timingErr, 8);
    result.residualCfoHz = syncRes.residualCfoHz;
    result.residualTimingErrorNs = syncRes.residualTimingErrorNs;
    result.agcConvergence = syncRes.agcConvergenceFactor;
    result.syncConfidence = syncRes.syncConfidence;
    
    uint32_t payloadBytes = 20; // default test payload
    BasebandResult bbRes = m_baseband->ComputeBasebandEvolution(
        spreadingFactor, effBw, internalSnrDb, syncRes.residualCfoHz, syncRes.residualTimingErrorNs,
        result.dopplerSpectrumWidthHz, result.delaySpreadNs, payloadBytes, linkId, currentTime);
    result.symbolErrorProbability = bbRes.symbolErrorProbability;
    result.packetCrcProbability = bbRes.packetCrcProbability;
    result.softSymbolMarginDb = bbRes.softSymbolMarginDb;
    result.phaseCoherence = bbRes.dechirpCoherenceScore;
    
    SemanticRegionInfo regInfo = m_region->GetRegionInfo(txPosition);
    result.environmentRegion = regInfo.regionName;
    result.clusterSpatialStability = (regInfo.losPersistenceProbability * 0.9 + 0.1);
    
    bool syncFailed = (syncRes.syncConfidence < 0.2);
    m_crossLayer->RecordPhyObservation(linkId, internalSnrDb, wasCollision, result.delaySpreadNs, result.fadingDb, syncFailed);
    
    // --- STAGE 5 FINAL ADVANCED EMULATION FRAMEWORK PIPELINE ---
    
    double nominalPacketAirtime = std::pow(2.0, spreadingFactor) / effBw * 20.0;
    double effectiveOverlapFraction = wasCaptured ? 0.0 : result.collisionOverlapFraction;
    IQStreamResult iqResStream = m_iqStream->EvolveIQStream(
        internalSnrDb, result.timestamp.cfoHz, dopplerShiftHz, spreadingFactor, effBw, nominalPacketAirtime, effectiveOverlapFraction);
    result.iqPhaseContinuity = iqResStream.iqPhaseContinuity;
    result.coherentEnergyRatio = iqResStream.coherentEnergyRatio;
    result.instantaneousFrequencyErrorHz = iqResStream.instantaneousFrequencyErrorHz;
    
    bool effectiveCollision = wasCollision && !wasCaptured;
    DspChainResult dspRes = m_dspChain->ProcessDspPipeline(
        iqResStream, result.rxPowerDbm, internalSnrDb, spreadingFactor, effBw, effectiveCollision);
    result.fftPeakConfidence = dspRes.fftPeakConfidence;
    result.symbolLikelihood = dspRes.symbolLikelihood;
    result.crcConfidence = dspRes.crcConfidence;
    
    double preambleDur = std::pow(2.0, spreadingFactor) / effBw * 8.0;
    PllResult pllRes = m_pll->ProcessPllLoop(result.timestamp.cfoHz, result.fadingDb, preambleDur);
    result.pllLocked = pllRes.pllLocked;
    if (!result.preambleLocked) {
        result.pllLocked = false;
        result.payloadCrcSuccess = false;
    }
    result.pllPhaseErrorRad = pllRes.pllPhaseErrorRad;
    result.cycleSlipOccurred = pllRes.cycleSlipOccurred;
    
    SymbolSyncResult symSyncRes = m_symbolSync->ProcessTimingRecovery(timingErr, internalSnrDb, spreadingFactor, effBw);
    result.timingLockQuality = symSyncRes.timingLockQuality;
    result.timingJitterNs = symSyncRes.timingJitterNs;
    
    RFNonlinearityResult rfRes = m_rfNonlinearity->ProcessNonlinearities(txPowerDbm, result.rxPowerDbm, result.strongestInterfererDbm);
    if (m_config.hardware.enabled) {
        result.phaseNoiseVariance = rfRes.phaseNoiseVariance;
    }
    
    ScalabilityEvaluationResult scalRes = m_scalability->EvaluateScalabilityFidelity(linkId, txPosition, rxPosition, static_cast<uint32_t>(m_stats.totalTransmissions), m_config.fidelityMode);
    result.fidelitySchedulerState = scalRes.activeFidelityLevel;
    result.batchProcessingGain = scalRes.batchProcessingGain;
    
    m_geographicMemory->RegisterObservation(txPosition, m_config.nlos.enabled ? activeMarkovState : 0, result.pathLossDb, result.shadowingDb, result.terrainDb, dopplerShiftHz);
    GeographicMemoryResult geoRes = m_geographicMemory->QueryGeographicMemory(txPosition);
    result.rfMemoryRegionId = geoRes.rfMemoryRegionId;
    
    GatewayReceptionCandidate gwCand;
    gwCand.gatewayId = rxGatewayId;
    gwCand.snrDb = internalSnrDb;
    gwCand.rxPowerDbm = result.rxPowerDbm;
    gwCand.preambleLocked = result.preambleLocked;
    gwCand.payloadCrcPassed = result.payloadCrcSuccess;
    gwCand.timestamp = result.timestamp;
    
    uint64_t timeHash = (uint64_t)(currentTime.GetMicroSeconds() / 1000);
    uint64_t packetUid = (timeHash << 16) | txNodeId;
    m_cooperativePhy->RegisterGatewayCandidate(packetUid, gwCand);
    CooperativePhyEvaluationResult coopRes = m_cooperativePhy->EvaluateCooperativeReception(packetUid, spreadingFactor);
    result.combinedSnrDb = coopRes.combinedSnrDb;
    result.gatewayDiversityOrder = coopRes.gatewayDiversityOrder;
    result.cooperativeDecodeSuccess = coopRes.cooperativeDecodeSuccess;
    
    m_validationSuite->RegisterSimulatedObservation(result.timestamp.cfoHz, result.residualTimingErrorNs, result.evmPercent, result.delaySpreadNs, dopplerShiftHz, result.symbolLikelihood);
    
    // --- STAGE 6 TRUE SDR DIGITAL TWIN PIPELINE ---
    
    // 1. Radio Frontend
    ambientTemp = m_atmospheric ? m_atmospheric->GetCurrentTemperature() : m_config.atmospheric.temperatureCelsius.mean;
    RadioFrontendResult rfRes6 = m_radioFrontend->ProcessAnalogFrontend(
        result.timestamp.cfoHz, ambientTemp + m_sampler.Sample(DistributionParam::Normal(0.0, 5.0, -10.0, 10.0)),
        currentTime, result.rxPowerDbm, result.strongestInterfererDbm);
        
    if (m_config.hardware.enabled) {
        result.oscillatorDriftHz = rfRes6.oscillatorDriftHz;
        result.thermalDriftHz = rfRes6.thermalDriftHz;
        result.phaseNoisePsd = rfRes6.phaseNoisePsd;
        result.spurResponseDb = rfRes6.spurResponseDb;
        result.agcRecoveryTimeMs = rfRes6.agcRecoveryTimeMs;
        result.frontendDesensitizationDb = rfRes6.frontendDesensitizationDb;
    
        // Apply desensitization to internal SNR
        internalSnrDb -= rfRes6.frontendDesensitizationDb;
    }
    
    // 2. Spectrum Occupancy
    SpectrumOccupancyResult specRes = m_spectrumOccupancy->EvaluateSpectrumEnvironment(currentTime, effBw);
    result.spectrumOccupancyDbm = specRes.spectrumOccupancyDbm;
    result.impulsiveNoiseEvents = specRes.impulsiveNoiseEvents;
    result.coloredNoiseVariance = specRes.coloredNoiseVariance;
    
    // 3. MicroDSP Activation
    MicroDspResult microRes = m_microDsp->EvaluateMicroDspWindow(
        internalSnrDb, effectiveOverlapFraction, result.pllLocked, 
        std::abs(result.residualCfoHz), !result.captureSucceeded && wasCollision, result.fftPeakConfidence);
        
    result.microDspActive = microRes.microDspActive;
    result.microDspFftConfidence = microRes.microDspFftConfidence;
    result.microDspSymbolAgreement = microRes.microDspSymbolAgreement;
    result.microDspRuntimeUs = microRes.microDspRuntimeUs;
    
    // 4. Distributed Cooperative PHY
    DistributedPhyGatewayState distState;
    distState.gatewayId = rxGatewayId;
    distState.symbolLikelihood = result.symbolLikelihood;
    distState.timingJitterNs = result.timingJitterNs;
    distState.residualCfoHz = result.residualCfoHz;
    distState.hasCollision = effectiveCollision;
    distState.rawSnrDb = internalSnrDb;
    
    m_distributedPhy->RegisterGatewayObservation(packetUid, distState);
    DistributedPhyResult distRes = m_distributedPhy->EvaluateDistributedFusion(packetUid);
    
    result.distributedDecodeGainDb = distRes.distributedDecodeGainDb;
    result.sharedPllConfidence = distRes.sharedPllConfidence;
    result.cooperativeSymbolConfidence = distRes.cooperativeSymbolConfidence;
    result.distributedSicTriggered = distRes.distributedSicTriggered;
    
    // 5. Distributed Scalability Execution
    DistributedScalabilityResult dscalRes = m_distributedScalability->EvaluatePartitionAndBatch(
        txPosition, result.interfererCount + 1, m_config.distributedScalability.gpuBatchingEnabled);
    result.regionSchedulerState = static_cast<uint32_t>(dscalRes.regionSchedulerState);
    result.gpuBatchSize = dscalRes.gpuBatchSize;
    
    // 6. Global Validation Pipeline
    m_globalValidation->RegisterGlobalObservation(internalSnrDb, result.delaySpreadNs, result.distributedDecodeGainDb);
    
    // --- END STAGE 6 PIPELINE ---

    if (m_config.fidelityMode == FIDELITY_SDR_GRADE_REALISM) {
        // Stage 6 enhances decode capability via Distributed SIC and MicroDSP, while penalizing with front-end desens
        bool effectiveCrcPassed = result.payloadCrcSuccess || (result.distributedDecodeGainDb > 3.0);
        bool effectiveCollisionPenalty = (wasCollision && !wasCaptured && !result.distributedSicTriggered);
        
        // Instead of binary blockers, Stage 3-4 impairments are converted into a soft sensitivity degradation penalty
        double softDegradationPenaltyDb = 0.0;
        if (!result.preambleLocked) {
            softDegradationPenaltyDb += m_config.receiver.preambleUnlockPenaltyDb;
        }
        if (!result.pllLocked) {
            softDegradationPenaltyDb += m_config.receiver.pllUnlockPenaltyDb;
        }
        if (result.evmPercent > m_config.receiver.evmPenaltyThresholdPercent) {
            softDegradationPenaltyDb +=
                (result.evmPercent - m_config.receiver.evmPenaltyThresholdPercent) *
                m_config.receiver.evmPenaltyDbPerPercent;
        }
        if (std::abs(result.residualCfoHz) > m_config.receiver.cfoPenaltyThresholdHz) {
            softDegradationPenaltyDb +=
                (std::abs(result.residualCfoHz) - m_config.receiver.cfoPenaltyThresholdHz) *
                m_config.receiver.cfoPenaltyDbPerHz;
        }
        if (!effectiveCrcPassed) {
            double confidenceFactor = std::max(result.crcConfidence, result.microDspFftConfidence);
            softDegradationPenaltyDb +=
                m_config.receiver.crcFailurePenaltyDb *
                (1.0 - std::clamp(confidenceFactor, 0.0, 1.0));
        }
        
        double finalEffectiveSensitivity = effectiveSensitivity + softDegradationPenaltyDb;
        result.effectiveSensitivityDbm = finalEffectiveSensitivity;
        result.sensitivityMarginDb = result.rxPowerDbm - finalEffectiveSensitivity;

        if (PassesReceiverSensitivity(result.rxPowerDbm, finalEffectiveSensitivity) && !effectiveCollisionPenalty) {
            result.packetReceived = true;
            m_linkHysteresisActive[linkId] = true;
        } else {
            result.packetReceived = false;
            m_linkHysteresisActive[linkId] = false;
        }
    } else {
        if (PassesReceiverSensitivity(result.rxPowerDbm, effectiveSensitivity)) {
            if (!wasCollision || wasCaptured) {
                result.packetReceived = true;
                m_linkHysteresisActive[linkId] = true;
            } else {
                result.packetReceived = false;
                m_linkHysteresisActive[linkId] = false;
            }
        } else {
            result.packetReceived = false;
            m_linkHysteresisActive[linkId] = false;
        }
    }
    
    // 7. Adaptive Environment Learning (runs after decode attempt)
    AdaptiveEnvironmentResult adaptRes = m_adaptiveEnvironment->EvaluateAndAdapt(
        result.packetReceived, internalSnrDb, (effectiveTxPower - result.pathLossDb) - result.noiseFloorDbm);
    result.environmentLearningConfidence = adaptRes.environmentLearningConfidence;
    result.updatedShadowingVariance = adaptRes.updatedShadowingVariance;
    result.updatedLosProbability = adaptRes.updatedLosProbability;
    
    // Feature 9: Adaptive Data Rate (ADR) awareness - recommend SF based on SNR margin
    // Reference: LoRaWAN specification; scitepress 111026
    // ADR selects SF to maintain SNR margin above threshold for robust reception
    result.adrAssignedSF = spreadingFactor;  // Default: use current SF
    result.snrMarginDb = result.sensitivityMarginDb;
    
    if (m_config.adr.adrEnabled) {
        // Feature 9: Compute SNR margin (difference between received SNR and sensitivity threshold)
        double snrMarginDb = result.finalSNRDb - effectiveSensitivity;
        result.snrMarginDb = snrMarginDb;
        
        // ADR goal: maintain SNR margin within margin threshold (typically 10 dB for reliability)
        // If margin too high: can reduce SF to increase capacity
        // If margin too low: should increase SF or reduce power to maintain link
        
        uint8_t recommendedSF = spreadingFactor;
        
        if (snrMarginDb > m_config.adr.adrSnrMarginThresholdDb + 5.0) {
            // Margin very high: try lower SF (more capacity)
            if (spreadingFactor > 7) {
                recommendedSF = spreadingFactor - 1;
            }
        } else if (snrMarginDb < m_config.adr.adrSnrMarginThresholdDb - 5.0) {
            // Margin low: try higher SF (better range)
            if (spreadingFactor < 12) {
                recommendedSF = spreadingFactor + 1;
            }
        }
        // else: margin is within acceptable range, keep current SF
        
        result.adrAssignedSF = recommendedSF;
    }
    
    if (result.packetReceived) {
        m_interference->NotifySuccessfulDecode(txNodeId, rxGatewayId, result.rxPowerDbm);
    }
    result.sicRecovered = m_interference->GetSicRecovered();
    result.fidelityLevel = m_config.fidelityMode;
    
    // Auto-scaling fidelity optimization (Issue #5)
    if (m_stats.totalTransmissions > 1000 && m_stats.GetPDR() > 0.98 && m_stats.interferenceDrops < 5) {
        if (m_config.fidelityMode == FIDELITY_SDR_GRADE_REALISM) {
            m_config.fidelityMode = FIDELITY_ADVANCED_TDL_COLLISIONS;
        }
    }
    
    // 18. Update stats tracking
    m_stats.totalTransmissions++;
    if (result.packetReceived) {
        m_stats.successfulReceptions++;
    } else {
        if (wasCollision && !wasCaptured) {
            m_stats.interferenceDrops++;
        } else if (result.fadingDb > 0.1) {
            double powerNoFading = result.rxPowerDbm + result.fadingDb;
            if (powerNoFading >= effectiveSensitivity) {
                m_stats.fadingDrops++;
            }
        }
    }
    
    if (result.wasNLOS) m_stats.nlosEvents++;
    if (result.captureEventOccurred) m_stats.captureEvents++;
    if (result.cycleSlipOccurred) m_stats.pllCycleSlips++;
    if (result.adcClippingOccurred) m_stats.dspAgcClippingEvents++;
    if (result.cooperativeDecodeSuccess) m_stats.cooperativeSuccesses++;
    
    m_stats.snrAccumulator += result.finalSNRDb;
    m_stats.shadowingAccumulator += result.shadowingDb;
    m_stats.fadingAccumulator += result.fadingDb;
    m_stats.iqPhaseContinuityAccumulator += result.iqPhaseContinuity;
    m_stats.coherentEnergyAccumulator += result.coherentEnergyRatio;
    m_stats.fftConfidenceAccumulator += result.fftPeakConfidence;
    m_stats.crcConfidenceAccumulator += result.crcConfidence;
    
    result.fadingModelUsed = m_config.fading.model;
    
    // Fix #9: Verbose per-packet link budget debug mode
    if (m_config.verboseLogging) {
        std::cout << "\n================================================================\n"
                  << "[LoRa Link Budget Debug] Packet " << txNodeId << " -> Gateway " << rxGatewayId << "\n"
                  << "----------------------------------------------------------------\n"
                  << std::fixed << std::setprecision(2)
                  << "Effective TX Power:      " << std::setw(8) << effectiveTxPower << " dBm\n"
                  << "Distance Path Loss:      " << std::setw(8) << result.pathLossDb << " dB  (Distance: " << dist_m << " m)\n"
                  << "Shadowing Loss/Gain:     " << std::setw(8) << result.shadowingDb << " dB\n"
                  << "Multipath Fading Loss:   " << std::setw(8) << result.fadingDb << " dB\n"
                  << "NLOS Obstruction Loss:   " << std::setw(8) << result.nlosDb << " dB\n"
                  << "Foliage Attenuation:     " << std::setw(8) << result.foliageDb << " dB\n"
                  << "Terrain Diffraction:     " << std::setw(8) << result.terrainDb << " dB\n"
                  << "Atmospheric Absorption:  " << std::setw(8) << result.atmosphericDb << " dB\n"
                  << "Interference Penalty:    " << std::setw(8) << result.interferencePenaltyDb << " dB  (Overlap Prob: " << result.interferenceOverlapProbability << ")\n"
                  << "Doppler Mobility Loss:   " << std::setw(8) << result.dopplerPenaltyDb << " dB\n"
                  << "----------------------------------------------------------------\n"
                  << "Total Accumulated Loss:  " << std::setw(8) << totalLoss << " dB  (Capped non-dist loss: " << additional_loss << " dB)\n"
                  << "Received Power (RX):     " << std::setw(8) << result.rxPowerDbm << " dBm\n"
                  << "Thermal Noise Floor:     " << std::setw(8) << result.noiseFloorDbm << " dBm\n"
                  << "Reported SNR:            " << std::setw(8) << result.finalSNRDb << " dB  (Internal SNR: " << internalSnrDb << " dB)\n"
                  << "Sensitivity Threshold:   " << std::setw(8) << effectiveSensitivity << " dBm (Base: " << rxSensitivity << " dBm)\n"
                  << "Sensitivity Margin:      " << std::setw(8) << result.sensitivityMarginDb << " dB\n"
                  << "Collision / Capture:     " << (wasCollision ? (wasCaptured ? "COLLISION (CAPTURED)" : "COLLISION (LOST)") : "NO COLLISION") << "\n"
                  << "Packet Decision:         " << (result.packetReceived ? "RECEIVED SUCCESSFULLY" : "DROPPED") << "\n"
                  << "================================================================" << std::endl;
    }
    
    // Transfer LinkPhysicsState internal variables to PhysicsResult2p4 for telemetry
    result.isTwoRay = state.isTwoRay;
    // P4: IsTunnel = true only for genuine tunnel environments, not concrete bunker
    result.isTunnel = (m_config.environment == ENV_TUNNEL_CONFINED);
    result.soilAttenuationDb = state.soilAttenuationDb;
    result.estimatedRangeM = state.estimatedRangeM;
    result.rangeErrorM = state.rangeErrorM;
    result.positionErrorM = state.positionErrorM;
    result.propagationLossBiasDb = state.propagationLossBiasDb;
    result.rangingSigmaM = state.rangingSigmaM;
    
    // P3: Write actual sampled rain rate, not the distribution mean
    result.sampledRainRateMmHr = m_atmospheric ? m_atmospheric->GetCurrentRainRate() : 0.0;
    
    result.isNLOS = state.isNLOS;
    result.kFactor = state.kFactor;
    result.shadowingStdEff = state.shadowingStdEff;
    
    result.buildingEntryDb = state.buildingEntryDb;
    result.wallPenetrationDb = state.wallPenetrationDb;
    result.floorAttenuationDb = state.floorAttenuationDb;
    result.bodyShadowingDb = state.bodyShadowingDb;
    result.temporalBlockageDb = state.temporalBlockageDb;

    return result;
}

void EngineStatistics2p4::PrintSummary() const {
    double n = (totalTransmissions > 0) ? (double)totalTransmissions : 1.0;
    std::cout << "\n============================================\n"
              << "       LORA PHYSICS ENGINE SUMMARY STATISTICS\n"
              << "============================================\n"
              << "Total Transmissions:  " << totalTransmissions << "\n"
              << "Successful Receptions: " << successfulReceptions << "\n"
              << "Packet Delivery Ratio: " << GetPDR() * 100.0 << " %\n"
              << "NLOS Obstruction Rate: " << GetNLOSRate() * 100.0 << " %\n"
              << "Capture Effect Events: " << captureEvents << "\n"
              << "Fading Alone Drops:   " << fadingDrops << "\n"
              << "Interference Drops:   " << interferenceDrops << "\n"
              << "PLL Cycle Slip Events: " << pllCycleSlips << "\n"
              << "AGC Clipping Events:  " << dspAgcClippingEvents << "\n"
              << "Cooperative Successes: " << cooperativeSuccesses << "\n"
              << "Mean Received SNR:    " << GetMeanSNR() << " dB\n"
              << "Mean Phase Continuity: " << (iqPhaseContinuityAccumulator / n) << "\n"
              << "Mean Coherent Energy:  " << (coherentEnergyAccumulator / n) << "\n"
              << "Mean FFT Peak Conf:    " << (fftConfidenceAccumulator / n) << "\n"
              << "Mean CRC Confidence:   " << (crcConfidenceAccumulator / n) << "\n"
              << "============================================\n" << std::endl;
}

void EngineStatistics2p4::WriteCSV(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not write statistics CSV to " << path << std::endl;
        return;
    }
    double n = (totalTransmissions > 0) ? (double)totalTransmissions : 1.0;
    file << "metric,value\n"
         << "total_transmissions," << totalTransmissions << "\n"
         << "successful_receptions," << successfulReceptions << "\n"
         << "pdr," << GetPDR() << "\n"
         << "nlos_rate," << GetNLOSRate() << "\n"
         << "capture_events," << captureEvents << "\n"
         << "fading_drops," << fadingDrops << "\n"
         << "interference_drops," << interferenceDrops << "\n"
         << "pll_cycle_slips," << pllCycleSlips << "\n"
         << "agc_clipping_events," << dspAgcClippingEvents << "\n"
         << "cooperative_successes," << cooperativeSuccesses << "\n"
         << "mean_snr," << GetMeanSNR() << "\n"
         << "mean_phase_continuity," << (iqPhaseContinuityAccumulator / n) << "\n"
         << "mean_coherent_energy," << (coherentEnergyAccumulator / n) << "\n"
         << "mean_fft_confidence," << (fftConfidenceAccumulator / n) << "\n"
         << "mean_crc_confidence," << (crcConfidenceAccumulator / n) << "\n";
    file.close();
}

void LoRa2p4PhysicsEngine::ResetGatewayJitter() {
    if (m_hardware) {
        m_hardware->ResetGatewayJitter(m_config.hardware);
    }
}

} // namespace ns3
