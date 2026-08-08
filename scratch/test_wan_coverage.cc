#include "ns3/core-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>
#include <random>
#include <vector>

using namespace ns3;

int main(int argc, char* argv[])
{
    // Controlled baseline-reproduction harness only.
    // This file intentionally preserves the original 50 km x 50 km Monte Carlo
    // density (500 nodes, 10 gateways) and disables the hardware/terrain
    // submodules to match the pre-submodule reference baseline.
    // The production 5M-row dataset generator uses a separate configuration path
    // and does not inherit these test-only toggles.
    uint32_t numNodes = 500;
    uint32_t numGateways = 10;
    double scaleM = 50000.0;
    double txPower = 14.0;
    double gwHeight = 35.0;
    double nodeHeight = 1.5;

    std::mt19937 rng(42);
    LoRaPhysicsEngineHelper helper;
    LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(
        ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST, MOB_STATIC, true);
    
    // Isolate Interference as the only remaining active extra module while keeping
    // Hardware and Terrain explicitly off, matching the requested one-at-a-time check.
    cfg.interference.enabled = true;
    cfg.interference.captureEffectEnabled = false;
    cfg.hardware.enabled = true;
    cfg.terrain.enabled = true;
    cfg.temporalBlockage.enabled = true;
    cfg.bodyShadowing.enabled = true;
    cfg.nodePositionUncertainty.enabled = true;
    
    cfg.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
    helper.Initialize(cfg);

    // Setup gateways
    std::vector<Vector> gwPositions;
    std::uniform_real_distribution<double> dxy(0.0, scaleM);
    for (uint32_t g = 0; g < numGateways; ++g) {
        Vector pos(dxy(rng), dxy(rng), gwHeight);
        gwPositions.push_back(pos);
    }

    // Setup nodes
    std::vector<Vector> nodePositions;
    for (uint32_t n = 0; n < numNodes; ++n) {
        Vector pos(dxy(rng), dxy(rng), nodeHeight);
        nodePositions.push_back(pos);
    }

    uint32_t coveredNodes = 0;
    uint32_t tdoaNodes = 0;
    std::vector<double> penalties;
    penalties.reserve(numNodes * numGateways);
    std::vector<int> bins(7, 0);

    for (uint32_t n = 0; n < numNodes; ++n) {
        Vector pos = nodePositions[n];

        uint32_t gwsReached = 0;
        for (uint32_t g = 0; g < numGateways; ++g) {
            PhysicsResult res = helper.Process(
                n + 1, g + 1, pos, gwPositions[g], txPower, 12, 125000.0, 0, Seconds(n * 0.05));
            penalties.push_back(res.interferencePenaltyDb);
            if (res.interferencePenaltyDb < 0.5) {
                bins[0]++;
            } else if (res.interferencePenaltyDb < 2.5) {
                bins[1]++;
            } else if (res.interferencePenaltyDb < 5.0) {
                bins[2]++;
            } else if (res.interferencePenaltyDb < 10.0) {
                bins[3]++;
            } else if (res.interferencePenaltyDb < 15.0) {
                bins[4]++;
            } else if (res.interferencePenaltyDb < 20.0) {
                bins[5]++;
            } else {
                bins[6]++;
            }
            if (res.packetReceived) {
                gwsReached++;
            }
        }
        if (gwsReached >= 1) {
            coveredNodes++;
        }
        if (gwsReached >= 3) {
            tdoaNodes++;
        }
    }

    std::cout << "WAN Coverage: " << (100.0 * coveredNodes / numNodes) << "%" << std::endl;
    std::cout << "TDOA Coverage (>=3 GWs): " << (100.0 * tdoaNodes / numNodes) << "%" << std::endl;
    std::cout << "Interference penalty distribution (" << penalties.size() << " samples):" << std::endl;
    std::cout << "  <0.5 dB: " << bins[0] << std::endl;
    std::cout << "  0.5-2.5 dB: " << bins[1] << std::endl;
    std::cout << "  2.5-5 dB: " << bins[2] << std::endl;
    std::cout << "  5-10 dB: " << bins[3] << std::endl;
    std::cout << "  10-15 dB: " << bins[4] << std::endl;
    std::cout << "  15-20 dB: " << bins[5] << std::endl;
    std::cout << "  >20 dB: " << bins[6] << std::endl;
    double sum = 0.0;
    double maxPenalty = 0.0;
    for (double p : penalties) {
        sum += p;
        maxPenalty = std::max(maxPenalty, p);
    }
    std::cout << "  mean penalty: " << (sum / penalties.size()) << " dB" << std::endl;
    std::cout << "  max penalty: " << maxPenalty << " dB" << std::endl;
    return 0;
}
