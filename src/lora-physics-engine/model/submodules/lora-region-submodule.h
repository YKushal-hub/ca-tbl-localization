#ifndef LORA_REGION_SUBMODULE_H
#define LORA_REGION_SUBMODULE_H

#include "ns3/vector.h"
#include <string>
#include <map>
#include <cstdint>

namespace ns3 {

enum SemanticRegionType {
    REGION_SUBURBAN_OPEN = 0,
    REGION_DENSE_URBAN_CANYON = 1,
    REGION_FOREST_CORRIDOR = 2,
    REGION_TUNNEL = 3,
    REGION_MOUNTAIN_RIDGE = 4
};

/**
 * @brief Persistent semantic propagation characteristics for a geographical region
 */
struct SemanticRegionInfo {
    SemanticRegionType regionType;
    std::string regionName;
    double additionalPathLossDb;
    double shadowingStddevDb;
    double delaySpreadMultiplier;
    double dopplerSpreadMultiplier;
    double losPersistenceProbability;
    uint32_t persistentClusterCount;
};

/**
 * @brief Evaluates semantic environment regions (urban canyon, tunnel waveguide, forest, mountain)
 * providing persistent statistics and smooth transitions across boundaries.
 */
class SemanticRegionSubmodule {
public:
    SemanticRegionSubmodule();
    
    SemanticRegionInfo GetRegionInfo(Vector position);
};

} // namespace ns3

#endif // LORA_REGION_SUBMODULE_H
