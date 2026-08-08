#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lora-physics-engine.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Stage6CityScaleValidation");

int main(int argc, char *argv[])
{
    LogComponentEnable("Stage6CityScaleValidation", LOG_LEVEL_INFO);
    
    // Command line arguments
    uint32_t numNodes = 10000; // Stress test node count
    uint32_t numGateways = 16; // Distributed cooperative gateways
    double simulationDuration = 10.0; // Seconds
    
    CommandLine cmd;
    cmd.AddValue("numNodes", "Number of end devices", numNodes);
    cmd.AddValue("numGateways", "Number of gateways", numGateways);
    cmd.AddValue("simTime", "Simulation time in seconds", simulationDuration);
    cmd.Parse(argc, argv);
    
    NS_LOG_INFO("================================================================");
    NS_LOG_INFO("  Stage 6 City-Scale Validation: Distributed Digital Twin Test  ");
    NS_LOG_INFO("================================================================");
    NS_LOG_INFO("Nodes: " << numNodes << " | Gateways: " << numGateways);

    // Initialize Stage 6 Extreme Config
    LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_RANDOM_WAYPOINT, false);
    cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM; // Max Realism
    
    // Enable all Stage 6 Distributed Digital Twin Modules
    cfg.microDsp.enabled = true;
    cfg.radioFrontend.enabled = true;
    cfg.spectrumOccupancy.enabled = true;
    cfg.distributedPhy.enabled = true;
    cfg.distributedScalability.enabled = true;
    cfg.distributedScalability.gpuBatchingEnabled = true;
    cfg.adaptiveEnvironment.enabled = true;
    cfg.geographicMemory.enabled = true;
    cfg.interference.enabled = true; // Dense ISM interference

    LoRaPhysicsEngine engine;
    engine.Initialize(cfg);

    // Gateway Registration
    std::vector<Vector> gwPositions;
    double gwSpacing = 2000.0; // 2km spacing
    for (uint32_t i = 0; i < numGateways; ++i) {
        double x = (i % 4) * gwSpacing - (gwSpacing * 1.5);
        double y = (i / 4) * gwSpacing - (gwSpacing * 1.5);
        Vector pos(x, y, 35.0); // 35m height
        engine.RegisterGateway(i + 1, pos, true);
        gwPositions.push_back(pos);
    }
    
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    
    uint32_t successfulDecodeCount = 0;
    uint32_t microDspTriggers = 0;
    uint32_t distributedGainTriggers = 0;
    
    NS_LOG_INFO("Commencing massive concurrent transmission simulation...");

    for (uint32_t n = 0; n < numNodes; ++n) {
        // Random node location within 4km x 4km city
        double nx = uv->GetValue(-3000.0, 3000.0);
        double ny = uv->GetValue(-3000.0, 3000.0);
        Vector txPos(nx, ny, 1.5);
        
        // Mixed mobility: Random velocities 0 - 30 m/s
        double velocity = uv->GetValue(0.0, 30.0);
        engine.SetNodeVelocity(n + 100, velocity);
        
        double txPower = uv->GetValue(10.0, 14.0);
        uint8_t sf = (uint8_t)uv->GetInteger(7, 12);
        
        // Find best gateway deterministically
        uint32_t bestGw = 1;
        double minDist = 99999.0;
        for (uint32_t g = 0; g < numGateways; ++g) {
            double dx = gwPositions[g].x - nx;
            double dy = gwPositions[g].y - ny;
            double dist = std::sqrt(dx*dx + dy*dy);
            if (dist < minDist) {
                minDist = dist;
                bestGw = g + 1;
            }
        }
        
        // Process
        Time simTime = Seconds(uv->GetValue(0.0, simulationDuration));
        PhysicsResult res = engine.ProcessTransmission(n + 100, bestGw, txPos, gwPositions[bestGw-1], txPower, sf, 125000.0, 0, simTime);
        
        if (res.packetReceived) successfulDecodeCount++;
        if (res.microDspActive) microDspTriggers++;
        if (res.distributedDecodeGainDb > 0.0) distributedGainTriggers++;
    }

    engine.GetStatistics().PrintSummary();
    
    NS_LOG_INFO("--- Stress Test Telemetry Validations ---");
    NS_LOG_INFO("Total Transmissions: " << numNodes);
    NS_LOG_INFO("Decodes: " << successfulDecodeCount);
    NS_LOG_INFO("MicroDSP Sample Activations: " << microDspTriggers);
    NS_LOG_INFO("Distributed Gateway Fusion Activations: " << distributedGainTriggers);

    return 0;
}
