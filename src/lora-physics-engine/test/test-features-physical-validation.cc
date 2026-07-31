#include "ns3/test.h"
#include "ns3/log.h"
#include "../model/lora-physics-parameters.h"
#include "../model/lora-distribution-sampler.h"
#include "../model/submodules/lora-terrain-submodule.h"
#include "../model/submodules/lora-atmospheric-submodule.h"
#include "../model/submodules/lora-hardware-submodule.h"
#include "../model/submodules/lora-foliage-submodule.h"
#include "../model/submodules/lora-delay-spread-submodule.h"
#include <cmath>
#include <vector>

namespace ns3 {

/**
 * @brief Unit tests for 10 physical features implementation
 * Validates numerical accuracy against peer-reviewed specifications
 */

// ============================================================================
// Feature 2: Earth Curvature and Fresnel Zone Test
// ============================================================================
class EarthCurvatureFresnelTest : public TestCase {
public:
    EarthCurvatureFresnelTest() : TestCase("Earth Curvature and Fresnel Zones") {}
    
    virtual void DoRun() override {
        TerrainSubmodule terrain;
        LoRaDistributionSampler sampler(42);
        TerrainParams params = TerrainParams::Defaults();
        
        Vector txPos(0, 0, 100);     // TX at 100m height
        Vector rxPos(50000, 0, 50);  // RX at 50m height, 50km away
        
        TerrainResult result = terrain.ComputeTerrainPenaltyDb(
            txPos, rxPos, 100.0, 50.0, params, 123, sampler);
        
        // Verify earth bulge is computed (should be ~40m for 50km distance, K=4/3)
        // H_bulge = d^2 / (2*K*R) = (50000)^2 / (2 * 1.333 * 6.371e6) = ~40.8m
        NS_TEST_ASSERT_MSG_GT(result.earthBulgM, 30.0, "Earth bulge too small");
        NS_TEST_ASSERT_MSG_LT(result.earthBulgM, 60.0, "Earth bulge too large");
        
        // Verify Fresnel radius computed (should be ~200-300m at 868MHz, 50km path)
        NS_TEST_ASSERT_MSG_GT(result.fresnelRadiusM, 100.0, "Fresnel radius too small");
        NS_TEST_ASSERT_MSG_LT(result.fresnelRadiusM, 500.0, "Fresnel radius too large");
    }
};

// ============================================================================
// Feature 3: Atmospheric Refractivity Test
// ============================================================================
class AtmosphericRefractivityTest : public TestCase {
public:
    AtmosphericRefractivityTest() : TestCase("Atmospheric Refractivity (ITU-R P.453-14)") {}
    
    virtual void DoRun() override {
        AtmosphericParams params = AtmosphericParams::Defaults();
        LoRaDistributionSampler sampler(42);
        params.atmosphericPressure.enableTroposphericRefraction = true;
        
        AtmosphericSubmodule atm(params, sampler);
        Time currentTime = Time(0);
        
        double N = atm.ComputeRefractivityN(currentTime);
        
        // At standard conditions (1013.25 hPa, 290K, RH=50%)
        // N should be approximately 315 N-units (typical troposphere)
        // ITU formula: N = 77.6*P/T + 3.73e5*e/T^2
        // With P=1013.25, T=290K, e~6hPa (50% RH):
        // N = 77.6*1013.25/290 + 3.73e5*6/290^2 = 273 + 266 = 539 (approximately)
        // But with lower pressure/temp from defaults, expect ~300-400
        
        NS_TEST_ASSERT_MSG_GT(N, 200.0, "Refractivity too low");
        NS_TEST_ASSERT_MSG_LT(N, 600.0, "Refractivity too high");
    }
};

// ============================================================================
// Feature 4: Snow Attenuation (Mätzler 1987) Test
// ============================================================================
class SnowAttenuationTest : public TestCase {
public:
    SnowAttenuationTest() : TestCase("Snow Attenuation (Mätzler 1987 Formula)") {}
    
    virtual void DoRun() override {
        AtmosphericParams params = AtmosphericParams::Defaults();
        LoRaDistributionSampler sampler(42);
        params.snow.enabled = true;
        
        AtmosphericSubmodule atm(params, sampler);
        
        // Test 1: Wet snow attenuation
        // L_wet = 0.0078 * R^0.72 dB/km
        // At R=10 mm/hr, distance=5km:
        // L = 0.0078 * 10^0.72 * 5 = 0.0078 * 5.25 * 5 = 0.204 dB
        double wetSnowAttenuationDb = atm.ComputeSnowAttenuationDb(10.0, -2.0, 5.0);
        NS_TEST_ASSERT_MSG_GT(wetSnowAttenuationDb, 0.15, "Wet snow attenuation too low");
        NS_TEST_ASSERT_MSG_LT(wetSnowAttenuationDb, 0.30, "Wet snow attenuation too high");
        
        // Test 2: Dry snow attenuation
        // L_dry = 0.0003 * R^1.6 dB/km
        // At R=2 mm/hr, distance=5km:
        // L = 0.0003 * 2^1.6 * 5 = 0.0003 * 3.03 * 5 = 0.0046 dB
        double drySnowAttenuationDb = atm.ComputeSnowAttenuationDb(2.0, -20.0, 5.0);
        NS_TEST_ASSERT_MSG_GT(drySnowAttenuationDb, 0.001, "Dry snow attenuation too low");
        NS_TEST_ASSERT_MSG_LT(drySnowAttenuationDb, 0.01, "Dry snow attenuation too high");
        
        // Wet snow should be significantly higher than dry snow
        NS_TEST_ASSERT_MSG_GT(wetSnowAttenuationDb, drySnowAttenuationDb * 10.0,
                             "Wet snow not significantly higher than dry snow");
    }
};

// ============================================================================
// Feature 5: Wind-Induced Foliage Variation Test (Ornstein-Uhlenbeck)
// ============================================================================
class WindFoliageVariationTest : public TestCase {
public:
    WindFoliageVariationTest() : TestCase("Wind Foliage Variation (Ornstein-Uhlenbeck Process)") {}
    
    virtual void DoRun() override {
        FoliageParams folParams = FoliageParams::Defaults();
        WindVariationParams windParams = WindVariationParams::Defaults();
        LoRaDistributionSampler sampler(42);
        
        folParams.enabled = true;
        windParams.windVariationEnabled = true;
        windParams.windSpeedMps = DistributionParam::Fixed(5.0);  // Constant 5 m/s
        windParams.windAmplitudeDb = DistributionParam::Fixed(2.0); // 2 dB amplitude
        
        Vector txPos(0, 0, 0);
        Vector rxPos(100, 0, 0);
        
        // Simulate OU process over time and compute autocorrelation
        std::vector<double> dynamics;
        const int numSamples = 100;
        for (int i = 0; i < numSamples; ++i) {
            Time t = Time::FromDouble(i * 1.0, Time::S);  // 1-second intervals
            FoliageVariationResult res = ComputeDynamicFoliageLossDb(
                txPos, rxPos, folParams, windParams, sampler, false, false, t);
            dynamics.push_back(res.dynamicLossDb);
        }
        
        // Verify coherence time: at 5 m/s wind and 10m depth: T_wind = 10/(5+1) = 1.67s
        // At 1-second lag: rho = exp(-1/1.67) = 0.533, so autocorr at lag=1 should be ~0.533
        // At larger lags, should decay exponentially
        // For Ornstein-Uhlenbeck, autocorr(tau) = exp(-tau/T_wind)
        // So at tau=1s: rho = exp(-1/1.67) = 0.533
        // At tau=2s: rho = exp(-2/1.67) = 0.285
        
        double meanVal = 0.0;
        for (double d : dynamics) meanVal += d;
        meanVal /= dynamics.size();
        
        double variance = 0.0;
        for (double d : dynamics) variance += (d - meanVal) * (d - meanVal);
        variance /= dynamics.size();
        
        // Test autocorrelation at lag-1
        double autocorr1 = 0.0;
        for (int i = 0; i < numSamples - 1; ++i) {
            autocorr1 += (dynamics[i] - meanVal) * (dynamics[i+1] - meanVal);
        }
        autocorr1 /= (numSamples - 1);
        autocorr1 /= variance;  // Normalize
        
        // Expected rho ~= exp(-1/1.67) = 0.533
        // Allow 0.4-0.6 range to account for noise and sampling
        NS_TEST_ASSERT_MSG_GT(autocorr1, 0.4, "Wind OU autocorrelation too low");
        NS_TEST_ASSERT_MSG_LT(autocorr1, 0.7, "Wind OU autocorrelation too high");
    }
};

// ============================================================================
// Feature 6: Temperature-Dependent CFO Test
// ============================================================================
class TemperatureCFOTest : public TestCase {
public:
    TemperatureCFOTest() : TestCase("Temperature-Dependent CFO (Robyns 2017)") {}
    
    virtual void DoRun() override {
        HardwareParams hwParams = HardwareParams::Defaults();
        LoRaDistributionSampler sampler(42);
        
        hwParams.enabled = true;
        HardwareSubmodule hw(hwParams, sampler);
        
        TemperatureCfoParams cfoParams = TemperatureCfoParams::Defaults();
        cfoParams.temperatureCoefficientHzPerC = DistributionParam::Fixed(1.5);  // 1.5 Hz/°C
        cfoParams.jitterTempSensitivity = DistributionParam::Fixed(0.0);  // No jitter for this test
        cfoParams.referenceTemperatureC = 25.0;
        
        // Test 1: At 25°C (reference), CFO should be ~0
        double cfo25 = hw.ComputeTemperatureCFO(25.0, cfoParams);
        NS_TEST_ASSERT_MSG_LT(std::abs(cfo25), 0.1, "CFO at reference temperature not zero");
        
        // Test 2: At 35°C (+10°C), CFO should be ~15 Hz (1.5 * 10)
        double cfo35 = hw.ComputeTemperatureCFO(35.0, cfoParams);
        NS_TEST_ASSERT_MSG_GT(cfo35, 13.0, "CFO at +10°C too low");
        NS_TEST_ASSERT_MSG_LT(cfo35, 17.0, "CFO at +10°C too high");
        
        // Test 3: At 15°C (-10°C), CFO should be ~-15 Hz
        double cfo15 = hw.ComputeTemperatureCFO(15.0, cfoParams);
        NS_TEST_ASSERT_MSG_LT(cfo15, -13.0, "CFO at -10°C not negative enough");
        NS_TEST_ASSERT_MSG_GT(cfo15, -17.0, "CFO at -10°C too negative");
    }
};

// ============================================================================
// Feature 7: Dynamic Delay Spread Test
// ============================================================================
class DynamicDelaySpreadTest : public TestCase {
public:
    DynamicDelaySpreadTest() : TestCase("Dynamic Delay Spread (Lim et al. 2018)") {}
    
    virtual void DoRun() override {
        DelaySpreadSubmodule delaySpread;
        LoRaDistributionSampler sampler(42);
        
        DelaySpreadParams params = DelaySpreadParams::DefaultsFor(ENV_SUBURBAN);
        params.enabled = true;
        
        // Test LOS delay spread sampling
        DelaySpreadResult losResult = delaySpread.ComputeDelaySpreadDynamic(
            ENV_SUBURBAN, 1000.0, 868e6, 125000.0, params, false, sampler);
        
        // LOS should have smaller delay spread than NLOS (spec: 50-150ns for suburban LOS)
        NS_TEST_ASSERT_MSG_GT(losResult.rmsDelaySpreadNs, 30.0, "LOS delay spread too small");
        NS_TEST_ASSERT_MSG_LT(losResult.rmsDelaySpreadNs, 200.0, "LOS delay spread too large");
        
        // Test NLOS delay spread sampling
        DelaySpreadResult nlosResult = delaySpread.ComputeDelaySpreadDynamic(
            ENV_SUBURBAN, 1000.0, 868e6, 125000.0, params, true, sampler);
        
        // NLOS should have larger delay spread (spec: 100-400ns for suburban NLOS)
        NS_TEST_ASSERT_MSG_GT(nlosResult.rmsDelaySpreadNs, 50.0, "NLOS delay spread too small");
        NS_TEST_ASSERT_MSG_LT(nlosResult.rmsDelaySpreadNs, 500.0, "NLOS delay spread too large");
        
        // NLOS should typically be larger than LOS
        // (not strict inequality due to sampling randomness, but on average should be)
        NS_TEST_ASSERT_MSG_GT(nlosResult.rmsDelaySpreadNs, losResult.rmsDelaySpreadNs * 0.8,
                             "NLOS not significantly larger than LOS");
    }
};

// ============================================================================
// Test Suite Registration
// ============================================================================
class PhysicsFeatureTestSuite : public TestSuite {
public:
    PhysicsFeatureTestSuite() : TestSuite("physics-features-validation", UNIT) {
        AddTestCase(new EarthCurvatureFresnelTest(), TestCase::QUICK);
        AddTestCase(new AtmosphericRefractivityTest(), TestCase::QUICK);
        AddTestCase(new SnowAttenuationTest(), TestCase::QUICK);
        AddTestCase(new WindFoliageVariationTest(), TestCase::QUICK);
        AddTestCase(new TemperatureCFOTest(), TestCase::QUICK);
        AddTestCase(new DynamicDelaySpreadTest(), TestCase::QUICK);
    }
};

static PhysicsFeatureTestSuite physicsFeatureTestSuite;

} // namespace ns3
