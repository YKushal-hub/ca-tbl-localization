/**
 * ca_tbl_trajectory_generator.cc -- CA-TBL Phase S0c
 *
 * Generates a time-ordered, spatially-correlated sequence of physics
 * observations. Uses ONE persistent LoRaUWBPhysicsEngine per run -- the
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
#include "ns3/lora-uwb-physics-engine.h"
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
NS_LOG_COMPONENT_DEFINE("CaTblTrajectoryGeneratorUwb");

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
                     uint8_t hopCount, const PhysicsResultUWB& r)
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
    std::string outputFile   = "./results/ca_tbl_trajectory_uwb.csv";
    double      txPowerDbm   = -16.47;  // UWB max EIRP: -14.32 dBm. Subtracting 2.15 dBi default antenna gain = -16.47 dBm
    uint8_t     sf           = 7;       // Placeholder logic
    double      bwHz         = 499.2e6; // DW3000 Channel 9 BW
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
    LoRaUWBPhysicsEngine engine;
    LoRaUWBPhysicsConfig config = LoRaUWBPhysicsConfig::ForScenario(
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

    // Short-range UWB anchor at origin
    std::vector<AnchorDef> anchors = {
        {100, Vector(0.0, 0.0, 2.0)},
    };
    for (auto& a : anchors) engine.RegisterGateway(a.id, a.pos, true);

    // Node starting 200m away, closing in on the anchor
    std::vector<NodeDef> nodes = {
        {10, Vector(-200.0, 0.0, nodeHeightM), 0.0, 1.0}, // Moves along X axis at 1 m/s
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
            if (node.pos.x < -5.0) {
                node.pos.x += node.speedMps * dt;
            }
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
                PhysicsResultUWB res = engine.ProcessTransmission(
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
