#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/lora-building-entry-submodule.h"
#include "ns3/lora-wall-material-submodule.h"
#include "ns3/lora-floor-attenuation-submodule.h"
#include "ns3/lora-node-position-uncertainty-submodule.h"
#include "ns3/lora-temporal-blockage-submodule.h"
#include "ns3/lora-body-shadowing-submodule.h"
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestTier2");

void TestIndoorTriad() {
    std::cout << "\n--- Tier 2: Indoor Triad ---" << std::endl;
    LoRaDistributionSampler sampler(42);
    
    BuildingEntryParams beParams = BuildingEntryParams::Defaults();
    WallMaterialParams wmParams = WallMaterialParams::Defaults();
    FloorAttenuationParams faParams = FloorAttenuationParams::Defaults();
    
    BuildingEntrySubmodule beOffice(beParams, ENV_INDOOR_OFFICE, sampler);
    WallMaterialSubmodule wmOffice(wmParams, ENV_INDOOR_OFFICE, sampler);
    FloorAttenuationSubmodule faOffice(faParams, ENV_INDOOR_OFFICE, sampler);
    
    LinkPhysicsState stateOffice;
    beOffice.Compute(stateOffice);
    wmOffice.Compute(stateOffice);
    faOffice.Compute(stateOffice);
    
    std::cout << "ENV_INDOOR_OFFICE: isIndoor=" << stateOffice.isIndoor 
              << ", buildingEntry=" << stateOffice.buildingEntryDb 
              << "dB, wall=" << stateOffice.wallPenetrationDb 
              << "dB, floor=" << stateOffice.floorAttenuationDb << "dB" << std::endl;
              
    BuildingEntrySubmodule beRural(beParams, ENV_RURAL_OPEN, sampler);
    WallMaterialSubmodule wmRural(wmParams, ENV_RURAL_OPEN, sampler);
    FloorAttenuationSubmodule faRural(faParams, ENV_RURAL_OPEN, sampler);
    
    LinkPhysicsState stateRural;
    beRural.Compute(stateRural);
    wmRural.Compute(stateRural);
    faRural.Compute(stateRural);
    
    std::cout << "ENV_RURAL: isIndoor=" << stateRural.isIndoor 
              << ", buildingEntry=" << stateRural.buildingEntryDb 
              << "dB, wall=" << stateRural.wallPenetrationDb 
              << "dB, floor=" << stateRural.floorAttenuationDb << "dB" << std::endl;
}

void TestUncertainty() {
    std::cout << "\n--- Tier 2: Node Position Uncertainty ---" << std::endl;
    LoRaDistributionSampler sampler(42);
    NodePositionUncertaintyParams params = NodePositionUncertaintyParams::Defaults();
    NodePositionUncertaintySubmodule submodule(params, ENV_URBAN_MODERATE, sampler);
    
    LinkPhysicsState state;
    submodule.Compute(state, 100.0);
    
    std::cout << "True Range: " << state.trueRangeM << "m, Error: " << state.rangeErrorM 
              << "m, Estimated Range: " << state.estimatedRangeM << "m" << std::endl;
}

void TestBlockageAndShadowing() {
    std::cout << "\n--- Tier 2: Blockage and Body Shadowing ---" << std::endl;
    LoRaDistributionSampler sampler(42);
    
    TemporalBlockageParams tbParams = TemporalBlockageParams::Defaults();
    BodyShadowingParams bsParams = BodyShadowingParams::Defaults();
    
    TemporalBlockageSubmodule tb(tbParams, ENV_URBAN_DENSE, sampler);
    BodyShadowingSubmodule bs(bsParams, ENV_URBAN_DENSE, sampler);
    
    double tbSum = 0.0;
    double bsSum = 0.0;
    int samples = 1000;
    int tbCount = 0;
    int bsCount = 0;
    
    for (int i=0; i<samples; ++i) {
        LinkPhysicsState state;
        state.isNLOS = false; // Blockage only happens if NOT already NLOS
        tb.Compute(state);
        bs.Compute(state);
        
        if (state.temporalBlockageDb > 0) {
            tbSum += state.temporalBlockageDb;
            tbCount++;
        }
        if (state.bodyShadowingDb > 0) {
            bsSum += state.bodyShadowingDb;
            bsCount++;
        }
    }
    
    std::cout << "Blockage occurred " << tbCount << "/" << samples << " times (Expected ~200 for URBAN_DENSE)." << std::endl;
    std::cout << "Average Blockage Loss (when occurs): " << (tbCount > 0 ? tbSum/tbCount : 0) << " dB" << std::endl;
    std::cout << "Body Shadowing occurred " << bsCount << "/" << samples << " times (Expected ~500)." << std::endl;
    std::cout << "Average Body Shadowing Loss (when occurs): " << (bsCount > 0 ? bsSum/bsCount : 0) << " dB" << std::endl;
}

int main(int argc, char* argv[]) {
    TestIndoorTriad();
    TestUncertainty();
    TestBlockageAndShadowing();
    return 0;
}
