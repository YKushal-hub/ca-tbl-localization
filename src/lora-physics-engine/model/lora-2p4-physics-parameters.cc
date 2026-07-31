#include "lora-2p4-physics-parameters.h"
#include <iostream>

namespace ns3 {

ReceiverDecisionParams2p4 ReceiverDecisionParams2p4::Defaults() {
    ReceiverDecisionParams2p4 p;
    p.softThresholdEnabled = true;
    p.softThresholdWidthDb = 3.0;
    
    // SX1280 Sensitivity: SF5 to SF12, BW=800kHz baseline
    p.sensitivityDbm[0] = -100.0; // SF5
    p.sensitivityDbm[1] = -103.0; // SF6
    p.sensitivityDbm[2] = -106.0; // SF7
    p.sensitivityDbm[3] = -109.0; // SF8
    p.sensitivityDbm[4] = -112.0; // SF9
    p.sensitivityDbm[5] = -115.0; // SF10
    p.sensitivityDbm[6] = -118.0; // SF11
    p.sensitivityDbm[7] = -121.0; // SF12
    
    // CR offsets for 2.4GHz
    p.codingRateSensitivityOffsetDb[0] = 0.0;
    p.codingRateSensitivityOffsetDb[1] = -0.5;
    p.codingRateSensitivityOffsetDb[2] = -1.0;
    p.codingRateSensitivityOffsetDb[3] = -1.5;

    p.linkHysteresisMarginDb = 2.0;
    p.preambleUnlockPenaltyDb = 5.0;
    p.pllUnlockPenaltyDb = 3.0;
    p.evmPenaltyThresholdPercent = 15.0;
    p.evmPenaltyDbPerPercent = 0.2;
    p.cfoPenaltyThresholdHz = 5000.0;
    p.cfoPenaltyDbPerHz = 0.0005;
    p.crcFailurePenaltyDb = 20.0;

    return p;
}

LoRa2p4PhysicsConfig LoRa2p4PhysicsConfig::ForScenario(EnvironmentClass env,
                                                       WeatherState weather,
                                                       FadingModelType fadingType,
                                                       VegetationType vegetation,
                                                       MobilityModelType mobilityModel,
                                                       bool fineTimestampGateways) {
    LoRa2p4PhysicsConfig c;
    c.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
    c.environment = env;
    c.initialWeather = weather;
    
    // Core physics params with 2.4 GHz derivations
    c.propagation = PropagationParams::DefaultsFor(env);
    c.propagation.frequencyHz = 2.44e9; // Fixed 2.4GHz channel
    
    c.shadowing = ShadowingParams::DefaultsFor(env);
    
    c.fading = FadingParams::DefaultsFor(env);
    c.fading.model = fadingType;
    if (fadingType == FADING_NONE) {
        c.fading.enabled = false;
    }
    
    // NLOS parameters re-derived for 2.4 GHz (approx +5dB for diffraction)
    c.nlos = NLOSParams::DefaultsFor(env);
    c.nlos.frequencyHz = c.propagation.frequencyHz;
    // 2.4 GHz vs 868 MHz Diffraction (NLOS):
    // Fresnel-Kirchhoff knife-edge diffraction parameter ν ∝ √f.
    // Loss L ≈ 20*log10(ν) (for ν > 2.4), so loss scales as 10*log10(f_new / f_old).
    // 10*log10(2440/868) ≈ 4.49 dB additional diffraction loss at 2.44 GHz.
    c.nlos.nlosAttenuationDb.mean += 4.49;
    c.nlos.diffractionLossDb.mean += 4.49;

    c.atmospheric = AtmosphericParams::DefaultsFor(weather);
    
    // Foliage specific-attenuation re-derived for 2.4 GHz
    // Scaling by ~2.17 per ITU-R P.833 (Am proportional to f^0.75)
    c.foliage = FoliageParams::DefaultsFor(vegetation);
    c.foliage.specificAttenuationDbPerM.mean *= 2.17;
    
    c.interference = InterferenceParams::DefaultsFor(env);
    
    // 2.4GHz specific receiver setup
    c.receiver = ReceiverDecisionParams2p4::Defaults();
    
    if (fineTimestampGateways) {
        c.hardware = HardwareParams::DefaultsFineTimestamp();
    } else {
        c.hardware = HardwareParams::Defaults();
    }
    
    // Other tier 1, 2, 3 settings (reused as approximations)
    c.mobility = MobilityParams::DefaultsFor(mobilityModel);
    c.terrain = TerrainParams::Defaults();
    c.antenna = AntennaParams::Defaults();
    c.iqImpairment = IQImpairmentParams::Defaults();
    
    c.delaySpread = DelaySpreadParams::DefaultsFor(env);
    c.snow = SnowParams::Defaults();
    c.atmosphericPressure = AtmosphericPressureParams::Defaults();
    c.windVariation = WindVariationParams::Defaults();
    c.temperatureCfo = TemperatureCfoParams::Defaults();
    c.humidityDielectric = HumidityDielectricParams::Defaults();
    c.adr = ADRParams::Defaults();
    c.freshnelTerrain = FreshnelTerrainParams::Defaults();
    
    c.buildingEntry = BuildingEntryParams::Defaults();
    c.wallMaterial = WallMaterialParams::Defaults();
    c.floorAttenuation = FloorAttenuationParams::Defaults();
    c.nodePositionUncertainty = NodePositionUncertaintyParams::Defaults();
    c.temporalBlockage = TemporalBlockageParams::Defaults();
    c.bodyShadowing = BodyShadowingParams::Defaults();
    
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

    c.microDsp = MicroDspConfigParams::Defaults();
    c.radioFrontend = RadioFrontendConfigParams::Defaults();
    c.distributedPhy = DistributedPhyConfigParams::Defaults();
    c.distributedScalability = DistributedScalabilityConfigParams::Defaults();
    c.globalValidation = GlobalValidationConfigParams::Defaults();
    c.spectrumOccupancy = SpectrumOccupancyConfigParams::Defaults();
    c.adaptiveEnvironment = AdaptiveEnvironmentConfigParams::Defaults();
    
    c.rangingModelMode = RANGING_SYNTHETIC_GPS_DENIED;
    
    c.randomSeed = 1;
    c.verboseLogging = false;

    return c;
}

LoRa2p4PhysicsConfig LoRa2p4PhysicsConfig::FromYAML(const std::string& filepath) {
    // Dummy implementation for 2.4GHz tier
    std::cerr << "Warning: FromYAML not implemented for 2.4GHz tier. Returning defaults." << std::endl;
    return ForScenario((EnvironmentClass)0, (WeatherState)0, (FadingModelType)0, (VegetationType)0, (MobilityModelType)0, false);
}

void LoRa2p4PhysicsConfig::ToYAML(const std::string& filepath) const {
    // Dummy implementation
    std::cerr << "Warning: ToYAML not implemented for 2.4GHz tier." << std::endl;
}

} // namespace ns3
