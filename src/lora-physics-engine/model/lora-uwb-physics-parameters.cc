#include "lora-uwb-physics-parameters.h"
#include <iostream>

namespace ns3 {

ReceiverDecisionParamsUWB ReceiverDecisionParamsUWB::Defaults() {
    ReceiverDecisionParamsUWB p;
    p.softThresholdEnabled = true;
    p.softThresholdWidthDb = 3.0;
    
    // UWB (DW3000-class) receiver sensitivity: one flat value for every index -- UWB has no
    // spreading factor. LABEL CORRECTED 2026-10-15: this line was a copy of the SX1280 caption.
    // The value is NOT checked against a DW3000 data sheet (gate G19 reports tier 3 NOT CHECKED).
    p.sensitivityDbm[0] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    p.sensitivityDbm[1] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    p.sensitivityDbm[2] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    p.sensitivityDbm[3] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    p.sensitivityDbm[4] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    p.sensitivityDbm[5] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    p.sensitivityDbm[6] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    p.sensitivityDbm[7] = -104.0; // UWB Flat Sensitivity (-104 dBm)
    
    // CR offsets for UWB
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

LoRaUWBPhysicsConfig LoRaUWBPhysicsConfig::ForScenario(EnvironmentClass env,
                                                       WeatherState weather,
                                                       FadingModelType fadingType,
                                                       VegetationType vegetation,
                                                       MobilityModelType mobilityModel,
                                                       bool fineTimestampGateways) {
    LoRaUWBPhysicsConfig c;
    c.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
    c.environment = env;
    c.initialWeather = weather;
    
    // Core physics params with UWB derivations
    c.propagation = PropagationParams::DefaultsFor(env);
    c.propagation.frequencyHz = 7.99e9; // DW3000 Channel 9
    
    c.shadowing = ShadowingParams::DefaultsFor(env);
    
    c.fading = FadingParams::DefaultsFor(env);
    c.fading.model = fadingType;
    if (fadingType == FADING_NONE) {
        c.fading.enabled = false;
    }
    
    // NLOS parameters re-derived for UWB (approx +5dB for diffraction)
    c.nlos = NLOSParams::DefaultsFor(env);
    c.nlos.frequencyHz = c.propagation.frequencyHz;
    // UWB vs 868 MHz Diffraction (NLOS):
    // Fresnel-Kirchhoff knife-edge diffraction parameter ν ∝ √f.
    // Loss L ≈ 20*log10(ν) (for ν > 2.4), so loss scales as 10*log10(f_new / f_old).
    // 10*log10(7990/868) ≈ 9.64 dB additional diffraction loss at 7.99 GHz.
    c.nlos.nlosAttenuationDb.mean += 9.64;
    c.nlos.diffractionLossDb.mean += 9.64;

    c.atmospheric = AtmosphericParams::DefaultsFor(weather);
    
    // Foliage specific-attenuation re-derived for UWB
    // Scaling by ~5.28 per ITU-R P.833 (Am proportional to f^0.75). LIMITATION: Wavelength at 8GHz is ~3.75cm, approaching leaf size, making f^0.75 an uncertain extrapolation.
    c.foliage = FoliageParams::DefaultsFor(vegetation);
    c.foliage.specificAttenuationDbPerM.mean *= 5.28;
    
    c.interference = InterferenceParams::DefaultsFor(env);
    
    // UWB specific receiver setup
    c.receiver = ReceiverDecisionParamsUWB::Defaults();
    
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

LoRaUWBPhysicsConfig LoRaUWBPhysicsConfig::FromYAML(const std::string& filepath) {
    // Dummy implementation for UWB tier
    std::cerr << "Warning: FromYAML not implemented for UWB tier. Returning defaults." << std::endl;
    return ForScenario((EnvironmentClass)0, (WeatherState)0, (FadingModelType)0, (VegetationType)0, (MobilityModelType)0, false);
}

void LoRaUWBPhysicsConfig::ToYAML(const std::string& filepath) const {
    // Dummy implementation
    std::cerr << "Warning: ToYAML not implemented for UWB tier." << std::endl;
}

} // namespace ns3
