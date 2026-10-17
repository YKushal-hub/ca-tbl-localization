// Gate G147 unit part (scope 2026-10-17, B6 --agcPerReceiver). Receiver A takes a strong frame (-20 dBm, AGC fires);
// 1 ms later receiver B takes a weak one (-90 dBm). Default: B is desensitised by A's AGC (one state per engine).
// --agcPerReceiver: B is not. Prints "G147U PASS|FAIL ..." and exits 1 on a FAIL.
#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include <cstdio>
using namespace ns3;
static double Desens(bool perRx)
{
    RadioFrontendSubmodule::SetAgcPerReceiver(perRx);
    RadioFrontendConfigParams p = RadioFrontendConfigParams::Defaults();
    LoRaDistributionSampler s(1);
    RadioFrontendSubmodule fe(p, s);
    fe.SetReceiver(101);
    fe.ProcessAnalogFrontend(0.0, 20.0, MilliSeconds(100), -20.0, -200.0);
    fe.SetReceiver(102);
    return fe.ProcessAnalogFrontend(0.0, 20.0, MilliSeconds(101), -90.0, -200.0).frontendDesensitizationDb;
}
int main(int argc, char** argv)
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    const double shared = Desens(false), own = Desens(true);
    RadioFrontendSubmodule::SetAgcPerReceiver(false);
    const bool ok1 = shared > 1.0, ok2 = own < 1e-9;
    std::printf("G147U %s AGCSHARED default: receiver B desensitised by A's AGC, %.2f dB (need > 1)\n", ok1 ? "PASS" : "FAIL", shared);
    std::printf("G147U %s AGCOWN    --agcPerReceiver: receiver B desensitised %.2f dB (need 0)\n", ok2 ? "PASS" : "FAIL", own);
    return (ok1 && ok2) ? 0 : 1;
}
