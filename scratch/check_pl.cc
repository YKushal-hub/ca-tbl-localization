#include "ns3/core-module.h"
#include "ns3/lora-physics-engine-helper.h"
#include <iostream>
#include <iomanip>

using namespace ns3;

int main(int argc, char* argv[]) {
    // 1. Check ENV_SUBURBAN
    LoRaPhysicsConfig cfgSub = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST,
        MOB_STATIC, false);
    
    // Disable other loss components to isolate path loss if needed, but ProcessTransmission computes result.pathLossDb directly.
    cfgSub.shadowing.enabled = false;
    cfgSub.fading.model = FADING_NONE;
    
    Ptr<LoRaPhysicsEngine> engineSub = CreateObject<LoRaPhysicsEngine>();
    engineSub->Initialize(cfgSub);
    
    Vector tx(0, 0, 1.5);
    Vector rxSub(24400, 0, 1.5); // 24.4 km
    
    PhysicsResult resSub = engineSub->ProcessTransmission(
        1, 2, tx, rxSub, 14.0, 12, 125000.0, 0, Seconds(0), 0.0);

    // 2. Check ENV_RURAL_FORESTED
    LoRaPhysicsConfig cfgRur = LoRaPhysicsConfig::ForScenario(
        ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST,
        MOB_STATIC, false);
    
    cfgRur.shadowing.enabled = false;
    cfgRur.fading.model = FADING_NONE;
    
    Ptr<LoRaPhysicsEngine> engineRur = CreateObject<LoRaPhysicsEngine>();
    engineRur->Initialize(cfgRur);
    
    Vector rxRur(24400, 0, 30.0); // rural WAN uses 30m gateway height typically, but let's test 1.5m to match pure d=24.4km if we want pure path loss, or 30m?
    // Wait, the path loss depends on height if two-ray ground is used, but we want to see the 124.2 vs 151.84 vs 132.10.
    // The Friis part is independent of height. Let's use 1.5m for rxRur to match the MESH test or just keep it simple.
    Vector rxRur2(24400, 0, 1.5);

    PhysicsResult resRur = engineRur->ProcessTransmission(
        3, 4, tx, rxRur2, 14.0, 12, 125000.0, 0, Seconds(0), 0.0);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "ENV_SUBURBAN at 24.4km:" << std::endl;
    std::cout << "  pathLossDb = " << resSub.pathLossDb << " dB" << std::endl;
    std::cout << "  totalLossDb = " << resSub.totalLossDb << " dB" << std::endl;
    
    std::cout << "ENV_RURAL_FORESTED at 24.4km:" << std::endl;
    std::cout << "  pathLossDb = " << resRur.pathLossDb << " dB" << std::endl;
    std::cout << "  totalLossDb = " << resRur.totalLossDb << " dB" << std::endl;
    
    return 0;
}
