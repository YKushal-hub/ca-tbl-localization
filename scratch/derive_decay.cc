
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>
#include <vector>
#include <random>

using namespace ns3;

static uint32_t g_numNodes     = 500;
static uint32_t g_numGateways  = 10;
static double   g_areaSideKm   = 50.0;
static uint8_t  g_sf          = 12;
static double   g_txPowerDbm  = 14.0;
static double   g_bwHz        = 125000.0;

struct Point2D { double x, y, heightM; };

std::vector<Point2D> PlaceUniform(uint32_t n, double sideM, double heightM, std::mt19937& rng) {
    std::uniform_real_distribution<double> uni(0.0, sideM);
    std::vector<Point2D> pts;
    for (uint32_t i = 0; i < n; ++i) pts.push_back({uni(rng), uni(rng), heightM});
    return pts;
}

int main(int argc, char *argv[]) {
    CommandLine cmd;
    double p2 = 0.55;
    cmd.AddValue("p2", "Hop 1->2 forward probability", p2);
    cmd.Parse(argc, argv);

    std::mt19937 rng(42);
    LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
        ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_DENSE_FOREST, MOB_STATIC, true);
    LoRaPhysicsEngineHelper helper;
    helper.Initialize(config);

    auto nodes = PlaceUniform(g_numNodes, g_areaSideKm * 1000.0, 1.5, rng);

    // Build adjacency
    std::vector<std::vector<bool>> adj(g_numNodes, std::vector<bool>(g_numNodes, false));
    int edgeCount = 0;
    for (uint32_t i = 0; i < g_numNodes; ++i) {
        for (uint32_t j = i + 1; j < g_numNodes; ++j) {
            Vector tx(nodes[i].x, nodes[i].y, nodes[i].heightM);
            Vector rx(nodes[j].x, nodes[j].y, nodes[j].heightM);
            PhysicsResult res = helper.Process(i, j, tx, rx, g_txPowerDbm, g_sf, g_bwHz, 0, Seconds(0));
            if (res.packetReceived) {
                adj[i][j] = true;
                adj[j][i] = true;
                edgeCount += 2;
            }
        }
    }
    
    // Simulate stochastic BFS for all nodes
    long total_forwards = 0;
    for (uint32_t start = 0; start < g_numNodes; ++start) {
        std::vector<int> dist(g_numNodes, -1);
        std::vector<uint32_t> queue;
        dist[start] = 0;
        queue.push_back(start);
        
        uint32_t forwards = 0;
        for (uint32_t qi = 0; qi < queue.size(); ++qi) {
            uint32_t cur = queue[qi];
            int hop = dist[cur];
            
            // Check if we forward
            if (hop == 1) {
                std::uniform_real_distribution<double> u(0.0, 1.0);
                if (u(rng) > 0.55) continue; // 45% drop
                forwards++;
            } else if (hop == 2) {
                std::uniform_real_distribution<double> u(0.0, 1.0);
                if (u(rng) > p2) continue;
                forwards++;
            } else if (hop > 2) {
                continue;
            } else if (hop == 0) {
                forwards++; // original transmission
            }

            // Expand
            if (hop < 2) {
                for (uint32_t nb = 0; nb < g_numNodes; ++nb) {
                    if (adj[cur][nb] && dist[nb] < 0) {
                        dist[nb] = hop + 1;
                        queue.push_back(nb);
                    }
                }
            }
        }
        total_forwards += forwards;
    }

    std::cout << "AvgDegree=" << (double)edgeCount / g_numNodes << " Forwards=" << total_forwards << std::endl;
    return 0;
}
