#include "ns3/core-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>

using namespace ns3;

int main(int argc, char* argv[])
{
    LoRaPhysicsEngineHelper helper;
    LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(
        ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST,
        MOB_STATIC, false);
    
    cfg.nlos.nlosScaleDistanceM = DistributionParam::Fixed(8000.0);
    helper.Initialize(cfg);
    
    std::cout << "Rural Forested 14 dBm Range Test (Gateway H:25m, Node H:1.5m)" << std::endl;
    for (double d = 1000; d <= 20000; d += 1000) {
        uint32_t received = 0;
        uint32_t total = 1000;
        for(uint32_t i=0; i<total; ++i) {
            PhysicsResult r = helper.Process(
                i+1, 0,
                Vector(0, 0, 1.5), Vector(d, 0, 25.0),
                14.0, 12, 125000.0, 0, Seconds(i * 0.1)
            );
            if(r.packetReceived) received++;
        }
        std::cout << "Dist: " << d << "m -> PDR: " << ((double)received/total)*100.0 << "%" << std::endl;
    }
    
    return 0;
}
