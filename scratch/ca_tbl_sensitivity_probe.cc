/**
 * \file ca_tbl_sensitivity_probe.cc
 * \brief G93 (A7 / D-5): print the receiver sensitivity each engine ACTUALLY applies.
 *
 * The six-decibel defect (D1, fixed at 0d5e747) was not in a table -- the table was
 * right -- but in the accessor that indexed it (`sf - 7` against an SF5-based table). It
 * passed every gate and verifier check in the project. So this program does not re-derive
 * anything: it builds each engine exactly as ca_tbl_causal_sim.cc's CA_TBL_MAKE_CONFIGS
 * does and calls the engine's own GetSensitivityThreshold() for every spreading factor and
 * coding rate, with the coding rate the engine is configured to use marked.
 *
 * Whether those numbers are RIGHT is decided by scripts/sensitivity_fidelity_check.py,
 * against data-sheet tables transcribed there with their table numbers -- a reference that
 * shares no code with the engine, which is the property the D1 lesson asks for.
 *
 * Output: CSV on stdout,  tier,sf,crDenom,configuredCr,thresholdDbm
 */

#include "ns3/core-module.h"
#include "ns3/lora-2p4-physics-engine.h"
#include "ns3/lora-2p4-physics-parameters.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-uwb-physics-engine.h"
#include "ns3/lora-uwb-physics-parameters.h"

#include <string>

#include <cmath>
#include <cstdio>

using namespace ns3;

int
main(int argc, char* argv[])
{
    // The scenario's defaults: ca_tbl_causal_sim.cc, envInt = 1 (RURAL_FORESTED, the wide
    // area, tier 1), envFacilityInt = 0 (RURAL_OPEN, tiers 2 and 3), DENSE_FOREST and
    // SPARSE_SHRUBS. Sensitivity tables do not depend on environment; matching the scenario
    // anyway means a future dependence cannot hide here.
    uint32_t envWide = 1;
    uint32_t envFacility = 0;
    uint32_t vegWide = static_cast<uint32_t>(VEG_DENSE_FOREST);
    uint32_t vegFacility = static_cast<uint32_t>(VEG_SPARSE_SHRUBS);
    CommandLine cmd(__FILE__);
    cmd.AddValue("env", "wide-area environment, as ca_tbl_causal_sim --env", envWide);
    cmd.AddValue("envFacility", "facility environment, as --envFacility", envFacility);
    double t2Row = 0.0;
    cmd.AddValue("t2SensitivityRowKHz",
                 "Apply the scenario's D-8 correction, exactly as --t2SensitivityRowKHz does, "
                 "so the corrected configuration can be checked by the same gate",
                 t2Row);
    double bw2p4 = 0.0;
    cmd.AddValue("bw2p4",
                 "SY-3: tier-2 bandwidth in Hz, applied exactly as ca_tbl_causal_sim --bw2p4 "
                 "does -- a 10*log10(BW/800 kHz) shift of the table, unless "
                 "--t2SensitivityRowKHz replaces the table, in which case no shift",
                 bw2p4);
    std::string uwbMode;
    cmd.AddValue("uwbMode",
                 "Apply the scenario's --uwbMode row (Dw3000Table9Row), exactly as ca_tbl_causal_sim "
                 "does -- tier 3's table filled flat with the row's sensitivity -- so G93 can check "
                 "tier 3 in the mode the scenario runs. Absent: the code default's -104 dBm table",
                 uwbMode);
    cmd.Parse(argc, argv);

    LoRaPhysicsConfig c868 =
        LoRaPhysicsConfig::ForScenario(static_cast<EnvironmentClass>(envWide),
                                       WEATHER_CLEAR_DRY,
                                       FADING_RICIAN,
                                       static_cast<VegetationType>(vegWide),
                                       MOB_CONSTANT_VELOCITY,
                                       false);
    LoRa2p4PhysicsConfig c2p4 =
        LoRa2p4PhysicsConfig::ForScenario(static_cast<EnvironmentClass>(envFacility),
                                          WEATHER_CLEAR_DRY,
                                          FADING_RICIAN,
                                          static_cast<VegetationType>(vegFacility),
                                          MOB_CONSTANT_VELOCITY,
                                          false);
    LoRaUWBPhysicsConfig cUwb =
        LoRaUWBPhysicsConfig::ForScenario(static_cast<EnvironmentClass>(envFacility),
                                          WEATHER_CLEAR_DRY,
                                          FADING_RICIAN,
                                          static_cast<VegetationType>(vegFacility),
                                          MOB_CONSTANT_VELOCITY,
                                          false);
    if (t2Row > 0.0 && !Sx1280LowPowerSensitivityRow(t2Row, c2p4.receiver.sensitivityDbm))
    {
        std::fprintf(stderr, "FATAL: %.3f kHz is not a tabulated SX1280 bandwidth\n", t2Row);
        return 1;
    }
    if (!uwbMode.empty())
    {
        uint32_t preamble = 0;
        double rateBps = 0.0;
        double sensDbm = 0.0;
        if (!Dw3000Table9Row(uwbMode, preamble, rateBps, sensDbm))
        {
            std::fprintf(stderr, "FATAL: --uwbMode must be 6m8 or 850k, got '%s'\n", uwbMode.c_str());
            return 1;
        }
        for (int si = 0; si < 8; ++si)
        {
            cUwb.receiver.sensitivityDbm[si] = sensDbm;
        }
    }
    if (bw2p4 > 0.0 && !(t2Row > 0.0))
    {
        for (int sfi = 0; sfi < 8; ++sfi)
        {
            c2p4.receiver.sensitivityDbm[sfi] += 10.0 * std::log10(bw2p4 / 800e3);
        }
    }
    c868.rangingModelMode = RANGING_PHYSICS_DERIVED;
    c2p4.rangingModelMode = RANGING_PHYSICS_DERIVED;
    cUwb.rangingModelMode = RANGING_PHYSICS_DERIVED;

    LoRaPhysicsEngine e868;
    e868.Initialize(c868);
    LoRa2p4PhysicsEngine e2p4;
    e2p4.Initialize(c2p4);
    LoRaUWBPhysicsEngine eUwb;
    eUwb.Initialize(cUwb);

    std::printf("tier,sf,crDenom,configuredCr,thresholdDbm\n");
    for (uint32_t sf = 5; sf <= 12; ++sf)
    {
        for (uint32_t cr = 5; cr <= 8; ++cr)
        {
            std::printf("868,%u,%u,%u,%.6f\n",
                        sf,
                        cr,
                        static_cast<unsigned>(e868.GetConfig().hardware.codingRateDenom),
                        e868.GetSensitivityThreshold(static_cast<uint8_t>(sf),
                                                     static_cast<uint8_t>(cr)));
            std::printf("2p4,%u,%u,%u,%.6f\n",
                        sf,
                        cr,
                        static_cast<unsigned>(e2p4.GetConfig().hardware.codingRateDenom),
                        e2p4.GetSensitivityThreshold(static_cast<uint8_t>(sf),
                                                     static_cast<uint8_t>(cr)));
            std::printf("uwb,%u,%u,%u,%.6f\n",
                        sf,
                        cr,
                        static_cast<unsigned>(eUwb.GetConfig().hardware.codingRateDenom),
                        eUwb.GetSensitivityThreshold(static_cast<uint8_t>(sf),
                                                     static_cast<uint8_t>(cr)));
        }
    }
    return 0;
}
