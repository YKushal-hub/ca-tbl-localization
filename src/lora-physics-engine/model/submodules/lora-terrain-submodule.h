#ifndef LORA_TERRAIN_SUBMODULE_H
#define LORA_TERRAIN_SUBMODULE_H

#include "ns3/vector.h"
#include "../lora-physics-parameters.h"
#include "../lora-distribution-sampler.h"

namespace ns3 {

/**
 * @brief Output container for geometric terrain obstruction and diffraction
 */
struct TerrainResult {
    double terrainPenaltyDb;
    double fresnelClearanceRatio;
    bool terrainLosBlocked;
    uint32_t knifeEdgeCount;
    
    // Feature 2: Earth curvature and Fresnel zone output
    double earthBulgM;
    double fresnelRadiusM;
    double fresnelVParameter;
};

/**
 * @brief Computes terrain-induced knife-edge diffraction loss and Fresnel clearance
 * using a procedural elevation map abstraction (Fractal Brownian Motion).
 */
class TerrainSubmodule {
public:
    TerrainSubmodule();
    
    double GetProceduralElevation(double x, double y, uint32_t seed);
    
    TerrainResult ComputeTerrainPenaltyDb(
        Vector txPos, Vector rxPos,
        double txHeightM, double rxHeightM,
        const TerrainParams& params, uint32_t randomSeed,
        LoRaDistributionSampler& sampler);
        
private:
    double Hash2D(int x, int y, uint32_t seed);
    double SmoothNoise(double x, double y, double scale, double amp, uint32_t seed);
};

} // namespace ns3

#endif // LORA_TERRAIN_SUBMODULE_H
