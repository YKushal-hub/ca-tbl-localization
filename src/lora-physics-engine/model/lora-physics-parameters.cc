#include "lora-physics-parameters.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace ns3 {

PropagationParams PropagationParams::DefaultsFor(EnvironmentClass env) {
    PropagationParams p;
    p.envClass = env;
    p.referenceDistanceM = 1.0;
    p.frequencyHz = 868e6; // Default European ISM band frequency
    p.gatewayHeightM = DistributionParam::Normal(30.0, 10.0, 10.0, 50.0);
    p.nodeHeightM = DistributionParam::Normal(1.0, 0.3, 0.5, 2.0);
    
    // Feature: Dual-Slope Path Loss (Default: disabled)
    p.enableDualSlope = false;
    p.dualSlopeBreakpointM = 1000.0;
    p.pathLossExponent2 = DistributionParam::Fixed(1.58); // ADAPTED: Rademacher 2021 (arXiv 2109.07768) single-slope urban fit n=1.58, PL1km=132.41 dB; used here as the far slope beyond 1 km

    // Path loss exponent presets. OURS: 2.2 / 2.75 are not in Petajajarvi 2015 (Oulu fit n=2.65, PLd0=132.25, as quoted by Rademacher 2021)
    switch (env) {
        case ENV_RURAL_OPEN:
            p.pathLossExponent = DistributionParam::Fixed(2.2);
            break;
        case ENV_RURAL_FORESTED:
            p.pathLossExponent = DistributionParam::Normal(2.3, 0.1, 2.1, 2.6);
            p.enableDualSlope = false;
            break;
        case ENV_SUBURBAN:
            p.pathLossExponent = DistributionParam::Fixed(2.75);
            p.enableDualSlope = false;
            break;
        case ENV_URBAN_MODERATE:
            p.pathLossExponent = DistributionParam::Fixed(2.39);
            p.enableDualSlope = true;
            p.dualSlopeBreakpointM = 1000.0;
            p.pathLossExponent2 = DistributionParam::Fixed(1.58);
            break;
        case ENV_URBAN_DENSE:
            p.pathLossExponent = DistributionParam::Fixed(2.45);
            p.enableDualSlope = true;
            p.dualSlopeBreakpointM = 1000.0;
            p.pathLossExponent2 = DistributionParam::Fixed(1.58);
            break;
        case ENV_INDUSTRIAL:
            p.pathLossExponent = DistributionParam::Fixed(1.8);
            break;
        case ENV_INDOOR_OFFICE:
            p.pathLossExponent = DistributionParam::Fixed(2.8);
            break;
        case ENV_CONCRETE_BUNKER:
            p.pathLossExponent = DistributionParam::Fixed(3.5);
            break;
        case ENV_TUNNEL_CONFINED:
            p.pathLossExponent = DistributionParam::Fixed(1.3);
            break;
    }
    return p;
}

ShadowingParams ShadowingParams::DefaultsFor(EnvironmentClass env) {
    ShadowingParams s;
    s.enabled = true;
    s.spatialCorrelationEnabled = true;
    s.updateIntervalS = 0.0; // Static shadowing map for simulation duration

    double stddevDb = 8.0;
    double corrDist = 20.0;
    double nlosRatio = 1.0;

    switch (env) {
        case ENV_URBAN_DENSE:
            stddevDb = 4.0; corrDist = 10.0; nlosRatio = 1.50; break;
        case ENV_URBAN_MODERATE:
            stddevDb = 4.0; corrDist = 15.0; nlosRatio = 1.52; break;
        case ENV_SUBURBAN:
            stddevDb = 8.0; corrDist = 20.0; nlosRatio = 1.35; break;
        case ENV_RURAL_OPEN:
            stddevDb = 5.0; corrDist = 50.0; nlosRatio = 1.25; break;
        case ENV_RURAL_FORESTED:
            stddevDb = 7.0; corrDist = 30.0; nlosRatio = 1.3; break;
        case ENV_INDOOR_OFFICE:
            stddevDb = 7.0; corrDist = 5.0; nlosRatio = 1.3; break;
        case ENV_CONCRETE_BUNKER:
            stddevDb = 6.0; corrDist = 5.0; nlosRatio = 1.3; break;
        case ENV_INDUSTRIAL:
            stddevDb = 4.0; corrDist = 10.0; nlosRatio = 1.25; break;
        case ENV_TUNNEL_CONFINED:
            stddevDb = 3.0; corrDist = 5.0; nlosRatio = 1.0; break;
    }

    s.shadowingStddevDb = DistributionParam::Fixed(stddevDb);
    s.correlationDistanceM = DistributionParam::Fixed(corrDist);
    s.nlosToLosSigmaRatio = nlosRatio;
    return s;
}

FadingParams FadingParams::DefaultsFor(EnvironmentClass env) {
    FadingParams f;
    f.enabled = true;
    f.useIsolatedSeed = false;
    f.isolatedSeed = 0;
    // Log-normal coherence time: target ~500ms, stddev ~200ms
    f.coherenceTimeMs = DistributionParam::LogNormal(std::log(500.0), 0.4, 50.0, 10000.0);

    switch (env) {
        case ENV_RURAL_OPEN:
            f.model = FADING_RICIAN;
            f.ricianKFactor = DistributionParam::Normal(8.0, 2.0, 4.0, 15.0);
            f.kNlosMean = 1.0;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_RURAL_FORESTED:
            f.model = FADING_RICIAN;
            f.ricianKFactor = DistributionParam::Normal(3.0, 1.0, 1.0, 6.0);
            f.kNlosMean = 0.5;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_SUBURBAN:
            f.model = FADING_RICIAN;
            f.ricianKFactor = DistributionParam::Normal(4.0, 1.5, 1.5, 8.0);
            f.kNlosMean = 0.5;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_URBAN_MODERATE:
            f.model = FADING_RAYLEIGH;
            f.ricianKFactor = DistributionParam::Fixed(0.0);
            f.kNlosMean = 0.0;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_URBAN_DENSE:
            f.model = FADING_RAYLEIGH;
            f.ricianKFactor = DistributionParam::Fixed(0.0);
            f.kNlosMean = 0.0;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_INDOOR_OFFICE:
            f.model = FADING_RICIAN;
            f.ricianKFactor = DistributionParam::Normal(2.0, 1.0, 0.0, 5.0);
            f.kNlosMean = 0.0;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_CONCRETE_BUNKER:
            f.model = FADING_RICIAN;
            f.ricianKFactor = DistributionParam::Normal(1.0, 0.5, 0.0, 3.0);
            f.kNlosMean = 0.0;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_INDUSTRIAL:
            f.model = FADING_RICIAN;
            f.ricianKFactor = DistributionParam::Normal(6.0, 2.0, 2.0, 10.0);
            f.kNlosMean = 1.0;
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
        case ENV_TUNNEL_CONFINED:
            f.model = FADING_RICIAN;
            f.ricianKFactor = DistributionParam::Normal(10.0, 3.0, 6.0, 20.0);
            f.kNlosMean = 0.0; 
            f.nakagamiM = DistributionParam::Fixed(1.0);
            break;
    }
    return f;
}

NLOSParams NLOSParams::DefaultsFor(EnvironmentClass env) {
    NLOSParams n;
    n.enabled = true;
    n.envClass = env;
    n.frequencyHz = 868e6;
    n.defaultNumberOfFloors = 1;
    n.buildingPenetrationDb = DistributionParam::Normal(12.0, 3.0, 8.0, 20.0);
    n.floorLossDbPerFloor = DistributionParam::Normal(2.5, 0.5, 1.5, 4.0);

    double scale = 200.0;
    double atten = 15.0;
    double sigma = 7.0; // Default sigma
    double diff = 6.0;

    switch (env) {
        case ENV_URBAN_DENSE:
            scale = 200.0; atten = 16.0; sigma = 14.0; diff = 12.0; break;
        case ENV_URBAN_MODERATE:
            scale = 300.0; atten = 17.0; sigma = 12.0; diff = 8.0; break;
        case ENV_SUBURBAN:
            scale = 8000.0; atten = 13.0; sigma = 9.0; diff = 6.0; break;
        case ENV_RURAL_OPEN:
            scale = 54000.0; atten = 6.0; sigma = 5.0; diff = 4.0; break;
        case ENV_RURAL_FORESTED:
            scale = 8000.0; atten = 10.0; sigma = 7.0; diff = 5.0; break;
        case ENV_INDOOR_OFFICE:
            scale = 50.0; atten = 2.0; sigma = 6.0; diff = 10.0; break;
        case ENV_CONCRETE_BUNKER:
            scale = 30.0; atten = 18.0; sigma = 8.0; diff = 2.0; break;
        case ENV_INDUSTRIAL:
            scale = 100.0; atten = 2.0; sigma = 5.0; diff = 8.0; break;
        case ENV_TUNNEL_CONFINED:
            scale = 30.0; atten = 18.0; sigma = 8.0; diff = 2.0; break;
    }

    if (env == ENV_TUNNEL_CONFINED) {
        n.nlosScaleDistanceM = DistributionParam::Fixed(1e9);
        n.nlosAttenuationDb = DistributionParam::Fixed(0.0);
    } else {
        n.nlosScaleDistanceM = DistributionParam::Fixed(scale);
        n.nlosAttenuationDb = DistributionParam::Normal(
            atten, sigma, std::max(0.0, atten - 3.0*sigma), atten + 3.0*sigma);
    }
    n.diffractionLossDb = DistributionParam::Normal(diff, 3.0, std::max(0.0, diff - 5.0), diff + 8.0);
    return n;
}

AtmosphericParams AtmosphericParams::DefaultsFor(WeatherState state) {
    AtmosphericParams a;
    a.enabled = true;
    a.initialWeatherState = state;
    a.weatherUpdateIntervalS = 3600.0;
    a.rainAttenuationK = 0.000065;      // ITU-R P.838-3 coefficient for 868 MHz
    a.rainAttenuationAlpha = 0.7;       // ITU-R P.838-3 coefficient for 868 MHz

    // Initialize feature extensions with their defaults
    a.atmosphericPressure = AtmosphericPressureParams::Defaults();
    a.snow = SnowParams::Defaults();
    a.humidityDielectric = HumidityDielectricParams::Defaults();

    switch (state) {
        case WEATHER_CLEAR_DRY:
            a.rainRateMmPerHr = DistributionParam::Fixed(0.0);
            a.temperatureCelsius = DistributionParam::Normal(25.0, 8.0, -10.0, 45.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(8.0, 3.0, 1.0, 20.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            break;
        case WEATHER_HUMID_HOT:
            a.rainRateMmPerHr = DistributionParam::Fixed(0.0);
            a.temperatureCelsius = DistributionParam::Normal(38.0, 5.0, 20.0, 50.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(20.0, 4.0, 10.0, 35.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            break;
        case WEATHER_LIGHT_RAIN:
            a.rainRateMmPerHr = DistributionParam::LogNormal(std::log(5.0), 0.5, 0.1, 15.0);
            a.temperatureCelsius = DistributionParam::Normal(18.0, 5.0, 0.0, 35.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(15.0, 4.0, 5.0, 25.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            break;
        case WEATHER_HEAVY_RAIN:
            a.rainRateMmPerHr = DistributionParam::LogNormal(std::log(50.0), 0.4, 15.0, 150.0);
            a.temperatureCelsius = DistributionParam::Normal(15.0, 5.0, 0.0, 30.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(18.0, 4.0, 5.0, 28.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            break;
        case WEATHER_FOG:
            a.rainRateMmPerHr = DistributionParam::Fixed(0.0);
            a.temperatureCelsius = DistributionParam::Normal(12.0, 4.0, -2.0, 25.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(12.0, 3.0, 5.0, 20.0);
            a.fogAttenuationDbPerKm = DistributionParam::LogNormal(std::log(0.5), 0.5, 0.05, 3.0);
            break;
        case WEATHER_WINDY_DRY:
            a.rainRateMmPerHr = DistributionParam::Fixed(0.0);
            a.temperatureCelsius = DistributionParam::Normal(22.0, 8.0, -5.0, 40.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(6.0, 2.0, 0.5, 15.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            break;
        case WEATHER_WINDY_WET:
            a.rainRateMmPerHr = DistributionParam::LogNormal(std::log(15.0), 0.5, 1.0, 80.0);
            a.temperatureCelsius = DistributionParam::Normal(16.0, 5.0, 0.0, 30.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(16.0, 4.0, 5.0, 25.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            break;
        case WEATHER_HEAVY_WET_SNOW:
            a.rainRateMmPerHr = DistributionParam::Fixed(0.0);
            a.temperatureCelsius = DistributionParam::Normal(0.0, 2.0, -5.0, 5.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(5.0, 2.0, 1.0, 10.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            a.snow.enabled = true;
            a.snow.snowfallRateMmHrEquiv = DistributionParam::Normal(10.0, 3.0, 5.0, 20.0);
            break;
        case WEATHER_LIGHT_SNOW:
            a.rainRateMmPerHr = DistributionParam::Fixed(0.0);
            a.temperatureCelsius = DistributionParam::Normal(-2.0, 3.0, -10.0, 2.0);
            a.waterVaporDensityGPerM3 = DistributionParam::Normal(4.0, 1.5, 0.5, 8.0);
            a.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
            a.snow.enabled = true;
            a.snow.snowfallRateMmHrEquiv = DistributionParam::Normal(2.0, 1.0, 0.5, 5.0);
            break;
    }
    return a;
}

FoliageParams FoliageParams::DefaultsFor(VegetationType type) {
    FoliageParams f;
    f.enabled = true;
    f.type = type;
    f.vegetationDepthM = DistributionParam::LogNormal(std::log(100.0), 0.6, 0.0, 500.0);
    f.seasonalMultiplier = DistributionParam::Normal(0.75, 0.1, 0.3, 1.0);
    f.wetFoliageAddDb = DistributionParam::Normal(2.0, 1.0, 0.0, 6.0);

    double gamma = 0.0;
    double amax = 0.0;

    switch (type) {
        case VEG_NONE:
            f.enabled = false;
            gamma = 0.0; amax = 0.0; break;
        case VEG_SPARSE_SHRUBS:
            gamma = 0.04; amax = 8.0; break;
        case VEG_LIGHT_FOREST:
            gamma = 0.10; amax = 15.0; break;
        case VEG_DENSE_FOREST:
            gamma = 0.20; amax = 25.0; break;
        case VEG_TROPICAL_FOREST:
            gamma = 0.30; amax = 35.0; break;
        case VEG_CROP_FIELD:
            gamma = 0.06; amax = 10.0; break;
        case VEG_ORCHARD:
            gamma = 0.08; amax = 12.0; break;
    }

    f.specificAttenuationDbPerM = DistributionParam::Normal(gamma, gamma * 0.25, gamma * 0.2, gamma * 1.8);
    f.maxAttenuationDb = DistributionParam::Normal(amax, amax * 0.2, amax * 0.5, amax * 2.0);
    return f;
}

InterferenceParams InterferenceParams::DefaultsFor(EnvironmentClass env) {
    InterferenceParams i;
    i.enabled = true;
    i.captureEffectEnabled = true;
    i.selfInterferenceEnabled = true;
    i.selfInterferenceWindowMs = 800.0;

    double captureMatrix[6][6] = {
        {   6.0, -16.0, -20.0, -25.0, -28.0, -32.0 },
        { -24.0,   6.0, -16.0, -20.0, -25.0, -28.0 },
        { -27.0, -24.0,   6.0, -16.0, -20.0, -25.0 },
        { -30.0, -27.0, -24.0,   6.0, -16.0, -20.0 },
        { -33.0, -30.0, -27.0, -24.0,   6.0, -16.0 },
        { -36.0, -33.0, -30.0, -27.0, -24.0,   6.0 }
    };
    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 6; ++c) {
            i.captureThresholdDb[r][c] = captureMatrix[r][c];
        }
    }

    // Inter-SF Isolation Matrix from Croce et al. 2018 (Table II)
    // rows are receiver SF7..SF12, columns are interferer SF7..SF12
    double isolation[6][6] = {
        {  6.0, 16.0, 17.0, 18.0, 19.0, 20.0}, // SF7
        { 24.0,  6.0, 20.0, 22.0, 23.0, 24.0}, // SF8
        { 27.0, 28.0,  6.0, 24.0, 25.0, 26.0}, // SF9
        { 30.0, 31.0, 32.0,  6.0, 28.0, 29.0}, // SF10
        { 32.0, 33.0, 34.0, 35.0,  6.0, 31.0}, // SF11
        { 33.0, 34.0, 35.0, 36.0, 37.0,  6.0}  // SF12
    };

    for (int r = 0; r < 6; ++r) {
        for (int c = 0; c < 6; ++c) {
            i.sfIsolationDb[r][c] = isolation[r][c];
        }
    }

    double dens = 0.3;
    switch (env) {
        case ENV_RURAL_OPEN:
        case ENV_RURAL_FORESTED:
            dens = 0.05; break;
        case ENV_SUBURBAN:
            dens = 0.3; break;
        case ENV_URBAN_MODERATE:
        case ENV_INDUSTRIAL:
            dens = 1.0; break;
        case ENV_URBAN_DENSE:
            dens = 3.0; break;
        case ENV_TUNNEL_CONFINED:
        case ENV_CONCRETE_BUNKER:
        case ENV_INDOOR_OFFICE:
            dens = 0.1; break;
    }

    i.externalInterfererDensityPerKm2 = DistributionParam::LogNormal(std::log(dens), 0.5, 0.001, dens * 5.0);
    i.externalTxPowerDbm = DistributionParam::Normal(14.0, 3.0, 10.0, 27.0);
    i.externalDutyCycle = DistributionParam::LogNormal(std::log(0.01), 0.5, 0.001, 0.05);
    i.adjacentChannelLeakageDb = DistributionParam::Normal(30.0, 5.0, 20.0, 40.0);
    return i;
}

ReceiverDecisionParams ReceiverDecisionParams::Defaults() {
    ReceiverDecisionParams r;
    r.softThresholdEnabled = true;
    r.softThresholdWidthDb = 0.35;

    const double sensitivities[6] = {-123.0, -126.0, -129.0, -132.0, -134.5, -137.0};
    for (int sf = 0; sf < 6; ++sf) {
        r.sensitivityDbm[sf] = sensitivities[sf];
    }

    const double codingRateOffsets[4] = {0.0, -1.0, -2.0, -3.0};
    for (int cr = 0; cr < 4; ++cr) {
        r.codingRateSensitivityOffsetDb[cr] = codingRateOffsets[cr];
    }

    r.linkHysteresisMarginDb = 1.5;
    r.preambleUnlockPenaltyDb = 5.0;
    r.pllUnlockPenaltyDb = 4.0;
    r.evmPenaltyThresholdPercent = 10.0;
    r.evmPenaltyDbPerPercent = 0.2;
    r.cfoPenaltyThresholdHz = 1000.0;
    r.cfoPenaltyDbPerHz = 0.002;
    r.crcFailurePenaltyDb = 6.0;
    return r;
}

HardwareParams HardwareParams::Defaults() {
    HardwareParams h;
    h.enabled = true;
    h.perNodeVariationEnabled = true;
    h.carrierFrequencyOffsetHz = DistributionParam::Normal(0.0, 2000.0, -8680.0, 8680.0);
    h.phaseNoiseRad = DistributionParam::Normal(0.08, 0.03, 0.01, 0.20);
    h.timestampJitterNs = DistributionParam::LogNormal(std::log(10000.0), 0.5, 500.0, 50000.0);
    h.timestampQuantizationNs = DistributionParam::Uniform(500.0, 2000.0);
    h.rssiResolutionDb = 1.0; // Semtech SX1276 fixed RSSI step
    h.rssiHardwareVariationDb = DistributionParam::Normal(0.0, 2.0, -5.0, 5.0);
    h.sensitivityVariationDb = DistributionParam::Normal(0.0, 1.0, -3.0, 3.0);
    h.txPowerAccuracyDb = DistributionParam::Normal(0.0, 0.8, -2.0, 2.0);
    h.fineTimestampMode = false;
    h.codingRateDenom = 5; // Default CR 4/5
    return h;
}

HardwareParams HardwareParams::DefaultsFineTimestamp() {
    HardwareParams h = HardwareParams::Defaults();
    h.timestampJitterNs = DistributionParam::Normal(100.0, 50.0, 50.0, 500.0);
    h.timestampQuantizationNs = DistributionParam::Uniform(50.0, 200.0);
    h.fineTimestampMode = true;
    return h;
}

MobilityParams MobilityParams::DefaultsFor(MobilityModelType model) {
    MobilityParams m;
    m.enabled = true;
    m.mobilityModel = model;
    m.directionUpdateIntervalS = 30.0;
    m.dopplerDegradationEnabled = true;
    m.dopplerSNRPenaltyDbPerHz = DistributionParam::Normal(0.05, 0.02, 0.01, 0.10);
    m.nodeDistribution = DIST_UNIFORM_DISK;
    m.nClusters = 3;
    m.clusterRadiusM = DistributionParam::Normal(1500.0, 500.0, 500.0, 3000.0);
    m.nodesPerCluster = DistributionParam::Normal(50.0, 15.0, 5.0, 200.0);

    switch (model) {
        case MOB_STATIC:
            m.velocityMps = DistributionParam::Fixed(0.0);
            break;
        case MOB_RANDOM_WALK:
            m.velocityMps = DistributionParam::LogNormal(std::log(0.5), 0.5, 0.0, 2.0);
            break;
        case MOB_RANDOM_WAYPOINT:
            m.velocityMps = DistributionParam::Normal(2.0, 1.0, 0.5, 8.0);
            break;
        case MOB_CONSTANT_VELOCITY:
            m.velocityMps = DistributionParam::Normal(10.0, 3.0, 5.0, 20.0);
            break;
        case MOB_ROAD_CONSTRAINED:
            m.velocityMps = DistributionParam::Normal(8.0, 4.0, 1.0, 25.0);
            break;
    }
    return m;
}

TerrainParams TerrainParams::Defaults() {
    TerrainParams t;
    t.enabled = true;
    t.gatewayHeightOptionsM = {10.0, 20.0, 30.0, 50.0};
    t.placement = GW_MIXED;
    t.terrainIrregularityM = DistributionParam::Normal(15.0, 10.0, 0.0, 100.0);
    t.losScaleDistanceM = DistributionParam::Normal(1000.0, 300.0, 100.0, 5000.0);
    t.fresnelClearanceFraction = 0.6;
    // 868 MHz on every tier: preserves the pre-existing hardcode exactly.
    t.frequencyHz = 868e6;
    t.effectiveEarthRadiusFactor = 4.0 / 3.0; // Standard atmospheric refraction
    return t;
}

// Feature 7: Dynamic delay spread parameters
DelaySpreadParams DelaySpreadParams::DefaultsFor(EnvironmentClass env) {
    DelaySpreadParams d;
    d.enabled = true;
    // OURS (nanoseconds): the cited 'Lim et al. 2018' source was not found, so these are engineering values
    switch (env) {
        case ENV_RURAL_OPEN:
            d.losDelaySpreadNs = DistributionParam::Normal(50.0, 20.0, 10.0, 150.0);
            d.nlosDelaySpreadNs = DistributionParam::Normal(120.0, 40.0, 50.0, 300.0);
            break;
        case ENV_SUBURBAN:
            d.losDelaySpreadNs = DistributionParam::Normal(100.0, 40.0, 30.0, 250.0);
            d.nlosDelaySpreadNs = DistributionParam::Normal(200.0, 80.0, 80.0, 500.0);
            break;
        case ENV_URBAN_MODERATE:
            d.losDelaySpreadNs = DistributionParam::Normal(150.0, 50.0, 50.0, 400.0);
            d.nlosDelaySpreadNs = DistributionParam::Normal(480.0, 100.0, 100.0, 900.0);
            break;
        case ENV_URBAN_DENSE:
            d.losDelaySpreadNs = DistributionParam::Normal(200.0, 70.0, 70.0, 450.0);
            d.nlosDelaySpreadNs = DistributionParam::Normal(380.0, 120.0, 150.0, 800.0);
            break;
        case ENV_RURAL_FORESTED:
            // OURS: LOS forest 80 ns mean, NLOS 300 ns mean (the cited Lim 2018 Table III was not found)
            d.losDelaySpreadNs  = DistributionParam::Normal(80.0,  30.0, 20.0,  200.0);
            d.nlosDelaySpreadNs = DistributionParam::Normal(300.0, 100.0, 100.0, 700.0);
            break;
        case ENV_TUNNEL_CONFINED:
        case ENV_CONCRETE_BUNKER:
        case ENV_INDOOR_OFFICE:
        case ENV_INDUSTRIAL:
            d.losDelaySpreadNs = DistributionParam::Normal(300.0, 100.0, 100.0, 700.0);
            d.nlosDelaySpreadNs = DistributionParam::Normal(500.0, 150.0, 200.0, 1000.0);
            break;
        default:
            d.losDelaySpreadNs = DistributionParam::Normal(100.0, 40.0, 10.0, 300.0);
            d.nlosDelaySpreadNs = DistributionParam::Normal(200.0, 80.0, 50.0, 600.0);
    }
    d.coherenceBandwidthScaleFactor = DistributionParam::Fixed(1.0);
    return d;
}

// Feature 4: Snow precipitation parameters
SnowParams SnowParams::Defaults() {
    SnowParams s;
    s.enabled = true;
    // Light snow: dry, T < -5C
    s.snowfallRateMmHrEquiv = DistributionParam::Normal(3.0, 1.0, 1.0, 5.0);
    s.snowTemperatureCelsius = DistributionParam::Normal(-10.0, 5.0, -25.0, -5.0);
    s.snowNLOSScaleFactor = 0.6;  // 30-50% scale reduction from snow bulk on vegetation
    return s;
}

// Feature 3: Atmospheric pressure extension
AtmosphericPressureParams AtmosphericPressureParams::Defaults() {
    AtmosphericPressureParams a;
    a.enableTroposphericRefraction = false;  // Default off for backward compatibility
    // Standard atmosphere: mean 1013 hPa, stddev 15 hPa
    a.atmosphericPressureHPa = DistributionParam::Normal(1013.0, 15.0, 960.0, 1050.0);
    return a;
}

// Feature 5: Wind-induced foliage variation
WindVariationParams WindVariationParams::Defaults() {
    WindVariationParams w;
    w.windVariationEnabled = false;  // Default off for backward compatibility
    w.windSpeedMps = DistributionParam::Normal(3.0, 2.0, 0.0, 20.0);
    w.windAmplitudeDb = DistributionParam::Normal(2.0, 0.5, 0.0, 6.0);
    return w;
}

// Feature 6: Temperature-dependent CFO
TemperatureCfoParams TemperatureCfoParams::Defaults() {
    TemperatureCfoParams t;
    // OURS: ~1-2 Hz/°C. Robyns 2017 is a fingerprinting paper and shows no CFO-temperature coefficient
    t.temperatureCoefficientHzPerC = DistributionParam::Normal(1.5, 0.5, 0.5, 3.0);
    t.jitterTempSensitivity = DistributionParam::Normal(0.005, 0.002, 0.001, 0.02);
    t.referenceTemperatureC = 25.0;
    return t;
}

// Feature 8: Humidity dielectric effect
HumidityDielectricParams HumidityDielectricParams::Defaults() {
    HumidityDielectricParams h;
    h.enableHumidityDielectric = false;  // Default off for backward compatibility
    return h;
}

// Feature 9: ADR parameters
ADRParams ADRParams::Defaults() {
    ADRParams a;
    a.adrEnabled = false;  // Default off for backward compatibility
    a.adrSnrMarginThresholdDb = 10.0;
    return a;
}

// Feature 2: Fresnel terrain extension
FreshnelTerrainParams FreshnelTerrainParams::Defaults() {
    FreshnelTerrainParams f;
    f.freshnelCalculationEnabled = true;
    f.frequencyHz = 868e6;  // 868 MHz default
    return f;
}

AntennaParams AntennaParams::Defaults() {
    AntennaParams a;
    a.peakGainDbi = 2.15; // standard dipole
    a.beamwidthDeg = 65.0;
    a.frontToBackRatioDb = 20.0;
    a.downtiltDeg = 0.0;
    a.polarizationMismatchLossDb = 0.0;
    a.directional = false;
    return a;
}

AntennaParams AntennaParams::DirectionalSector() {
    AntennaParams a;
    a.peakGainDbi = 14.0; // 3GPP directional sector
    a.beamwidthDeg = 65.0;
    a.frontToBackRatioDb = 25.0;
    a.downtiltDeg = 6.0;
    a.polarizationMismatchLossDb = 0.5;
    a.directional = true;
    return a;
}

IQImpairmentParams IQImpairmentParams::Defaults() {
    IQImpairmentParams i;
    i.enabled = true;
    i.iqAmplitudeImbalanceDb = DistributionParam::Normal(0.5, 0.2, 0.0, 3.0);
    i.iqPhaseMismatchDeg = DistributionParam::Normal(1.5, 0.5, 0.0, 10.0);
    i.dcOffsetDb = DistributionParam::Normal(-45.0, 5.0, -60.0, -20.0);
    i.adcEffectiveBits = DistributionParam::Normal(10.0, 0.5, 8.0, 14.0);
    i.adcClippingThresholdDbm = DistributionParam::Normal(-15.0, 2.0, -30.0, 0.0);
    i.degradeRxPowerOnlyWhenClipped = false;
    return i;
}

LoRaPhysicsConfig LoRaPhysicsConfig::ForScenario(EnvironmentClass env,
                                                 WeatherState weather,
                                                 FadingModelType fadingType,
                                                 VegetationType vegetation,
                                                 MobilityModelType mobilityModel,
                                                 bool fineTimestampGateways) {
    LoRaPhysicsConfig c;
    c.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL; // Default to fast statistical model
    c.environment = env;
    c.initialWeather = weather;
    
    c.propagation = PropagationParams::DefaultsFor(env);
    c.shadowing = ShadowingParams::DefaultsFor(env);
    c.fading = FadingParams::DefaultsFor(env);
    c.fading.model = fadingType;
    if (fadingType == FADING_NONE) {
        c.fading.enabled = false;
    }
    
    c.nlos = NLOSParams::DefaultsFor(env);
    c.nlos.frequencyHz = c.propagation.frequencyHz;
    c.atmospheric = AtmosphericParams::DefaultsFor(weather);
    c.foliage = FoliageParams::DefaultsFor(vegetation);
    c.interference = InterferenceParams::DefaultsFor(env);
    c.receiver = ReceiverDecisionParams::Defaults();
    
    if (fineTimestampGateways) {
        c.hardware = HardwareParams::DefaultsFineTimestamp();
    } else {
        c.hardware = HardwareParams::Defaults();
    }
    
    c.mobility = MobilityParams::DefaultsFor(mobilityModel);
    c.terrain = TerrainParams::Defaults();
    c.antenna = AntennaParams::Defaults();
    c.iqImpairment = IQImpairmentParams::Defaults();
    
    // Feature extension parameters initialization
    c.delaySpread = DelaySpreadParams::DefaultsFor(env);
    c.snow = SnowParams::Defaults();
    c.atmosphericPressure = AtmosphericPressureParams::Defaults();
    c.windVariation = WindVariationParams::Defaults();
    c.temperatureCfo = TemperatureCfoParams::Defaults();
    c.humidityDielectric = HumidityDielectricParams::Defaults();
    c.adr = ADRParams::Defaults();
    c.freshnelTerrain = FreshnelTerrainParams::Defaults();
    c.freshnelTerrain.frequencyHz = c.propagation.frequencyHz;  // Use propagation frequency
    
    // Tier 2 parameters initialization
    c.buildingEntry = BuildingEntryParams::Defaults();
    c.wallMaterial = WallMaterialParams::Defaults();
    c.floorAttenuation = FloorAttenuationParams::Defaults();
    c.nodePositionUncertainty = NodePositionUncertaintyParams::Defaults();
    c.temporalBlockage = TemporalBlockageParams::Defaults();
    c.bodyShadowing = BodyShadowingParams::Defaults();
    
    // Tier 3 parameters initialization
    c.tunnel = TunnelParams::Defaults();
    c.twoRayGround = TwoRayGroundParams::Defaults();
    c.soilAttenuation = SoilAttenuationParams::Defaults();
    
    c.baseband = BasebandConfigParams::Defaults();
    c.syncLoop = SyncLoopConfigParams::Defaults();
    c.crossLayer = CrossLayerConfigParams::Defaults();
    c.sdrValidation = SdrValidationConfigParams::Defaults();
    
    c.iqStream = IQStreamConfigParams::Defaults();
    c.dspChain = DspChainConfigParams::Defaults();
    c.pll = PllConfigParams::Defaults();
    c.symbolSync = SymbolSyncConfigParams::Defaults();
    c.scalability = ScalabilityConfigParams::Defaults();
    c.advancedValidation = AdvancedValidationConfigParams::Defaults();
    c.rfNonlinearity = RFNonlinearityConfigParams::Defaults();
    c.cooperativePhy = CooperativePhyConfigParams::Defaults();
    c.geographicMemory = GeographicMemoryConfigParams::Defaults();
    
    // Stage 6 parameters
    c.microDsp = MicroDspConfigParams::Defaults();
    c.radioFrontend = RadioFrontendConfigParams::Defaults();
    c.distributedPhy = DistributedPhyConfigParams::Defaults();
    c.distributedScalability = DistributedScalabilityConfigParams::Defaults();
    c.globalValidation = GlobalValidationConfigParams::Defaults();
    c.spectrumOccupancy = SpectrumOccupancyConfigParams::Defaults();
    c.adaptiveEnvironment = AdaptiveEnvironmentConfigParams::Defaults();
    
    switch(env) {
        case ENV_INDUSTRIAL:
            c.temporalBlockage.enabled = true;
            c.temporalBlockage.blockageLossDb = DistributionParam::Fixed(15.0);
            break;
        case ENV_INDOOR_OFFICE:
            c.buildingEntry.enabled = true;
            c.wallMaterial.enabled = true;
            c.floorAttenuation.lossPerFloorDb = DistributionParam::Fixed(7.0);
            c.temporalBlockage.enabled = false;
            break;
        case ENV_CONCRETE_BUNKER:
            c.buildingEntry.enabled = true;
            c.wallMaterial.enabled = true;
            c.temporalBlockage.enabled = false;
            break;
        default:
            c.temporalBlockage.enabled = false;
            break;
    }
    
    c.randomSeed = 42;
    c.verboseLogging = false;
    
    return c;
}

static DistributionParam::DistributionType ParseType(const std::string& s) {
    if (s == "DETERMINISTIC" || s == "0") return DistributionParam::DETERMINISTIC;
    if (s == "UNIFORM" || s == "1") return DistributionParam::UNIFORM;
    if (s == "NORMAL" || s == "2") return DistributionParam::NORMAL;
    if (s == "LOGNORMAL" || s == "3") return DistributionParam::LOGNORMAL;
    if (s == "EXPONENTIAL" || s == "4") return DistributionParam::EXPONENTIAL;
    if (s == "NAKAGAMI" || s == "5") return DistributionParam::NAKAGAMI;
    return DistributionParam::DETERMINISTIC;
}

static std::string TypeToString(DistributionParam::DistributionType t) {
    switch (t) {
        case DistributionParam::DETERMINISTIC: return "DETERMINISTIC";
        case DistributionParam::UNIFORM: return "UNIFORM";
        case DistributionParam::NORMAL: return "NORMAL";
        case DistributionParam::LOGNORMAL: return "LOGNORMAL";
        case DistributionParam::EXPONENTIAL: return "EXPONENTIAL";
        case DistributionParam::NAKAGAMI: return "NAKAGAMI";
    }
    return "DETERMINISTIC";
}

static void SetDistParamField(DistributionParam& p, const std::string& suffix, const std::string& val) {
    if (suffix == "type") p.type = ParseType(val);
    else if (suffix == "min") p.min = std::stod(val);
    else if (suffix == "max") p.max = std::stod(val);
    else if (suffix == "mean") p.mean = std::stod(val);
    else if (suffix == "stddev") p.stddev = std::stod(val);
    else if (suffix == "lambda") p.lambda = std::stod(val);
    else if (suffix == "shape") p.shape = std::stod(val);
}

static void WriteDistParamToYAML(std::ostream& os, const std::string& indent, const std::string& name, const DistributionParam& p) {
    os << indent << name << "_type: " << TypeToString(p.type) << "\n";
    os << indent << name << "_min: " << p.min << "\n";
    os << indent << name << "_max: " << p.max << "\n";
    os << indent << name << "_mean: " << p.mean << "\n";
    os << indent << name << "_stddev: " << p.stddev << "\n";
    os << indent << name << "_lambda: " << p.lambda << "\n";
    os << indent << name << "_shape: " << p.shape << "\n";
}

LoRaPhysicsConfig LoRaPhysicsConfig::FromYAML(const std::string& filepath) {
    LoRaPhysicsConfig c = ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open YAML config: " << filepath << ". Using defaults." << std::endl;
        return c;
    }

    std::string line;
    std::string current_section = "";
    while (std::getline(file, line)) {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) continue;
        size_t last = line.find_last_not_of(" \t\r\n");
        line = line.substr(first, (last - first + 1));
        if (line.empty()) continue;
        if (line.back() == ':') {
            current_section = line.substr(0, line.length() - 1);
            continue;
        }
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string val = line.substr(colon_pos + 1);
            size_t k_f = key.find_first_not_of(" \t");
            size_t k_l = key.find_last_not_of(" \t");
            if (k_f != std::string::npos) key = key.substr(k_f, k_l - k_f + 1);
            size_t v_f = val.find_first_not_of(" \t");
            size_t v_l = val.find_last_not_of(" \t");
            if (v_f != std::string::npos) val = val.substr(v_f, v_l - v_f + 1);
            
            if (current_section == "general") {
                if (key == "verboseLogging") c.verboseLogging = (val == "true" || val == "1");
                else if (key == "randomSeed") c.randomSeed = std::stoul(val);
                else if (key == "fidelityMode") c.fidelityMode = static_cast<SimulationFidelityMode>(std::stoi(val));
                else if (key == "environment") c.environment = static_cast<EnvironmentClass>(std::stoi(val));
                else if (key == "initialWeather") c.initialWeather = static_cast<WeatherState>(std::stoi(val));
            } else if (current_section == "propagation") {
                if (key == "frequencyHz") c.propagation.frequencyHz = std::stod(val);
                else if (key == "referenceDistanceM") c.propagation.referenceDistanceM = std::stod(val);
                else if (key.find("pathLossExponent_") == 0) SetDistParamField(c.propagation.pathLossExponent, key.substr(17), val);
                else if (key.find("gatewayHeightM_") == 0) SetDistParamField(c.propagation.gatewayHeightM, key.substr(15), val);
                else if (key.find("nodeHeightM_") == 0) SetDistParamField(c.propagation.nodeHeightM, key.substr(12), val);
            } else if (current_section == "shadowing") {
                if (key == "enabled") c.shadowing.enabled = (val == "true" || val == "1");
                else if (key == "spatialCorrelationEnabled") c.shadowing.spatialCorrelationEnabled = (val == "true" || val == "1");
                else if (key == "updateIntervalS") c.shadowing.updateIntervalS = std::stod(val);
                else if (key.find("shadowingStddevDb_") == 0) SetDistParamField(c.shadowing.shadowingStddevDb, key.substr(18), val);
                else if (key.find("correlationDistanceM_") == 0) SetDistParamField(c.shadowing.correlationDistanceM, key.substr(20), val);
            } else if (current_section == "fading") {
                if (key == "enabled") c.fading.enabled = (val == "true" || val == "1");
                else if (key == "model") c.fading.model = static_cast<FadingModelType>(std::stoi(val));
                else if (key.find("ricianKFactor_") == 0) SetDistParamField(c.fading.ricianKFactor, key.substr(14), val);
                else if (key.find("nakagamiM_") == 0) SetDistParamField(c.fading.nakagamiM, key.substr(10), val);
                else if (key.find("coherenceTimeMs_") == 0) SetDistParamField(c.fading.coherenceTimeMs, key.substr(16), val);
            } else if (current_section == "nlos") {
                if (key == "enabled") c.nlos.enabled = (val == "true" || val == "1");
                else if (key == "defaultNumberOfFloors") c.nlos.defaultNumberOfFloors = std::stoi(val);
                else if (key.find("nlosScaleDistanceM_") == 0) SetDistParamField(c.nlos.nlosScaleDistanceM, key.substr(19), val);
                else if (key.find("nlosAttenuationDb_") == 0) SetDistParamField(c.nlos.nlosAttenuationDb, key.substr(18), val);
                else if (key.find("diffractionLossDb_") == 0) SetDistParamField(c.nlos.diffractionLossDb, key.substr(18), val);
                else if (key.find("buildingPenetrationDb_") == 0) SetDistParamField(c.nlos.buildingPenetrationDb, key.substr(22), val);
                else if (key.find("floorLossDbPerFloor_") == 0) SetDistParamField(c.nlos.floorLossDbPerFloor, key.substr(20), val);
            } else if (current_section == "atmospheric") {
                if (key == "enabled") c.atmospheric.enabled = (val == "true" || val == "1");
                else if (key == "rainAttenuationK") c.atmospheric.rainAttenuationK = std::stod(val);
                else if (key == "rainAttenuationAlpha") c.atmospheric.rainAttenuationAlpha = std::stod(val);
                else if (key.find("rainRateMmPerHr_") == 0) SetDistParamField(c.atmospheric.rainRateMmPerHr, key.substr(16), val);
                else if (key.find("waterVaporDensityGPerM3_") == 0) SetDistParamField(c.atmospheric.waterVaporDensityGPerM3, key.substr(24), val);
                else if (key.find("temperatureCelsius_") == 0) SetDistParamField(c.atmospheric.temperatureCelsius, key.substr(19), val);
                else if (key.find("fogAttenuationDbPerKm_") == 0) SetDistParamField(c.atmospheric.fogAttenuationDbPerKm, key.substr(22), val);
            } else if (current_section == "foliage") {
                if (key == "enabled") c.foliage.enabled = (val == "true" || val == "1");
                else if (key == "type") c.foliage.type = static_cast<VegetationType>(std::stoi(val));
                else if (key.find("specificAttenuationDbPerM_") == 0) SetDistParamField(c.foliage.specificAttenuationDbPerM, key.substr(26), val);
                else if (key.find("maxAttenuationDb_") == 0) SetDistParamField(c.foliage.maxAttenuationDb, key.substr(17), val);
                else if (key.find("vegetationDepthM_") == 0) SetDistParamField(c.foliage.vegetationDepthM, key.substr(17), val);
                else if (key.find("seasonalMultiplier_") == 0) SetDistParamField(c.foliage.seasonalMultiplier, key.substr(19), val);
                else if (key.find("wetFoliageAddDb_") == 0) SetDistParamField(c.foliage.wetFoliageAddDb, key.substr(16), val);
            } else if (current_section == "interference") {
                if (key == "enabled") c.interference.enabled = (val == "true" || val == "1");
                else if (key == "captureEffectEnabled") c.interference.captureEffectEnabled = (val == "true" || val == "1");
                else if (key == "selfInterferenceEnabled") c.interference.selfInterferenceEnabled = (val == "true" || val == "1");
                else if (key == "selfInterferenceWindowMs") c.interference.selfInterferenceWindowMs = std::stod(val);
                else if (key.rfind("captureThresholdSF", 0) == 0 && key.size() >= 20) {
                    // Ignored: Capture threshold is now a 6x6 matrix initialized by default
                }
                else if (key.find("externalInterfererDensityPerKm2_") == 0) SetDistParamField(c.interference.externalInterfererDensityPerKm2, key.substr(32), val);
                else if (key.find("externalTxPowerDbm_") == 0) SetDistParamField(c.interference.externalTxPowerDbm, key.substr(19), val);
                else if (key.find("externalDutyCycle_") == 0) SetDistParamField(c.interference.externalDutyCycle, key.substr(18), val);
                else if (key.find("adjacentChannelLeakageDb_") == 0) SetDistParamField(c.interference.adjacentChannelLeakageDb, key.substr(25), val);
            } else if (current_section == "receiver") {
                if (key == "softThresholdEnabled") c.receiver.softThresholdEnabled = (val == "true" || val == "1");
                else if (key == "softThresholdWidthDb") c.receiver.softThresholdWidthDb = std::stod(val);
                else if (key == "linkHysteresisMarginDb") c.receiver.linkHysteresisMarginDb = std::stod(val);
                else if (key == "preambleUnlockPenaltyDb") c.receiver.preambleUnlockPenaltyDb = std::stod(val);
                else if (key == "pllUnlockPenaltyDb") c.receiver.pllUnlockPenaltyDb = std::stod(val);
                else if (key == "evmPenaltyThresholdPercent") c.receiver.evmPenaltyThresholdPercent = std::stod(val);
                else if (key == "evmPenaltyDbPerPercent") c.receiver.evmPenaltyDbPerPercent = std::stod(val);
                else if (key == "cfoPenaltyThresholdHz") c.receiver.cfoPenaltyThresholdHz = std::stod(val);
                else if (key == "cfoPenaltyDbPerHz") c.receiver.cfoPenaltyDbPerHz = std::stod(val);
                else if (key == "crcFailurePenaltyDb") c.receiver.crcFailurePenaltyDb = std::stod(val);
                else if (key.rfind("sensitivitySF", 0) == 0 && key.size() >= 17) {
                    int sf = std::stoi(key.substr(13, key.size() - 16));
                    if (sf >= 7 && sf <= 12) c.receiver.sensitivityDbm[sf - 7] = std::stod(val);
                } else if (key.rfind("codingRateOffsetCr4", 0) == 0 && key.size() >= 22) {
                    int denom = std::stoi(key.substr(19, key.size() - 21));
                    if (denom >= 5 && denom <= 8) c.receiver.codingRateSensitivityOffsetDb[denom - 5] = std::stod(val);
                }
            } else if (current_section == "hardware") {
                if (key == "enabled") c.hardware.enabled = (val == "true" || val == "1");
                else if (key == "perNodeVariationEnabled") c.hardware.perNodeVariationEnabled = (val == "true" || val == "1");
                else if (key == "codingRateDenom") c.hardware.codingRateDenom = std::stoul(val);
                else if (key == "fineTimestampMode") c.hardware.fineTimestampMode = (val == "true" || val == "1");
                else if (key == "rssiResolutionDb") c.hardware.rssiResolutionDb = std::stod(val);
                else if (key.find("carrierFrequencyOffsetHz_") == 0) SetDistParamField(c.hardware.carrierFrequencyOffsetHz, key.substr(25), val);
                else if (key.find("phaseNoiseRad_") == 0) SetDistParamField(c.hardware.phaseNoiseRad, key.substr(14), val);
                else if (key.find("timestampJitterNs_") == 0) SetDistParamField(c.hardware.timestampJitterNs, key.substr(18), val);
                else if (key.find("timestampQuantizationNs_") == 0) SetDistParamField(c.hardware.timestampQuantizationNs, key.substr(24), val);
                else if (key.find("rssiHardwareVariationDb_") == 0) SetDistParamField(c.hardware.rssiHardwareVariationDb, key.substr(24), val);
                else if (key.find("sensitivityVariationDb_") == 0) SetDistParamField(c.hardware.sensitivityVariationDb, key.substr(23), val);
                else if (key.find("txPowerAccuracyDb_") == 0) SetDistParamField(c.hardware.txPowerAccuracyDb, key.substr(18), val);
            } else if (current_section == "mobility") {
                if (key == "enabled") c.mobility.enabled = (val == "true" || val == "1");
                else if (key == "mobilityModel") c.mobility.mobilityModel = static_cast<MobilityModelType>(std::stoi(val));
                else if (key == "directionUpdateIntervalS") c.mobility.directionUpdateIntervalS = std::stod(val);
                else if (key == "dopplerDegradationEnabled") c.mobility.dopplerDegradationEnabled = (val == "true" || val == "1");
                else if (key == "nodeDistribution") c.mobility.nodeDistribution = static_cast<NodeDistributionType>(std::stoi(val));
                else if (key == "nClusters") c.mobility.nClusters = std::stoi(val);
                else if (key.find("velocityMps_") == 0) SetDistParamField(c.mobility.velocityMps, key.substr(12), val);
                else if (key.find("dopplerSNRPenaltyDbPerHz_") == 0) SetDistParamField(c.mobility.dopplerSNRPenaltyDbPerHz, key.substr(25), val);
                else if (key.find("clusterRadiusM_") == 0) SetDistParamField(c.mobility.clusterRadiusM, key.substr(15), val);
                else if (key.find("nodesPerCluster_") == 0) SetDistParamField(c.mobility.nodesPerCluster, key.substr(16), val);
            } else if (current_section == "terrain") {
                if (key == "enabled") c.terrain.enabled = (val == "true" || val == "1");
                else if (key == "fresnelClearanceFraction") c.terrain.fresnelClearanceFraction = std::stod(val);
                else if (key == "effectiveEarthRadiusFactor") c.terrain.effectiveEarthRadiusFactor = std::stod(val);
                else if (key.find("terrainIrregularityM_") == 0) SetDistParamField(c.terrain.terrainIrregularityM, key.substr(21), val);
                else if (key.find("losScaleDistanceM_") == 0) SetDistParamField(c.terrain.losScaleDistanceM, key.substr(18), val);
            } else if (current_section == "antenna") {
                if (key == "directional") c.antenna.directional = (val == "true" || val == "1");
                else if (key == "peakGainDbi") c.antenna.peakGainDbi = std::stod(val);
                else if (key == "downtiltDeg") c.antenna.downtiltDeg = std::stod(val);
                else if (key == "beamwidthDeg") c.antenna.beamwidthDeg = std::stod(val);
                else if (key == "frontToBackRatioDb") c.antenna.frontToBackRatioDb = std::stod(val);
                else if (key == "polarizationMismatchLossDb") c.antenna.polarizationMismatchLossDb = std::stod(val);
            } else if (current_section == "iq_impairment") {
                if (key == "enabled") c.iqImpairment.enabled = (val == "true" || val == "1");
                else if (key.find("iqAmplitudeImbalanceDb_") == 0) SetDistParamField(c.iqImpairment.iqAmplitudeImbalanceDb, key.substr(23), val);
                else if (key.find("iqPhaseMismatchDeg_") == 0) SetDistParamField(c.iqImpairment.iqPhaseMismatchDeg, key.substr(19), val);
                else if (key.find("dcOffsetDb_") == 0) SetDistParamField(c.iqImpairment.dcOffsetDb, key.substr(11), val);
                else if (key.find("adcEffectiveBits_") == 0) SetDistParamField(c.iqImpairment.adcEffectiveBits, key.substr(17), val);
                else if (key.find("adcClippingThresholdDbm_") == 0) SetDistParamField(c.iqImpairment.adcClippingThresholdDbm, key.substr(24), val);
            } else if (current_section == "baseband") {
                if (key == "enabled") c.baseband.enabled = (val == "true" || val == "1");
                else if (key == "chirpPhaseTracking") c.baseband.chirpPhaseTracking = (val == "true" || val == "1");
                else if (key == "dirichletSpectralLeakage") c.baseband.dirichletSpectralLeakage = (val == "true" || val == "1");
            } else if (current_section == "sync_loop") {
                if (key == "enabled") c.syncLoop.enabled = (val == "true" || val == "1");
                else if (key == "agcSettlingSpeed") c.syncLoop.agcSettlingSpeed = std::stod(val);
            } else if (current_section == "cross_layer") {
                if (key == "enabled") c.crossLayer.enabled = (val == "true" || val == "1");
                else if (key == "adrAdaptationSpeed") c.crossLayer.adrAdaptationSpeed = std::stod(val);
            } else if (current_section == "sdr_validation") {
                if (key == "enabled") c.sdrValidation.enabled = (val == "true" || val == "1");
                else if (key == "traceLogPath") c.sdrValidation.traceLogPath = val;
            } else if (current_section == "iq_stream") {
                if (key == "enabled") c.iqStream.enabled = (val == "true" || val == "1");
                else if (key == "sampleRateHz") c.iqStream.sampleRateHz = std::stod(val);
            } else if (current_section == "dsp_chain") {
                if (key == "enabled") c.dspChain.enabled = (val == "true" || val == "1");
                else if (key == "agcThresholdDbm") c.dspChain.agcThresholdDbm = std::stod(val);
            } else if (current_section == "pll") {
                if (key == "enabled") c.pll.enabled = (val == "true" || val == "1");
                else if (key == "loopBandwidthHz") c.pll.loopBandwidthHz = std::stod(val);
                else if (key == "dampingFactor") c.pll.dampingFactor = std::stod(val);
            } else if (current_section == "symbol_sync") {
                if (key == "enabled") c.symbolSync.enabled = (val == "true" || val == "1");
                else if (key == "timingGateFraction") c.symbolSync.timingGateFraction = std::stod(val);
            } else if (current_section == "scalability") {
                if (key == "enabled") c.scalability.enabled = (val == "true" || val == "1");
                else if (key == "spatialTileResolutionM") c.scalability.spatialTileResolutionM = std::stod(val);
                else if (key == "activeLinkThreshold") c.scalability.activeLinkThreshold = std::stoul(val);
            } else if (current_section == "advanced_validation") {
                if (key == "enabled") c.advancedValidation.enabled = (val == "true" || val == "1");
                else if (key == "datasetPath") c.advancedValidation.datasetPath = val;
            } else if (current_section == "rf_nonlinearity") {
                if (key == "enabled") c.rfNonlinearity.enabled = (val == "true" || val == "1");
            } else if (current_section == "cooperative_phy") {
                if (key == "enabled") c.cooperativePhy.enabled = (val == "true" || val == "1");
                else if (key == "maximalRatioCombining") c.cooperativePhy.maximalRatioCombining = (val == "true" || val == "1");
            } else if (current_section == "geographic_memory") {
                if (key == "enabled") c.geographicMemory.enabled = (val == "true" || val == "1");
                else if (key == "memoryCellSizeM") c.geographicMemory.memoryCellSizeM = std::stod(val);
            } else if (current_section == "micro_dsp") {
                if (key == "enabled") c.microDsp.enabled = (val == "true" || val == "1");
                else if (key == "activationSnrThresholdDb") c.microDsp.activationSnrThresholdDb = std::stod(val);
            } else if (current_section == "radio_frontend") {
                if (key == "enabled") c.radioFrontend.enabled = (val == "true" || val == "1");
                else if (key == "oscillatorAgingDriftHzPerYear") c.radioFrontend.oscillatorAgingDriftHzPerYear = std::stod(val);
            } else if (current_section == "spectrum_occupancy") {
                if (key == "enabled") c.spectrumOccupancy.enabled = (val == "true" || val == "1");
                else if (key == "backgroundIsmNoiseFloorDbm") c.spectrumOccupancy.backgroundIsmNoiseFloorDbm = std::stod(val);
            } else if (current_section == "distributed_phy") {
                if (key == "enabled") c.distributedPhy.enabled = (val == "true" || val == "1");
            } else if (current_section == "distributed_scalability") {
                if (key == "enabled") c.distributedScalability.enabled = (val == "true" || val == "1");
                else if (key == "gpuBatchingEnabled") c.distributedScalability.gpuBatchingEnabled = (val == "true" || val == "1");
            } else if (current_section == "global_validation") {
                if (key == "enabled") c.globalValidation.enabled = (val == "true" || val == "1");
            } else if (current_section == "adaptive_environment") {
                if (key == "enabled") c.adaptiveEnvironment.enabled = (val == "true" || val == "1");
                else if (key == "learningRate") c.adaptiveEnvironment.learningRate = std::stod(val);
            } else if (current_section == "delay_spread") {
                if (key == "enabled") c.delaySpread.enabled = (val == "true" || val == "1");
                else if (key.find("losDelaySpreadNs_") == 0) SetDistParamField(c.delaySpread.losDelaySpreadNs, key.substr(17), val);
                else if (key.find("nlosDelaySpreadNs_") == 0) SetDistParamField(c.delaySpread.nlosDelaySpreadNs, key.substr(18), val);
                else if (key.find("coherenceBandwidthScaleFactor_") == 0) SetDistParamField(c.delaySpread.coherenceBandwidthScaleFactor, key.substr(30), val);
            } else if (current_section == "snow") {
                if (key == "enabled") c.snow.enabled = (val == "true" || val == "1");
                else if (key == "snowNLOSScaleFactor") c.snow.snowNLOSScaleFactor = std::stod(val);
                else if (key.find("snowfallRateMmHrEquiv_") == 0) SetDistParamField(c.snow.snowfallRateMmHrEquiv, key.substr(22), val);
                else if (key.find("snowTemperatureCelsius_") == 0) SetDistParamField(c.snow.snowTemperatureCelsius, key.substr(23), val);
            } else if (current_section == "atmospheric_pressure") {
                if (key == "enableTroposphericRefraction") c.atmosphericPressure.enableTroposphericRefraction = (val == "true" || val == "1");
                else if (key.find("atmosphericPressureHPa_") == 0) SetDistParamField(c.atmosphericPressure.atmosphericPressureHPa, key.substr(23), val);
            } else if (current_section == "wind_variation") {
                if (key == "windVariationEnabled") c.windVariation.windVariationEnabled = (val == "true" || val == "1");
                else if (key.find("windSpeedMps_") == 0) SetDistParamField(c.windVariation.windSpeedMps, key.substr(13), val);
                else if (key.find("windAmplitudeDb_") == 0) SetDistParamField(c.windVariation.windAmplitudeDb, key.substr(16), val);
            } else if (current_section == "temperature_cfo") {
                if (key == "referenceTemperatureC") c.temperatureCfo.referenceTemperatureC = std::stod(val);
                else if (key.find("temperatureCoefficientHzPerC_") == 0) SetDistParamField(c.temperatureCfo.temperatureCoefficientHzPerC, key.substr(29), val);
                else if (key.find("jitterTempSensitivity_") == 0) SetDistParamField(c.temperatureCfo.jitterTempSensitivity, key.substr(21), val);
            } else if (current_section == "humidity_dielectric") {
                if (key == "enableHumidityDielectric") c.humidityDielectric.enableHumidityDielectric = (val == "true" || val == "1");
            } else if (current_section == "adr") {
                if (key == "adrEnabled") c.adr.adrEnabled = (val == "true" || val == "1");
                else if (key == "adrSnrMarginThresholdDb") c.adr.adrSnrMarginThresholdDb = std::stod(val);
            } else if (current_section == "fresnel_terrain") {
                if (key == "freshnelCalculationEnabled") c.freshnelTerrain.freshnelCalculationEnabled = (val == "true" || val == "1");
                else if (key == "frequencyHz") c.freshnelTerrain.frequencyHz = std::stod(val);
            }
        }
    }
    file.close();
    return c;
}

void LoRaPhysicsConfig::ToYAML(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not write YAML config to " << filepath << std::endl;
        return;
    }
    file << "# LoRa Physics Engine Auto-generated Configuration\n";
    file << "general:\n";
    file << "  fidelityMode: " << (int)fidelityMode << "\n";
    file << "  verboseLogging: " << (verboseLogging ? "true" : "false") << "\n";
    file << "  randomSeed: " << randomSeed << "\n";
    file << "  environment: " << (int)environment << "\n";
    file << "  initialWeather: " << (int)initialWeather << "\n";
    file << "propagation:\n";
    file << "  frequencyHz: " << propagation.frequencyHz << "\n";
    file << "  referenceDistanceM: " << propagation.referenceDistanceM << "\n";
    WriteDistParamToYAML(file, "  ", "pathLossExponent", propagation.pathLossExponent);
    WriteDistParamToYAML(file, "  ", "gatewayHeightM", propagation.gatewayHeightM);
    WriteDistParamToYAML(file, "  ", "nodeHeightM", propagation.nodeHeightM);
    file << "shadowing:\n";
    file << "  enabled: " << (shadowing.enabled ? "true" : "false") << "\n";
    file << "  spatialCorrelationEnabled: " << (shadowing.spatialCorrelationEnabled ? "true" : "false") << "\n";
    file << "  updateIntervalS: " << shadowing.updateIntervalS << "\n";
    WriteDistParamToYAML(file, "  ", "shadowingStddevDb", shadowing.shadowingStddevDb);
    WriteDistParamToYAML(file, "  ", "correlationDistanceM", shadowing.correlationDistanceM);
    file << "fading:\n";
    file << "  enabled: " << (fading.enabled ? "true" : "false") << "\n";
    file << "  model: " << (int)fading.model << "\n";
    WriteDistParamToYAML(file, "  ", "ricianKFactor", fading.ricianKFactor);
    WriteDistParamToYAML(file, "  ", "nakagamiM", fading.nakagamiM);
    WriteDistParamToYAML(file, "  ", "coherenceTimeMs", fading.coherenceTimeMs);
    file << "nlos:\n";
    file << "  enabled: " << (nlos.enabled ? "true" : "false") << "\n";
    file << "  defaultNumberOfFloors: " << nlos.defaultNumberOfFloors << "\n";
    WriteDistParamToYAML(file, "  ", "nlosScaleDistanceM", nlos.nlosScaleDistanceM);
    WriteDistParamToYAML(file, "  ", "nlosAttenuationDb", nlos.nlosAttenuationDb);
    WriteDistParamToYAML(file, "  ", "diffractionLossDb", nlos.diffractionLossDb);
    WriteDistParamToYAML(file, "  ", "buildingPenetrationDb", nlos.buildingPenetrationDb);
    WriteDistParamToYAML(file, "  ", "floorLossDbPerFloor", nlos.floorLossDbPerFloor);
    file << "atmospheric:\n";
    file << "  enabled: " << (atmospheric.enabled ? "true" : "false") << "\n";
    file << "  rainAttenuationK: " << atmospheric.rainAttenuationK << "\n";
    file << "  rainAttenuationAlpha: " << atmospheric.rainAttenuationAlpha << "\n";
    WriteDistParamToYAML(file, "  ", "rainRateMmPerHr", atmospheric.rainRateMmPerHr);
    WriteDistParamToYAML(file, "  ", "waterVaporDensityGPerM3", atmospheric.waterVaporDensityGPerM3);
    WriteDistParamToYAML(file, "  ", "temperatureCelsius", atmospheric.temperatureCelsius);
    WriteDistParamToYAML(file, "  ", "fogAttenuationDbPerKm", atmospheric.fogAttenuationDbPerKm);
    file << "foliage:\n";
    file << "  enabled: " << (foliage.enabled ? "true" : "false") << "\n";
    file << "  type: " << (int)foliage.type << "\n";
    WriteDistParamToYAML(file, "  ", "specificAttenuationDbPerM", foliage.specificAttenuationDbPerM);
    WriteDistParamToYAML(file, "  ", "maxAttenuationDb", foliage.maxAttenuationDb);
    WriteDistParamToYAML(file, "  ", "vegetationDepthM", foliage.vegetationDepthM);
    WriteDistParamToYAML(file, "  ", "seasonalMultiplier", foliage.seasonalMultiplier);
    WriteDistParamToYAML(file, "  ", "wetFoliageAddDb", foliage.wetFoliageAddDb);
    file << "interference:\n";
    file << "  enabled: " << (interference.enabled ? "true" : "false") << "\n";
    file << "  captureEffectEnabled: " << (interference.captureEffectEnabled ? "true" : "false") << "\n";
    file << "  selfInterferenceEnabled: " << (interference.selfInterferenceEnabled ? "true" : "false") << "\n";
    file << "  selfInterferenceWindowMs: " << interference.selfInterferenceWindowMs << "\n";
    for (int sf = 0; sf < 6; ++sf) {
        file << "  captureThresholdSF" << (sf + 7) << "Db: " << interference.captureThresholdDb[sf] << "\n";
    }
    WriteDistParamToYAML(file, "  ", "externalInterfererDensityPerKm2", interference.externalInterfererDensityPerKm2);
    WriteDistParamToYAML(file, "  ", "externalTxPowerDbm", interference.externalTxPowerDbm);
    WriteDistParamToYAML(file, "  ", "externalDutyCycle", interference.externalDutyCycle);
    WriteDistParamToYAML(file, "  ", "adjacentChannelLeakageDb", interference.adjacentChannelLeakageDb);
    file << "receiver:\n";
    file << "  softThresholdEnabled: " << (receiver.softThresholdEnabled ? "true" : "false") << "\n";
    file << "  softThresholdWidthDb: " << receiver.softThresholdWidthDb << "\n";
    for (int sf = 0; sf < 6; ++sf) {
        file << "  sensitivitySF" << (sf + 7) << "Dbm: " << receiver.sensitivityDbm[sf] << "\n";
    }
    for (int cr = 0; cr < 4; ++cr) {
        file << "  codingRateOffsetCr4" << (cr + 5) << "Db: " << receiver.codingRateSensitivityOffsetDb[cr] << "\n";
    }
    file << "  linkHysteresisMarginDb: " << receiver.linkHysteresisMarginDb << "\n";
    file << "  preambleUnlockPenaltyDb: " << receiver.preambleUnlockPenaltyDb << "\n";
    file << "  pllUnlockPenaltyDb: " << receiver.pllUnlockPenaltyDb << "\n";
    file << "  evmPenaltyThresholdPercent: " << receiver.evmPenaltyThresholdPercent << "\n";
    file << "  evmPenaltyDbPerPercent: " << receiver.evmPenaltyDbPerPercent << "\n";
    file << "  cfoPenaltyThresholdHz: " << receiver.cfoPenaltyThresholdHz << "\n";
    file << "  cfoPenaltyDbPerHz: " << receiver.cfoPenaltyDbPerHz << "\n";
    file << "  crcFailurePenaltyDb: " << receiver.crcFailurePenaltyDb << "\n";
    file << "hardware:\n";
    file << "  enabled: " << (hardware.enabled ? "true" : "false") << "\n";
    file << "  perNodeVariationEnabled: " << (hardware.perNodeVariationEnabled ? "true" : "false") << "\n";
    file << "  fineTimestampMode: " << (hardware.fineTimestampMode ? "true" : "false") << "\n";
    file << "  codingRateDenom: " << (int)hardware.codingRateDenom << "\n";
    file << "  rssiResolutionDb: " << hardware.rssiResolutionDb << "\n";
    WriteDistParamToYAML(file, "  ", "carrierFrequencyOffsetHz", hardware.carrierFrequencyOffsetHz);
    WriteDistParamToYAML(file, "  ", "phaseNoiseRad", hardware.phaseNoiseRad);
    WriteDistParamToYAML(file, "  ", "timestampJitterNs", hardware.timestampJitterNs);
    WriteDistParamToYAML(file, "  ", "timestampQuantizationNs", hardware.timestampQuantizationNs);
    WriteDistParamToYAML(file, "  ", "rssiHardwareVariationDb", hardware.rssiHardwareVariationDb);
    WriteDistParamToYAML(file, "  ", "sensitivityVariationDb", hardware.sensitivityVariationDb);
    WriteDistParamToYAML(file, "  ", "txPowerAccuracyDb", hardware.txPowerAccuracyDb);
    file << "mobility:\n";
    file << "  enabled: " << (mobility.enabled ? "true" : "false") << "\n";
    file << "  mobilityModel: " << (int)mobility.mobilityModel << "\n";
    file << "  directionUpdateIntervalS: " << mobility.directionUpdateIntervalS << "\n";
    file << "  dopplerDegradationEnabled: " << (mobility.dopplerDegradationEnabled ? "true" : "false") << "\n";
    file << "  nodeDistribution: " << (int)mobility.nodeDistribution << "\n";
    file << "  nClusters: " << mobility.nClusters << "\n";
    WriteDistParamToYAML(file, "  ", "velocityMps", mobility.velocityMps);
    WriteDistParamToYAML(file, "  ", "dopplerSNRPenaltyDbPerHz", mobility.dopplerSNRPenaltyDbPerHz);
    WriteDistParamToYAML(file, "  ", "clusterRadiusM", mobility.clusterRadiusM);
    WriteDistParamToYAML(file, "  ", "nodesPerCluster", mobility.nodesPerCluster);
    file << "terrain:\n";
    file << "  enabled: " << (terrain.enabled ? "true" : "false") << "\n";
    file << "  fresnelClearanceFraction: " << terrain.fresnelClearanceFraction << "\n";
    file << "  effectiveEarthRadiusFactor: " << terrain.effectiveEarthRadiusFactor << "\n";
    WriteDistParamToYAML(file, "  ", "terrainIrregularityM", terrain.terrainIrregularityM);
    WriteDistParamToYAML(file, "  ", "losScaleDistanceM", terrain.losScaleDistanceM);
    file << "antenna:\n";
    file << "  directional: " << (antenna.directional ? "true" : "false") << "\n";
    file << "  peakGainDbi: " << antenna.peakGainDbi << "\n";
    file << "  downtiltDeg: " << antenna.downtiltDeg << "\n";
    file << "  beamwidthDeg: " << antenna.beamwidthDeg << "\n";
    file << "  frontToBackRatioDb: " << antenna.frontToBackRatioDb << "\n";
    file << "  polarizationMismatchLossDb: " << antenna.polarizationMismatchLossDb << "\n";
    file << "iq_impairment:\n";
    file << "  enabled: " << (iqImpairment.enabled ? "true" : "false") << "\n";
    WriteDistParamToYAML(file, "  ", "iqAmplitudeImbalanceDb", iqImpairment.iqAmplitudeImbalanceDb);
    WriteDistParamToYAML(file, "  ", "iqPhaseMismatchDeg", iqImpairment.iqPhaseMismatchDeg);
    WriteDistParamToYAML(file, "  ", "dcOffsetDb", iqImpairment.dcOffsetDb);
    WriteDistParamToYAML(file, "  ", "adcEffectiveBits", iqImpairment.adcEffectiveBits);
    WriteDistParamToYAML(file, "  ", "adcClippingThresholdDbm", iqImpairment.adcClippingThresholdDbm);
    file << "baseband:\n";
    file << "  enabled: " << (baseband.enabled ? "true" : "false") << "\n";
    file << "  chirpPhaseTracking: " << (baseband.chirpPhaseTracking ? "true" : "false") << "\n";
    file << "  dirichletSpectralLeakage: " << (baseband.dirichletSpectralLeakage ? "true" : "false") << "\n";
    file << "sync_loop:\n";
    file << "  enabled: " << (syncLoop.enabled ? "true" : "false") << "\n";
    file << "  agcSettlingSpeed: " << syncLoop.agcSettlingSpeed << "\n";
    file << "cross_layer:\n";
    file << "  enabled: " << (crossLayer.enabled ? "true" : "false") << "\n";
    file << "  adrAdaptationSpeed: " << crossLayer.adrAdaptationSpeed << "\n";
    file << "sdr_validation:\n";
    file << "  enabled: " << (sdrValidation.enabled ? "true" : "false") << "\n";
    file << "  traceLogPath: " << sdrValidation.traceLogPath << "\n";
    file << "iq_stream:\n";
    file << "  enabled: " << (iqStream.enabled ? "true" : "false") << "\n";
    file << "  sampleRateHz: " << iqStream.sampleRateHz << "\n";
    file << "dsp_chain:\n";
    file << "  enabled: " << (dspChain.enabled ? "true" : "false") << "\n";
    file << "  agcThresholdDbm: " << dspChain.agcThresholdDbm << "\n";
    file << "pll:\n";
    file << "  enabled: " << (pll.enabled ? "true" : "false") << "\n";
    file << "  loopBandwidthHz: " << pll.loopBandwidthHz << "\n";
    file << "  dampingFactor: " << pll.dampingFactor << "\n";
    file << "symbol_sync:\n";
    file << "  enabled: " << (symbolSync.enabled ? "true" : "false") << "\n";
    file << "  timingGateFraction: " << symbolSync.timingGateFraction << "\n";
    file << "scalability:\n";
    file << "  enabled: " << (scalability.enabled ? "true" : "false") << "\n";
    file << "  spatialTileResolutionM: " << scalability.spatialTileResolutionM << "\n";
    file << "  activeLinkThreshold: " << scalability.activeLinkThreshold << "\n";
    file << "advanced_validation:\n";
    file << "  enabled: " << (advancedValidation.enabled ? "true" : "false") << "\n";
    file << "  datasetPath: " << advancedValidation.datasetPath << "\n";
    file << "rf_nonlinearity:\n";
    file << "  enabled: " << (rfNonlinearity.enabled ? "true" : "false") << "\n";
    file << "cooperative_phy:\n";
    file << "  enabled: " << (cooperativePhy.enabled ? "true" : "false") << "\n";
    file << "  maximalRatioCombining: " << (cooperativePhy.maximalRatioCombining ? "true" : "false") << "\n";
    file << "geographic_memory:\n";
    file << "  enabled: " << (geographicMemory.enabled ? "true" : "false") << "\n";
    file << "  memoryCellSizeM: " << geographicMemory.memoryCellSizeM << "\n";
    file << "micro_dsp:\n";
    file << "  enabled: " << (microDsp.enabled ? "true" : "false") << "\n";
    file << "  activationSnrThresholdDb: " << microDsp.activationSnrThresholdDb << "\n";
    file << "radio_frontend:\n";
    file << "  enabled: " << (radioFrontend.enabled ? "true" : "false") << "\n";
    file << "  oscillatorAgingDriftHzPerYear: " << radioFrontend.oscillatorAgingDriftHzPerYear << "\n";
    file << "spectrum_occupancy:\n";
    file << "  enabled: " << (spectrumOccupancy.enabled ? "true" : "false") << "\n";
    file << "  backgroundIsmNoiseFloorDbm: " << spectrumOccupancy.backgroundIsmNoiseFloorDbm << "\n";
    file << "distributed_phy:\n";
    file << "  enabled: " << (distributedPhy.enabled ? "true" : "false") << "\n";
    file << "distributed_scalability:\n";
    file << "  enabled: " << (distributedScalability.enabled ? "true" : "false") << "\n";
    file << "  gpuBatchingEnabled: " << (distributedScalability.gpuBatchingEnabled ? "true" : "false") << "\n";
    file << "global_validation:\n";
    file << "  enabled: " << (globalValidation.enabled ? "true" : "false") << "\n";
    file << "adaptive_environment:\n";
    file << "  enabled: " << (adaptiveEnvironment.enabled ? "true" : "false") << "\n";
    file << "  learningRate: " << adaptiveEnvironment.learningRate << "\n";
    file << "delay_spread:\n";
    file << "  enabled: " << (delaySpread.enabled ? "true" : "false") << "\n";
    WriteDistParamToYAML(file, "  ", "losDelaySpreadNs", delaySpread.losDelaySpreadNs);
    WriteDistParamToYAML(file, "  ", "nlosDelaySpreadNs", delaySpread.nlosDelaySpreadNs);
    WriteDistParamToYAML(file, "  ", "coherenceBandwidthScaleFactor", delaySpread.coherenceBandwidthScaleFactor);
    file << "snow:\n";
    file << "  enabled: " << (snow.enabled ? "true" : "false") << "\n";
    file << "  snowNLOSScaleFactor: " << snow.snowNLOSScaleFactor << "\n";
    WriteDistParamToYAML(file, "  ", "snowfallRateMmHrEquiv", snow.snowfallRateMmHrEquiv);
    WriteDistParamToYAML(file, "  ", "snowTemperatureCelsius", snow.snowTemperatureCelsius);
    file << "atmospheric_pressure:\n";
    file << "  enableTroposphericRefraction: " << (atmosphericPressure.enableTroposphericRefraction ? "true" : "false") << "\n";
    WriteDistParamToYAML(file, "  ", "atmosphericPressureHPa", atmosphericPressure.atmosphericPressureHPa);
    file << "wind_variation:\n";
    file << "  windVariationEnabled: " << (windVariation.windVariationEnabled ? "true" : "false") << "\n";
    WriteDistParamToYAML(file, "  ", "windSpeedMps", windVariation.windSpeedMps);
    WriteDistParamToYAML(file, "  ", "windAmplitudeDb", windVariation.windAmplitudeDb);
    file << "temperature_cfo:\n";
    file << "  referenceTemperatureC: " << temperatureCfo.referenceTemperatureC << "\n";
    WriteDistParamToYAML(file, "  ", "temperatureCoefficientHzPerC", temperatureCfo.temperatureCoefficientHzPerC);
    WriteDistParamToYAML(file, "  ", "jitterTempSensitivity", temperatureCfo.jitterTempSensitivity);
    file << "humidity_dielectric:\n";
    file << "  enableHumidityDielectric: " << (humidityDielectric.enableHumidityDielectric ? "true" : "false") << "\n";
    file << "adr:\n";
    file << "  adrEnabled: " << (adr.adrEnabled ? "true" : "false") << "\n";
    file << "  adrSnrMarginThresholdDb: " << adr.adrSnrMarginThresholdDb << "\n";
    file << "fresnel_terrain:\n";
    file << "  freshnelCalculationEnabled: " << (freshnelTerrain.freshnelCalculationEnabled ? "true" : "false") << "\n";
    file << "  frequencyHz: " << freshnelTerrain.frequencyHz << "\n";
}

BuildingEntryParams BuildingEntryParams::Defaults() {
    BuildingEntryParams p;
    p.enabled = true;
    return p;
}

WallMaterialParams WallMaterialParams::Defaults() {
    WallMaterialParams p;
    p.enabled = true;
    p.glassLossDb = DistributionParam::Normal(3.0, 1.0, 1.0, 5.0);
    p.woodLossDb = DistributionParam::Normal(7.0, 2.0, 3.0, 12.0);
    p.concreteLossDb = DistributionParam::Normal(15.0, 3.0, 8.0, 25.0);
    p.metalLossDb = DistributionParam::Normal(25.0, 4.0, 15.0, 40.0);
    return p;
}

FloorAttenuationParams FloorAttenuationParams::Defaults() {
    FloorAttenuationParams p;
    p.enabled = true;
    // ITU-R P.1238: 7-18 dB per floor (office buildings)
    // Normal(12, 3, 5, 18): mean 12 dB, max 18 dB/floor
    // At 5 floors max: 5 × 18 = 90 dB upper bound (was 5 × 30 = 150 dB — P5 fix)
    p.lossPerFloorDb = DistributionParam::Normal(12.0, 3.0, 5.0, 18.0);
    return p;
}

NodePositionUncertaintyParams NodePositionUncertaintyParams::Defaults() {
    NodePositionUncertaintyParams p;
    p.enabled = true;
    p.horizontalErrorM = DistributionParam::Normal(5.0, 2.0, 0.0, 15.0);
    p.verticalErrorM = DistributionParam::Normal(3.0, 1.0, 0.0, 10.0);
    return p;
}

TemporalBlockageParams TemporalBlockageParams::Defaults() {
    TemporalBlockageParams p;
    p.enabled = true;
    p.blockageDurationS = DistributionParam::Normal(2.0, 0.5, 0.5, 5.0);
    p.blockageLossDb = DistributionParam::Normal(5.0, 2.0, 1.0, 15.0);
    return p;
}

BodyShadowingParams BodyShadowingParams::Defaults() {
    BodyShadowingParams p;
    p.enabled = true;
    p.nodeIsBodyWorn = false;
    p.bodyLossDb = DistributionParam::Normal(4.0, 2.0, 0.0, 10.0);
    return p;
}

TunnelParams TunnelParams::Defaults() {
    TunnelParams p;
    p.enabled = true;
    p.waveguideLossPerKmDb = DistributionParam::Normal(5.0, 2.0, 1.0, 15.0);
    return p;
}

TwoRayGroundParams TwoRayGroundParams::Defaults() {
    TwoRayGroundParams p;
    p.enabled = true;
    p.defaultRelativePermittivity = 15.0; // Typical average soil
    return p;
}

SoilAttenuationParams SoilAttenuationParams::Defaults() {
    SoilAttenuationParams p;
    p.enabled = true;
    p.burialDepthM = DistributionParam::Uniform(0.1, 0.5);
    p.soilMoisturePercent = DistributionParam::Normal(15.0, 5.0, 0.0, 40.0);
    return p;
}

} // namespace ns3
