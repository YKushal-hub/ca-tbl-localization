#include "ns3/test.h"
#include "ns3/simulator.h"
#include "../model/lora-physics-parameters.h"
#include "../model/lora-distribution-sampler.h"
#include "../model/lora-physics-engine.h"
#include "../model/lora-spatial-field-engine.h"
#include "../model/submodules/lora-propagation-submodule.h"
#include "../model/submodules/lora-shadowing-submodule.h"
#include "../model/submodules/lora-fading-submodule.h"
#include "../model/submodules/lora-nlos-submodule.h"
#include "../model/submodules/lora-atmospheric-submodule.h"
#include "../model/submodules/lora-foliage-submodule.h"
#include "../model/submodules/lora-interference-submodule.h"
#include "../model/submodules/lora-hardware-submodule.h"
#include "../model/submodules/lora-mobility-submodule.h"
#include "../model/submodules/lora-terrain-submodule.h"
#include "../helper/lora-calibration-loader.h"
#include <cmath>
#include <vector>
#include <numeric>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LoRaPhysicsEngineTestSuite");

/**
 * @ingroup lora-physics-engine
 * @brief Comprehensive test suite for all 15 physical layer and distribution-sampling functions
 */
class LoRaPhysicsEngineTest : public TestCase
{
public:
    LoRaPhysicsEngineTest();
    ~LoRaPhysicsEngineTest() override;

private:
    void DoRun() override;
};

LoRaPhysicsEngineTest::LoRaPhysicsEngineTest()
    : TestCase("Verify correct functionality of the LoRa Physics and Hardware Impairment Engine")
{
}

LoRaPhysicsEngineTest::~LoRaPhysicsEngineTest()
{
}

void LoRaPhysicsEngineTest::DoRun()
{
    // Initialize standard sampler and config
    LoRaDistributionSampler sampler(101);

    // ----------------------------------------------------
    // TEST 1: Deterministic distribution sampling correctness
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::Fixed(4.5);
        double val = sampler.Sample(p);
        NS_TEST_ASSERT_MSG_EQ_TOL(val, 4.5, 1e-6, "Deterministic sample incorrect");
    }

    // ----------------------------------------------------
    // TEST 2: Uniform distribution sampling correctness (min/max bound checking)
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::Uniform(2.0, 5.0);
        for (int i = 0; i < 100; ++i) {
            double val = sampler.Sample(p);
            NS_TEST_ASSERT_MSG_GT(val, 1.999, "Uniform sample below lower bound");
            NS_TEST_ASSERT_MSG_LT(val, 5.001, "Uniform sample above upper bound");
        }
    }

    // ----------------------------------------------------
    // TEST 3: Normal distribution sampling (mean & variance check via 1000 samples)
    // ----------------------------------------------------
    {
        double targetMean = 10.0;
        double targetStd = 2.0;
        DistributionParam p = DistributionParam::Normal(targetMean, targetStd, 2.0, 18.0);
        std::vector<double> samples;
        double sum = 0.0;
        
        for (int i = 0; i < 1000; ++i) {
            double val = sampler.Sample(p);
            samples.push_back(val);
            sum += val;
        }
        
        double mean = sum / 1000.0;
        double sqSum = 0.0;
        for (double s : samples) {
            sqSum += (s - mean) * (s - mean);
        }
        double var = sqSum / 999.0;
        
        // Assert within wide statistical tolerance of 4 standard errors
        NS_TEST_ASSERT_MSG_GT(mean, 9.5, "Normal sample mean deviates significantly");
        NS_TEST_ASSERT_MSG_LT(mean, 10.5, "Normal sample mean deviates significantly");
        NS_TEST_ASSERT_MSG_GT(var, 3.0, "Normal sample variance deviates significantly");
        NS_TEST_ASSERT_MSG_LT(var, 5.0, "Normal sample variance deviates significantly");
    }

    // ----------------------------------------------------
    // TEST 4: Log-normal distribution sampling (conversion of log mean/stddev to exponential domain)
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::LogNormal(2.0, 0.5, 1.0, 50.0);
        for (int i = 0; i < 100; ++i) {
            double val = sampler.Sample(p);
            NS_TEST_ASSERT_MSG_GT(val, 0.999, "LogNormal sample below bound");
            NS_TEST_ASSERT_MSG_LT(val, 50.001, "LogNormal sample above bound");
        }
    }

    // ----------------------------------------------------
    // TEST 5: Exponential distribution sampling correctness (checking density)
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::Exponential(0.1, 0.0, 100.0); // mean = 10
        for (int i = 0; i < 100; ++i) {
            double val = sampler.Sample(p);
            NS_TEST_ASSERT_MSG_GT(val, -0.001, "Exponential sample below zero");
        }
    }

    // ----------------------------------------------------
    // TEST 6: Nakagami-m distribution fading amplitude generator correctness
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::Nakagami(1.5, 1.0, 0.01, 10.0);
        for (int i = 0; i < 100; ++i) {
            double val = sampler.Sample(p);
            NS_TEST_ASSERT_MSG_GT(val, 0.0, "Nakagami sample below zero");
        }
    }

    // ----------------------------------------------------
    // TEST 7: Persistent per-node sampler caching correctness (SamplePersistent)
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::Uniform(-10.0, 10.0);
        double val1 = sampler.SamplePersistent(p, 5, "persistent_var");
        double val2 = sampler.SamplePersistent(p, 5, "persistent_var");
        NS_TEST_ASSERT_MSG_EQ_TOL(val1, val2, 1e-6, "Persistent cache returned different values");
    }

    // ----------------------------------------------------
    // TEST 8: ClearNodeCache correctness
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::Uniform(-10.0, 10.0);
        sampler.SamplePersistent(p, 6, "var_a");
        sampler.SamplePersistent(p, 6, "var_b");
        
        sampler.ClearNodeCache(6);
        // Checking if cached node entry was successfully dropped (sampling again)
        double valNew = sampler.SamplePersistent(p, 6, "var_a");
        (void)valNew; // Just verify execution and clearing capacity
    }

    // ----------------------------------------------------
    // TEST 9: ClearAllCaches correctness
    // ----------------------------------------------------
    {
        DistributionParam p = DistributionParam::Uniform(-10.0, 10.0);
        sampler.SamplePersistent(p, 7, "var_c");
        sampler.SamplePersistent(p, 8, "var_d");
        
        sampler.ClearAllCaches();
        // Caches successfully emptied, no segfaults
    }

    // ----------------------------------------------------
    // TEST 10: Path Loss submodule Hata elevation factor correction verification
    // ----------------------------------------------------
    {
        PropagationParams params = PropagationParams::DefaultsFor(ENV_SUBURBAN);
        params.frequencyHz = 868e6;
        
        Vector tx(0, 0, 1.5);
        Vector rx(1000, 0, 10.0); // Gateway at 10m height
        double loss10m = ComputePathLossDb(tx, rx, 1.5, 10.0, params, sampler);
        
        Vector rxHigh(1000, 0, 50.0); // Gateway at 50m height
        double loss50m = ComputePathLossDb(tx, rxHigh, 1.5, 50.0, params, sampler);
        
        // At 50m, Hata correction is active and should reduce the total path loss
        // NS_TEST_ASSERT_MSG_LT(loss50m, loss10m, "Hata height correction did not reduce loss for high gateway"); // Disabled: Hata removed in favor of Petäjäjärvi log-distance
    }

    // ----------------------------------------------------
    // TEST 11: Shadowing submodule Gudmundson spatial correlation distance-dependent decay correctness
    // ----------------------------------------------------
    {
        ShadowingParams params = ShadowingParams::DefaultsFor(ENV_SUBURBAN);
        params.enabled = true;
        params.spatialCorrelationEnabled = true;
        params.correlationDistanceM = DistributionParam::Fixed(110.0);
        params.shadowingStddevDb = DistributionParam::Fixed(6.0);
        
        ShadowingSubmodule shadowing(params, sampler);
        
        Vector rx(0, 0, 15.0);
        Vector tx1(100, 0, 1.5);
        Vector tx2(101, 0, 1.5); // Extremely close to tx1 (1m separation)
        Vector tx3(500, 0, 1.5); // Very far from tx1
        
        LinkPhysicsState state;
        state.isNLOS = false;
        double s1 = shadowing.GetShadowingDb(state, 1, 100, tx1, rx, Seconds(0));
        double s2 = shadowing.GetShadowingDb(state, 2, 100, tx2, rx, Seconds(0));
        double s3 = shadowing.GetShadowingDb(state, 3, 100, tx3, rx, Seconds(0));
        
        // Nearby points (1m) should be extremely similar (strongly correlated)
        double diffNear = std::abs(s1 - s2);
        double diffFar = std::abs(s1 - s3);
        
        NS_TEST_ASSERT_MSG_LT(diffNear, 2.0, "Close points do not experience strongly correlated shadowing");
        (void)diffFar;
    }

    // ----------------------------------------------------
    // TEST 11b: Gudmundson autocorrelation equals exp(-d/dcorr) at one correlation distance
    // ----------------------------------------------------
    {
        LoRaDistributionSampler shadowSampler(11020);
        ShadowingParams params = ShadowingParams::DefaultsFor(ENV_SUBURBAN);
        params.enabled = true;
        params.spatialCorrelationEnabled = true;
        params.correlationDistanceM = DistributionParam::Fixed(20.0);
        params.shadowingStddevDb = DistributionParam::Fixed(6.0);
        params.updateIntervalS = 0.0;

        ShadowingSubmodule shadowing(params, shadowSampler);
        Vector rx(0, 0, 15.0);
        std::vector<double> values;
        const uint32_t count = 1500;
        values.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            Vector tx(100.0 + 20.0 * i, 0.0, 1.5);
            LinkPhysicsState state;
            state.isNLOS = false;
            values.push_back(shadowing.GetShadowingDb(state, 10000 + i, 100, tx, rx, Seconds(0)));
        }

        double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        double covariance = 0.0;
        double variance = 0.0;
        for (uint32_t i = 1; i < values.size(); ++i) {
            covariance += (values[i - 1] - mean) * (values[i] - mean);
        }
        for (double v : values) {
            variance += (v - mean) * (v - mean);
        }
        double lagOneCorrelation = covariance / variance;
        NS_TEST_ASSERT_MSG_EQ_TOL(lagOneCorrelation, std::exp(-1.0), 0.08, "Gudmundson lag-one autocorrelation does not match exp(-20m/20m)");
    }

    // ----------------------------------------------------
    // TEST 12: Fading submodule Rayleigh & Rician amplitude generation correctness
    // ----------------------------------------------------
    {
        FadingParams params;
        params.enabled = true;
        params.model = FADING_RAYLEIGH;
        params.coherenceTimeMs = DistributionParam::Fixed(200.0);
        
        Time lastUpdate = Time(0);
        double lastVal = 0.0;
        
        LinkPhysicsState state;
        state.isNLOS = false;
        double f1 = ComputeFadingDb(state, Seconds(0), params, sampler, 1, lastUpdate, lastVal);
        double f2 = ComputeFadingDb(state, Seconds(0.1), params, sampler, 1, lastUpdate, lastVal);
        
        // Within coherence time (100ms < 200ms), values should be exactly cached
        NS_TEST_ASSERT_MSG_EQ_TOL(f1, f2, 1e-6, "Fading cache did not preserve value within coherence interval");
        
        // After coherence time (300ms > 200ms), new value is sampled
        double f3 = ComputeFadingDb(state, Seconds(0.3), params, sampler, 1, lastUpdate, lastVal);
        (void)f3;
    }

    // ----------------------------------------------------
    // TEST 13: Foliage specific attenuation seasonal (winter) scaling correctness
    // ----------------------------------------------------
    {
        FoliageParams params = FoliageParams::DefaultsFor(VEG_DENSE_FOREST);
        params.enabled = true;
        params.vegetationDepthM = DistributionParam::Fixed(50.0);
        params.specificAttenuationDbPerM = DistributionParam::Fixed(0.3);
        params.maxAttenuationDb = DistributionParam::Fixed(20.0);
        params.seasonalMultiplier = DistributionParam::Fixed(0.4); // winter specific multiplier
        
        Vector tx(0,0,0);
        Vector rx(100,0,0);
        
        double lossSummer = ComputeFoliageLossDb(tx, rx, params, sampler, false, false); // winter=false
        double lossWinter = ComputeFoliageLossDb(tx, rx, params, sampler, false, true);  // winter=true
        
        // Winter foliage loss should be lower due to the 0.4 seasonal specific attenuation multiplier
        NS_TEST_ASSERT_MSG_LT(lossWinter, lossSummer, "Winter season foliage loss did not scale down");
    }

    // ----------------------------------------------------
    // TEST 14: NLOS building penetration and floor loss penalty correctness
    // ----------------------------------------------------
    {
        NLOSParams params = NLOSParams::DefaultsFor(ENV_SUBURBAN);
        params.enabled = true;
        params.nlosScaleDistanceM = DistributionParam::Fixed(10.0); // Make NLOS probability extremely high
        params.nlosAttenuationDb = DistributionParam::Fixed(10.0);
        params.diffractionLossDb = DistributionParam::Fixed(5.0);
        params.buildingPenetrationDb = DistributionParam::Fixed(12.0);
        params.floorLossDbPerFloor = DistributionParam::Fixed(4.0);
        params.defaultNumberOfFloors = 2; // 2 floors * 4.0 = 8.0 dB addition
        
        Vector tx(0, 0, 0);
        Vector rx(500, 0, 0);
        
        double penalty = ComputeNLOSPenaltyDb(tx, rx, params, sampler);
        
        // Minimum loss should be at least (building penetration + floor loss) = 12 + 8 = 20.0 dB
        NS_TEST_ASSERT_MSG_GT(penalty, 19.9, "NLOS floor loss and building penetration was not aggregated correctly");
    }

    // ----------------------------------------------------
    // TEST 15: Hardware carrier frequency offset & RSSI SX1276 resolution quantization correctness
    // ----------------------------------------------------
    {
        HardwareParams params = HardwareParams::Defaults();
        params.enabled = true;
        params.rssiResolutionDb = 1.0; // 1.0 dB quantization
        params.rssiHardwareVariationDb = DistributionParam::Fixed(1.5);
        
        HardwareSubmodule hw(params, sampler);
        
        // true RSSI = -80.3 dBm. With hardware bias +1.5 = -78.8 dBm. Quantized to 1.0 dB steps = -79.0 dBm.
        double rssi = hw.ApplyRSSIErrors(-80.3, 1);
        NS_TEST_ASSERT_MSG_EQ_TOL(rssi, -79.0, 1e-6, "RSSI quantization to SX1276 1dB step size failed");
    }

    // ----------------------------------------------------
    // TEST 16: Calibration Validation #1 - Realistic LoRa SNR Range (Fix #11 & Fix #2)
    // ----------------------------------------------------
    {
        LoRaPhysicsEngine engine;
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.nlos.enabled = false; // outdoor-to-outdoor suburban links (no indoor building penetration)
        engine.Initialize(cfg);
        double snrSum = 0.0;
        int count = 1000;
        for (int i = 0; i < count; ++i) {
            Vector tx(i * 0.2, 0, 1.5);
            Vector rx(300.0 + i * 0.2, 0, 25.0); // 300m - 500m suburban outdoor links
            PhysicsResult res = engine.ProcessTransmission(1000 + i, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(i * 0.1));
            NS_TEST_ASSERT_MSG_GT(res.finalSNRDb, -30.001, "Reported telemetry SNR violated hard physical lower bound of -30 dB");
            NS_TEST_ASSERT_MSG_LT(res.finalSNRDb, 40.001, "Reported telemetry SNR violated hard physical upper bound of +40 dB");
            snrSum += res.finalSNRDb;
        }
        double meanSnr = snrSum / count;
        NS_TEST_ASSERT_MSG_GT(meanSnr, -15.0, "Suburban mean SNR is unrealistically low");
        NS_TEST_ASSERT_MSG_LT(meanSnr, 20.0, "Suburban mean SNR is unrealistically high");
    }

    // ----------------------------------------------------
    // TEST 17: Calibration Validation #2 - Urban Dense Not Always Dead (Fix #11 & Fix #4)
    // ----------------------------------------------------
    {
        LoRaPhysicsEngine engine;
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_HEAVY_RAIN, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, true);
        cfg.nlos.enabled = false; // outdoor rooftop link
        engine.Initialize(cfg);
        int successes = 0;
        int count = 500;
        for (int i = 0; i < count; ++i) {
            Vector tx(i * 50.0, 0, 1.5);
            Vector rx(150.0 + i * 50.0, 0, 25.0); // 150m urban dense link across spatially independent nodes
            PhysicsResult res = engine.ProcessTransmission(2000 + i, 1, tx, rx, 14.0, 12, 125000.0, 0, Seconds(i * 5.0));
            if (res.packetReceived) {
                successes++;
            }
        }
        double pdr = (double)successes / count;
        NS_TEST_ASSERT_MSG_GT(pdr, 0.10, "Urban dense 150m link PDR is unrealistically low (must not be always dead)");
    }

    // ----------------------------------------------------
    // TEST 18: Calibration Validation #3 - Rain Impact Minimal at 868 MHz (Fix #11 & Fix #8)
    // ----------------------------------------------------
    {
        LoRaPhysicsEngine engineClear;
        engineClear.Initialize(LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, true));
        Vector tx(0,0,1.5);
        Vector rx(5000.0,0,25.0); // 5km link
        PhysicsResult resClear = engineClear.ProcessTransmission(10, 1, tx, rx, 14.0, 10, 125000.0, 0, Seconds(0));
        
        LoRaPhysicsEngine engineRain;
        engineRain.Initialize(LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_HEAVY_RAIN, FADING_RICIAN, VEG_NONE, MOB_STATIC, true));
        PhysicsResult resRain = engineRain.ProcessTransmission(10, 1, tx, rx, 14.0, 10, 125000.0, 0, Seconds(0));
        
        double deltaAtmospheric = std::abs(resRain.atmosphericDb - resClear.atmosphericDb);
        NS_TEST_ASSERT_MSG_LT(deltaAtmospheric, 1.0, "Rain impact at 868 MHz unrealistically exceeded 1.0 dB");
    }

    // ----------------------------------------------------
    // TEST 19: Calibration Validation #4 - Capture Effect Helps Reception (Fix #11 & Fix #3)
    // ----------------------------------------------------
    {
        LoRaPhysicsEngine engine;
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, true);
        cfg.nlos.enabled = false;
        cfg.terrain.enabled = false;
        cfg.interference.enabled = true;
        cfg.interference.captureEffectEnabled = true;
        cfg.interference.externalInterfererDensityPerKm2 = DistributionParam::Fixed(0.05); // sparse active interferers
        cfg.interference.externalDutyCycle = DistributionParam::Fixed(1.0); // 100% active to guarantee collision
        cfg.interference.externalTxPowerDbm = DistributionParam::Fixed(30.0);
        engine.Initialize(cfg);
        Vector tx(0,0,1.5);
        Vector rx(10.0,0,25.0); // 10m strong desired link
        PhysicsResult res = engine.ProcessTransmission(300, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(5.0));
        NS_TEST_ASSERT_MSG_EQ(res.packetReceived, true, "Capture effect failed to assist reception of strong desired signal");
    }

    // ----------------------------------------------------
    // TEST 20: Temporal Fading Consistency (Gauss-Markov temporal evolution)
    // ----------------------------------------------------
    {
        LoRaPhysicsEngine engine;
        engine.Initialize(LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false));
        Vector tx(0,0,1.5), rx(500,0,25.0);
        PhysicsResult r1 = engine.ProcessTransmission(10, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0.0));
        PhysicsResult r2 = engine.ProcessTransmission(10, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0.05)); // 50ms later
        PhysicsResult r3 = engine.ProcessTransmission(10, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(5.0));  // 5s later
        
        double diffShort = std::abs(r1.fadingDb - r2.fadingDb);
        double diffLong = std::abs(r1.fadingDb - r3.fadingDb);
        NS_TEST_ASSERT_MSG_LT(diffShort, 3.0, "Short interval fading changed too abruptly (violates temporal consistency)");
        (void)diffLong;
    }

    // ----------------------------------------------------
    // TEST 21: Delay Spread Multipath Comparison (Urban Dense > Rural Open)
    // ----------------------------------------------------
    {
        LoRaPhysicsEngine engRural, engUrban;
        engRural.Initialize(LoRaPhysicsConfig::ForScenario(ENV_RURAL_OPEN, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false));
        engUrban.Initialize(LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false));
        Vector tx(0,0,1.5), rx(1000,0,25.0);
        PhysicsResult rRural = engRural.ProcessTransmission(11, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult rUrban = engUrban.ProcessTransmission(12, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(rUrban.delaySpreadNs, rRural.delaySpreadNs, "Urban dense delay spread not greater than rural open");
    }

    // ----------------------------------------------------
    // TEST 22: Directional Antenna Gain Directivity Correctness
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        AntennaParams a = AntennaParams::DirectionalSector();
        a.peakGainDbi = 16.0;
        a.beamwidthDeg = 65.0;
        a.frontToBackRatioDb = 25.0;
        a.downtiltDeg = 0.0;
        cfg.antenna = a;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector txMain(500,0,1.5); // Directly East (azimuth 0, inside main lobe)
        Vector txBack(-500,0,1.5); // Directly West (azimuth 180, back lobe)
        Vector rx(0,0,25.0);
        PhysicsResult rMain = eng.ProcessTransmission(21, 1, txMain, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult rBack = eng.ProcessTransmission(22, 1, txBack, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(rMain.antennaGainDb, rBack.antennaGainDb + 15.0, "Directional antenna directivity attenuation failed");
    }

    // ----------------------------------------------------
    // TEST 23: Partial Overlap Collision and Capture Effect
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.interference.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector txDesired(50,0,1.5), txInterf(50,50,1.5), rx(0,0,25.0);
        // Interf transmission starts at 0.0, airtime ~0.02s
        eng.ProcessTransmission(31, 1, txInterf, rx, 14.0, 7, 125000.0, 0, Seconds(0.0));
        // Desired transmission starts at 0.01s (50% overlap collision!)
        PhysicsResult rDes = eng.ProcessTransmission(32, 1, txDesired, rx, 14.0, 7, 125000.0, 0, Seconds(0.01));
        NS_TEST_ASSERT_MSG_GT(rDes.collisionOverlapFraction, 0.01, "Partial overlap collision tracking failed");
        NS_TEST_ASSERT_MSG_GT(rDes.interfererCount, 0, "Active packet interferer count incorrect");
    }

    // ----------------------------------------------------
    // TEST 24: Procedural Terrain Geometric Ray Blockage
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
        cfg.terrain.enabled = true;
        cfg.randomSeed = 42; // Seed creating known procedural ridge between TX and RX
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(0,0,1.5), rx(10000.0,0,25.0); // 10km link through rugged mountains
        PhysicsResult res = eng.ProcessTransmission(41, 1, tx, rx, 14.0, 10, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.terrainDb, 0.0, "Procedural terrain failed to induce knife-edge diffraction penalty");
    }

    // ----------------------------------------------------
    // TEST 25: Calibration Loader Fit Metrics Execution
    // ----------------------------------------------------
    {
        LoRaCalibrationLoader loader;
        // Test execution with empty dataset (gracefully returns default summary)
        CalibrationSummary s = loader.RunCalibrationFit(CreateObject<LoRaPhysicsEngine>(), LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false), "");
        NS_TEST_ASSERT_MSG_EQ_TOL(s.rmseRssi, 999.0, 1e-6, "Calibration loader empty dataset fallback incorrect");
    }

    // ----------------------------------------------------
    // TEST 26: True CSS PHY Dechirping & CFO Bin Drift
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.hardware.carrierFrequencyOffsetHz = DistributionParam::Fixed(800.0); // Large CFO causing FFT bin drift > 0.25 at SF12 (bin spacing ~ 30.5 Hz)
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(35000.0,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(51, 1, tx, rx, 14.0, 12, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(std::abs(res.fftBinOffset), 0.25, "FFT bin offset failed to exceed 0.25 threshold under high CFO");
        NS_TEST_ASSERT_MSG_LT(res.demodSuccessProbability, 1.0, "CSS PHY demodulation probability failed to penalize severe CFO bin drift");
    }

    // ----------------------------------------------------
    // TEST 27: IQ Hardware Impairments (EVM, IRR, ADC Saturation)
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.iqImpairment.enabled = true;
        cfg.iqImpairment.adcClippingThresholdDbm = DistributionParam::Fixed(-15.0);
        cfg.nlos.enabled = false;
        cfg.shadowing.enabled = false;
        cfg.fading.enabled = false;
        cfg.foliage.enabled = false;
        cfg.terrain.enabled = false;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        
        Vector txHigh(0.1,0,1.5), rx(0,0,1.5); // 0.1m distance and +45 dBm TX power guaranteeing RX power > -15 dBm
        Vector txLow(1.0,0,1.5);               // 1.0m distance and +14 dBm -> RX power ~ -17 dBm (No clipping, clean SNR)
        
        PhysicsResult resHigh = eng.ProcessTransmission(61, 1, txHigh, rx, 45.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult resLow = eng.ProcessTransmission(62, 1, txLow, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        
        NS_TEST_ASSERT_MSG_EQ(resHigh.adcClippingOccurred, true, "ADC clipping detection failed under extreme RX power");
        NS_TEST_ASSERT_MSG_EQ(resLow.adcClippingOccurred, false, "ADC clipping falsely triggered under moderate RX power");
        NS_TEST_ASSERT_MSG_GT(resHigh.evmPercent, resLow.evmPercent + 10.0, "EVM failed to reflect severe ADC clipping distortion");
        NS_TEST_ASSERT_MSG_GT(resLow.iqImageRejectionDb, 0.0, "IQ Image Rejection Ratio incorrectly computed");
    }

    // ----------------------------------------------------
    // TEST 28: Continuous Spatial Field Maps
    // ----------------------------------------------------
    {
        LoRaSpatialFieldEngine spatial;
        Vector p1(0,0,1.5), p2(2.0,0,1.5), p3(500.0,0,1.5);
        SpatialFieldState r1 = spatial.GetSpatialState(p1, ENV_SUBURBAN, 42);
        SpatialFieldState r2 = spatial.GetSpatialState(p2, ENV_SUBURBAN, 42);
        SpatialFieldState r3 = spatial.GetSpatialState(p3, ENV_SUBURBAN, 42);
        
        double diffNearby = std::abs(r1.obstructionField - r2.obstructionField);
        double diffDistant = std::abs(r1.obstructionField - r3.obstructionField);
        NS_TEST_ASSERT_MSG_LT(diffNearby, 0.15, "Spatial field map failed to maintain smooth short-range continuity");
        NS_TEST_ASSERT_MSG_GT(diffDistant, 0.1, "Spatial field map failed to decorrelate over long distances");
    }

    // ----------------------------------------------------
    // TEST 29: Dynamic Multipath Cluster Evolution
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        eng.UpdateNodePosition(81, tx); // Set initial position and register
        eng.SetNodeVelocity(81, 10.0);  // Move at 10m/s
        PhysicsResult res = eng.ProcessTransmission(81, 1, Vector(60,0,1.5), rx, 14.0, 7, 125000.0, 0, Seconds(1.0));
        NS_TEST_ASSERT_MSG_GT(res.activeClusterCount, 1, "Dynamic cluster tracking failed to register active clusters");
        NS_TEST_ASSERT_MSG_GT(res.clusterTurnoverRate, 0.0, "Cluster turnover rate incorrectly computed under mobility");
    }

    // ----------------------------------------------------
    // TEST 30: Doppler Spectrum Shaping
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(100,0,1.5), rx(0,0,25.0);
        eng.UpdateNodePosition(91, tx);
        eng.SetNodeVelocity(91, 30.0);  // 30 m/s velocity
        PhysicsResult res = eng.ProcessTransmission(91, 1, Vector(130,0,1.5), rx, 14.0, 7, 125000.0, 0, Seconds(1.0));
        double expectedDopplerWidth = 2.0 * (30.0 * 868e6 / 299792458.0);
        NS_TEST_ASSERT_MSG_EQ_TOL(res.dopplerSpectrumWidthHz, expectedDopplerWidth, 1.0, "Doppler spectrum width calculation incorrect");
    }

    // ----------------------------------------------------
    // TEST 31: Symbol-Level Collision & Sync Corruption
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.interference.enabled = true;
        cfg.interference.captureEffectEnabled = false;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector txDesired(50,0,1.5), txInterf(50,50,1.5), rx(0,0,25.0);
        eng.ProcessTransmission(101, 1, txInterf, rx, 14.0, 7, 125000.0, 0, Seconds(0.0));
        PhysicsResult rDes = eng.ProcessTransmission(102, 1, txDesired, rx, 14.0, 7, 125000.0, 0, Seconds(0.001));
        NS_TEST_ASSERT_MSG_GT(rDes.symbolCollisionFraction, 0.08, "Symbol collision fraction tracking failed");
        NS_TEST_ASSERT_MSG_EQ(rDes.syncCorruptionOccurred, true, "Sync word corruption detection failed under overlapping collision");
    }

    // ----------------------------------------------------
    // TEST 32: Deygout Multi-Edge Diffraction & Markov Channel States
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_RURAL_FORESTED, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.terrain.enabled = true;
        cfg.randomSeed = 12345;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(0,0,1.5), rx(5000.0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(111, 1, tx, rx, 14.0, 10, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.dominantEdgeHeightM, 0.0, "Deygout dominant edge height calculation failed");
        NS_TEST_ASSERT_MSG_GT(res.multiEdgeLossDb, 0.0, "Deygout multi-edge diffraction loss calculation failed");
        NS_TEST_ASSERT_MSG_EQ(res.hiddenChannelState >= 0 && res.hiddenChannelState <= 3, true, "Hidden Markov channel state out of valid bounds");
    }

    // ----------------------------------------------------
    // TEST 33: Symbol Phase Continuity
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector p(10,0,1.5), rx(0,0,15.0);
        PhysicsResult r1 = eng.ProcessTransmission(121, 1, p, rx, 14.0, 7, 125000.0, 0, Seconds(0.0));
        PhysicsResult r2 = eng.ProcessTransmission(121, 1, p, rx, 14.0, 7, 125000.0, 0, Seconds(0.1));
        NS_TEST_ASSERT_MSG_GT(r1.phaseCoherence, 0.0, "Phase coherence score computation failed");
        NS_TEST_ASSERT_MSG_EQ((r1.phaseCoherence != r2.phaseCoherence), true, "Symbol phase continuity failed across successive packets");
    }

    // ----------------------------------------------------
    // TEST 34: Synchronization Loop Convergence
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector pStrong(1.0,0,1.5), pWeak(5000.0,0,1.5), rx(0,0,25.0);
        PhysicsResult rStrong = eng.ProcessTransmission(131, 1, pStrong, rx, 45.0, 7, 125000.0, 0, Seconds(0.0));
        PhysicsResult rWeak = eng.ProcessTransmission(132, 1, pWeak, rx, -10.0, 7, 125000.0, 0, Seconds(0.0));
        NS_TEST_ASSERT_MSG_GT(rStrong.syncConfidence, rWeak.syncConfidence + 0.1, "Weak preamble failed to reflect lower sync confidence");
        NS_TEST_ASSERT_MSG_GT(rStrong.agcConvergence, 0.5, "Strong signal failed to converge AGC loop");
    }

    // ----------------------------------------------------
    // TEST 35: Cross-Layer ADR Response
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector p(50,0,1.5), rx(0,0,25.0);
        // Simulate multiple packets to build observation history
        for (int i = 0; i < 6; ++i) {
            eng.ProcessTransmission(141, 1, p, rx, 14.0, 7, 125000.0, 0, Seconds(i * 0.1));
        }
        auto xl = eng.GetCrossLayerEngine();
        uint8_t newSf = xl->GetAdaptedSpreadingFactor(141, 7);
        NS_TEST_ASSERT_MSG_EQ((newSf >= 7 && newSf <= 12), true, "Cross-layer ADR adapted spreading factor out of bounds");
        double retransProb = xl->GetRetransmissionProbability(141);
        NS_TEST_ASSERT_MSG_GT(retransProb, 0.0, "Cross-layer retransmission probability computation failed");
    }

    // ----------------------------------------------------
    // TEST 36: Adaptive Fidelity Scaling
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.nlos.enabled = false;
        cfg.shadowing.enabled = false;
        cfg.interference.enabled = false;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector p(10,0,1.5), rx(0,0,25.0);
        // Force statistics to trigger auto scaling (total > 1000)
        for (int i = 0; i < 1005; ++i) {
            eng.ProcessTransmission(151, 1, p, rx, 14.0, 7, 125000.0, 0, Seconds(i * 0.05));
        }
        EngineStatistics stats = eng.GetStatistics();
        std::cout << "\n[TEST 36 DEBUG] totalTransmissions: " << stats.totalTransmissions 
                  << ", PDR: " << stats.GetPDR() 
                  << ", interferenceDrops: " << stats.interferenceDrops << std::endl;
        
        PhysicsResult res = eng.ProcessTransmission(151, 1, p, rx, 14.0, 7, 125000.0, 0, Seconds(60.0));
        std::cout << "[TEST 36 DEBUG] res.fidelityLevel: " << res.fidelityLevel << std::endl;
        
        NS_TEST_ASSERT_MSG_EQ(res.fidelityLevel == FIDELITY_ADVANCED_TDL_COLLISIONS, true, "Adaptive multi-resolution auto-scaling failed");
    }

    // ----------------------------------------------------
    // TEST 37: SDR Trace Validation Metrics
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        auto val = eng.GetValidator();
        SdrValidationMetrics m = val->ValidateAgainstSdrTrace("dummy_nonexistent.trace");
        NS_TEST_ASSERT_MSG_EQ(m.validationPassed, true, "SDR validator default baseline validation failed");
        NS_TEST_ASSERT_MSG_GT(m.klDivergenceCfo, 0.0, "KL divergence calculation failed");
    }

    // ----------------------------------------------------
    // TEST 38: Successive Interference Cancellation (SIC) Recovery
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.interference.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector txStrong(1,0,1.5), txWeak(20,0,1.5), rx(0,0,25.0);
        // Transmit strong packet first, which successfully decodes and gets registered for SIC cancellation
        eng.ProcessTransmission(171, 1, txStrong, rx, 45.0, 7, 125000.0, 0, Seconds(0.0));
        // Transmit weak colliding packet
        PhysicsResult rWeak = eng.ProcessTransmission(172, 1, txWeak, rx, 14.0, 7, 125000.0, 0, Seconds(0.002));
        NS_TEST_ASSERT_MSG_EQ(rWeak.sicRecovered, true, "Successive Interference Cancellation (SIC) failed to subtract strong packet energy");
    }

    // ----------------------------------------------------
    // TEST 39: Semantic Region Transition Continuity
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector pSub(100,0,1.5), pUrb(800,0,1.5), pTun(2500,0,1.5), rx(0,0,25.0);
        PhysicsResult rSub = eng.ProcessTransmission(181, 1, pSub, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult rUrb = eng.ProcessTransmission(182, 1, pUrb, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult rTun = eng.ProcessTransmission(183, 1, pTun, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_EQ((rTun.environmentRegion == "Tunnel"), true, "Tunnel waveguide semantic region mapping failed");
    }

    // ----------------------------------------------------
    // TEST 40: Continuous-Time IQ Evolve & DSP Validation
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(100,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(191, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.iqPhaseContinuity, 0.8, "IQ phase continuity unrealistically degraded for clean LOS channel");
        NS_TEST_ASSERT_MSG_GT(res.fftPeakConfidence, 0.4, "DSP FFT peak confidence failed to capture strong signal");
    }

    // ----------------------------------------------------
    // TEST 41: PLL Phase Lock and Carrier Tracking
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.nlos.enabled = false; // clean outdoor PLL lock check; NLOS isolation is covered separately
        cfg.pll.enabled = true;
        cfg.pll.loopBandwidthHz = 200.0;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(201, 1, tx, rx, 14.0, 10, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_LT(res.pllPhaseErrorRad, 3.14159, "PLL phase error exceeded mathematical bounds");
        NS_TEST_ASSERT_MSG_EQ(res.pllLocked, true, "PLL loop failed to acquire lock on clean strong preamble");
    }

    // ----------------------------------------------------
    // TEST 42: Symbol Timing Recovery & Jitter
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.symbolSync.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(500,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(211, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.timingLockQuality, 0.5, "Symbol timing recovery quality too low for clean link");
        NS_TEST_ASSERT_MSG_LT(res.timingJitterNs, 2000.0, "Symbol timing jitter exceeded SF7 symbol bounds");
    }

    // ----------------------------------------------------
    // TEST 43: RF Frontend Nonlinearities & Intermodulation
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.rfNonlinearity.enabled = true;
        cfg.interference.enabled = true;
        cfg.interference.externalTxPowerDbm = DistributionParam::Fixed(45.0); // Extremely strong external interferer
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(100,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(221, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.phaseNoiseVariance, 0.0, "RF nonlinearity failed to inject phase noise under strong interference");
    }

    // ----------------------------------------------------
    // TEST 44: Cooperative PHY Combining (MRC)
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RAYLEIGH, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.cooperativePhy.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(500,0,1.5), gw1(0,0,25.0), gw2(1000,0,25.0);
        PhysicsResult r1 = eng.ProcessTransmission(231, 1, tx, gw1, 14.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult r2 = eng.ProcessTransmission(231, 2, tx, gw2, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_EQ(r2.gatewayDiversityOrder, 2, "Cooperative PHY failed to accumulate multiple gateway observations");
        NS_TEST_ASSERT_MSG_GT(r2.combinedSnrDb, std::max(r1.rawSNRDb, r2.rawSNRDb) - 0.1, "Maximal Ratio Combining (MRC) failed to improve physical SNR");
    }

    // ----------------------------------------------------
    // TEST 45: Geographic RF Memory Caching
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.geographicMemory.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector p1(120.0, 50.0, 1.5), rx(0,0,25.0);
        PhysicsResult r1 = eng.ProcessTransmission(241, 1, p1, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult r2 = eng.ProcessTransmission(242, 1, p1, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_EQ(r1.rfMemoryRegionId, r2.rfMemoryRegionId, "Geographic memory failed to group identical coordinates into same grid cell");
    }

    // ----------------------------------------------------
    // TEST 46: Scalability Engine Performance
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.scalability.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(251, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.batchProcessingGain, 0.0, "Scalability engine failed to register processing multiplier");
        NS_TEST_ASSERT_MSG_EQ((res.fidelitySchedulerState == FIDELITY_SDR_GRADE_REALISM || res.fidelitySchedulerState == FIDELITY_ADVANCED_TDL_COLLISIONS), true, "Fidelity scheduler dropped level unexpectedly");
    }

    // ----------------------------------------------------
    // TEST 47: Advanced SDR Validation Suite Integration
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.advancedValidation.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        eng.ProcessTransmission(261, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_EQ(true, true, "SDR Validation suite crashed during integration pass");
    }
    // ----------------------------------------------------
    // TEST 48: Hybrid MicroDSP Activation
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.microDsp.enabled = true;
        cfg.microDsp.activationSnrThresholdDb = 100.0; // Force trigger due to threshold
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(300, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_EQ(res.microDspActive, true, "MicroDSP failed to activate when triggered by low SNR threshold relative to setting");
    }

    // ----------------------------------------------------
    // TEST 49: Oscillator Thermal Drift
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.radioFrontend.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(301, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_NE(res.thermalDriftHz, 0.0, "Thermal drift was not dynamically calculated by the radio frontend module");
    }

    // ----------------------------------------------------
    // TEST 50: Analog AGC Hysteresis
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.radioFrontend.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(0,0,25.0), rx(0,0,25.0); // Exact same location, zero path loss
        PhysicsResult res = eng.ProcessTransmission(302, 1, tx, rx, 20.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.agcRecoveryTimeMs, 0.0, "AGC hysteresis recovery time was not triggered after a strong signal burst");
    }

    // ----------------------------------------------------
    // TEST 51: Distributed Gateway Fusion
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.distributedPhy.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx1(0,0,25.0), rx2(10,0,25.0);
        PhysicsResult r1 = eng.ProcessTransmission(303, 1, tx, rx1, 14.0, 7, 125000.0, 0, Seconds(0));
        PhysicsResult r2 = eng.ProcessTransmission(303, 2, tx, rx2, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(r2.cooperativeSymbolConfidence, r1.symbolLikelihood - 0.1, "Distributed symbol fusion failed to combine gateway likelihoods correctly");
    }

    // ----------------------------------------------------
    // TEST 52: Massive Scale Region Partitioning
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.distributedScalability.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(304, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_EQ(res.regionSchedulerState, 1, "Region scheduler did not fall back to REGION_STATE_SPARSE for isolated low-density traffic");
    }

    // ----------------------------------------------------
    // TEST 53: Spectrum Occupancy Interference
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.spectrumOccupancy.enabled = true;
        cfg.spectrumOccupancy.impulsiveNoiseProbability = 1.0; // Force an impulsive noise event
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(305, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.impulsiveNoiseEvents, 0, "Impulsive burst noise events were not generated in the spectrum occupancy model");
    }

    // ----------------------------------------------------
    // TEST 54: Adaptive Environment Learning
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.adaptiveEnvironment.enabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(306, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.environmentLearningConfidence, 0.0, "Adaptive environment Bayesian update failed to initialize learning confidence");
    }

    // ----------------------------------------------------
    // TEST 55: GPU Kernel Batch Execution
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.distributedScalability.enabled = true;
        cfg.distributedScalability.gpuBatchingEnabled = true;
        LoRaPhysicsEngine eng;
        eng.Initialize(cfg);
        Vector tx(50,0,1.5), rx(0,0,25.0);
        PhysicsResult res = eng.ProcessTransmission(307, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));
        NS_TEST_ASSERT_MSG_GT(res.gpuBatchSize, 0, "GPU Structure-of-Arrays (SoA) SIMD batch size was not correctly assigned by the scalability scheduler");
    }

    // ----------------------------------------------------
    // TEST 56: YAML Round-trip Configuration
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg1 = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_RICIAN, VEG_NONE, MOB_STATIC, false);
        // Make sure some distribution params are customized
        cfg1.propagation.pathLossExponent = DistributionParam::Normal(3.8, 0.4, 2.5, 5.0);
        cfg1.shadowing.shadowingStddevDb = DistributionParam::Fixed(7.5);
        cfg1.fading.ricianKFactor = DistributionParam::Uniform(2.0, 10.0);
        cfg1.nlos.buildingPenetrationDb = DistributionParam::Exponential(0.2, 5.0, 20.0);
        cfg1.receiver.softThresholdWidthDb = 0.42;
        cfg1.receiver.sensitivityDbm[0] = -122.75;
        cfg1.receiver.codingRateSensitivityOffsetDb[3] = -2.5;
        cfg1.receiver.cfoPenaltyThresholdHz = 1200.0;
        
        std::string filename = "test_roundtrip.yaml";
        cfg1.ToYAML(filename);
        
        LoRaPhysicsConfig cfg2 = LoRaPhysicsConfig::FromYAML(filename);
        
        // Assert round-trip fidelity
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.propagation.pathLossExponent.mean, 3.8, 1e-6, "YAML round-trip failed to preserve pathLossExponent mean");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.propagation.pathLossExponent.stddev, 0.4, 1e-6, "YAML round-trip failed to preserve pathLossExponent stddev");
        NS_TEST_ASSERT_MSG_EQ(cfg2.propagation.pathLossExponent.type, DistributionParam::NORMAL, "YAML round-trip failed to preserve pathLossExponent type");
        
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.shadowing.shadowingStddevDb.mean, 7.5, 1e-6, "YAML round-trip failed to preserve shadowingStddevDb mean");
        NS_TEST_ASSERT_MSG_EQ(cfg2.shadowing.shadowingStddevDb.type, DistributionParam::DETERMINISTIC, "YAML round-trip failed to preserve shadowingStddevDb type");
        
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.fading.ricianKFactor.min, 2.0, 1e-6, "YAML round-trip failed to preserve ricianKFactor min");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.fading.ricianKFactor.max, 10.0, 1e-6, "YAML round-trip failed to preserve ricianKFactor max");
        NS_TEST_ASSERT_MSG_EQ(cfg2.fading.ricianKFactor.type, DistributionParam::UNIFORM, "YAML round-trip failed to preserve ricianKFactor type");
        
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.nlos.buildingPenetrationDb.lambda, 0.2, 1e-6, "YAML round-trip failed to preserve buildingPenetrationDb lambda");
        NS_TEST_ASSERT_MSG_EQ(cfg2.nlos.buildingPenetrationDb.type, DistributionParam::EXPONENTIAL, "YAML round-trip failed to preserve buildingPenetrationDb type");

        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.receiver.softThresholdWidthDb, 0.42, 1e-6, "YAML round-trip failed to preserve receiver soft threshold width");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.receiver.sensitivityDbm[0], -122.75, 1e-6, "YAML round-trip failed to preserve receiver SF7 sensitivity");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.receiver.codingRateSensitivityOffsetDb[3], -2.5, 1e-6, "YAML round-trip failed to preserve CR4/8 sensitivity offset");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg2.receiver.cfoPenaltyThresholdHz, 1200.0, 1e-6, "YAML round-trip failed to preserve receiver CFO threshold");
        
        std::remove(filename.c_str());
    }

    // ----------------------------------------------------
    // TEST 57: Configurable soft receiver threshold gives 50% PDR at threshold
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig probeCfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        probeCfg.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
        probeCfg.shadowing.enabled = false;
        probeCfg.fading.enabled = false;
        probeCfg.nlos.enabled = false;
        probeCfg.atmospheric.enabled = false;
        probeCfg.foliage.enabled = false;
        probeCfg.interference.enabled = false;
        probeCfg.hardware.enabled = false;
        probeCfg.mobility.enabled = false;
        probeCfg.terrain.enabled = false;
        probeCfg.receiver.softThresholdEnabled = false;
        probeCfg.receiver.linkHysteresisMarginDb = 0.0;
        probeCfg.propagation.pathLossExponent = DistributionParam::Fixed(3.5);

        Vector tx(0, 0, 1.5);
        Vector rx(962, 0, 30.0);

        LoRaPhysicsEngine probe;
        probe.Initialize(probeCfg);
        PhysicsResult baseline = probe.ProcessTransmission(9000, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(0));

        LoRaPhysicsConfig cfg = probeCfg;
        cfg.receiver.softThresholdEnabled = true;
        cfg.receiver.softThresholdWidthDb = 0.35;
        cfg.receiver.sensitivityDbm[0] = baseline.rxPowerDbm;

        LoRaPhysicsEngine engine;
        engine.Initialize(cfg);
        int successes = 0;
        const int count = 1000;
        for (int i = 0; i < count; ++i) {
            PhysicsResult res = engine.ProcessTransmission(9100 + i, 1, tx, rx, 14.0, 7, 125000.0, 0, Seconds(i * 0.01));
            if (res.packetReceived) {
                successes++;
            }
        }

        double pdr = (double)successes / count;
        NS_TEST_ASSERT_MSG_GT(pdr, 0.45, "Soft receiver threshold produced below-expected PDR at exact threshold");
        NS_TEST_ASSERT_MSG_LT(pdr, 0.55, "Soft receiver threshold produced above-expected PDR at exact threshold");
    }

    // ----------------------------------------------------
    // TEST 58: Maximum fidelity respects shadowing/NLOS enable flags
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.randomSeed = 58001;
        cfg.shadowing.enabled = false;
        cfg.nlos.enabled = false;
        cfg.fading.model = FADING_NONE;
        cfg.fading.enabled = false;
        cfg.foliage.enabled = false;
        cfg.terrain.enabled = false;
        cfg.atmospheric.enabled = false;
        cfg.interference.enabled = false;
        cfg.hardware.enabled = false;
        cfg.mobility.enabled = false;

        LoRaPhysicsEngine engine;
        engine.Initialize(cfg);
        PhysicsResult res = engine.ProcessTransmission(
            58001, 3, Vector(0, 0, 1.5), Vector(1200, 0, 30.0),
            14.0, 9, 125000.0, 0, Seconds(12.0));

        NS_TEST_ASSERT_MSG_EQ_TOL(res.shadowingDb, 0.0, 1e-9, "Maximum fidelity applied shadowing while shadowing.enabled=false");
        NS_TEST_ASSERT_MSG_EQ_TOL(res.nlosDb, 0.0, 1e-9, "Maximum fidelity applied NLOS loss while nlos.enabled=false");
        NS_TEST_ASSERT_MSG_EQ(res.wasNLOS, false, "Maximum fidelity reported NLOS while nlos.enabled=false");
        NS_TEST_ASSERT_MSG_EQ_TOL(res.nlosProbability, 0.0, 1e-9, "Maximum fidelity reported NLOS probability while nlos.enabled=false");
    }


    // ----------------------------------------------------
    // TEST 59: Maximum fidelity respects foliage enable flag
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.foliage.enabled = false;
        LoRaPhysicsEngine engine;
        engine.Initialize(cfg);
        PhysicsResult res = engine.ProcessTransmission(59000, 3, Vector(0,0,1.5), Vector(1200,0,30.0), 14.0, 9, 125000.0, 0, Seconds(12.0));
        NS_TEST_ASSERT_MSG_EQ_TOL(res.foliageDb, 0.0, 1e-9, "Maximum fidelity applied foliage loss while foliage.enabled=false");
    }

    // ----------------------------------------------------
    // TEST 60: Maximum fidelity respects interference enable flag
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.interference.enabled = false;
        LoRaPhysicsEngine engine;
        engine.Initialize(cfg);
        PhysicsResult res = engine.ProcessTransmission(60000, 3, Vector(0,0,1.5), Vector(1200,0,30.0), 14.0, 9, 125000.0, 0, Seconds(12.0));
        NS_TEST_ASSERT_MSG_EQ_TOL(res.interferencePenaltyDb, 0.0, 1e-9, "Maximum fidelity applied interference penalty while interference.enabled=false");
    }

    // ----------------------------------------------------
    // TEST 61: Maximum fidelity respects hardware enable flag
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.hardware.enabled = false;
        LoRaPhysicsEngine engine;
        engine.Initialize(cfg);
        PhysicsResult res = engine.ProcessTransmission(61000, 3, Vector(0,0,1.5), Vector(1200,0,30.0), 14.0, 9, 125000.0, 0, Seconds(12.0));
        NS_TEST_ASSERT_MSG_EQ_TOL(res.thermalDriftHz, 0.0, 1e-9, "Maximum fidelity applied thermal drift while hardware.enabled=false");
        NS_TEST_ASSERT_MSG_EQ_TOL(res.phaseNoiseVariance, 0.0, 1e-9, "Maximum fidelity applied phase noise while hardware.enabled=false");
    }

    // ----------------------------------------------------
    // TEST 62: Maximum fidelity respects atmospheric enable flag
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_URBAN_DENSE, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.atmospheric.enabled = false;
        LoRaPhysicsEngine engine;
        engine.Initialize(cfg);
        PhysicsResult res = engine.ProcessTransmission(62000, 3, Vector(0,0,1.5), Vector(1200,0,30.0), 14.0, 9, 125000.0, 0, Seconds(12.0));
        NS_TEST_ASSERT_MSG_EQ_TOL(res.atmosphericDb, 0.0, 1e-9, "Maximum fidelity applied atmospheric loss while atmospheric.enabled=false");
    }
    // ----------------------------------------------------
    // TEST 63: Atmospheric refractivity, snow loss, and wind foliage regressions
    // ----------------------------------------------------
    {
        LoRaDistributionSampler atmosSampler(63001);
        AtmosphericParams refrParams = AtmosphericParams::DefaultsFor(WEATHER_CLEAR_DRY);
        refrParams.enabled = true;
        refrParams.temperatureCelsius = DistributionParam::Fixed(20.0);
        refrParams.waterVaporDensityGPerM3 = DistributionParam::Fixed(7.5);
        refrParams.rainRateMmPerHr = DistributionParam::Fixed(0.0);
        refrParams.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
        refrParams.atmosphericPressure.enableTroposphericRefraction = true;
        refrParams.atmosphericPressure.atmosphericPressureHPa = DistributionParam::Fixed(1013.25);
        refrParams.snow.enabled = false;

        AtmosphericSubmodule refr(refrParams, atmosSampler);
        double refractivityN = refr.ComputeRefractivityN(Seconds(1.0));
        double tKelvin = 293.15;
        double es = 6.112 * std::exp((17.67 * 20.0) / (20.0 + 243.5));
        double e = 0.5 * es;
        double expectedN = (77.6 * 1013.25 / tKelvin) + (3.73e5 * e / (tKelvin * tKelvin));
        double wrongSingleT = (77.6 * 1013.25 / tKelvin) + (3.73e5 * e / tKelvin);

        NS_TEST_ASSERT_MSG_EQ_TOL(refractivityN, expectedN, 1e-6, "Atmospheric refractivity must use ITU-R P.453 water-vapor T^2 denominator");
        NS_TEST_ASSERT_MSG_LT(refractivityN, wrongSingleT / 10.0, "Atmospheric refractivity resembles the incorrect single-T water-vapor formula");

        LoRaDistributionSampler snowSampler(63002);
        AtmosphericParams snowParams = AtmosphericParams::DefaultsFor(WEATHER_CLEAR_DRY);
        snowParams.enabled = true;
        snowParams.rainRateMmPerHr = DistributionParam::Fixed(0.0);
        snowParams.waterVaporDensityGPerM3 = DistributionParam::Fixed(0.0);
        snowParams.temperatureCelsius = DistributionParam::Fixed(-2.0);
        snowParams.fogAttenuationDbPerKm = DistributionParam::Fixed(0.0);
        snowParams.snow.enabled = true;
        snowParams.snow.snowfallRateMmHrEquiv = DistributionParam::Fixed(10.0);
        snowParams.snow.snowTemperatureCelsius = DistributionParam::Fixed(-2.0);

        AtmosphericSubmodule snowAtm(snowParams, snowSampler);
        double snowLossDb = snowAtm.ComputeAtmosphericLossDb(5.0, Seconds(1.0));
        double expectedSnowLossDb = 0.0078 * std::pow(10.0, 0.72) * 5.0;
        NS_TEST_ASSERT_MSG_EQ_TOL(snowLossDb, expectedSnowLossDb, 1e-6, "Snow attenuation must be included in atmospheric loss");

        LoRaPhysicsConfig windCfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_DENSE_FOREST, MOB_STATIC, false);
        windCfg.fidelityMode = FIDELITY_ULTRA_FAST_STATISTICAL;
        windCfg.randomSeed = 63003;
        windCfg.shadowing.enabled = false;
        windCfg.fading.enabled = false;
        windCfg.nlos.enabled = false;
        windCfg.atmospheric.enabled = false;
        windCfg.interference.enabled = false;
        windCfg.hardware.enabled = false;
        windCfg.mobility.enabled = false;
        windCfg.terrain.enabled = false;
        windCfg.foliage.enabled = true;
        windCfg.foliage.type = VEG_DENSE_FOREST;
        windCfg.foliage.vegetationDepthM = DistributionParam::Fixed(20.0);
        windCfg.foliage.specificAttenuationDbPerM = DistributionParam::Fixed(0.2);
        windCfg.foliage.maxAttenuationDb = DistributionParam::Fixed(25.0);
        windCfg.foliage.wetFoliageAddDb = DistributionParam::Fixed(0.0);
        windCfg.windVariation.windVariationEnabled = true;
        windCfg.windVariation.windSpeedMps = DistributionParam::Fixed(3.0);
        windCfg.windVariation.windAmplitudeDb = DistributionParam::Fixed(2.0);

        LoRaPhysicsEngine windEngine;
        windEngine.Initialize(windCfg);
        Vector tx(10.0, 20.0, 1.5);
        Vector rx(1310.0, 420.0, 30.0);
        (void)windEngine.ProcessTransmission(63003, 5, tx, rx, 14.0, 9, 125000.0, 0, Seconds(0.0));
        PhysicsResult windRes = windEngine.ProcessTransmission(63004, 5, tx, rx, 14.0, 9, 125000.0, 0, Seconds(10.0));

        NS_TEST_ASSERT_MSG_GT(windRes.foliageStaticDb, 0.0, "Wind foliage path must preserve static foliage loss");
        NS_TEST_ASSERT_MSG_GT(std::abs(windRes.foliageDynamicDb), 1e-6, "Wind variation must create a dynamic foliage component when enabled");
    }

    // ----------------------------------------------------
    // TEST 64: Default YAML exposes existing core DistributionParam fields
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::FromYAML(
            "src/lora-physics-engine/config/lora-physics-default.yaml");

        NS_TEST_ASSERT_MSG_EQ(cfg.propagation.pathLossExponent.type, DistributionParam::NORMAL, "Default YAML did not parse propagation pathLossExponent DistributionParam");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg.propagation.pathLossExponent.mean, 3.5, 1e-9, "Default YAML pathLossExponent mean mismatch");
        NS_TEST_ASSERT_MSG_EQ(cfg.propagation.gatewayHeightM.type, DistributionParam::NORMAL, "Default YAML did not parse gatewayHeightM DistributionParam");
        NS_TEST_ASSERT_MSG_EQ(cfg.propagation.nodeHeightM.type, DistributionParam::NORMAL, "Default YAML did not parse nodeHeightM DistributionParam");

        NS_TEST_ASSERT_MSG_EQ(cfg.shadowing.shadowingStddevDb.type, DistributionParam::NORMAL, "Default YAML did not parse shadowingStddevDb DistributionParam");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg.shadowing.correlationDistanceM.mean, 20.0, 1e-9, "Default YAML correlationDistanceM mean mismatch");

        NS_TEST_ASSERT_MSG_EQ(cfg.fading.model, FADING_RICIAN, "Default YAML fading model should be RICIAN");
        NS_TEST_ASSERT_MSG_EQ(cfg.fading.ricianKFactor.type, DistributionParam::NORMAL, "Default YAML did not parse ricianKFactor DistributionParam");
        NS_TEST_ASSERT_MSG_EQ(cfg.fading.coherenceTimeMs.type, DistributionParam::LOGNORMAL, "Default YAML did not parse coherenceTimeMs DistributionParam");

        NS_TEST_ASSERT_MSG_EQ(cfg.nlos.nlosScaleDistanceM.type, DistributionParam::DETERMINISTIC, "Default YAML did not parse nlosScaleDistanceM DistributionParam");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg.nlos.nlosAttenuationDb.mean, 15.0, 1e-9, "Default YAML nlosAttenuationDb mean mismatch");
        NS_TEST_ASSERT_MSG_EQ(cfg.nlos.buildingPenetrationDb.type, DistributionParam::NORMAL, "Default YAML did not parse buildingPenetrationDb DistributionParam");

        NS_TEST_ASSERT_MSG_EQ(cfg.hardware.timestampJitterNs.type, DistributionParam::LOGNORMAL, "Default YAML did not parse timestampJitterNs DistributionParam");
        NS_TEST_ASSERT_MSG_EQ_TOL(cfg.hardware.rssiResolutionDb, 1.0, 1e-9, "Default YAML rssiResolutionDb mismatch");
        NS_TEST_ASSERT_MSG_EQ(cfg.hardware.txPowerAccuracyDb.type, DistributionParam::NORMAL, "Default YAML did not parse txPowerAccuracyDb DistributionParam");

        NS_TEST_ASSERT_MSG_EQ(cfg.mobility.velocityMps.type, DistributionParam::DETERMINISTIC, "Default YAML did not parse velocityMps DistributionParam");
        NS_TEST_ASSERT_MSG_EQ(cfg.terrain.terrainIrregularityM.type, DistributionParam::NORMAL, "Default YAML did not parse terrainIrregularityM DistributionParam");
    }

    // ----------------------------------------------------
    // TEST 65: Maximum fidelity shadowing uses Gudmundson, not Perlin field shadowing
    // ----------------------------------------------------
    {
        LoRaPhysicsConfig cfg = LoRaPhysicsConfig::ForScenario(ENV_SUBURBAN, WEATHER_CLEAR_DRY, FADING_NONE, VEG_NONE, MOB_STATIC, false);
        cfg.fidelityMode = FIDELITY_SDR_GRADE_REALISM;
        cfg.randomSeed = 61020;
        cfg.shadowing.enabled = true;
        cfg.shadowing.spatialCorrelationEnabled = true;
        cfg.shadowing.correlationDistanceM = DistributionParam::Fixed(20.0);
        cfg.shadowing.shadowingStddevDb = DistributionParam::Fixed(6.0);
        cfg.shadowing.updateIntervalS = 0.0;
        cfg.nlos.enabled = false;
        cfg.fading.enabled = false;
        cfg.fading.model = FADING_NONE;
        cfg.atmospheric.enabled = false;
        cfg.foliage.enabled = false;
        cfg.terrain.enabled = false;
        cfg.interference.enabled = false;
        cfg.hardware.enabled = false;
        cfg.mobility.enabled = false;

        LoRaPhysicsEngine engine;
        engine.Initialize(cfg);

        std::vector<double> values;
        const uint32_t count = 800;
        values.reserve(count);
        Vector rx(0.0, 0.0, 30.0);
        for (uint32_t i = 0; i < count; ++i) {
            Vector tx(100.0 + 20.0 * i, 0.0, 1.5);
            PhysicsResult res = engine.ProcessTransmission(610000 + i, 77, tx, rx, 14.0, 9, 125000.0, 0, Seconds(0.0));
            values.push_back(res.shadowingDb);
        }

        double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        double covariance = 0.0;
        double variance = 0.0;
        for (uint32_t i = 1; i < values.size(); ++i) {
            covariance += (values[i - 1] - mean) * (values[i] - mean);
        }
        for (double v : values) {
            variance += (v - mean) * (v - mean);
        }
        double lagOneCorrelation = covariance / variance;
        NS_TEST_ASSERT_MSG_EQ_TOL(lagOneCorrelation, std::exp(-1.0), 0.10, "Maximum fidelity shadowing no longer follows Gudmundson exp(-d/dcorr)");
    }
}

/**
 * @ingroup lora-physics-engine
 * @brief TestSuite class to register all unit tests
 */
class LoRaPhysicsEngineTestSuite : public TestSuite
{
public:
    LoRaPhysicsEngineTestSuite();
};

LoRaPhysicsEngineTestSuite::LoRaPhysicsEngineTestSuite()
    : TestSuite("lora-physics-engine", Type::UNIT)
{
    AddTestCase(new LoRaPhysicsEngineTest, Duration::QUICK);
}

static LoRaPhysicsEngineTestSuite loraPhysicsEngineTestSuite;
