#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include <iostream>
#include <iomanip>

using namespace ns3;

int main(int argc, char* argv[]) {
    LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_LIGHT_FOREST, MOB_STATIC, false);
    
    // Enable submodules so their outputs are non-zero
    cfg.bodyShadowing.enabled = true;
    cfg.bodyShadowing.nodeIsBodyWorn = true;
    cfg.temporalBlockage.enabled = true;
    cfg.nodePositionUncertainty.enabled = true;
    cfg.nodePositionUncertainty.horizontalErrorM = DistributionParam::Normal(0.0, 500.0, -1500.0, 1500.0);
    
    Ptr<LoRaPhysicsEngine> engine = CreateObject<LoRaPhysicsEngine>();
    engine->Initialize(cfg);
    
    // Dist = ~5km to see real attenuation and blockage effects
    Vector tx(0, 0, 1.5);
    Vector rx(5000, 0, 25.0);
    
    PhysicsResult res;
    // Loop until we get a hit on temporal blockage which is stochastic (10% prob in suburban)
    for(int i=0; i<1000; i++) {
        res = engine->ProcessTransmission(1, 2, tx, rx, 14.0, 12, 125000.0, 0, Seconds(0), 0.0);
        if (res.temporalBlockageDb > 0.1) break;
    }

    std::cout << "\n=== PHASE C TELEMETRY OUTPUT EVIDENCE ===" << std::endl;
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "isTwoRay:              " << (res.isTwoRay ? "true" : "false") << std::endl;
    std::cout << "isTunnel:              " << (res.isTunnel ? "true" : "false") << std::endl;
    std::cout << "soilAttenuationDb:     " << res.soilAttenuationDb << " dB" << std::endl;
    std::cout << "estimatedRangeM:       " << res.estimatedRangeM << " m" << std::endl;
    std::cout << "rangeErrorM:           " << res.rangeErrorM << " m" << std::endl;
    std::cout << "positionErrorM:        " << res.positionErrorM << " m" << std::endl;
    std::cout << "propagationLossBiasDb: " << res.propagationLossBiasDb << " dB" << std::endl;
    std::cout << "sampledRainRateMmHr:   " << res.sampledRainRateMmHr << " mm/hr" << std::endl;
    std::cout << "isNLOS:                " << (res.isNLOS ? "true" : "false") << std::endl;
    std::cout << "kFactor:               " << res.kFactor << std::endl;
    std::cout << "shadowingStdEff:       " << res.shadowingStdEff << " dB" << std::endl;
    std::cout << "buildingEntryDb:       " << res.buildingEntryDb << " dB" << std::endl;
    std::cout << "wallPenetrationDb:     " << res.wallPenetrationDb << " dB" << std::endl;
    std::cout << "floorAttenuationDb:    " << res.floorAttenuationDb << " dB" << std::endl;
    std::cout << "bodyShadowingDb:       " << res.bodyShadowingDb << " dB" << std::endl;
    std::cout << "temporalBlockageDb:    " << res.temporalBlockageDb << " dB" << std::endl;

    return 0;
}
