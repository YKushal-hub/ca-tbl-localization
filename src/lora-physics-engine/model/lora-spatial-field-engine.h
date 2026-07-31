#ifndef LORA_SPATIAL_FIELD_ENGINE_H
#define LORA_SPATIAL_FIELD_ENGINE_H

#include "ns3/vector.h"
#include "lora-physics-parameters.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

/**
 * @brief Continuous procedural global environmental field maps
 */
struct SpatialFieldState {
    double shadowingFieldDb;
    double foliageDensityField;
    double obstructionField;
    double urbanCanyonField;
    double multipathRichnessField;
};

/**
 * @brief Emulates spatially continuous RF propagation environments using fractal Perlin noise
 */
class LoRaSpatialFieldEngine {
public:
    LoRaSpatialFieldEngine();
    
    SpatialFieldState GetSpatialState(Vector position, EnvironmentClass env, uint32_t seed);
    
private:
    double SmoothPerlinNoise2D(double x, double y, double scale, uint32_t seed);
};

} // namespace ns3

#endif // LORA_SPATIAL_FIELD_ENGINE_H
