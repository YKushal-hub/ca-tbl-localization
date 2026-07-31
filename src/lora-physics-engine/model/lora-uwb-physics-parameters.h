#ifndef LORA_UWB_PHYSICS_PARAMETERS_H
#define LORA_UWB_PHYSICS_PARAMETERS_H

#include "lora-physics-parameters.h"

namespace ns3 {

/**
 * @brief UWB specific Receiver Decision Parameters
 * SX1280 supports SF5-SF12.
 */
struct ReceiverDecisionParamsUWB {
    bool softThresholdEnabled;
    double softThresholdWidthDb;
    double sensitivityDbm[8];              // index 0=SF5, 7=SF12
    double codingRateSensitivityOffsetDb[4]; // index 0=CR4/5, 3=CR4/8
    double linkHysteresisMarginDb;
    double preambleUnlockPenaltyDb;
    double pllUnlockPenaltyDb;
    double evmPenaltyThresholdPercent;
    double evmPenaltyDbPerPercent;
    double cfoPenaltyThresholdHz;
    double cfoPenaltyDbPerHz;
    double crcFailurePenaltyDb;

    static ReceiverDecisionParamsUWB Defaults();
};

/**
 * @brief UWB specific master configuration
 * Uses the same structs as 868 MHz where physics logic is identical,
 * but substitutes UWB specific structs (ReceiverDecisionParamsUWB)
 * where the hardware parameters fundamentally differ in shape.
 */
struct LoRaUWBPhysicsConfig {
    SimulationFidelityMode fidelityMode;
    EnvironmentClass environment;
    WeatherState initialWeather;
    
    PropagationParams propagation;
    ShadowingParams shadowing;
    FadingParams fading;
    NLOSParams nlos;
    AtmosphericParams atmospheric;
    FoliageParams foliage;
    InterferenceParams interference;
    ReceiverDecisionParamsUWB receiver; // UWB specific receiver
    HardwareParams hardware;
    MobilityParams mobility;
    TerrainParams terrain;
    AntennaParams antenna;
    IQImpairmentParams iqImpairment;
    
    // Feature extension parameters (10 features)
    DelaySpreadParams delaySpread;
    SnowParams snow;
    AtmosphericPressureParams atmosphericPressure;
    WindVariationParams windVariation;
    TemperatureCfoParams temperatureCfo;
    HumidityDielectricParams humidityDielectric;
    ADRParams adr;
    FreshnelTerrainParams freshnelTerrain;
    
    // Tier 2 Parameters
    BuildingEntryParams buildingEntry;
    WallMaterialParams wallMaterial;
    FloorAttenuationParams floorAttenuation;
    NodePositionUncertaintyParams nodePositionUncertainty;
    TemporalBlockageParams temporalBlockage;
    BodyShadowingParams bodyShadowing;
    
    // Tier 3 Parameters
    TunnelParams tunnel;
    TwoRayGroundParams twoRayGround;
    SoilAttenuationParams soilAttenuation;
    
    BasebandConfigParams baseband;
    SyncLoopConfigParams syncLoop;
    CrossLayerConfigParams crossLayer;
    SdrValidationConfigParams sdrValidation;
    
    IQStreamConfigParams iqStream;
    DspChainConfigParams dspChain;
    PllConfigParams pll;
    SymbolSyncConfigParams symbolSync;
    ScalabilityConfigParams scalability;
    AdvancedValidationConfigParams advancedValidation;
    RFNonlinearityConfigParams rfNonlinearity;
    CooperativePhyConfigParams cooperativePhy;
    GeographicMemoryConfigParams geographicMemory;

    // Stage 6 parameters
    MicroDspConfigParams microDsp;
    RadioFrontendConfigParams radioFrontend;
    DistributedPhyConfigParams distributedPhy;
    DistributedScalabilityConfigParams distributedScalability;
    GlobalValidationConfigParams globalValidation;
    SpectrumOccupancyConfigParams spectrumOccupancy;
    AdaptiveEnvironmentConfigParams adaptiveEnvironment;
    
    // CA-TBL ranging model selection
    RangingModelMode rangingModelMode = RANGING_SYNTHETIC_GPS_DENIED;
    RangingParams ranging;

    uint32_t randomSeed;
    bool verboseLogging;
    
    // Uses UWB specific parameter generation (e.g. 40.2 dB FSPL, new NLOS/Foliage)
    static LoRaUWBPhysicsConfig ForScenario(EnvironmentClass env,
                                         WeatherState weather,
                                         FadingModelType fadingType,
                                         VegetationType vegetation,
                                         MobilityModelType mobilityModel,
                                         bool fineTimestampGateways);
                                         
    static LoRaUWBPhysicsConfig FromYAML(const std::string& filepath);
    void ToYAML(const std::string& filepath) const;
};

} // namespace ns3

#endif // LORA_UWB_PHYSICS_PARAMETERS_H
