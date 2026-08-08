
/**
 * topology_observability_sim.cc
 *
 * PURPOSE: Generate the 10-Fold geometric dataset for TDOA and Stage-2 fusion.
 */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lora-physics-engine-helper.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <iomanip>
#include <filesystem>

using namespace ns3;

static uint32_t g_numNodes     = 500;
static uint32_t g_numGateways  = 10;
static double   g_areaSideKm   = 50.0;
static uint32_t g_runs         = 1000;
static uint32_t g_hopLimitMesh = 2;
static uint32_t g_hopLimitHybrid = 3;
static uint32_t g_tdoaMinGw   = 3;
static uint8_t  g_sf          = 12;
static double   g_txPowerDbm  = 14.0;
static double   g_bwHz        = 125000.0;
static double   g_nodeHeightM = 1.5;
static double   g_gwHeightM   = 25.0;

struct Point2D {
    double x, y;
    double heightM;
};

std::vector<Point2D> PlaceUniform(uint32_t n, double sideM, double heightM, std::mt19937& rng) {
    std::uniform_real_distribution<double> uni(0.0, sideM);
    std::vector<Point2D> pts;
    pts.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
        pts.push_back({uni(rng), uni(rng), heightM});
    return pts;
}

double Dist2D(const Point2D& a, const Point2D& b) {
    double dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx*dx + dy*dy);
}

PhysicsResult EvalLinkResult(LoRaPhysicsEngineHelper& helper,
                             const Point2D& tx, const Point2D& rx,
                             uint32_t txNodeId, uint32_t rxNodeId,
                             Time txTime) {
    Vector txVec(tx.x, tx.y, tx.heightM);
    Vector rxVec(rx.x, rx.y, rx.heightM);
    PhysicsResult r = helper.Process(
        txNodeId, rxNodeId,
        txVec, rxVec,
        g_txPowerDbm, g_sf, g_bwHz,
        0,
        txTime
    );
    return r;
}

// MESH reachability using exact physics results and stochastic hop decay
// Returns a list of Gateway IDs that are reachable.
std::vector<int> MeshGwReach(uint32_t startNode,
                             const std::vector<std::vector<bool>>& nodeAdj,
                             const std::vector<std::vector<int>>& nodeToGwDirect,
                             uint32_t hopLimit,
                             std::mt19937& rng,
                             double hop2DropProb,
                             long& totalForwards) {
    uint32_t n = nodeAdj.size();
    std::vector<int> dist(n, -1);
    std::vector<uint32_t> queue;
    dist[startNode] = 0;
    queue.push_back(startNode);
    
    std::vector<int> reachedGws;
    for (int gw : nodeToGwDirect[startNode]) {
        reachedGws.push_back(gw);
    }

    std::uniform_real_distribution<double> u(0.0, 1.0);

    for (uint32_t qi = 0; qi < queue.size(); ++qi) {
        uint32_t cur = queue[qi];
        int hop = dist[cur];
        
        // Probability to forward (Stochastic hop-decay)
        if (hop == 1) {
            if (u(rng) > 0.55) continue; // 45% drop probability
            totalForwards++;
        } else if (hop == 2) {
            int degree = std::count(nodeAdj[cur].begin(), nodeAdj[cur].end(), true);
            double localDropProb = std::min(1.0, 0.0214 * degree);
            if (u(rng) > (1.0 - localDropProb)) continue;
            totalForwards++;
        } else if (hop == 0) {
            totalForwards++;
        } else if (hop > 2) {
            continue;
        }

        if ((uint32_t)hop >= hopLimit) continue;
        
        for (uint32_t nb = 0; nb < n; ++nb) {
            if (nodeAdj[cur][nb] && dist[nb] < 0) {
                dist[nb] = hop + 1;
                queue.push_back(nb);
                for (int gw : nodeToGwDirect[nb]) {
                    if (std::find(reachedGws.begin(), reachedGws.end(), gw) == reachedGws.end()) {
                        reachedGws.push_back(gw);
                    }
                }
            }
        }
    }
    return reachedGws;
}

struct ReachedGateway {
    int id;
    double x;
    double y;
    double arrivalNs; // timingSyncErrorNs + true propagation time if we wanted, but Python adds propagation!
    // Python TDOA solver expects measured arrivals. We'll just output the timing error directly, 
    // and Python will compute True Arrival + Error.
    double timeSyncErrorNs;
};

struct NodeResult {
    uint32_t fold;
    uint32_t run;
    std::string topology;
    uint32_t nodeId;
    double nodeX, nodeY;
    int gwObsCount;
    int tdoaEligible;
    double u_lambda;
    
    // Stage 2
    double dist25, nlos25;
    double dist50, nlos50;
    double dist100, nlos100;
    
    // Gateways (up to 10)
    std::vector<ReachedGateway> gws;
};

std::vector<NodeResult> RunOneTrial(
    uint32_t runIdx, std::mt19937& rng,
    LoRaPhysicsEngineHelper& helperGw,
    LoRaPhysicsEngineHelper& helperNode,
    LoRaPhysicsEngineHelper& helperStage2,
    double hop2DropProb,
    long& meshForwards,
    long& hybridForwards)
{
    uint32_t fold = runIdx / 100;
    double sideM = g_areaSideKm * 1000.0;
    auto nodes = PlaceUniform(g_numNodes, sideM, g_nodeHeightM, rng);
    auto gws   = PlaceUniform(g_numGateways, sideM, g_gwHeightM, rng);

    std::uniform_real_distribution<double> uni_time(0.0, 3600.0);
    std::vector<Time> nodeTxTimes(g_numNodes);
    for (uint32_t i = 0; i < g_numNodes; ++i) {
        nodeTxTimes[i] = Seconds(uni_time(rng));
    }

    std::vector<std::vector<PhysicsResult>> nodeGwPhysics(g_numNodes, std::vector<PhysicsResult>(g_numGateways));
    std::vector<std::vector<int>> nodeGwDirectIds(g_numNodes);

    for (uint32_t i = 0; i < g_numNodes; ++i) {
        for (uint32_t j = 0; j < g_numGateways; ++j) {
            PhysicsResult r = EvalLinkResult(helperGw, nodes[i], gws[j], i + 1, g_numNodes + j + 1, nodeTxTimes[i]);
            nodeGwPhysics[i][j] = r;
            if (r.packetReceived) {
                nodeGwDirectIds[i].push_back(j);
            }
        }
    }

    std::vector<std::vector<bool>> nodeAdj(g_numNodes, std::vector<bool>(g_numNodes, false));
    const double MAX_NODE_RANGE_M = 25000.0;
    for (uint32_t i = 0; i < g_numNodes; ++i) {
        for (uint32_t j = i+1; j < g_numNodes; ++j) {
            double d = Dist2D(nodes[i], nodes[j]);
            if (d > MAX_NODE_RANGE_M) continue;
            PhysicsResult r = EvalLinkResult(helperNode, nodes[i], nodes[j], i + 1, j + 1, nodeTxTimes[i]);
            nodeAdj[i][j] = nodeAdj[j][i] = r.packetReceived;
        }
    }

    std::vector<NodeResult> results;
    results.reserve(g_numNodes * 3);
    
    std::uniform_real_distribution<double> uni_u(0.000001, 0.999999);

    for (uint32_t i = 0; i < g_numNodes; ++i) {
        // Shared Stage 2 randomness for this node across topologies
        double u_lambda = uni_u(rng);
        
        // Compute distances for lambdas 25, 50, 100
        double dist25 = std::sqrt(-std::log(u_lambda) / (M_PI * 25.0)) * 1000.0;
        double dist50 = std::sqrt(-std::log(u_lambda) / (M_PI * 50.0)) * 1000.0;
        double dist100 = std::sqrt(-std::log(u_lambda) / (M_PI * 100.0)) * 1000.0;
        
        Point2D p25 = {nodes[i].x + dist25, nodes[i].y, g_gwHeightM}; // Assume beacon is at 25m? Or 1.5m? Let's use 1.5m.
        Point2D p50 = {nodes[i].x + dist50, nodes[i].y, g_gwHeightM};
        Point2D p100 = {nodes[i].x + dist100, nodes[i].y, g_gwHeightM};
        
        PhysicsResult r25 = EvalLinkResult(helperStage2, nodes[i], p25, i + 1, g_numNodes + g_numGateways + 1, nodeTxTimes[i]);
        PhysicsResult r50 = EvalLinkResult(helperStage2, nodes[i], p50, i + 1, g_numNodes + g_numGateways + 2, nodeTxTimes[i]);
        PhysicsResult r100 = EvalLinkResult(helperStage2, nodes[i], p100, i + 1, g_numNodes + g_numGateways + 3, nodeTxTimes[i]);

        for (const std::string& topo : {"WAN", "MESH", "HYBRID"}) {
            std::vector<int> reachedGwIds;
            if (topo == "WAN") {
                reachedGwIds = nodeGwDirectIds[i];
            } else if (topo == "MESH") {
                reachedGwIds = MeshGwReach(i, nodeAdj, nodeGwDirectIds, g_hopLimitMesh, rng, hop2DropProb, meshForwards);
            } else { // HYBRID
                if (nodeGwDirectIds[i].size() > 0) {
                    reachedGwIds = nodeGwDirectIds[i]; // Direct WAN anchor
                } else {
                    reachedGwIds = MeshGwReach(i, nodeAdj, nodeGwDirectIds, g_hopLimitHybrid, rng, hop2DropProb, hybridForwards);
                }
            }
            
            // Limit to max 10 gateways just to be safe
            if (reachedGwIds.size() > g_numGateways) {
                reachedGwIds.resize(g_numGateways);
            }

            int gwObs = reachedGwIds.size();
            // Multi-Hop TDOA Fallacy fix: TDOA eligibility ONLY relies on direct gateways!
            int eligible = (nodeGwDirectIds[i].size() >= g_tdoaMinGw) ? 1 : 0;
            
            NodeResult res;
            res.fold = fold;
            res.run = runIdx;
            res.topology = topo;
            res.nodeId = i;
            res.nodeX = nodes[i].x;
            res.nodeY = nodes[i].y;
            res.gwObsCount = gwObs;
            res.tdoaEligible = eligible;
            res.u_lambda = u_lambda;
            res.dist25 = dist25;
            res.nlos25 = r25.nlosDb;
            res.dist50 = dist50;
            res.nlos50 = r50.nlosDb;
            res.dist100 = dist100;
            res.nlos100 = r100.nlosDb;
            
            // Fix 2: Multi-Hop TDOA Fallacy fix - ONLY output direct gateways for TDOA localization.
            // Relayed gateways provide coverage but their timing uncertainty is destroyed by the hop.
            for (int gwId : nodeGwDirectIds[i]) {
                ReachedGateway rg;
                rg.id = gwId;
                rg.x = gws[gwId].x;
                rg.y = gws[gwId].y;
                // Add the exact timing error realization from the gateway link
                rg.timeSyncErrorNs = nodeGwPhysics[i][gwId].timingSyncErrorNs;
                res.gws.push_back(rg);
            }
            results.push_back(res);
        }
    }
    return results;
}

int main(int argc, char* argv[])
{
    uint32_t startRun = 0;
    uint32_t endRun = g_runs;
    double hop2DropProb = 0.0;
    
    CommandLine cmd;
    cmd.AddValue("startRun", "Start run index", startRun);
    cmd.AddValue("endRun", "End run index (exclusive)", endRun);
    cmd.AddValue("areaSideKm", "Area side in km", g_areaSideKm);
    cmd.AddValue("hop2DropProb", "Hop 1->2 drop probability", hop2DropProb);
    cmd.Parse(argc, argv);
    
    std::string outName = "results/geometric_training_dataset_" + std::to_string(startRun) + "_" + std::to_string(endRun) + ".csv";
    
    std::cout << "Starting Generation: Runs " << startRun << " to " << (endRun - 1) << "..." << std::endl;
    std::cout << "Config: area=" << g_areaSideKm << "km, hop2DropProb=" << hop2DropProb << std::endl;
    std::filesystem::create_directories("results");

    LoRaPhysicsConfig cfgGw   = LoRaPhysicsConfig::ForScenario(ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST, MOB_STATIC, true);
    cfgGw.verboseLogging = false;
    LoRaPhysicsConfig cfgNode = LoRaPhysicsConfig::ForScenario(ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST, MOB_STATIC, true);
    cfgNode.verboseLogging = false;
    
    // Stage 2 beacon config (indoor/outdoor mixed, short range)
    LoRaPhysicsConfig cfgStage2 = LoRaPhysicsConfig::ForScenario(ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST, MOB_STATIC, true);
    cfgStage2.verboseLogging = false;

    std::ofstream csvOut(outName);
    csvOut << "Fold,Run,Topology,NodeId,NodeX_m,NodeY_m,GwObsCount,TdoaEligible,u_lambda,"
           << "Dist25,Nlos25,Dist50,Nlos50,Dist100,Nlos100";
    for (int k=1; k<=10; ++k) {
        csvOut << ",Gw" << k << "Id,Gw" << k << "X,Gw" << k << "Y,Gw" << k << "ErrNs";
    }
    csvOut << "\n";

    int total_wan = 0, eligible_wan = 0;
    int total_mesh = 0, eligible_mesh = 0;
    int total_hybrid = 0, eligible_hybrid = 0;

    uint32_t simClock = 1;
    for (uint32_t run = startRun; run < endRun; ++run) {
        std::mt19937 rng(run * 12345 + 67890);
        
        // Initialize helpers once per run to preserve spatial correlation within the run
        LoRaPhysicsEngineHelper helperGw, helperNode, helperStage2;
        helperGw.Initialize(cfgGw);
        helperNode.Initialize(cfgNode);
        helperStage2.Initialize(cfgStage2);

        long meshForwards = 0;
        long hybridForwards = 0;
        auto results = RunOneTrial(run, rng, helperGw, helperNode, helperStage2, hop2DropProb, meshForwards, hybridForwards);
        
        // Print forwards periodically if doing a small run test
        if (endRun - startRun <= 5) {
            std::cout << "[Run " << run << "] MeshForwards=" << meshForwards << " HybridForwards=" << hybridForwards << std::endl;
        }

        for (const auto& r : results) {
            if (r.topology == "WAN") { total_wan++; if (r.tdoaEligible) eligible_wan++; }
            if (r.topology == "MESH") { total_mesh++; if (r.tdoaEligible) eligible_mesh++; }
            if (r.topology == "HYBRID") { total_hybrid++; if (r.tdoaEligible) eligible_hybrid++; }
            
            csvOut << r.fold << "," << r.run << "," << r.topology << "," << r.nodeId << ","
                   << std::fixed << std::setprecision(1)
                   << r.nodeX << "," << r.nodeY << ","
                   << r.gwObsCount << "," << r.tdoaEligible << "," << std::setprecision(6) << r.u_lambda << ","
                   << std::setprecision(2)
                   << r.dist25 << "," << r.nlos25 << ","
                   << r.dist50 << "," << r.nlos50 << ","
                   << r.dist100 << "," << r.nlos100;
                   
            for (size_t k = 0; k < 10; ++k) {
                if (k < r.gws.size()) {
                    csvOut << "," << r.gws[k].id << "," << r.gws[k].x << "," << r.gws[k].y << "," << r.gws[k].timeSyncErrorNs;
                } else {
                    csvOut << ",NaN,NaN,NaN,NaN";
                }
            }
            csvOut << "\n";
        }

        if ((run+1) % 10 == 0) {
            std::cout << "  Completed Run " << (run+1) << "/" << g_runs << std::endl;
        }
    }
    csvOut.close();
    
    std::cout << "Dataset generation complete: results/geometric_training_dataset.csv" << std::endl;
    return 0;
}
