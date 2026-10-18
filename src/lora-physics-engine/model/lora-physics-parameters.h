#ifndef LORA_PHYSICS_PARAMETERS_H
#define LORA_PHYSICS_PARAMETERS_H

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include "ns3/vector.h"

namespace ns3 {

/**
 * @brief Persistent state for a single transmission link, passed sequentially
 * through the physics engine DAG of submodules.
 */
struct LinkPhysicsState {
    // Computed by NLOSSubmodule (upstream — must run first)
    bool   isNLOS             = false;
    double nlosPenaltyDb      = 0.0;
    double pendingTerrainDb   = 0.0;

    // Computed by PropagationSubmodule
    double propagationLossDb  = 0.0;
    bool   isTwoRay           = false;

    // Computed by AtmosphericSubmodule (used by FoliageSubmodule)
    double rainRateMmHr       = 0.0;
    double atmosphericDb      = 0.0;

    // Computed by TerrainSubmodule (used by ShadowingSubmodule)
    double terrainDb          = 0.0;
    bool   inTerrainShadow    = false;

    // Modified by NLOS-K coupling (using isNLOS from NLOSSubmodule)
    double kFactor            = 4.0;   // starts as environment default, overridden

    // Modified by NLOS-sigma coupling (using isNLOS from NLOSSubmodule)
    double shadowingStdEff    = 8.0;   // starts as environment default, overridden
    double shadowingDb        = 0.0;

    // Modified by NLOS-delayspread coupling
    double delaySpreadNs      = 100.0;

    // Computed by FadingSubmodule
    double fadingDb           = 0.0;

    // Modified by FoliageSubmodule (reads rainRateMmHr for wet-foliage effect)
    double foliageDb          = 0.0;

    // Computed by BuildingEntrySubmodule (reads environment, writes isIndoor)
    bool   isIndoor           = false;

    // New submodules
    double buildingEntryDb    = 0.0;
    double wallPenetrationDb  = 0.0;
    double floorAttenuationDb = 0.0;
    double bodyShadowingDb    = 0.0;
    double temporalBlockageDb = 0.0;
    double tunnelLossDb       = 0.0;
    double soilAttenuationDb  = 0.0;
    
    // Node configuration
    uint32_t txNodeId = 0;
    double nodeOrientationRadians = 0.0;
    
    // Geometry
    Vector txPos;
    Vector rxPos;

    // GPS-denied position uncertainty
    double trueRangeM         = 0.0;
    double estimatedRangeM    = 0.0;
    double rangeErrorM        = 0.0;
    double positionErrorM     = 0.0;
    double propagationLossBiasDb = 0.0;
    // Ranging 1-sigma in metres, for the EKF R-matrix diagonal. Written by
    // ComputeRangingErrorM() in RANGING_PHYSICS_DERIVED mode only; stays 0.0 under
    // RANGING_SYNTHETIC_GPS_DENIED, and consumers must treat 0.0 as "not available"
    // rather than as "perfect".
    //
    // This field exists because the sigma was previously computed and thrown away:
    // ComputeRangingErrorM() returned it, all three engine call sites ignored the
    // return value, and CaTblPolicy built R from a fixed per-tier constant instead
    // (losSigmaM = {50, 15, 0.1} m). Against the measured error that constant was
    // over-confident on tier 1 by up to 58x at 6 km and under-confident on tier 2 by
    // 5-19x, so the filter discounted good ranges and trusted bad ones.
    double rangingSigmaM      = 0.0;
    double rangingShadowingDb = 0.0; //!< S1: this link's shadowing draw, handed to the 868 ranging
    
    bool   isTunnel           = false;
    
    // Terrain state
    bool terrainLosBlocked    = false;

    // Thermal noise
    double thermalNoiseShiftDb = 0.0;

    // Total (computed at end of pipeline)
    double totalPathLossDb    = 0.0;
    double rxPowerDbm         = 0.0;
    double snrDb              = 0.0;
    double marginDb           = 0.0;
    bool   payloadValid       = false;
};

/**
 * @brief Every configurable physical quantity in the engine uses a DistributionParam.
 * No hardcoded values are allowed in calculations, enabling extensive sensitivity analyses.
 */
struct DistributionParam {
    double min;          // Hard lower bound - sampled values are clamped here
    double max;          // Hard upper bound - sampled values are clamped here
    double mean;         // Mean of the distribution OR deterministic value
    double stddev;       // Standard deviation for NORMAL and LOGNORMAL
    double lambda;       // Rate parameter for EXPONENTIAL (lambda = 1 / mean)
    double shape;        // Shape parameter for NAKAGAMI (m)
    
    enum DistributionType {
        DETERMINISTIC,   // Always returns mean, ignores other parameters
        UNIFORM,         // Uniform between min and max
        NORMAL,          // Normal(mean, stddev) clamped to [min, max]
        LOGNORMAL,       // Lognormal(mean, stddev) of underlying normal clamped to [min, max]
        EXPONENTIAL,     // Exponential(lambda) clamped to [min, max]
        NAKAGAMI         // Nakagami(shape, mean) for fading amplitude clamped to [min, max]
    } type;
    
    // Constructor for deterministic
    static DistributionParam Fixed(double value) {
        DistributionParam p;
        p.min = value;
        p.max = value;
        p.mean = value;
        p.stddev = 0.0;
        p.lambda = 0.0;
        p.shape = 0.0;
        p.type = DETERMINISTIC;
        return p;
    }
    
    // Constructor for normal
    static DistributionParam Normal(double mean, double stddev, double min, double max) {
        DistributionParam p;
        p.min = min;
        p.max = max;
        p.mean = mean;
        p.stddev = stddev;
        p.lambda = 0.0;
        p.shape = 0.0;
        p.type = NORMAL;
        return p;
    }
    
    // Constructor for lognormal
    static DistributionParam LogNormal(double mean, double stddev, double min, double max) {
        DistributionParam p;
        p.min = min;
        p.max = max;
        p.mean = mean;
        p.stddev = stddev;
        p.lambda = 0.0;
        p.shape = 0.0;
        p.type = LOGNORMAL;
        return p;
    }
    
    // Constructor for uniform
    static DistributionParam Uniform(double min, double max) {
        DistributionParam p;
        p.min = min;
        p.max = max;
        p.mean = (min + max) / 2.0;
        p.stddev = (max - min) / std::sqrt(12.0);
        p.lambda = 0.0;
        p.shape = 0.0;
        p.type = UNIFORM;
        return p;
    }
    
    // Constructor for exponential
    static DistributionParam Exponential(double lambda, double min, double max) {
        DistributionParam p;
        p.min = min;
        p.max = max;
        p.mean = 1.0 / lambda;
        p.stddev = 1.0 / lambda;
        p.lambda = lambda;
        p.shape = 0.0;
        p.type = EXPONENTIAL;
        return p;
    }

    // Constructor for nakagami
    static DistributionParam Nakagami(double shape, double mean, double min, double max) {
        DistributionParam p;
        p.min = min;
        p.max = max;
        p.mean = mean;
        p.stddev = 0.0;
        p.lambda = 0.0;
        p.shape = shape;
        p.type = NAKAGAMI;
        return p;
    }
};

/**
 * @brief Environment Classifications representing standard LoRa deployment scenarios
 */
enum EnvironmentClass {
    ENV_RURAL_OPEN,
    ENV_RURAL_FORESTED,
    ENV_SUBURBAN,
    ENV_URBAN_MODERATE,
    ENV_URBAN_DENSE,
    ENV_INDUSTRIAL,
    ENV_INDOOR_OFFICE,
    ENV_CONCRETE_BUNKER,
    ENV_TUNNEL_CONFINED
};

/**
 * @brief Weather State representing dynamic atmospheric absorption conditions
 */
enum WeatherState {
    WEATHER_CLEAR_DRY,
    WEATHER_HUMID_HOT,
    WEATHER_LIGHT_RAIN,
    WEATHER_HEAVY_RAIN,
    WEATHER_FOG,
    WEATHER_WINDY_DRY,
    WEATHER_WINDY_WET,
    WEATHER_LIGHT_SNOW,
    WEATHER_HEAVY_WET_SNOW
};

/**
 * @brief Fading Model options for small-scale amplitude variation
 */
enum FadingModelType {
    FADING_NONE,
    FADING_RAYLEIGH,
    FADING_RICIAN,
    FADING_NAKAGAMI
};

/**
 * @brief Vegetation Type for foliage attenuation models (ITU-R P.833-10)
 */
enum VegetationType {
    VEG_NONE,
    VEG_SPARSE_SHRUBS,
    VEG_LIGHT_FOREST,
    VEG_DENSE_FOREST,
    VEG_TROPICAL_FOREST,
    VEG_CROP_FIELD,
    VEG_ORCHARD
};

/**
 * @brief Mobility models governing node speeds and Doppler shifts
 */
enum MobilityModelType {
    MOB_STATIC,
    MOB_RANDOM_WALK,
    MOB_RANDOM_WAYPOINT,
    MOB_CONSTANT_VELOCITY,
    MOB_ROAD_CONSTRAINED
};

/**
 * @brief Spatial distributions of end-device nodes
 */
enum NodeDistributionType {
    DIST_UNIFORM_DISK,
    DIST_CLUSTERED_POISSON,
    DIST_ROAD_LINEAR
};

/**
 * @brief Gateway antenna placement styles
 */
enum GatewayPlacementType {
    GW_ROOFTOP_URBAN,
    GW_TOWER_RURAL,
    GW_HILLTOP,
    GW_GROUND_LEVEL,
    GW_MIXED
};

/**
 * @brief Advanced antenna parameters including 3GPP sector directivity
 */
struct AntennaParams {
    double peakGainDbi;
    double beamwidthDeg;
    double frontToBackRatioDb;
    double downtiltDeg;
    double polarizationMismatchLossDb;
    bool directional;
    
    static AntennaParams Defaults();
    static AntennaParams DirectionalSector();
};

/**
 * @brief Path loss parameters governing path-loss exponent and heights
 */
struct PropagationParams {
    DistributionParam pathLossExponent;
    EnvironmentClass envClass;       // To allow emergent Hata/2-Ray branching
    double referenceDistanceM;       // Fixed at 1.0m (physical standard d0)
    double frequencyHz;              // Frequency of deployment (default 868e6)
    DistributionParam gatewayHeightM;
    DistributionParam nodeHeightM;
    
    // Dual-Slope Path Loss Feature
    bool enableDualSlope;
    double dualSlopeBreakpointM;
    DistributionParam pathLossExponent2;
    
    static PropagationParams DefaultsFor(EnvironmentClass env);
};

/**
 * @brief Shadowing parameters (log-normal large-scale fading)
 */
struct ShadowingParams {
    bool enabled;
    DistributionParam shadowingStddevDb;
    DistributionParam correlationDistanceM;
    bool spatialCorrelationEnabled;
    double updateIntervalS;          // 0 = static shadowing map for the full run
    double nlosToLosSigmaRatio;
    
    static ShadowingParams DefaultsFor(EnvironmentClass env);
};

/**
 * @brief Small-scale multipath fading parameters
 */
struct FadingParams {
    bool enabled;
    FadingModelType model;
    DistributionParam ricianKFactor;
    double kNlosMean;
    DistributionParam nakagamiM;
    DistributionParam coherenceTimeMs;
    
    // Dataset E fading isolation
    bool useIsolatedSeed;
    uint32_t isolatedSeed;
    
    static FadingParams DefaultsFor(EnvironmentClass env);
};

/**
 * @brief Non-Line-of-Sight (NLOS) penetration and obstruction penalties
 */
struct NLOSParams {
    bool enabled;
    EnvironmentClass envClass; // Added for emergent building intersection
    double frequencyHz; // Added for 3GPP UMa NLOS calculation
    DistributionParam nlosScaleDistanceM; // Controls P_NLOS(d) = 1 - exp(-d / scale)
    DistributionParam nlosAttenuationDb;
    DistributionParam diffractionLossDb;
    DistributionParam buildingPenetrationDb;
    DistributionParam floorLossDbPerFloor;
    int defaultNumberOfFloors;
    // V2a-2 (master plan v3), --nlos3gppRMa. false (default): P_NLOS = 1 - exp(-d / scale) and the NLOS loss is drawn
    // from the per-environment Normal above (values OURS). true: for links whose higher end is >= 10 m (the RMa
    // base-station range, 10-150 m; the gateways), P_LOS and the NLOS excess loss follow 3GPP TR 38.901 v17.0.0 RMa:
    // Table 7.4.2-1 (Pr_LOS = 1 for d2D <= 10 m, else exp(-(d2D - 10)/1000)) and Table 7.4.1-1 (excess =
    // max(PL_LOS, PL'_NLOS) - PL_LOS, h = 5 m, W = 20 m, note 5's breakpoint). Lower links (finders, handheld) keep the
    // model above: RMa is not defined for them.
    bool rma3gpp = false;
    
    static NLOSParams DefaultsFor(EnvironmentClass env);
};

/**
 * @brief Atmospheric absorption and thermal noise shift parameters (ITU-R P.838-3)
 */
/**
 * @brief Snow precipitation parameters (Feature 4)
 * Reference: Mätzler, C. (1987): Applications of microwave interaction with natural snow
 */
struct SnowParams {
    bool enabled;
    DistributionParam snowfallRateMmHrEquiv;
    DistributionParam snowTemperatureCelsius;
    double snowNLOSScaleFactor;  // 0.6 = 30-50% scale reduction

    static SnowParams Defaults();
};

/**
 * @brief Atmospheric pressure extension parameters (Feature 3)
 * Reference: ITU-R P.453-14 (2019): radio refractive index
 */
struct AtmosphericPressureParams {
    bool enableTroposphericRefraction;
    DistributionParam atmosphericPressureHPa;

    static AtmosphericPressureParams Defaults();
};

/**
 * @brief Humidity dielectric effect parameters (Feature 8)
 * Reference: ITU-R P.676-12 (2019): atmospheric gas and water vapor effects
 */
struct HumidityDielectricParams {
    bool enableHumidityDielectric;
    // humidity percentage already in AtmosphericParams.humidity

    static HumidityDielectricParams Defaults();
};

struct AtmosphericParams {
    bool enabled;
    WeatherState initialWeatherState;
    double weatherUpdateIntervalS;
    DistributionParam rainRateMmPerHr;
    double rainAttenuationK;         // ITU-R P.838-3 k coeff
    double rainAttenuationAlpha;     // ITU-R P.838-3 alpha coeff
    DistributionParam waterVaporDensityGPerM3;
    DistributionParam temperatureCelsius;
    DistributionParam fogAttenuationDbPerKm;
    // Feature extensions referenced by AtmosphericSubmodule
    AtmosphericPressureParams atmosphericPressure;
    SnowParams snow;
    HumidityDielectricParams humidityDielectric;
    
    static AtmosphericParams DefaultsFor(WeatherState state);
};

/**
 * @brief Foliage attenuation parameters (ITU-R P.833-10 exponential decay)
 */
struct FoliageParams {
    bool enabled;
    VegetationType type;
    DistributionParam specificAttenuationDbPerM;
    DistributionParam maxAttenuationDb;
    DistributionParam vegetationDepthM;
    DistributionParam seasonalMultiplier;
    DistributionParam wetFoliageAddDb;
    //! B3f --foliageDepthClamp: clamp the drawn vegetation depth to the link length (default false).
    bool clampDepthToLink = false;
    
    static FoliageParams DefaultsFor(VegetationType type);
};

/**
 * @brief Interference and Capture Effect parameters
 */
struct InterferenceParams {
    bool enabled;
    DistributionParam externalInterfererDensityPerKm2;
    DistributionParam externalTxPowerDbm;
    DistributionParam externalDutyCycle;
    bool captureEffectEnabled;
    double captureThresholdDb[6][6];     // index [rxSF-7][txSF-7]
    double sfIsolationDb[6][6];       // index [txSF-7][rxSF-7] isolation matrix
    DistributionParam adjacentChannelLeakageDb;
    bool selfInterferenceEnabled;
    double selfInterferenceWindowMs;
    
    static InterferenceParams DefaultsFor(EnvironmentClass env);
};

/**
 * @brief Receiver sensitivity and soft-decision thresholds.
 */
struct ReceiverDecisionParams {
    bool softThresholdEnabled;
    double softThresholdWidthDb;
    double sensitivityDbm[6];              // index 0=SF7, 5=SF12
    double codingRateSensitivityOffsetDb[4]; // index 0=CR4/5, 3=CR4/8
    double linkHysteresisMarginDb;
    double preambleUnlockPenaltyDb;
    double pllUnlockPenaltyDb;
    double evmPenaltyThresholdPercent;
    double evmPenaltyDbPerPercent;
    double cfoPenaltyThresholdHz;
    double cfoPenaltyDbPerHz;
    double crcFailurePenaltyDb;

    static ReceiverDecisionParams Defaults();
};

/**
 * @brief Gateway and node hardware imperfections
 */
struct HardwareParams {
    bool enabled;
    bool perNodeVariationEnabled;
    DistributionParam carrierFrequencyOffsetHz;
    DistributionParam phaseNoiseRad;
    DistributionParam timestampJitterNs;
    DistributionParam timestampQuantizationNs;
    double rssiResolutionDb;         // SX1276 default is 1.0 dB
    DistributionParam rssiHardwareVariationDb;
    DistributionParam sensitivityVariationDb;
    DistributionParam txPowerAccuracyDb;
    bool fineTimestampMode;          // true = GPS disciplined PPS gateway
    uint8_t codingRateDenom;         // CR denominator: 5 for 4/5, 6 for 4/6, etc.
    
    static HardwareParams Defaults();
    static HardwareParams DefaultsFineTimestamp();
};

/**
 * @brief Mobility and Doppler SNR degradation parameters
 */
struct MobilityParams {
    bool enabled;
    DistributionParam velocityMps;
    MobilityModelType mobilityModel;
    double directionUpdateIntervalS;
    bool dopplerDegradationEnabled;
    DistributionParam dopplerSNRPenaltyDbPerHz;
    NodeDistributionType nodeDistribution;
    int nClusters;
    DistributionParam clusterRadiusM;
    DistributionParam nodesPerCluster;
    
    static MobilityParams DefaultsFor(MobilityModelType model);
};

/**
 * @brief Terrain profile and Fresnel zone knife-edge obstruction parameters
 */
struct TerrainParams {
    bool enabled;
    std::vector<double> gatewayHeightOptionsM;
    GatewayPlacementType placement;
    DistributionParam terrainIrregularityM;
    DistributionParam losScaleDistanceM;
    double fresnelClearanceFraction;  // Standard is 0.6
    //! Wavelength basis for the Fresnel and diffraction geometry.
    //!
    //! Defaults to 868e6 on EVERY tier, which is what the submodule
    //! hardcoded before this field existed. That is wrong for the other two
    //! tiers -- UWB Ch9 is 7.99 GHz, so its wavelength is out by a factor of
    //! 9.2 -- but it is the behaviour every result in the tree was measured
    //! under, so it stays the default and moves only behind
    //! --terrainPerTierFreq.
    double frequencyHz;
    double effectiveEarthRadiusFactor; // Standard K-factor is 4/3
    
    static TerrainParams Defaults();
};

/**
 * @brief Simulation Fidelity Modes for scalable computational complexity
 */
enum SimulationFidelityMode {
    FIDELITY_ULTRA_FAST_STATISTICAL = 0,
    FIDELITY_CORRELATED_PHY = 1,
    FIDELITY_ADVANCED_TDL_COLLISIONS = 2,
    FIDELITY_SDR_GRADE_REALISM = 3
};

/**
 * @brief Ranging model selection for CA-TBL three-tier simulation.
 *
 * RANGING_SYNTHETIC_GPS_DENIED (default): NodePositionUncertaintySubmodule
 *   (Sub.21) runs in the Tier-2/3 DAG group, producing synthetic Gaussian
 *   position noise independent of channel state. All ml_dataset_generator.cc
 *   outputs and validated statistics are unaffected.
 *
 * RANGING_PHYSICS_DERIVED: ComputeRangingErrorM() runs between Step 4
 *   (Fading) and Step 5 (Delay Spread), reading channel state Sub.21 never
 *   had access to. Sub.21 Compute() is skipped in this mode.
 */
enum RangingModelMode {
    RANGING_SYNTHETIC_GPS_DENIED,  // default -- Sub.21 behavior, unchanged
    RANGING_PHYSICS_DERIVED        // CA-TBL -- channel-state-correlated error
};

/**
 * @brief IQ Impairment parameters (amplitude imbalance, phase mismatch, ADC clipping)
 */
struct IQImpairmentParams {
    bool enabled;
    DistributionParam iqAmplitudeImbalanceDb;
    DistributionParam iqPhaseMismatchDeg;
    DistributionParam dcOffsetDb;
    DistributionParam adcEffectiveBits;
    DistributionParam adcClippingThresholdDbm;
    
    static IQImpairmentParams Defaults();
};

/**
 * @brief Baseband coherent symbol evolution parameters
 */
struct BasebandConfigParams {
    bool enabled;
    bool chirpPhaseTracking;
    bool dirichletSpectralLeakage;
    
    static BasebandConfigParams Defaults() {
        return {true, true, true};
    }
};

/**
 * @brief Receiver synchronization loop parameters
 */
struct SyncLoopConfigParams {
    bool enabled;
    double agcSettlingSpeed;
    
    static SyncLoopConfigParams Defaults() {
        return {true, 0.4};
    }
};

/**
 * @brief Cross-layer MAC/PHY adaptation parameters
 */
struct CrossLayerConfigParams {
    bool enabled;
    double adrAdaptationSpeed;
    
    static CrossLayerConfigParams Defaults() {
        return {true, 1.0};
    }
};

/**
 * @brief Empirical SDR validation parameters
 */
struct SdrValidationConfigParams {
    bool enabled;
    std::string traceLogPath;
    
    static SdrValidationConfigParams Defaults() {
        return {true, "empirical_validation.trace"};
    }
};

/**
 * @brief Continuous-time sparse IQ stream parameters
 */
struct IQStreamConfigParams {
    bool enabled;
    double sampleRateHz;
    
    static IQStreamConfigParams Defaults() {
        return {true, 1000000.0};
    }
};

/**
 * @brief Lightweight SDR DSP receiver chain parameters
 */
struct DspChainConfigParams {
    bool enabled;
    double agcThresholdDbm;
    
    static DspChainConfigParams Defaults() {
        return {true, -120.0};
    }
};

/**
 * @brief Second-order digital PLL model parameters
 */
struct PllConfigParams {
    bool enabled;
    double loopBandwidthHz;
    double dampingFactor;
    
    static PllConfigParams Defaults() {
        return {true, 150.0, 0.707};
    }
};

/**
 * @brief Symbol timing recovery dynamics parameters
 */
struct SymbolSyncConfigParams {
    bool enabled;
    double timingGateFraction;
    
    static SymbolSyncConfigParams Defaults() {
        return {true, 0.05};
    }
};

/**
 * @brief City-scale simulation scalability parameters
 */
struct ScalabilityConfigParams {
    bool enabled;
    double spatialTileResolutionM;
    uint32_t activeLinkThreshold;
    
    static ScalabilityConfigParams Defaults() {
        return {true, 50.0, 5000};
    }
};

/**
 * @brief Comprehensive SDR validation suite parameters
 */
struct AdvancedValidationConfigParams {
    bool enabled;
    std::string datasetPath;
    
    static AdvancedValidationConfigParams Defaults() {
        return {true, "empirical_sdr_dataset.csv"};
    }
};

/**
 * @brief RF frontend nonlinearity parameters
 */
struct RFNonlinearityConfigParams {
    bool enabled;
    DistributionParam paCompressionThresholdDbm;
    DistributionParam lnaSaturationThresholdDbm;
    DistributionParam iip3Dbm;
    
    static RFNonlinearityConfigParams Defaults() {
        return {true, 
                DistributionParam::Normal(20.0, 1.0, 15.0, 25.0), 
                DistributionParam::Normal(-20.0, 2.0, -30.0, -10.0), 
                DistributionParam::Normal(-5.0, 1.5, -10.0, 5.0)};
    }
};

/**
 * @brief Multi-gateway cooperative PHY combining parameters
 */
struct CooperativePhyConfigParams {
    bool enabled;
    bool maximalRatioCombining;
    
    static CooperativePhyConfigParams Defaults() {
        return {true, true};
    }
};

/**
 * @brief Persistent geographic RF memory parameters
 */
struct GeographicMemoryConfigParams {
    bool enabled;
    double memoryCellSizeM;
    
    static GeographicMemoryConfigParams Defaults() {
        return {true, 25.0};
    }
};

/**
 * @brief Stage 6: Hybrid MicroDSP configuration
 */
struct MicroDspConfigParams {
    bool enabled;
    double activationSnrThresholdDb;
    double collisionOverlapThreshold;
    
    static MicroDspConfigParams Defaults() {
        return {true, 5.0, 0.20};
    }
};

/**
 * @brief Stage 6: Advanced Radio Frontend configuration
 */
struct RadioFrontendConfigParams {
    bool enabled;
    double oscillatorAgingDriftHzPerYear;
    double thermalDriftHzPerCelsius;
    double agcRecoveryTimeMs;
    
    static RadioFrontendConfigParams Defaults() {
        return {true, 50.0, 3.3, 2.5};
    }
};

/**
 * @brief Stage 6: Distributed Cooperative PHY configuration
 */
struct DistributedPhyConfigParams {
    bool enabled;
    double fusionConfidenceThreshold;
    
    static DistributedPhyConfigParams Defaults() {
        return {true, 0.6};
    }
};

/**
 * @brief Stage 6: Distributed Scalability configuration
 */
struct DistributedScalabilityConfigParams {
    bool enabled;
    uint32_t regionPartitionSizeM;
    bool gpuBatchingEnabled;
    
    static DistributedScalabilityConfigParams Defaults() {
        return {true, 1000, true};
    }
};

/**
 * @brief Stage 6: Global Validation Engine configuration
 */
struct GlobalValidationConfigParams {
    bool enabled;
    std::string empiricalDatasetPath;
    
    static GlobalValidationConfigParams Defaults() {
        return {true, ""};
    }
};

/**
 * @brief Stage 6: Spectrum Occupancy configuration
 */
struct SpectrumOccupancyConfigParams {
    bool enabled;
    double backgroundIsmNoiseFloorDbm;
    double impulsiveNoiseProbability;
    
    static SpectrumOccupancyConfigParams Defaults() {
        return {true, -105.0, 0.05};
    }
};

/**
 * @brief Stage 6: Adaptive Environment Learning configuration
 */
struct AdaptiveEnvironmentConfigParams {
    bool enabled;
    double learningRate;
    double memoryDecayFactor;
    
    static AdaptiveEnvironmentConfigParams Defaults() {
        return {true, 0.1, 0.95};
    }
};

/**
 * @brief Dynamic delay spread parameters (Feature 7)
 * Values OURS: the cited 'Lim et al. 2018 (MDPI Sensors)' source was not found
 */
struct DelaySpreadParams {
    bool enabled;
    DistributionParam losDelaySpreadNs;
    DistributionParam nlosDelaySpreadNs;
    DistributionParam coherenceBandwidthScaleFactor;
    
    static DelaySpreadParams DefaultsFor(EnvironmentClass env);
};

/**
 * @brief Wind-induced foliage variation parameters (Feature 5)
 * Values OURS. Wind fading model: Cheffena & Ekman, EURASIP JWCN 2009, doi 10.1155/2009/306876 (Meng 2009 TAP is static path loss)
 */
struct WindVariationParams {
    bool windVariationEnabled;
    DistributionParam windSpeedMps;
    DistributionParam windAmplitudeDb;

    static WindVariationParams Defaults();
};

/**
 * @brief Temperature-dependent CFO and jitter parameters (Feature 6)
 * Values OURS: ~1-2 Hz/°C for SX1272. Robyns et al. 2017 (WiSec, fingerprinting) shows no such coefficient
 */
struct TemperatureCfoParams {
    DistributionParam temperatureCoefficientHzPerC;
    DistributionParam jitterTempSensitivity;
    double referenceTemperatureC;  // Standard 25.0°C

    static TemperatureCfoParams Defaults();
};


/**
 * @brief ADR mode parameters (Feature 9)
 * Reference: LoRaWAN specification, Semtech ADR algorithm
 */
struct ADRParams {
    bool adrEnabled;  // Default false for backward compatibility
    double adrSnrMarginThresholdDb;
    
    static ADRParams Defaults();
};

/**
 * @brief Earth curvature and Fresnel zone extension (Feature 2)
 * Reference: ITU-R P.526-15 (2019): Fresnel zone formula and earth bulge
 */
struct FreshnelTerrainParams {
    bool freshnelCalculationEnabled;
    double frequencyHz;  // Propagate from propagation config
    
    static FreshnelTerrainParams Defaults();
};

// --- Tier 2 Parameters ---

struct BuildingEntryParams {
    bool enabled;
    static BuildingEntryParams Defaults();
};

struct WallMaterialParams {
    bool enabled;
    DistributionParam glassLossDb;
    DistributionParam woodLossDb;
    DistributionParam concreteLossDb;
    DistributionParam metalLossDb;
    static WallMaterialParams Defaults();
};

struct FloorAttenuationParams {
    bool enabled;
    DistributionParam lossPerFloorDb;
    static FloorAttenuationParams Defaults();
};

struct NodePositionUncertaintyParams {
    bool enabled;
    DistributionParam horizontalErrorM;
    DistributionParam verticalErrorM;
    static NodePositionUncertaintyParams Defaults();
};

struct TemporalBlockageParams {
    bool enabled;
    DistributionParam blockageDurationS;
    DistributionParam blockageLossDb;
    static TemporalBlockageParams Defaults();
};

struct BodyShadowingParams {
    bool enabled;
    bool nodeIsBodyWorn;
    DistributionParam bodyLossDb;
    static BodyShadowingParams Defaults();
};

// --- Tier 3 Parameters ---

struct TunnelParams {
    bool enabled;
    DistributionParam waveguideLossPerKmDb;
    static TunnelParams Defaults();
};

struct TwoRayGroundParams {
    bool enabled;
    double defaultRelativePermittivity;
    static TwoRayGroundParams Defaults();
};

struct SoilAttenuationParams {
    bool enabled;
    DistributionParam burialDepthM;
    DistributionParam soilMoisturePercent;
    static SoilAttenuationParams Defaults();
};

/**
 * @brief Master configuration combining all engine components
 */
/**
 * @brief Parameters for RANGING_PHYSICS_DERIVED mode only.
 *
 * RANGING_SYNTHETIC_GPS_DENIED (Sub.21 / NodePositionUncertaintySubmodule)
 * does not consult this struct at all.
 *
 * WHAT CHANGED AND WHY (CA-TBL handoff section 6).
 * The tier-2 and tier-3 branches of ComputeRangingErrorM() previously used the
 * tofLosNoise / tofNlosNoise / uwbLosNoise / uwbNlosNoise standard deviations AS
 * the ranging model. Those distributions contain no bandwidth, no SNR and no
 * distance term, so the "physics-derived" range error was a constant: the
 * configured sigma read back out. Measured 2.4 GHz error was 0.245 / 0.242 /
 * 0.237 m at 100 / 430 / 900 m -- identical at every distance -- against a
 * Cramer-Rao bound of ~15 m at 800 kHz. Tier 2 consequently dominated every
 * accuracy-per-joule comparison for a non-physical reason and made claim 1
 * (hybrid beats single-band) look false.
 *
 * The four noise distributions are therefore GONE, not merely deprecated: a dead
 * field that used to be the model is an invitation to reuse the wrong thing.
 * They are replaced by explicit hardware FLOORS. Ranging sigma is now
 *
 *     sigma = max( CRLB(beta_rms, SNR_effective), floor )
 *
 * where the CRLB term carries all of the distance and SNR dependence and the
 * floor is what the transceiver cannot beat however good the link gets.
 *
 * The NLOS bias distributions are UNCHANGED and remain physical: first-path
 * suppression is a positive excess delay, so NLOS bias is additive on top of the
 * noise term. uwbNlosBias stays lognormal for the same reason (IEEE 802.15.4a).
 */
struct RangingParams {
    bool enabled;

    // --- Hardware ranging floors, metres, 1-sigma --------------------------
    //
    // tofLosFloorM   3.0 m  -- CITED to MEASUREMENT, not to a datasheet claim.
    //                          Zhou et al., "Impact of Environmental Factors on
    //                          LoRa 2.4 GHz Time of Flight Ranging Outdoors",
    //                          arXiv:2509.23125 (2025): SX1280 RF ToF ranging on an
    //                          open sports field, LOS, 40 cm antenna height, ~1000
    //                          samples at each of nine reference points spanning
    //                          roughly 28-57 m from the base station. Standard
    //                          deviation of absolute ranging error: 1.27 m at the
    //                          single best point, "the majority of points fall in
    //                          the 2.5-3.2 m range". 3.0 m is the upper-middle of
    //                          the typical band.
    //
    //                          This supersedes two earlier values. 0.3 m was an
    //                          unsourced placeholder. 1.0 m came from Semtech's
    //                          "meter-level, up to 2 m" product claim, which is a
    //                          best-case marketing figure; the measured field
    //                          spread is 2.5-3.2x worse at 28-57 m, and the
    //                          deployment modelled here is forest SAR, not a
    //                          groomed sports field.
    //
    //                          DECLARED CONFLICT: raising this floor weakens the
    //                          static_2p4 baseline and therefore moves the claim-1
    //                          comparison in the direction that favours the
    //                          project's own thesis. The justification is external
    //                          and holds regardless of that outcome -- peer-reviewed
    //                          field measurement beats a vendor best-case claim --
    //                          but for exactly that reason the arms MUST be reported
    //                          at both 1.0 m and 3.0 m. If the architectural
    //                          conclusion only survives at 3.0 m, that dependence is
    //                          the result.
    // tofNlosFloorM  3.6 m  -- NOT CITED. 1.2x the LOS floor, preserving the
    //                          LOS/NLOS ratio of the original placeholders. The
    //                          field study is LOS-only, so there is no measurement
    //                          to anchor this to. UNCALIBRATED -- sweep in S7.
    // uwbLosFloorM   0.10 m -- CITED. Benouakta et al., Electronics 2023, report
    //                          10-33 cm for DW3000-class hardware; 0.10 m is the
    //                          best-case end of the cited interval. Replaces the
    //                          previous 0.05 m, which sat BELOW the interval it
    //                          claimed to cite.
    // uwbNlosFloorM  0.25 m -- NOT CITED. Carried over from the old uwbNlosNoise
    //                          stddev. UNCALIBRATED -- sweep in S7.
    double tofLosFloorM;
    double tofNlosFloorM;
    double uwbLosFloorM;
    double uwbNlosFloorM;

    // --- NLOS excess-delay bias, metres (unchanged) ------------------------
    DistributionParam tofNlosBias;
    DistributionParam uwbNlosBias;

    // --- Coherent integration gain ----------------------------------------
    //
    // The time-of-arrival CRLB is a function of E/N0 at the correlator output,
    // not of in-band SNR. All three engines compute
    //   noiseFloorDbm  = -174 + 10*log10(occupiedBandwidth) + NF + thermalShift
    //   internalSnrDb  = rxPowerDbm - noiseFloorDbm
    // so result.snr_db is IN-BAND and pre-integration. No despread or
    // accumulation gain is applied anywhere upstream -- verified by inspection of
    // lora-physics-engine.cc, lora-2p4-physics-engine.cc and
    // lora-uwb-physics-engine.cc, all identical at lines 532 / 612 / 621 / 644.
    // The integration gain must therefore be added inside the ranging model.
    //
    // Tier 2 (LoRa CSS, dechirp): gain = 10*log10(2^SF), derived from SF at the
    //   call site, so nothing to configure. At SF7 that is 21.07 dB. Arithmetic
    //   at the measured 900 m operating point (SNR 16.9 dB, BW 800 kHz,
    //   beta_rms = 230,940 Hz, two-way): with the gain sigma = 1.30 m, without it
    //   sigma = 14.76 m. The 1.30 m figure lands inside the SX1280's specified
    //   meter-level performance, which is the independent check that the gain
    //   belongs here.
    //
    // Tier 3 (HRP UWB, preamble accumulation): gain = 10*log10(preambleSymbols).
    //   This one is not optional -- it is what makes UWB decodable at all. Raw
    //   UWB SNR is NEGATIVE (-17.4 dB at 2 m) because the thermal noise floor
    //   over 499.2 MHz is about -87 dBm against a -104 dBm sensitivity gate;
    //   decoding below the wideband noise floor is what a correlator receiver is
    //   for. At 2 m: without the gain sigma = 1.23 m, which contradicts the cited
    //   Benouakta 10-33 cm by 4-12x; with it sigma = 0.0384 m.
    //
    // uwbPreambleSymbols = 1024. The DW3000 Family User Manual lists PSR settings
    // of 16 / 64 / 128 / 1024 / 4096; 1024 is the long-preamble setting and gives
    // 30.1 dB. This is a real configurable hardware parameter (it trades airtime
    // and energy against accuracy), not a fitting knob, and S7 can sweep it.
    double uwbPreambleSymbols;

    // Tier 2 field bias, metres, added to EVERY tier-2 range, LOS or NLOS, with no
    // random draw. 0 (the default) adds exactly 0. Set only by --tofFieldBiasM, the
    // sensitivity to measured outdoor SX1280 ranging: Mueller et al., ICL-GNSS 2021
    // (doi 10.1109/ICL-GNSS51451.2021.9452277), sect. III, report a positive mean
    // distance error -- the range over-estimated -- of 46.4 m on a rural route up to
    // 1400 m and 39 m on a suburban route up to 500 m (BW 400 kHz, SF 8).
    double tofFieldBiasM;

    // S1 (master plan v2), --rssiRangeFromShadowing. false (default): the 868 MHz range error is an
    // independent, symmetric, truncated Gaussian each reading, as in every earlier result. true: the
    // range is the log-distance inversion of the link's path loss plus its own (Gudmundson-correlated)
    // shadowing draw, so the error is log-normal and correlated along the motion, as RSSI ranging is
    // (Patwari et al. 2005, p. 58-59). The Gaussian is still drawn, so no random stream moves.
    bool rssiRangeFromShadowing;

    // S2 (master plan v2, owner decision D3), --tofErrorModel. 0 (default, "engine"): the CRLB with
    // the floors above. 1 "gottschalk": calibrated static ranging, sigma = 1.5 m + 1 % of the range,
    // unbiased (Gottschalk et al., Internet of Things 37:101945, 2026: sub-2 m positioning to 150 m
    // after calibration). 2 "muller": the rural-route bins of Mueller et al., ICL-GNSS 2021 (Figs. 4, 6,
    // mean and SD per 100 m bin) with their eq. (5) correction d_c = 0.95 d + 1 applied. 3 "wolf":
    // Wolf et al., WPNC 2019, Table II, ranges <= 500 m: LoS mean 75 m / SD 69 m, NLoS 144 m / 112 m,
    // uncorrected. The engine's existing standard-normal draw is reused, so no random stream moves.
    int tofErrorModel;

    // V2a (master plan v3), --nlosRangeBias. false (default): an NLOS link's range is as unbiased as a
    // LOS one at 868 MHz (the inversion ignores the NLOS loss) and biased by N(1.5, 0.8) m at 2.4 GHz.
    // true: (i) 868 MHz -- the RSSI a receiver measures includes the link's NLOS excess loss, so the
    // log-distance inversion is applied to path loss + NLOS loss (+ shadowing under S1) and the range
    // is over-estimated by 10^(L/10n); (ii) 2.4 GHz -- an NLOS link's ToF range gains an exponentially
    // distributed positive excess (Guvenc & Chong, IEEE COMST 11(3) 2009, p. 109, eq. (2): exponential is
    // one of the standard NLOS-bias laws) with mean tofNlosExcessMeanM. Not applied under the "wolf" ToF
    // preset, whose NLoS row already carries it.
    bool nlosRangeBias;
    // Wolf et al., WPNC 2019, Table II (SX1280, <= 500 m): mean error NLoS 144 m - LoS 75 m = 69 m.
    double tofNlosExcessMeanM;

    // V2b (master plan v3), --rangeErrorNoLinkLeak. false (default): the 868 MHz ranging branch writes
    // propagationLossBiasDb = 10 n log10(estimated / true range) and Step 14 of the 868 engine adds it to the
    // link budget -- so the RANGE ERROR changes RECEPTION (a spurious random loss by default; the shadowing
    // counted twice under --rssiRangeFromShadowing: probe r80 1492 -> 1194 m). The 2.4 GHz and UWB engines
    // already zero the field before Step 14 for this reason. true: the 868 engine does the same; the
    // reported result.propagationLossBiasDb is unchanged.
    bool rangeErrorNoLinkLeak;

    // V3 (master plan v3), --rssiSigmaLogNormal. false (default): the 868 MHz range sigma reported to the
    // filter is the first-order (delta-method) d ln10 sigma_S / (10 n). Under --rssiRangeFromShadowing the
    // error is log-normal, est = d exp(s Z) with s = ln10 sigma_S / (10 n), whose exact SD is
    // d sqrt(exp(s^2) (exp(s^2) - 1)): at sigma_S = 7 dB, n = 2.3 that is 1.01 d against 0.70 d, the variance
    // ratio 2.1 measured as E[z^2] = 2.3-2.5 in the per-range log. true: report the exact SD (at d = est).
    bool rssiSigmaLogNormal;

    // C5 (scope 2026-10-17), --tofCalibrate. false (default): a 2.4 GHz range carries its preset's mean offset (wolf:
    // +75 m LoS, Wolf 2019 Table II, uncorrected; --tofFieldBiasM if set). true: the mean LoS offset a static field
    // calibration measures is subtracted -- 75 m under wolf, --tofFieldBiasM otherwise (gottschalk is unbiased,
    // muller already applies its eq. (5) correction). The spread and any NLoS excess remain.
    bool tofCalibrate;

    RangingParams()
        : enabled(true),
          tofLosFloorM (3.0),
          tofNlosFloorM(3.6),
          uwbLosFloorM (0.10),
          uwbNlosFloorM(0.25),
          tofNlosBias  (DistributionParam::Normal(1.5, 0.8,           0.0, 6.0)),
          uwbNlosBias  (DistributionParam::LogNormal(std::log(0.3f), 0.6, 0.0, 3.0)),
          uwbPreambleSymbols(1024.0),
          tofFieldBiasM(0.0),
          rssiRangeFromShadowing(false),
          tofErrorModel(0),
          nlosRangeBias(false),
          tofNlosExcessMeanM(69.0),
          rangeErrorNoLinkLeak(false),
          rssiSigmaLogNormal(false),
          tofCalibrate(false)
    {}
};

struct LoRaPhysicsConfig {
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
    ReceiverDecisionParams receiver;
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
    
    // CA-TBL ranging model selection (Phase S0a)
    // Default preserves ml_dataset_generator.cc bit-for-bit.
    RangingModelMode rangingModelMode = RANGING_SYNTHETIC_GPS_DENIED;
    RangingParams ranging;  // only used when rangingModelMode == RANGING_PHYSICS_DERIVED

    uint32_t randomSeed;
    bool verboseLogging;
    
    static LoRaPhysicsConfig ForScenario(EnvironmentClass env,
                                         WeatherState weather,
                                         FadingModelType fadingType,
                                         VegetationType vegetation,
                                         MobilityModelType mobilityModel,
                                         bool fineTimestampGateways);
    
    static LoRaPhysicsConfig FromYAML(const std::string& filepath);
    void ToYAML(const std::string& filepath) const;
};

} // namespace ns3

#endif // LORA_PHYSICS_PARAMETERS_H
