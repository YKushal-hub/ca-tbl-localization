// Gate G146 unit level (master plan v3, V2a-2 --nlos3gppRMa). Calls ComputeNLOSPenaltyDb directly.
//  PROB     at d2D = 1000 m, hBS 15 m: the NLOS fraction over 40 000 calls matches 1 - exp(-0.99) = 0.628 (+/- 0.01)
//  EXCESS   every NLOS call's loss equals the 3GPP RMa excess computed independently in Python (within 1e-6 dB)
//  FALLBACK with both ends below 10 m the old model runs (its NLOS fraction differs from RMa's)
//  OFF      with the flag off at hBS 15 m the old model runs too
// --break=far moves the gateway to 5 m height, which must turn PROB and EXCESS red (rule 6).
#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-nlos-submodule.h"
#include <cmath>
#include <cstdio>
#include <string>
using namespace ns3;
static int g_bad = 0;
static void Check(bool ok, const char* l, const char* w) { std::printf("G146U %s %-9s %s\n", ok ? "PASS" : "FAIL", l, w); g_bad += ok ? 0 : 1; }

static double Frac(bool rma, double hBS, double hUT, double f, double d2D, int n, double* oneExcess)
{
    NLOSParams p = NLOSParams::DefaultsFor(ENV_RURAL_FORESTED);
    p.frequencyHz = f;
    p.rma3gpp = rma;
    LoRaDistributionSampler s(1);
    int k = 0;
    for (int i = 0; i < n; ++i)
    {
        const double v = ComputeNLOSPenaltyDb(Vector(0, 0, hBS), Vector(d2D, 0, hUT), p, s);
        if (v > 0.0) { ++k; if (oneExcess) *oneExcess = v; }
    }
    return static_cast<double>(k) / n;
}

int main(int argc, char** argv)
{
    std::string brk;
    CommandLine cmd;
    cmd.AddValue("break", "fixture", brk);
    cmd.Parse(argc, argv);
    RngSeedManager::SetSeed(5);
    const double hG = (brk == "far") ? 5.0 : 15.0;
    char b[200];
    double ex = -1;
    const double fr = Frac(true, hG, 1.5, 868e6, 1000.0, 40000, &ex);
    std::snprintf(b, sizeof b, "NLOS fraction %.4f (3GPP 0.6284)", fr);
    Check(std::fabs(fr - (1.0 - std::exp(-0.99))) <= 0.01, "PROB", b);
    struct R { double f, d, ref; } refs[] = {{868e6, 500, 27.614689}, {868e6, 3000, 27.438952}, {2.4e9, 1000, 34.296164}, {2.4e9, 200, 21.944824}};
    bool ok = true; double worst = 0;
    for (auto& r : refs)
    {
        double e = -1;
        Frac(true, hG, 1.5, r.f, r.d, 2000, &e);
        worst = std::max(worst, std::fabs(e - r.ref));
        ok = ok && std::fabs(e - r.ref) < 1e-6;
    }
    std::snprintf(b, sizeof b, "NLOS excess vs Python reference: worst |diff| %.2e dB", worst);
    Check(ok, "EXCESS", b);
    const double frLow = Frac(true, 1.5, 1.5, 868e6, 1000.0, 40000, nullptr);
    std::snprintf(b, sizeof b, "both ends 1.5 m: NLOS fraction %.4f (old model, not RMa 0.628)", frLow);
    Check(std::fabs(frLow - 0.628) > 0.1, "FALLBACK", b);
    const double frOff = Frac(false, 15.0, 1.5, 868e6, 1000.0, 40000, nullptr);
    std::snprintf(b, sizeof b, "flag off at 15 m: NLOS fraction %.4f (old model)", frOff);
    Check(std::fabs(frOff - 0.628) > 0.1, "OFF", b);
    return g_bad ? 1 : 0;
}
