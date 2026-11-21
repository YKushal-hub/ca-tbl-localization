/*
 * CA-TBL collapse extension: the WORLD generator (DD 37).
 *
 * A collapsed building is a rubble pile (CaTblRubbleWorld, DD 36) holding N buried
 * worker devices. Rescuers stand on the pile surface at candidate points, holding
 * their three-radio device at one of several heights. Stations stand on the ground
 * around the pile. For every time slot, every band and every (worker, point),
 * (worker, station) and (worker, worker) pair, the three physics engines draw one
 * link outcome. The rubble on the straight path adds its loss to the budget and its
 * delay to time-of-flight ranges.
 *
 * Estimation is NOT done here. This program writes two kinds of file:
 *   obs_*.csv    what the system can observe (link outcomes; GPS fixes of rescuer
 *                points and stations, with GPS noise);
 *   truth_*.csv  ground truth, read only by the analysis that scores the estimates.
 * The system-side estimators (scripts/collapse/) read obs_*.csv only (World/system
 * split, CLAUDE.md; enforced by gate G178).
 *
 * The engines use the keyed random mode (--rngKeyed in the main simulator): every
 * draw is keyed to (link, time), so the outcome of a (worker, point, slot) link does
 * not depend on which other links were evaluated. A rescuer policy that chooses
 * points from this field therefore sees exactly what it would have measured had it
 * gone there.
 *
 * The original simulator (ca_tbl_causal_sim) is untouched; every earlier result
 * reproduces byte for byte.
 *
 * Every geometry value is a required argument, taken in a pre-registration from a
 * row of docs/SOURCES_COLLAPSE.md or swept across a sourced range. There are no
 * hidden geometry defaults.
 */
#include "ns3/ca-tbl-rubble-world.h"
#include "ns3/core-module.h"
#include "ns3/lora-2p4-physics-engine.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-uwb-physics-engine.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace ns3;

namespace
{

struct Phy
{
    uint8_t sf;
    double bw;
    double txDbm;
};

// Same PHY as the main simulator's defaults (ca_tbl_causal_sim.cc, phy[] in main()).
const Phy kPhy[3] = {{7, 125e3, 14.0}, {7, 800e3, 12.5}, {7, 499.2e6, -16.47}};
const char* kBand[3] = {"868", "2400", "uwb"};

struct Pt
{
    uint32_t id;
    Vector p;
};

std::vector<double>
ParseList(const std::string& s)
{
    std::vector<double> v;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ','))
    {
        if (!tok.empty())
        {
            v.push_back(std::stod(tok));
        }
    }
    return v;
}

bool
Missing(double v)
{
    return std::isnan(v);
}

} // namespace

int
main(int argc, char** argv)
{
    const double NaN = std::nan("");
    uint32_t seed = 1;
    std::string outdir;
    std::string bracketArg = "central";
    std::string envArg = "open";
    double footX = NaN, footY = NaN, pileH = NaN, depthMin = NaN, depthMax = NaN;
    uint32_t nVictims = 0;
    double voidHalf = 0.0;
    double gridStep = NaN;
    std::string holdArg;
    uint32_t nStations = 0;
    double stationStandoff = NaN, stationH = NaN;
    uint32_t slots = 0;
    double slotS = NaN;
    double gpsSigmaH = NaN, gpsSigmaV = NaN;
    bool victimLinks = true;
    double localSigma = NaN;
    bool airLinks = true;

    CommandLine cmd;
    cmd.AddValue("seed", "Run seed (engines and World draws)", seed);
    cmd.AddValue("outdir", "Output directory (required; never the source tree)", outdir);
    cmd.AddValue("bracket", "Rubble bracket low|central|high (DD 36)", bracketArg);
    cmd.AddValue("env", "Engine environment around the pile: open (ENV_RURAL_OPEN, all "
                        "obstruction is the rubble model) or industrial (ENV_INDUSTRIAL, "
                        "the engines' own clutter on top: sensitivity case)", envArg);
    cmd.AddValue("footX", "Pile footprint along x, m (required)", footX);
    cmd.AddValue("footY", "Pile footprint along y, m (required)", footY);
    cmd.AddValue("pileH", "Pile height, m (required; row PILE-ALPHA x intact height)", pileH);
    cmd.AddValue("nVictims", "Buried worker devices (required, >= 1)", nVictims);
    cmd.AddValue("depthMin", "Least rubble above a device, m (required)", depthMin);
    cmd.AddValue("depthMax", "Most rubble above a device, m (required, <= pileH)", depthMax);
    cmd.AddValue("voidHalf", "Half-size of the survivable void around each device, m "
                             "(0 = none)", voidHalf);
    cmd.AddValue("gridStep", "Spacing of candidate rescuer points on the surface, m (required)",
                 gridStep);
    cmd.AddValue("holdHeights", "Device heights above the surface, m, comma list (required)",
                 holdArg);
    cmd.AddValue("nStations", "Ground stations on a ring around the pile (0 allowed)", nStations);
    cmd.AddValue("stationStandoff", "Station distance outside the footprint edge, m", stationStandoff);
    cmd.AddValue("stationH", "Station antenna height, m", stationH);
    cmd.AddValue("slots", "Time slots (required, >= 1)", slots);
    cmd.AddValue("slotS", "Slot length, s (required)", slotS);
    cmd.AddValue("gpsSigmaH", "GPS error per horizontal axis, m (rows GPS-SPS-*)", gpsSigmaH);
    cmd.AddValue("gpsSigmaV", "GPS vertical error, m (rows GPS-SPS-*)", gpsSigmaV);
    cmd.AddValue("victimLinks", "Also draw worker-to-worker links", victimLinks);
    cmd.AddValue("localSigma", "Error per axis of a rescuer point / station in the rescuers' local "
                               "frame (UWB line-of-sight ranging among rescuers), m (required)", localSigma);
    cmd.AddValue("airLinks", "Also draw point-to-station links (clear air, no rubble), which the "
                             "system uses to fit its own air path-loss law", airLinks);
    cmd.Parse(argc, argv);

    // ---- refusals: every geometry value must be given --------------------------------
    auto fatal = [](const std::string& m) {
        std::cerr << "FATAL: " << m << "\n";
        return 1;
    };
    if (outdir.empty())
    {
        return fatal("--outdir is required");
    }
    CaTblRubbleBracket bracket;
    if (!ParseRubbleBracket(bracketArg, bracket))
    {
        return fatal("--bracket must be low, central or high");
    }
    if (envArg != "open" && envArg != "industrial")
    {
        return fatal("--env must be open or industrial");
    }
    if (Missing(footX) || Missing(footY) || Missing(pileH) || Missing(depthMin) ||
        Missing(depthMax) || Missing(gridStep) || Missing(slotS) || Missing(gpsSigmaH) ||
        Missing(gpsSigmaV) || Missing(localSigma) || holdArg.empty() || nVictims == 0 || slots == 0)
    {
        return fatal("a required geometry/time/GPS argument is missing (see --PrintHelp)");
    }
    if (footX <= 0 || footY <= 0 || pileH <= 0 || depthMin <= 0 || depthMax < depthMin ||
        depthMax > pileH || gridStep <= 0 || slotS <= 0 || gpsSigmaH < 0 || gpsSigmaV < 0 || localSigma < 0 ||
        voidHalf < 0)
    {
        return fatal("geometry out of range (need 0 < depthMin <= depthMax <= pileH, positive sizes)");
    }
    if (nStations > 0 && (Missing(stationStandoff) || Missing(stationH)))
    {
        return fatal("--nStations > 0 needs --stationStandoff and --stationH");
    }
    const std::vector<double> holds = ParseList(holdArg);
    if (holds.empty())
    {
        return fatal("--holdHeights is empty");
    }
    mkdir(outdir.c_str(), 0755);

    // ---- World: pile and devices ----------------------------------------------------
    CaTblRubbleWorld world(bracket);
    if (!world.AddRubble(Vector(0, 0, 0), Vector(footX, footY, pileH)))
    {
        return fatal("pile box refused");
    }
    std::mt19937_64 wrng(0x9E3779B97F4A7C15ULL ^ static_cast<uint64_t>(seed));
    std::uniform_real_distribution<double> U(0.0, 1.0);
    std::vector<Pt> victims;
    const double margin = std::max(voidHalf, 0.5);
    for (uint32_t i = 0; i < nVictims; ++i)
    {
        const double x = margin + U(wrng) * (footX - 2 * margin);
        const double y = margin + U(wrng) * (footY - 2 * margin);
        const double d = depthMin + U(wrng) * (depthMax - depthMin);
        const Vector p(x, y, pileH - d);
        if (voidHalf > 0.0)
        {
            const double zlo = std::max(0.0, p.z - voidHalf);
            const double zhi = std::min(pileH - 1e-6, p.z + voidHalf);
            world.AddVoid(Vector(x - voidHalf, y - voidHalf, zlo), Vector(x + voidHalf, y + voidHalf, zhi));
            // an overlapping void is refused; the device then sits in rubble (recorded in truth)
        }
        victims.push_back({1 + i, p});
    }
    // Candidate rescuer points: a grid on the surface, one point per hold height.
    std::vector<Pt> points;
    uint32_t pid = 10000;
    for (double x = gridStep / 2; x < footX; x += gridStep)
    {
        for (double y = gridStep / 2; y < footY; y += gridStep)
        {
            for (double h : holds)
            {
                points.push_back({pid++, Vector(x, y, pileH + h)});
            }
        }
    }
    std::vector<Pt> stations;
    for (uint32_t s = 0; s < nStations; ++s)
    {
        const double a = 2.0 * M_PI * s / nStations;
        const double rx = footX / 2 + stationStandoff, ry = footY / 2 + stationStandoff;
        stations.push_back({20000 + s, Vector(footX / 2 + rx * std::cos(a), footY / 2 + ry * std::sin(a), stationH)});
    }

    // ---- Engines --------------------------------------------------------------------
    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(1);
    RngSeedManager::ResetNextStreamIndex();
    LoRaDistributionSampler::SetKeyedMode(true, seed);
    const EnvironmentClass env = (envArg == "open") ? ENV_RURAL_OPEN : ENV_INDUSTRIAL;
    LoRaPhysicsConfig c868 = LoRaPhysicsConfig::ForScenario(env, WEATHER_CLEAR_DRY, FADING_RICIAN,
                                                            VEG_NONE, MOB_CONSTANT_VELOCITY, false);
    LoRa2p4PhysicsConfig c2p4 = LoRa2p4PhysicsConfig::ForScenario(env, WEATHER_CLEAR_DRY, FADING_RICIAN,
                                                                  VEG_NONE, MOB_CONSTANT_VELOCITY, false);
    LoRaUWBPhysicsConfig cUwb = LoRaUWBPhysicsConfig::ForScenario(env, WEATHER_CLEAR_DRY, FADING_RICIAN,
                                                                  VEG_NONE, MOB_CONSTANT_VELOCITY, false);
    c868.randomSeed = seed;
    c2p4.randomSeed = seed;
    cUwb.randomSeed = seed;
    c868.rangingModelMode = RANGING_PHYSICS_DERIVED;
    c2p4.rangingModelMode = RANGING_PHYSICS_DERIVED;
    cUwb.rangingModelMode = RANGING_PHYSICS_DERIVED;
    // Link outcomes here mean "this link scheduled alone": all draws of a slot share one instant,
    // so the engines' registry of the scenario's own transmissions would score them as colliding
    // with each other. Contention among many devices is the MAC stage's job (B5), not the
    // World generator's. The registry and the external-interferer field are one switch
    // (interference.enabled), so both are off here; external interference at a collapse site
    // is a sensitivity case for later, not modelled in this generator (stated in DD 37).
    c868.interference.enabled = false;
    c2p4.interference.enabled = false;
    cUwb.interference.enabled = false;
    LoRaPhysicsEngine e868;
    e868.Initialize(c868);
    LoRa2p4PhysicsEngine e2p4;
    e2p4.Initialize(c2p4);
    LoRaUWBPhysicsEngine eUwb;
    eUwb.Initialize(cUwb);
    e868.SetRubbleWorld(&world);
    e2p4.SetRubbleWorld(&world);
    eUwb.SetRubbleWorld(&world);
    auto reg = [&](const Pt& q) {
        e868.RegisterGateway(q.id, q.p, true);
        e2p4.RegisterGateway(q.id, q.p, true);
        eUwb.RegisterGateway(q.id, q.p, true);
        e868.RegisterNode(q.id, q.p);
        e2p4.RegisterNode(q.id, q.p);
        eUwb.RegisterNode(q.id, q.p);
    };
    for (const auto& q : victims)
    {
        reg(q);
    }
    for (const auto& q : points)
    {
        reg(q);
    }
    for (const auto& q : stations)
    {
        reg(q);
    }

    // ---- Truth and GPS observations ---------------------------------------------------
    {
        std::ofstream t(outdir + "/truth_victims.csv");
        t << "id,x,y,z,depth_m,in_void\n" << std::fixed << std::setprecision(4);
        for (const auto& v : victims)
        {
            t << v.id << "," << v.p.x << "," << v.p.y << "," << v.p.z << "," << (pileH - v.p.z) << ","
              << (world.InRubble(v.p) ? 0 : 1) << "\n";
        }
        std::ofstream ta(outdir + "/truth_anchors.csv");
        std::ofstream oa(outdir + "/obs_anchors.csv");
        ta << "id,kind,x,y,z\n" << std::fixed << std::setprecision(4);
        oa << "id,kind,gps_x,gps_y,gps_z,sigma_h,sigma_v,loc_x,loc_y,loc_z,sigma_loc\n"
           << std::fixed << std::setprecision(4);
        std::mt19937_64 grng(0xD1B54A32D192ED03ULL ^ static_cast<uint64_t>(seed));
        std::mt19937_64 lrng(0xA0761D6478BD642FULL ^ static_cast<uint64_t>(seed));
        std::normal_distribution<double> N01(0.0, 1.0);
        auto put = [&](const Pt& q, const char* kind) {
            ta << q.id << "," << kind << "," << q.p.x << "," << q.p.y << "," << q.p.z << "\n";
            const double gx = q.p.x + gpsSigmaH * N01(grng);
            const double gy = q.p.y + gpsSigmaH * N01(grng);
            const double gz = q.p.z + gpsSigmaV * N01(grng);
            oa << q.id << "," << kind << "," << gx << "," << gy << "," << gz << "," << gpsSigmaH << ","
               << gpsSigmaV << "," << q.p.x + localSigma * N01(lrng) << "," << q.p.y + localSigma * N01(lrng)
               << "," << q.p.z + localSigma * N01(lrng) << "," << localSigma << "\n";
        };
        for (const auto& q : points)
        {
            put(q, "point");
        }
        for (const auto& q : stations)
        {
            put(q, "station");
        }
    }

    // ---- Link draws, one scheduled event per slot ------------------------------------
    std::ofstream ol(outdir + "/obs_links.csv");
    ol << "slot,t_s,band,tx,rx,received,rx_dbm,snr_db,range_m\n";
    ol << std::fixed << std::setprecision(4);
    std::ofstream tl(outdir + "/truth_links.csv"); // World diagnostics: pile metres, rubble loss
    tl << "band,tx,rx,dist_m,path_kind,path_len_m,rubble_m,extra_db,bias_m\n" << std::fixed << std::setprecision(4);
    uint64_t draws = 0;
    bool truthWritten = false;
    auto draw = [&](uint32_t slot, int b, const Pt& tx, const Pt& rx) {
        double rxDbm = 0, snr = 0, rng = 0;
        bool ok = false;
        const Time now = Simulator::Now();
        if (b == 0)
        {
            PhysicsResult r = e868.ProcessTransmission(tx.id, rx.id, tx.p, rx.p, kPhy[0].txDbm, kPhy[0].sf,
                                                       kPhy[0].bw, 0, now);
            ok = r.packetReceived;
            rxDbm = r.rxPowerDbm;
            snr = r.snr_db;
            rng = r.estimatedRangeM;
        }
        else if (b == 1)
        {
            PhysicsResult2p4 r = e2p4.ProcessTransmission(tx.id, rx.id, tx.p, rx.p, kPhy[1].txDbm, kPhy[1].sf,
                                                          kPhy[1].bw, 0, now);
            ok = r.packetReceived;
            rxDbm = r.rxPowerDbm;
            snr = r.snr_db;
            rng = r.estimatedRangeM;
        }
        else
        {
            PhysicsResultUWB r = eUwb.ProcessTransmission(tx.id, rx.id, tx.p, rx.p, kPhy[2].txDbm, kPhy[2].sf,
                                                          kPhy[2].bw, 0, now);
            ok = r.packetReceived;
            rxDbm = r.rxPowerDbm;
            snr = r.snr_db;
            rng = r.estimatedRangeM;
        }
        ++draws;
        // The system observes a range only on a received frame.
        ol << slot << "," << now.GetSeconds() << "," << kBand[b] << "," << tx.id << "," << rx.id << ","
           << (ok ? 1 : 0) << "," << rxDbm << "," << snr << ",";
        if (ok)
        {
            ol << rng;
        }
        ol << "\n";
        if (!truthWritten)
        {
            const double dx = tx.p.x - rx.p.x, dy = tx.p.y - rx.p.y, dz = tx.p.z - rx.p.z;
            const double f = (b == 0) ? 868e6 : (b == 1) ? 2.44e9 : 7.99e9;
            const CaTblRubblePath rp = world.Path(f, tx.p, rx.p);
            tl << kBand[b] << "," << tx.id << "," << rx.id << "," << std::sqrt(dx * dx + dy * dy + dz * dz) << ","
               << rp.kind << "," << rp.lengthM << "," << rp.rubbleM << "," << rp.extraLossDb << "," << rp.biasM
               << "\n";
        }
    };
    for (uint32_t s = 0; s < slots; ++s)
    {
        Simulator::Schedule(Seconds(slotS * s), [&, s]() {
            for (int b = 0; b < 3; ++b)
            {
                for (const auto& v : victims)
                {
                    for (const auto& q : points)
                    {
                        draw(s, b, v, q);
                    }
                    for (const auto& q : stations)
                    {
                        draw(s, b, v, q);
                    }
                }
                if (airLinks)
                {
                    for (const auto& q : points)
                    {
                        for (const auto& st : stations)
                        {
                            draw(s, b, q, st);
                        }
                    }
                }
                if (victimLinks)
                {
                    for (size_t i = 0; i < victims.size(); ++i)
                    {
                        for (size_t j = i + 1; j < victims.size(); ++j)
                        {
                            draw(s, b, victims[i], victims[j]);
                        }
                    }
                }
            }
            truthWritten = true;
        });
    }
    Simulator::Run();
    Simulator::Destroy();

    std::ofstream rp(outdir + "/run.txt");
    rp << "ca_tbl_collapse_sim seed=" << seed << " bracket=" << bracketArg << " env=" << envArg
       << " footX=" << footX << " footY=" << footY << " pileH=" << pileH << " nVictims=" << nVictims
       << " depth=[" << depthMin << "," << depthMax << "] voidHalf=" << voidHalf << " gridStep=" << gridStep
       << " holdHeights=" << holdArg << " nStations=" << nStations << " slots=" << slots << " slotS=" << slotS
       << " gpsSigmaH=" << gpsSigmaH << " gpsSigmaV=" << gpsSigmaV << " localSigma=" << localSigma
       << " victimLinks=" << victimLinks << " airLinks=" << airLinks << "\n"
       << "points=" << points.size() << " stations=" << stations.size() << " draws=" << draws << "\n";
    std::printf("COLLAPSE draws=%llu points=%zu victims=%zu\n", static_cast<unsigned long long>(draws),
                points.size(), victims.size());
    return 0;
}
