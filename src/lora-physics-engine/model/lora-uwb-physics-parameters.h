#ifndef LORA_UWB_PHYSICS_PARAMETERS_H
#define LORA_UWB_PHYSICS_PARAMETERS_H

#include "lora-physics-parameters.h"

#include <cstdint>
#include <string>

namespace ns3 {

/**
 * @brief One DW3000 operating mode, as one data-sheet row (--uwbMode, TECH_DEBT 2.4).
 *
 * DW3000 Data Sheet v1.1 (Decawave 2020), Table 9, channel 9, 1 % PER: 850 kb/s with a
 * 1024-symbol preamble -99.3 dBm; 6.8 Mb/s with a 64-symbol preamble -93.2 dBm. The scenario
 * bills frames, integrates ranging and decodes in the mode this returns, and the sensitivity
 * probe applies the same row, so G93 checks what the scenario runs.
 * scripts/sensitivity_fidelity_check.py holds an independent copy.
 *
 * Returns false (and leaves the outputs untouched) unless \p mode is "6m8" or "850k".
 */
bool Dw3000Table9Row(const std::string& mode,
                     uint32_t& preambleSymbols,
                     double& dataRateBps,
                     double& sensitivityDbm);

/**
 * @brief Which LOS/NLOS state model the UWB engine uses (TECH_DEBT M.2).
 *
 * LEGACY is the model every UWB result so far was produced under: ComputeNLOSPenaltyDb()
 * with the environment's kilometre-scale NLOS distance (~0.1 % NLOS at UWB range
 * outdoors) and a +9.64 dB-shifted attenuation that exceeds UWB's margin.
 *
 * IEEE802154A is report/PREREG_UWB_NLOS.md sect 2, implemented as written there: the state
 * from the 3GPP TR 38.901 InF-SL LOS law, the excess from the IEEE 802.15.4a NLOS-minus-LOS
 * path loss. It is an experiment, never the default: 802.15.4a has no LOS/NLOS probability
 * and no data for disaster areas (its Remark 1), so the mapping onto this scenario is OURS.
 */
enum UwbNlosModel
{
    UWB_NLOS_LEGACY = 0,
    UWB_NLOS_IEEE802154A = 1
};

/**
 * @brief P_NLOS(d_2D) = 1 - exp(-d_2D / k), k = -d_clutter / ln(1 - r) = 44.81 m.
 *
 * CITED: the form is 3GPP TR 38.901 V19.2.0 Table 7.4.2-1, InF-SL row
 * (Pr_LOS = exp(-d_2D / k_subsce)); d_clutter = 10 m is Table 7.2-4's InF-SL clutter size;
 * r = 20 % is Table 7.8-7's low clutter density. OURS: using a factory's sparse-clutter law
 * for a ground-level outdoor SAR link (report/PREREG_UWB_NLOS.md sect 2). d_2D is the
 * horizontal distance, as in the source; d_2D <= 0 gives 0.
 */
double UwbNlosProbabilityInfSl(double d2DM);

/**
 * @brief The IEEE 802.15.4a NLOS-minus-LOS path loss for one environment, in dB.
 *
 *   dPL = (PL0_N - PL0_L) + 10 (n_N - n_L) log10(max(d_3D, 1 m) / 1 m)
 *         + 20 (kappa_N - kappa_L) log10(f / 5 GHz)
 *
 * CITED: IEEE P802.15-04-0662-02-004a (Molisch et al.), eq. (5) at d0 = 1 m and the
 * frequency term of eq. (12), PL0 referenced at fc = 5 GHz (eq. 10); the antenna
 * attenuation is common to both sets and cancels. Pairs: CM3/CM4 office for
 * ENV_INDOOR_OFFICE, CM5/CM6 outdoor for every other class -- the same mapping
 * CaTblCirModelFor() uses to pick the CIR's S-V set, so the link budget and the CIR describe
 * one channel. The outdoor NLOS n, sigma_S and kappa are marked "educated guesses" in the
 * report's own table, and its fits are measured over 3-28 m (office) and 5-17 m (outdoor):
 * beyond that this is an extrapolation. OURS: adding this difference of two 802.15.4a fits
 * on top of the engine's own absolute LOS path loss.
 */
double UwbNlosExcessDb802154a(double d3DM, double carrierHz, EnvironmentClass env);

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

    // TECH_DEBT M.2: the LOS/NLOS state model. LEGACY unless --uwbNlosModel says otherwise.
    UwbNlosModel nlosModel = UWB_NLOS_LEGACY;

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
