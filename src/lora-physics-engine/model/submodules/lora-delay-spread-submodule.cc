#include "lora-delay-spread-submodule.h"
#include <cmath>
#include <algorithm>

namespace ns3 {

DelaySpreadSubmodule::DelaySpreadSubmodule() {}

DelaySpreadResult DelaySpreadSubmodule::ComputeDelaySpread(
    EnvironmentClass env, double distM, double carrierFreqHz, double signalBandwidthHz,
    double correlatedDelaySpreadNs, LoRaDistributionSampler& sampler)
{
    DelaySpreadResult res;
    res.taps.clear();
    
    // Select base Power Delay Profile (PDP) based on environmental class
    std::vector<double> baseDelays;
    std::vector<double> basePowers;
    
    switch (env) {
        case ENV_RURAL_OPEN:
        case ENV_RURAL_FORESTED:
            // 3GPP Rural Macro (RMa) inspired 3-tap profile
            baseDelays = {0.0, 45.0, 110.0};
            basePowers = {0.0, -10.0, -18.0};
            break;
            
        case ENV_SUBURBAN:
        case ENV_URBAN_MODERATE:
            // 6-tap moderate exponential profile
            baseDelays = {0.0, 60.0, 150.0, 310.0, 550.0, 850.0};
            basePowers = {0.0, -3.0, -7.0, -12.0, -17.0, -22.0};
            break;
            
        case ENV_URBAN_DENSE:
        case ENV_CONCRETE_BUNKER:
            // 3GPP Urban Macro (UMa) inspired 10-tap dense exponential profile
            baseDelays = {0.0, 50.0, 120.0, 210.0, 350.0, 520.0, 720.0, 950.0, 1250.0, 1600.0};
            basePowers = {0.0, -2.0, -4.5, -7.0, -9.5, -12.5, -15.5, -19.0, -22.5, -26.0};
            break;
            
        case ENV_INDUSTRIAL:
        case ENV_INDOOR_OFFICE:
            // Sparse echoes with very long excess delays
            baseDelays = {0.0, 450.0, 1200.0, 2500.0};
            basePowers = {0.0, -8.0, -14.0, -20.0};
            break;
            
        case ENV_TUNNEL_CONFINED:
            // Waveguide reflections: clustered multipath
            baseDelays = {0.0, 120.0, 240.0, 360.0, 480.0};
            basePowers = {0.0, -1.5, -4.0, -8.0, -15.0};
            break;
    }
    
    // Calculate nominal RMS delay spread of the base profile
    double totalP = 0.0;
    double meanTauP = 0.0;
    for (size_t i = 0; i < baseDelays.size(); ++i) {
        double pLinear = std::pow(10.0, basePowers[i] / 10.0);
        totalP += pLinear;
        meanTauP += baseDelays[i] * pLinear;
    }
    double meanTau = meanTauP / totalP;
    
    double varTauP = 0.0;
    for (size_t i = 0; i < baseDelays.size(); ++i) {
        double pLinear = std::pow(10.0, basePowers[i] / 10.0);
        double diff = baseDelays[i] - meanTau;
        varTauP += (diff * diff) * pLinear;
    }
    double nominalRms = std::sqrt(varTauP / totalP);
    if (nominalRms < 1.0) nominalRms = 1.0;
    
    // Scale tap delays so the exact profile RMS matches the correlated target
    double delayScale = correlatedDelaySpreadNs / nominalRms;
    
    double actualTotalP = 0.0;
    double actualMeanTauP = 0.0;
    
    for (size_t i = 0; i < baseDelays.size(); ++i) {
        MultipathTap t;
        t.excessDelayNs = baseDelays[i] * delayScale;
        t.relativePowerDb = basePowers[i];
        t.phaseRad = sampler.Sample(DistributionParam::Uniform(0.0, 2.0 * M_PI));
        res.taps.push_back(t);
        
        double pL = std::pow(10.0, t.relativePowerDb / 10.0);
        actualTotalP += pL;
        actualMeanTauP += t.excessDelayNs * pL;
    }
    
    double actualMeanTau = actualMeanTauP / actualTotalP;
    double actualVarTauP = 0.0;
    for (size_t i = 0; i < res.taps.size(); ++i) {
        double pL = std::pow(10.0, res.taps[i].relativePowerDb / 10.0);
        double diff = res.taps[i].excessDelayNs - actualMeanTau;
        actualVarTauP += (diff * diff) * pL;
    }
    
    res.rmsDelaySpreadNs = std::sqrt(actualVarTauP / actualTotalP);
    
    // Coherence bandwidth approximation: B_c ~ 1 / (5 * sigma_tau)
    if (res.rmsDelaySpreadNs > 0.0) {
        res.coherenceBandwidthHz = 1.0 / (5.0 * res.rmsDelaySpreadNs * 1e-9);
    } else {
        res.coherenceBandwidthHz = 1e9; // Essentially flat fading
    }
    
    // Frequency-selective fading notch penalty
    double effBandwidth = (signalBandwidthHz > 0.0 ? signalBandwidthHz : 125000.0);
    if (effBandwidth > res.coherenceBandwidthHz) {
        // Channel is frequency-selective: inter-symbol interference and notch fading penalty
        double selectivityRatio = effBandwidth / res.coherenceBandwidthHz;
        res.frequencySelectivePenaltyDb = 10.0 * std::log10(1.0 + selectivityRatio * 0.5);
        if (res.frequencySelectivePenaltyDb > 15.0) res.frequencySelectivePenaltyDb = 15.0;
    } else {
        res.frequencySelectivePenaltyDb = 0.0; // Flat fading within channel bandwidth
    }
    
    return res;
}

// Feature 7: Dynamic delay spread sampling from LOS/NLOS distributions
// Values OURS: the cited 'Lim et al. 2018 (MDPI Sensors)' source was not found
DelaySpreadResult DelaySpreadSubmodule::ComputeDelaySpreadDynamic(
    LinkPhysicsState& state, EnvironmentClass env, double distM, double carrierFreqHz, double signalBandwidthHz,
    const DelaySpreadParams& params, LoRaDistributionSampler& sampler)
{
    if (!params.enabled) {
        // Fall back to static correlation if dynamic disabled
        DelaySpreadResult res = ComputeDelaySpread(env, distM, carrierFreqHz, signalBandwidthHz, 195.78, sampler);
        state.delaySpreadNs = res.rmsDelaySpreadNs;
        return res;
    }
    
    // Feature 7 / Coupling 3: Sample delay spread from environment and LOS/NLOS appropriate distributions
    double sampledDelaySpreadNs;
    if (state.isNLOS) {
        sampledDelaySpreadNs = sampler.Sample(params.nlosDelaySpreadNs);
    } else {
        sampledDelaySpreadNs = sampler.Sample(params.losDelaySpreadNs);
    }
    
    // Use sampled delay spread for the TDL computation
    DelaySpreadResult res = ComputeDelaySpread(env, distM, carrierFreqHz, signalBandwidthHz, 
                                               sampledDelaySpreadNs, sampler);
    state.delaySpreadNs = res.rmsDelaySpreadNs;
    return res;
}

} // namespace ns3
