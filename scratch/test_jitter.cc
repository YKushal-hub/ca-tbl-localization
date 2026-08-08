#include "ns3/core-module.h"
#include "ns3/lora-hardware-submodule.h"
#include <iostream>

using namespace ns3;
int main(int argc, char *argv[]) {
    LoRaDistributionSampler sampler(42);
    HardwareParams paramsFine = HardwareParams::DefaultsFineTimestamp();
    HardwareSubmodule hwFine(paramsFine, sampler);
    for (int i = 0; i < 10; ++i) {
        ns3::TimestampObservation obsFine = hwFine.GenerateTimestampObservation(1, i, Seconds(0), -100, 10, 7, 0);
        double jFine = obsFine.measuredArrivalTimeNs - obsFine.trueArrivalTimeNs;
        std::cout << "i=" << i << " jFine=" << jFine << std::endl;
    }
    return 0;
}
