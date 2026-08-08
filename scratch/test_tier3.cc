#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include <iostream>

using namespace ns3;

void TestTier3Couplings() {
    std::cout << "--- Testing Tier 3 Submodules & Couplings ---" << std::endl;
    
    LoRaDistributionSampler sampler(12345);
    
    // 1. Test Tunnel Environment
    LoRaPhysicsConfig configTunnel = LoRaPhysicsConfig::ForScenario(ENV_TUNNEL_CONFINED, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
    configTunnel.randomSeed = 12345;
    LoRaPhysicsEngine engineTunnel;
    engineTunnel.Initialize(configTunnel);
    
    Vector tx(0, 0, 1.5);
    Vector rxTunnel(1000, 0, 1.5); // 1 km inside tunnel
    
    PhysicsResult resTunnel = engineTunnel.ProcessTransmission(1, 100, tx, rxTunnel, 14.0, 7, 125000, 0, Seconds(0));
    std::cout << "Tunnel Path Loss (1km): " << resTunnel.pathLossDb << " dB" << std::endl;
    
    // 2. Test Two-Ray Ground in Open Field
    LoRaPhysicsConfig configOpen = LoRaPhysicsConfig::ForScenario(ENV_RURAL_OPEN, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
    configOpen.randomSeed = 12345;
    LoRaPhysicsEngine engineOpen;
    engineOpen.Initialize(configOpen);
    
    Vector rxOpen(2000, 0, 1.5); // 2 km
    PhysicsResult resOpen = engineOpen.ProcessTransmission(1, 100, tx, rxOpen, 14.0, 7, 125000, 0, Seconds(0));
    std::cout << "Two-Ray Ground Path Loss (2km): " << resOpen.pathLossDb << " dB" << std::endl;
    
    // 3. Test Soil Attenuation in Rural
    LoRaPhysicsConfig configRural = LoRaPhysicsConfig::ForScenario(ENV_RURAL_OPEN, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
    configRural.randomSeed = 12345;
    // We assume burial 10% of time; run multiple to find a buried one
    LoRaPhysicsEngine engineRural;
    engineRural.Initialize(configRural);
    
    double maxLoss = 0;
    for (int i=0; i<50; i++) {
        PhysicsResult resRural = engineRural.ProcessTransmission(1, 100, tx, rxOpen, 14.0, 7, 125000, 0, Seconds(i));
        if (resRural.totalLossDb > maxLoss) maxLoss = resRural.totalLossDb;
    }
    std::cout << "Max Rural Total Loss (checking soil bursts): " << maxLoss << " dB" << std::endl;
    
    // 4. Test Coupling 4: Rain -> Foliage
    LoRaPhysicsConfig configRain = LoRaPhysicsConfig::ForScenario(ENV_RURAL_FORESTED, WEATHER_HEAVY_RAIN, FADING_RAYLEIGH, VEG_DENSE_FOREST, MOB_STATIC, false);
    configRain.foliage.enabled = true;
    configRain.randomSeed = 12345;
    LoRaPhysicsEngine engineRain;
    engineRain.Initialize(configRain);
    
    PhysicsResult resRain = engineRain.ProcessTransmission(1, 100, tx, rxOpen, 14.0, 7, 125000, 0, Seconds(0));
    std::cout << "Foliage Loss (Heavy Rain): " << resRain.foliageDb << " dB" << std::endl;
    
    LoRaPhysicsConfig configDry = LoRaPhysicsConfig::ForScenario(ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_DENSE_FOREST, MOB_STATIC, false);
    configDry.foliage.enabled = true;
    configDry.randomSeed = 12345;
    LoRaPhysicsEngine engineDry;
    engineDry.Initialize(configDry);
    
    PhysicsResult resDry = engineDry.ProcessTransmission(1, 100, tx, rxOpen, 14.0, 7, 125000, 0, Seconds(0));
    std::cout << "Foliage Loss (Dry): " << resDry.foliageDb << " dB" << std::endl;
    
    // 5. Test Coupling 5: Terrain -> Shadowing Correlation
    std::cout << "Coupling 5 implicit in shadowing variation when Terrain Los Blocked." << std::endl;
}

int main(int argc, char *argv[]) {
    TestTier3Couplings();
    return 0;
}
