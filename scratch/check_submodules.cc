#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include <iostream>
#include <iomanip>

using namespace ns3;

void PrintSubmoduleEffect(std::string name, double lossOff, double lossOn) {
    std::cout << name << ": OFF=" << lossOff << " dB, ON=" << lossOn 
              << " dB. Measurable change? " << (std::abs(lossOff - lossOn) > 1e-6 ? "YES" : "NO") << std::endl;
}

int main(int argc, char* argv[]) {
    LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST, MOB_STATIC, false);
    
    // Disable random stochasticity to get deterministic baseline for toggle check
    cfg.randomSeed = 12345;
    
    // Baseline
    cfg.bodyShadowing.enabled = false;
    cfg.temporalBlockage.enabled = false;
    cfg.nodePositionUncertainty.enabled = false;
    
    Ptr<LoRaPhysicsEngine> engineBase = CreateObject<LoRaPhysicsEngine>();
    engineBase->Initialize(cfg);
    
    Vector tx(0, 0, 1.5);
    Vector rx(1000, 0, 25.0);
    
    PhysicsResult resBase = engineBase->ProcessTransmission(
        1, 2, tx, rx, 14.0, 12, 125000.0, 0, Seconds(0), 0.0);

    // 1. Body Shadowing
    cfg.bodyShadowing.enabled = true;
    cfg.bodyShadowing.nodeIsBodyWorn = true; // Turn it on
    Ptr<LoRaPhysicsEngine> engineBody = CreateObject<LoRaPhysicsEngine>();
    engineBody->Initialize(cfg);
    PhysicsResult resBody = engineBody->ProcessTransmission(
        1, 2, tx, rx, 14.0, 12, 125000.0, 0, Seconds(0), M_PI);
    
    // 2. Temporal Blockage
    cfg.bodyShadowing.enabled = false; // Reset
    // TemporalBlockage is usually computed externally in ml_dataset_generator.cc for INDUSTRIAL.
    // Let's test if it's evaluated here. Actually in ml_dataset_generator it is computed externally.
    // Is it computed internally in ProcessTransmission? The module might be part of engine.
    cfg.temporalBlockage.enabled = true;
    Ptr<LoRaPhysicsEngine> engineTemp = CreateObject<LoRaPhysicsEngine>();
    engineTemp->Initialize(cfg);
    PhysicsResult resTemp;
    for (int i = 0; i < 1000; i++) {
        // Change time to vary the seed internally if it depends on time, but sampler is initialized once.
        // Actually the sampler state advances on each sample! So just calling ProcessTransmission again works.
        resTemp = engineTemp->ProcessTransmission(1, 2, tx, rx, 14.0, 12, 125000.0, 0, Seconds(0), 0.0);
        if (resTemp.temporalBlockageDb > 0.0) break;
    }
    
    // 3. Node Position Uncertainty
    cfg.temporalBlockage.enabled = false; // Reset
    cfg.nodePositionUncertainty.enabled = true;
    cfg.nodePositionUncertainty.horizontalErrorM = DistributionParam::Fixed(200.0); // 200m uncertainty
    Ptr<LoRaPhysicsEngine> enginePos = CreateObject<LoRaPhysicsEngine>();
    enginePos->Initialize(cfg);
    PhysicsResult resPos = enginePos->ProcessTransmission(
        1, 2, tx, rx, 14.0, 12, 125000.0, 0, Seconds(0), 0.0);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "--- Submodule Toggle Test ---" << std::endl;
    PrintSubmoduleEffect("BodyShadowingLossDb", resBase.bodyShadowingDb, resBody.bodyShadowingDb);
    PrintSubmoduleEffect("TemporalBlockageLossDb", resBase.temporalBlockageDb, resTemp.temporalBlockageDb);
    PrintSubmoduleEffect("NodePositionUncertainty (BiasDb)", resBase.propagationLossBiasDb, resPos.propagationLossBiasDb);
    
    return 0;
}
