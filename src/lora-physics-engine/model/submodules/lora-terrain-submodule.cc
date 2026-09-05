#include "lora-terrain-submodule.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

TerrainSubmodule::TerrainSubmodule() {}

double TerrainSubmodule::Hash2D(int x, int y, uint32_t seed) {
    uint64_t h = ((uint64_t)x * 374761393U + (uint64_t)y * 668265263U + seed) * 1274126177U;
    h = (h ^ (h >> 16)) * 2654435769U;
    h ^= h >> 16;
    return (double)(h & 0xFFFFFF) / (double)0xFFFFFF; // [0.0, 1.0]
}

double TerrainSubmodule::SmoothNoise(double x, double y, double scale, double amp, uint32_t seed) {
    double nx = x / scale;
    double ny = y / scale;
    int ix = (int)std::floor(nx);
    int iy = (int)std::floor(ny);
    double fx = nx - ix;
    double fy = ny - iy;
    
    // Smoothstep interpolation: 3*t^2 - 2*t^3
    double ux = fx * fx * (3.0 - 2.0 * fx);
    double uy = fy * fy * (3.0 - 2.0 * fy);
    
    double v00 = Hash2D(ix, iy, seed);
    double v10 = Hash2D(ix + 1, iy, seed);
    double v01 = Hash2D(ix, iy + 1, seed);
    double v11 = Hash2D(ix + 1, iy + 1, seed);
    
    double top = v00 * (1.0 - ux) + v10 * ux;
    double bot = v01 * (1.0 - ux) + v11 * ux;
    return amp * (top * (1.0 - uy) + bot * uy);
}

double TerrainSubmodule::GetProceduralElevation(double x, double y, uint32_t seed) {
    // 3 octaves of Fractal Brownian Motion (fBm) representing realistic procedural hills and valleys
    double e = SmoothNoise(x, y, 500.0, 1.0, seed) +
               SmoothNoise(x, y, 200.0, 0.4, seed + 1) +
               SmoothNoise(x, y, 75.0, 0.1, seed + 2);
    return (e - 0.75) * 10.0; // Scaled to nominal environmental irregularity centered around 0
}

TerrainResult TerrainSubmodule::ComputeTerrainPenaltyDb(
    Vector txPos, Vector rxPos,
    double txHeightM, double rxHeightM,
    const TerrainParams& params, uint32_t randomSeed,
    LoRaDistributionSampler& sampler)
{
    TerrainResult res;
    res.terrainPenaltyDb = 0.0;
    res.fresnelClearanceRatio = 1.0;
    res.terrainLosBlocked = false;
    res.knifeEdgeCount = 0;
    res.earthBulgM = 0.0;
    res.fresnelRadiusM = 0.0;
    res.fresnelVParameter = 0.0;
    
    if (!params.enabled) {
        return res;
    }
    
    double dx = txPos.x - rxPos.x;
    double dy = txPos.y - rxPos.y;
    double dz = txPos.z - rxPos.z;
    double totalDist = std::max(1.0, std::sqrt(dx*dx + dy*dy + dz*dz));
    
    // WAS hardcoded 868e6 here, applied to all three tiers. Now carried on
    // TerrainParams, which still defaults to 868e6 so nothing moves unless
    // --terrainPerTierFreq asks it to.
    double frequencyHz = (params.frequencyHz > 0.0) ? params.frequencyHz : 868e6;
    double lambda = 299792458.0 / frequencyHz;
    
    int nPoints = 30;
    double minClearanceRatio = 10.0;
    double worstV = -10.0;
    
    double groundTx = GetProceduralElevation(txPos.x, txPos.y, randomSeed);
    double groundRx = GetProceduralElevation(rxPos.x, rxPos.y, randomSeed);
    
    // Feature 2: Earth curvature correction (IEEE 9037503, ITU-R P.526-15)
    double horizontalDistM = std::sqrt(dx*dx + dy*dy);
    double gwHeight = std::max(txHeightM, rxHeightM);
    double nodeHeight = std::min(txHeightM, rxHeightM);
    
    // Step 1: Radio horizon distance
    double d_horizon_km = 4.12 * (std::sqrt(gwHeight) + std::sqrt(nodeHeight));
    double d_horizon_m  = d_horizon_km * 1000.0;
    
    res.earthBulgM = 0.0;
    if (horizontalDistM > d_horizon_m) {
        double a_e_m = 8495000.0; // Effective earth radius (4/3 * 6371km)
        double h_bulge = (horizontalDistM * horizontalDistM) / (8.0 * a_e_m);
        double h_obs = h_bulge - ((gwHeight + nodeHeight) / 2.0);
        if (h_obs > 0.0) {
            res.earthBulgM = h_obs; // Only positive obstruction beyond horizon
        }
    }
    
    for (int i = 1; i < nPoints; ++i) {
        double fraction = (double)i / (double)nPoints;
        double px = txPos.x - fraction * dx;
        double py = txPos.y - fraction * dy;
        
        // Ray height above baseline
        double rayH = (1.0 - fraction) * txHeightM + fraction * rxHeightM;
        // Baseline ground elevation
        double baseGround = (1.0 - fraction) * groundTx + fraction * groundRx;
        // Actual procedural ground elevation
        double actualGround = GetProceduralElevation(px, py, randomSeed);
        // Terrain deviation above baseline
        double terrDeviation = actualGround - baseGround;
        
        double clearance = rayH - terrDeviation;
        
        double d1 = fraction * totalDist;
        double d2 = (1.0 - fraction) * totalDist;
        double r_F1 = std::sqrt(lambda * d1 * d2 / totalDist);
        if (r_F1 < 0.1) r_F1 = 0.1;
        
        // Feature 2: Track Fresnel radius (first Fresnel zone) and midpoint V-parameter
        if (i == nPoints / 2) {  // Midpoint for representative Fresnel values
            res.fresnelRadiusM = r_F1;
        }
        double clearanceRatio = clearance / r_F1;
        if (clearanceRatio < minClearanceRatio) {
            minClearanceRatio = clearanceRatio;
        }
        
        if (clearance < 0.0) {
            res.terrainLosBlocked = true;
            res.knifeEdgeCount++;
            
            double hObstruction = -clearance;
            double v = hObstruction * std::sqrt(2.0 * totalDist / (lambda * d1 * d2));
            if (v > worstV) {
                worstV = v;
            }
        }
    }
    
    res.fresnelClearanceRatio = minClearanceRatio;
    res.fresnelVParameter = worstV;  // Feature 2: Track V-parameter for telemetry
    
    // If the Fresnel zone is obstructed (ratio < 0.6), calculate procedural diffraction penalty
    double clearanceLimit = (params.fresnelClearanceFraction > 0.0 ? params.fresnelClearanceFraction : 0.6);
    double proceduralLoss = 0.0;
    if (minClearanceRatio < clearanceLimit) {
        double v_proc = worstV;
        if (!res.terrainLosBlocked) {
            v_proc = -minClearanceRatio * std::sqrt(2.0);
        }
        if (v_proc >= 0.0) {
            proceduralLoss = -20.0 * std::log10(0.5 * std::exp(-0.95 * v_proc)); // Approx for simplicity here
            if (v_proc > 1.0) {
                double arg = 0.1184 - std::pow(0.38 - 0.1 * v_proc, 2);
                if (arg >= 0.0) {
                    double val = 0.4 - std::sqrt(arg);
                    if (val > 1e-4) proceduralLoss = -20.0 * std::log10(val);
                    else proceduralLoss = 13.0 + 20.0 * std::log10(v_proc);
                } else proceduralLoss = 13.0 + 20.0 * std::log10(v_proc);
            }
        }
    }
    
    // Feature 2: Macro earth curvature diffraction (ITU-R P.526-15)
    double macroTerrainLoss = 0.0;
    if (horizontalDistM > d_horizon_m) {
        double a_e_m = 8495000.0;
        double h_bulge = (horizontalDistM * horizontalDistM) / (8.0 * a_e_m);
        double h_obs = h_bulge - ((gwHeight + nodeHeight) / 2.0);
        if (h_obs > 0.0) {
            double nu = h_obs * std::sqrt(2.0 * horizontalDistM / (lambda * (horizontalDistM / 2.0) * (horizontalDistM / 2.0)));
            if (nu > 2.4) {
                macroTerrainLoss = 20.0 * std::log10(0.225 / nu);
            } else if (nu > 1.0) {
                double arg = std::max(0.0, 0.1184 - std::pow(0.38 - 0.1 * nu, 2));
                macroTerrainLoss = 20.0 * std::log10(0.4 - std::sqrt(arg));
            } else if (nu > 0.0) {
                macroTerrainLoss = 20.0 * std::log10(0.5 * std::exp(-0.95 * nu));
            } else if (nu > -0.78) {
                macroTerrainLoss = 20.0 * std::log10(0.5 - 0.62 * nu);
            }
            macroTerrainLoss = -macroTerrainLoss; // Convert to positive loss dB
            // DEBUG
            if (horizontalDistM > 60000.0) {
                // std::cout << "d=" << horizontalDistM << " d_horiz=" << d_horizon_m << " h_bulge=" << h_bulge << " h_obs=" << h_obs << " nu=" << nu << " loss=" << macroTerrainLoss << std::endl;
            }
        }
    }
    
    res.terrainPenaltyDb = std::max(proceduralLoss, macroTerrainLoss);
    return res;
}

} // namespace ns3
