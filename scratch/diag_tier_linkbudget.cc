/**
 * \file diag_tier_linkbudget.cc
 * \brief Diagnostic: per-tier link budget and reception rate versus distance.
 *
 * Why this exists
 * ---------------
 * In the first causal run, the always-on-UWB arm produced zero fixes in 600
 * cycles -- including at t=0, with the node 21 m from an anchor. That is either
 * (a) correct physics, because 6.5 GHz through dense forest is genuinely brutal,
 * or (b) a misconfiguration in how the UWB engine is being driven.
 *
 * Those two possibilities have opposite responses. If it is (a), the finding is
 * real and important -- it means tier 3 is a final-approach-only tier in
 * vegetated terrain, and the architecture should say so. If it is (b), a config
 * is wrong and any result built on it is worthless.
 *
 * Guessing between them by adjusting constants until UWB "works" would be the
 * worst possible move: the physics engine is the most validated part of this
 * project, and quietly inflating its numbers to make an architectural claim
 * come out right would invalidate everything downstream of it.
 *
 * So: measure. This sweeps distance, runs many trials per point, and prints the
 * full loss decomposition alongside the reception rate for each tier. The answer
 * is then a fact rather than an opinion.
 *
 * Usage:
 *   ./ns3 run "diag_tier_linkbudget --veg=3 --trials=200"
 *   ./ns3 run "diag_tier_linkbudget --veg=0 --trials=200"   # bare ground, control
 */

#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-2p4-physics-engine.h"
#include "ns3/lora-uwb-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-2p4-physics-parameters.h"
#include "ns3/lora-uwb-physics-parameters.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;

namespace
{

struct Accum
{
    double pathLoss = 0;
    double shadowing = 0;
    double fading = 0;
    double nlos = 0;
    double foliage = 0;
    double atmospheric = 0;
    double terrain = 0;
    double total = 0;
    double rxPower = 0;
    double effSens = 0;
    double margin = 0;
    double snr = 0;
    double noiseFloor = 0;
    uint32_t received = 0;
    uint32_t trials = 0;

    void Mean(double n)
    {
        if (n <= 0)
        {
            return;
        }
        pathLoss /= n;
        shadowing /= n;
        fading /= n;
        nlos /= n;
        foliage /= n;
        atmospheric /= n;
        terrain /= n;
        total /= n;
        rxPower /= n;
        effSens /= n;
        margin /= n;
        snr /= n;
        noiseFloor /= n;
    }
};

template <typename ResultT>
void
Absorb(Accum& a, const ResultT& r)
{
    a.pathLoss += r.pathLossDb;
    a.shadowing += r.shadowingDb;
    a.fading += r.fadingDb;
    a.nlos += r.nlosDb;
    a.foliage += r.foliageDb;
    a.atmospheric += r.atmosphericDb;
    a.terrain += r.terrainDb;
    a.total += r.totalLossDb;
    a.rxPower += r.rxPowerDbm;
    a.effSens += r.effectiveSensitivityDbm;
    a.margin += r.sensitivityMarginDb;
    a.snr += r.finalSNRDb;
    a.noiseFloor += r.noiseFloorDbm;
    if (r.packetReceived)
    {
        a.received++;
    }
    a.trials++;
}

void
PrintHeader(const std::string& tier)
{
    std::cout << "\n=== " << tier << " ===\n";
    std::cout << std::right << std::setw(9) << "dist_m" << std::setw(9) << "PDR" << std::setw(10)
              << "pathL" << std::setw(9) << "shadow" << std::setw(9) << "fade" << std::setw(9)
              << "nlos" << std::setw(9) << "foliage" << std::setw(9) << "atmos" << std::setw(9)
              << "terrain" << std::setw(10) << "totalL" << std::setw(10) << "rx_dBm" << std::setw(10)
              << "sens" << std::setw(9) << "margin" << std::setw(9) << "snr\n";
}

void
PrintRow(double d, const Accum& a)
{
    const double pdr = (a.trials > 0) ? static_cast<double>(a.received) / a.trials : 0.0;
    std::cout << std::fixed << std::right << std::setprecision(1) << std::setw(9) << d
              << std::setprecision(3) << std::setw(9) << pdr << std::setprecision(1) << std::setw(10)
              << a.pathLoss << std::setw(9) << a.shadowing << std::setw(9) << a.fading
              << std::setw(9) << a.nlos << std::setw(9) << a.foliage << std::setw(9)
              << a.atmospheric << std::setw(9) << a.terrain << std::setw(10) << a.total
              << std::setw(10) << a.rxPower << std::setw(10) << a.effSens << std::setw(9) << a.margin
              << std::setw(9) << a.snr << "\n";
}

} // namespace

int
main(int argc, char* argv[])
{
    uint32_t envInt = 1;
    uint32_t vegInt = 3; // VEG_DENSE_FOREST in the enum used by the generators
    uint32_t trials = 200;
    uint32_t seed = 7;
    std::string outCsv = "results/diag_tier_linkbudget.csv";

    CommandLine cmd;
    cmd.AddValue("env", "EnvironmentClass", envInt);
    cmd.AddValue("veg", "VegetationClass (0=none .. 3=dense forest)", vegInt);
    cmd.AddValue("trials", "Trials per distance point", trials);
    cmd.AddValue("seed", "Random seed", seed);
    cmd.AddValue("out", "CSV output path", outCsv);
    cmd.Parse(argc, argv);

    RngSeedManager::SetSeed(seed);

    const EnvironmentClass env = static_cast<EnvironmentClass>(envInt);
    const VegetationType veg = static_cast<VegetationType>(vegInt);

    LoRaPhysicsConfig cfg868 = LoRaPhysicsConfig::ForScenario(
        env, WEATHER_CLEAR_DRY, FADING_RICIAN, veg, MOB_CONSTANT_VELOCITY, false);
    cfg868.randomSeed = seed;
    cfg868.rangingModelMode = RANGING_PHYSICS_DERIVED;

    LoRa2p4PhysicsConfig cfg2p4 = LoRa2p4PhysicsConfig::ForScenario(
        env, WEATHER_CLEAR_DRY, FADING_RICIAN, veg, MOB_CONSTANT_VELOCITY, false);
    cfg2p4.randomSeed = seed;
    cfg2p4.rangingModelMode = RANGING_PHYSICS_DERIVED;

    LoRaUWBPhysicsConfig cfgUwb = LoRaUWBPhysicsConfig::ForScenario(
        env, WEATHER_CLEAR_DRY, FADING_RICIAN, veg, MOB_CONSTANT_VELOCITY, false);
    cfgUwb.randomSeed = seed;
    cfgUwb.rangingModelMode = RANGING_PHYSICS_DERIVED;

    LoRaPhysicsEngine e868;
    e868.Initialize(cfg868);
    LoRa2p4PhysicsEngine e2p4;
    e2p4.Initialize(cfg2p4);
    LoRaUWBPhysicsEngine eUwb;
    eUwb.Initialize(cfgUwb);

    const uint32_t nodeId = 1;
    const uint32_t gwId = 101;
    const Vector gwPos(0.0, 0.0, 10.0);

    e868.RegisterGateway(gwId, gwPos, true);
    e2p4.RegisterGateway(gwId, gwPos, true);
    eUwb.RegisterGateway(gwId, gwPos, true);
    e868.RegisterNode(nodeId, Vector(1.0, 0.0, 1.5));
    e2p4.RegisterNode(nodeId, Vector(1.0, 0.0, 1.5));
    eUwb.RegisterNode(nodeId, Vector(1.0, 0.0, 1.5));

    // Log-spaced-ish distances covering every tier's plausible envelope, dense
    // at the short end because that is where the UWB question lives.
    const double dists[] = {1,   2,   3,   5,   8,   10,  15,  20,   30,   50,
                            75,  100, 150, 200, 300, 500, 750, 1000, 2000, 5000};
    const int nd = static_cast<int>(sizeof(dists) / sizeof(dists[0]));

    std::ofstream csv(outCsv.c_str());
    csv << "tier,dist_m,pdr,pathLossDb,shadowingDb,fadingDb,nlosDb,foliageDb,atmosphericDb,"
        << "terrainDb,totalLossDb,rxPowerDbm,effSensDbm,marginDb,snrDb,noiseFloorDbm\n";

    std::cout << "Link-budget sweep  env=" << envInt << " veg=" << vegInt << " trials=" << trials
              << " seed=" << seed << "\n";
    std::cout << "TX power: 868=14.0 dBm  2.4=12.5 dBm  UWB=-16.47 dBm\n";

    for (int tier = 0; tier < 3; ++tier)
    {
        const char* name = (tier == 0) ? "868MHz" : (tier == 1) ? "2.4GHz" : "UWB";
        PrintHeader(name);

        for (int i = 0; i < nd; ++i)
        {
            const double d = dists[i];
            const Vector nodePos(d, 0.0, 1.5);
            Accum acc;

            for (uint32_t k = 0; k < trials; ++k)
            {
                const Time now = Seconds(static_cast<double>(k) * 0.5);
                if (tier == 0)
                {
                    e868.UpdateNodePosition(nodeId, nodePos);
                    Absorb(acc,
                           e868.ProcessTransmission(nodeId, gwId, nodePos, gwPos, 14.0, 7, 125e3, 0, now));
                }
                else if (tier == 1)
                {
                    e2p4.UpdateNodePosition(nodeId, nodePos);
                    Absorb(acc,
                           e2p4.ProcessTransmission(nodeId, gwId, nodePos, gwPos, 12.5, 7, 800e3, 0, now));
                }
                else
                {
                    eUwb.UpdateNodePosition(nodeId, nodePos);
                    Absorb(
                        acc,
                        eUwb.ProcessTransmission(nodeId, gwId, nodePos, gwPos, -16.47, 7, 499.2e6, 0, now));
                }
            }

            const double pdr =
                (acc.trials > 0) ? static_cast<double>(acc.received) / acc.trials : 0.0;
            acc.Mean(static_cast<double>(acc.trials));
            PrintRow(d, acc);

            csv << name << "," << d << "," << pdr << "," << acc.pathLoss << "," << acc.shadowing
                << "," << acc.fading << "," << acc.nlos << "," << acc.foliage << ","
                << acc.atmospheric << "," << acc.terrain << "," << acc.total << "," << acc.rxPower
                << "," << acc.effSens << "," << acc.margin << "," << acc.snr << "," << acc.noiseFloor
                << "\n";
        }
    }

    csv.close();

    std::cout << "\nWrote " << outCsv << "\n\n";
    std::cout
        << "How to read this:\n"
        << "  * Find the distance where each tier's PDR crosses 0.5. That is the tier's\n"
        << "    usable envelope in this environment, and it is what the anchor spacing\n"
        << "    and the trajectory in ca_tbl_causal_sim must be built around.\n"
        << "  * If UWB's PDR is ~0 even at 1-5 m, something is misconfigured -- the DW3000\n"
        << "    class part is specified to -104 dBm here and should close a link at that\n"
        << "    range even in dense vegetation.\n"
        << "  * If UWB's PDR is high at 1-10 m and collapses by 20-30 m, the physics is\n"
        << "    RIGHT and the earlier zero-fix result was a scenario error: the walker was\n"
        << "    roaming a 1.2 km box, so it was outside tier 3's envelope essentially\n"
        << "    always. That is a finding to report, not a constant to change.\n"
        << "  * Compare the foliage column across tiers. Vegetation loss scales sharply\n"
        << "    with frequency, so a large foliage term at 6.5 GHz that is small at\n"
        << "    868 MHz is expected physics, not a bug.\n";

    return 0;
}
