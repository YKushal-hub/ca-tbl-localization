#include "lora-correlation-engine.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

LoRaCorrelationEngine::LoRaCorrelationEngine() {}

CorrelatedChannelState LoRaCorrelationEngine::GetCorrelatedState(
    uint64_t linkId, Vector txPos, Vector rxPos,
    EnvironmentClass env, WeatherState weather,
    Time currentTime, LoRaDistributionSampler& sampler)
{
    double dx = txPos.x - rxPos.x;
    double dy = txPos.y - rxPos.y;
    double dz = txPos.z - rxPos.z;
    double dist = std::max(1.0, std::sqrt(dx*dx + dy*dy + dz*dz));
    
    CorrelatedChannelState s;
    if (m_channelStates.count(linkId) == 0) {
        s.lastUpdateTime = currentTime;
        s.gmShadowingState = sampler.SamplePersistent(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0), linkId, "gm_shad");
        s.gmFadingState = sampler.SamplePersistent(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0), linkId, "gm_fade");
        s.gmDelaySpreadState = sampler.SamplePersistent(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0), linkId, "gm_ds");
        s.gmDopplerState = sampler.SamplePersistent(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0), linkId, "gm_dop");
    } else {
        s = m_channelStates[linkId];
        double dtS = (currentTime - s.lastUpdateTime).GetSeconds();
        if (dtS > 0.0) {
            // Gauss-Markov temporal evolution with nominal coherence time ~ 1.0s
            double T_c = 1.0;
            if (weather == WEATHER_HEAVY_RAIN || weather == WEATHER_WINDY_WET) {
                T_c = 0.5; // Coherence time decreases under extreme weather
            }
            double rho = std::exp(-dtS / T_c);
            double noise_scale = std::sqrt(std::max(0.0, 1.0 - rho*rho));
            
            double u1 = sampler.Sample(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0));
            double u2 = sampler.Sample(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0));
            double u3 = sampler.Sample(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0));
            double u4 = sampler.Sample(DistributionParam::Normal(0.0, 1.0, -3.0, 3.0));
            
            s.gmShadowingState = rho * s.gmShadowingState + noise_scale * u1;
            s.gmFadingState = rho * s.gmFadingState + noise_scale * u2;
            s.gmDelaySpreadState = rho * s.gmDelaySpreadState + noise_scale * u3;
            s.gmDopplerState = rho * s.gmDopplerState + noise_scale * u4;
            s.lastUpdateTime = currentTime;
        }
    }
    
    // Clamp GM states to standard bounds
    s.gmShadowingState = std::max(-3.0, std::min(3.0, s.gmShadowingState));
    s.gmFadingState = std::max(-3.0, std::min(3.0, s.gmFadingState));
    s.gmDelaySpreadState = std::max(-3.0, std::min(3.0, s.gmDelaySpreadState));
    s.gmDopplerState = std::max(-3.0, std::min(3.0, s.gmDopplerState));

    // Environmental latent variable coupling
    double nlosProb = 1.0 - std::exp(-dist / 200.0);
    double baseShadowStddevDb = 8.0;
    double baseDelaySpreadNs = 100.0;
    
    switch (env) {
        case ENV_RURAL_OPEN:
            nlosProb = 1.0 - std::exp(-dist / 500.0);
            baseShadowStddevDb = 6.0;
            baseDelaySpreadNs = 50.0;
            break;
        case ENV_RURAL_FORESTED:
            nlosProb = 1.0 - std::exp(-dist / 300.0);
            baseShadowStddevDb = 9.0;
            baseDelaySpreadNs = 120.0;
            break;
        case ENV_SUBURBAN:
            nlosProb = 1.0 - std::exp(-dist / 200.0);
            baseShadowStddevDb = 8.0;
            baseDelaySpreadNs = 180.0;
            break;
        case ENV_URBAN_MODERATE:
        case ENV_INDUSTRIAL:
            nlosProb = 1.0 - std::exp(-dist / 100.0);
            baseShadowStddevDb = 10.0;
            baseDelaySpreadNs = 350.0;
            break;
        case ENV_URBAN_DENSE:
            nlosProb = 1.0 - std::exp(-dist / 50.0);
            baseShadowStddevDb = 12.0;
            baseDelaySpreadNs = 550.0;
            break;
        case ENV_TUNNEL_CONFINED:
        case ENV_CONCRETE_BUNKER:
            nlosProb = 0.1;
            baseShadowStddevDb = 4.0;
            baseDelaySpreadNs = 250.0;
            break;
        case ENV_INDOOR_OFFICE:
            nlosProb = 1.0 - std::exp(-dist / 80.0);
            baseShadowStddevDb = 11.0;
            baseDelaySpreadNs = 400.0;
            break;
    }
    
    // Rain wetness coupling
    s.rainWetnessFactor = 0.0;
    if (weather == WEATHER_HEAVY_RAIN || weather == WEATHER_WINDY_WET) {
        s.rainWetnessFactor = 1.0;
    } else if (weather == WEATHER_LIGHT_RAIN || weather == WEATHER_FOG) {
        s.rainWetnessFactor = 0.5;
    }
    
    // Correlated parameter derivations
    s.shadowingBiasDb = s.gmShadowingState * baseShadowStddevDb;
    
    // Line-of-sight test coupled to shadowing obstruction and NLOS prob
    s.obstructionFactor = std::max(0.0, std::min(1.0, nlosProb + 0.15 * s.gmShadowingState));
    s.los = (s.obstructionFactor < 0.5);
    if (env == ENV_TUNNEL_CONFINED) {
        s.los = true; // Tunnel waveguide effect
    }
    
    s.foliageDensity = 0.2;
    if (env == ENV_RURAL_FORESTED) s.foliageDensity = 0.8;
    else if (env == ENV_SUBURBAN || env == ENV_URBAN_MODERATE) s.foliageDensity = 0.4;
    s.foliageDensity = std::max(0.0, std::min(1.0, s.foliageDensity + 0.2 * s.rainWetnessFactor));
    
    s.fadingSeverity = (s.los ? 0.2 : 0.8);
    s.fadingSeverity = std::max(0.0, std::min(1.0, s.fadingSeverity + 0.1 * s.gmFadingState));
    
    s.delaySpreadNs = baseDelaySpreadNs * (1.0 + 0.3 * s.gmDelaySpreadState) * (1.0 + 0.5 * s.obstructionFactor);
    s.delaySpreadNs = std::max(10.0, s.delaySpreadNs);
    
    s.angularSpreadDeg = 15.0 * (1.0 + s.obstructionFactor);
    s.coherenceTimeScale = std::max(0.1, 1.0 - 0.3 * s.rainWetnessFactor + 0.1 * s.gmDopplerState);
    s.terrainBlockageFactor = std::max(0.0, std::min(1.0, 0.5 * s.obstructionFactor + 0.1 * s.gmShadowingState));
    
    m_channelStates[linkId] = s;
    return s;
}

} // namespace ns3
