// V1 (master plan v3): moment check of LoRaDistributionSampler keyed draws against the ns-3 streams.
// Prints, per distribution, mean and SD for both modes over N draws; exit 1 if any keyed moment departs
// from the ns-3 moment by more than 4 standard errors. Test program only.
#include "ns3/core-module.h"
#include "ns3/lora-physics-engine.h"
#include <cmath>
#include <cstdio>
#include <vector>
using namespace ns3;

static void Moments(const std::vector<double>& v, double& m, double& sd)
{
    double s = 0, q = 0;
    for (double x : v) { s += x; q += x * x; }
    m = s / v.size();
    sd = std::sqrt(std::max(0.0, q / v.size() - m * m));
}

int main(int argc, char** argv)
{
    const int N = 400000;
    struct C { const char* name; DistributionParam p; };
    std::vector<C> cs = {
        {"uniform", DistributionParam::Uniform(0.0, 3.99)},
        {"normal", DistributionParam::Normal(1.5, 0.8, 0.0, 6.0)},
        {"normal_trunc4", DistributionParam::Normal(0.0, 0.7071, -2.83, 2.83)},
        {"lognormal", DistributionParam::LogNormal(std::log(0.3), 0.6, 0.0, 3.0)},
        {"exponential", DistributionParam::Exponential(1.0 / 69.0, 0.0, 1380.0)},
        {"nakagami_0.5", DistributionParam::Nakagami(0.5, 1.0, 0.001, 10.0)},
        {"nakagami_1", DistributionParam::Nakagami(1.0, 1.0, 0.001, 10.0)},
        {"nakagami_3", DistributionParam::Nakagami(3.0, 1.0, 0.001, 10.0)},
    };
    int bad = 0;
    for (auto& c : cs)
    {
        std::vector<double> a, b;
        RngSeedManager::SetSeed(3);
        RngSeedManager::ResetNextStreamIndex();
        LoRaDistributionSampler::SetKeyedMode(false, 3);
        LoRaDistributionSampler su(3);
        for (int i = 0; i < N; ++i) a.push_back(su.Sample(c.p));
        LoRaDistributionSampler::SetKeyedMode(true, 3);
        LoRaDistributionSampler sk(3);
        sk.SetKeyedSalt(0x868);
        for (int i = 0; i < N; ++i)
        {
            sk.BeginContext(static_cast<uint32_t>(i % 977), 101, static_cast<int64_t>(i) * 1000);
            b.push_back(sk.Sample(c.p));
            sk.EndContext();
        }
        double ma, sa, mb, sb;
        Moments(a, ma, sa);
        Moments(b, mb, sb);
        const double seM = std::sqrt(sa * sa / N + sb * sb / N);
        const double seS = std::sqrt(sa * sa / (2.0 * N) + sb * sb / (2.0 * N)) * 1.5; // loose for heavy tails
        const bool ok = std::fabs(ma - mb) <= 4 * seM && std::fabs(sa - sb) <= 4 * seS;
        bad += ok ? 0 : 1;
        std::printf("%-14s ns3 mean %.5f sd %.5f | keyed mean %.5f sd %.5f | %s\n", c.name, ma, sa, mb, sb, ok ? "ok" : "DIFFERS");
    }
    LoRaDistributionSampler::SetKeyedMode(false, 0);
    return bad ? 1 : 0;
}
