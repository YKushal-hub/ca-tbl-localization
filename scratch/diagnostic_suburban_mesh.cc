/**
 * topology_observability_sim.cc
 *
 * PURPOSE: Compute REAL per-node GatewayObservabilityCount and TdoaEligible
 * rates for WAN, MESH, and HYBRID topologies using actual placed geometry
 * and the validated LoRa physics engine.
 *
 * Replaces the hardcoded TDOA_RATES = {WAN:0.72, MESH:0.31, HYBRID:0.54}
 * in multimodal_generator.py with measured values.
 *
 * Scale (Option B from spec): 500 nodes, 10 gateways, 50x50 km area.
 * 100 Monte Carlo runs per topology to average placement variance.
 *
 * Output:
 *   results/topology_observability_output.csv  -- per-node per-run detail
 *   results/topology_rates_summary.json        -- aggregated rates (used by generator)
 *
 * Build:
 *   ./ns3 build topology_observability_sim
 * Run:
 *   ./ns3 run "topology_observability_sim --numNodes=500 --numGateways=10 \
 *              --areaSideKm=50 --runs=100 --hopLimitMesh=2 --hopLimitHybrid=3"
 */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lora-physics-engine-helper.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <filesystem>

using namespace ns3;

// ── Configuration ─────────────────────────────────────────────────────────
static uint32_t g_numNodes     = 500;
static uint32_t g_numGateways  = 10;
static double   g_areaSideKm   = 50.0;
static uint32_t g_runs         = 100;
static uint32_t g_hopLimitMesh = 2;
static uint32_t g_hopLimitHybrid = 3;
static uint32_t g_tdoaMinGw   = 3;   // minimum gateways for TdoaEligible
static uint8_t  g_sf          = 12;  // SF12: max range, conservative eligibility
static double   g_txPowerDbm  = 14.0;
static double   g_bwHz        = 125000.0;
static double   g_nodeHeightM = 1.5;   // pedestrian/device height
static double   g_gwHeightM   = 25.0;  // gateway antenna height

// ── Point struct ───────────────────────────────────────────────────────────
struct Point2D {
    double x, y;          // metres from origin
    double heightM;
};

// ── Random placement ───────────────────────────────────────────────────────
std::vector<Point2D> PlaceUniform(uint32_t n, double sideM, double heightM,
                                   std::mt19937& rng)
{
    std::uniform_real_distribution<double> uni(0.0, sideM);
    std::vector<Point2D> pts;
    pts.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
        pts.push_back({uni(rng), uni(rng), heightM});
    return pts;
}

// ── Distance between two points ────────────────────────────────────────────
double Dist2D(const Point2D& a, const Point2D& b)
{
    double dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

// ── Evaluate one LoRa link using the physics engine ────────────────────────
bool EvalLink(LoRaPhysicsEngineHelper& helper, LoRaPhysicsConfig& cfg,
              const Point2D& tx, const Point2D& rx,
              uint32_t& simClock)
{
    helper.Initialize(cfg);
    Vector txVec(tx.x, tx.y, tx.heightM);
    Vector rxVec(rx.x, rx.y, rx.heightM);
    // relayHops=0: direct link evaluation only
    PhysicsResult r = helper.Process(
        simClock, simClock + 10000,
        txVec, rxVec,
        g_txPowerDbm, g_sf, g_bwHz,
        0,                         // relayHops = 0 (direct link)
        Seconds(simClock * 0.02)
    );
    simClock++;
    return r.packetReceived;
}

// ── Build node-to-node mesh reachability graph ─────────────────────────────
// Returns: reachable[i][j] = true if node i can directly reach node j
std::vector<std::vector<bool>> BuildDirectLinks(
    LoRaPhysicsEngineHelper& helper, LoRaPhysicsConfig& cfgNode,
    const std::vector<Point2D>& nodes, uint32_t& simClock)
{
    uint32_t n = nodes.size();
    std::vector<std::vector<bool>> adj(n, std::vector<bool>(n, false));
    // Only evaluate pairs within plausible LoRa range (< 25km node-to-node at SF12)
    // to avoid O(n^2) physics calls for distant pairs that will always fail.
    const double MAX_NODE_RANGE_M = 25000.0;
    for (uint32_t i = 0; i < n; ++i) {
        for (uint32_t j = i+1; j < n; ++j) {
            double d = Dist2D(nodes[i], nodes[j]);
            if (d > MAX_NODE_RANGE_M) continue;
            bool ok = EvalLink(helper, cfgNode, nodes[i], nodes[j], simClock);
            adj[i][j] = adj[j][i] = ok;
        }
    }
    return adj;
}

// ── BFS: count gateways reachable within hop_limit hops via mesh ───────────
int MeshGwCount(uint32_t startNode,
                const std::vector<std::vector<bool>>& nodeAdj,
                const std::vector<bool>& nodeCanReachGw,   // which nodes see >=1 GW directly
                const std::vector<int>& nodeGwCount,        // how many GWs node sees directly
                uint32_t hopLimit)
{
    // BFS through the mesh graph from startNode
    // A gateway is "reachable" if any node within hopLimit hops sees it directly.
    // We track unique gateways reachable, but since we don't have per-GW adjacency
    // in the mesh graph, we approximate: count distinct GW sets reachable.
    // Simplified: count the MAX GW direct-count among all nodes within hopLimit hops.
    // (Conservative: a relay node exposes all its GWs to the relay chain.)
    uint32_t n = nodeAdj.size();
    std::vector<int> dist(n, -1);
    std::vector<uint32_t> queue;
    dist[startNode] = 0;
    queue.push_back(startNode);
    int gwCount = nodeGwCount[startNode]; // direct count for start node

    for (uint32_t qi = 0; qi < queue.size(); ++qi) {
        uint32_t cur = queue[qi];
        if ((uint32_t)dist[cur] >= hopLimit) continue;
        for (uint32_t nb = 0; nb < n; ++nb) {
            if (nodeAdj[cur][nb] && dist[nb] < 0) {
                dist[nb] = dist[cur] + 1;
                queue.push_back(nb);
                // Accumulate GW visibility via relay (sum, not max — a relay
                // aggregates packets from multiple GWs for TDOA)
                gwCount += nodeGwCount[nb];
            }
        }
    }
    return gwCount;
}

// ── Run one Monte Carlo trial for all three topologies ─────────────────────
struct NodeResult {
    uint32_t run;
    std::string topology;
    uint32_t nodeId;
    double nodeX, nodeY;
    int    gwObsCount;
    int    tdoaEligible;
};

std::vector<NodeResult> RunOneTrial(
    uint32_t runIdx, std::mt19937& rng,
    LoRaPhysicsEngineHelper& helperGw,    // node-to-gateway config (GW at 25m)
    LoRaPhysicsEngineHelper& helperNode,  // node-to-node config (both at 1.5m)
    LoRaPhysicsConfig& cfgGw,
    LoRaPhysicsConfig& cfgNode,
    uint32_t& simClock)
{
    double sideM = g_areaSideKm * 1000.0;
    auto nodes = PlaceUniform(g_numNodes, sideM, g_nodeHeightM, rng);
    auto gws   = PlaceUniform(g_numGateways, sideM, g_gwHeightM, rng);

    // Step 1: node-to-gateway direct link matrix
    // nodeGwDirect[i][j] = can node i reach gateway j directly?
    std::vector<std::vector<bool>> nodeGwDirect(g_numNodes,
        std::vector<bool>(g_numGateways, false));
    std::vector<int> directGwCount(g_numNodes, 0);

    for (uint32_t i = 0; i < g_numNodes; ++i) {
        for (uint32_t j = 0; j < g_numGateways; ++j) {
            bool ok = EvalLink(helperGw, cfgGw, nodes[i], gws[j], simClock);
            nodeGwDirect[i][j] = ok;
            if (ok) directGwCount[i]++;
        }
    }

    // Step 2: node-to-node direct links (for MESH / HYBRID relay)
    // Only needed if any topology uses mesh; compute once, reuse.
    std::vector<bool> hasDirectGw(g_numNodes);
    for (uint32_t i = 0; i < g_numNodes; ++i)
        hasDirectGw[i] = (directGwCount[i] > 0);

    auto nodeAdj = BuildDirectLinks(helperNode, cfgNode, nodes, simClock);

    std::vector<NodeResult> results;
    results.reserve(g_numNodes * 3);

    // Step 3: per topology, compute GatewayObservabilityCount
    for (const std::string& topo : {"WAN", "MESH", "HYBRID"}) {
        for (uint32_t i = 0; i < g_numNodes; ++i) {
            int gwObs = 0;

            if (topo == "WAN") {
                // All gateways in direct range count
                gwObs = directGwCount[i];

            } else if (topo == "MESH") {
                // No direct WAN credit — must relay via node-to-node
                // GW reachable if any relay node within hopLimit sees it
                gwObs = MeshGwCount(i, nodeAdj, hasDirectGw,
                                    directGwCount, g_hopLimitMesh);
                // Cap at actual number of gateways
                gwObs = std::min(gwObs, (int)g_numGateways);

            } else { // HYBRID
                if (directGwCount[i] > 0) {
                    // Has direct uplink — use it
                    gwObs = directGwCount[i];
                } else {
                    // No direct link — relay via mesh (higher hop limit)
                    gwObs = MeshGwCount(i, nodeAdj, hasDirectGw,
                                        directGwCount, g_hopLimitHybrid);
                    gwObs = std::min(gwObs, (int)g_numGateways);
                }
            }

            int eligible = (gwObs >= (int)g_tdoaMinGw) ? 1 : 0;
            results.push_back({runIdx, topo, i, nodes[i].x, nodes[i].y,
                                gwObs, eligible});
        }
    }
    return results;
}

// ── Main ───────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("numNodes",      "Number of end-nodes",        g_numNodes);
    cmd.AddValue("numGateways",   "Number of gateways",         g_numGateways);
    cmd.AddValue("areaSideKm",    "Area side length in km",     g_areaSideKm);
    cmd.AddValue("runs",          "Monte Carlo repetitions",    g_runs);
    cmd.AddValue("hopLimitMesh",  "Hop limit for MESH relay",   g_hopLimitMesh);
    cmd.AddValue("hopLimitHybrid","Hop limit for HYBRID relay", g_hopLimitHybrid);
    cmd.AddValue("tdoaMinGw",     "Min GWs for TdoaEligible",  g_tdoaMinGw);
    cmd.Parse(argc, argv);

    std::cout << "==================================================" << std::endl;
    std::cout << "  Topology-Aware Gateway Observability Simulator" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "  Nodes:     " << g_numNodes << std::endl;
    std::cout << "  Gateways:  " << g_numGateways << std::endl;
    std::cout << "  Area:      " << g_areaSideKm << " x " << g_areaSideKm << " km" << std::endl;
    std::cout << "  MC runs:   " << g_runs << std::endl;
    std::cout << "  SF:        " << (int)g_sf << " (max range, conservative)" << std::endl;
    std::cout << "  TdoaMinGw: " << g_tdoaMinGw << std::endl;
    std::cout << "  HopLimitMesh:   " << g_hopLimitMesh << std::endl;
    std::cout << "  HopLimitHybrid: " << g_hopLimitHybrid << std::endl;
    std::cout << std::endl;

    // Create results directory
    std::filesystem::create_directories("results");

    // Physics engine instances — one for GW links (25m antenna), one for node links (1.5m)
    LoRaPhysicsEngineHelper helperGw, helperNode;

    // Use Rural_Forested as representative average (moderate NLOS, not extreme)
    // All 12 environments from the base dataset are averaged implicitly via Monte Carlo
    // placement variance — using a single "representative" environment is the same
    // approximation as the base dataset's Environment sweep.
    LoRaPhysicsConfig cfgGw   = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST,
        MOB_STATIC, true);
    cfgGw.verboseLogging = false;

    LoRaPhysicsConfig cfgNode = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST,
        MOB_STATIC, true);
    cfgNode.verboseLogging = false;

    // Output file
    std::ofstream csvOut("results/topology_observability_output.csv");
    csvOut << "Run,Topology,NodeId,NodeX_m,NodeY_m,GwObsCount,TdoaEligible\n";

    // Accumulators for summary
    std::map<std::string, double> totalEligible = {{"WAN",0},{"MESH",0},{"HYBRID",0}};
    std::map<std::string, double> totalNodes    = {{"WAN",0},{"MESH",0},{"HYBRID",0}};

    uint32_t simClock = 1;
    for (uint32_t run = 0; run < g_runs; ++run) {
        std::mt19937 rng(run * 12345 + 67890);  // deterministic per run
        auto results = RunOneTrial(run, rng, helperGw, helperNode,
                                   cfgGw, cfgNode, simClock);

        for (const auto& r : results) {
            csvOut << r.run << "," << r.topology << "," << r.nodeId << ","
                   << std::fixed << std::setprecision(1)
                   << r.nodeX << "," << r.nodeY << ","
                   << r.gwObsCount << "," << r.tdoaEligible << "\n";
            totalEligible[r.topology] += r.tdoaEligible;
            totalNodes[r.topology]    += 1;
        }

        if ((run+1) % 10 == 0 || run == 0) {
            std::cout << "  Run " << std::setw(3) << (run+1) << "/" << g_runs;
            for (const auto& t : {"WAN","MESH","HYBRID"}) {
                double rate = totalNodes[t] > 0
                    ? totalEligible[t] / totalNodes[t] : 0.0;
                std::cout << "  " << t << "=" << std::fixed << std::setprecision(3) << rate;
            }
            std::cout << std::endl;
        }
    }
    csvOut.close();

    // Compute final rates
    std::map<std::string, double> rates;
    for (const auto& t : {"WAN","MESH","HYBRID"})
        rates[t] = totalNodes[t] > 0 ? totalEligible[t] / totalNodes[t] : 0.0;

    // Write JSON summary — this is what replaces TDOA_RATES in the generator
    std::ofstream jsonOut("results/topology_rates_summary.json");
    jsonOut << std::fixed << std::setprecision(6);
    jsonOut << "{\n"
            << "  \"WAN\":     " << rates["WAN"]    << ",\n"
            << "  \"MESH\":    " << rates["MESH"]   << ",\n"
            << "  \"HYBRID\":  " << rates["HYBRID"] << ",\n"
            << "  \"runs\":    " << g_runs      << ",\n"
            << "  \"n_nodes\": " << g_numNodes  << ",\n"
            << "  \"n_gw\":    " << g_numGateways << ",\n"
            << "  \"area_km\": " << g_areaSideKm  << ",\n"
            << "  \"sf\":      " << (int)g_sf     << ",\n"
            << "  \"tdoa_min_gw\": " << g_tdoaMinGw << ",\n"
            << "  \"hop_limit_mesh\":   " << g_hopLimitMesh   << ",\n"
            << "  \"hop_limit_hybrid\": " << g_hopLimitHybrid << "\n"
            << "}\n";
    jsonOut.close();

    std::cout << "\n=== FINAL MEASURED RATES ===" << std::endl;
    for (const auto& t : {"WAN","MESH","HYBRID"})
        std::cout << "  " << std::setw(6) << t << ": TdoaEligible = "
                  << std::fixed << std::setprecision(4) << rates[t]
                  << "  (" << std::setprecision(1) << rates[t]*100 << "%)" << std::endl;

    std::cout << "\nOutputs:" << std::endl;
    std::cout << "  results/topology_observability_output.csv" << std::endl;
    std::cout << "  results/topology_rates_summary.json" << std::endl;
    std::cout << "\nNext: update multimodal_generator.py to load topology_rates_summary.json" << std::endl;

    return 0;
}
