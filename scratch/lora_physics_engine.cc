#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/lora-interference-submodule.h"
#include "ns3/lora-hardware-submodule.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace ns3;

struct NodePos {
    uint32_t id;
    Vector pos;
};

struct TransmissionEvent {
    uint32_t nodeId;
    Vector pos;
    double txTimeS;
    double txPower;
    
    bool operator<(const TransmissionEvent& other) const {
        return txTimeS < other.txTimeS;
    }
};

int main(int argc, char *argv[]) {
    double rangeM = 35000;
    uint32_t numNodes = 500;
    uint32_t numGateways = 10;
    uint32_t sf = 7;
    double durationS = 3600;
    uint32_t seed = 7;
    std::string envStr = "ENV_SUBURBAN";
    std::string weatherStr = "CLEAR_DRY";
    double gatewayHeight = 30.0;
    double nodeHeight = 1.5;
    bool fineTimestamp = false;
    double fadingK = -1.0;
    std::string outputFile = "";
    double bw = 125000.0;
    double txPower = 14.0;
    double appPeriodS = 0.0; // 0 = single TX per node (default); >0 = repeat every N seconds

    CommandLine cmd;
    cmd.AddValue("range", "Radius of simulation circle in meters", rangeM);
    cmd.AddValue("nodes", "Number of nodes", numNodes);
    cmd.AddValue("gateways", "Number of gateways", numGateways);
    cmd.AddValue("sf", "Spreading factor", sf);
    cmd.AddValue("duration", "Simulation duration in seconds", durationS);
    cmd.AddValue("seed", "Random seed", seed);
    cmd.AddValue("env", "Environment type: ENV_URBAN_MODERATE, ENV_SUBURBAN, ENV_RURAL, ENV_OPEN_FIELD", envStr);
    cmd.AddValue("weather", "Weather", weatherStr);
    cmd.AddValue("gateway_height", "Gateway height", gatewayHeight);
    cmd.AddValue("node_height", "Node height", nodeHeight);
    cmd.AddValue("fine_timestamp", "Fine timestamp", fineTimestamp);
    cmd.AddValue("fading_k", "Fading K-factor", fadingK);
    cmd.AddValue("output", "Output file path", outputFile);
    cmd.AddValue("bw", "Bandwidth", bw);
    cmd.AddValue("txpower", "TX Power", txPower);
    cmd.AddValue("app_period", "Application period in seconds (0=single TX per node)", appPeriodS);
    cmd.Parse(argc, argv);

    // Seed RNG
    ns3::RngSeedManager::SetSeed(seed);
    ns3::RngSeedManager::SetRun(1);

    EnvironmentClass envType = ENV_SUBURBAN;
    if (envStr == "ENV_URBAN_MODERATE") envType = ENV_URBAN_MODERATE;
    else if (envStr == "ENV_URBAN_DENSE") envType = ENV_URBAN_DENSE;
    else if (envStr == "ENV_RURAL_OPEN") envType = ENV_RURAL_OPEN;
    else if (envStr == "ENV_SUBURBAN") envType = ENV_SUBURBAN;
    else {
        NS_FATAL_ERROR("Unknown environment: " << envStr << ". Valid: ENV_URBAN_MODERATE, ENV_SUBURBAN, ENV_RURAL_OPEN, ENV_URBAN_DENSE");
    }

    LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(envType, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, fineTimestamp);
    // Explicitly enforce the fix values
    config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
    if (fadingK >= 0.0) {
        config.fading.ricianKFactor = DistributionParam::Fixed(fadingK);
    }
    
    config.propagation.gatewayHeightM = DistributionParam::Fixed(gatewayHeight);
    config.propagation.nodeHeightM = DistributionParam::Fixed(nodeHeight);
    
    config.bodyShadowing.enabled = true;
    
    config.temporalBlockage.enabled = true;
    config.nodePositionUncertainty.enabled = true;
    
    // Submodules (needed to query observation)
    LoRaDistributionSampler sampler(seed);
    HardwareSubmodule hw(config.hardware, sampler);

    LoRaPhysicsEngine engine;
    engine.Initialize(config);

    std::vector<NodePos> nodes;
    std::vector<NodePos> gateways;

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    
    auto generatePos = [&](double rMax, double height) {
        double r = rMax * std::sqrt(uv->GetValue());
        double theta = 2.0 * M_PI * uv->GetValue();
        return Vector(r * std::cos(theta), r * std::sin(theta), height);
    };

    for (uint32_t i = 1; i <= numGateways; ++i) {
        gateways.push_back({i, generatePos(rangeM * 0.4, gatewayHeight)});
    }
    for (uint32_t i = 1; i <= numNodes; ++i) {
        nodes.push_back({1000 + i, generatePos(rangeM, nodeHeight)});
    }

    std::ostream* outStream = &std::cout;
    std::ofstream outFileStream;
    if (!outputFile.empty()) {
        outFileStream.open(outputFile);
        if (outFileStream.is_open()) {
            outStream = &outFileStream;
        }
    }

    *outStream << "TxID,RxID,TimestampNs,RxPowerDbm,SnrDb,TrueRangeM,NlosPenaltyDb,FadingDb,AtmosphericDb,ShadowingDb,FoliageDb,TerrainDb,PropagationLossDb,TotalPathLossDb,CFO_Hz,PhaseNoiseRad,TimestampJitterNs,RssiOffsetDb,PreambleDetected,HeaderValid,payload_valid,CollidedWith,CaptureSIR,DelaySpreadNs,MarginDb,ThermalNoiseShiftDb,ElevationAngle,K_factor,Environment,InterferenceDrops,NlosIsNlos,ShadowingSigmaEff,BuildingEntryLossDb,WallPenetrationLossDb,FloorAttenuationDb,BodyShadowingLossDb,TemporalBlockageLossDb,IsTwoRay,IsTunnel,SoilAttenuationDb,EstimatedRangeM,RangeErrorM,PositionErrorM,PropagationLossBiasDb" << std::endl;

    std::vector<TransmissionEvent> txEvents;
    for (const auto& node : nodes) {
        if (appPeriodS > 0.0) {
            // Multi-packet mode: each node transmits every appPeriodS seconds
            // First TX at a random offset within [0, appPeriodS) to stagger
            double firstTxS = uv->GetValue(0, appPeriodS);
            for (double t = firstTxS; t < durationS; t += appPeriodS) {
                double actualTxPower = hw.ApplyTxPowerVariation(txPower, node.id);
                txEvents.push_back({node.id, node.pos, t, actualTxPower});
            }
        } else {
            // Single-packet mode (original behaviour)
            double txTimeS = uv->GetValue(0, durationS);
            double actualTxPower = hw.ApplyTxPowerVariation(txPower, node.id);
            txEvents.push_back({node.id, node.pos, txTimeS, actualTxPower});
        }
    }
    
    std::sort(txEvents.begin(), txEvents.end());

    for (const auto& ev : txEvents) {
        Time txTime = Seconds(ev.txTimeS);

        for (const auto& gw : gateways) {
            double bodyOrientation = uv->GetValue(0, 2.0 * M_PI);
            PhysicsResult res = engine.ProcessTransmission(
                ev.nodeId, gw.id,
                ev.pos, gw.pos,
                ev.txPower, sf, bw, 0, txTime, bodyOrientation
            );

            double trueRangeM = std::sqrt(
                std::pow(ev.pos.x - gw.pos.x, 2) +
                std::pow(ev.pos.y - gw.pos.y, 2) +
                std::pow(ev.pos.z - gw.pos.z, 2)
            );

            if (trueRangeM > rangeM * 1.02) continue; // Skip links exceeding range

            double elevAngle = std::atan2(gw.pos.z - ev.pos.z, 
                std::sqrt(std::pow(gw.pos.x - ev.pos.x, 2) + std::pow(gw.pos.y - ev.pos.y, 2)));
            double elevAngleDeg = elevAngle * 180.0 / M_PI;

            double exactMarginDb = res.rxPowerDbm - res.effectiveSensitivityDbm;

            // Generate timestamp observation
            TimestampObservation obs = hw.GenerateTimestampObservation(
                ev.nodeId, gw.id, txTime, res.rxPowerDbm, res.snr_db, sf, 0
            );

            double timestampJitterNs = obs.measuredArrivalTimeNs - obs.trueArrivalTimeNs;
            double rssiOffsetDb = obs.measuredRssiDbm - obs.trueRssiDbm;

            double exactTotalLossDb = 14.0 - res.rxPowerDbm;

            *outStream << ev.nodeId << ","
                      << gw.id << ","
                      << (long long)(obs.measuredArrivalTimeNs) << ","
                      << res.rxPowerDbm << ","
                      << res.snr_db << ","
                      << trueRangeM << ","
                      << res.nlosDb << ","
                      << res.fadingDb << ","
                      << res.atmosphericDb << ","
                      << res.shadowingDb << ","
                      << res.foliageDb << ","
                      << res.terrainDb << ","
                      << res.pathLossDb << ","
                      << exactTotalLossDb << ","
                      << obs.cfoHz << ","
                      << obs.phaseNoiseRad << ","
                      << timestampJitterNs << ","
                      << rssiOffsetDb << ","
                      << (res.preambleLocked ? 1 : 0) << ","
                      << (res.packetReceived ? 1 : 0) << ","
                      << (res.payloadCrcSuccess ? 1 : 0) << ","
                      << (res.collisionOccurred ? (int)res.interfererCount : -1) << ","
                      << res.captureMarginDb << ","
                      << res.delaySpreadNs << ","
                      << exactMarginDb << ","
                      << res.thermalNoiseShiftDb << ","
                      << elevAngleDeg << ","
                      << res.sampledKFactor << ","
                      << envStr << ","
                      << "0" << "," // InterferenceDrops
                      << (res.isNLOS ? 1 : 0) << ","
                      << res.shadowingStdEff << ","
                      << res.buildingEntryDb << ","
                      << res.wallPenetrationDb << ","
                      << res.floorAttenuationDb << ","
                      << res.bodyShadowingDb << ","
                      << res.temporalBlockageDb << ","
                      << (res.isTwoRay ? 1 : 0) << ","
                      << (res.isTunnel ? 1 : 0) << ","
                      << res.soilAttenuationDb << ","
                      << res.estimatedRangeM << ","
                      << res.rangeErrorM << ","
                      << res.positionErrorM << ","
                      << res.propagationLossBiasDb
                      << std::endl;
        }
    }

    return 0;
}
