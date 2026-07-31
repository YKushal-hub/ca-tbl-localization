#include "lora-spatial-field-engine.h"

namespace ns3 {

LoRaSpatialFieldEngine::LoRaSpatialFieldEngine() {}

double LoRaSpatialFieldEngine::SmoothPerlinNoise2D(double x, double y, double scale, uint32_t seed) {
    if (scale <= 0.0) scale = 100.0;
    double nx = x / scale;
    double ny = y / scale;
    
    int ix = (int)std::floor(nx);
    int iy = (int)std::floor(ny);
    double fx = nx - ix;
    double fy = ny - iy;
    
    auto hash = [seed](int i, int j) -> double {
        uint64_t h = (uint64_t)seed + (uint64_t)i * 374761393ULL + (uint64_t)j * 668265263ULL;
        h = (h ^ (h >> 13)) * 1274126177ULL;
        return ((double)(h % 1000000ULL)) / 1000000.0;
    };
    
    double u = fx * fx * (3.0 - 2.0 * fx);
    double v = fy * fy * (3.0 - 2.0 * fy);
    
    double v00 = hash(ix, iy);
    double v10 = hash(ix + 1, iy);
    double v01 = hash(ix, iy + 1);
    double v11 = hash(ix + 1, iy + 1);
    
    double i1 = v00 * (1.0 - u) + v10 * u;
    double i2 = v01 * (1.0 - u) + v11 * u;
    return i1 * (1.0 - v) + i2 * v;
}

SpatialFieldState LoRaSpatialFieldEngine::GetSpatialState(Vector position, EnvironmentClass env, uint32_t seed) {
    SpatialFieldState res;
    
    double n1 = SmoothPerlinNoise2D(position.x, position.y, 150.0, seed + 1);
    double n2 = SmoothPerlinNoise2D(position.x, position.y, 40.0, seed + 2);
    double n3 = SmoothPerlinNoise2D(position.x, position.y, 10.0, seed + 3);
    
    double composite = std::clamp(0.6 * n1 + 0.3 * n2 + 0.1 * n3, 0.0, 1.0);
    
    double baseShadow = 10.0;
    switch (env) {
        case ENV_URBAN_DENSE: baseShadow = 14.0; break;
        case ENV_SUBURBAN: baseShadow = 8.0; break;
        case ENV_RURAL_OPEN: baseShadow = 5.0; break;
        default: baseShadow = 10.0; break;
    }
    
    res.shadowingFieldDb = (composite - 0.5) * 2.0 * baseShadow; // [-baseShadow, +baseShadow]
    res.foliageDensityField = SmoothPerlinNoise2D(position.x, position.y, 80.0, seed + 4);
    res.obstructionField = SmoothPerlinNoise2D(position.x, position.y, 50.0, seed + 5);
    res.urbanCanyonField = SmoothPerlinNoise2D(position.x, position.y, 120.0, seed + 6);
    res.multipathRichnessField = SmoothPerlinNoise2D(position.x, position.y, 60.0, seed + 7);
    
    return res;
}

} // namespace ns3
