#include "ns3/core-module.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/lora-physics-parameters.h"
#include <iostream>

using namespace ns3;

int main(int argc, char *argv[])
{
    LoRaDistributionSampler sampler(42);
    DistributionParam param = DistributionParam::LogNormal(std::log(0.01), 0.5, 0.001, 0.05);
    std::cout << "mean: " << param.mean << std::endl;
    for(int i=0; i<10; i++) {
        double val = sampler.Sample(param);
        std::cout << "Sampled duty cycle: " << val << std::endl;
    }
    return 0;
}
