#include "lora-region-submodule.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

SemanticRegionSubmodule::SemanticRegionSubmodule() {}

SemanticRegionInfo SemanticRegionSubmodule::GetRegionInfo(Vector position) {
    SemanticRegionInfo info;
    
    // We map spatial coordinates into distinct persistent semantic regions
    // using continuous bounding partitions along X coordinate
    double x = position.x;
    if (x < 500.0) {
        info.regionType = REGION_SUBURBAN_OPEN;
        info.regionName = "Suburban Open";
        info.additionalPathLossDb = 0.0;
        info.shadowingStddevDb = 5.8;
        info.delaySpreadMultiplier = 1.0;
        info.dopplerSpreadMultiplier = 1.0;
        info.losPersistenceProbability = 0.85;
        info.persistentClusterCount = 3;
    } else if (x < 1200.0) {
        info.regionType = REGION_DENSE_URBAN_CANYON;
        info.regionName = "Dense Urban Canyon";
        info.additionalPathLossDb = 12.0;
        info.shadowingStddevDb = 10.2;
        info.delaySpreadMultiplier = 2.5;
        info.dopplerSpreadMultiplier = 1.5;
        info.losPersistenceProbability = 0.15;
        info.persistentClusterCount = 8;
    } else if (x < 2000.0) {
        info.regionType = REGION_FOREST_CORRIDOR;
        info.regionName = "Forest Corridor";
        info.additionalPathLossDb = 18.0;
        info.shadowingStddevDb = 8.5;
        info.delaySpreadMultiplier = 1.8;
        info.dopplerSpreadMultiplier = 1.2;
        info.losPersistenceProbability = 0.30;
        info.persistentClusterCount = 5;
    } else if (x < 3000.0) {
        info.regionType = REGION_TUNNEL;
        info.regionName = "Tunnel";
        info.additionalPathLossDb = -5.0; // Waveguide effect
        info.shadowingStddevDb = 3.2;
        info.delaySpreadMultiplier = 4.0;
        info.dopplerSpreadMultiplier = 0.8;
        info.losPersistenceProbability = 0.95;
        info.persistentClusterCount = 12;
    } else {
        info.regionType = REGION_MOUNTAIN_RIDGE;
        info.regionName = "Mountain Ridge";
        info.additionalPathLossDb = 6.0;
        info.shadowingStddevDb = 12.5;
        info.delaySpreadMultiplier = 3.0;
        info.dopplerSpreadMultiplier = 2.0;
        info.losPersistenceProbability = 0.50;
        info.persistentClusterCount = 4;
    }
    
    return info;
}

} // namespace ns3
