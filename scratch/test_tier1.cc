#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/lora-shadowing-submodule.h"
#include "ns3/lora-fading-submodule.h"
#include "ns3/lora-delay-spread-submodule.h"
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestTier1");

void TestC1() {
    std::cout << "\n--- C1: Fading K-Factor in ENV_URBAN_DENSE ---" << std::endl;
    LoRaDistributionSampler sampler(42);
    FadingParams params = FadingParams::DefaultsFor(ENV_URBAN_DENSE);
    
    LinkPhysicsState stateLOS;
    stateLOS.isNLOS = false;
    double dummyLastVal = 0.0;
    Time dummyLastUpdate = Seconds(0);
    ComputeFadingDb(stateLOS, Seconds(1), params, sampler, 1, dummyLastUpdate, dummyLastVal);
    
    LinkPhysicsState stateNLOS;
    stateNLOS.isNLOS = true;
    ComputeFadingDb(stateNLOS, Seconds(2), params, sampler, 1, dummyLastUpdate, dummyLastVal);
    
    std::cout << "LOS K-factor: " << stateLOS.kFactor << " (Expected: 0)" << std::endl;
    std::cout << "NLOS K-factor: " << stateNLOS.kFactor << " (Expected: 0)" << std::endl;
}

void TestC2() {
    std::cout << "\n--- C2: Shadowing Sigma in ENV_SUBURBAN ---" << std::endl;
    LoRaDistributionSampler sampler(42);
    ShadowingParams params = ShadowingParams::DefaultsFor(ENV_SUBURBAN);
    ShadowingSubmodule shadowing(params, sampler);
    
    LinkPhysicsState stateLOS;
    stateLOS.isNLOS = false;
    shadowing.GetShadowingDb(stateLOS, 1, 2, Vector(0,0,0), Vector(100,0,0), Seconds(1));
    
    // Create new module so cache is empty and it samples again
    ShadowingSubmodule shadowing2(params, sampler);
    LinkPhysicsState stateNLOS;
    stateNLOS.isNLOS = true;
    shadowing2.GetShadowingDb(stateNLOS, 1, 2, Vector(0,0,0), Vector(100,0,0), Seconds(2));
    
    std::cout << "LOS effective sigma: " << stateLOS.shadowingStdEff << std::endl;
    std::cout << "NLOS effective sigma: " << stateNLOS.shadowingStdEff << std::endl;
    std::cout << "Ratio: " << stateNLOS.shadowingStdEff / stateLOS.shadowingStdEff << " (Expected: 1.35)" << std::endl;
}

void TestC3() {
    std::cout << "\n--- C3: Delay Spread in ENV_RURAL ---" << std::endl;
    LoRaDistributionSampler sampler(42);
    DelaySpreadParams params = DelaySpreadParams::DefaultsFor(ENV_RURAL_OPEN);
    DelaySpreadSubmodule delaySpread;
    
    double meanLos = 0.0;
    double meanNlos = 0.0;
    int samples = 1000;
    
    for (int i = 0; i < samples; ++i) {
        LinkPhysicsState stateLOS;
        stateLOS.isNLOS = false;
        delaySpread.ComputeDelaySpreadDynamic(stateLOS, ENV_RURAL_OPEN, 1000.0, 868e6, 125e3, params, sampler);
        meanLos += stateLOS.delaySpreadNs;
        
        LinkPhysicsState stateNLOS;
        stateNLOS.isNLOS = true;
        delaySpread.ComputeDelaySpreadDynamic(stateNLOS, ENV_RURAL_OPEN, 1000.0, 868e6, 125e3, params, sampler);
        meanNlos += stateNLOS.delaySpreadNs;
    }
    
    meanLos /= samples;
    meanNlos /= samples;
    
    std::cout << "Average LOS Delay Spread: " << meanLos << " ns (Expected: ~50 ns)" << std::endl;
    std::cout << "Average NLOS Delay Spread: " << meanNlos << " ns (Expected: ~150 ns)" << std::endl;
}

int main(int argc, char* argv[]) {
    TestC1();
    TestC2();
    TestC3();
    return 0;
}
