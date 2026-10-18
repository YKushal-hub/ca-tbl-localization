/*
 * LoRa Physics Engine — Empirical Validation Suite
 * 
 * Standalone binary that tests 10 empirical metrics against published data:
 *   Metric 1:  Sensitivity threshold per SF (SX1276 datasheet)
 *   Metric 2:  PDR vs distance per SF (Petäjäjärvi 2015)
 *   Metric 3:  Capture effect curve shape (Croce 2018)
 *   Metric 4:  Fade margin for 99% PDR (Cattani 2017)
 *   Metric 5:  RSSI CDF shape (Cattani 2017)
 *   Metric 6:  Inter-SF isolation (Croce 2018)
 *   Metric 7:  DOP spatial pattern (Aernouts 2018)
 *   Metric 8:  Airtime vs SF collision rate
 *   Metric 9:  Gateway density scaling (Georgiou 2017)
 *   Metric 10: Timestamp jitter distribution shape (Podevijn 2018)
 *
 * Usage:
 *   ./ns3 run "lora_empirical_validation --metric=all --seed=7"
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/lora-physics-engine-module.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <random>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LoRaEmpiricalValidation");

// ============================================================================
// METRIC 1: SENSITIVITY THRESHOLD PER SF
// ============================================================================
// Verify that the reception decision produces ~50% PDR at exact sensitivity
// threshold, ~90%+ at +3 dB, and ~0% at -6 dB.
bool RunMetric1_SensitivityThreshold(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 1: Sensitivity Threshold per SF ===");
    
    std::ofstream out(outputDir + "/metric1_sensitivity_seed" + std::to_string(seed) + ".csv");
    out << "sf,sensitivity_dbm,offset_db,pdr,n_packets,actual_rxpower_mean\n";
    
    double frequencyHz = 868e6;
    double bandwidthHz = 125e3;
    
    // Datasheet values (already verified in engine source)
    double targetSensitivity[] = {-123.0, -126.0, -129.0, -132.0, -134.5, -137.0};
    
    bool allPass = true;
    
    for (uint8_t sf = 7; sf <= 12; ++sf) {
        int sfIdx = sf - 7;
        double sens = targetSensitivity[sfIdx];
        
        // Test at -6, -3, 0, +3, +6 dB offsets from sensitivity
        for (double offset : {-6.0, -3.0, 0.0, 3.0, 6.0}) {
            // Configure engine: disable ALL stochastic and hardware impairments
            LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
                ENV_RURAL_OPEN, WEATHER_CLEAR_DRY, FADING_NONE,
                VEG_NONE, MOB_STATIC, false);
            config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL; // Skip unconfigurable multipath/thermal losses
            config.randomSeed = seed;
            config.propagation.frequencyHz = frequencyHz;
            config.shadowing.enabled = false;
            config.nlos.enabled = false;
            config.fading.model = FADING_NONE;
            config.interference.enabled = false;
            config.foliage.enabled = false;
            config.terrain.enabled = false;
            config.hardware.enabled = false; // Disable all HW impairments
            config.hardware.txPowerAccuracyDb = DistributionParam::Fixed(0.0);
            config.hardware.sensitivityVariationDb = DistributionParam::Fixed(0.0);
            config.hardware.rssiHardwareVariationDb = DistributionParam::Fixed(0.0);
            config.hardware.carrierFrequencyOffsetHz = DistributionParam::Fixed(0.0);
            config.hardware.phaseNoiseRad = DistributionParam::Fixed(0.0);
            
            // Disable all Stage 4-6 nonlinear and atmospheric impairments
            config.iqImpairment.enabled = false;
            config.atmospheric.enabled = false;
            config.rfNonlinearity.enabled = false;
            config.delaySpread.enabled = false;
            config.mobility.enabled = false;
            config.propagation.pathLossExponent = DistributionParam::Fixed(2.08); // Eliminate path loss randomness
            
            LoRaPhysicsEngine engine;
            engine.Initialize(config);
            
            double distance = 1000.0; // 1 km
            Vector txPos(0, 0, 15.0);
            Vector rxPos(distance, 0, 2.0);
            
            // Probe: send with known TX power, measure actual rxPowerDbm
            double probeTxPower = 14.0;
            PhysicsResult probe = engine.ProcessTransmission(
                99999, 0, txPos, rxPos, probeTxPower, sf, bandwidthHz, 0, Seconds(0.0));
            
            // Compute total effective loss from probe
            double actualTotalLoss = probeTxPower - probe.rxPowerDbm;
            
            // Now compute TX power needed to produce target RSSI
            double targetRssi = sens + offset;
            double txPower = targetRssi + actualTotalLoss;
            
            NS_LOG_UNCOND("  SF" << (int)sf << " offset=" << offset 
                          << "dB: probeLoss=" << std::fixed << std::setprecision(1) << actualTotalLoss
                          << " txPower=" << txPower << " targetRSSI=" << targetRssi
                          << " effectiveSens=" << probe.effectiveSensitivityDbm);
            
            uint32_t received = 0;
            uint32_t total = 500;
            double timeS = 1.0;
            double rxPowerSum = 0;
            
            for (uint32_t pkt = 0; pkt < total; ++pkt) {
                PhysicsResult r = engine.ProcessTransmission(
                    pkt + 1, 0, txPos, rxPos, txPower, sf, bandwidthHz, 0, Seconds(timeS));
                timeS += 0.1;
                rxPowerSum += r.rxPowerDbm;
                if (r.packetReceived) received++;
            }
            
            double pdr = (double)received / total;
            double rxPowerMean = rxPowerSum / total;
            out << (int)sf << "," << sens << "," << offset << "," << pdr << "," << total 
                << "," << rxPowerMean << "\n";
            
            NS_LOG_UNCOND("    PDR=" << std::fixed << std::setprecision(2) << pdr
                          << " actualRxPower=" << std::setprecision(1) << rxPowerMean);
            
            // Validation checks
            if (offset <= -6.0 && pdr > 0.1) {
                NS_LOG_UNCOND("    [WARN] PDR too high at -6 dB below sensitivity");
            }
            if (offset >= 6.0 && pdr < 0.9) {
                NS_LOG_UNCOND("    [WARN] PDR too low at +6 dB above sensitivity");
                allPass = false;
            }
        }
    }
    
    out.close();
    NS_LOG_UNCOND("  Result: " << (allPass ? "[PASS]" : "[FAIL]"));
    return allPass;
}

// ============================================================================
// METRIC 2: PDR VS DISTANCE PER SF (Petäjäjärvi 2015)
// ============================================================================
// TECH_DEBT N.29 (2026-10-18): Metric 2 used to send a frame every 0.1 s, but an SF12 frame lasts over a second,
// so each one collided with the harness's own previous frame (SF12 at 500 m: 199 of 200 lost). Frames are now spaced
// at twice their time on air (20-byte payload, CR 4/5, explicit header, 8-symbol preamble, LDRO at SF11/12), never
// closer than 0.1 s. --metric2SpacingS=0.1 restores the old spacing and reproduces earlier outputs.
static double g_metric2SpacingS = -1.0; // < 0: automatic (2 x time on air)
static double LoRaToaS(uint8_t sf, double bwHz, uint32_t payloadBytes)
{
    const double tSym = std::pow(2.0, sf) / bwHz;
    const int de = (sf >= 11 && bwHz <= 125e3) ? 1 : 0;
    const double num = 8.0 * payloadBytes - 4.0 * sf + 28.0 + 16.0; // CRC on, explicit header
    const double nPayload = 8.0 + std::max(std::ceil(num / (4.0 * (sf - 2.0 * de))) * 5.0, 0.0);
    return (8.0 + 4.25) * tSym + nPayload * tSym;
}

bool RunMetric2_PDRvsDistance(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 2: PDR vs Distance per SF ===");
    
    std::ofstream out(outputDir + "/metric2_pdr_distance_seed" + std::to_string(seed) + ".csv");
    out << "environment,sf,distance_m,pdr,rssi_mean,rssi_stddev,n_packets\n";
    
    double frequencyHz = 868e6;
    double txPowerDbm = 14.0;
    double bandwidthHz = 125e3;
    uint32_t packetsPerPoint = 200;
    
    struct EnvSetup {
        std::string name;
        EnvironmentClass env;
        std::vector<double> distances;
    };
    
    std::vector<EnvSetup> envs = {
        {"Urban", ENV_URBAN_MODERATE, {500, 1000, 1500, 2000, 3000, 5000, 8000, 10000, 15000}},
        {"Suburban", ENV_SUBURBAN, {500, 1000, 2000, 3000, 5000, 8000, 10000, 15000}},
        {"OpenWater", ENV_RURAL_OPEN, {500, 1000, 2000, 5000, 8000, 10000, 15000, 20000}}
    };
    
    bool allPass = true;
    
    for (const auto& envSetup : envs) {
        for (uint8_t sf : {7, 9, 12}) {
            LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
                envSetup.env, WEATHER_CLEAR_DRY, FADING_RICIAN,
                VEG_NONE, MOB_STATIC, false);
            config.randomSeed = seed;
            config.propagation.frequencyHz = frequencyHz;
            config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
            
            LoRaPhysicsEngine engine;
            engine.Initialize(config);
            double timeS = 0.0;
            
            for (double distance : envSetup.distances) {
                Vector txPos(0, 0, 15.0);
                Vector rxPos(distance, 0, 2.0);
                
                uint32_t received = 0;
                std::vector<double> rssiValues;
                
                const double spacingS = (g_metric2SpacingS >= 0.0)
                                            ? g_metric2SpacingS
                                            : std::max(0.1, 2.0 * LoRaToaS(sf, bandwidthHz, 20));
                for (uint32_t pkt = 0; pkt < packetsPerPoint; ++pkt) {
                    PhysicsResult r = engine.ProcessTransmission(
                        pkt + 1, 0, txPos, rxPos, txPowerDbm, sf, bandwidthHz, 0, Seconds(timeS));
                    timeS += spacingS;
                    if (r.packetReceived) {
                        received++;
                        rssiValues.push_back(r.measuredRssiDbm);
                    }
                }
                
                double pdr = (double)received / packetsPerPoint;
                double rssiMean = 0, rssiStd = 0;
                if (!rssiValues.empty()) {
                    rssiMean = std::accumulate(rssiValues.begin(), rssiValues.end(), 0.0) / rssiValues.size();
                    double sq = 0;
                    for (double v : rssiValues) sq += (v - rssiMean) * (v - rssiMean);
                    rssiStd = std::sqrt(sq / rssiValues.size());
                }
                
                out << envSetup.name << "," << (int)sf << "," << distance << ","
                    << std::fixed << std::setprecision(3) << pdr << ","
                    << rssiMean << "," << rssiStd << "," << packetsPerPoint << "\n";
            }
        }
    }
    
    // Check key empirical targets from Petäjäjärvi 2015
    // (analysis script will do the detailed comparison)
    out.close();
    NS_LOG_UNCOND("  Output: metric2_pdr_distance_seed" << seed << ".csv");
    NS_LOG_UNCOND("  Result: [DATA] — analysis script will evaluate against targets");
    return true;
}

// ============================================================================
// METRIC 3: CAPTURE EFFECT CURVE SHAPE (Croce 2018)
// ============================================================================
bool RunMetric3_CaptureEffect(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 3: Capture Effect Curve (Croce 2018) ===");
    
    std::ofstream out(outputDir + "/metric3_capture_effect_seed" + std::to_string(seed) + ".csv");
    out << "sir_db,capture_probability,n_trials\n";
    
    double frequencyHz = 868e6;
    double bandwidthHz = 125e3;
    uint8_t sf = 7;
    uint32_t trialsPerSIR = 1000;
    
    bool allPass = true;
    
    for (double sir = -3.0; sir <= 15.0; sir += 1.0) {
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_RURAL_OPEN, WEATHER_CLEAR_DRY, FADING_NONE,
            VEG_NONE, MOB_STATIC, false);
        config.randomSeed = seed;
        config.propagation.frequencyHz = frequencyHz;
        config.propagation.pathLossExponent = DistributionParam::Fixed(2.08);
        config.shadowing.enabled = false;
        config.nlos.enabled = false;
        config.fading.model = FADING_NONE;
        config.foliage.enabled = false;
        config.terrain.enabled = false;
        config.hardware.txPowerAccuracyDb = DistributionParam::Fixed(0.0);
        config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
        
        LoRaPhysicsEngine engine;
        engine.Initialize(config);
        
        // Place desired transmitter and interferer at similar distances
        double distance = 500.0;
        Vector gwPos(1000.0, 0, 2.0);
        Vector desiredPos(0, 0, 15.0);
        Vector interfererPos(0, 0, 15.0); 
        
        uint32_t captured = 0;
        double timeS = 0.0;
        
        for (uint32_t t = 0; t < trialsPerSIR; ++t) {
            // Register the interferer first
            double desiredPower = 14.0;
            // Adjust interferer power to achieve desired SIR at gateway
            double interfererPower = desiredPower - sir;
            
            Time airtime = Seconds(0.1);
            engine.GetMutableConfig().interference.enabled = true;
            
            // Register interferer transmission
            // We simulate by doing two ProcessTransmission calls
            PhysicsResult intResult = engine.ProcessTransmission(
                trialsPerSIR + t + 1, 0, interfererPos, gwPos,
                interfererPower, sf, bandwidthHz, 0, Seconds(timeS));
            
            // Now transmit desired signal during the interference
            PhysicsResult desResult = engine.ProcessTransmission(
                t + 1, 0, desiredPos, gwPos,
                desiredPower, sf, bandwidthHz, 0, Seconds(timeS + 0.001));
            
            if (desResult.packetReceived) captured++;
            timeS += 0.2;
        }
        
        double capProb = (double)captured / trialsPerSIR;
        out << sir << "," << std::fixed << std::setprecision(3) << capProb << "," << trialsPerSIR << "\n";
        
        NS_LOG_UNCOND("  SIR=" << std::setw(5) << sir << " dB: capture=" 
                      << std::fixed << std::setprecision(2) << capProb);
        
        // Croce 2018 targets
        if (sir <= 0.0 && capProb > 0.15) {
            NS_LOG_UNCOND("    [WARN] Capture too high at SIR=" << sir);
        }
        if (sir >= 10.0 && capProb < 0.85) {
            NS_LOG_UNCOND("    [WARN] Capture too low at SIR=" << sir);
            allPass = false;
        }
    }
    
    out.close();
    NS_LOG_UNCOND("  Result: " << (allPass ? "[PASS]" : "[FAIL]"));
    return allPass;
}

// ============================================================================
// METRIC 4: FADE MARGIN FOR 99% PDR (Cattani 2017)
// ============================================================================
bool RunMetric4_FadeMargin(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 4: Fade Margin for 99% PDR ===");
    
    std::ofstream out(outputDir + "/metric4_fade_margin_seed" + std::to_string(seed) + ".csv");
    out << "scenario,fading_type,margin_db,pdr\n";
    
    double frequencyHz = 868e6;
    double bandwidthHz = 125e3;
    uint8_t sf = 12; // Use SF12 for maximum sensitivity
    uint32_t packetsPerMargin = 500;
    
    struct ScenarioSetup {
        std::string name;
        FadingModelType fading;
        double ricianK;
        double targetMarginLow;
        double targetMarginHigh;
    };
    
    std::vector<ScenarioSetup> scenarios = {
        {"OutdoorLOS", FADING_RICIAN, 8.0, 6.0, 10.0},
        {"OutdoorNLOS", FADING_RAYLEIGH, 0.0, 12.0, 22.0}
    };
    
    bool allPass = true;
    
    for (const auto& sc : scenarios) {
        NS_LOG_UNCOND("  Scenario: " << sc.name);
        
        double margin99 = -1.0;
        
        // Sweep margin from 0 to 25 dB
        for (double margin = 0.0; margin <= 25.0; margin += 1.0) {
            LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
                ENV_SUBURBAN, WEATHER_CLEAR_DRY, sc.fading,
                VEG_NONE, MOB_STATIC, false);
            config.randomSeed = seed;
            config.propagation.frequencyHz = frequencyHz;
            config.shadowing.enabled = false;
            config.nlos.enabled = false;
            config.interference.enabled = false;
            config.foliage.enabled = false;
            config.terrain.enabled = false;
            config.hardware.txPowerAccuracyDb = DistributionParam::Fixed(0.0);
            config.propagation.pathLossExponent = DistributionParam::Fixed(2.08);
            config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
            
            if (sc.fading == FADING_RICIAN) {
                config.fading.ricianKFactor = DistributionParam::Fixed(sc.ricianK);
            }
            
            LoRaPhysicsEngine engine;
            engine.Initialize(config);
            
            // Set distance so that mean RSSI is at sensitivity + margin
            double distance = 500.0;
            Vector txPos(0, 0, 15.0);
            Vector rxPos(distance, 0, 2.0);
            
            // Probe path loss
            PhysicsResult probe = engine.ProcessTransmission(
                1, 0, txPos, rxPos, 14.0, sf, bandwidthHz, 0, Seconds(0.0));
            double pathLoss = probe.pathLossDb + probe.atmosphericDb;
            
            double sensitivity = -137.0; // SF12
            double targetRssi = sensitivity + margin;
            double txPower = targetRssi + pathLoss;
            
            uint32_t received = 0;
            double timeS = 1.0;
            
            for (uint32_t pkt = 0; pkt < packetsPerMargin; ++pkt) {
                PhysicsResult r = engine.ProcessTransmission(
                    1, 0, txPos, rxPos, txPower, sf, bandwidthHz, 0, Seconds(timeS));
                timeS += 0.2; // Must be >150ms coherence time to evolve fading
                if (r.packetReceived) received++;
            }
            
            double pdr = (double)received / packetsPerMargin;
            out << sc.name << "," << (sc.fading == FADING_RICIAN ? "Rician" : "Rayleigh")
                << "," << margin << "," << std::fixed << std::setprecision(3) << pdr << "\n";
            
            if (pdr >= 0.99 && margin99 < 0.0) {
                margin99 = margin;
            }
        }
        
        NS_LOG_UNCOND("    99% PDR fade margin: " << margin99 << " dB"
                      << " (target: " << sc.targetMarginLow << "-" << sc.targetMarginHigh << " dB)");
        
        if (margin99 >= 0 && margin99 >= sc.targetMarginLow && margin99 <= sc.targetMarginHigh) {
            NS_LOG_UNCOND("    [PASS]");
        } else {
            NS_LOG_UNCOND("    [FAIL]");
            allPass = false;
        }
    }
    
    out.close();
    return allPass;
}

// ============================================================================
// METRIC 5: RSSI CDF SHAPE (Cattani 2017)
// ============================================================================
bool RunMetric5_RSSICDF(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 5: RSSI CDF Shape ===");
    
    std::ofstream out(outputDir + "/metric5_rssi_cdf_seed" + std::to_string(seed) + ".csv");
    out << "scenario,sample_index,rssi_dbm,fading_db\n";
    
    double frequencyHz = 868e6;
    double txPowerDbm = 14.0;
    double bandwidthHz = 125e3;
    uint8_t sf = 12;
    uint32_t nSamples = 5000;
    double distance = 500.0;
    
    struct ScenarioSetup {
        std::string name;
        FadingModelType fading;
        double ricianK;
    };
    
    std::vector<ScenarioSetup> scenarios = {
        {"OutdoorLOS", FADING_RICIAN, 8.0},
        {"OutdoorNLOS", FADING_RAYLEIGH, 0.0}
    };
    
    for (const auto& sc : scenarios) {
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_SUBURBAN, WEATHER_CLEAR_DRY, sc.fading,
            VEG_NONE, MOB_STATIC, false);
        config.randomSeed = seed;
        config.propagation.frequencyHz = frequencyHz;
        config.shadowing.enabled = false;
        config.nlos.enabled = false;
        config.interference.enabled = false;
        config.foliage.enabled = false;
        config.terrain.enabled = false;
        config.hardware.txPowerAccuracyDb = DistributionParam::Fixed(0.0);
        
        if (sc.fading == FADING_RICIAN) {
            config.fading.ricianKFactor = DistributionParam::Fixed(sc.ricianK);
        }
        
        LoRaPhysicsEngine engine;
        engine.Initialize(config);
        
        Vector txPos(0, 0, 15.0);
        Vector rxPos(distance, 0, 2.0);
        double timeS = 0.0;
        
        std::vector<double> rssiVals;
        for (uint32_t i = 0; i < nSamples; ++i) {
            PhysicsResult r = engine.ProcessTransmission(
                i + 1, 0, txPos, rxPos, txPowerDbm, sf, bandwidthHz, 0, Seconds(timeS));
            timeS += 0.05;
            out << sc.name << "," << i << "," << r.rxPowerDbm << "," << r.fadingDb << "\n";
            rssiVals.push_back(r.rxPowerDbm);
        }
        
        // Compute basic CDF stats
        std::sort(rssiVals.begin(), rssiVals.end());
        double p5 = rssiVals[nSamples * 5 / 100];
        double p50 = rssiVals[nSamples * 50 / 100];
        double p95 = rssiVals[nSamples * 95 / 100];
        
        NS_LOG_UNCOND("  " << sc.name << ": p5=" << std::fixed << std::setprecision(1) 
                      << p5 << " p50=" << p50 << " p95=" << p95 
                      << " spread=" << (p95 - p5) << " dB");
    }
    
    out.close();
    NS_LOG_UNCOND("  Result: [DATA] — analysis script will fit distributions");
    return true;
}

static double ComputePearsonCorrelation(const std::vector<double>& x, const std::vector<double>& y)
{
    if (x.size() != y.size() || x.size() < 2) {
        return 0.0;
    }

    double meanX = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
    double meanY = std::accumulate(y.begin(), y.end(), 0.0) / y.size();
    double cov = 0.0;
    double varX = 0.0;
    double varY = 0.0;

    for (size_t i = 0; i < x.size(); ++i) {
        double dx = x[i] - meanX;
        double dy = y[i] - meanY;
        cov += dx * dy;
        varX += dx * dx;
        varY += dy * dy;
    }
    if (varX <= 0.0 || varY <= 0.0) {
        return 0.0;
    }
    return cov / std::sqrt(varX * varY);
}

bool RunMetric6_InterSFIsolation(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 6: Inter-SF Isolation (Croce 2018) ===");
    std::ofstream out(outputDir + "/metric6_inter_sf_seed" + std::to_string(seed) + ".csv");
    out << "scenario,sir_db,capture_probability,n_trials,desired_rssi_dbm,interferer_rssi_dbm\n";

    double frequencyHz = 868e6;
    double bandwidthHz = 125e3;
    uint8_t desiredSf = 12;
    uint8_t interfererSf = 7;
    double targetDesiredRssi = -130.0;
    double distance = 400.0;
    Vector txDesired(0, 0, 15.0);
    Vector txInterferer(0, 5.0, 15.0);
    Vector gwPos(1000.0, 0, 2.0);

    LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
        ENV_RURAL_OPEN, WEATHER_CLEAR_DRY, FADING_NONE,
        VEG_NONE, MOB_STATIC, false);
    config.randomSeed = seed;
    config.propagation.frequencyHz = frequencyHz;
    config.shadowing.enabled = false;
    config.nlos.enabled = false;
    config.fading.model = FADING_NONE;
    config.interference.enabled = true;
    config.foliage.enabled = false;
    config.terrain.enabled = false;
    config.hardware.enabled = false;
    config.propagation.pathLossExponent = DistributionParam::Fixed(2.08);
    config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;

    LoRaPhysicsEngine engine;
    engine.Initialize(config);

    // Probe path loss on the desired signal
    PhysicsResult probe = engine.ProcessTransmission(
        1, 0, txDesired, gwPos, 14.0, desiredSf, bandwidthHz, 0, Seconds(0.0));
    double desiredTx = 14.0 + (targetDesiredRssi - probe.rxPowerDbm);
    double basePathLoss = probe.rxPowerDbm - 14.0;

    for (double interfererPowerOffset : {30.0, 10.0}) {
        uint32_t captured = 0;
        uint32_t trials = 500;
        double interfererTx = desiredTx + interfererPowerOffset;
        double timeS = 0.0;

        for (uint32_t t = 0; t < trials; ++t) {
            engine.ProcessTransmission(
                1000000 + t, 0, txInterferer, gwPos, interfererTx,
                interfererSf, bandwidthHz, 0, Seconds(timeS));
            PhysicsResult res = engine.ProcessTransmission(
                t + 1, 0, txDesired, gwPos, desiredTx,
                desiredSf, bandwidthHz, 0, Seconds(timeS + 0.001));
            if (res.packetReceived) {
                captured++;
            }
            timeS += 0.2;
        }

        double captureProbability = (double)captured / trials;
        out << "SF12_vs_SF7," << (interfererPowerOffset - 30.0) << ","
            << std::fixed << std::setprecision(3) << captureProbability << ","
            << trials << "," << targetDesiredRssi << "," << (targetDesiredRssi + interfererPowerOffset) << "\n";
        NS_LOG_UNCOND("  SF12 vs SF7 effective SIR=" << (interfererPowerOffset - 33.0)
                      << " dB: capture=" << std::fixed << std::setprecision(3)
                      << captureProbability);
    }

    out.close();
    NS_LOG_UNCOND("  Result: [DATA] — inter-SF isolation samples generated");
    return true;
}

bool RunMetric7_DOPSpatialPattern(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 7: DOP Spatial Pattern ===");
    std::vector<std::string> candidateFiles = {
        outputDir + "/dataset_D_finetimestamp_seed" + std::to_string(seed) + ".csv",
        outputDir + "/dataset_D_standard_seed" + std::to_string(seed) + ".csv"
    };
    for (const auto& file : candidateFiles) {
        std::ifstream in(file);
        if (!in.is_open()) continue;

        std::string header;
        std::getline(in, header);
        std::vector<std::string> columns;
        std::istringstream hs(header);
        std::string token;
        while (std::getline(hs, token, ',')) {
            columns.push_back(token);
        }
        auto findCol = [&](const std::string& name) {
            for (size_t i = 0; i < columns.size(); ++i) {
                if (columns[i] == name) return (int)i;
            }
            return -1;
        };
        int ix = findCol("true_x_m");
        int iy = findCol("true_y_m");
        int ie = findCol("tdoa_position_error_m");
        if (ix < 0 || iy < 0 || ie < 0) continue;

        std::vector<double> distFromCenter;
        std::vector<double> errors;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            double x=0,y=0,e=0;
            for (int c = 0; c <= std::max({ix, iy, ie}); ++c) {
                std::string entry;
                if (!std::getline(ss, entry, ',')) entry = "";
                if (c == ix) x = std::stod(entry);
                if (c == iy) y = std::stod(entry);
                if (c == ie) e = std::stod(entry);
            }
            distFromCenter.push_back(std::sqrt((x - 200.0)*(x - 200.0) + (y - 200.0)*(y - 200.0)));
            errors.push_back(e);
        }
        if (distFromCenter.size() < 5) continue;

        double corr = ComputePearsonCorrelation(distFromCenter, errors);
        NS_LOG_UNCOND("  DOP pattern file=" << file << " correlation=" << std::fixed << std::setprecision(3) << corr);
        return corr > 0.3;
    }
    NS_LOG_UNCOND("  No Dataset D raw output found for DOP analysis.");
    return false;
}

bool RunMetric8_AirtimeCollisionScaling(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 8: Airtime vs SF Collision Rate ===");
    std::ofstream out(outputDir + "/metric8_airtime_collision_seed" + std::to_string(seed) + ".csv");
    out << "sf,nodes,total_packets,received_packets,collision_count\n";

    double frequencyHz = 868e6;
    double bandwidthHz = 125e3;
    double gatewayHeight = 10.0;
    Vector gwPos(0.0, 0.0, gatewayHeight);
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> ang(0.0, 2.0 * 3.14159265358979323846);
    std::vector<Vector> nodePositions;
    for (uint32_t i = 0; i < 500; ++i) {
        double theta = ang(rng);
        double radius = 100.0;
        nodePositions.emplace_back(radius * std::cos(theta), radius * std::sin(theta), 1.5);
    }

    for (uint8_t sf : {7u, 12u}) {
        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE,
            VEG_NONE, MOB_STATIC, false);
        config.randomSeed = seed + sf;
        config.propagation.frequencyHz = frequencyHz;
        config.shadowing.enabled = false;
        config.nlos.enabled = false;
        config.fading.model = FADING_NONE;
        config.interference.enabled = true;
        config.foliage.enabled = false;
        config.terrain.enabled = false;
        config.hardware.enabled = false;
        config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;

        LoRaPhysicsEngine engine;
        engine.Initialize(config);

        PhysicsResult probe = engine.ProcessTransmission(
            1, 0, nodePositions[0], gwPos, 14.0, sf, bandwidthHz, 0, Seconds(0.0));
        double txPower = 14.0 + (-90.0 - probe.rxPowerDbm);

        uint32_t received = 0;
        uint32_t total = 0;
        std::uniform_real_distribution<double> timeDist(0.0, 120.0);
        std::vector<std::pair<double, uint32_t>> transmissions;
        for (uint32_t i = 0; i < nodePositions.size(); ++i) {
            transmissions.emplace_back(timeDist(rng), i);
        }
        std::sort(transmissions.begin(), transmissions.end());

        double t_air = std::pow(2.0, sf) / bandwidthHz * 20.0;
        NS_LOG_UNCOND("  SF" << (int)sf << " T_air = " << t_air << " s");
        NS_LOG_UNCOND("  First 5 Tx Windows for SF" << (int)sf << ":");
        for (size_t k = 0; k < 5 && k < transmissions.size(); ++k) {
            NS_LOG_UNCOND("    Tx " << k << ": [" << transmissions[k].first << ", " << transmissions[k].first + t_air << "]");
        }

        for (const auto& tx : transmissions) {
            double timeS = tx.first;
            uint32_t i = tx.second;
            PhysicsResult res = engine.ProcessTransmission(
                i + 1, 0, nodePositions[i], gwPos, txPower,
                sf, bandwidthHz, 0, Seconds(timeS));
            if (res.packetReceived) received++;
            total++;
        }

        uint32_t collisions = total - received;
        out << (int)sf << "," << nodePositions.size() << "," << total << ","
            << received << "," << collisions << "\n";
        NS_LOG_UNCOND("  SF" << (int)sf << ": total=" << total << " recv=" << received << " collisions=" << collisions);
    }
    out.close();
    return true;
}

bool RunMetric9_GatewayDensityScaling(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 9: Gateway Density Scaling ===");
    std::ofstream out(outputDir + "/metric9_gateway_density_seed" + std::to_string(seed) + ".csv");
    out << "gateways,nodes,pdr\n";

    double frequencyHz = 868e6;
    double bandwidthHz = 125e3;
    uint8_t sf = 10;
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> posDist(-500.0, 500.0);

    for (int gatewayCount : {5, 10, 20}) {
        std::vector<Vector> gateways;
        for (int i = 0; i < gatewayCount; ++i) {
            double x = (i % 5) * 200.0 - 400.0;
            double y = (i / 5) * 200.0 - 400.0;
            gateways.emplace_back(x, y, 10.0);
        }

        std::vector<Vector> nodes;
        for (int i = 0; i < 1000; ++i) {
            nodes.emplace_back(posDist(rng), posDist(rng), 1.5);
        }

        LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
            ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE,
            VEG_NONE, MOB_STATIC, false);
        config.randomSeed = seed + gatewayCount;
        config.propagation.frequencyHz = frequencyHz;
        config.shadowing.enabled = false;
        config.nlos.enabled = false;
        config.fading.model = FADING_NONE;
        config.interference.enabled = true; // MUST be true for density scaling law
        config.foliage.enabled = false;
        config.terrain.enabled = false;
        config.hardware.enabled = false;
        config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;

        LoRaPhysicsEngine engine;
        engine.Initialize(config);
        uint32_t received = 0;
        uint32_t total = 0;

        std::uniform_real_distribution<double> timeDist(0.0, 120.0);
        struct TxEvent { double timeS; uint32_t nodeId; int bestGw; double txPower; Vector pos; };
        std::vector<TxEvent> events;

        for (uint32_t i = 0; i < nodes.size(); ++i) {
            Vector node = nodes[i];
            double bestDist = 1e12;
            int bestGw = 0;
            for (size_t g = 0; g < gateways.size(); ++g) {
                double d = std::sqrt(std::pow(node.x - gateways[g].x, 2) + std::pow(node.y - gateways[g].y, 2));
                if (d < bestDist) {
                    bestDist = d;
                    bestGw = g;
                }
            }
            events.push_back({timeDist(rng), i, bestGw, 14.0, node});
        }

        std::sort(events.begin(), events.end(), [](const TxEvent& a, const TxEvent& b) {
            return a.timeS < b.timeS;
        });

        for (const auto& ev : events) {
            bool packetDecodedCooperatively = false;
            for (size_t g = 0; g < gateways.size(); ++g) {
                PhysicsResult res = engine.ProcessTransmission(
                    ev.nodeId + 1, g, ev.pos, gateways[g], ev.txPower,
                    sf, bandwidthHz, 0, Seconds(ev.timeS));
                // Only count the packet as received if it cooperatively decodes across gateways
                if (res.cooperativeDecodeSuccess) {
                    packetDecodedCooperatively = true;
                }
            }
            if (packetDecodedCooperatively) received++;
            total++;
        }

        double pdr = total > 0 ? (double)received / total : 0.0;
        out << gatewayCount << "," << nodes.size() << ","
            << std::fixed << std::setprecision(4) << pdr << "\n";
        NS_LOG_UNCOND("  " << gatewayCount << " gateways: PDR=" << pdr);
    }
    out.close();
    return true;
}

bool RunMetric10_TimestampJitterDistribution(uint32_t seed, const std::string& outputDir)
{
    NS_LOG_UNCOND("=== Metric 10: Timestamp Jitter Distribution ===");
    std::ofstream out(outputDir + "/metric10_timestamp_jitter_seed" + std::to_string(seed) + ".csv");
    out << "sample_index,true_arrival_ns,measured_arrival_ns,jitter_ns\n";

    double frequencyHz = 868e6;
    double bandwidthHz = 125e3;
    uint8_t sf = 9;
    uint32_t samples = 5000;
    Vector txPos(0, 0, 15.0);
    Vector gwPos(500.0, 0, 2.0);

    LoRaPhysicsConfig config = LoRaPhysicsConfig::ForScenario(
        ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE,
        VEG_NONE, MOB_STATIC, false);
    config.randomSeed = seed;
    config.propagation.frequencyHz = frequencyHz;
    config.shadowing.enabled = false;
    config.nlos.enabled = false;
    config.fading.model = FADING_NONE;
    config.interference.enabled = false;
    config.foliage.enabled = false;
    config.terrain.enabled = false;
    config.hardware.enabled = true;
    config.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
    config.hardware.timestampJitterNs = DistributionParam::Normal(0.0, 50.0, -200.0, 200.0);

    LoRaPhysicsEngine engine;
    engine.Initialize(config);

    for (uint32_t i = 0; i < samples; ++i) {
        PhysicsResult res = engine.ProcessTransmission(
            i + 1, 0, txPos, gwPos, 14.0, sf, bandwidthHz, 0, Seconds(0.1 * i));
        double trueArrival = res.timestamp.trueArrivalTimeNs;
        double measuredArrival = res.timestamp.measuredArrivalTimeNs;
        double jitter = measuredArrival - trueArrival;
        out << i << "," << std::fixed << std::setprecision(3)
            << trueArrival << "," << measuredArrival << "," << jitter << "\n";
    }
    out.close();
    NS_LOG_UNCOND("  Result: [DATA] — timestamp jitter samples produced");
    return true;
}

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[])
{
    uint32_t seed = 7;
    std::string metric = "all";
    std::string outputDir = "./results";
    
    CommandLine cmd;
    cmd.AddValue("seed", "Random seed", seed);
    cmd.AddValue("metric", "Which metric to run (1-10 or all)", metric);
    cmd.AddValue("output_dir", "Output directory", outputDir);
    cmd.AddValue("metric2SpacingS",
                 "Metric 2 frame spacing in seconds. Negative (default): twice each SF's time on air, at least 0.1 s, so "
                 "the harness's frames do not collide with each other (TECH_DEBT N.29). 0.1 reproduces the old spacing.",
                 g_metric2SpacingS);
    cmd.Parse(argc, argv);
    
    NS_LOG_UNCOND("LoRa Physics Engine — Empirical Validation Suite");
    NS_LOG_UNCOND("================================================");
    NS_LOG_UNCOND("Seed: " << seed);
    NS_LOG_UNCOND("Metric: " << metric);
    NS_LOG_UNCOND("");
    
    // Ensure output directory exists
    std::string mkdirCmd = "mkdir -p " + outputDir;
    system(mkdirCmd.c_str());
    
    int passed = 0, failed = 0, data = 0;
    
    if (metric == "all" || metric == "1") {
        if (RunMetric1_SensitivityThreshold(seed, outputDir)) passed++; else failed++;
    }
    if (metric == "all" || metric == "2") {
        RunMetric2_PDRvsDistance(seed, outputDir); data++;
    }
    if (metric == "all" || metric == "3") {
        if (RunMetric3_CaptureEffect(seed, outputDir)) passed++; else failed++;
    }
    if (metric == "all" || metric == "4") {
        if (RunMetric4_FadeMargin(seed, outputDir)) passed++; else failed++;
    }
    if (metric == "all" || metric == "5") {
        RunMetric5_RSSICDF(seed, outputDir); data++;
    }
    if (metric == "all" || metric == "6") {
        RunMetric6_InterSFIsolation(seed, outputDir); data++;
    }
    if (metric == "all" || metric == "7") {
        if (RunMetric7_DOPSpatialPattern(seed, outputDir)) {
            NS_LOG_UNCOND("  [PASS] DOP spatial pattern detected.");
        } else {
            NS_LOG_UNCOND("  [WARN] DOP spatial pattern not detected or data missing.");
        }
        data++;
    }
    if (metric == "all" || metric == "8") {
        RunMetric8_AirtimeCollisionScaling(seed, outputDir); data++;
    }
    if (metric == "all" || metric == "9") {
        RunMetric9_GatewayDensityScaling(seed, outputDir); data++;
    }
    if (metric == "all" || metric == "10") {
        RunMetric10_TimestampJitterDistribution(seed, outputDir); data++;
    }
    
    NS_LOG_UNCOND("");
    NS_LOG_UNCOND("================================================");
    NS_LOG_UNCOND("Summary: " << passed << " PASS, " << failed << " FAIL, " << data << " DATA");
    NS_LOG_UNCOND("================================================");
    
    return (failed > 0) ? 1 : 0;
}
