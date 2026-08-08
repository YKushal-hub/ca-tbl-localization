#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iomanip>

using namespace ns3;

int main(int argc, char* argv[]) {
    uint32_t numRows = 5000000; // 5M rows
    uint32_t seed = 42;
    std::string outputFile = "./results/ml_training_dataset.csv";
    
    CommandLine cmd(__FILE__);
    cmd.AddValue("rows", "Number of rows to generate", numRows);
    cmd.AddValue("seed", "Random seed", seed);
    cmd.AddValue("output", "Output CSV file", outputFile);
    cmd.Parse(argc, argv);
    
    std::ofstream out(outputFile);
    
    // 43 Columns
    out << "Environment,"
        << "DistanceM,"
        << "SpreadingFactor,"
        << "TxPowerDbm,"
        << "GatewayHeightM,"
        << "NodeHeightM,"
        << "PathLossDb,"
        << "ShadowingDb,"
        << "FadingDb,"
        << "NlosDb,"
        << "BuildingEntryLossDb,"
        << "WallPenetrationLossDb,"
        << "FloorAttenuationDb,"
        << "BodyShadowingLossDb,"
        << "TemporalBlockageLossDb,"
        << "IsTwoRay,"
        << "IsTunnel,"
        << "SoilAttenuationDb,"
        << "AtmosphericDb,"
        << "FoliageDb,"
        << "TerrainDb,"
        << "InterferencePenaltyDb,"
        << "DopplerPenaltyDb,"
        << "HwTxVariationDb,"
        << "EvmPercent,"
        << "IqImageRejectionDb,"
        << "AdcClippingOccurred,"
        << "TimeSyncErrorNs,"
        << "TotalLossDb,"
        << "TrueRssiDbm,"
        << "MeasuredRssiDbm,"
        << "NoiseFloorDbm,"
        << "RawSnrDb,"
        << "FinalSnrDb,"
        << "EffectiveSensitivityDbm,"
        << "SensitivityMarginDb,"
        << "PreambleLocked,"
        << "PayloadCrcSuccess,"
        << "CollisionOccurred,"
        << "CaptureSucceeded,"
        << "DelaySpreadNs,"
        << "RainRateMmHr,"
        << "EstimatedRangeM,"
        << "PositionErrorM,"
        << "RangeErrorM,"
        << "PropagationLossBiasDb,"
        << "PacketReceived\n";

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> envDist(0, 8); // ENV_RURAL_OPEN to ENV_TUNNEL_CONFINED
    std::uniform_real_distribution<> distDist(10.0, 15000.0);
    std::uniform_int_distribution<> sfDist(7, 12);
    std::uniform_real_distribution<> txPowDist(2.0, 14.0);
    std::uniform_real_distribution<> gwHeightDist(2.0, 50.0);
    std::uniform_real_distribution<> nodeHeightDist(0.5, 3.0);
    std::uniform_int_distribution<> weatherDist(0, 5); // 0=CLEAR, 1=HUMID, 2=LIGHT_RAIN, 3=HEAVY_RAIN, 4=FOG, 5=WINDY_DRY
    // P3: Heavy-rain bias — sample rain intensity bucket: 0=dry (0-2mm), 1=light (2-15mm), 2=heavy (>15mm)
    std::uniform_int_distribution<> rainBucketDist(0, 3); // 0=dry,1=dry,2=light,3=heavy → 25% heavy
    
    // Progress tracking
    uint32_t reportInterval = numRows / 10;
    if (reportInterval == 0) reportInterval = 1;

    NS_LOG_UNCOND("Starting ML Dataset Generation...");
    NS_LOG_UNCOND("Target rows: " << numRows);
    NS_LOG_UNCOND("Output file: " << outputFile);

    static LoRaDistributionSampler tbSampler(seed);
    static TemporalBlockageSubmodule blockageModule(TemporalBlockageParams::Defaults(), ENV_INDUSTRIAL, tbSampler);

    for (uint32_t i = 0; i < numRows; ++i) {
        if (i > 0 && i % reportInterval == 0) {
            NS_LOG_UNCOND("Generated " << i << " rows (" << (i * 100 / numRows) << "%)");
        }

        EnvironmentClass env = static_cast<EnvironmentClass>(envDist(gen));
        double distance = distDist(gen);
        uint8_t sf = sfDist(gen);
        double txPower = txPowDist(gen);
        double gwH = gwHeightDist(gen);
        double nH = nodeHeightDist(gen);

        // P3: Force ~25% of rows to heavy rain to verify Coupling 4 at high rain rates
        int rainBucket = rainBucketDist(gen);
        WeatherState weather;
        if (rainBucket == 3) {
            weather = WEATHER_HEAVY_RAIN;  // 25% heavy rain
        } else if (rainBucket == 2) {
            weather = WEATHER_LIGHT_RAIN;  // 25% light rain
        } else {
            // 50% dry — distribute across other weather states for variety
            WeatherState dry_states[] = {WEATHER_CLEAR_DRY, WEATHER_HUMID_HOT, WEATHER_FOG, WEATHER_WINDY_DRY};
            weather = dry_states[weatherDist(gen) % 4];
        }
        VegetationType veg = (env == ENV_RURAL_FORESTED) ? VEG_DENSE_FOREST : VEG_NONE;

        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            env, weather, FADING_RICIAN, veg, MOB_STATIC, false);
        
        config.randomSeed = seed + i;
        config.propagation.gatewayHeightM = DistributionParam::Fixed(gwH);
        config.propagation.nodeHeightM = DistributionParam::Fixed(nH);

        // Turn on all the impairments randomly to get a rich dataset
        config.buildingEntry.enabled = (env == ENV_CONCRETE_BUNKER || env == ENV_INDOOR_OFFICE);
        config.wallMaterial.enabled = config.buildingEntry.enabled;
        config.floorAttenuation.enabled = config.buildingEntry.enabled;
        
        // Fix D: Temporal Blockage computed statically outside engine
        config.temporalBlockage.enabled = false;
        
        // Fix C: Body shadowing flag
        bool isBodyWorn = (i % 5 == 0); // ~20% body-worn
        config.bodyShadowing.enabled = true;
        config.bodyShadowing.nodeIsBodyWorn = isBodyWorn;
        std::uniform_real_distribution<> angleDist(0.0, 2 * M_PI);
        double bodyOrientation = isBodyWorn ? angleDist(gen) : 0.0;
        
        // Fix B: Sub.21 NodePositionUncertainty
        const std::vector<double> SIGMA_M_LEVELS = {0.0, 50.0, 200.0, 500.0};
        double sigmaM = SIGMA_M_LEVELS[i % 4];
        config.nodePositionUncertainty.enabled = (sigmaM > 0.0);
        config.nodePositionUncertainty.horizontalErrorM = DistributionParam::Fixed(sigmaM);
        
        bool isTwoRay = (env == ENV_RURAL_OPEN && gwH < 5.0 && nH < 3.5);
        config.twoRayGround.enabled = isTwoRay;
        config.tunnel.enabled = (env == ENV_TUNNEL_CONFINED || env == ENV_CONCRETE_BUNKER);
        
        config.iqImpairment.enabled = true;
        config.hardware.enabled = true;

        LoRaPhysicsEngine engine;
        engine.Initialize(config);
        
        Vector txPos(distance, 0, nH);
        Vector rxPos(0, 0, gwH);

        // Simulate interference randomly for some packets
        uint32_t interferers = (i % 5 == 0) ? 1 : 0; 
        
        PhysicsResult res = engine.ProcessTransmission(
            i, 2, txPos, rxPos, txPower, sf, 125e3, interferers, Seconds(0), bodyOrientation);

        // Apply Temporal Blockage externally
        if (env == ENV_INDUSTRIAL) {
            LinkPhysicsState tbState;
            tbState.txPos = txPos;
            tbState.rxPos = rxPos;
            blockageModule.Compute(tbState);
            double blockageLoss = tbState.temporalBlockageDb;
            res.temporalBlockageDb = blockageLoss;
            res.totalLossDb += blockageLoss;
            res.trueRssiDbm -= blockageLoss;
            res.measuredRssiDbm -= blockageLoss;
        } else {
            res.temporalBlockageDb = 0.0;
        }
        
        // Fix F: Enforce TrueRssiDbm = TxPower - TotalLossDb
        res.trueRssiDbm = txPower - res.totalLossDb;

        out << std::fixed << std::setprecision(4)
            << (int)env << ","
            << distance << ","
            << (int)sf << ","
            << txPower << ","
            << gwH << ","
            << nH << ","
            << res.pathLossDb << ","
            << res.shadowingDb << ","
            << res.fadingDb << ","
            << res.nlosDb << ","
            << res.buildingEntryDb << ","
            << res.wallPenetrationDb << ","
            << res.floorAttenuationDb << ","
            << res.bodyShadowingDb << ","
            << res.temporalBlockageDb << ","
            << (res.isTwoRay ? 1 : 0) << ","
            << (res.isTunnel ? 1 : 0) << ","
            << res.soilAttenuationDb << ","
            << res.atmosphericDb << ","
            << res.foliageDb << ","
            << res.terrainDb << ","
            << res.interferencePenaltyDb << ","
            << res.dopplerPenaltyDb << ","
            << res.hwTxVariationDb << ","
            << res.evmPercent << ","
            << res.iqImageRejectionDb << ","
            << (res.adcClippingOccurred ? 1 : 0) << ","
            << res.timingSyncErrorNs << ","
            << res.totalLossDb << ","
            << res.trueRssiDbm << ","
            << res.measuredRssiDbm << ","
            << res.noiseFloorDbm << ","
            << res.raw_snr_db << ","
            << res.finalSNRDb << ","
            << res.effectiveSensitivityDbm << ","
            << res.sensitivityMarginDb << ","
            << (res.preambleLocked ? 1 : 0) << ","
            << (res.payloadCrcSuccess ? 1 : 0) << ","
            << (res.collisionOccurred ? 1 : 0) << ","
            << (res.captureSucceeded ? 1 : 0) << ","
            << res.delaySpreadNs << ","
            << res.sampledRainRateMmHr << ","
            << res.estimatedRangeM << ","
            << res.positionErrorM << ","
            << res.rangeErrorM << ","
            << res.propagationLossBiasDb << ","
            << (res.packetReceived ? 1 : 0) << "\n";
    }

    out.close();
    NS_LOG_UNCOND("Done! Saved 5M rows to " << outputFile);
    return 0;
}
