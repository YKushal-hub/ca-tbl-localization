#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>
#include <vector>
#include <iomanip>

using namespace ns3;

/**
 * @brief Simulates a hybrid LoRaWAN mesh network transmission schedule across various regimes
 * and logs detailed physical layer telemetry using the LoRaPhysicsEngineHelper.
 */
int main(int argc, char *argv[])
{
    CommandLine cmd;
    bool verbose = false;
    cmd.AddValue("verbose", "Print verbose simulation output", verbose);
    cmd.Parse(argc, argv);

    std::cout << "================================================================" << std::endl;
    std::cout << "     LoRaWAN Hybrid Mesh Regime Sweep & Physics Simulation" << std::endl;
    std::cout << "================================================================" << std::endl;

    // Define regimes to sweep
    struct Regime {
        std::string name;
        EnvironmentClass env;
        WeatherState weather;
        VegetationType veg;
        MobilityModelType mob;
    };

    std::vector<Regime> regimes = {
        {"Suburban_Clear", ENV_SUBURBAN, WEATHER_CLEAR_DRY, VEG_LIGHT_FOREST, MOB_STATIC},
        {"Urban_Dense_Rain", ENV_URBAN_DENSE, WEATHER_HEAVY_RAIN, VEG_NONE, MOB_RANDOM_WAYPOINT},
        {"Rural_Forested_Fog", ENV_RURAL_FORESTED, WEATHER_FOG, VEG_DENSE_FOREST, MOB_STATIC},
        {"Tunnel_Confined", ENV_TUNNEL_CONFINED, WEATHER_CLEAR_DRY, VEG_NONE, MOB_STATIC}
    };

    // Instantiate LoRaPhysicsEngineHelper
    LoRaPhysicsEngineHelper physicsHelper;
    physicsHelper.EnableResultLogging("regime_sweep_with_physics.csv");

    uint32_t txNodeId = 1;
    uint32_t rxGatewayId = 100;
    double bandwidthHz = 125000.0;
    double txPowerDbm = 14.0;

    double simTimeS = 0.0;

    std::cout << "Running regime sweep across " << regimes.size() << " environments..." << std::endl;

    for (const auto& regime : regimes) {
        std::cout << "--> Regime: " << std::setw(20) << std::left << regime.name;

        // Initialize engine configuration for this regime
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            regime.env, regime.weather, FADING_RICIAN, regime.veg, regime.mob, true);
        config.verboseLogging = verbose;
        physicsHelper.Initialize(config);

        uint32_t successCount = 0;
        uint32_t totalAttempts = 0;

        // Sweep spreading factors 7 to 12
        for (uint8_t sf = 7; sf <= 12; ++sf) {
            // Sweep distances from 200m to 4000m
            for (double distanceM = 200.0; distanceM <= 4000.0; distanceM += 400.0) {
                // Sweep relay hops (0 = direct, 1 = single-hop mesh relay, 2 = dual-hop mesh relay)
                for (uint8_t relayHops = 0; relayHops <= 2; ++relayHops) {
                    Vector txPos(0.0, 0.0, 1.5);
                    Vector rxPos(distanceM, 0.0, 25.0);

                    // Execute transmission through physics helper
                    PhysicsResult res = physicsHelper.Process(
                        txNodeId++, rxGatewayId, txPos, rxPos, txPowerDbm,
                        sf, bandwidthHz, relayHops, Seconds(simTimeS));

                    totalAttempts++;
                    if (res.packetReceived) {
                        successCount++;
                    }

                    simTimeS += 0.05; // 50 ms per packet attempt
                }
            }
        }

        double pdr = (double)successCount / totalAttempts;
        std::cout << " | PDR: " << std::fixed << std::setprecision(1) << (pdr * 100.0) << "% ("
                  << successCount << "/" << totalAttempts << " successful)" << std::endl;
    }

    physicsHelper.CloseResultLogging();

    std::cout << "================================================================" << std::endl;
    std::cout << "Simulation complete. Results written to: regime_sweep_with_physics.csv" << std::endl;
    std::cout << "================================================================" << std::endl;

    return 0;
}
