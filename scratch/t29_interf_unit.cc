// Gate G153 unit level (T29 --interferenceDistance; TECH_DEBT M.1). Drives InterferenceSubmodule directly.
// One interferer registered with its own 100 m link (its received power -80 dBm at its own receiver). A victim
// receiver is evaluated at 100 m and at 2 km from the interferer.
//  OFF    without the flag the strongest-interferer power is the same at both distances (the defect, by construction)
//  ON     with it, the far receiver hears the interferer 10 n log10(2000/100) dB weaker (n = 2.3), within 1e-6 dB
//  NEAR   with it, at the interferer's own link length the power is unchanged
// --break=ignore evaluates ON with the flag cleared, which must turn ON red (rule 6).
#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-interference-submodule.h"
#include <cmath>
#include <cstdio>
#include <string>
using namespace ns3;
static int g_bad = 0;
static void Check(bool ok, const char* l, const char* w) { std::printf("G153U %s %-5s %s\n", ok ? "PASS" : "FAIL", l, w); g_bad += ok ? 0 : 1; }

static double Strongest(bool flag, double victimDist)
{
    InterferenceParams p = InterferenceParams::DefaultsFor(ENV_RURAL_FORESTED);
    p.externalInterfererDensityPerKm2 = DistributionParam::Fixed(0.0);
    p.attenuateByDistance = flag;
    p.attenuationExponent = 2.3;
    InterferenceSubmodule s;
    LoRaDistributionSampler sm(1);
    // interferer 7 at the origin, its own link 100 m, received at -80 dBm there
    s.RegisterActiveTransmission(7, 101, Seconds(0.0), Seconds(1.0), 7, 125e3, 5, -80.0, Vector(0, 0, 1.5), 100.0);
    s.ComputeInterferencePenaltyDb(9, 7, 125e3, Seconds(0.1), Seconds(0.5), Vector(victimDist + 50.0, 0, 1.5),
                                   Vector(victimDist, 0, 1.5), Seconds(0.1), p, sm, -90.0, -125.0);
    return s.GetStrongestInterfererPowerDbm();
}

int main(int argc, char** argv)
{
    std::string brk;
    CommandLine cmd;
    cmd.AddValue("break", "fixture", brk);
    cmd.Parse(argc, argv);
    char b[200];
    const double offN = Strongest(false, 100.0), offF = Strongest(false, 2000.0);
    std::snprintf(b, sizeof b, "flag off: %.3f dBm at 100 m, %.3f dBm at 2 km", offN, offF);
    Check(std::fabs(offN - offF) < 1e-9, "OFF", b);
    const bool onFlag = brk != "ignore";
    const double onN = Strongest(onFlag, 100.0), onF = Strongest(onFlag, 2000.0);
    const double want = 10.0 * 2.3 * std::log10(2000.0 / 100.0);
    std::snprintf(b, sizeof b, "flag on: %.3f dBm at 2 km, %.3f dB weaker (want %.3f)", onF, onN - onF, want);
    Check(std::fabs((onN - onF) - want) < 1e-6, "ON", b);
    std::snprintf(b, sizeof b, "flag on at the own-link length: %.3f dBm (off %.3f)", onN, offN);
    Check(std::fabs(onN - offN) < 1e-9, "NEAR", b);
    return g_bad ? 1 : 0;
}
