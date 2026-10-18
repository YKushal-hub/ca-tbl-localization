// Gate G148 unit level (scope 2026-10-17, C4 --foliageP833). Calls the P.833-10 helpers directly.
//  TABLE   P833GammaDbPerM reproduces Table 1 of ITU-R P.833-10 at its five frequencies (within 1e-12 dB/m)
//  EQ2     P833AmDb equals 1.37 f^0.42 and lies within 3.5 dB of Table 1 (eq. 2 is the fit to the two St. Petersburg forest parks, Table 1 the mixed-forest paths; 3.3 dB at 1852.2 MHz)'s Am at 949.0/1852.2/2117.5 MHz
//  BANDS   868 MHz: gamma 0.163, Am 23.5; 2440 MHz: gamma 0.388, Am 36.3 (the values the help text quotes)
//  TYPES   ApplyP833Forest sets dense forest and leaves sparse shrubs unchanged
//  LOSS    one-terminal-in-woodland excess A = Am (1 - exp(-d gamma / Am)) at 2440 MHz saturates near Am for deep paths
// --break=table perturbs the table lookup frequency, which must turn TABLE and BANDS red (rule 6).
#include "ns3/core-module.h"
#include "ns3/lora-foliage-submodule.h"
#include <cmath>
#include <cstdio>
#include <string>
using namespace ns3;
static int g_bad = 0;
static void Check(bool ok, const char* l, const char* w) { std::printf("G148U %s %-6s %s\n", ok ? "PASS" : "FAIL", l, w); g_bad += ok ? 0 : 1; }

int main(int argc, char** argv)
{
    std::string brk;
    CommandLine cmd;
    cmd.AddValue("break", "fixture", brk);
    cmd.Parse(argc, argv);
    const double k = (brk == "table") ? 1.05 : 1.0;
    char b[240];
    const double F[5] = {105.9, 466.475, 949.0, 1852.2, 2117.5}, G[5] = {0.04, 0.12, 0.17, 0.30, 0.34}, A[5] = {9.4, 18.0, 26.5, 29.0, 34.1};
    double w = 0.0, wa = 0.0;
    for (int i = 0; i < 5; ++i)
    {
        w = std::max(w, std::fabs(P833GammaDbPerM(F[i] * k) - G[i]));
        if (i >= 2) wa = std::max(wa, std::fabs(P833AmDb(F[i]) - A[i]));
    }
    std::snprintf(b, sizeof b, "worst |gamma - Table 1| %.3g dB/m", w); Check(w < 1e-12, "TABLE", b);
    const double e2 = std::fabs(P833AmDb(1000.0) - 1.37 * std::pow(1000.0, 0.42));
    std::snprintf(b, sizeof b, "eq. (2) exact (diff %.2g); worst |Am - Table 1| over 949-2117.5 MHz %.2f dB", e2, wa); Check(e2 < 1e-12 && wa < 3.5, "EQ2", b);
    const double g8 = P833GammaDbPerM(868.0 * k), a8 = P833AmDb(868.0), g24 = P833GammaDbPerM(2440.0 * k), a24 = P833AmDb(2440.0);
    std::snprintf(b, sizeof b, "868: %.3f dB/m, %.1f dB; 2440: %.3f dB/m, %.1f dB", g8, a8, g24, a24);
    Check(std::fabs(g8 - 0.163) < 0.0015 && std::fabs(a8 - 23.5) < 0.1 && std::fabs(g24 - 0.388) < 0.0015 && std::fabs(a24 - 36.3) < 0.1, "BANDS", b);
    FoliageParams d = FoliageParams::DefaultsFor(VEG_DENSE_FOREST), s = FoliageParams::DefaultsFor(VEG_SPARSE_SHRUBS);
    const double s0 = s.specificAttenuationDbPerM.mean;
    const bool okD = ApplyP833Forest(d, 2440.0) && std::fabs(d.specificAttenuationDbPerM.mean - g24 / k * k) < 0.02 && std::fabs(d.maxAttenuationDb.mean - a24) < 1e-9;
    const bool okS = !ApplyP833Forest(s, 2440.0) && s.specificAttenuationDbPerM.mean == s0;
    std::snprintf(b, sizeof b, "dense forest set (%.3f dB/m, %.1f dB); sparse shrubs unchanged (%.3f dB/m)", d.specificAttenuationDbPerM.mean, d.maxAttenuationDb.mean, s.specificAttenuationDbPerM.mean);
    Check(okD && okS, "TYPES", b);
    const double l50 = a24 * (1 - std::exp(-50 * g24 / a24)), l500 = a24 * (1 - std::exp(-500 * g24 / a24));
    std::snprintf(b, sizeof b, "2440 MHz excess at 50 m %.1f dB, 500 m %.1f dB (Am %.1f)", l50, l500, a24);
    Check(l50 > 10.0 && l50 < 20.0 && l500 > 0.98 * a24, "LOSS", b);
    return g_bad ? 1 : 0;
}
