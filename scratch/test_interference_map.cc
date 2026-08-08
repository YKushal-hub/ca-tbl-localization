#include "ns3/core-module.h"
#include "ns3/lora-interference-submodule.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/lora-physics-parameters.h"
#include <iostream>

using namespace ns3;

int main(int argc, char *argv[]) {
    CommandLine cmd;
    cmd.Parse(argc, argv);
    
    LoRaDistributionSampler sampler(42);
    InterferenceParams params = InterferenceParams::DefaultsFor(ENV_URBAN_MODERATE);
    
    // Create submodule
    InterferenceSubmodule interference;
    
    std::cout << "Testing Interference Map Initialization..." << std::endl;
    std::cout << "Params externalInterfererDensityPerKm2 Mean: " << params.externalInterfererDensityPerKm2.mean << std::endl;
    std::cout << "Params externalInterfererDensityPerKm2 Std: " << params.externalInterfererDensityPerKm2.stddev << std::endl;
    
    // Manually trigger initialization via ComputeInterferencePenaltyDb (which calls InitializeInterfererMap)
    double penalty = interference.ComputeInterferencePenaltyDb(
        1, 7, 125000.0,
        Seconds(0), Seconds(0.1),
        Vector(100,0,0), Vector(0,0,0), Seconds(0),
        params, sampler,
        -100.0, -120.0
    );
    
    std::cout << "Penalty Computed: " << penalty << " dB" << std::endl;
    
    return 0;
}
