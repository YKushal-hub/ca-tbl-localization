#include "ns3/core-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>

using namespace ns3;

int main(int argc, char* argv[]) {
    LoRaPhysicsConfig cfgGw = LoRaPhysicsConfig::ForScenario(
        ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST,
        MOB_STATIC, true);

    LoRaPhysicsConfig cfgNode = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST,
        MOB_STATIC, true);

    std::cout << "ENV_RURAL_FORESTED PLE: " << cfgGw.propagation.pathLossExponent.mean << std::endl;
    std::cout << "ENV_SUBURBAN PLE: " << cfgNode.propagation.pathLossExponent.mean << std::endl;

    return 0;
}
