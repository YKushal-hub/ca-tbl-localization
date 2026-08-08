#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ScaleSweepSimulation");

void RunScaleSimulation(double scaleKm, uint32_t seed, const std::string& csvPath)
{
    std::cout << "\n================================================================" << std::endl;
    std::cout << "Running Simulation scale: " << scaleKm << " km | Seed: " << seed << std::endl;
    std::cout << "================================================================" << std::endl;

    RngSeedManager::SetSeed(seed);

    uint32_t numNodes = 500;
    uint32_t numGateways = 10;
    double simDurationS = 3600.0;
    double scaleM = scaleKm * 1000.0;

    LoRaPhysicsEngineHelper helper;
    helper.EnableResultLogging(csvPath);

    // Initialize Suburban Clear Scenario
    LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
    config.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
    config.randomSeed = seed;
    // Set NLOS scale distance slightly less aggressive for suburban regime to avoid binary cutoff
    config.nlos.nlosScaleDistanceM = DistributionParam::Fixed(8000.0);
    helper.Initialize(config);

    // Layout 10 gateways in a circle around the center at 25% of scale radius
    std::vector<Vector> gwPositions;
    double gwRadius = 0.25 * scaleM;
    for (uint32_t g = 0; g < numGateways; ++g) {
        double angle = 2.0 * M_PI * g / numGateways;
        Vector pos(gwRadius * std::cos(angle), gwRadius * std::sin(angle), 35.0);
        helper.GetEngine()->RegisterGateway(g + 1, pos, true);
        gwPositions.push_back(pos);
    }

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();

    uint32_t totalTransmissions = 0;
    uint32_t totalSuccessfulDecodes = 0;

    // Track each node's packets and successes for coverage
    std::vector<uint32_t> nodeAttempts(numNodes, 0);
    std::vector<uint32_t> nodeSuccesses(numNodes, 0);

    // Nodes are distributed uniformly over the Scale radius
    std::vector<Vector> nodePositions;
    for (uint32_t n = 0; n < numNodes; ++n) {
        double theta = uv->GetValue(0.0, 2.0 * M_PI);
        double r = uv->GetValue(100.0, scaleM);
        Vector pos(r * std::cos(theta), r * std::sin(theta), 1.5);
        nodePositions.push_back(pos);
    }

    // Process transmissions over 3600 seconds. Each node transmits periodically.
    double intervalS = simDurationS / 50.0; // 50 transmissions per node over 3600s
    for (uint32_t txIdx = 0; txIdx < 50; ++txIdx) {
        double baseTimeS = txIdx * intervalS;
        for (uint32_t n = 0; n < numNodes; ++n) {
            Vector txPos = nodePositions[n];
            double txTimeS = baseTimeS + uv->GetValue(0.0, intervalS - 0.1);
            
            // Randomize packet parameters
            double txPower = 14.0;
            uint8_t sf = 9;
            double bandwidthHz = 125000.0;

            // Packet is transmitted to all gateways
            bool decryptedByAny = false;
            for (uint32_t g = 0; g < numGateways; ++g) {
                PhysicsResult res = helper.Process(
                    n + 1, g + 1, txPos, gwPositions[g], txPower, sf, bandwidthHz, 0, Seconds(txTimeS));
                
                totalTransmissions++;
                nodeAttempts[n]++;
                
                if (res.packetReceived) {
                    decryptedByAny = true;
                    totalSuccessfulDecodes++;
                    nodeSuccesses[n]++;
                }
            }
        }
    }

    helper.CloseResultLogging();

    // Calculate overall statistics
    uint32_t nodesCovered = 0;
    for (uint32_t n = 0; n < numNodes; ++n) {
        if (nodeSuccesses[n] > 0) {
            nodesCovered++;
        }
    }

    double overallPdr = (double)totalSuccessfulDecodes / totalTransmissions;
    double nodeCoverage = (double)nodesCovered / numNodes;

    std::cout << "Simulation Completed!" << std::endl;
    std::cout << "  --> Overall PDR        : " << (overallPdr * 100.0) << "%" << std::endl;
    std::cout << "  --> Node Coverage      : " << (nodeCoverage * 100.0) << "%" << std::endl;
    std::cout << "  --> Total Decoded      : " << totalSuccessfulDecodes << " / " << totalTransmissions << std::endl;
}

int main(int argc, char *argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    // Run the sweep across 35 km, 60 km, and 100 km immediately
    std::vector<double> scales = {35.0, 60.0, 100.0};
    std::vector<uint32_t> seeds = {7, 11, 23};

    for (double scale : scales) {
        for (uint32_t seed : seeds) {
            std::string csvPath = "physics_scale_sweep_" + std::to_string((int)scale) + "km_seed_" + std::to_string(seed) + ".csv";
            RunScaleSimulation(scale, seed, csvPath);
        }
    }

    return 0;
}
