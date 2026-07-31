#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-2p4-physics-engine.h"
#include "ns3/lora-uwb-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-2p4-physics-parameters.h"
#include "ns3/lora-uwb-physics-parameters.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace ns3;

struct NodeDef {
    uint32_t id;
    Vector pos;
    double speedMps;
    double headingRad;
};

struct AnchorDef {
    uint32_t id;
    Vector pos;
};

// Write a CSV header
static void WriteHeader(std::ofstream& out) {
    out << "Tier,NodeId,AnchorId,TimestampS,TrueX,TrueY,TrueZ,PathLossDb,ShadowingDb,FadingDb,NlosDb,"
        << "TotalLossDb,TrueRssiDbm,NoiseFloorDbm,FinalSnrDb,PacketReceived,"
        << "EstimatedRangeM,RangeErrorM,HiddenChannelState\n";
}

// Write a single result row
static void WriteRow(std::ofstream& out, const std::string& tierName,
                     uint32_t nodeId, uint32_t anchorId, double t,
                     const Vector& truePos, const PhysicsResult& res) {
    out << tierName << "," << nodeId << "," << anchorId << "," << std::fixed << std::setprecision(4) << t << ","
        << truePos.x << "," << truePos.y << "," << truePos.z << ","
        << res.pathLossDb << "," << res.shadowingDb << "," << res.fadingDb << "," << res.nlosDb << ","
        << res.totalLossDb << "," << res.trueRssiDbm << "," << res.noiseFloorDbm << ","
        << res.finalSNRDb << "," << res.packetReceived << ","
        << res.estimatedRangeM << "," << res.rangeErrorM << "," << res.hiddenChannelState << "\n";
}
static void WriteRow2p4(std::ofstream& out, const std::string& tierName,
                     uint32_t nodeId, uint32_t anchorId, double t,
                     const Vector& truePos, const PhysicsResult2p4& res) {
    out << tierName << "," << nodeId << "," << anchorId << "," << std::fixed << std::setprecision(4) << t << ","
        << truePos.x << "," << truePos.y << "," << truePos.z << ","
        << res.pathLossDb << "," << res.shadowingDb << "," << res.fadingDb << "," << res.nlosDb << ","
        << res.totalLossDb << "," << res.trueRssiDbm << "," << res.noiseFloorDbm << ","
        << res.finalSNRDb << "," << res.packetReceived << ","
        << res.estimatedRangeM << "," << res.rangeErrorM << "," << res.hiddenChannelState << "\n";
}
static void WriteRowUWB(std::ofstream& out, const std::string& tierName,
                     uint32_t nodeId, uint32_t anchorId, double t,
                     const Vector& truePos, const PhysicsResultUWB& res) {
    out << tierName << "," << nodeId << "," << anchorId << "," << std::fixed << std::setprecision(4) << t << ","
        << truePos.x << "," << truePos.y << "," << truePos.z << ","
        << res.pathLossDb << "," << res.shadowingDb << "," << res.fadingDb << "," << res.nlosDb << ","
        << res.totalLossDb << "," << res.trueRssiDbm << "," << res.noiseFloorDbm << ","
        << res.finalSNRDb << "," << res.packetReceived << ","
        << res.estimatedRangeM << "," << res.rangeErrorM << "," << res.hiddenChannelState << "\n";
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    uint32_t envInt = 1; // 1 = ENV_RURAL_FORESTED
    double simDurationS = 60.0;
    double dt = 1.0;
    uint32_t seed = 42;
    std::string outputFile = "./results/ca_tbl_trajectory_multi.csv";
    
    cmd.AddValue("env", "EnvironmentClass (0=Urban, 1=Forest, 2=Suburban)", envInt);
    cmd.AddValue("duration", "Sim duration in seconds", simDurationS);
    cmd.AddValue("dt", "Time step in seconds", dt);
    cmd.AddValue("seed", "Random seed", seed);
    cmd.AddValue("output", "Output CSV file path", outputFile);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed);
    EnvironmentClass env = static_cast<EnvironmentClass>(envInt);

    // Setup configurations for all 3 tiers
    LoRaPhysicsConfig config868 = LoRaPhysicsConfig::ForScenario(env, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_DENSE_FOREST, MOB_CONSTANT_VELOCITY, false);
    config868.randomSeed = seed;
    config868.rangingModelMode = RANGING_PHYSICS_DERIVED;

    LoRa2p4PhysicsConfig config2p4 = LoRa2p4PhysicsConfig::ForScenario(env, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_DENSE_FOREST, MOB_CONSTANT_VELOCITY, false);
    config2p4.randomSeed = seed;
    config2p4.rangingModelMode = RANGING_PHYSICS_DERIVED;

    LoRaUWBPhysicsConfig configUwb = LoRaUWBPhysicsConfig::ForScenario(env, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_DENSE_FOREST, MOB_CONSTANT_VELOCITY, false);
    configUwb.randomSeed = seed;
    configUwb.rangingModelMode = RANGING_PHYSICS_DERIVED;

    // Instanciate engines
    LoRaPhysicsEngine engine868;
    engine868.Initialize(config868);
    
    LoRa2p4PhysicsEngine engine2p4;
    engine2p4.Initialize(config2p4);
    
    LoRaUWBPhysicsEngine engineUwb;
    engineUwb.Initialize(configUwb);

    // 3 Anchors, ~100m spacing
    std::vector<AnchorDef> anchors = {
        {101, Vector(0.0, 0.0, 10.0)},
        {102, Vector(100.0, 0.0, 10.0)},
        {103, Vector(50.0, 86.6, 10.0)}
    };
    for (auto& a : anchors) {
        engine868.RegisterGateway(a.id, a.pos, true);
        engine2p4.RegisterGateway(a.id, a.pos, true);
        engineUwb.RegisterGateway(a.id, a.pos, true);
    }

    // Moving Node (Starts at 20, 15, moves at 0.8, 0.5 m/s)
    std::vector<NodeDef> nodes = {
        {1, Vector(20.0, 15.0, 1.5), 0.943, 0.558} // speed=0.943, heading=0.558 rad
    };
    for (auto& n : nodes) {
        engine868.RegisterNode(n.id, n.pos);
        engine2p4.RegisterNode(n.id, n.pos);
        engineUwb.RegisterNode(n.id, n.pos);
    }

    std::ofstream out(outputFile);
    WriteHeader(out);

    uint64_t rx868 = 0, rx2p4 = 0, rxUwb = 0, total = 0;
    
    // SF/BW and Tx Power for each
    uint8_t sf868 = 7; double bw868 = 125e3; double txPwr868 = 14.0;
    uint8_t sf2p4 = 7; double bw2p4 = 800e3; double txPwr2p4 = 12.5;
    uint8_t sfUwb = 7; double bwUwb = 499.2e6; double txPwrUwb = -16.47;

    for (double t = 0; t < simDurationS; t += dt) {
        for (auto& node : nodes) {
            node.pos.x += 0.8 * dt;
            node.pos.y += 0.5 * dt;
            
            engine868.UpdateNodePosition(node.id, node.pos);
            engine2p4.UpdateNodePosition(node.id, node.pos);
            engineUwb.UpdateNodePosition(node.id, node.pos);
        }
        
        for (auto& node : nodes) {
            for (auto& gw : anchors) {
                // Tier 1: 868 MHz
                PhysicsResult res868 = engine868.ProcessTransmission(
                    node.id, gw.id, node.pos, gw.pos, txPwr868, sf868, bw868, 0, Seconds(t), node.headingRad);
                WriteRow(out, "868MHz", node.id, gw.id, t, node.pos, res868);
                if (res868.packetReceived) rx868++;

                // Tier 2: 2.4 GHz
                PhysicsResult2p4 res2p4 = engine2p4.ProcessTransmission(
                    node.id, gw.id, node.pos, gw.pos, txPwr2p4, sf2p4, bw2p4, 0, Seconds(t), node.headingRad);
                WriteRow2p4(out, "2.4GHz", node.id, gw.id, t, node.pos, res2p4);
                if (res2p4.packetReceived) rx2p4++;

                // Tier 3: UWB
                PhysicsResultUWB resUwb = engineUwb.ProcessTransmission(
                    node.id, gw.id, node.pos, gw.pos, txPwrUwb, sfUwb, bwUwb, 0, Seconds(t), node.headingRad);
                WriteRowUWB(out, "UWB", node.id, gw.id, t, node.pos, resUwb);
                if (resUwb.packetReceived) rxUwb++;

                total++;
            }
        }
    }
    out.close();
    
    std::cout << "Done! Output to " << outputFile << "\n";
    std::cout << "868MHz PDR: " << (double)rx868 / total * 100.0 << "%\n";
    std::cout << "2.4GHz PDR: " << (double)rx2p4 / total * 100.0 << "%\n";
    std::cout << "UWB PDR:    " << (double)rxUwb / total * 100.0 << "%\n";
    return 0;
}
