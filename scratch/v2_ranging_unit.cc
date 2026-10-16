// Gate G139 unit level (master plan v3, V2a --nlosRangeBias). Calls ComputeRangingErrorM directly with forced
// LOS/NLOS link states, so the mechanism is tested whether or not the scenario produces NLOS links.
// Prints one line per check, "G139U PASS|FAIL <label> <detail>"; exit 1 if any check fails.
// Fixture modes (rule 6): --break=excess makes the 2.4 GHz check expect no excess; --break=wolf expects one
// under the wolf preset; --break=loss868 expects no 868 inflation. Each must turn its check red.
#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-ranging-submodule.h"
#include <cmath>
#include <cstdio>
#include <string>
using namespace ns3;

static int g_bad = 0;
static void Check(bool ok, const char* label, const std::string& why)
{
    std::printf("G139U %s %-10s %s\n", ok ? "PASS" : "FAIL", label, why.c_str());
    g_bad += ok ? 0 : 1;
}

// Mean range error over N evaluations with the given flag; keyed contexts make "on" and "off" draw the same
// numbers for every evaluation, so a LOS difference must be exactly zero.
static double MeanErr(RangingTier tier, bool flag, bool nlos, int model, double nlosDb, bool s1, int N, double* maxAbsDiffOut = nullptr,
                      const double* ref = nullptr, double* store = nullptr)
{
    PropagationParams prop = PropagationParams::DefaultsFor(ENV_RURAL_FORESTED);
    RangingParams rp;
    rp.nlosRangeBias = flag;
    rp.tofErrorModel = model;
    rp.rssiRangeFromShadowing = s1;
    LoRaDistributionSampler smp(1);
    smp.SetKeyedSalt(0x139);
    double sum = 0.0, mx = 0.0;
    for (int i = 0; i < N; ++i)
    {
        LinkPhysicsState st;
        st.isNLOS = nlos;
        st.nlosPenaltyDb = nlosDb;
        st.trueRangeM = 300.0;
        st.propagationLossDb = 100.0;
        st.rangingShadowingDb = 0.0;
        st.shadowingStdEff = 6.0;
        smp.BeginContext(7, 101, static_cast<int64_t>(i) * 1000000);
        ComputeRangingErrorM(st, tier, rp, prop, tier == RANGING_TIER_868 ? 868e6 : 2.4e9, 812500.0, 10.0, 9, smp);
        smp.EndContext();
        const double e = (tier == RANGING_TIER_868) ? st.estimatedRangeM : st.rangeErrorM;
        sum += e;
        if (store) store[i] = e;
        if (ref) mx = std::max(mx, std::fabs(e - ref[i]));
    }
    if (maxAbsDiffOut) *maxAbsDiffOut = mx;
    return sum / N;
}

int main(int argc, char** argv)
{
    std::string brk = "";
    CommandLine cmd;
    cmd.AddValue("break", "fixture mode", brk);
    cmd.Parse(argc, argv);
    LoRaDistributionSampler::SetKeyedMode(true, 139);
    const int N = 40000;
    static double a[40000];
    char buf[256];

    // 2.4 GHz NLOS, engine model: excess mean ~69 m (exponential), SE = 69/sqrt(N) = 0.35 m.
    const double off = MeanErr(RANGING_TIER_2P4, false, true, 0, 15.0, false, N);
    const double on = MeanErr(RANGING_TIER_2P4, brk != "excess", true, 0, 15.0, false, N);
    std::snprintf(buf, sizeof buf, "NLOS mean error %.2f -> %.2f m (excess %.2f, expected 69 +/- 2)", off, on, on - off);
    Check(std::fabs((on - off) - 69.0) <= 2.0, "EXCESS24", buf);

    // 2.4 GHz LOS: the flag must change nothing, draw for draw.
    double mx = 0;
    MeanErr(RANGING_TIER_2P4, false, false, 0, 0.0, false, N, nullptr, nullptr, a);
    MeanErr(RANGING_TIER_2P4, true, false, 0, 0.0, false, N, &mx, a);
    std::snprintf(buf, sizeof buf, "LOS max |on - off| = %.3g m", mx);
    Check(mx == 0.0, "LOS24", buf);

    // wolf preset already carries NLOS: no excess added (exactly equal).
    MeanErr(RANGING_TIER_2P4, false, true, 3, 15.0, false, N, nullptr, nullptr, a);
    MeanErr(RANGING_TIER_2P4, brk != "wolf", true, brk == "wolf" ? 0 : 3, 15.0, false, N, &mx, a);
    std::snprintf(buf, sizeof buf, "wolf NLOS max |on - off| = %.3g m", mx);
    Check(mx == 0.0, "WOLF", buf);

    // 868 MHz, S1 on, NLOS loss 10 dB: estimated range inflated, as RSSI inversion of the extra loss would.
    const double e0 = MeanErr(RANGING_TIER_868, false, true, 0, 10.0, true, 200);
    const double e1 = MeanErr(RANGING_TIER_868, brk != "loss868", true, 0, 10.0, true, 200);
    const double n = PropagationParams::DefaultsFor(ENV_RURAL_FORESTED).pathLossExponent.mean;
    const double expect = std::pow(10.0, 10.0 / (10.0 * n));
    std::snprintf(buf, sizeof buf, "868 S1 estimate %.1f -> %.1f m, ratio %.3f (log-distance 10^(10/10n) = %.3f, n = %.2f)", e0, e1,
                  e1 / e0, expect, n);
    Check(e1 / e0 > 1.05 && std::fabs(e1 / e0 - expect) / expect < 0.25, "LOSS868", buf);

    // 868 MHz, S1 off, NLOS: also inflated (additive inversion difference).
    const double f0 = MeanErr(RANGING_TIER_868, false, true, 0, 10.0, false, 2000);
    const double f1 = MeanErr(RANGING_TIER_868, brk != "loss868", true, 0, 10.0, false, 2000);
    std::snprintf(buf, sizeof buf, "868 S1-off mean estimate %.1f -> %.1f m", f0, f1);
    Check(f1 > f0 + 10.0, "LOSS868B", buf);

    // 868 MHz LOS: unchanged draw for draw.
    double* b = a;
    MeanErr(RANGING_TIER_868, false, false, 0, 0.0, true, 2000, nullptr, nullptr, b);
    MeanErr(RANGING_TIER_868, true, false, 0, 0.0, true, 2000, &mx, b);
    std::snprintf(buf, sizeof buf, "868 LOS max |on - off| = %.3g m", mx);
    Check(mx == 0.0, "LOS868", buf);
    return g_bad ? 1 : 0;
}
