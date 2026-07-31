#include "lora-fading-submodule.h"
#include <cmath>

namespace ns3 {

static std::map<uint64_t, std::pair<double, double>> g_fadingComponents;

double ComputeFadingDb(
    LinkPhysicsState& state, Time currentTime,
    const FadingParams& params, LoRaDistributionSampler& sampler,
    uint64_t linkId, Time& lastFadingUpdate, double& lastFadingValue)
{
    if (!params.enabled || params.model == FADING_NONE) {
        state.fadingDb = 0.0;
        return 0.0;
    }

    double coherenceTimeMs = sampler.Sample(params.coherenceTimeMs);
    double T_c = std::max(0.01, coherenceTimeMs / 1000.0); // Coherence time in seconds

    bool isFirstCall = (g_fadingComponents.count(linkId) == 0);
    double dt = (currentTime - lastFadingUpdate).GetSeconds();

    if (isFirstCall || dt >= 0.15) {
        double rho = isFirstCall ? 0.0 : std::exp(-dt / T_c);
        double noiseScale = std::sqrt(std::max(0.0, 1.0 - rho*rho));
        double r = 1.0;

        if (params.model == FADING_RAYLEIGH) {
            double sigma = 1.0 / std::sqrt(2.0);
            double ux = sampler.Sample(DistributionParam::Normal(0.0, sigma, -4.0 * sigma, 4.0 * sigma));
            double uy = sampler.Sample(DistributionParam::Normal(0.0, sigma, -4.0 * sigma, 4.0 * sigma));
            
            double curX = isFirstCall ? ux : (rho * g_fadingComponents[linkId].first + noiseScale * ux);
            double curY = isFirstCall ? uy : (rho * g_fadingComponents[linkId].second + noiseScale * uy);
            
            g_fadingComponents[linkId] = {curX, curY};
            r = std::sqrt(curX*curX + curY*curY);
            state.kFactor = 0.0;
        }
        else if (params.model == FADING_RICIAN) {
            // Coupling 1: NLOS State -> K-Factor
            double K;
            if (state.isNLOS) {
                // NLOS: K approaches Rayleigh (K->0), small residual from diffuse LOS
                K = std::max(0.0, sampler.Sample(DistributionParam::Normal(params.kNlosMean, 1.0, 0.0, params.kNlosMean + 3.0)));
            } else {
                // LOS: use environment's LOS K-factor distribution unchanged
                K = sampler.Sample(params.ricianKFactor);
            }
            if (K < 0.0) K = 0.0;
            state.kFactor = K;
            
            double nu = std::sqrt(K / (K + 1.0));
            double sigma = std::sqrt(1.0 / (2.0 * (K + 1.0)));
            
            double ux = sampler.Sample(DistributionParam::Normal(0.0, sigma, -4.0 * sigma, 4.0 * sigma));
            double uy = sampler.Sample(DistributionParam::Normal(0.0, sigma, -4.0 * sigma, 4.0 * sigma));
            
            double curX = isFirstCall ? ux : (rho * g_fadingComponents[linkId].first + noiseScale * ux);
            double curY = isFirstCall ? uy : (rho * g_fadingComponents[linkId].second + noiseScale * uy);
            
            g_fadingComponents[linkId] = {curX, curY};
            r = std::sqrt((curX + nu)*(curX + nu) + curY*curY);
        }
        else if (params.model == FADING_NAKAGAMI) {
            double m = sampler.Sample(params.nakagamiM);
            if (m < 0.5) m = 0.5;
            double targetR = sampler.Sample(DistributionParam::Nakagami(m, 1.0, 0.001, 10.0));
            r = isFirstCall ? targetR : (rho * lastFadingValue > 0.0 ? std::pow(10.0, -rho * lastFadingValue / 20.0) : targetR);
        }

        if (r < 1e-4) r = 1e-4;
        double lossDb = -20.0 * std::log10(r);

        lastFadingValue = lossDb;
        lastFadingUpdate = currentTime;
    }

    state.fadingDb = lastFadingValue;
    return lastFadingValue;
}

void ResetFadingState()
{
    g_fadingComponents.clear();
}

} // namespace ns3
