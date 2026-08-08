/**
 * ca_tbl_trajectory_generator.cc -- CA-TBL Phase S0c
 *
 * Generates a time-ordered, spatially-correlated sequence of physics
 * observations. Uses ONE persistent LoRaPhysicsEngine per run -- the
 * opposite of ml_dataset_generator.cc's fresh-engine-per-row pattern.
 *
 * Persistent engine activates:
 *   - Gudmundson shadowing spatial correlation (ShadowingSubmodule::m_cache)
 *   - Hidden Markov channel hysteresis (m_hiddenChannelStates map)
 *   - Temporal Blockage Markov chain (TemporalBlockageSubmodule::m_blockageState)
 *
 * S0c scope: single-hop, 868 MHz, RANGING_PHYSICS_DERIVED mode.
 * Purpose: verify persistent-engine behavior on the validated tier before
 * S1/S2 add untested physics stacks.
 *
 * NOTE: ml_dataset_generator.cc passes a variable named `interferers` into
 * the relayHops parameter slot -- these are semantically different. This
 * generator passes relayHops=0 until hop semantics are formally defined.
 *
 * Build: auto-detected by ns-3 scratch/ CMake scan.
 * Run:   ./ns3 run ca_tbl_trajectory_generator [options]
 */

#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-physics-parameters.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>

using namespace ns3;

// Required because this file uses NS_LOG_*. Without it the macros expand to a
// reference to a g_log that does not exist, and the file fails to compile under
// any logging-enabled profile -- which is every profile except `optimized`.
// It built locally only because this tree is configured `--build-profile=optimized`,
// where the macros compile away. A fresh clone following the README's configure
// line got the `default` profile and could not build the scratch directory at all.
NS_LOG_COMPONENT_DEFINE("CaTblTrajectoryGenerator");

struct AnchorDef { uint32_t id; Vector pos; };
struct NodeDef   { uint32_t id; Vector pos; double headingRad; double speedMps; };

static Vector AdvancePosition(Vector pos, double headingRad,
                               double speedMps, double dt)
{
    return Vector(pos.x + speedMps * dt * std::cos(headingRad),
                  pos.y + speedMps * dt * std::sin(headingRad),
                  pos.z);
}

static bool IsReachable(Vector a, Vector b) {
    double dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz) < 15000.0;
}

static void WriteHeader(std::ofstream& out) {
    out << "NodeId,TimestampS,HopCount,Tier,"
        << "PathLossDb,ShadowingDb,FadingDb,NlosDb,"
        << "BuildingEntryLossDb,WallPenetrationLossDb,FloorAttenuationDb,"
        << "BodyShadowingLossDb,TemporalBlockageLossDb,"
        << "IsTwoRay,IsTunnel,SoilAttenuationDb,AtmosphericDb,FoliageDb,TerrainDb,"
        << "InterferencePenaltyDb,DopplerPenaltyDb,HwTxVariationDb,"
        << "EvmPercent,IqImageRejectionDb,AdcClippingOccurred,TimeSyncErrorNs,"
        << "TotalLossDb,TrueRssiDbm,MeasuredRssiDbm,NoiseFloorDbm,"
        << "RawSnrDb,FinalSnrDb,EffectiveSensitivityDbm,SensitivityMarginDb,"
        << "PreambleLocked,PayloadCrcSuccess,CollisionOccurred,CaptureSucceeded,"
        << "DelaySpreadNs,RainRateMmHr,"
        << "EstimatedRangeM,PositionErrorM,RangeErrorM,PropagationLossBiasDb,"
        << "PacketReceived,HiddenChannelState\n";
}

static void WriteRow(std::ofstream& out, uint32_t nodeId, double t,
                     uint8_t hopCount, const PhysicsResult& r)
{
    out << std::fixed << std::setprecision(4)
        << nodeId << "," << t << "," << (int)hopCount << "," << 0 << ","
        << r.pathLossDb << "," << r.shadowingDb << "," << r.fadingDb << ","
        << r.nlosDb << "," << r.buildingEntryDb << "," << r.wallPenetrationDb << ","
        << r.floorAttenuationDb << "," << r.bodyShadowingDb << ","
        << r.temporalBlockageDb << ","
        << (r.isTwoRay?1:0) << "," << (r.isTunnel?1:0) << ","
        << r.soilAttenuationDb << "," << r.atmosphericDb << "," << r.foliageDb << ","
        << r.terrainDb << "," << r.interferencePenaltyDb << ","
        << r.dopplerPenaltyDb << "," << r.hwTxVariationDb << ","
        << r.evmPercent << "," << r.iqImageRejectionDb << ","
        << (r.adcClippingOccurred?1:0) << "," << r.timingSyncErrorNs << ","
        << r.totalLossDb << "," << r.trueRssiDbm << "," << r.measuredRssiDbm << ","
        << r.noiseFloorDbm << "," << r.raw_snr_db << "," << r.finalSNRDb << ","
        << r.effectiveSensitivityDbm << "," << r.sensitivityMarginDb << ","
        << (r.preambleLocked?1:0) << "," << (r.payloadCrcSuccess?1:0) << ","
        << (r.collisionOccurred?1:0) << "," << (r.captureSucceeded?1:0) << ","
        << r.delaySpreadNs << "," << r.sampledRainRateMmHr << ","
        << r.estimatedRangeM << "," << r.positionErrorM << ","
        << r.rangeErrorM << "," << r.propagationLossBiasDb << ","
        << (r.packetReceived?1:0) << "," << r.hiddenChannelState << "\n";
}

int main(int argc, char* argv[])
{
    uint32_t    envInt       = 2;       // ENV_SUBURBAN -- closest to Amrita testbed
    double      simDurationS = 120.0;
    double      dt           = 1.0;
    uint32_t    seed         = 42;
    std::string outputFile   = "./results/ca_tbl_trajectory.csv";
    double      txPowerDbm   = 14.0;
    uint8_t     sf           = 9;
    double      bwHz         = 125e3;
    double      gwHeightM    = 10.0;
    double      nodeHeightM  = 1.5;

    CommandLine cmd(__FILE__);
    cmd.AddValue("env",      "EnvironmentClass int (0-8)", envInt);
    cmd.AddValue("duration", "Simulation duration seconds", simDurationS);
    cmd.AddValue("dt",       "Timestep seconds",            dt);
    cmd.AddValue("seed",     "Random seed",                 seed);
    cmd.AddValue("output",   "Output CSV path",             outputFile);
    cmd.AddValue("txpower",  "TX power dBm",                txPowerDbm);
    cmd.AddValue("sf",       "Spreading factor 7-12",       sf);
    cmd.Parse(argc, argv);

    EnvironmentClass env = static_cast<EnvironmentClass>(
        std::min((uint32_t)8, envInt));

    // ONE engine, ONE Initialize() -- persists across all timesteps.
    LoRaPhysicsEngine engine;
    LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
        env, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
    config.randomSeed = seed;
    config.propagation.gatewayHeightM = DistributionParam::Fixed(gwHeightM);
    config.propagation.nodeHeightM    = DistributionParam::Fixed(nodeHeightM);
    config.rangingModelMode           = RANGING_PHYSICS_DERIVED;
    config.temporalBlockage.enabled   = (env == ENV_INDUSTRIAL);
    config.bodyShadowing.enabled      = true;
    config.iqImpairment.enabled       = true;
    config.hardware.enabled           = true;
    engine.Initialize(config);

    // Amrita 3-anchor geometry placeholder (equilateral triangle ~1 km side)
    std::vector<AnchorDef> anchors = {
        {1, Vector(   0.0,   0.0, gwHeightM)},
        {2, Vector(1000.0,   0.0, gwHeightM)},
        {3, Vector( 500.0, 866.0, gwHeightM)},
    };
    for (auto& a : anchors) engine.RegisterGateway(a.id, a.pos, true);

    // 5-node placeholder -- straight-line motion at fixed heading/speed
    std::vector<NodeDef> nodes = {
        {10, Vector(200.0, 200.0, nodeHeightM), 0.45, 2.0},
        {11, Vector(700.0, 300.0, nodeHeightM), 1.20, 1.5},
        {12, Vector(300.0, 600.0, nodeHeightM), 2.80, 1.0},
        {13, Vector(800.0, 700.0, nodeHeightM), 0.00, 0.5},
        {14, Vector(500.0, 100.0, nodeHeightM), 3.14, 3.0},
    };
    for (auto& n : nodes) engine.RegisterNode(n.id, n.pos);

    std::ofstream out(outputFile);
    if (!out.is_open()) {
        NS_LOG_ERROR("Cannot open: " << outputFile);
        return 1;
    }
    WriteHeader(out);

    uint64_t totalPkts = 0, rxPkts = 0;
    NS_LOG_UNCOND("CA-TBL trajectory generator: env=" << envInt
                  << " dur=" << simDurationS << "s dt=" << dt << "s");

    for (double t = 0.0; t < simDurationS; t += dt) {
        for (auto& node : nodes) {
            node.pos = AdvancePosition(node.pos, node.headingRad, node.speedMps, dt);
            engine.UpdateNodePosition(node.id, node.pos);
        }
        // Stagger transmissions by 20 ms per node within the dt window.
        // Without this, all 5 nodes transmit at exactly Seconds(t) to
        // the same gateways and the engine's interference model treats
        // them as co-channel collisions -- physically correct for a real
        // LoRa network but wrong for CA-TBL's per-link channel evaluation.
        double nodeOffset = 0.0;
        for (auto& node : nodes) {
            for (auto& anchor : anchors) {
                if (!IsReachable(node.pos, anchor.pos)) continue;
                PhysicsResult res = engine.ProcessTransmission(
                    node.id, anchor.id, node.pos, anchor.pos,
                    txPowerDbm, sf, bwHz,
                    /*relayHops=*/0,
                    Seconds(t + nodeOffset), node.headingRad);
                WriteRow(out, node.id, t, 0, res); // log at t, not t+offset
                totalPkts++;
                if (res.packetReceived) rxPkts++;
            }
            nodeOffset += 1.0; // 1s stagger between nodes
        }
        if (std::fmod(t, 10.0) < dt) {
            NS_LOG_UNCOND("t=" << t << "s packets=" << totalPkts
                          << " PDR=" << (totalPkts>0 ? 100.0*rxPkts/totalPkts : 0.0) << "%");
        }
    }
    out.close();
    engine.GetStatistics().PrintSummary();
    NS_LOG_UNCOND("Done -> " << outputFile << "  total=" << totalPkts
                  << " rx=" << rxPkts);
    return 0;
}
