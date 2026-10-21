#ifndef LORA_UWB_PHYSICS_ENGINE_H
#define LORA_UWB_PHYSICS_ENGINE_H

#include "ns3/object.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"
#include "lora-uwb-physics-parameters.h"
#include "lora-distribution-sampler.h"
#include "lora-correlation-engine.h"
#include "submodules/lora-propagation-submodule.h"
#include "submodules/lora-shadowing-submodule.h"
#include "submodules/lora-fading-submodule.h"
#include "submodules/lora-nlos-submodule.h"
#include "submodules/lora-ranging-submodule.h"   // CA-TBL Phase S0b
#include "submodules/lora-atmospheric-submodule.h"
#include "submodules/lora-foliage-submodule.h"
#include "submodules/lora-interference-submodule.h"
#include "submodules/lora-hardware-submodule.h"
#include "submodules/lora-mobility-submodule.h"
#include "submodules/lora-terrain-submodule.h"
#include "submodules/lora-delay-spread-submodule.h"
#include "submodules/lora-antenna-submodule.h"
#include "submodules/lora-css-phy-submodule.h"
#include "submodules/lora-iq-impairment-submodule.h"
#include "submodules/lora-baseband-submodule.h"
#include "submodules/lora-sync-loop-submodule.h"
#include "submodules/lora-region-submodule.h"
#include "submodules/lora-iq-stream-submodule.h"
#include "submodules/lora-dsp-chain-submodule.h"
#include "submodules/lora-pll-submodule.h"
#include "submodules/lora-symbol-sync-submodule.h"
#include "submodules/lora-rf-nonlinearity-submodule.h"
#include "ns3/lora-building-entry-submodule.h"
#include "ns3/lora-wall-material-submodule.h"
#include "ns3/lora-floor-attenuation-submodule.h"
#include "ns3/lora-node-position-uncertainty-submodule.h"
#include "ns3/lora-temporal-blockage-submodule.h"
#include "ns3/lora-body-shadowing-submodule.h"
#include "ns3/lora-tunnel-submodule.h"
#include "ns3/lora-two-ray-ground-submodule.h"
#include "ns3/lora-soil-attenuation-submodule.h"
#include "lora-scalability-engine.h"
#include "lora-geographic-memory-engine.h"
#include "../helper/lora-crosslayer-engine.h"
#include "../helper/lora-sdr-validator.h"
#include "../helper/lora-validation-suite.h"
#include "../helper/lora-cooperative-phy-engine.h"
#include "submodules/lora-microdsp-submodule.h"
#include "submodules/lora-radio-frontend-submodule.h"
#include "submodules/lora-spectrum-occupancy-submodule.h"
#include "../helper/lora-distributed-phy-engine.h"
#include "../helper/lora-global-validation-engine.h"
#include "lora-distributed-scalability-engine.h"
#include "lora-adaptive-environment-engine.h"
#include "lora-spatial-field-engine.h"
#include <map>
#include <memory>
#include <string>

namespace ns3 {

/**
 * @brief Accumulated statistics tracking for LoRa Physics Engine
 */
struct EngineStatisticsUWB {
    uint64_t totalTransmissions;
    uint64_t successfulReceptions;
    uint64_t nlosEvents;
    uint64_t captureEvents;
    uint64_t fadingDrops;       // Lost to multipath fading alone
    uint64_t interferenceDrops; // Lost to co-channel/adjacent interference alone
    uint64_t pllCycleSlips;
    uint64_t dspAgcClippingEvents;
    uint64_t cooperativeSuccesses;
    
    double snrAccumulator;
    double shadowingAccumulator;
    double fadingAccumulator;
    double iqPhaseContinuityAccumulator;
    double coherentEnergyAccumulator;
    double fftConfidenceAccumulator;
    double crcConfidenceAccumulator;
    
    EngineStatisticsUWB()
        : totalTransmissions(0),
          successfulReceptions(0),
          nlosEvents(0),
          captureEvents(0),
          fadingDrops(0),
          interferenceDrops(0),
          pllCycleSlips(0),
          dspAgcClippingEvents(0),
          cooperativeSuccesses(0),
          snrAccumulator(0.0),
          shadowingAccumulator(0.0),
          fadingAccumulator(0.0),
          iqPhaseContinuityAccumulator(0.0),
          coherentEnergyAccumulator(0.0),
          fftConfidenceAccumulator(0.0),
          crcConfidenceAccumulator(0.0) {}

    double GetPDR() const { 
        if (totalTransmissions == 0) return 0.0;
        return (double)successfulReceptions / totalTransmissions; 
    }
    double GetMeanSNR() const { 
        if (totalTransmissions == 0) return 0.0;
        return snrAccumulator / totalTransmissions; 
    }
    double GetNLOSRate() const { 
        if (totalTransmissions == 0) return 0.0;
        return (double)nlosEvents / totalTransmissions; 
    }
    
    void PrintSummary() const;
    void WriteCSV(const std::string& path) const;
};

/**
 * @brief Detailed received packet physics breakdown for telemetry logging
 */
struct PhysicsResultUWB {
    bool packetReceived;
    bool captureEventOccurred;
    
    // Path loss term breakdown (all in dB, positive = loss)
    double pathLossDb;
    double shadowingDb;
    double fadingDb;
    double nlosDb;
    double atmosphericDb;
    double foliageDb;
    double interferencePenaltyDb;
    double thermalNoiseShiftDb;
    double dopplerPenaltyDb;
    double terrainDb;
    double hwTxVariationDb;
    double finalSNRDb;
    double noiseFloorDbm;
    double rxPowerDbm;
    double totalLossDb;
    double sampledKFactor;
    
    // Telemetry observation struct
    TimestampObservation timestamp;
    
    // Context
    bool wasNLOS;
    double nlosProbability;
    WeatherState weatherAtTime;
    FadingModelType fadingModelUsed;
    
    // Explicit reception and link budget audit fields
    bool collisionOccurred;
    bool captureSucceeded;
    double sensitivityDbm;
    double effectiveSensitivityDbm;
    double sensitivityMarginDb;
    double measuredRssiDbm;
    double trueRssiDbm;
    double snr_db;
    double raw_snr_db;

    // Stage 6 Telemetry
    bool microDspActive;
    double microDspFftConfidence;
    double microDspSymbolAgreement;
    double microDspRuntimeUs;
    
    double oscillatorDriftHz;
    double thermalDriftHz;
    double phaseNoisePsd;
    double spurResponseDb;
    double agcRecoveryTimeMs;
    double frontendDesensitizationDb;
    
    double distributedDecodeGainDb;
    double sharedPllConfidence;
    double cooperativeSymbolConfidence;
    bool distributedSicTriggered;
    
    uint32_t regionSchedulerState;
    uint32_t gpuBatchSize;
    
    double spectrumOccupancyDbm;
    uint32_t impulsiveNoiseEvents;
    double coloredNoiseVariance;
    
    double environmentLearningConfidence;
    double updatedShadowingVariance;
    double updatedLosProbability;
    double effectiveTxPowerDbm;
    double interferenceOverlapProbability;
    
    // Advanced second-stage telemetry fields
    double delaySpreadNs;
    double coherenceBandwidthHz;
    double fresnelClearanceRatio;
    bool terrainLosBlocked;
    double collisionOverlapFraction;
    double captureMarginDb;
    double antennaGainDb;
    double dopplerPhaseErrorRad;
    uint32_t interfererCount;
    double strongestInterfererDbm;
    double correlatedObstructionFactor;
    
    // SDR-grade advanced PHY telemetry (Stage 3)
    double demodSuccessProbability;
    double fftBinOffset;
    double timingSyncErrorNs;
    double evmPercent;
    double iqImageRejectionDb;
    bool adcClippingOccurred;
    uint32_t activeClusterCount;
    double strongestClusterPowerDb;
    double clusterTurnoverRate;
    double symbolCollisionFraction;
    bool preambleLocked;
    bool payloadCrcSuccess;
    bool syncCorruptionOccurred;
    double multiEdgeLossDb;
    double dominantEdgeHeightM;
    uint32_t terrainReflectionCount;
    uint32_t hiddenChannelState;
    double dopplerSpectrumWidthHz;
    
    // Advanced SDR/emulation framework telemetry (Stage 4)
    double symbolErrorProbability;
    double packetCrcProbability;
    double softSymbolMarginDb;
    double residualCfoHz;
    double residualTimingErrorNs;
    double syncConfidence;
    double agcConvergence;
    bool sicRecovered;
    uint32_t fidelityLevel;
    double clusterSpatialStability;
    std::string environmentRegion;
    double phaseCoherence;
    
    // Final Advanced Emulation Framework Telemetry (Stage 5)
    double iqPhaseContinuity;
    double coherentEnergyRatio;
    double instantaneousFrequencyErrorHz;
    double fftPeakConfidence;
    double symbolLikelihood;
    double crcConfidence;
    bool pllLocked;
    double pllPhaseErrorRad;
    bool cycleSlipOccurred;
    double timingLockQuality;
    double timingJitterNs;
    uint32_t fidelitySchedulerState;
    double batchProcessingGain;
    double phaseNoiseVariance;
    double combinedSnrDb;
    uint32_t gatewayDiversityOrder;
    bool cooperativeDecodeSuccess;
    uint32_t rfMemoryRegionId;
    
    // Feature 1: Gudmundson correlation output fields
    double gudmundsonCacheDistance;
    double gudmundsonRho;
    
    // Feature 2: Earth curvature and Fresnel zone output fields
    double earthBulgM;
    double fresnelRadiusM;
    double fresnelVParameter;
    
    // Feature 3: Atmospheric pressure and refraction output fields
    double effectiveEarthRadiusK;
    double atmosphericRefractivityN;
    
    // Feature 4: Snow model output fields
    double snowAttenuationDb;
    
    // Feature 5: Wind foliage variation output fields
    double foliageStaticDb;
    double foliageDynamicDb;
    
    // Feature 6: Temperature-dependent CFO output fields
    double temperatureAtTime;
    double temperatureCFOContributionHz;
    
    // Feature 7: Dynamic delay spread output fields (already has delaySpreadNs and coherenceBandwidthHz)
    double delaySpreadSelectivityPenaltyDb;
    
    // Feature 8: Humidity dielectric output fields
    double humidityRefractivityN;
    double humidityPhaseShiftRad;
    
    // Feature 9: ADR output fields
    uint8_t adrAssignedSF;
    double snrMarginDb;
    
    // SNR fields (unclamped vs clamped)
    double rawSNRDb;  // Actual computed SNR (may be negative and below -30)
    
    // Missing Tier 2/3 state outputs
    bool isTwoRay;
    bool isTunnel;
    double soilAttenuationDb;
    double estimatedRangeM;
    double rangeErrorM;
    double positionErrorM;
    double propagationLossBiasDb;
    //! Ranging 1-sigma in metres for the EKF R diagonal. 0.0 means unavailable
    //! (RANGING_SYNTHETIC_GPS_DENIED), NOT zero uncertainty.
    double rangingSigmaM;
    double sampledRainRateMmHr;  // P3: actual sampled rain rate (not the dist mean)
    
    // Existing submodule states needed for Phase C
    bool isNLOS;
    double kFactor;
    double shadowingStdEff;
    
    // Missing Tier 2 losses
    double buildingEntryDb;
    double wallPenetrationDb;
    double floorAttenuationDb;
    double bodyShadowingDb;
    double temporalBlockageDb;
};

/**
 * @brief Main LoRa Propagation and Reception impairment engine
 */
class LoRaUWBPhysicsEngine : public Object {
public:
    static TypeId GetTypeId();
    LoRaUWBPhysicsEngine();
    virtual ~LoRaUWBPhysicsEngine() override;
    
    // Static singleton instance registration for integration into ns-3 event loop
    static Ptr<LoRaUWBPhysicsEngine> GetInstance();
    static void SetInstance(Ptr<LoRaUWBPhysicsEngine> instance);
    
    // Initialize engine with a specific configuration
    void Initialize(const LoRaUWBPhysicsConfig& config);
    
    // Load config from a YAML file
    void LoadConfig(const std::string& yamlPath);
    
    // Core physical layer entry point called per packet attempt
    PhysicsResultUWB ProcessTransmission(
        uint32_t txNodeId, uint32_t rxGatewayId,
        Vector txPosition, Vector rxPosition,
        double txPowerDbm, uint8_t spreadingFactor, double bandwidthHz,
        uint8_t relayHops, Time transmissionTime,
        double nodeOrientationRadians = 0.0);
    
    // Semtech SX1276 power sensitivity thresholds in dBm per Spreading Factor and Coding Rate
    double GetSensitivityThreshold(uint8_t spreadingFactor, uint8_t codingRateDenom);
    
    // Registers a gateway for unique CFO and hardware bias tracking
    void RegisterGateway(uint32_t gatewayId, Vector position,
                          bool fineTimestamp);
    
    // Registers a node for CFO and persistent TX power tracking
    void RegisterNode(uint32_t nodeId, Vector initialPosition);
    
    // Updates node position dynamically for correct distance and velocity tracking
    void UpdateNodePosition(uint32_t nodeId, Vector newPosition);
    //! TECH_DEBT M.1(b), --relayRoundInSlice: journal the interference registry (see InterferenceSubmodule).
    void SetInterferenceJournal(bool on) { if (m_interference) { m_interference->SetJournal(on); } }
    //! Put back the journalled transmissions overlapping [from, to); returns how many.
    uint32_t ReplayInterferenceJournal(Time from, Time to)
    {
        return m_interference ? m_interference->ReplayJournal(from, to) : 0u;
    }
    
    // Explicitly sets node velocity and heading (for unit testing and static time simulation)
    void SetNodeVelocity(uint32_t nodeId, double velocityMps, double headingRad = 0.0);
    
    // Returns active atmospheric weather state
    WeatherState GetCurrentWeather() const;
    
    // Sets active weather state (for unit testing and scenario shifts)
    void SetWeatherState(WeatherState state);
    void UpdateAtmosphericConfig() {
        if (m_atmospheric) {
            m_atmospheric = std::make_unique<AtmosphericSubmodule>(m_config.atmospheric, m_sampler);
        }
    }
    
    // Telemetry statistics accessors
    EngineStatisticsUWB GetStatistics() const;
    void ResetStatistics();
    
    // Resets hardware jitter distribution (for Dataset D fine timestamp mode)
    void ResetGatewayJitter();
    
    std::shared_ptr<LoRaCrossLayerEngine> GetCrossLayerEngine() const { return m_crossLayer; }
    LoRaSdrValidator* GetValidator() const { return m_validator.get(); }
    LoRaValidationSuite* GetValidationSuite() const { return m_validationSuite.get(); }
    LoRaCooperativePhyEngine* GetCooperativePhy() const { return m_cooperativePhy.get(); }
    LoRaScalabilityEngine* GetScalabilityEngine() const { return m_scalability.get(); }
    LoRaGeographicMemoryEngine* GetGeographicMemory() const { return m_geographicMemory.get(); }

    // Mutable config accessor for parameter sweeps (avoids re-initialization that resets correlation state)
    LoRaUWBPhysicsConfig& GetMutableConfig() { return m_config; }
    const LoRaUWBPhysicsConfig& GetConfig() const { return m_config; }

private:
    LoRaUWBPhysicsConfig m_config;
    LoRaDistributionSampler m_sampler;
    LoRaDistributionSampler m_fadingSampler;
    
    // Submodule computing engines
    std::unique_ptr<LoRaCorrelationEngine> m_correlation;
    std::unique_ptr<LoRaSpatialFieldEngine> m_spatial;
    std::unique_ptr<ShadowingSubmodule> m_shadowing;
    std::unique_ptr<InterferenceSubmodule> m_interference;
    std::unique_ptr<HardwareSubmodule> m_hardware;
    std::unique_ptr<AtmosphericSubmodule> m_atmospheric;
    std::unique_ptr<DelaySpreadSubmodule> m_delaySpread;
    std::unique_ptr<AntennaSubmodule> m_antenna;
    std::unique_ptr<TerrainSubmodule> m_terrain;
    std::unique_ptr<CSSPHYSubmodule> m_cssPhy;
    std::unique_ptr<IQImpairmentSubmodule> m_iqImpairment;
    std::unique_ptr<BasebandSubmodule> m_baseband;
    std::unique_ptr<SyncLoopSubmodule> m_syncLoop;
    std::unique_ptr<SemanticRegionSubmodule> m_region;
    std::shared_ptr<LoRaCrossLayerEngine> m_crossLayer;
    std::unique_ptr<LoRaSdrValidator> m_validator;
    
    // Tier 2 Submodules
    std::unique_ptr<BuildingEntrySubmodule> m_buildingEntry;
    std::unique_ptr<WallMaterialSubmodule> m_wallMaterial;
    std::unique_ptr<FloorAttenuationSubmodule> m_floorAttenuation;
    std::unique_ptr<NodePositionUncertaintySubmodule> m_nodePositionUncertainty;
    std::unique_ptr<TemporalBlockageSubmodule> m_temporalBlockage;
    std::unique_ptr<BodyShadowingSubmodule> m_bodyShadowing;
    
    // Tier 3 Submodules
    std::unique_ptr<TunnelSubmodule> m_tunnel;
    std::unique_ptr<TwoRayGroundSubmodule> m_twoRayGround;
    std::unique_ptr<SoilAttenuationSubmodule> m_soilAttenuation;
    
    // Stage 5 Emulation Framework Submodules
    std::unique_ptr<IQStreamSubmodule> m_iqStream;
    std::unique_ptr<DspChainSubmodule> m_dspChain;
    std::unique_ptr<PllSubmodule> m_pll;
    std::unique_ptr<SymbolSyncSubmodule> m_symbolSync;
    std::unique_ptr<RFNonlinearitySubmodule> m_rfNonlinearity;
    std::unique_ptr<LoRaScalabilityEngine> m_scalability;
    std::unique_ptr<LoRaGeographicMemoryEngine> m_geographicMemory;
    std::unique_ptr<LoRaValidationSuite> m_validationSuite;
    std::unique_ptr<LoRaCooperativePhyEngine> m_cooperativePhy;

    // Stage 6 Scale and Realism Submodules
    std::unique_ptr<MicroDspSubmodule> m_microDsp;
    std::unique_ptr<RadioFrontendSubmodule> m_radioFrontend;
    std::unique_ptr<SpectrumOccupancySubmodule> m_spectrumOccupancy;
    std::unique_ptr<DistributedPhyEngine> m_distributedPhy;
    std::unique_ptr<GlobalValidationEngine> m_globalValidation;
    std::unique_ptr<DistributedScalabilityEngine> m_distributedScalability;
    std::unique_ptr<AdaptiveEnvironmentEngine> m_adaptiveEnvironment;
    
    // Fading tracking: linkId -> last fading term and last update time
    struct FadingState {
        double lastFadingDb;
        Time lastUpdateTime;
    };
    std::map<uint64_t, FadingState> m_fadingStates;
    std::map<uint64_t, bool> m_linkHysteresisActive;
    std::map<uint64_t, uint32_t> m_hiddenChannelStates;
    std::map<uint64_t, Time> m_lastStateTransition;
    
    // Dynamic node states tracking (mobility)
    struct NodeState {
        Vector position;
        double velocityMps;
        double headingRad;
        Time lastPositionUpdate;
    };
    std::map<uint32_t, NodeState> m_nodeStates;
    
    // Gateway locations registry
    struct GatewayState {
        Vector position;
        bool fineTimestamp;
    };
    std::map<uint32_t, GatewayState> m_gatewayStates;
    
    // Telemetry statistics
    EngineStatisticsUWB m_stats;
    bool m_initialized;
    
    // Generate unique link ID from tx and rx node IDs
    uint64_t MakeLinkId(uint32_t txId, uint32_t rxId);
    bool PassesReceiverSensitivity(double rxPowerDbm, double effectiveSensitivityDbm);
};

} // namespace ns3

#endif // LORA_UWB_PHYSICS_ENGINE_H
