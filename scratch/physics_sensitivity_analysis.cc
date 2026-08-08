#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

using namespace ns3;

struct SensitivityResult {
    std::string paramName;
    double paramValue;
    uint8_t sf;
    double distanceM;
    double meanSnrDb;
    double pdr;
    double meanPathLossDb;
    double meanShadowingDb;
    double meanFadingDb;
    double meanNlosDb;
    double meanAtmosphericDb;
    double meanFoliageDb;
};

void RunSensitivitySweep(std::vector<SensitivityResult>& results)
{
    uint32_t txNodeId = 500;
    uint32_t rxGatewayId = 1000;
    double txPowerDbm = 14.0;
    double bandwidthHz = 125000.0;
    double distanceM = 1200.0;
    uint8_t sf = 9;
    uint32_t attempts = 100;
    double simTimeS = 0.0;

    Vector txPos(0.0, 0.0, 1.5);
    Vector rxPos(distanceM, 0.0, 30.0);

    LoRaPhysicsEngineHelper helper;

    std::cout << "Starting physical layer sensitivity sweeps..." << std::endl;

    // 1. Sweep Foliage Depth (0m to 300m)
    std::cout << "--> Sweeping Foliage Depth..." << std::endl;
    for (double depth = 0.0; depth <= 300.0; depth += 50.0) {
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_DENSE_FOREST, MOB_STATIC, false);
        config.foliage.vegetationDepthM = DistributionParam::Fixed(depth);
        config.shadowing.spatialCorrelationEnabled = false; // Disable spatial shadowing correlation to isolate foliage
        helper.Initialize(config);

        uint32_t successCount = 0;
        double totSnr=0, totPL=0, totShad=0, totFad=0, totNlos=0, totAtmo=0, totFol=0;

        for (uint32_t i = 0; i < attempts; ++i) {
            PhysicsResult r = helper.Process(txNodeId++, rxGatewayId, txPos, rxPos, txPowerDbm, sf, bandwidthHz, 0, Seconds(simTimeS += 0.02));
            if (r.packetReceived) successCount++;
            totSnr += r.finalSNRDb; totPL += r.pathLossDb; totShad += r.shadowingDb;
            totFad += r.fadingDb; totNlos += r.nlosDb; totAtmo += r.atmosphericDb; totFol += r.foliageDb;
        }
        results.push_back({"foliage_depth_m", depth, sf, distanceM, totSnr/attempts, (double)successCount/attempts,
                           totPL/attempts, totShad/attempts, totFad/attempts, totNlos/attempts, totAtmo/attempts, totFol/attempts});
    }

    // 2. Sweep Rain Rate (0 to 100 mm/hr)
    std::cout << "--> Sweeping Rain Rate..." << std::endl;
    for (double rain = 0.0; rain <= 100.0; rain += 20.0) {
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_SUBURBAN, WEATHER_HEAVY_RAIN, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        config.atmospheric.rainRateMmPerHr = DistributionParam::Fixed(rain);
        helper.Initialize(config);

        uint32_t successCount = 0;
        double totSnr=0, totPL=0, totShad=0, totFad=0, totNlos=0, totAtmo=0, totFol=0;

        // Reset txNodeId and simTimeS per rain sweep iteration to lock the fading random seed/time sequence
        uint32_t rainTxNodeId = 1500;
        double rainSimTimeS = 200.0;

        for (uint32_t i = 0; i < attempts; ++i) {
            PhysicsResult r = helper.Process(rainTxNodeId++, rxGatewayId, txPos, rxPos, txPowerDbm, sf, bandwidthHz, 0, Seconds(rainSimTimeS += 0.02));
            if (r.packetReceived) successCount++;
            totSnr += r.finalSNRDb; totPL += r.pathLossDb; totShad += r.shadowingDb;
            totFad += r.fadingDb; totNlos += r.nlosDb; totAtmo += r.atmosphericDb; totFol += r.foliageDb;
        }
        results.push_back({"rain_rate_mm_hr", rain, sf, distanceM, totSnr/attempts, (double)successCount/attempts,
                           totPL/attempts, totShad/attempts, totFad/attempts, totNlos/attempts, totAtmo/attempts, totFol/attempts});
    }

    // 3. Sweep Building Penetration Loss (0 dB to 30 dB)
    std::cout << "--> Sweeping NLOS Building Penetration..." << std::endl;
    for (double bldLoss = 0.0; bldLoss <= 30.0; bldLoss += 5.0) {
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
        config.nlos.buildingPenetrationDb = DistributionParam::Fixed(bldLoss);
        config.nlos.defaultNumberOfFloors = 2;
        helper.Initialize(config);

        uint32_t successCount = 0;
        double totSnr=0, totPL=0, totShad=0, totFad=0, totNlos=0, totAtmo=0, totFol=0;

        for (uint32_t i = 0; i < attempts; ++i) {
            PhysicsResult r = helper.Process(txNodeId++, rxGatewayId, txPos, rxPos, txPowerDbm, sf, bandwidthHz, 0, Seconds(simTimeS += 0.02));
            if (r.packetReceived) successCount++;
            totSnr += r.finalSNRDb; totPL += r.pathLossDb; totShad += r.shadowingDb;
            totFad += r.fadingDb; totNlos += r.nlosDb; totAtmo += r.atmosphericDb; totFol += r.foliageDb;
        }
        results.push_back({"building_penetration_db", bldLoss, sf, distanceM, totSnr/attempts, (double)successCount/attempts,
                           totPL/attempts, totShad/attempts, totFad/attempts, totNlos/attempts, totAtmo/attempts, totFol/attempts});
    }

    // 4. Sweep Spreading Factor (7 to 12) at Long Distance (3500m)
    std::cout << "--> Sweeping Spreading Factor at Long Distance (3500m)..." << std::endl;
    Vector longRxPos(3500.0, 0.0, 30.0);
    for (uint8_t curSf = 7; curSf <= 12; ++curSf) {
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST, MOB_STATIC, false);
        helper.Initialize(config);

        uint32_t successCount = 0;
        double totSnr=0, totPL=0, totShad=0, totFad=0, totNlos=0, totAtmo=0, totFol=0;

        for (uint32_t i = 0; i < attempts; ++i) {
            PhysicsResult r = helper.Process(txNodeId++, rxGatewayId, txPos, longRxPos, txPowerDbm, curSf, bandwidthHz, 0, Seconds(simTimeS += 0.02));
            if (r.packetReceived) successCount++;
            totSnr += r.finalSNRDb; totPL += r.pathLossDb; totShad += r.shadowingDb;
            totFad += r.fadingDb; totNlos += r.nlosDb; totAtmo += r.atmosphericDb; totFol += r.foliageDb;
        }
        results.push_back({"spreading_factor", (double)curSf, curSf, 3500.0, totSnr/attempts, (double)successCount/attempts,
                           totPL/attempts, totShad/attempts, totFad/attempts, totNlos/attempts, totAtmo/attempts, totFol/attempts});
    }
}

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    std::cout << "================================================================" << std::endl;
    std::cout << "      LoRa Physics Engine Sensitivity Analysis Simulation       " << std::endl;
    std::cout << "================================================================" << std::endl;

    std::vector<SensitivityResult> results;
    RunSensitivitySweep(results);

    std::string csvPath = "physics_sensitivity_analysis.csv";
    std::ofstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open output file: " << csvPath << std::endl;
        return 1;
    }

    file << "parameter_name,parameter_value,spreading_factor,distance_m,mean_snr_db,pdr,mean_path_loss_db,mean_shadowing_db,mean_fading_db,mean_nlos_db,mean_atmospheric_db,mean_foliage_db\n";
    for (const auto& r : results) {
        file << r.paramName << ","
             << std::fixed << std::setprecision(2) << r.paramValue << ","
             << (int)r.sf << ","
             << r.distanceM << ","
             << r.meanSnrDb << ","
             << std::setprecision(4) << r.pdr << ","
             << std::setprecision(2) << r.meanPathLossDb << ","
             << r.meanShadowingDb << ","
             << r.meanFadingDb << ","
             << r.meanNlosDb << ","
             << r.meanAtmosphericDb << ","
             << r.meanFoliageDb << "\n";
    }
    file.close();

    std::cout << "================================================================" << std::endl;
    std::cout << "Sensitivity analysis complete! Output written to: " << csvPath << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
