/**
 * \file ca_tbl_causal_sim.cc
 * \brief CA-TBL Phase S1 + S2 + S2.5 -- causal tri-tier simulation with baseline arms.
 *
 * S1 (causal decision) and S2 (energy accounting)
 * ----------------------------------------------
 * Exactly one tier transmits per cycle -- the one the policy selected on the
 * previous cycle -- and only that tier is charged energy. An arm that never
 * escalates accrues exactly zero UWB joules, because no UWB packet was ever
 * generated. That is what makes the energy comparison mean anything.
 *
 * Escalation takes effect on the NEXT cycle and costs a request/grant round-trip
 * on the current tier. Both are deliberate: a real handoff has latency, and if
 * thrashing is free the hysteresis parameters cannot be shown to do any work.
 *
 * S2.5 (tiered deployment geometry) -- WHY THIS REWRITE HAPPENED
 * -------------------------------------------------------------
 * The first version placed ONE set of three anchors in a 100 m equilateral
 * triangle and let all three tiers range against it, while the node random-walked
 * inside a 1.2 km box. That produced results/causal/arm_summary.csv, in which:
 *
 *   - static_uwb had noFixCycles = 600/600. UWB's ceiling is ~100 m LOS / 30 m
 *     NLOS, so it could essentially never see three anchors 100 m apart from
 *     anywhere in a 1.2 km box. Its 288 m RMSE was pure dead reckoning.
 *   - static_uwb was consequently the energy FLOOR (0.68 J) rather than the
 *     ceiling, because a radio that never completes a ranging exchange is never
 *     charged for one. The energy axis of the architectural claim was inverted.
 *   - 497-600 of 600 cycles produced no fix in five of six arms, so every RMSE
 *     was dominated by EKF coasting rather than by localisation.
 *   - static_2p4 beat the proposed system 3.8x on accuracy-per-joule.
 *
 * None of that measured the policy. It measured a geometry in which two of the
 * three tiers were physically inoperable. A tier can only be shown to be worth
 * escalating to if there exist places where it works and places where it does
 * not; a single flat anchor triangle cannot express that.
 *
 * The deployment is now tiered, which is also what the design document actually
 * specifies:
 *
 *   Tier 1, 868 MHz  -- 5 wide-area gateways over the whole 3 km box.
 *                       Always available, coarse, cheap. The coverage floor.
 *   Tier 2, 2.4 GHz  -- 3-anchor triads at each facility site, ~150 m triad
 *                       radius. Usable within a few hundred metres of a site.
 *   Tier 3, UWB      -- dense 3-anchor triads co-located at the same sites,
 *                       ~35 m triad radius. Usable only inside a site.
 *
 * and the trajectory is now a facility tour rather than an unconstrained random
 * walk, so the node repeatedly enters and leaves tier-2 and tier-3 coverage.
 * That transit is the regime the escalation decision exists to handle. Without
 * it there is nothing for the policy to be right or wrong about.
 *
 * Ground-truth coverage labels
 * ----------------------------
 * Each step also records how many anchors of each tier lie within that tier's
 * nominal range (vis868 / vis2p4 / visUwb). These are pure geometry, computed
 * independently of the physics outcome, and they are the substrate for claim (3):
 * "should have escalated to UWB" is definable as visUwb >= 3, which gives the
 * trigger a confusion matrix instead of an assertion. See S7 in
 * CA_TBL_SIM_AUDIT.md.
 *
 * Arms
 * ----
 *   static_868          never escalates -- long range, coarse, cheap
 *   static_2p4          pinned to tier 2
 *   static_uwb          always-on UWB -- accuracy ceiling, energy ceiling
 *   fixed_threshold     flat variance trigger -- the prior-art baseline
 *   ca_tbl_no_integrity PCS escalation only -- isolates the integrity check
 *   ca_tbl_full         proposed system
 *
 * Usage:
 *   # 1. Check the geometry against the physics BEFORE trusting any arm result.
 *   ./ns3 run "ca_tbl_causal_sim --probe=true"
 *
 *   # 2. Single seed, all six arms.
 *   ./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal"
 *
 *   # 3. Seed sweep once the invariants pass.
 *   ./ns3 run "ca_tbl_causal_sim --seedStart=1 --seedCount=20 --duration=3600 \
 *              --outdir=results/causal --trace=false"
 */

#include "ns3/ca-tbl-relay-admission.h"
#include "ns3/ca-tbl-routing.h"
#include "ns3/ca-tbl-node-lifecycle.h"
#include "ns3/ca-tbl-session.h"
#include "ns3/ca-tbl-sparse-anchor.h"
#include "ns3/ca-tbl-tdoa.h"
#include "ns3/ca-tbl-nlos-classifier.h"
#include "ns3/ca-tbl-imu-model.h"
#include "ns3/lora-shs-engine.h"
#include "ns3/lora-shs-parameters.h"
#include "ns3/core-module.h"
#include "ns3/ca-tbl-energy-model.h"
#include "ns3/ca-tbl-header.h"
#include "ns3/ca-tbl-mac.h"
#include "ns3/ca-tbl-policy.h"
#include "ns3/lora-physics-engine.h"
#include "ns3/lora-2p4-physics-engine.h"
#include "ns3/lora-uwb-physics-engine.h"
#include "ns3/lora-physics-parameters.h"
#include "ns3/lora-2p4-physics-parameters.h"
#include "ns3/lora-uwb-physics-parameters.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <deque>
#include <fstream>
#include <functional>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace ns3;

namespace
{

struct AnchorDef
{
    uint32_t id;
    Vector pos;
};

/** Per-tier PHY settings, kept identical to the previous generator so physics
 *  results remain comparable with the datasets already collected. */
struct TierPhy
{
    uint8_t sf;
    double bandwidthHz;
    double txPowerDbm;
};

/**
 * \brief Tiered anchor infrastructure.
 *
 * Each tier owns its own anchor set. This is the S2.5 fix: previously a single
 * anchor list was shared by all three tiers, which forced every tier into the
 * same spatial scale and made two of them inoperable.
 *
 * \c nominalRangeM is NOT used to gate reception -- the physics engines decide
 * that. It is used only to compute the geometric visibility labels that claim (3)
 * scores the trigger against. Keeping the two separate is deliberate: if the
 * label were derived from the physics outcome it would be circular.
 */
struct Deployment
{
    std::vector<AnchorDef> anchors[CA_TBL_TIER_COUNT];
    double nominalRangeM[CA_TBL_TIER_COUNT];
    std::vector<Vector> facilities;
    double boxHalfM;
    //! Where the infrastructure stands. hasBaseCamp is false for every deployment
    //! before 2026-09-08, in which the gateways surround the casualty rather than
    //! sitting somewhere a search sets out FROM.
    bool hasBaseCamp = false;
    double baseCampX = 0.0;
    double baseCampY = 0.0;
    /** How far the node may wander from a site centre while dwelling there.
     *  Tied to the UWB ring radius so "at a facility" and "in UWB range" cannot
     *  drift apart. */
    double dwellRadiusM;
};

/**
 * \brief Build the tiered deployment.
 *
 * Distances are chosen from this repo's own measured PDR-vs-distance data
 * (results/metric2_pdr_distance_seed7.csv) and from the UWB range figures the
 * validation journey settled on (100 m LOS / 30 m NLOS, corrected in Bug #9 of
 * simstatus.md), not from assumption:
 *
 *   - SF7 at 868 MHz holds PDR >= 0.945 at 3 km in Suburban (PLE 2.75). Forest
 *     (PLE 2.3) is less lossy than Suburban, so gateways at up to ~1.9 km
 *     slant range are comfortably inside the reliable regime.
 *   - Note SF12 is NOT used: in the same dataset SF12 PDR is 0.255-0.355 even at
 *     500 m, far below SF7. That inversion is collision-dominated (SF12 airtime
 *     is ~30x SF7, so it is exposed to far more interferer overlap) and is worth
 *     investigating separately, but it means SF12 is not a usable tier-1 setting
 *     in this simulator today.
 *   - UWB triad radius 35 m gives a ~60 m side length, so a node anywhere inside
 *     a site is within ~70 m of all three anchors -- inside the 100 m LOS
 *     ceiling with margin for the fuzzy edge, and NOT inside the 30 m NLOS
 *     ceiling, which is the point: UWB must sometimes fail even at a site.
 *
 * If --probe disagrees with any of this, change the geometry, not the claim.
 */
Deployment
BuildDeployment(uint32_t gwGrid,
                double boxHalfM,
                double uwbTriadRadiusM,
                double t2TriadRadiusM,
                uint32_t uwbAnchorsPerSite,
                uint32_t t2AnchorsPerSite,
                uint32_t sitingMode,
                uint32_t topology,
                uint32_t nGateways,
                double gwRingFrac,
                bool gwRidge,
                double gwRidgeY,
                double gwArcDeg,
                double gwArcCentreDeg)
{
    Deployment dep;
    dep.boxHalfM = boxHalfM;
    dep.dwellRadiusM = uwbTriadRadiusM;

    // Nominal ranges for the geometric visibility labels only (NOT reception
    // gates -- the engines decide that).
    //
    // These are r50, and an earlier attempt to "correct" them to r80 was WRONG in a
    // way worth recording, because it silently zeroed the entire trigger metric.
    //
    // The mistaken argument was: the label means "could this tier plausibly have
    // solved here", a ranging exchange needs most attempts to land, so r50 labels as
    // covered a distance at which half the links fail. That reasoning treats a fix
    // as needing ONE good link. It needs THREE, out of N anchors, and this
    // deployment deliberately sites N > 3 (5 UWB, 3 tier-2 per site, 9 tier-1). At
    // per-link p = 0.5 with N = 5 and 3 required,
    //     P(>=3 of 5) = (10 + 5 + 1)/32 = 0.5
    // so r50 IS the boundary at which a fix becomes a coin flip. That is exactly
    // what "could plausibly have solved" means. At r80 the same geometry gives
    // P(>=3 of 5) = 0.94, i.e. "would reliably solve" -- a different question, and
    // the wrong one for a ground-truth coverage label.
    //
    // The r80 version also had a second, fatal defect: setting the UWB label radius
    // to 10.0 m while --uwbTriad is also 10.0 m makes the label unsatisfiable. The
    // anchors sit ON a 10 m ring at z = 2.5 and the node is at z = 1.5, so even at
    // the exact facility centre the slant range is sqrt(10^2 + 1^2) = 10.0499 m,
    // just outside. VisibleCount() returned 0 at every step of every arm, visUwb was
    // identically zero, and trigF1 was therefore 0.000 on all six arms. A label
    // radius must never be set equal to the anchor ring radius it is labelling.
    //
    // Values from the 64-site decorrelated probe (2000 trials/point):
    //   868  r80 1522 m, r50 3217 m. 3000 m labels the whole +/-1000 m box as
    //        covered, which the physics agrees with: static_868 records 0 nofix
    //        cycles out of 1440, i.e. tier 1 solves every cycle everywhere.
    //   2p4  r80 >= 900 m -- CENSORED, the sweep ends at 900 m with PDR 0.804 and
    //        the tier has not fallen off. 900 m is a LOWER BOUND, not a measurement.
    //        Extend dists2p4 past 900 m before treating this as tier-2's range.
    //   UWB  r80 12.9 m, r50 29.5 m. Near-pure-LOS run (nlos_frac <= 0.004) yet r80
    //        is a tenth of the 100 m LOS reference spec in simstatus.md Bug #9: the
    //        facility UWB link budget is the constraint, not NLOS.
    dep.nominalRangeM[CA_TBL_TIER_868] = 3000.0;
    dep.nominalRangeM[CA_TBL_TIER_2P4] = 900.0;
    dep.nominalRangeM[CA_TBL_TIER_UWB] = 30.0;

    // ---- Tier 1: wide-area gateways on a 3x3 grid. -------------------------
    // Previously 4 corners + centre at 0.9 * boxHalf. With the measured 868 r80
    // of 1000 m that put most of the box outside reliable range of any three
    // gateways, so tier 1 -- the coverage floor the whole architecture rests on --
    // would have been intermittent and every arm would have been measuring
    // dead reckoning again.
    //
    // A 3x3 grid spanning +/-0.9*boxHalf gives a cell pitch of 0.9*boxHalf, so the
    // worst-case point (a cell centre) is 0.64 * pitch from its three nearest
    // gateways. At the default boxHalf of 1000 m that is ~640 m, where measured
    // PDR is ~0.95.
    //
    // This is not a free parameter choice: it says the tier-1 gateway density
    // needed for reliable 3-gateway coverage at SF7 in forest is ~9 gateways per
    // 4 km^2. That is a derived deployment-cost result and it belongs in the
    // report -- it is the "gateway density dominates everything downstream"
    // conclusion of simstatus.md 8.5, now grounded in measured PDR instead of
    // assumed range. Sweep --boxHalf to trace the sensitivity curve.
    // gwGrid generalises the 3x3 above. THE GRID IS FIXED WHILE THE AREA IS NOT,
    // so sweeping --boxHalf at gwGrid=3 varies gateway DENSITY as well as tier-2
    // coverage: 9 gateways over (2*boxHalf)^2 is 2.25/km^2 at boxHalf 1000 -- the
    // ~9-per-4km^2 the comment above derives as the requirement -- but 0.25/km^2 at
    // boxHalf 3000, one ninth of it. Measured consequence: a typical tier-1 fix at
    // boxHalf 3000 uses 3.70 of the 9 gateways, barely above the minimum of three.
    // Hold density constant with gwGrid = 3 * (boxHalf/1000) to separate "tier-2
    // coverage became spotty" from "tier-1 redundancy collapsed".
    const double g = boxHalfM * 0.9;
    const double gwH = 15.0;
    uint32_t id = 101;
    if (nGateways > 0)
    {
        // RING placement. The brief asks for three gateways, and a square grid cannot
        // produce three; the closest it manages is four. Evenly spaced on a ring gives
        // an equilateral triangle at N = 3 -- the minimum geometry for a 2-D fix, and
        // the best conditioning three points can offer the interior.
        //
        // The ring radius is a fraction of boxHalf rather than an absolute, so the
        // deployment scales with the area the way the grid does and the two remain
        // comparable across box sizes.
        const double R = gwRingFrac * boxHalfM;
        if (gwRidge)
        {
            // RIDGE placement: gateways strung along a line, which is how coverage is
            // actually deployed on a hiking route -- repeaters follow the ridge because
            // that is where the line of sight and the access road are, not because it is
            // good geometry.
            //
            // AND IT IS BAD GEOMETRY, DELIBERATELY. Collinear anchors give poor GDOP
            // across the line and near-singular conditioning along it, so a walker
            // north or south of the ridge is far worse localised than one on it. That is
            // the real deployment's real weakness and the reason a deviation off the
            // trail is dangerous rather than merely inconvenient. Placing them on a ring
            // would hide the effect the scenario exists to show.
            const double span = 1.6 * boxHalfM;
            for (uint32_t k = 0; k < nGateways; ++k)
            {
                const double fx = (nGateways == 1)
                                      ? 0.0
                                      : -span / 2.0 + span * static_cast<double>(k) /
                                                          static_cast<double>(nGateways - 1);
                dep.anchors[CA_TBL_TIER_868].push_back(
                    {id++, Vector(fx, gwRidgeY * boxHalfM, gwH)});
            }
        }
        else
        {
        // ARC placement when gwArcDeg < 360: the anchors follow a curving access
        // route rather than closing a ring around the search area. Centred on north, so
        // the trail runs beneath the arc and a subject can stray OUTSIDE it (through the
        // anchors, where they collapse toward one bearing) or INSIDE it (away from them,
        // toward the centre of curvature). Those two are geometrically different cases
        // and the scenario should be able to produce both.
        //
        // At 360 this is exactly the previous even ring, so existing runs reproduce.
        const double arc = (gwArcDeg > 0.0 && gwArcDeg < 360.0) ? gwArcDeg : 360.0;
        const double sweep = arc * M_PI / 180.0;
        for (uint32_t k = 0; k < nGateways; ++k)
        {
            double th;
            if (arc >= 360.0)
            {
                th = 2.0 * M_PI * static_cast<double>(k) /
                         static_cast<double>(nGateways) + M_PI / 2.0;
            }
            else
            {
                // Spread across the sweep INCLUSIVE of both ends, so the arc's extent is
                // the stated angle rather than one gateway short of it.
                const double f = (nGateways == 1)
                                     ? 0.5
                                     : static_cast<double>(k) /
                                           static_cast<double>(nGateways - 1);
                th = gwArcCentreDeg * M_PI / 180.0 - sweep / 2.0 + sweep * f;
            }
            dep.anchors[CA_TBL_TIER_868].push_back(
                {id++, Vector(R * std::cos(th), R * std::sin(th), gwH)});
        }
        }
    }
    else
    {
        const int gw = std::max(2, static_cast<int>(gwGrid));
        for (int gx = 0; gx < gw; ++gx)
        {
            for (int gy = 0; gy < gw; ++gy)
            {
                const double fx = -g + 2.0 * g * static_cast<double>(gx) / (gw - 1);
                const double fy = -g + 2.0 * g * static_cast<double>(gy) / (gw - 1);
                dep.anchors[CA_TBL_TIER_868].push_back({id++, Vector(fx, fy, gwH)});
            }
        }
    }

    // ---- Facility sites: where short-range infrastructure actually exists. --
    // Anchors are not spread uniformly over 9 km^2 -- that is not how anyone
    // deploys. They cluster at gates, huts, checkpoints. The gaps between sites
    // are what force the policy to decide rather than to always escalate.
    const double f = boxHalfM * 0.47;
    dep.facilities.push_back(Vector(-f, -f, 0.0));
    dep.facilities.push_back(Vector(f, -f, 0.0));
    dep.facilities.push_back(Vector(f, f, 0.0));
    dep.facilities.push_back(Vector(-f, f, 0.0));

    // ---- TRI-BAND GATEWAYS (topology 0, the default and the correct model) ----
    //
    // Every gateway is ONE fixed node carrying ALL THREE radios. The same id and the
    // same position appear in all three tier lists, so the setup loop below builds one
    // MAC per (gateway, tier) on that tier's channel -- three radios on one node --
    // and the provider admits that node on any of the three.
    //
    // This is what makes tier selection a RADIO decision instead of a destination
    // decision. It also makes tier-3 coverage honest: UWB reaches tens of metres, so a
    // fixed gateway offers it only to something standing next to the gateway. Closing
    // that gap is a FINDER's job -- a moving node that carries UWB to the casualty --
    // not a ring of UWB posts at a facility the casualty may never visit.
    if (topology == 0)
    {
        for (size_t i = 0; i < dep.anchors[CA_TBL_TIER_868].size(); ++i)
        {
            dep.anchors[CA_TBL_TIER_2P4].push_back(dep.anchors[CA_TBL_TIER_868][i]);
            dep.anchors[CA_TBL_TIER_UWB].push_back(dep.anchors[CA_TBL_TIER_868][i]);
        }
        return dep;
    }

    // ---- LEGACY per-tier anchors (topology 1) ------------------------------
    // Retained ONLY so results produced before 2026-09-07 stay reproducible. It models
    // three separate anchor populations, which the system does not have.
    // ---- Tiers 2 and 3: anchor rings at each site. -------------------------
    // Co-location is deliberate. If tier-2 and tier-3 coverage were in different
    // places, an escalation failure could be a siting artefact rather than a
    // decision error, and the comparison between the two refinement tiers would
    // be confounded.
    //
    // UWB gets MORE than three anchors per site by default. A fix needs three
    // ranges; with exactly three anchors that means needing all three, so at the
    // measured r80 per-anchor PDR of 0.885 the probability of solving is
    // 0.885^3 = 0.69 -- the tier would be intermittent at its own best range. With
    // five anchors, three-of-five succeeds far more often at the same per-link
    // PDR, and it is also the more realistic siting: a facility gate would have
    // several anchors, not exactly the mathematical minimum. This raises
    // reliability without inflating the assumed range, which is the distinction
    // that matters -- range is measured, redundancy is a design choice.
    // ---- A5: where tier 3 goes ---------------------------------------------
    //
    // sitingMode 0 (DEFAULT) co-locates tier 3 with tier 2, which is the siting every
    // existing result was produced under and the reason the co-location comment above
    // exists: if the two refinement tiers covered different ground, an escalation
    // failure could be a siting artefact rather than a decision error.
    //
    // sitingMode 1 is COMPLEMENTARY siting, and it is the thing A5 asks about. The
    // facility square has its corners at (+-f, +-f), radius f*sqrt(2). Rotating that
    // square by 45 degrees puts tier 3 at (+-f*sqrt(2), 0) and (0, +-f*sqrt(2)) -- the
    // SAME radius from the centre, in the GAPS BETWEEN the tier-2 clusters.
    //
    // Same radius is the point. Moving tier 3 outward would confound "different place"
    // with "further from the middle", and the comparison would no longer isolate
    // siting. The only thing that changes is the angle.
    //
    // Tier 2 stays where it is in both modes: the question is whether tier 3 should
    // complement tier 2, not whether the whole deployment should move. And
    // dep.facilities is untouched, because the walker dwells there -- moving it would
    // change the MOBILITY as well as the siting and the arms would no longer be
    // comparable.
    std::vector<Vector> uwbSites;
    if (sitingMode == 1)
    {
        const double fr = f * std::sqrt(2.0);
        uwbSites.push_back(Vector(fr, 0.0, 0.0));
        uwbSites.push_back(Vector(0.0, fr, 0.0));
        uwbSites.push_back(Vector(-fr, 0.0, 0.0));
        uwbSites.push_back(Vector(0.0, -fr, 0.0));
    }
    else
    {
        uwbSites = dep.facilities;
    }

    id = 201;
    uint32_t idUwb = 301;
    for (size_t s = 0; s < dep.facilities.size(); ++s)
    {
        const Vector c = dep.facilities[s];
        const Vector cUwb = (s < uwbSites.size()) ? uwbSites[s] : c;

        for (uint32_t k = 0; k < t2AnchorsPerSite; ++k)
        {
            const double th =
                2.0 * M_PI * static_cast<double>(k) / static_cast<double>(t2AnchorsPerSite) + 0.3;
            dep.anchors[CA_TBL_TIER_2P4].push_back(
                {id++,
                 Vector(c.x + t2TriadRadiusM * std::cos(th),
                        c.y + t2TriadRadiusM * std::sin(th),
                        6.0)});
        }

        for (uint32_t k = 0; k < uwbAnchorsPerSite; ++k)
        {
            const double th =
                2.0 * M_PI * static_cast<double>(k) / static_cast<double>(uwbAnchorsPerSite) + 0.3;
            dep.anchors[CA_TBL_TIER_UWB].push_back(
                {idUwb++,
                 Vector(cUwb.x + uwbTriadRadiusM * std::cos(th),
                        cUwb.y + uwbTriadRadiusM * std::sin(th),
                        2.5)});
        }
    }

    return dep;
}

/** Anchors of \p tier whose slant range to \p p is within the tier's nominal
 *  range. Pure geometry -- the ground-truth coverage label, not a physics
 *  outcome. */
uint32_t
VisibleCount(const Deployment& dep, CaTblTier tier, const Vector& p)
{
    uint32_t n = 0;
    for (size_t i = 0; i < dep.anchors[tier].size(); ++i)
    {
        const Vector& a = dep.anchors[tier][i].pos;
        const double d =
            std::sqrt((a.x - p.x) * (a.x - p.x) + (a.y - p.y) * (a.y - p.y) +
                      (a.z - p.z) * (a.z - p.z));
        if (d <= dep.nominalRangeM[tier])
        {
            ++n;
        }
    }
    return n;
}

/**
 * \brief Which physical quantity the operative NLOS tag is built from.
 *
 * THE DEFECT THIS EXISTS TO FIX
 * -----------------------------
 * The tag was `(nlosDb > 0.0) || (hiddenChannelState > 0)`. hiddenChannelState is
 * a 4-state Markov FADING REGIME, drawn as
 *
 *     newState = (uint32_t) Sample(Uniform(0.0, 3.99));
 *
 * so `> 0` selects three of four regimes. Measured over seed 7, all six arms, fix
 * cycles only: 41018 of 54174 links tagged, 75.7%, with 2 of 7338 cycles all-LOS.
 * That is not a mis-set threshold, it is a category error -- "fading regime 1" is
 * not "there is an obstruction". The consequences were all load-bearing:
 * nlosSigmaInflation (3.0x) became an effectively global variance multiplier on
 * the EKF, PCS term 3 was pinned near a constant, and the integrity gate's
 * all-LOS precondition was satisfiable essentially never.
 *
 * WHY THE MARKOV STATE IS NOT AN ACCEPTABLE PROXY EITHER
 * -----------------------------------------------------
 * A `severe` mode keyed on `hiddenChannelState >= 2` was considered and
 * deliberately NOT implemented. In this scenario the hidden state has no physical
 * effect whatsoever: every coupling from it into the channel is gated behind
 * FIDELITY_SDR_GRADE_REALISM -- the +6/+18 dB obstruction penalties
 * (lora-physics-engine.cc:352-355), the terrain reflection count (406-411) and
 * the delay-spread expansion (483) -- while LoRaPhysicsConfig::ForScenario()
 * sets FIDELITY_ULTRA_FAST_STATISTICAL (lora-physics-parameters.cc:625) and
 * nothing here overrides it. The state attenuates nothing, delays nothing and
 * biases nothing. Inflating measurement variance on it would be inflating noise
 * on the basis of a variable that does not affect the measurement -- the same
 * defect at half strength. The population is still emitted (nHiddenChannel) so
 * this judgement stays checkable rather than merely asserted.
 *
 * BEWARE when grepping: lora-physics-engine.cc:273 sets
 * `result.fidelitySchedulerState = FIDELITY_SDR_GRADE_REALISM` as an unrelated
 * telemetry constant. It is not m_config.fidelityMode and does not gate anything.
 *
 * NEITHER MODE PERTURBS THE CHANNEL. Both read fields off an already-computed
 * PhysicsResult and consume no RNG draw, so all sources share one channel
 * realisation and differ only in how the EKF weights it. That is what makes
 * legacy byte-identical and makes the comparison between sources clean.
 */
enum CaTblNlosSource
{
    //! (nlosDb > 0) || (hiddenChannelState > 0). The defect, retained as the
    //! default so results/baseline_v3 reproduces byte-identically.
    CA_TBL_NLOS_LEGACY = 0,
    //! The propagation layer's own determination: PhysicsResult::isNLOS, which is
    //! exactly `nlosDb > 0.01` and is set from ComputeNLOSPenaltyDb(). This is the
    //! same field the pre-flight probe already reports as nlos_frac, so the probe
    //! and the arms finally agree on what the word means.
    CA_TBL_NLOS_PROPAGATION
};

/**
 * \brief Build the operative NLOS tag from the two candidate populations.
 *
 * \param src         selected source
 * \param nlosDb      PhysicsResult::nlosDb -- the obstruction penalty in dB
 * \param propNlos    PhysicsResult::isNLOS -- the engine's own LOS/NLOS verdict
 * \param hiddenState PhysicsResult::hiddenChannelState -- the fading regime
 *
 * LEGACY IS SPELLED OUT LITERALLY, on nlosDb rather than on isNLOS, even though
 * the two are the same event in every practical case. isNLOS is `nlosDb > 0.01`,
 * so the two verdicts can differ when 0 < nlosDb <= 0.01 dB -- reachable in
 * principle because both ComputeNLOSPenaltyDb() draws are truncated at zero.
 * Byte-identical reproduction of results/baseline_v3 is the acceptance test for
 * this whole change, and it should hold by construction rather than by an
 * argument about how improbable a sub-0.01 dB obstruction is.
 */
inline bool
CaTblNlosFlag(CaTblNlosSource src, double nlosDb, bool propNlos, uint32_t hiddenState)
{
    if (src == CA_TBL_NLOS_PROPAGATION)
    {
        return propNlos;
    }
    return (nlosDb > 0.0) || (hiddenState > 0);
}

/**
 * \brief CaTblPhyProvider backed by the three real physics engines.
 *
 * This is the wiring the S4 MAC was written against and never had: until now the
 * only implementation was StubPhy in ca_tbl_packet_validator.cc, which returns a
 * fixed power for every link. Everything the validator reports about contention is
 * therefore true of a channel where all links are identical -- which is why it
 * measured 1129 collision events and ZERO captures. Capture arbitration needs a
 * power spread to arbitrate over, and a stub cannot supply one.
 *
 * Three modelling decisions here are approximations, and each is a limitation of
 * the result rather than of the code:
 *
 * 1. RECIPROCITY. The engines expose ProcessTransmission(node, gateway, ...) and
 *    are directional in their argument names but not in their physics: the two-ray
 *    crossover is dc = 4*pi*txH*rxH/lambda, which is symmetric in the two heights,
 *    and shadowing/foliage/terrain are keyed on the link's endpoints. An anchor ->
 *    node beacon is therefore evaluated with the same call as a node -> anchor
 *    poll. This is standard for a link-budget model and it is stated, not assumed
 *    away: it means antenna-pattern asymmetry and TX-power asymmetry between the
 *    two ends are NOT modelled.
 *
 * 2. LINKS EXIST BETWEEN A FIELD NODE AND AN ANCHOR, AND NOWHERE ELSE. Field
 *    nodes are registered as nodes on the engines and anchors as gateways on
 *    their own tier's engine, and Evaluate() resolves exactly one endpoint of
 *    each kind. Two classes of pair therefore have no evaluable link and get
 *    "not received":
 *
 *      ANCHOR-TO-ANCHOR. Every anchor is a HIDDEN TERMINAL to every other
 *      anchor. Their CAD never detects each other, so they transmit into each
 *      other freely and collisions at the node are decided purely by arrival
 *      overlap and capture. That is a harsher contention regime than a real
 *      deployment with inter-anchor audibility, so beacon loss here is an upper
 *      bound, not an estimate.
 *
 *      NODE-TO-NODE, i.e. tag to tag. This one is reachable rather than
 *      impossible: ProcessTransmission auto-registers an unknown receiver as a
 *      gateway with fineTimestamp=false (lora-physics-engine.cc:298), which is
 *      the correct hardware class for a tag, so the link budget itself would
 *      evaluate. It is NOT enabled here because nothing would be entitled to
 *      read the estimatedRangeM it produced: a tag-to-tag range enters the
 *      solution as a RELAYED measurement, and the relayed-ranging error model
 *      that says how such a measurement should be weighted is a written,
 *      reviewed precondition of C5 that does not exist yet. Enabling the link
 *      without it would produce ranges that look usable and are not attributable.
 *      This is the C5 gate, not a shortcut.
 *
 * 2a. MULTI-NODE. Until 2026-09-05 this class held a single m_nodeId and a single
 *    m_nodePos, and Evaluate() returned early for any frame with neither endpoint
 *    equal to it. Because that early return happens BELOW the MAC, a peer tag's
 *    frames were counted as sent, lost nothing to the duty governor, to CAD or to
 *    collision -- and no anchor ever received one. Measured at nTags=2: 162
 *    ESC_REQUESTs sent, zero dropped, zero delivered. The peer was a transmitter
 *    that could not be heard, which is a silent failure of exactly the kind that
 *    reads as a working mechanism. A position per node is what fixes it.
 *
 * 3. HOPS. hopsTraversed is forwarded to the engines, which previously always
 *    received 0 from this scenario. Any per-hop degradation the engines model now
 *    actually engages on relayed frames.
 */
class EnginePhyProvider : public CaTblPhyProvider
{
  public:
    EnginePhyProvider(LoRaPhysicsEngine& e868,
                      LoRa2p4PhysicsEngine& e2p4,
                      LoRaUWBPhysicsEngine& eUwb,
                      const TierPhy* phy,
                      uint16_t fieldNodeId)
        : m_e868(e868),
          m_e2p4(e2p4),
          m_eUwb(eUwb),
          m_phy(phy),
          m_nodeId(fieldNodeId),
          m_nodePos(0.0, 0.0, 0.0),
          m_evaluations(0),
          m_skipped(0),
          m_nlosSource(CA_TBL_NLOS_LEGACY)
    {
    }

    //! CaTblLinkOutcome::nlos is currently WRITE-ONLY -- nothing in the MAC or the
    //! channel reads it -- so this cannot change any result. It is routed through
    //! the same selector anyway so the field does not sit here quietly
    //! contradicting the definition the policy uses, which is how a dead field
    //! becomes a live defect the day someone wires it up.
    void SetNlosSource(CaTblNlosSource src) { m_nlosSource = src; }

    //! A gateway carries ALL THREE radios, so a node id maps to a SET of tiers and
    //! not to one. Storing a single tier per id was what forced the three-population
    //! model: the last registration won and the node became "a UWB anchor" or "a
    //! 2.4 GHz anchor" rather than a gateway that has both.
    void RegisterAnchor(CaTblTier tier, uint16_t id, const Vector& p)
    {
        m_anchorTiers[id] |= static_cast<uint8_t>(1u << static_cast<int>(tier));
        m_anchorPos[id] = p;
    }

    void SetNodePosition(const Vector& p) { m_nodePos = p; }

    //! SY-5. The measured node's injected link-budget faults, set each cycle by the
    //! scenario and 0, 0 outside the fault window. A transmit-power drop lowers what
    //! the node sends; an antenna loss lowers what it sends AND what it receives, since
    //! a detuned antenna is reciprocal.
    void SetNodeFaultDb(double txDropDb, double antennaLossDb)
    {
        m_faultTxDb = txDropDb + antennaLossDb;
        m_faultRxDb = antennaLossDb;
    }
    uint64_t GetFaultTxLinks() const { return m_faultTxLinks; }
    uint64_t GetFaultRxLinks() const { return m_faultRxLinks; }

    //! NW-0. Under mesh links ("all") every registered pair on a tier both carry is a
    //! link -- tag-tag, gateway-gateway, finder-gateway -- not only field node to anchor.
    void SetMeshAll(bool m) { m_meshAll = m; }
    uint64_t GetMeshEvaluations() const { return m_meshEvals; }

    //! Register an additional field node (a peer tag). The measured tag keeps its
    //! own scalar id/position rather than joining this map, so that with nTags=1
    //! the map stays empty and every branch below behaves as it did before.
    void RegisterPeerNode(uint16_t id, const Vector& p) { m_peerPos[id] = p; }

    void SetPeerPosition(uint16_t id, const Vector& p) { m_peerPos[id] = p; }

    //! A FINDER is a gateway that moves. Same link evaluation, new position each cycle.
    void SetAnchorPosition(uint16_t id, const Vector& p) { m_anchorPos[id] = p; }

    CaTblLinkOutcome Evaluate(uint16_t txId,
                              uint16_t rxId,
                              CaTblTier tier,
                              double txPowerDbm,
                              uint8_t hopsTraversed) override
    {
        CaTblLinkOutcome o;

        // Exactly one endpoint must be a field node and the other an anchor --
        // see limitation 2 above. The measured tag is checked first and by
        // scalar compare, so with no peers registered this is the same test,
        // in the same order, that it was before peers existed.
        uint16_t anchor = 0;
        uint16_t node = m_nodeId;
        const Vector* nodePos = &m_nodePos;
        if (m_meshAll && !StarPair(txId, rxId, tier))
        {
            return EvaluateMeshPair(txId, rxId, tier, txPowerDbm, hopsTraversed);
        }
        if (txId == m_nodeId)
        {
            anchor = rxId;
            if (m_faultTxDb != 0.0)
            {
                txPowerDbm -= m_faultTxDb; // SY-5
                ++m_faultTxLinks;
            }
        }
        else if (rxId == m_nodeId)
        {
            anchor = txId;
            if (m_faultRxDb != 0.0)
            {
                txPowerDbm -= m_faultRxDb; // SY-5: the link is evaluated reciprocally
                ++m_faultRxLinks;
            }
        }
        else if (!m_peerPos.empty())
        {
            // A peer tag transmitting to an anchor. Exactly one endpoint may be a
            // peer: if both are, the pair is tag-to-tag and has no evaluable link.
            std::map<uint16_t, Vector>::const_iterator pTx = m_peerPos.find(txId);
            std::map<uint16_t, Vector>::const_iterator pRx = m_peerPos.find(rxId);
            const bool txIsPeer = (pTx != m_peerPos.end());
            const bool rxIsPeer = (pRx != m_peerPos.end());
            if (txIsPeer && !rxIsPeer)
            {
                node = txId;
                nodePos = &pTx->second;
                anchor = rxId;
            }
            else if (rxIsPeer && !txIsPeer)
            {
                node = rxId;
                nodePos = &pRx->second;
                anchor = txId;
            }
            else
            {
                ++m_skipped;
                return o;
            }
        }
        else
        {
            ++m_skipped;
            return o;
        }

        // The anchor must belong to THIS tier. The channel iterates every
        // registered MAC on a completed transmission, so without this an 868
        // frame would be handed to the 2.4 GHz engine with a gateway id that
        // engine has never seen registered.
        std::map<uint16_t, uint8_t>::const_iterator at = m_anchorTiers.find(anchor);
        if (at == m_anchorTiers.end() ||
            (at->second & static_cast<uint8_t>(1u << static_cast<int>(tier))) == 0)
        {
            ++m_skipped;
            return o;
        }
        const Vector& anchorPos = m_anchorPos[anchor];
        const Time now = Simulator::Now();
        const int t = static_cast<int>(tier);
        ++m_evaluations;

        if (tier == CA_TBL_TIER_868)
        {
            PhysicsResult r = m_e868.ProcessTransmission(node,
                                                         anchor,
                                                         *nodePos,
                                                         anchorPos,
                                                         txPowerDbm,
                                                         m_phy[t].sf,
                                                         m_phy[t].bandwidthHz,
                                                         hopsTraversed,
                                                         now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        else if (tier == CA_TBL_TIER_2P4)
        {
            PhysicsResult2p4 r = m_e2p4.ProcessTransmission(node,
                                                            anchor,
                                                            *nodePos,
                                                            anchorPos,
                                                            txPowerDbm,
                                                            m_phy[t].sf,
                                                            m_phy[t].bandwidthHz,
                                                            hopsTraversed,
                                                            now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        else
        {
            PhysicsResultUWB r = m_eUwb.ProcessTransmission(node,
                                                            anchor,
                                                            *nodePos,
                                                            anchorPos,
                                                            txPowerDbm,
                                                            m_phy[t].sf,
                                                            m_phy[t].bandwidthHz,
                                                            hopsTraversed,
                                                            now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        return o;
    }

    /**
     * \brief L.1.4. The one frame for which an ANCHOR-TO-ANCHOR link exists.
     *
     * Limitation 2 above says anchor-to-anchor pairs have no evaluable link, and for
     * every other frame that stays true -- including the gateways' own beacons, so the
     * hidden-terminal upper bound on beacon loss (TECH_DEBT sect 3.5) is untouched.
     * Opening the pair class for beacons would be a different change: DISCOVERY is
     * flooded, so every gateway would start rebroadcasting every other gateway's beacon.
     *
     * The GRANT_GOSSIP frame is the exception because Network sect 6 is anchor-to-anchor
     * traffic by definition, and a gossip nobody can hear is a mechanism that cannot
     * fire. Only gateways registered through AddGossipAnchor() qualify -- finders are
     * anchors to the provider but carry no coordinator, and a finder-to-gateway link
     * would be a new pair class this change has no business opening.
     *
     * The link is the tier's own engine, the transmitting gateway placed in the engine's
     * node slot. Both ends carry their surveyed 15 m mast height through position z,
     * which is what the engines read for height, so no field-node height leaks in; and
     * the node slot's velocity is zero, which is right for the one immobile class.
     *
     * Empty set (the default) or any other frame: exactly Evaluate().
     */
    CaTblLinkOutcome EvaluateFrame(uint16_t txId,
                                   uint16_t rxId,
                                   CaTblTier tier,
                                   double txPowerDbm,
                                   const CaTblHeader& hdr) override
    {
        if (hdr.GetMsgType() == CA_TBL_MSG_GRANT_GOSSIP && !m_gossipAnchors.empty() &&
            m_gossipAnchors.count(txId) != 0 && m_gossipAnchors.count(rxId) != 0)
        {
            return EvaluateAnchorPair(txId, rxId, tier, txPowerDbm, hdr.GetHopsTraversed());
        }
        return Evaluate(txId, rxId, tier, txPowerDbm, hdr.GetHopsTraversed());
    }

    //! L.1.4. Admit a gateway to the anchor-to-anchor gossip link class. Called only
    //! under --anchorGrantGossipS > 0.
    void AddGossipAnchor(uint16_t id) { m_gossipAnchors.insert(id); }

    double SensitivityDbm(CaTblTier tier, uint8_t sf) override
    {
        // codingRateDenom 5 == CR 4/5, matching m_codingRate = 1 elsewhere.
        if (tier == CA_TBL_TIER_868)
        {
            return m_e868.GetSensitivityThreshold(sf, 5);
        }
        if (tier == CA_TBL_TIER_2P4)
        {
            return m_e2p4.GetSensitivityThreshold(sf, 5);
        }
        return m_eUwb.GetSensitivityThreshold(sf, 5);
    }

    uint64_t GetEvaluations() const { return m_evaluations; }
    uint64_t GetSkipped() const { return m_skipped; }

  private:
    //! NW-0. Whether (tx, rx) is a pair the star model already evaluates: exactly one
    //! endpoint a field node (the measured node or a peer) and the other an anchor that
    //! carries the tier. Those keep their exact evaluation, in the same order.
    bool StarPair(uint16_t txId, uint16_t rxId, CaTblTier tier) const
    {
        const bool txField = (txId == m_nodeId) || m_peerPos.count(txId) != 0;
        const bool rxField = (rxId == m_nodeId) || m_peerPos.count(rxId) != 0;
        if (txField == rxField)
        {
            return false;
        }
        const uint16_t anchor = txField ? rxId : txId;
        std::map<uint16_t, uint8_t>::const_iterator at = m_anchorTiers.find(anchor);
        return at != m_anchorTiers.end() &&
               (at->second & static_cast<uint8_t>(1u << static_cast<int>(tier))) != 0;
    }

    //! NW-0. Any other registered pair on a tier both carry, the transmitter in the
    //! engine's node slot (as EvaluateAnchorPair does). Field nodes carry every tier.
    CaTblLinkOutcome EvaluateMeshPair(uint16_t txId, uint16_t rxId, CaTblTier tier,
                                      double txPowerDbm, uint8_t hopsTraversed)
    {
        CaTblLinkOutcome o;
        const uint8_t bit = static_cast<uint8_t>(1u << static_cast<int>(tier));
        auto where = [&](uint16_t id, Vector& p) -> bool {
            if (id == m_nodeId)
            {
                p = m_nodePos;
                return true;
            }
            std::map<uint16_t, Vector>::const_iterator pp = m_peerPos.find(id);
            if (pp != m_peerPos.end())
            {
                p = pp->second;
                return true;
            }
            std::map<uint16_t, uint8_t>::const_iterator at = m_anchorTiers.find(id);
            if (at != m_anchorTiers.end() && (at->second & bit) != 0)
            {
                p = m_anchorPos[id];
                return true;
            }
            return false;
        };
        Vector txPos, rxPos;
        if (txId == rxId || !where(txId, txPos) || !where(rxId, rxPos))
        {
            ++m_skipped;
            return o;
        }
        if (txId == m_nodeId && m_faultTxDb != 0.0)
        {
            txPowerDbm -= m_faultTxDb; // SY-5, as on the star pairs
        }
        else if (rxId == m_nodeId && m_faultRxDb != 0.0)
        {
            txPowerDbm -= m_faultRxDb;
        }
        const Time now = Simulator::Now();
        const int t = static_cast<int>(tier);
        ++m_evaluations;
        ++m_meshEvals;
        if (tier == CA_TBL_TIER_868)
        {
            PhysicsResult r = m_e868.ProcessTransmission(txId, rxId, txPos, rxPos, txPowerDbm,
                                                         m_phy[t].sf, m_phy[t].bandwidthHz,
                                                         hopsTraversed, now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        else if (tier == CA_TBL_TIER_2P4)
        {
            PhysicsResult2p4 r = m_e2p4.ProcessTransmission(txId, rxId, txPos, rxPos,
                                                            txPowerDbm, m_phy[t].sf,
                                                            m_phy[t].bandwidthHz,
                                                            hopsTraversed, now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        else
        {
            PhysicsResultUWB r = m_eUwb.ProcessTransmission(txId, rxId, txPos, rxPos,
                                                            txPowerDbm, m_phy[t].sf,
                                                            m_phy[t].bandwidthHz,
                                                            hopsTraversed, now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        return o;
    }

    //! L.1.4. Gateway-to-gateway link on \p tier, for the gossip frame only. Same
    //! engine calls as Evaluate(), with the transmitting gateway in the node slot.
    CaTblLinkOutcome EvaluateAnchorPair(uint16_t txId,
                                        uint16_t rxId,
                                        CaTblTier tier,
                                        double txPowerDbm,
                                        uint8_t hopsTraversed)
    {
        CaTblLinkOutcome o;
        const uint8_t bit = static_cast<uint8_t>(1u << static_cast<int>(tier));
        std::map<uint16_t, uint8_t>::const_iterator tt = m_anchorTiers.find(txId);
        std::map<uint16_t, uint8_t>::const_iterator rt = m_anchorTiers.find(rxId);
        if (txId == rxId || tt == m_anchorTiers.end() || rt == m_anchorTiers.end() ||
            (tt->second & bit) == 0 || (rt->second & bit) == 0)
        {
            ++m_skipped;
            return o;
        }
        const Vector txPos = m_anchorPos[txId];
        const Vector rxPos = m_anchorPos[rxId];
        const Time now = Simulator::Now();
        const int t = static_cast<int>(tier);
        ++m_evaluations;
        if (tier == CA_TBL_TIER_868)
        {
            PhysicsResult r = m_e868.ProcessTransmission(txId, rxId, txPos, rxPos, txPowerDbm,
                                                         m_phy[t].sf, m_phy[t].bandwidthHz,
                                                         hopsTraversed, now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        else if (tier == CA_TBL_TIER_2P4)
        {
            PhysicsResult2p4 r = m_e2p4.ProcessTransmission(txId, rxId, txPos, rxPos,
                                                            txPowerDbm, m_phy[t].sf,
                                                            m_phy[t].bandwidthHz,
                                                            hopsTraversed, now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        else
        {
            PhysicsResultUWB r = m_eUwb.ProcessTransmission(txId, rxId, txPos, rxPos,
                                                            txPowerDbm, m_phy[t].sf,
                                                            m_phy[t].bandwidthHz,
                                                            hopsTraversed, now);
            o.received = r.packetReceived;
            o.rxPowerDbm = r.rxPowerDbm;
            o.snrDb = r.snr_db;
            o.nlos = CaTblNlosFlag(m_nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
            o.estimatedRangeM = r.estimatedRangeM;
        }
        return o;
    }

    LoRaPhysicsEngine& m_e868;
    LoRa2p4PhysicsEngine& m_e2p4;
    LoRaUWBPhysicsEngine& m_eUwb;
    const TierPhy* m_phy;
    uint16_t m_nodeId;
    Vector m_nodePos;
    //! Peer field nodes. EMPTY unless --nTags>1, which is the default-off flag
    //! this mechanism sits behind: with it empty, Evaluate() takes the same
    //! branches it always did.
    std::map<uint16_t, Vector> m_peerPos;
    //! id -> bitmask of tiers this node has a radio for.
    std::map<uint16_t, uint8_t> m_anchorTiers;
    std::map<uint16_t, Vector> m_anchorPos;
    uint64_t m_evaluations;
    uint64_t m_skipped;
    CaTblNlosSource m_nlosSource;
    //! L.1.4. Gateways admitted to the gossip link class. EMPTY unless
    //! --anchorGrantGossipS > 0, and with it empty EvaluateFrame() is Evaluate().
    std::set<uint16_t> m_gossipAnchors;
    //! SY-5. 0 unless the link-fault flags are on and the window is open.
    double m_faultTxDb = 0.0;
    double m_faultRxDb = 0.0;
    uint64_t m_faultTxLinks = 0;
    uint64_t m_faultRxLinks = 0;
    //! NW-0. False unless --meshLinks=all.
    bool m_meshAll = false;
    uint64_t m_meshEvals = 0;
};

/**
 * \brief Self-rescheduling beacon source for one anchor.
 *
 * Beacon phase is drawn from the RNG rather than spread on a fixed lattice. An
 * evenly-spaced lattice would make simultaneous transmission impossible by
 * construction, and collisions would then be an artefact of the schedule rather
 * than an emergent property -- which is the thing claim 6 is supposed to be
 * measuring. Real unsynchronised anchors have random phase; so do these.
 */
struct BeaconDriver
{
    CaTblMac* mac;
    CaTblTier tier;
    double periodS;
    double jitterMaxS;
    double untilS;
    Ptr<UniformRandomVariable> rng;

    void Fire()
    {
        mac->Send(CA_TBL_MSG_DISCOVERY, tier, 1, 0);
        const double next = periodS + rng->GetValue(0.0, jitterMaxS);
        if (Simulator::Now().GetSeconds() + next < untilS)
        {
            Simulator::Schedule(Seconds(next), &BeaconDriver::Fire, this);
        }
    }
};

/**
 * \brief C3. An anchor asking "where is target N", on a period.
 *
 * Mirrors BeaconDriver deliberately: same shape, same jitter, same self-rescheduling.
 * The only difference is that it carries the identity being searched for, which is
 * what makes the flood a SEARCH rather than a broadcast.
 */
struct QueryDriver
{
    CaTblMac* mac;
    CaTblTier tier;
    double periodS;
    double jitterMaxS;
    double untilS;
    uint8_t manifestId;
    Ptr<UniformRandomVariable> rng;
    //! NW-9 (--queryV2): send the spec's QUERY, for this list, at ROUTINE priority.
    bool v2 = false;
    std::vector<uint8_t> manifests;
    uint8_t priority = 0; //!< NW-4: 1 = URGENT

    void Fire()
    {
        if (v2)
        {
            mac->SendQueryV2(tier, manifests, priority);
        }
        else
        {
            mac->SendQuery(tier, manifestId);
        }
        const double next = periodS + rng->GetValue(0.0, jitterMaxS);
        if (Simulator::Now().GetSeconds() + next < untilS)
        {
            Simulator::Schedule(Seconds(next), &QueryDriver::Fire, this);
        }
    }
};

/**
 * \brief C4. A peer asking for the UWB slot on a period, as SYNTHETIC LOAD.
 *
 * Exists because a peer has no policy and therefore never escalates on its own, so
 * without it the arbiter is handed one requester and never has to choose. It is a
 * timer, not a decision: the degradation and battery terms it carries are stated
 * constants rather than measurements, and nothing about them reflects a real need.
 *
 * Use it to exercise deny and pre-empt. Do not read it as traffic a real second
 * target would generate.
 */
struct PeerRequestDriver
{
    CaTblMac* mac;
    CaTblTier tier;
    double periodS;
    double jitterMaxS;
    double untilS;
    uint8_t degradation;
    uint8_t battery;
    Ptr<UniformRandomVariable> rng;

    void Fire()
    {
        mac->SetRequestTerms(degradation, battery);
        mac->Send(CA_TBL_MSG_ESC_REQUEST, tier, 0, 0);
        const double next = periodS + rng->GetValue(0.0, jitterMaxS);
        if (Simulator::Now().GetSeconds() + next < untilS)
        {
            Simulator::Schedule(Seconds(next), &PeerRequestDriver::Fire, this);
        }
    }
};

/** C4. Quantise a [0,1] term to a byte, saturating at both ends. */
inline uint8_t
CaTblQuantise(double x)
{
    if (!(x > 0.0))
    {
        return 0;
    }
    if (x >= 1.0)
    {
        return 255;
    }
    return static_cast<uint8_t>(x * 255.0 + 0.5);
}

/** DC-11. A range's source kind from the id classes: 101-499 surveyed gateways (and the
 *  legacy topology's anchors), 501+ finders, anything else a peer tag. */
static uint8_t
CaTblSourceKindOf(uint32_t id)
{
    if (id >= 501)
    {
        return CA_TBL_SRC_FINDER;
    }
    if (id >= 101)
    {
        return CA_TBL_SRC_SURVEYED_GATEWAY;
    }
    return CA_TBL_SRC_PEER;
}

/**
 * \brief MC-1. The C4 terms as they stand when a request is SENT.
 *
 * The end-of-cycle update computes T_deg from the last five PCS values and T_batt from
 * the energy spent, and it runs AFTER the cycle's request has gone, so by default every
 * request carries the previous cycle's terms. These are the same arithmetic, in the same
 * order, on the history with this cycle's PCS appended. With fewer than three values the
 * end-of-cycle update leaves T_deg where it was, and so does this.
 */
static double
CaTblDegradationWith(const std::deque<double>& hist, double pcsNow, double prev)
{
    std::deque<double> h = hist;
    h.push_back(pcsNow);
    while (h.size() > 5)
    {
        h.pop_front();
    }
    if (h.size() < 3)
    {
        return prev;
    }
    const size_t np = h.size();
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (size_t i = 0; i < np; ++i)
    {
        const double x = static_cast<double>(i);
        sx += x;
        sy += h[i];
        sxx += x * x;
        sxy += x * h[i];
    }
    const double den = static_cast<double>(np) * sxx - sx * sx;
    const double slope = (den != 0.0) ? ((static_cast<double>(np) * sxy - sx * sy) / den) : 0.0;
    return 1.0 - std::exp(-std::max(0.0, -slope));
}

static double
CaTblBatteryUsedFrac(double totalJ, double capJ)
{
    return (capJ > 0.0) ? std::min(1.0, totalJ / capJ) : 0.0;
}

//! NW-5. The battery term a request carries: the fraction SPENT by default (C4), the
//! fraction REMAINING under --arbitrationV2 (Network sect 5's 1/battery_remaining).
static double
ArbBatteryTerm(bool remaining, double usedFrac)
{
    return remaining ? 1.0 - usedFrac : usedFrac;
}

// SY-2. --radioBasis=lr1121: every field-node and gateway-gossip energy account takes the
// LR1121 profiles for 868 MHz and 2.4 GHz (UWB stays DW3000). A file-scope global, like
// --terrainPerTierFreq's and --uwbNlosModel's: written once after cmd.Parse(), read where
// an account is created.
static bool g_radioBasisLr1121 = false;

static void
ApplyRadioBasis(CaTblEnergyModel& e)
{
    if (g_radioBasisLr1121)
    {
        e.SetProfile(CA_TBL_TIER_868, RadioPowerProfile::Lr1121_868());
        e.SetProfile(CA_TBL_TIER_2P4, RadioPowerProfile::Lr1121_2p4());
    }
}

/**
 * \brief L.1.4 / Network sect 6 -- anchors gossip the UWB slots they hold, and on an
 *        overlap for the same field node the LOWER node id defers.
 *
 * WHAT WAS MISSING. CaTblGrantCoordinator and its suite existed; nothing constructed one.
 * With --sendHandshakeFrames every gateway that hears an ESC_REQUEST grants it (C1), and
 * on tier 1 that is several at once: in the arc deployment (NC2's flags, seed 7) most
 * ESC_REQUESTs at HEAD reached three or four gateways in the same instant, and their
 * answering grants are visible colliding at the field node.
 *
 * WHICH RADIO. Tier 1, 868 MHz. Every gateway carries all three radios, but only 868 MHz
 * closes a gateway-to-gateway link: gateways stand hundreds of metres to kilometres
 * apart, UWB reaches tens of metres, 2.4 GHz a few hundred. The frame is GRANT_GOSSIP,
 * LOW access category (the spec's "low priority CW"), one hop, unacknowledged. It goes
 * through the gateway's own CAD and its 10% WPC Table-II access-point duty governor, in
 * the same sliding window as its beacons and grants -- nothing is exempted.
 *
 * HOW IT IS CHARGED. Gateways had NO energy account before this: CaTblMac charges no TX
 * or RX joules and SetEnergyModel() is unset on every MAC, so gateway beacons have always
 * been free. That is left alone. The gossip gets its own per-gateway tier-1 account,
 * charged each cycle from what the MAC measured -- TX airtime of frames that actually
 * went on air, one CAD probe per CAD the MAC actually ran (sent + busy), and RX airtime of
 * gossip delivered to the gateway. It is reported on stdout, never added to the field
 * node's joules. The field node pays for gossip only as it already pays for any frame it
 * hears on a radio that is awake (AccountListen per delivered frame), so a node whose
 * tier-1 radio sleeps accrues nothing for it.
 *
 * WHEN. Triggered from the per-cycle loop: every periodS each gateway schedules one
 * gossip at an independent uniform offset inside the coming cycle. The offset is not
 * optional -- all gateways would otherwise CAD at the same instant, find the channel
 * idle, and collide at every receiver. The payload is composed at SEND time from the
 * grants still held then, so a grant yielded earlier in the same round is not repeated.
 *
 * WHAT IT DOES NOT DO. The two anchors that hear an ESC_REQUEST in the same instant both
 * grant it; nothing can prevent that without a consensus round the spec rules out. The
 * double grant is settled when their gossip crosses, which bounds it to the gossip
 * period -- and only if the period is shorter than the slot window (CaTblMacConfig::
 * arbSlotHoldS), since a grant that has ended is never gossiped. The yield is local: no
 * revoke frame exists in the taxonomy and the field node does not yet act on a grant
 * (the tier change commits in Step()), so what yielding changes is which anchor holds
 * the slot, which anchor answers the node's next request, and the C4 arbiter's state.
 */
struct GrantGossip
{
    GrantGossip(double periodS, double dtS, const std::string& logPath, const TierPhy* phy);
    void AttachAnchor(uint16_t id, CaTblTier tier, CaTblMac* mac, EnginePhyProvider& provider);
    //! NW-6. A finder joins as an arbiter that gossips: its radios share a coordinator and
    //! its tier-1 radio gossips. It carries no energy account (finders never have), and
    //! its links to gateways are NW-0's mesh links, not the gateway-mast pair class.
    void AttachFinder(uint16_t id, CaTblTier tier, CaTblMac* mac);
    void Cycle(double t, Ptr<UniformRandomVariable> rng);
    void Report(std::ostream& os, double durationS) const;

    double periodS;
    double dtS;
    double nextS;
    double windowS = 10.0; //!< the slot hold it reports (--arbSlotHoldS)
    uint64_t rounds = 0;
    const TierPhy* phy;
    std::ofstream log;
    std::deque<CaTblGrantCoordinator> coords; //!< deque: attached pointers must not move
    std::map<uint16_t, CaTblGrantCoordinator*> byId;
    std::vector<CaTblMac*> all;   //!< every MAC of every gateway
    std::vector<CaTblMac*> tier1; //!< one per gateway: the radio the gossip rides
    std::vector<CaTblMac*> finder1; //!< NW-6: one per finder, unbilled
    std::deque<CaTblEnergyModel> energy; //!< parallel to tier1
    std::vector<double> txChargedS;
    std::vector<double> rxChargedS;
    std::vector<uint64_t> framesCharged;
    std::vector<uint64_t> cadCharged;
};

__attribute__((noinline))
GrantGossip::GrantGossip(double p, double dt, const std::string& logPath, const TierPhy* ph)
    : periodS(p),
      dtS(dt),
      nextS(p),
      phy(ph)
{
    log.open(logPath.c_str());
    if (!log.is_open())
    {
        std::cerr << "FATAL: cannot open " << logPath
                  << " for writing -- does --outdir exist?\n";
        std::exit(1);
    }
    log << "t,anchorId,targetId,otherAnchorId,source,verdict,action,windowStartS,windowEndS\n";
}

__attribute__((noinline)) void
GrantGossip::AttachAnchor(uint16_t id, CaTblTier tier, CaTblMac* mac, EnginePhyProvider& provider)
{
    std::map<uint16_t, CaTblGrantCoordinator*>::iterator it = byId.find(id);
    if (it == byId.end())
    {
        coords.emplace_back(id);
        it = byId.insert(std::make_pair(id, &coords.back())).first;
    }
    mac->SetGrantCoordinator(it->second);
    mac->SetGrantLog(&log);
    all.push_back(mac);
    if (tier == CA_TBL_TIER_868)
    {
        tier1.push_back(mac);
        energy.emplace_back();
        ApplyRadioBasis(energy.back()); // SY-2
        energy.back().SetLoRaPhy(CA_TBL_TIER_868, phy[0].sf, phy[0].bandwidthHz, 1);
        txChargedS.push_back(0.0);
        rxChargedS.push_back(0.0);
        framesCharged.push_back(0);
        cadCharged.push_back(0);
        provider.AddGossipAnchor(id);
    }
}

__attribute__((noinline)) void
GrantGossip::AttachFinder(uint16_t id, CaTblTier tier, CaTblMac* mac)
{
    std::map<uint16_t, CaTblGrantCoordinator*>::iterator it = byId.find(id);
    if (it == byId.end())
    {
        coords.emplace_back(id);
        it = byId.insert(std::make_pair(id, &coords.back())).first;
    }
    mac->SetGrantCoordinator(it->second);
    mac->SetGrantLog(&log);
    all.push_back(mac);
    if (tier == CA_TBL_TIER_868)
    {
        finder1.push_back(mac);
    }
}

__attribute__((noinline)) void
GrantGossip::Cycle(double t, Ptr<UniformRandomVariable> rng)
{
    // Charge what went on air, and what arrived, during the slice that just ran.
    for (size_t i = 0; i < tier1.size(); ++i)
    {
        const CaTblMsgCounters& c = tier1[i]->GetCounters(CA_TBL_MSG_GRANT_GOSSIP);
        const uint64_t cadNow = c.sent + c.cadBusyEvents;
        const double rxNow = tier1[i]->GetGossipRxAirtimeS();
        if (c.sent > framesCharged[i])
        {
            energy[i].AccountTx(CA_TBL_TIER_868, c.airtimeS - txChargedS[i],
                                c.sent - framesCharged[i]);
        }
        for (uint64_t k = cadCharged[i]; k < cadNow; ++k)
        {
            energy[i].AccountCad(CA_TBL_TIER_868);
        }
        if (rxNow > rxChargedS[i])
        {
            energy[i].AccountListen(CA_TBL_TIER_868, rxNow - rxChargedS[i]);
        }
        txChargedS[i] = c.airtimeS;
        framesCharged[i] = c.sent;
        cadCharged[i] = cadNow;
        rxChargedS[i] = rxNow;
    }

    if (t + 1e-9 < nextS)
    {
        return;
    }
    nextS = t + periodS;
    ++rounds;
    const double spread = std::min(dtS, periodS);
    for (size_t i = 0; i < tier1.size(); ++i)
    {
        Simulator::Schedule(Seconds(rng->GetValue(0.0, spread)),
                            &CaTblMac::SendGrantGossip,
                            tier1[i],
                            CA_TBL_TIER_868);
    }
    for (size_t i = 0; i < finder1.size(); ++i)
    {
        Simulator::Schedule(Seconds(rng->GetValue(0.0, spread)),
                            &CaTblMac::SendGrantGossip,
                            finder1[i],
                            CA_TBL_TIER_868);
    }
}

__attribute__((noinline)) void
GrantGossip::Report(std::ostream& os, double durationS) const
{
    os << "    L.1.4 grant tie-break: gossip every " << std::fixed << std::setprecision(1)
       << periodS << " s on tier 1, grant window " << windowS << " s, " << rounds
       << " rounds, " << tier1.size() << " gateways\n";
    for (size_t i = 0; i < tier1.size(); ++i)
    {
        const uint16_t id = tier1[i]->GetNodeId();
        const CaTblMsgCounters& c = tier1[i]->GetCounters(CA_TBL_MSG_GRANT_GOSSIP);
        uint64_t heard = 0, withheld = 0, yielded = 0, skipped = 0;
        for (size_t m = 0; m < all.size(); ++m)
        {
            if (all[m]->GetNodeId() != id)
            {
                continue;
            }
            heard += all[m]->GetGossipItemsHeard();
            withheld += all[m]->GetGrantsWithheld();
            yielded += all[m]->GetGrantsYielded();
            skipped += all[m]->GetGossipSkippedEmpty();
        }
        const CaTblGrantCoordinator* gc = byId.at(id);
        const TierEnergyAccount& a = energy[i].GetAccount(CA_TBL_TIER_868);
        os << "      anchor " << id << ": gossip sent " << c.sent << " (duty-dropped "
           << c.droppedDutyCycle << ", cad-dropped " << c.droppedCadExhausted
           << ", nothing-to-say " << skipped << "), received " << c.received
           << " (collided " << c.lostCollision << "), new items " << heard
           << "; tie-break WIN " << gc->Wins() << " DEFER " << gc->Defers()
           << "; requests withheld " << withheld << ", grants yielded " << yielded << "\n"
           << "        gossip energy (tier 1, gateway account): tx " << std::setprecision(6)
           << a.txJ << " J, rx " << a.rxJ << " J, cad+wake " << (a.cadJ + a.idleJ)
           << " J; gossip airtime " << c.airtimeS << " s = " << std::setprecision(5)
           << (durationS > 0.0 ? 100.0 * c.airtimeS / durationS : 0.0)
           << " % of the run\n";
    }
}

/** Everything one arm produced, for the summary table. */
struct ArmResult
{
    std::string arm;
    uint32_t seed;
    double rmseM;
    double cep50M;
    double cep95M;
    double maxErrM;
    //! Rival systems, from the SAME measurements. See BaselineMultilaterate().
    double rmseMlatM, cep95MlatM;   //!< WAN-style snapshot multilateration
    double rmseWclM, cep95WclM;     //!< weighted centroid, range-free
    double rmseProxM, cep95ProxM;   //!< proximity / contact
    double rmseLkpM, cep95LkpM;     //!< last known position, the no-system floor
    uint64_t mlatSolvedCycles;      //!< cycles the snapshot solver could solve
    //! RMSE and CEP50 over ONLY the cycles that produced a fix.
    //!
    //! The unconditional rmseM mixes localisation with dead reckoning, in a ratio that
    //! differs per arm, so it cannot answer "is this tier's ranging precise". For an
    //! arm whose tier is spot-coverage by construction the unconditional figure is
    //! dominated by coasting: static_uwb fixes on 11.6% of cycles and reports 150.6 m
    //! unconditional against a cep50 of 55.2 m. The fix-conditional figure is what the
    //! "UWB is the most accurate tier" check was always trying to test.
    //!
    //! Report BOTH. The conditional number alone would flatter every arm by hiding how
    //! often it had nothing to work with.
    double rmseFixedM;
    double cep50FixedM;
    //! Mean error, unconditional and fix-conditional. Reported beside the RMS
    //! because the two separate only when the tail is heavy, and which of them
    //! is which is the whole question in a system that spends cycles coasting.
    double meanErrM;
    double meanErrFixedM;
    //! When the system first knew anything, and when it first knew the position
    //! to within 100 m. The second is the operational one: the moment a ground
    //! team can be sent to a circle it can actually walk. NEGATIVE MEANS IT NEVER
    //! HAPPENED inside the run, which is a result and not a missing value.
    double tFirstFixS;
    double tFirst100mS;
    uint64_t fixCycles;
    double totalJ;
    double tierJ[CA_TBL_TIER_COUNT];
    uint64_t tierCycles[CA_TBL_TIER_COUNT];
    double airtimeS[CA_TBL_TIER_COUNT];
    uint64_t escalations;
    uint64_t deescalations;
    uint64_t suspects;
    uint64_t noFixCycles;
    //! Escalation handshakes suppressed by the tier-1 duty-cycle governor, and the
    //! total wait for the airtime budget to refill. Emitted because the regulation
    //! delaying a handoff propagates into position error -- a legal constraint
    //! reaching the localisation result.
    uint64_t handshakeDeferrals;
    double handshakeDeferralDelayS;
    uint64_t escalationsDeferredAway;
    //! Cycles where the node sacrificed a ranging exchange so that a pending
    //! escalation handshake could use the duty-cycle budget instead. The slot
    //! surrendered is the one on the CAPPED tier the handshake departs from
    //! (tier 1), not the active tier — tiers 2 and 3 carry no duty cap, so
    //! surrendering their ranging would free nothing the handshake can use.
    //! This implements the access-category priority the MAC spec already
    //! defines (escalation at CW_high above ranging at CW_normal) in the
    //! duty-cycle governor, where the budget is the binding constraint. The
    //! cost — this cycle's tier-1 measurements, and the position error that
    //! follows — is the honest price of the handoff.
    uint64_t reinitFallback;
    uint64_t handshakePreemptions;
    uint64_t steps;
    double durationS;

    // Trigger scoring against the geometric ground truth (claim 3).
    // "Should be on UWB" is defined as visUwb >= 3: the tier is geometrically
    // solvable. Confusion counts are accumulated per cycle over the active tier.
    uint64_t tp; //!< on UWB while visUwb >= 3
    uint64_t fp; //!< on UWB while visUwb <  3  -- escalated into dead air
    uint64_t fn; //!< not on UWB while visUwb >= 3 -- missed a free refinement
    uint64_t tn;
    uint64_t cyclesInUwbCoverage;

    // ---- S4 discovery / energy-axis diagnostics ----------------------------
    //
    // The energy breakdown is emitted because the §6 blocking defect could not be
    // diagnosed without it, and first-principles arithmetic on the old totals left
    // a ~9% residual on both static_868 and static_2p4 that nobody could attribute.
    // A single totalJ scalar is not enough to tell "this tier is expensive" from
    // "this tier is polling too many anchors": the first shows up in txJ, the
    // second almost entirely in rxJ. Both were previously invisible.
    double txJ[CA_TBL_TIER_COUNT];
    double rxJ[CA_TBL_TIER_COUNT];
    double cadJ[CA_TBL_TIER_COUNT];
    double idleJ[CA_TBL_TIER_COUNT];
    double sleepJ[CA_TBL_TIER_COUNT];

    //! Mean size of the one-hop neighbour table on each tier, over all cycles.
    //! This is the denominator the ranging poll now uses, so it is the number
    //! that replaced "every anchor in the deployment". If it equals the anchor
    //! count, the gate is not gating and the energy axis is unchanged.
    double meanNeighbours[CA_TBL_TIER_COUNT];
    //! Cycles on which the ACTIVE tier had an empty neighbour table, so no poll
    //! was issued at all. A large number here means escalation is landing in
    //! undiscovered space, which is a policy finding, not a bug.
    uint64_t blindCycles;
    //! Total first-time insertions into the neighbour tables -- discovery churn.
    uint64_t neighbourInsertions;

    // Emergent MAC outcomes at the node's receivers (claim 6). Zero captures with
    // nonzero collisions means the capture path is untested, not that capture
    // never happens -- read them together.
    uint64_t macTx;
    uint64_t macCollisions;
    uint64_t macCaptures;
    uint64_t macDeliveries;
    //! MS-1. subject_summary.csv rows for this arm and seed, one per measured subject;
    //! empty unless --measuredSubjects > 1. Appended last.
    std::vector<std::string> subjectRows;
};

/**
 * \brief Node trajectory: a tour of the facility sites.
 *
 * The previous version used an unconstrained random walk. That is the wrong
 * mobility model for this claim, not merely a less realistic one: with
 * short-range coverage confined to a few small sites, a random walk almost never
 * enters one, so the escalation decision is never posed. A tour poses it
 * repeatedly and in both directions (entering and leaving), which is what makes
 * hysteresis and de-escalation observable.
 *
 * Heading noise is retained so the path is not a clean straight line between
 * sites -- a perfectly straight approach would give the constant-velocity filter
 * an unrealistically easy prediction and flatter every arm equally.
 */
class Tourist
{
  public:
    Tourist(const std::vector<Vector>& sites,
            double speedMps,
            double dwellS,
            double boxHalfM,
            double dwellRadiusM,
            Ptr<UniformRandomVariable> rng)
        : m_sites(sites),
          m_speed(speedMps),
          m_dwellS(dwellS),
          m_boxHalf(boxHalfM),
          m_dwellRadius(dwellRadiusM),
          m_rng(rng),
          m_target(0),
          m_dwellLeft(0.0),
          m_heading(0.0),
          m_trail(false),
          m_trailY(0.0),
          m_deviateAtS(0.0),
          m_stopAfterS(0.0),
          m_elapsed(0.0)
    {
        // Start off-site so the run begins in the tier-1-only regime.
        m_x = m_rng->GetValue(-boxHalfM * 0.2, boxHalfM * 0.2);
        m_y = m_rng->GetValue(-boxHalfM * 0.2, boxHalfM * 0.2);
        PickTarget();
    }

    /**
     * \brief Walk a marked trail, then leave it.
     *
     * The SAR story the deployment is for: a walker follows a ridge path, deviates from
     * it, and stops being where anyone would look. Tourist's waypoint wander is the
     * right model for a monitored enclosure and the wrong one for this -- a wanderer is
     * never LOST, it is just somewhere else, and the difference is the whole scenario.
     *
     * \param trailY       the trail runs east-west at this northing
     * \param deviateAtS   when the walker leaves it. Before this the track is on the
     *                     trail and predictable, which is what makes the deviation
     *                     legible: a viewer sees the moment it stops being routine.
     * \param stopAfterS   seconds after deviating before the walker halts. A stationary
     *                     subject is the hard case -- K13 measured that no steering
     *                     helps once contact rate is ~0 -- and it must be reachable.
     */
    void SetTrail(double trailY, double deviateAtS, double stopAfterS,
                  double deviateSide = -1.0)
    {
        m_trail = true;
        // -1 walks south (away from an arc centred north), +1 north (through it).
        // Default -1 is the previous hard-coded behaviour.
        m_deviateSide = (deviateSide >= 0.0) ? 1.0 : -1.0;
        m_trailY = trailY;
        m_deviateAtS = deviateAtS;
        m_stopAfterS = stopAfterS;
        m_x = -m_boxHalf * 0.8;
        m_y = trailY;
        m_elapsed = 0.0;
    }

    //! Has the walker left the trail? The demo needs the moment, not just the track.
    double m_deviateSide = -1.0;
    bool HasDeviated() const { return m_trail && m_elapsed >= m_deviateAtS; }
    bool HasStopped() const
    {
        return m_trail && m_stopAfterS > 0.0 && m_elapsed >= m_deviateAtS + m_stopAfterS;
    }

    void Step(double dt)
    {
        if (m_trail)
        {
            m_elapsed += dt;
            if (HasStopped())
            {
                return; // sat down, and this is the case that defeats searching
            }
            if (m_elapsed < m_deviateAtS)
            {
                // On the trail: due east at walking pace, entirely predictable.
                m_x += m_speed * dt;
            }
            else
            {
                // Off it. Heading is fixed at the moment of deviation rather than
                // wandering, because a person who has lost the path walks a line they
                // believe is right -- and a fixed bearing is what makes the search
                // problem geometric rather than diffusive.
                m_x += m_speed * dt * 0.35;
                m_y += m_deviateSide * m_speed * dt * 0.94;
            }
            if (m_x > m_boxHalf) { m_x = m_boxHalf; }
            if (m_y < -m_boxHalf) { m_y = -m_boxHalf; }
            return;
        }
        if (m_dwellLeft > 0.0)
        {
            // Milling about inside a site rather than standing perfectly still.
            m_dwellLeft -= dt;
            {
                const double w = m_rng->GetValue(-1.2, 1.2);
                const double lim = (m_maxTurnRate > 0.0)
                                       ? std::max(-m_maxTurnRate,
                                                  std::min(m_maxTurnRate, w))
                                       : w;
                m_heading += lim * dt;
            }
            m_x += 0.3 * m_speed * std::cos(m_heading) * dt;
            m_y += 0.3 * m_speed * std::sin(m_heading) * dt;

            // Keep the mill-about inside the site. Unbounded, 0.36 m/s over a 90 s
            // dwell wanders up to ~32 m, which walks straight out of a 10 m UWB
            // anchor ring -- the node would be "at a facility" by the scenario's
            // logic while being out of range of the tier that facility exists to
            // provide. Someone waiting at a gate stays at the gate.
            const Vector site = m_sites[m_target];
            const double dr = std::hypot(m_x - site.x, m_y - site.y);
            if (dr > m_dwellRadius && dr > 1e-9)
            {
                const double s = m_dwellRadius / dr;
                m_x = site.x + (m_x - site.x) * s;
                m_y = site.y + (m_y - site.y) * s;
                // Turn back inward. Unbounded this was an INSTANTANEOUS half-turn,
                // which is exactly what a yaw integrator cannot follow -- and it
                // fired every time the mill-about touched the dwell radius. With a
                // rate set, steer toward the site centre at that rate instead.
                if (m_maxTurnRate > 0.0)
                {
                    const double inward = std::atan2(site.y - m_y, site.x - m_x);
                    double d = inward - m_heading;
                    while (d > M_PI) d -= 2.0 * M_PI;
                    while (d < -M_PI) d += 2.0 * M_PI;
                    m_heading += std::max(-m_maxTurnRate * dt,
                                          std::min(m_maxTurnRate * dt, d));
                }
                else
                {
                    m_heading += M_PI;
                }
            }

            if (m_dwellLeft <= 0.0)
            {
                PickTarget();
            }
            Clamp();
            return;
        }

        const Vector t = m_sites[m_target];
        const double bearing = std::atan2(t.y - m_y, t.x - m_x);

        // Steer toward the bearing but do not snap to it: correlated wander
        // around the intended course.
        double diff = bearing - m_heading;
        while (diff > M_PI)
        {
            diff -= 2.0 * M_PI;
        }
        while (diff < -M_PI)
        {
            diff += 2.0 * M_PI;
        }
        m_heading += std::max(-0.35 * dt, std::min(0.35 * dt, diff)) +
                     m_rng->GetValue(-0.10, 0.10) * dt;

        m_x += m_speed * std::cos(m_heading) * dt;
        m_y += m_speed * std::sin(m_heading) * dt;

        if (std::hypot(t.x - m_x, t.y - m_y) < 25.0)
        {
            m_dwellLeft = m_dwellS * m_rng->GetValue(0.6, 1.4);
        }
        Clamp();
    }

    Vector Position() const { return Vector(m_x, m_y, 1.5); }

  private:
    void PickTarget()
    {
        if (m_sites.size() <= 1)
        {
            m_target = 0;
            return;
        }
        size_t next = m_target;
        while (next == m_target)
        {
            next = static_cast<size_t>(m_rng->GetValue(0.0, static_cast<double>(m_sites.size())));
            if (next >= m_sites.size())
            {
                next = m_sites.size() - 1;
            }
        }
        m_target = next;
    }

    void Clamp()
    {
        if (m_x < -m_boxHalf || m_x > m_boxHalf || m_y < -m_boxHalf || m_y > m_boxHalf)
        {
            m_heading += M_PI;
            m_x = std::max(-m_boxHalf, std::min(m_boxHalf, m_x));
            m_y = std::max(-m_boxHalf, std::min(m_boxHalf, m_y));
        }
    }

    std::vector<Vector> m_sites;
    double m_speed;
    double m_dwellS;
    double m_boxHalf;
    double m_dwellRadius;
    //! Max heading change per second, rad/s. 0 = unbounded, the behaviour every
    //! run before 2026-09-08 had. See SetMaxTurnRate().
    double m_maxTurnRate = 0.0;
    Ptr<UniformRandomVariable> m_rng;
    size_t m_target;
    double m_dwellLeft;
    double m_heading;
    bool m_trail;
    double m_trailY;
    double m_deviateAtS;
    double m_stopAfterS;
    double m_elapsed;
    double m_x;
    double m_y;

  public:
    //! Place the walker. Used to start finders at a base camp rather than in the
    //! middle of the search area; the tour target is unaffected, so the walker
    //! simply sets off from somewhere else.
    //! Bound how fast the walker may change heading.
    //!
    //! A yaw-integrating heading estimator cannot follow a discontinuity, and the
    //! dwell had two: a mill-about wander up to 1.2 rad/s, and an INSTANTANEOUS
    //! half-turn when the walker reached the dwell radius. With a rate set, the
    //! wander is clamped and the reversal becomes a bounded turn back toward the
    //! site centre, taken over several steps the way a person turns around.
    void SetMaxTurnRate(double radPerS) { m_maxTurnRate = radPerS; }

    void SetPosition(double x, double y)
    {
        m_x = x;
        m_y = y;
    }
};

/**
 * \brief Unfiltered least-squares multilateration -- the WAN-localisation
 *        baseline, and what a snapshot solver gives with no memory.
 *
 * Linearised about the first anchor: subtracting the reference circle's equation
 * from each of the others removes the quadratic term and leaves a linear system,
 * solved here in closed form for the 2x2 normal equations. Returns false when
 * fewer than three anchors are available or the geometry is degenerate, and the
 * caller then HOLDS its previous estimate rather than being charged for silence.
 */
bool
BaselineMultilaterate(const std::vector<CaTblRangeMeasurement>& m,
                      double& outX,
                      double& outY)
{
    if (m.size() < 3)
    {
        return false;
    }
    const double x0 = m[0].anchorX;
    const double y0 = m[0].anchorY;
    const double r0 = m[0].rangeM;
    double a11 = 0.0, a12 = 0.0, a22 = 0.0, b1 = 0.0, b2 = 0.0;
    for (size_t i = 1; i < m.size(); ++i)
    {
        const double ax = 2.0 * (m[i].anchorX - x0);
        const double ay = 2.0 * (m[i].anchorY - y0);
        const double bb = (r0 * r0 - m[i].rangeM * m[i].rangeM)
                        + (m[i].anchorX * m[i].anchorX - x0 * x0)
                        + (m[i].anchorY * m[i].anchorY - y0 * y0);
        a11 += ax * ax;
        a12 += ax * ay;
        a22 += ay * ay;
        b1 += ax * bb;
        b2 += ay * bb;
    }
    const double det = a11 * a22 - a12 * a12;
    if (std::fabs(det) < 1e-9)
    {
        return false;
    }
    outX = (b1 * a22 - a12 * b2) / det;
    outY = (a11 * b2 - a12 * b1) / det;
    return std::isfinite(outX) && std::isfinite(outY);
}

double
Percentile(std::vector<double> v, double p)
{
    if (v.empty())
    {
        return 0.0;
    }
    std::sort(v.begin(), v.end());
    const double idx = p * (static_cast<double>(v.size()) - 1.0);
    const size_t lo = static_cast<size_t>(std::floor(idx));
    const size_t hi = static_cast<size_t>(std::ceil(idx));
    if (lo == hi)
    {
        return v[lo];
    }
    const double frac = idx - static_cast<double>(lo);
    return v[lo] * (1.0 - frac) + v[hi] * frac;
}

/**
 * \brief Cramer-Rao lower bound on ToF ranging std-dev, in metres.
 *
 *   sigma_t     = 1 / (2*pi * beta_rms * sqrt(2*SNR))
 *   beta_rms    = BW / sqrt(12)
 *   sigma_range = c * sigma_t          (/ sqrt(2) for a symmetric two-way exchange)
 *
 * snrDb here is E/N0 at the correlator output. Callers must add the coherent
 * integration gain first -- see CoherentGainDb(). Passing raw in-band SNR is the
 * bug this check itself used to contain.
 *
 * This is the project's own established formula, not a new one: beta_rms =
 * 125e3/sqrt(12) = 36 084 Hz reproduces the figure simstatus.md 4.1 used to
 * calibrate the 868 MHz timing jitter.
 *
 * Why it is now checked rather than trusted. A ranging model reporting precision
 * BELOW this bound is not optimistic, it is impossible -- and the consequence is
 * not cosmetic. A tier with unphysical precision silently dominates every
 * architectural comparison. That is precisely what the first S2.5 arm run showed:
 * 2.4 GHz reported a flat 0.24 m mean range error at every distance, because the
 * tier-2 and tier-3 ranging branches used a configured sigma with no bandwidth,
 * SNR or distance term in them at all, and it consequently beat the proposed
 * hybrid on accuracy-per-joule, making claim 1 look false. Bugs #10 and #15 in
 * simstatus.md are the same class of defect -- a ranging or jitter figure not
 * traceable to the physics that must produce it.
 *
 * Reference arithmetic at the measured 900 m tier-2 operating point (BW 800 kHz,
 * beta_rms = 230 940 Hz, in-band SNR 16.9 dB, two-way): with the SF7 dechirp gain
 * of 21.07 dB the bound is 1.30 m; without it, 14.76 m. The measured 0.24 m
 * violated both. The engine now floors tier 2 at the SX1280's specified
 * meter-level hardware limit, so 1.30 m is the value this check should see the
 * model approach at that range, not beat.
 *
 * Duplicated deliberately from CrlbRangeStdM() in lora-ranging-submodule.cc. This
 * is the audit of that model; sharing one implementation would let a wrong formula
 * agree with itself.
 */
double
CrlbRangeStdM(double bandwidthHz, double snrDb, bool twoWay)
{
    if (bandwidthHz <= 0.0)
    {
        return -1.0;
    }
    const double snrLin = std::pow(10.0, snrDb / 10.0);
    if (snrLin <= 0.0)
    {
        return -1.0;
    }
    const double betaRms = bandwidthHz / std::sqrt(12.0);
    const double sigmaT = 1.0 / (2.0 * M_PI * betaRms * std::sqrt(2.0 * snrLin));
    const double sigmaM = 299792458.0 * sigmaT;
    return twoWay ? sigmaM / std::sqrt(2.0) : sigmaM;
}

/**
 * \brief Coherent integration gain, dB, to convert in-band SNR to E/N0.
 *
 * The previous version of this check fed result.snr_db straight into
 * CrlbRangeStdM(). That is wrong in a direction that matters: result.snr_db is
 * rxPowerDbm - noiseFloorDbm, and noiseFloorDbm is computed over the OCCUPIED
 * bandwidth, so it is a pre-integration figure. The ToA Cramer-Rao bound is a
 * function of E/N0 at the correlator output.
 *
 * The consequence was a false verdict on UWB specifically. Raw UWB SNR is negative
 * (about -17 dB at 2 m), so the un-gained bound came out at metres, and a measured
 * 0.04 m was reported as a 156.8x violation. With the 30.1 dB accumulation gain the
 * bound is 0.038 m and 0.04 m is legitimate. The 2.4 GHz violation was real; the
 * UWB one was an artefact of this function's own omission.
 *
 *   tier 1  0 dB. RSSI-inversion ranging is not a ToA estimator, so the bound does
 *           not apply to it at all. The column is retained only because tier-1
 *           error (52 m at 100 m) sits far above any bound and so cannot mask a
 *           defect; read it as "not applicable", not as "passing".
 *   tier 2  10*log10(2^SF), the LoRa CSS dechirp gain.
 *   tier 3  10*log10(PSR), DW3000 preamble accumulation. Must match
 *           RangingParams::uwbPreambleSymbols or this check audits a different
 *           model than the engine runs.
 */
double
CoherentGainDb(CaTblTier tier, uint8_t sf, double uwbPreambleSymbols)
{
    if (tier == CA_TBL_TIER_2P4)
    {
        return 10.0 * std::log10(std::pow(2.0, static_cast<double>(sf)));
    }
    if (tier == CA_TBL_TIER_UWB)
    {
        return 10.0 * std::log10(uwbPreambleSymbols > 0.0 ? uwbPreambleSymbols : 1.0);
    }
    return 0.0;
}

// The three engine configs are built with the same copy-initialisation pattern the
// previous version used (ForScenario at the point of declaration) rather than being
// default-constructed and assigned in a helper, because these config types are not
// guaranteed to be default-constructible. Kept as a macro-free duplication in the
// two call sites deliberately -- a helper here would trade a compile guarantee for
// six saved lines.
// Vegetation is per-tier, and that is a statement about WHERE each tier's
// infrastructure is, not a tuning knob:
//
//   vegWide     -- tier 1. The 868 MHz mesh spans kilometres of the search area
//                  and genuinely crosses canopy. VEG_DENSE_FOREST.
//   vegFacility -- tiers 2 and 3. Their anchors sit AT a facility site (gate,
//                  hut, checkpoint), which is a clearing by construction of the
//                  deployment. lora-uwb-physics-parameters.cc scales foliage
//                  specific attenuation by 5.28x for 7.99 GHz; at dense-forest
//                  levels (~2 dB/m) that consumes the entire UWB link budget
//                  inside ~15 m, which is why the first probe showed UWB deaf at
//                  2 m. Applying canopy loss to a 35 m intra-facility link is a
//                  category error about anchor siting.
//
// Default VEG_SPARSE_SHRUBS rather than VEG_NONE: a real clearing has scrub, and
// VEG_NONE would be an idealisation. Exposed as --facilityVeg so it can be swept;
// if the architectural conclusion depends on this value, that dependence is a
// result and must be reported, not hidden.
// Environment class is per-tier for the same reason vegetation is: it describes
// where the tier's infrastructure sits.
//
//   envWide/vegWide         -- tier 1 spans kilometres of search area and crosses
//                              canopy. ENV_RURAL_FORESTED + VEG_DENSE_FOREST.
//   envFacility/vegFacility -- tiers 2 and 3 range against anchors AT a facility
//                              site. ENV_RURAL_OPEN + VEG_SPARSE_SHRUBS.
//
// Leaving these global was a real error and it produced a real artefact: the
// first probe showed UWB at 17.7% PDR at 2 m, and after the vegetation fix it
// still collapsed by 10 m, because a 7.99 GHz link over 35 m of open ground was
// being charged forested-PLE (2.3) plus canopy loss. Both are correct physics for
// the wide-area mesh and wrong physics for an intra-facility link.
// D-4. Terrain derived its wavelength from a hardcoded 868 MHz and applied the
// resulting Fresnel and diffraction geometry to every tier, UWB at 7.99 GHz
// included -- a wavelength wrong by 9.2x. This carries the per-tier carrier
// into TerrainParams instead. It is a FILE-SCOPE GLOBAL because the configs are
// built by a macro expanded at several call sites and a macro cannot see a
// local; it is written once, after cmd.Parse(), and read-only thereafter.
static bool g_terrainPerTierFreq = false;
// TECH_DEBT M.2 (report/PREREG_UWB_NLOS.md). --uwbNlosModel, a file-scope global for the
// same reason as the one above: written once after cmd.Parse(), read by the macro.
static UwbNlosModel g_uwbNlosModel = UWB_NLOS_LEGACY;

// SY-3. Receiver sensitivity offset for a bandwidth other than the one each engine's table
// was written for (868 MHz: 125 kHz; 2.4 GHz: the 800 kHz this scenario has always used).
// LoRa sensitivity scales with the noise bandwidth, 10*log10(BW/BW0) dB, which is how the
// Semtech datasheet tables step between bandwidths. Written once after cmd.Parse(), read by
// the macro below; 0 (every default) leaves the tables untouched.
static double g_sensShiftDb868 = 0.0;
static double g_sensShiftDb2p4 = 0.0;
//! SY-3. True when any of --sf868/--bw868/--sf2p4/--bw2p4 was given: RunArm then prints the
//! sensitivity its engines were actually configured with.
static bool g_sy3Given = false;

#define CA_TBL_MAKE_CONFIGS(envWide, envFacility, seed, vegWide, vegFacility, c868, c2p4, cUwb)    \
    LoRaPhysicsConfig c868 = LoRaPhysicsConfig::ForScenario((envWide),                             \
                                                           WEATHER_CLEAR_DRY,                      \
                                                           FADING_RICIAN,                          \
                                                           (vegWide),                              \
                                                           MOB_CONSTANT_VELOCITY,                  \
                                                           false);                                 \
    c868.randomSeed = (seed);                                                                      \
    c868.rangingModelMode = RANGING_PHYSICS_DERIVED;                                               \
    LoRa2p4PhysicsConfig c2p4 = LoRa2p4PhysicsConfig::ForScenario((envFacility),                   \
                                                                 WEATHER_CLEAR_DRY,                \
                                                                 FADING_RICIAN,                    \
                                                                 (vegFacility),                    \
                                                                 MOB_CONSTANT_VELOCITY,            \
                                                                 false);                           \
    c2p4.randomSeed = (seed);                                                                      \
    c2p4.rangingModelMode = RANGING_PHYSICS_DERIVED;                                               \
    LoRaUWBPhysicsConfig cUwb = LoRaUWBPhysicsConfig::ForScenario((envFacility),                   \
                                                                 WEATHER_CLEAR_DRY,                \
                                                                 FADING_RICIAN,                    \
                                                                 (vegFacility),                    \
                                                                 MOB_CONSTANT_VELOCITY,            \
                                                                 false);                           \
    cUwb.randomSeed = (seed);                                                                      \
    cUwb.rangingModelMode = RANGING_PHYSICS_DERIVED;                                               \
    cUwb.nlosModel = g_uwbNlosModel;                                                               \
    if (g_terrainPerTierFreq)                                                                      \
    {                                                                                              \
        /* Carriers from Architecture.md 2.1: 868 MHz, 2.4 GHz ISM, UWB Ch9 7.99 GHz. */           \
        c2p4.terrain.frequencyHz = 2.4e9;                                                          \
        cUwb.terrain.frequencyHz = 7.9872e9;                                                       \
    }                                                                                              \
    if (g_sensShiftDb868 != 0.0)                                                                   \
    {                                                                                              \
        for (int sfi = 0; sfi < 6; ++sfi)                                                          \
        {                                                                                          \
            c868.receiver.sensitivityDbm[sfi] += g_sensShiftDb868;                                 \
        }                                                                                          \
    }                                                                                              \
    if (g_sensShiftDb2p4 != 0.0)                                                                   \
    {                                                                                              \
        for (int sfi = 0; sfi < 8; ++sfi)                                                          \
        {                                                                                          \
            c2p4.receiver.sensitivityDbm[sfi] += g_sensShiftDb2p4;                                 \
        }                                                                                          \
    }

/**
 * \brief MS-2. Gateway positions for a named placement shape (--gwLayout).
 *
 * The shapes GEO-01 (docs/SPEC_CONFORMANCE_PLAN.md sect 3.4) compares, besides the
 * ring, arc, ridge and grid BuildDeployment already builds. Every one keeps to positions
 * a vehicle could plausibly reach -- the box edge, roads out of a base camp -- except
 * `file`, which takes whatever the optimiser wrote. Coordinates are clamped to the box.
 * All constants are OURS. Returns an empty vector, with a reason, on a bad input.
 *
 *   perimeter  N points evenly spaced along the square of half-width frac*boxHalf, the
 *              first at its west midpoint, going south (counter-clockwise).
 *   cluster    N points on a circle of radius clusterR around the camp (N = 1: at it).
 *   star       N points on `arms` straight roads out of the camp, `spacing` apart along
 *              each, the roads spread over 120 degrees about the bearing from the camp to
 *              the box centre (east when the camp is the centre); point k on road k % arms.
 *   lshape     two perpendicular roads meeting at the south-west corner of the frac
 *              square, up its west side and along its south side: point 0 at the corner,
 *              then alternately north and east, ceil((N-1)/2) steps of 2*frac*boxHalf/
 *              ceil((N-1)/2) each way.
 *   file       one "x,y" per line from a CSV (a header line and '#' lines are skipped).
 */
static std::vector<Vector>
CaTblGatewayLayout(const std::string& layout, uint32_t n, double boxHalfM, double frac,
                   double campX, double campY, double clusterR, uint32_t arms,
                   double spacingM, const std::string& file, std::string& why)
{
    std::vector<Vector> pts;
    const double gwH = 15.0; // the mast height every BuildDeployment shape uses
    auto clampBox = [boxHalfM](double v) { return std::max(-boxHalfM, std::min(boxHalfM, v)); };
    const double h = frac * boxHalfM;
    if (layout == "perimeter")
    {
        const double L = 8.0 * h;
        for (uint32_t k = 0; k < n; ++k)
        {
            double s = L * static_cast<double>(k) / static_cast<double>(n);
            double x, y;
            // Legs from the west midpoint: south half-side, south side, east side,
            // north side, north half of the west side.
            if (s < h)
            {
                x = -h;
                y = -s;
            }
            else if (s < 3.0 * h)
            {
                x = -h + (s - h);
                y = -h;
            }
            else if (s < 5.0 * h)
            {
                x = h;
                y = -h + (s - 3.0 * h);
            }
            else if (s < 7.0 * h)
            {
                x = h - (s - 5.0 * h);
                y = h;
            }
            else
            {
                x = -h;
                y = h - (s - 7.0 * h);
            }
            pts.push_back(Vector(clampBox(x), clampBox(y), gwH));
        }
    }
    else if (layout == "cluster")
    {
        for (uint32_t k = 0; k < n; ++k)
        {
            const double th = 2.0 * M_PI * static_cast<double>(k) / static_cast<double>(n);
            const double r = (n == 1) ? 0.0 : clusterR;
            pts.push_back(Vector(clampBox(campX + r * std::cos(th)),
                                 clampBox(campY + r * std::sin(th)), gwH));
        }
    }
    else if (layout == "star")
    {
        const double centre = (campX == 0.0 && campY == 0.0) ? 0.0 : std::atan2(-campY, -campX);
        const double spread = 120.0 * M_PI / 180.0;
        for (uint32_t k = 0; k < n; ++k)
        {
            const uint32_t arm = k % arms;
            const uint32_t step = 1 + k / arms;
            const double th = (arms == 1)
                                  ? centre
                                  : centre - spread / 2.0 +
                                        spread * static_cast<double>(arm) /
                                            static_cast<double>(arms - 1);
            const double r = spacingM * static_cast<double>(step);
            pts.push_back(Vector(clampBox(campX + r * std::cos(th)),
                                 clampBox(campY + r * std::sin(th)), gwH));
        }
    }
    else if (layout == "lshape")
    {
        const uint32_t steps = std::max<uint32_t>(1, (n) / 2); // ceil((n-1)/2)
        const double d = 2.0 * h / static_cast<double>(steps);
        for (uint32_t k = 0; k < n; ++k)
        {
            double x = -h, y = -h;
            if (k > 0)
            {
                const uint32_t step = 1 + (k - 1) / 2;
                if ((k - 1) % 2 == 0)
                {
                    y += d * static_cast<double>(step); // north, up the west side
                }
                else
                {
                    x += d * static_cast<double>(step); // east, along the south side
                }
            }
            pts.push_back(Vector(clampBox(x), clampBox(y), gwH));
        }
    }
    else if (layout == "file")
    {
        std::ifstream in(file.c_str());
        if (!in.is_open())
        {
            why = "cannot open --gwLayoutFile '" + file + "'";
            return std::vector<Vector>();
        }
        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#')
            {
                continue;
            }
            std::istringstream ls(line);
            double x, y;
            char comma;
            if (!(ls >> x >> comma >> y) || comma != ',')
            {
                continue; // a header line
            }
            pts.push_back(Vector(clampBox(x), clampBox(y), gwH));
        }
        if (pts.empty())
        {
            why = "--gwLayoutFile '" + file + "' holds no x,y rows";
        }
    }
    else
    {
        why = "unknown --gwLayout '" + layout + "'";
    }
    return pts;
}

/**
 * \brief Pre-flight geometry check.
 *
 * Sweeps distance per tier and reports reception rate and ranging error. Run
 * this before believing any arm result. The point is that the anchor spacing in
 * BuildDeployment() is a claim about the physics, and this is the measurement
 * that either supports it or refutes it.
 *
 * What must be true for the arm comparison to mean anything:
 *   - tier 1 usable out to at least ~2 km   (else tier-1 GDOP is the story)
 *   - tier 2 usable to a few hundred metres (else the middle tier is decoration)
 *   - tier 3 usable to ~100 m and NOT beyond (else there is no coverage gap to
 *     decide about, and escalation is unconditionally correct)
 */
void
RunProbe(EnvironmentClass envWide,
         EnvironmentClass envFacility,
         uint32_t seed,
         VegetationType vegWide,
         VegetationType vegFacility,
         const TierPhy phy[CA_TBL_TIER_COUNT],
         const std::string& outdir,
         bool traceDiagnostic)
{
    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(1);
    // Same reason as RunArm(): the probe's engines must not inherit a stream index
    // left behind by anything constructed before them.
    RngSeedManager::ResetNextStreamIndex();

    CA_TBL_MAKE_CONFIGS(envWide, envFacility, seed, vegWide, vegFacility, c868, c2p4, cUwb)

    LoRaPhysicsEngine e868;
    e868.Initialize(c868);
    LoRa2p4PhysicsEngine e2p4;
    e2p4.Initialize(c2p4);
    LoRaUWBPhysicsEngine eUwb;
    eUwb.Initialize(cUwb);

    const uint32_t gwId = 9001;
    const uint32_t nodeId = 1;

    // Anchor heights MUST match BuildDeployment(), per tier. They previously did
    // not: the probe used 6.0 m for all three tiers while the deployment uses
    // 15.0 m for tier-1 gateways, 6.0 m for tier-2 anchors and 2.5 m for UWB
    // anchors. Height is not a cosmetic difference here -- the two-ray ground
    // submodule's crossover distance is dc = 4*pi*txH*rxH/lambda, so the probe was
    // measuring a different propagation regime than the arms run in, and the r80
    // values the deployment geometry is SIZED FROM came from the wrong geometry.
    //
    // Node height stays 1.5 m, matching FieldNode::Position().
    const double nodeH = 1.5;
    const double anchorH[CA_TBL_TIER_COUNT] = {15.0, 6.0, 2.5};

    // ---- Spatial decorrelation sites --------------------------------------
    //
    // Rotating the azimuth at fixed radius d moves the node 2*pi*d/trials per
    // trial, so the path swept is proportional to d. The instrumented run showed
    // this is too weak at short range: the realised sample stddev of the shadowing
    // draws was 0.21x the effective sigma at UWB 10 m and 0.39x at 2.4 GHz 10 m,
    // i.e. on the order of 7 to 23 independent draws instead of 2000. UWB r80 is
    // computed entirely from points in that regime, which is why it moved 10 m to
    // 5 m between two runs of the same seed.
    //
    // Fix: sweep the azimuth AND cycle the link across kSites well-separated
    // gateway sites. Each site is a distinct gateway id, so each gets its own
    // Gudmundson cache and its own absolute position in the shadowing, foliage and
    // terrain fields. Site spacing is kilometres, far beyond any shadowing
    // correlation length, so consecutive trials at the same radius are drawn from
    // genuinely different channel realisations.
    //
    // Sites are placed on a 2D additive-recurrence (plastic-number) low-discrepancy
    // lattice rather than a grid: a grid at fixed pitch could itself alias against
    // the procedural terrain's spatial period.
    //
    // kSites = 64 and trials = 2000 gives 31-32 trials per site, and because
    // 2*pi*64/2000 = 11.5 degrees of azimuth advance per site revisit, each site
    // still sees a nearly complete azimuth sweep. The two sequences do not lock.
    const uint32_t kSites = 64;
    const double siteHalfM = 2000.0;
    const double kPlastic1 = 0.7548776662466927;
    const double kPlastic2 = 0.5698402909980532;

    std::vector<Vector> siteXY;
    siteXY.reserve(kSites);
    for (uint32_t s = 0; s < kSites; ++s)
    {
        const double u = std::fmod(0.5 + kPlastic1 * static_cast<double>(s + 1), 1.0);
        const double v = std::fmod(0.5 + kPlastic2 * static_cast<double>(s + 1), 1.0);
        siteXY.push_back(Vector((u - 0.5) * 2.0 * siteHalfM, (v - 0.5) * 2.0 * siteHalfM, 0.0));
    }

    // Gateway ids are per-site and per-tier so no two engines share a link id.
    for (uint32_t s = 0; s < kSites; ++s)
    {
        e868.RegisterGateway(gwId + s, Vector(siteXY[s].x, siteXY[s].y, anchorH[0]), true);
        e2p4.RegisterGateway(gwId + s, Vector(siteXY[s].x, siteXY[s].y, anchorH[1]), true);
        eUwb.RegisterGateway(gwId + s, Vector(siteXY[s].x, siteXY[s].y, anchorH[2]), true);
    }
    e868.RegisterNode(nodeId, Vector(siteXY[0].x, siteXY[0].y, nodeH));
    e2p4.RegisterNode(nodeId, Vector(siteXY[0].x, siteXY[0].y, nodeH));
    eUwb.RegisterNode(nodeId, Vector(siteXY[0].x, siteXY[0].y, nodeH));

    const double dists868[] = {100, 250, 500, 1000, 1500, 2000, 3000, 4000, 6000};
    // Extended past 900 m. The previous array ended at 900 m with PDR 0.804 and no
    // falloff, so tier-2 r80 was CENSORED -- a lower bound printed as if it were a
    // measurement. That number is what claim 1 turns on: static_2p4 beats the
    // proposed system on accuracy-per-joule, and whether that is an artefact of an
    // undersized tier-2 triad (150 m against an unknown reach) or a real result
    // cannot be decided without knowing where tier 2 actually stops.
    //
    // Top end is 6000 m, matching dists868, so the question is closed rather than
    // pushed one grid step further out. Tier-2 anchors sit in the FACILITY
    // environment (RURAL_OPEN / SPARSE_SHRUBS), not the wide-area forest, which is
    // why the tier reaches further than the 8.83 dB of extra free-space loss at
    // 2.4 GHz vs 868 MHz would suggest -- 20*log10(2400/868) = 8.83 dB.
    const double dists2p4[] = {10,   25,   50,   100,  150,  200,  300,  430, 600,
                               900,  1200, 1600, 2000, 2500, 3000, 4000, 6000};
    const double distsUwb[] = {2, 5, 10, 20, 30, 50, 70, 100, 130, 180};
    const int n868 = static_cast<int>(sizeof(dists868) / sizeof(double));
    const int n2p4 = static_cast<int>(sizeof(dists2p4) / sizeof(double));
    const int nUwb = static_cast<int>(sizeof(distsUwb) / sizeof(double));
    // 2000, not 400. At 400 the UWB r80 estimate moved from 10 m to 5 m between
    // two runs of the same seed -- i.e. the statistic the deployment geometry is
    // being sized from was not stable to within a factor of two. Binomial
    // standard error at pdr=0.5 is 0.025 at n=400 and 0.011 at n=2000.
    const int trials = 2000;

    std::ofstream out((outdir + "/probe_range_sweep.csv").c_str());
    // CHECKED -- same defect as arm_summary.csv, on the writer that produced the
    // tier-2 r80 = 909.5 m measurement the deployment geometry is sized from. The
    // "Wrote ...probe_range_sweep.csv" line below printed unconditionally.
    if (!out)
    {
        std::cerr << "FATAL: cannot open " << outdir
                  << "/probe_range_sweep.csv for writing -- does --outdir exist?\n";
        std::exit(1);
    }
    // Columns after n are diagnostic, added to resolve the UWB PDR
    // non-monotonicity (0.099 at 30 m rising to 0.385 at 50 m). They are here
    // because the mechanism was being guessed at from the PDR column alone.
    //
    //   coherent_gain_db  what was added to mean_snr_db before computing the bound
    //   two_ray_frac      fraction of ALL trials with result.isTwoRay set
    //   nlos_frac         fraction of ALL trials with result.isNLOS set
    //
    // NOTE: these two were documented here as "fraction of RECEIVED trials". They
    // are not, and the extended tier-2 sweep made that visible: the 6000 m row
    // reports pdr 0.000 with two_ray_frac 0.889 and nlos_frac 0.111, which is only
    // possible if the accumulator counts every trial rather than every decode. The
    // columns are still the right diagnostic -- channel state does not depend on
    // whether the frame happened to decode -- but do not read them as conditioned
    // on reception. Unlike mean_abs_range_err_m, which IS so conditioned, and which
    // is therefore survivorship-biased wherever pdr is low. See the PDR floor on
    // the CRLB gate below.
    //   mean_pl_db        mean result.pathLossDb
    //   mean_margin_db    mean result.sensitivityMarginDb
    //   shadow_sd_db      SAMPLE stddev of result.shadowingDb ACROSS trials
    //   shadow_sd_eff_db  mean result.shadowingStdEff, i.e. what that sample
    //                     stddev should converge to if the trials are independent
    //
    // The last two are the decisive pair. The engine's shadowing is Gudmundson
    // spatially correlated, and the azimuth rotation only moves the node by an arc
    // of 2*pi*d/trials per trial. At d = 30 m the whole 2000-trial circle is 188 m
    // of path, so if the correlation length is tens of metres the point averages
    // over a handful of effectively independent draws, not 2000. If shadow_sd_db is
    // well below shadow_sd_eff_db then the trials at that distance are NOT
    // independent, the binomial standard error of 0.011 quoted for n=2000 does not
    // apply, and the "26 sigma" reading of the 30-to-50 m rise is not supported.
    // That is Bug #1 from simstatus.md recurring at short range, where the azimuth
    // fix is geometrically too weak to decorrelate anything.
    // mean_snr_all_db is gated behind --traceDiagnostic so the default output is
    // byte-identical to every run that preceded this change. It is the D2 control
    // column: mean_snr_db conditioned on delivery, mean_snr_all_db over all trials.
    out << "tier,distance_m,pdr,mean_abs_range_err_m,mean_snr_db,crlb_floor_m,"
        << "measured_over_crlb,n,coherent_gain_db,two_ray_frac,nlos_frac,"
        << "mean_pl_db,mean_margin_db,shadow_sd_db,shadow_sd_eff_db"
        << (traceDiagnostic ? ",mean_snr_all_db" : "") << "\n";

    std::cout << "\nPre-flight probe: reception and ranging error vs distance\n"
              << "  (" << trials << " trials per point)\n"
              << "  tier 1 (wide area): env=" << static_cast<int>(envWide)
              << " veg=" << static_cast<int>(vegWide) << "\n"
              << "  tiers 2/3 (facility): env=" << static_cast<int>(envFacility)
              << " veg=" << static_cast<int>(vegFacility) << "\n\n";

    for (int t = 0; t < CA_TBL_TIER_COUNT; ++t)
    {
        const CaTblTier tier = static_cast<CaTblTier>(t);
        const double* ds = (t == 0) ? dists868 : (t == 1) ? dists2p4 : distsUwb;
        const int nd = (t == 0) ? n868 : (t == 1) ? n2p4 : nUwb;

        const double gainDb = CoherentGainDb(tier, phy[t].sf, cUwb.ranging.uwbPreambleSymbols);
        const double gwH = anchorH[t];

        std::cout << "  " << CaTblTierName(tier) << "  (BW=" << phy[t].bandwidthHz / 1e3
                  << " kHz, anchor h=" << std::fixed << std::setprecision(1) << gwH
                  << " m, coherent gain=" << std::setprecision(2) << gainDb << " dB, "
                  << kSites << " sites)\n"
                  << "    " << std::right << std::setw(10) << "dist_m" << std::setw(10) << "pdr"
                  << std::setw(16) << "range_err_m" << std::setw(10) << "snr_dB" << std::setw(14)
                  << "crlb_floor_m" << std::setw(10) << "meas/crlb" << std::setw(10) << "2ray"
                  << std::setw(9) << "nlos" << std::setw(11) << "shadowSD" << std::setw(11)
                  << "shadowEff\n";

        std::vector<double> pdrSeries;
        std::vector<double> shadowSdSeries;
        std::vector<double> shadowEffSeries;
        double worstCrlbRatio = 1e18;
        double worstCrlbDist = 0.0;
        double worstCrlbMae = 0.0;
        double worstCrlbFloor = 0.0;
        double worstCrlbSnr = 0.0;

        for (int i = 0; i < nd; ++i)
        {
            uint32_t rx = 0;
            double errSum = 0.0;
            uint32_t errN = 0;
            double snrSum = 0.0;
            uint32_t snrN = 0;
            // D2 (DEFECTS.md). snrSum above is accumulated only over DELIVERED
            // trials, which contradicts the convention stated at the diagnostic
            // block below and biases every consumer of mean_snr_db. The
            // unconditional accumulator runs ALONGSIDE it rather than replacing
            // it, so the conditioning bias can be measured at every point before
            // any consumer is switched over -- notably the CRLB gate, which reads
            // meanSnr and whose correct input is a design question, not a bug fix.
            double snrAllSum = 0.0;
            uint32_t snrAllN = 0;

            // Diagnostic accumulators. shadowSum/shadowSqSum give the sample
            // variance of the shadowing draws actually realised at this distance.
            uint32_t twoRayN = 0;
            uint32_t nlosN = 0;
            double plSum = 0.0;
            double marginSum = 0.0;
            double shadowSum = 0.0;
            double shadowSqSum = 0.0;
            double shadowEffSum = 0.0;
            uint32_t diagN = 0;

            for (int k = 0; k < trials; ++k)
            {
                const Time now = Seconds(static_cast<double>(k) * 1.0);
                bool received = false;
                double range = 0.0;
                double snr = 0.0;
                bool twoRay = false;
                bool nlos = false;
                double plDb = 0.0;
                double marginDb = 0.0;
                double shadowDb = 0.0;
                double shadowEffDb = 0.0;

                // Each trial samples a DIFFERENT position at the same range, by
                // rotating the azimuth. This is not cosmetic.
                //
                // The engine's shadowing is Gudmundson spatially-correlated. Held
                // at one fixed position, 400 trials share essentially one
                // shadowing draw -- only fast fading and the interferer coin
                // re-roll -- so each distance point inherits a single arbitrary
                // fade offset of 7-11 dB (x1.25-1.5 under NLOS). That offset
                // swamps the distance trend and produces a PDR curve that RISES
                // with distance, which is what the first probe run showed on all
                // three tiers. Rotating the azimuth decorrelates the shadowing
                // field between trials so the point is an average over positions
                // at that range, which is what a PDR-vs-distance figure means.
                const double az = 2.0 * M_PI * static_cast<double>(k) / static_cast<double>(trials);

                // Azimuth decorrelates the local channel; the site cycle
                // decorrelates the large-scale field. Both are needed -- see the
                // kSites comment above.
                const uint32_t site = static_cast<uint32_t>(k) % kSites;
                const uint32_t gwIdK = gwId + site;
                const Vector gwPos(siteXY[site].x, siteXY[site].y, gwH);
                const Vector pk(siteXY[site].x + ds[i] * std::cos(az),
                                siteXY[site].y + ds[i] * std::sin(az),
                                nodeH);

                if (tier == CA_TBL_TIER_868)
                {
                    e868.UpdateNodePosition(nodeId, pk);
                    PhysicsResult r = e868.ProcessTransmission(
                        nodeId, gwIdK, pk, gwPos, phy[0].txPowerDbm, phy[0].sf, phy[0].bandwidthHz, 0, now);
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    snr = r.snr_db;
                    twoRay = r.isTwoRay;
                    nlos = r.isNLOS;
                    plDb = r.pathLossDb;
                    marginDb = r.sensitivityMarginDb;
                    shadowDb = r.shadowingDb;
                    shadowEffDb = r.shadowingStdEff;
                }
                else if (tier == CA_TBL_TIER_2P4)
                {
                    e2p4.UpdateNodePosition(nodeId, pk);
                    PhysicsResult2p4 r = e2p4.ProcessTransmission(
                        nodeId, gwIdK, pk, gwPos, phy[1].txPowerDbm, phy[1].sf, phy[1].bandwidthHz, 0, now);
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    snr = r.snr_db;
                    twoRay = r.isTwoRay;
                    nlos = r.isNLOS;
                    plDb = r.pathLossDb;
                    marginDb = r.sensitivityMarginDb;
                    shadowDb = r.shadowingDb;
                    shadowEffDb = r.shadowingStdEff;
                }
                else
                {
                    eUwb.UpdateNodePosition(nodeId, pk);
                    PhysicsResultUWB r = eUwb.ProcessTransmission(
                        nodeId, gwIdK, pk, gwPos, phy[2].txPowerDbm, phy[2].sf, phy[2].bandwidthHz, 0, now);
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    snr = r.snr_db;
                    twoRay = r.isTwoRay;
                    nlos = r.isNLOS;
                    plDb = r.pathLossDb;
                    marginDb = r.sensitivityMarginDb;
                    shadowDb = r.shadowingDb;
                    shadowEffDb = r.shadowingStdEff;
                }

                // Channel diagnostics are accumulated over ALL trials, not only
                // received ones. Conditioning them on reception would select
                // exactly the favourable draws and hide the mechanism being looked
                // for: a distance point whose PDR is low BECAUSE its shadowing
                // draws were unlucky would report a healthy mean shadowing.
                if (std::isfinite(plDb))
                {
                    ++diagN;
                    twoRayN += twoRay ? 1u : 0u;
                    nlosN += nlos ? 1u : 0u;
                    plSum += plDb;
                    marginSum += marginDb;
                    if (std::isfinite(snr))
                    {
                        snrAllSum += snr;
                        ++snrAllN;
                    }
                    shadowSum += shadowDb;
                    shadowSqSum += shadowDb * shadowDb;
                    shadowEffSum += shadowEffDb;
                }

                if (received)
                {
                    ++rx;
                    if (std::isfinite(snr))
                    {
                        snrSum += snr;
                        ++snrN;
                    }
                    if (std::isfinite(range) && range > 0.0)
                    {
                        const double trueD =
                            std::sqrt(ds[i] * ds[i] + (gwPos.z - nodeH) * (gwPos.z - nodeH));
                        errSum += std::fabs(range - trueD);
                        ++errN;
                    }
                }
            }

            const double pdr = static_cast<double>(rx) / static_cast<double>(trials);
            const double mae = (errN > 0) ? errSum / static_cast<double>(errN) : -1.0;
            // D3 (DEFECTS.md). 0.0 was indistinguishable from a genuine 0 dB
            // measurement; NaN is not a value the quantity can take. Every
            // consumer below is already guarded on snrN > 0, so NaN reaches the
            // CSV only where no trial was delivered and nothing was measured.
            const double meanSnr = (snrN > 0) ? snrSum / static_cast<double>(snrN)
                                              : std::numeric_limits<double>::quiet_NaN();
            const double meanSnrAll = (snrAllN > 0) ? snrAllSum / static_cast<double>(snrAllN)
                                                    : std::numeric_limits<double>::quiet_NaN();

            const double dN = static_cast<double>(diagN);
            const double twoRayFrac = (diagN > 0) ? static_cast<double>(twoRayN) / dN : -1.0;
            const double nlosFrac = (diagN > 0) ? static_cast<double>(nlosN) / dN : -1.0;
            const double meanPl = (diagN > 0) ? plSum / dN : 0.0;
            const double meanMargin = (diagN > 0) ? marginSum / dN : 0.0;
            const double meanShadowEff = (diagN > 0) ? shadowEffSum / dN : 0.0;
            double shadowSd = -1.0;
            if (diagN > 1)
            {
                const double m = shadowSum / dN;
                const double var = std::max(0.0, shadowSqSum / dN - m * m);
                shadowSd = std::sqrt(var);
            }

            // 868 broadcasts one frame that anchors timestamp (one-way TDoA);
            // 2.4 GHz and UWB run symmetric two-way exchanges.
            const bool twoWay = (tier != CA_TBL_TIER_868);
            // NOT COMPUTED ON TIER 1 -- report §10.8, remediation item 7.
            //
            // The ToA Cramer-Rao bound is a bound on TIMING estimation. Tier 1
            // does not estimate timing: it inverts RSSI through the path-loss
            // model, and its error derives from the shadowing sigma, not from
            // bandwidth and SNR. lora-physics-engine.cc:468 says so explicitly at
            // the call site -- "RSSI-inversion ranging is not a time-of-arrival
            // estimator and the ToA Cramer-Rao bound does not apply to it" -- and
            // the engine passes snrDbInBand = 0.0 into the tier-1 branch for that
            // reason.
            //
            // Comparing tier 1's realised error against a ToA bound is therefore a
            // category error: the ratio is not a statement about the ranging model,
            // it is a statement about two unrelated quantities. It was also
            // reported as a PASS, which is worse than reporting nothing -- a gate
            // that cannot fail on a tier it does not apply to is a gate that
            // manufactures confidence.
            const double crlb =
                (snrN > 0 && tier != CA_TBL_TIER_868)
                    ? CrlbRangeStdM(phy[t].bandwidthHz, meanSnr + gainDb, twoWay)
                    : -1.0;
            const double ratio = (crlb > 0.0 && mae > 0.0) ? mae / crlb : -1.0;

            // PDR floor on the CRLB gate. The mean range error at a point is
            // conditioned on the trials that DECODED, and at low PDR that is a
            // survivorship-biased sample: a link whose shadowing draw was
            // favourable enough to decode at the edge of coverage also ranges
            // better than a typical link at that distance. The ratio measured/floor
            // is then not a statement about the ranging model.
            //
            // This became load-bearing when dists2p4 was extended from 900 m to
            // 6000 m to un-censor r80. The gate's reported worst case moved from
            // 900 m at pdr 0.804 to 4000 m at essentially zero pdr, and reported
            // 0.612 -- a pass, computed on a sample that cannot support the claim.
            // Extending the sweep was correct; letting the gate follow it into the
            // tail was not.
            //
            // 0.20 is OURS. It is not a physical threshold, it is the point below
            // which the conditioning is severe enough that the number stops meaning
            // what the gate reads it as meaning.
            const double kCrlbMinPdr = 0.20;
            if (ratio > 0.0 && pdr >= kCrlbMinPdr && ratio < worstCrlbRatio)
            {
                worstCrlbRatio = ratio;
                worstCrlbDist = ds[i];
                worstCrlbMae = mae;
                worstCrlbFloor = crlb;
                worstCrlbSnr = meanSnr;
            }

            out << CaTblTierName(tier) << "," << std::fixed << std::setprecision(1) << ds[i] << ","
                << std::setprecision(4) << pdr << "," << std::setprecision(3) << mae << ","
                << std::setprecision(2) << meanSnr << "," << std::setprecision(3) << crlb << ","
                << std::setprecision(4) << ratio << "," << trials << ","
                << std::setprecision(2) << gainDb << "," << std::setprecision(4) << twoRayFrac
                << "," << nlosFrac << "," << std::setprecision(2) << meanPl << "," << meanMargin
                << "," << std::setprecision(3) << shadowSd << "," << meanShadowEff;
            if (traceDiagnostic)
            {
                out << "," << std::setprecision(2) << meanSnrAll;
            }
            out << "\n";
            std::cout << "    " << std::right << std::fixed << std::setprecision(1) << std::setw(10)
                      << ds[i] << std::setprecision(3) << std::setw(10) << pdr << std::setw(16)
                      << mae << std::setprecision(1) << std::setw(10) << meanSnr
                      << std::setprecision(3) << std::setw(14) << crlb << std::setw(10) << ratio
                      << std::setprecision(3) << std::setw(10) << twoRayFrac << std::setw(9)
                      << nlosFrac << std::setprecision(2) << std::setw(11) << shadowSd
                      << std::setw(11) << meanShadowEff << "\n";
            pdrSeries.push_back(pdr);
            shadowSdSeries.push_back(shadowSd);
            shadowEffSeries.push_back(meanShadowEff);
        }

        // Monotonicity gate. PDR must not rise materially with distance. When it
        // does, the sweep is not measuring distance -- it is measuring whatever
        // per-point state failed to re-randomise. The first probe run violated
        // this on all three tiers because each distance point reused one
        // spatially-correlated shadowing draw; that is a real class of defect and
        // it must fail loudly rather than be eyeballed off a table.
        //
        // Tolerance is 0.10 absolute: with 400 trials the binomial standard error
        // near pdr=0.5 is ~0.025, so 0.10 is ~4 sigma and will not fire on noise.
        uint32_t violations = 0;
        double worst = 0.0;
        for (size_t i = 1; i < pdrSeries.size(); ++i)
        {
            const double rise = pdrSeries[i] - pdrSeries[i - 1];
            if (rise > 0.10)
            {
                ++violations;
                worst = std::max(worst, rise);
            }
        }
        if (violations > 0)
        {
            std::cout << "    *** MONOTONICITY VIOLATION: " << violations
                      << " distance step(s) where PDR ROSE by >0.10 (worst +" << std::fixed
                      << std::setprecision(3) << worst << ").\n"
                      << "        This sweep is not measuring distance. Do not read the\n"
                      << "        range gates below until it is fixed.\n";
        }
        else
        {
            std::cout << "    monotonicity OK (no distance step raises PDR by >0.10)\n";
        }

        // ---- Shadowing independence gate -----------------------------------
        // A monotonicity verdict is only meaningful if the per-point PDRs have the
        // precision the trial count implies. They do not if the trials at a point
        // share shadowing draws.
        //
        // The azimuth rotation decorrelates the Gudmundson field by moving the node
        // 2*pi*d/trials per trial, which is proportional to d. It is therefore
        // strong at tier-1 ranges and weak at UWB ranges: at d = 2 m the entire
        // 2000-trial circle is 12.6 m of path. If the realised sample stddev of the
        // shadowing draws is far below the effective sigma the engine thinks it is
        // applying, the effective sample size is not 2000 and no sigma-based
        // reading of a PDR difference is defensible.
        //
        // Threshold 0.5: a sample stddev below half the effective sigma means the
        // point is averaging over roughly four or fewer independent draws.
        uint32_t shadowFlags = 0;
        double worstShadowRatio = 1e18;
        double worstShadowDist = 0.0;
        for (size_t i = 0; i < shadowSdSeries.size(); ++i)
        {
            if (shadowEffSeries[i] <= 0.0 || shadowSdSeries[i] < 0.0)
            {
                continue;
            }
            const double rho = shadowSdSeries[i] / shadowEffSeries[i];
            if (rho < worstShadowRatio)
            {
                worstShadowRatio = rho;
                worstShadowDist = ds[i];
            }
            if (rho < 0.5)
            {
                ++shadowFlags;
            }
        }
        if (shadowFlags > 0)
        {
            const double nEff = (worstShadowRatio > 0.0)
                                    ? 1.0 / (worstShadowRatio * worstShadowRatio)
                                    : 0.0;
            std::cout << "    *** SHADOWING NOT DECORRELATED at " << shadowFlags
                      << " distance point(s). Worst: realised stddev is "
                      << std::fixed << std::setprecision(2) << worstShadowRatio
                      << "x the effective sigma at " << std::setprecision(1) << worstShadowDist
                      << " m,\n"
                      << "        implying on the order of " << std::setprecision(0) << nEff
                      << " independent draws rather than " << trials << ".\n"
                      << "        The azimuth rotation moves the node only 2*pi*d/trials per\n"
                      << "        trial, so it is geometrically too weak at short range. PDR at\n"
                      << "        these points is a few arbitrary shadowing draws, NOT a\n"
                      << "        2000-trial average, and the binomial standard error of 0.011\n"
                      << "        does not apply. Do not attribute a PDR step at these\n"
                      << "        distances to distance.\n";
        }
        else if (worstShadowRatio < 1e17)
        {
            std::cout << "    shadowing decorrelation OK (worst realised/effective sigma = "
                      << std::fixed << std::setprecision(2) << worstShadowRatio << ")\n";
        }

        // ---- CRLB verdict --------------------------------------------------
        // Flagged at ratio < 1/3: beating the single-shot bound by up to ~3x is
        // explainable by averaging (N samples buys sqrt(N), so 3x needs N~9), but
        // below that the model is claiming information its bandwidth does not
        // carry. A tier reporting unphysical precision silently wins every
        // architectural comparison, so this must be loud.
        if (tier == CA_TBL_TIER_868)
        {
            // Report §10.8 / remediation item 7. Say why it is absent rather than
            // silently omitting it, so a reader does not conclude the gate was
            // forgotten.
            std::cout << "    CRLB: NOT APPLICABLE on " << CaTblTierName(tier)
                      << " -- RSSI-inversion is not a\n"
                      << "        time-of-arrival estimator, so the ToA Cramer-Rao bound does not\n"
                      << "        bound it. Previously this tier was measured against that bound\n"
                      << "        and reported as passing; that was a category error.\n";
        }
        else if (worstCrlbRatio < 1e17)
        {
            std::cout << "    CRLB (points with pdr >= 0.20 only): worst measured/floor = "
                      << std::fixed << std::setprecision(3)
                      << worstCrlbRatio << " at " << std::setprecision(1) << worstCrlbDist
                      << " m (measured " << std::setprecision(3) << worstCrlbMae << " m vs floor "
                      << worstCrlbFloor << " m at " << std::setprecision(1) << worstCrlbSnr
                      << " dB SNR)\n";
            if (worstCrlbRatio < 0.333)
            {
                const double impliedN = 1.0 / (worstCrlbRatio * worstCrlbRatio);
                std::cout << "    *** CRLB VIOLATION on " << CaTblTierName(tier) << ": reported "
                          << "precision is " << std::setprecision(1) << (1.0 / worstCrlbRatio)
                          << "x better than the\n"
                          << "        Cramer-Rao bound for " << phy[t].bandwidthHz / 1e3
                          << " kHz at " << std::setprecision(2) << gainDb
                          << " dB coherent gain. Explaining it would\n"
                          << "        need ~" << std::setprecision(0) << impliedN
                          << " independent\n"
                          << "        averaged samples per fix. This tier's ranging model is not\n"
                          << "        traceable to its bandwidth -- it will dominate every\n"
                          << "        architectural comparison for a non-physical reason.\n"
                          << "        DO NOT read the arm table until this is resolved.\n";
            }
            else
            {
                // ONE-SIDED, AND SAY SO. Report §10.8: this printed "within 3x of
                // the single-shot bound" for ANY ratio >= 0.333, including ratios
                // far ABOVE 3 -- a tier ranging ten times WORSE than the bound was
                // reported as "within 3x", which is simply false. The test only
                // ever asks whether the model claims precision its bandwidth
                // cannot carry; it says nothing about a tier that is merely bad.
                if (worstCrlbRatio > 3.0)
                {
                    std::cout << "    CRLB: not below the bound (one-sided test passes at "
                              << std::fixed << std::setprecision(2) << worstCrlbRatio
                              << "x the floor).\n"
                              << "        NOTE: this is ABOVE 3x, i.e. the tier ranges considerably"
                              << " worse than\n        the bound. That is not flagged here -- this"
                              << " gate only detects\n        unphysically GOOD precision.\n";
                }
                else
                {
                    std::cout << "    CRLB OK: " << std::fixed << std::setprecision(2)
                              << worstCrlbRatio << "x the single-shot bound (one-sided test;"
                              << " only ratios < 0.333 are flagged)\n";
                }
            }
        }

        // Measured usable range, so anchor spacing is sized from this sweep rather
        // than from an assumed datasheet figure. r80 is the planning number (a
        // ranging exchange needs most attempts to land); r50 is where the tier is
        // already unreliable.
        // Grid-snapped r80 was unstable: two runs of the identical configuration gave
        // tier-1 r80 = 1500 m and 1000 m, because PDR at 1500 m straddles 0.80
        // (0.813 vs 0.797). At n=2000 the binomial SE near pdr=0.8 is 0.0089, so a
        // 0.016 difference is 1.8 sigma -- ordinary noise, but a hard threshold on a
        // coarse grid turns it into a factor-of-1.5 jump in the number the deployment
        // geometry is sized from.
        //
        // Linear interpolation on the bracketing pair instead. It does not remove the
        // sampling noise, but it converts a discontinuous estimator into a continuous
        // one, so the noise shows up as a few percent rather than a whole grid step.
        // Still censored at the sweep edge: if the last point is above the threshold
        // the returned value is that last distance and is a LOWER BOUND.
        //
        // Censoring is now RETURNED, not left as a comment. The previous version
        // printed a censored lower bound in exactly the same format as a measured
        // crossing, and the consequence is on record: tier-2 "r80 = 900 m" was read
        // and quoted as a measurement for a whole session when the sweep had simply
        // run out of distance points. A bound and a measurement must not print the
        // same. This is the same failure mode as the grid-snapping bug above, one
        // level up: the estimator was fine, its reporting was not.
        auto interpRange = [&](double thresh, bool& censored) {
            censored = false;
            double last = 0.0;
            for (int i = 0; i < nd; ++i)
            {
                const double p = pdrSeries[static_cast<size_t>(i)];
                if (p >= thresh)
                {
                    last = ds[i];
                    continue;
                }
                if (i == 0)
                {
                    return 0.0;
                }
                const double pPrev = pdrSeries[static_cast<size_t>(i - 1)];
                const double denom = pPrev - p;
                if (denom <= 0.0)
                {
                    return last;
                }
                return ds[i - 1] + (ds[i] - ds[i - 1]) * (pPrev - thresh) / denom;
            }
            // Never fell below the threshold anywhere in the sweep.
            censored = true;
            return last;
        };
        bool r80Censored = false;
        bool r50Censored = false;
        const double r80 = interpRange(0.80, r80Censored);
        const double r50 = interpRange(0.50, r50Censored);
        std::cout << "    usable range: r80" << (r80Censored ? ">=" : "=") << std::fixed
                  << std::setprecision(1) << r80 << " m   r50" << (r50Censored ? ">=" : "=") << r50
                  << " m\n";
        if (r80Censored || r50Censored)
        {
            std::cout << "    *** CENSORED: PDR is still above the threshold at the last swept\n"
                      << "        distance (" << std::setprecision(1) << ds[nd - 1]
                      << " m). The value above is a LOWER BOUND, not a\n"
                      << "        measurement. Extend this tier's distance array before sizing\n"
                      << "        any geometry, or quoting any range, from it.\n";
        }
        if (tier == CA_TBL_TIER_UWB)
        {
            // A node inside a facility sits at roughly the triad radius from each
            // anchor, and needs 3 of 3 to solve. Sizing rule: triad radius must be
            // inside r80, not merely inside r50, or the tier will be intermittent
            // exactly where it is supposed to be the accuracy ceiling.
            std::cout << "    => set --uwbTriad no larger than r80 (" << r80
                      << " m). Reference spec is 100 m LOS / 30 m NLOS\n"
                      << "       (simstatus.md Bug #9); which of those this run "
                         "resembles tells you\n"
                      << "       whether the facility model is behaving as LOS or NLOS.\n";
        }
        if (tier == CA_TBL_TIER_2P4)
        {
            std::cout << "    => set --t2Triad no larger than r80 (" << r80 << " m).\n";
        }
        std::cout << "\n";
    }

    out.close();
    std::cout << "Wrote " << outdir << "/probe_range_sweep.csv\n\n"
              << "How to act on this:\n"
              << "  1. Monotonicity must be OK on all three tiers. If not, nothing below\n"
              << "     is measuring distance -- fix that first.\n"
              << "  2. Size the anchor triads from the measured r80 values, not from\n"
              << "     datasheet range figures. --uwbTriad <= uwb r80, --t2Triad <= 2p4 r80.\n"
              << "  3. Tier 1 needs r80 to cover the gateway spacing that BuildDeployment()\n"
              << "     uses (0.9 * boxHalf, so ~1900 m diagonal at the default 3 km box).\n"
              << "     If 868 r80 is well below that, reduce --boxHalf rather than pretend\n"
              << "     the link closes.\n"
              << "  4. Ranging error is reported alongside PDR for a reason. A tier whose\n"
              << "     mean range error is a large fraction of the true distance is not\n"
              << "     contributing ranging information even when its PDR is 1.0 -- its\n"
              << "     fixes come from multilateration geometry, not from link ranging.\n"
              << "     Read the 868 column with that in mind before reading any RMSE.\n";
}

/**
 * \brief Run one arm end to end.
 *
 * The engines are re-created per arm from the same seed, and Tourist draws from an
 * explicitly-streamed RNG, so every arm walks the IDENTICAL TRAJECTORY.
 *
 * It does NOT follow that every arm sees the identical channel realisation, and
 * the previous version of this comment claimed that it did. The engines seed a
 * private LoRaDistributionSampler from config.randomSeed, so all arms read the
 * SAME STREAM -- but they consume it at arm-dependent RATES, because which tier is
 * polled, how many anchors are addressed, and (since the MAC was wired) how much
 * beacon traffic is evaluated all depend on the arm's own behaviour. Arm A's tenth
 * draw and arm B's tenth draw are the same number; they are not applied to the
 * same link at the same time.
 *
 * Consequence, and it is not small: a cross-arm RMSE difference has ALWAYS carried
 * a channel-draw component, so no single-seed comparison between arms isolates the
 * policy. The 20-seed sweep is not a refinement of the single-seed table, it is
 * the minimum evidence for any claim that compares two arms.
 *
 * Note also that ns-3's RngSeedManager::SetSeed() does NOT reset the automatic
 * stream index -- ResetNextStreamIndex() is a separate call that nothing here
 * makes (rng-seed-manager.cc). Any RandomVariableStream created without an
 * explicit SetStream() would therefore land on a different substream on every arm.
 * Every such object in this file sets its stream explicitly, before first use, for
 * exactly that reason. Do not add one that does not.
 */
/**
 * \brief Everything belonging to ONE target.
 *
 * Promoted out of RunArm so a second target is a MEASURED subject rather than a
 * protocol peer: its own filter, its own confidence score, its own energy account
 * and its own per-cycle statistics. The section 6 invariant becomes per-tag --
 * exactly one tier transmits per cycle PER TAG, and only that tier is charged on
 * that tag account.
 *
 * The trajectory is a pointer because Tourist has no default constructor. The
 * objects pointed into are owned by a deque in RunArm, so push_back cannot
 * invalidate a pointer already handed out.
 */
struct CaTblTagState
{
    uint32_t nodeId = 0;
    Tourist* walker = nullptr;
    Vector pos;
    CaTblEnergyModel energy;
    CaTblPolicy policy;
    CaTblMac* nodeMac[CA_TBL_TIER_COUNT] = {nullptr, nullptr, nullptr};

    double beaconToaS[CA_TBL_TIER_COUNT];
    uint64_t lastRx[CA_TBL_TIER_COUNT] = {0, 0, 0};
    double lastNonEmptyAtS[CA_TBL_TIER_COUNT] = {-1.0, -1.0, -1.0};
    double nbrSum[CA_TBL_TIER_COUNT] = {0.0, 0.0, 0.0};
    uint64_t blindCycles = 0;
    uint32_t floorHealthyStreak = 0;
    uint64_t cycleIndex = 0;
    uint64_t floorRangedCycles = 0;
    uint64_t handshakeDeferrals = 0;
    uint64_t handshakeIntraCycleDefers = 0;
    double handshakeIntraCycleWaitS = 0.0;
    double handshakeDeferralDelayS = 0.0;
    uint64_t handshakePreemptions = 0;
    uint64_t escalationsDeferredAway = 0;
    bool pendingHandshake = false;
    CaTblTier pendingHandshakeTier = CA_TBL_TIER_868;
    uint64_t directEngineCalls = 0;
    //! C4: this tag's own arbitration terms. See the note at the use site.
    std::deque<double> pcsHist;
    double pcsDegradation = 0.0;
    double batteryUsedFrac = 0.0;
    std::vector<double> errors;
    std::vector<double> errorsFixed;
    uint64_t steps = 0;
    uint64_t tp = 0;
    uint64_t fp = 0;
    uint64_t fn = 0;
    uint64_t tn = 0;
    uint64_t inCov = 0;
    uint64_t staleSteerRefusals = 0;
    double lostContactSinceS = -1.0;
    double lowConfSinceS = -1.0;
    bool partyDeployed = false;
    double deployedAtS = -1.0;
    // 0 = no fix last cycle, or the weighted solve went degenerate. Carried across
    // cycles because the alert is evaluated before this cycle's decision exists,
    // which is also how a real base camp works: it acts on the last report it has.
    double lastReportedSigmaM = -1.0;
    bool lastReportHadFix = false;
    const char* deployCause = "";
    bool batteryDead = false;
    double batteryDeadAtS = -1.0;
    CaTblNodeLifecycle lifecycle;
    CaTblSessionManager sessions;
    uint32_t sessionId = 0;
    uint64_t renderCounts[4] = {0, 0, 0, 0};
    uint64_t caseAAttempts = 0;
    uint64_t caseSolved[5] = {0, 0, 0, 0, 0}; // [0]=D 3+ [1]=A [2]=B [3]=C [4]=floor
    double prevMeanRangeM = -1.0;
    uint64_t mobCounts[3] = {0, 0, 0};
    uint64_t caseAAdmitted = 0;
    uint64_t caseARefused[4] = {0, 0, 0, 0};
};

/**
 * C5_INSTANT -- THE RELAYS' ROUND, GIVEN INSTANTS OF ITS OWN, under --relaySlots.
 *
 * ON BY DEFAULT IN EVERY RELAYED RUN SINCE 2026-09-30 (--relaySlots=auto; the owner's
 * decision on TECH_DEBT M.1). It was default off from d24690c until then, so every relayed
 * run before that date without an explicit --relaySlots=true was measured under the defect
 * below. --relaySlots=false restores it, byte-for-byte against the build before the change.
 *
 * THE DEFECT. Every exchange a cycle makes by calling an engine directly carried the same
 * `now`, and the engines' interference registry (lora-interference-submodule.cc:49-162)
 * decides overlap on time and transmitter id alone. It never attenuates an interferer by
 * its distance from the receiver (the `dist` it computes at :129 is unused): a concurrent
 * exchange interferes at the power of its OWN link. So each relay's polls collided with
 * every exchange registered before them in that instant -- the other relays' and the
 * measured tag's. Measured at 86e43f2 with only --relayedRanging switched on (static_2p4,
 * 300 s, --boxHalf=200, four tags): the measured tag's 3+-anchor cycles fall from
 * 107/112/106 of 120 (seeds 7/11/13) to 2/0/11, and relays 3 and 4 never hold one fix
 * (their trace(P) is the pure-prediction value). Under --relayChain no relay-to-relay
 * exchange can deliver, so no chain fuses and depth 2 never arises (G17b, since b390ed6).
 *
 * THE SCHEDULE. On, the relays' round takes slots of its own at the END of the cycle,
 * finishing before the instant the measured tag ranges at. That is the order the code
 * already assumes: the tag's relayed leg reads every relay's estimate of THIS cycle
 * (relayAgeS 0), and a short relay reads the holders' -- so their exchanges come first.
 * In evaluation order, each instant later than the last:
 *   relay pt's anchor polls           at now - (K - pt + 1) * S,   pt = 1 .. n-1
 *   the j-th relay-to-relay exchange  at now - (C - j) * S,        j = 0 .. C-1
 *   the measured tag, relayed leg included, at now, exactly as before
 * with n = --nTags, C = (n-1)(n-2) under --relayChain (each short relay may range every
 * other relay) else 0, K = (n-1) + C, and S = FixRouter::kSlotS, 50 ms: L.1.1's frame slot,
 * longer than the engines' nominal airtime (2^sf/bw * 20, lora-*-physics-engine.cc:~560)
 * on every tier at SF7 -- 868 MHz 20.48 ms, 2.4 GHz 3.2 ms -- so no two transmitters'
 * registry intervals overlap. The slot, the order and the fixed layout are OURS; the spec
 * defines no schedule. The run is refused when the round does not fit inside --dt or the
 * relay tier's airtime is not below S.
 *
 * WHY THE END OF THE CYCLE AND NOT AFTER THE INSTANT, where L.1.1's frames go. After it,
 * the relays' exchanges would follow the tag that already read their results; and the
 * first one evaluated would purge from the registry the MAC frames straddling the instant
 * (the purge drops entries ended before the CURRENT call), before the measured tag -- at
 * the instant -- was evaluated against them. Instants that rise in evaluation order keep
 * that purge what it is in every other run: bookkeeping that cannot change a result.
 *
 * A RELAY'S POLLS SHARE ONE INSTANT, as the measured tag's always have: the registry never
 * collides a transmitter with itself, so spreading a burst would change nothing it sees.
 *
 * WHAT DOES NOT MOVE. Every call, its order and its arguments but the instant. Nothing is
 * added, removed or re-ordered, so the number of engine evaluations is the flag-off
 * number for the same outcomes. Each exchange is charged exactly as before -- to the relay
 * that sent it, on --relayTier, inside the per-cycle loop (CLAUDE.md sect 6) -- with its
 * duty governor armed from the instant it took. The measured tag and L.1.1's frames stay.
 *
 * KNOWN LIMIT. The round sits in the slice the event layer has ALREADY run. A MAC frame
 * completing in its last K*S was evaluated before the round was registered, and every
 * later MAC evaluation purged it, so the round meets no MAC frame and no MAC frame meets
 * the round. Flag off, a direct exchange at the instant meets only the frames straddling
 * it -- at most one airtime of look-back. In G17b's regime the MAC layer sends 30 051
 * frames in 600 s over three tiers (~50/s), which bounds that look-back at 50/s x 3.2 ms,
 * about 16% of exchanges on 2.4 GHz and less on the one tier: that is the sliver of MAC
 * interference the relays give up in the round. The measured tag keeps its sliver.
 *
 * Off -- --relaySlots=false, or any run without a relay to schedule -- this is never
 * constructed: every instant is `now` and every charge time `t`.
 */
struct RelayRound
{
    RelayRound(uint32_t nTags, bool chain, double slotS);
    void BeginCycle();
    //! Relay pt's anchor polls, one instant for the burst.
    Time Burst(Time now, size_t pt) const;
    //! The same instant in seconds, for the charge that arms the relay's governor.
    double BurstS(double t, size_t pt) const;
    //! The next relay-to-relay exchange of this cycle; its charge time goes to *atS. Past
    //! the C reserved slots the run stops: a slot the round does not have would be the
    //! collision it exists to remove.
    Time NextChain(Time now, double t, double* atS);
    void Report(std::ostream& os, double dt) const;

    double slotS;
    uint64_t slotNs;
    uint32_t relays;          //!< n - 1
    uint32_t chainSlots;      //!< C
    uint32_t chainUsed = 0;   //!< this cycle
    uint32_t chainPeak = 0;
    uint64_t polls = 0;       //!< anchor polls evaluated in a relay's slot
    uint64_t bursts = 0;      //!< relay-cycles that polled at least one anchor
    uint64_t chainMoved = 0;  //!< relay-to-relay exchanges evaluated in a round slot
};

__attribute__((noinline))
RelayRound::RelayRound(uint32_t nTags, bool chain, double s)
    : slotS(s),
      slotNs(static_cast<uint64_t>(std::llround(s * 1e9))),
      relays(nTags > 1 ? nTags - 1 : 0),
      chainSlots((chain && nTags > 2) ? (nTags - 1) * (nTags - 2) : 0)
{
}

__attribute__((noinline)) void
RelayRound::BeginCycle()
{
    chainUsed = 0;
}

__attribute__((noinline)) Time
RelayRound::Burst(Time now, size_t pt) const
{
    const uint64_t k = static_cast<uint64_t>(relays + chainSlots) + 1u - pt;
    return now - NanoSeconds(slotNs * k);
}

__attribute__((noinline)) double
RelayRound::BurstS(double t, size_t pt) const
{
    return t - slotS * static_cast<double>(static_cast<uint64_t>(relays + chainSlots) + 1u - pt);
}

__attribute__((noinline)) Time
RelayRound::NextChain(Time now, double t, double* atS)
{
    if (chainUsed >= chainSlots)
    {
        std::cerr << "FATAL: relay-to-relay exchange " << (chainUsed + 1)
                  << " in one cycle, but the --relaySlots round reserves " << chainSlots
                  << "\n";
        std::exit(1);
    }
    const uint64_t k = chainSlots - chainUsed;
    ++chainUsed;
    chainPeak = std::max(chainPeak, chainUsed);
    ++chainMoved;
    *atS = t - slotS * static_cast<double>(k);
    return now - NanoSeconds(slotNs * k);
}

__attribute__((noinline)) void
RelayRound::Report(std::ostream& os, double dt) const
{
    // Its own stream, so std::fixed does not outlive the call on std::cout.
    std::ostringstream o;
    const uint32_t k = relays + chainSlots;
    o << "    C5_INSTANT relay round: slot " << std::fixed << std::setprecision(1)
      << slotS * 1e3 << " ms x " << k << " = " << slotS * 1e3 * k
      << " ms before the instant, of a " << dt * 1e3 << " ms cycle; anchor polls moved "
      << polls << " in " << bursts << " bursts; relay-to-relay moved " << chainMoved
      << ", at most " << chainPeak << " of " << chainSlots << " slots in a cycle\n";
    os << o.str();
}

/**
 * NW-1 step 3 (--hopVarianceTerms). The final plan's item 1 models a fix's confidence as a
 * function of the path it travelled -- "hop count, per-hop RSSI/SNR variance, per-hop NLOS
 * likelihood" -- and Physical sect 2.5 folds that into trace(Sigma). C5 already adds the
 * relay's projected position variance (sigma_eff^2 = sigma^2 + u'P_B u); this adds the two
 * per-hop terms to the leg's own sigma, before C5 composes it:
 *   - THE LEG'S VARIABILITY. The von Neumann sigma of its RSSI and SNR readings, the
 *     statistic tier 1's classifier uses (window 8, three readings, the larger of the two),
 *     keyed by (initiator, relay). Converted to range by the tier's own nominal path-loss
 *     slope at the measured range: sigma_var = s / |dPL/dd|. The conversion is OURS: tiers
 *     2 and 3 range by time of flight, so RSSI variability measures how unsettled the leg
 *     is, not a term of the range error itself, and on short legs it is large.
 *   - THE TIER'S NLOS SCORE ON THE LEG. 2.4 GHz: the ToF/RSSI excess against a pool of legs
 *     (200, scoring from 20, as the anchor links' pool); UWB: the leading-edge test on a
 *     synthesised CIR. A leg scored at or above --nlosScoreThreshold has its sigma
 *     multiplied by nlosSigmaInflation, as Step() multiplies a direct NLOS link's.
 * sigma_leg = sqrt(sigma_raw^2 + sigma_var^2), times the inflation when NLOS, and C5 adds
 * u'P_B u to that. Its own windows, pool and CIR stream: the anchor links' classifier
 * (nlos_links_*) does not see a leg.
 */
struct LegTerms
{
    //! What the engine returned on the leg.
    struct Obs
    {
        int tier = -1;
        double rssiDbm = 0.0;
        double snrDb = 0.0;
        double txPowerDbm = 0.0;
        bool truthNlos = false;
        double kFactor = 0.0;
        double nlosDb = 0.0;
    };
    //! The terms, one per leg that reached admission.
    struct Out
    {
        double rssiDbm = 0.0;
        double snrDb = 0.0;
        double rangeM = 0.0;
        uint32_t n = 0;
        double sDb = 0.0;
        double slopeDbPerM = 0.0;
        double varSigmaM = 0.0;
        double score = -1.0;
        bool nlos = false;
        double sigmaRawM = 0.0;
        double sigmaLegM = 0.0;
    };

    LegTerms(uint32_t seed,
             double threshold,
             double inflation,
             const LoRa2p4PhysicsConfig& c2p4,
             const LoRaUWBPhysicsConfig& cUwb)
        : thr(threshold),
          infl(inflation),
          cir(static_cast<int64_t>(seed) * 7919 + 1400071, CaTblCirConfig()),
          var(8, 3),
          ref2p4(200, 20),
          prop2p4(c2p4.propagation),
          propUwb(cUwb.propagation),
          envUwb(cUwb.environment),
          accumGainUwbDb(10.0 * std::log10(std::max(cUwb.ranging.uwbPreambleSymbols, 1.0)))
    {
    }

    Out Apply(uint32_t tx, uint32_t rx, const Obs& o, double rangeM, double sigRaw)
    {
        Out out;
        out.rssiDbm = o.rssiDbm;
        out.snrDb = o.snrDb;
        out.rangeM = rangeM;
        out.sigmaRawM = sigRaw;
        out.sigmaLegM = sigRaw;
        if (o.tier != CA_TBL_TIER_2P4 && o.tier != CA_TBL_TIER_UWB)
        {
            return out; // tier 1: A1 refuses the leg whatever its terms
        }
        ++legs;
        const CaTblLinkVariability::Result v =
            var.Push(((tx & 0xFFFFu) << 16) | (rx & 0xFFFFu), o.rssiDbm, o.snrDb);
        out.n = v.n;
        const PropagationParams& prop = (o.tier == CA_TBL_TIER_2P4) ? prop2p4 : propUwb;
        // Central difference, +-1 % of the range; the nominal model clamps its distance at
        // 1 m, so the range is held where both points clear that.
        const double h = 0.01;
        const double d = std::max(rangeM, 1.0 / (1.0 - h));
        out.slopeDbPerM = std::fabs(CaTblNominalPathLossDb(d * (1.0 + h), prop) -
                                    CaTblNominalPathLossDb(d * (1.0 - h), prop)) /
                          (2.0 * h * d);
        if (v.valid && out.slopeDbPerM > 0.0)
        {
            out.sDb = std::max(v.sRssiDb, v.sSnrDb);
            out.varSigmaM = out.sDb / out.slopeDbPerM;
            ++withVar;
            varSum += out.varSigmaM;
        }
        if (o.tier == CA_TBL_TIER_2P4)
        {
            const CaTblTofRssiResidual e =
                CaTblTofRssiExcess(rangeM, o.rssiDbm, o.txPowerDbm, prop2p4);
            if (e.valid)
            {
                double z = 0.0;
                double med = 0.0;
                double sig = 0.0;
                if (ref2p4.Score(e.excessDb, z, med, sig))
                {
                    out.score = CaTblNlosRamp(z, 1.0, 3.0); // NlosScoring's kZLo, kZHi
                }
                ref2p4.Add(e.excessDb);
            }
        }
        else
        {
            out.score = cir.Classify(envUwb, o.truthNlos, o.kFactor, o.nlosDb,
                                     o.snrDb + accumGainUwbDb)
                            .score;
        }
        if (out.score >= 0.0)
        {
            ++scored;
            out.nlos = (out.score >= thr);
        }
        out.sigmaLegM = std::sqrt(sigRaw * sigRaw + out.varSigmaM * out.varSigmaM) *
                        (out.nlos ? infl : 1.0);
        inflated += out.nlos ? 1 : 0;
        rawSum += sigRaw;
        legSum += out.sigmaLegM;
        return out;
    }

    void Report(std::ostream& os, const char* arm) const
    {
        std::ostringstream o;
        o << std::fixed << std::setprecision(4) << "  NW-1 hop variance terms (" << arm
          << "): legs " << legs << "; a variance term on " << withVar << " (mean sigma_var "
          << (withVar ? varSum / withVar : 0.0) << " m); NLOS-scored " << scored
          << ", inflated " << inflated << " (x" << infl << " at score >= " << thr
          << "); mean sigma raw " << (legs ? rawSum / legs : 0.0) << " m -> leg "
          << (legs ? legSum / legs : 0.0) << " m\n";
        os << o.str();
    }

    double thr;
    double infl;
    CaTblCirSynth cir;
    CaTblLinkVariability var;
    CaTblRobustRef ref2p4;
    PropagationParams prop2p4;
    PropagationParams propUwb;
    EnvironmentClass envUwb;
    double accumGainUwbDb;
    uint64_t legs = 0;
    uint64_t withVar = 0;
    uint64_t scored = 0;
    uint64_t inflated = 0;
    double varSum = 0.0;
    double rawSum = 0.0;
    double legSum = 0.0;
};

/**
 * N.14 -- --fuseFloorActive. One row per node-cycle whose fix set was the union of the
 * active tier's ranges and the floor's: fusefloor_<arm>_seed<N>.csv (digested), written
 * only under the flag, and one N.14 line. The fix kind is what Step() made of the union.
 */
struct FuseFloorLog
{
    explicit FuseFloorLog(const std::string& path)
    {
        log.open(path.c_str());
        if (!log.is_open())
        {
            std::cerr << "FATAL: cannot open " << path << " for writing -- does --outdir exist?\n";
            std::exit(1);
        }
        log << "t,nodeId,activeTier,nActive,nFloor,nFused,hadFix,fixKind,fixTier\n";
    }
    void Row(double t, uint32_t nodeId, CaTblTier active, size_t nA, size_t nF,
             const CaTblDecision& d, CaTblTier fixTier)
    {
        log << std::fixed << std::setprecision(4) << t << ',' << nodeId << ','
            << CaTblTierName(active) << ',' << nA << ',' << nF << ',' << (nA + nF) << ','
            << (d.hadFix ? 1 : 0) << ',' << static_cast<int>(d.fixKind) << ','
            << CaTblTierName(fixTier) << '\n';
        ++sets;
        ++byActive[std::min<size_t>(nA, 2)];
        ++byTier[static_cast<int>(active)];
        ++byKind[std::min<int>(4, d.fixKind)];
        if (d.hadFix && nA > 0)
        {
            ++fixesWithActive[static_cast<int>(active)];
        }
    }
    void Report(std::ostream& os, const char* arm)
    {
        log.close();
        os << "    N.14 floor+active fusion (" << arm << "): sets " << sets << " (active ranges 0/1/2 "
           << byActive[0] << "/" << byActive[1] << "/" << byActive[2] << "; active tier 868/2.4/UWB "
           << byTier[0] << "/" << byTier[1] << "/" << byTier[2] << "); Step made none/full/A/B/D "
           << byKind[0] << "/" << byKind[1] << "/" << byKind[2] << "/" << byKind[3] << "/" << byKind[4]
           << "; fixes using active ranges 868/2.4/UWB " << fixesWithActive[0] << "/"
           << fixesWithActive[1] << "/" << fixesWithActive[2] << "\n";
    }
    std::ofstream log;
    uint64_t sets = 0;
    uint64_t byActive[3] = {0, 0, 0};
    uint64_t byTier[3] = {0, 0, 0};
    uint64_t byKind[5] = {0, 0, 0, 0, 0};
    uint64_t fixesWithActive[3] = {0, 0, 0};
};

/**
 * L.1.2 -- RELAY DEPTH 2 (Network sect 5), behind --relayChain.
 *
 * WHAT WAS MISSING. CaTblComposeRelayChain, --maxRelayDepth and --relayVarBudget existed,
 * and the scenario hard-set hopDepth = 1: every relay ranged surveyed anchors and nothing
 * else, so no relay's own position was ever relayed, depth 2 could not arise, and the
 * ceiling and the budget never bound on the thing they exist for.
 *
 * WHO THE RELAYS ARE. The peer field nodes (ids 2+) that --relayedRanging gives a filter.
 * TECH_DEBT L.1.2 says "finder"; a finder carries GNSS and runs no filter, so it is an
 * anchor here, never a relay. The relay role is the filtered peer's.
 *
 * WHAT THIS ADDS. Each cycle a relay that ranged three or more anchors HOLDS A FIX and is
 * re-grounded at depth 0. A relay short of three is not stepped at once: after every
 * relay has ranged its anchors, it ranges each relay holding a fix THIS cycle, on
 * --relayTier, and each such range is a RELAYED measurement -- A1-A5 applied, hopDepth =
 * the holder's depth + 1, P = the holder's covariance composed with what the holder
 * itself inherited. Admitted ranges join its anchor ranges and it steps. If that step
 * FUSED them -- three or more measurements, so it holds a fix too -- its depth becomes
 * the deepest holder it used + 1, and a short relay processed after it may chain through
 * it. The measured tag, ranging a relay, reads that relay's depth -- so a range through a
 * relay that leaned on another is hopDepth 2, and the ceiling and the budget decide it.
 *
 * ONLY A FUSED RANGE MOVES DEPTH. Below three measurements CaTblPolicy::Step() coasts on
 * the prediction and uses none of them, so a short relay that admitted a relayed range
 * and still had two measurements has NOT leaned on anyone: its estimate is exactly what
 * it would have been without the exchange. Marking it deeper would manufacture depth-2
 * candidates out of relays whose position no relay ever touched -- counts that look
 * like the mechanism working and are not. (The first draft of this did exactly that: in
 * every regime probed, not one short relay reached three measurements, and depth 2 was
 * still reported hundreds of times.)
 *
 * AN EXCHANGE THAT CANNOT COMPLETE A FIX IS NOT MADE. If its anchor ranges plus every
 * holder it could range still come to fewer than three, even a clean sweep leaves the
 * filter coasting, so the exchanges would buy nothing and are neither made nor charged.
 * OURS, not cited -- a node that knows the fix floor does not spend airtime below it.
 *
 * HOW A SHORT RELAY KNOWS WHO HOLDS A FIX is not modelled: it ranges every holder, as
 * the measured tag's relayed exchange ranges every relay with no discovery step. The
 * holder's estimate and covariance are read, not transmitted -- P_B is not yet
 * serialised (TECH_DEBT sect 3.1 item 1), on either leg.
 *
 * DEPTH IS CARRIED ACROSS CYCLES, and that is a choice. A relay that coasts keeps the
 * depth of what it last rested on, because its estimate still rests on it; only three
 * direct anchor ranges in one cycle re-ground it.
 *
 * WHY THE HOLDER'S COVARIANCE IS COMPOSED, although the short relay's filter already fused
 * the range at sigma_eff. sigma_eff sizes ONE measurement. A5 stops a relay contributing
 * twice within an update; nothing stops it contributing once per cycle, cycle after
 * cycle, and the holder's position error is shared by all of those. The short relay's
 * filter treats it as fresh noise each time and shrinks P_B on it -- the correlation of
 * RELAYED_RANGING_ERROR_MODEL.md sect 3, across time rather than within an update. The
 * composition adds the projected term back along the upstream leg. Where the filter has
 * not yet averaged it down this double-counts part of it: conservative, in the direction
 * the error model prefers (it discards information rather than inventing it).
 *
 * WHICH INHERITANCE IS CARRIED -- OURS, not cited. A relay that admitted ranges through
 * several holders in one update carries the LARGEST projected term and that leg's
 * direction. A sum would count as independent the errors sect 3 says are not.
 *
 * ENERGY, and the sect 6 invariant. The exchange is one more ranging exchange on
 * --relayTier -- the one radio a relay ranges on; it carries all three like every node,
 * but it is a fixed-tier measured node (C5) and never escalates -- inside the per-cycle
 * loop, charged to the SHORT relay's own account whether or not the range is admitted.
 * That is the rule the measured tag's relayed exchange already follows: the initiator
 * pays, and the answering relay is not charged, as an answering gateway is not. No other
 * tier and no other account is touched; the measured tag's energy is not.
 *
 * UNDER --relaySlots=false THE EXCHANGE IS STAMPED AT THE CYCLE'S ONE INSTANT, like every C5
 * exchange. Every ProcessTransmission in a cycle then carries the same `now`, and the
 * engines' interference registry keys on time overlap and transmitter id alone, so every
 * relay's exchanges collide with every exchange registered before them in that instant.
 * Measured with this flag on at b390ed6, twelve regimes, 5 760 cycles: two relays held a
 * fix in the same cycle ONCE, 17 relay-to-relay exchanges were possible and made and none
 * delivered a range, so no chain ever fused and depth 2 never arose. That scheduling
 * belongs to C5, for every relay exchange and not only this one; --relaySlots (RelayRound,
 * C5_INSTANT) gives each exchange here the next slot of the relays' round, and has been on
 * by default in relayed runs since 2026-09-30.
 *
 * Off, this is never constructed, the relays step exactly as before, and not one engine
 * evaluation is added. Every relayed-range decision it sees -- both legs -- is written to
 * relay_chain_<arm>_seed<N>.csv, which exists only when the flag is on, and so is every
 * FUSE: a row with leg `fuse` for each short relay whose step fused a relayed range,
 * carrying the depth it took. A depth-d candidate with no earlier fuse at depth d-1 for
 * its relay is depth that came from nowhere, and G17 refuses it.
 */
struct RelayChain
{
    RelayChain(size_t nTags, const std::string& logPath, bool legCols = false);
    void BeginCycle();
    //! A relay's anchor ranging is done. True: defer its step (short of three).
    bool Defer(size_t pt, const std::vector<CaTblRangeMeasurement>& pmeas, size_t addressed);
    //! Short relays range this cycle's fix-holders, then step. \p round null (no
    //! --relaySlots): every exchange at \p now, charged at \p t.
    void RangeShortRelays(std::vector<CaTblTagState>& tags,
                          uint32_t nTags,
                          CaTblTier rt,
                          const TierPhy* phy,
                          LoRaPhysicsEngine& e868,
                          LoRa2p4PhysicsEngine& e2p4,
                          LoRaUWBPhysicsEngine& eUwb,
                          double t,
                          double dt,
                          Time now,
                          uint32_t maxRelayDepth,
                          double budgetM2,
                          RelayRound* round);
    //! The measured tag's leg: stamp depth and composed P onto a candidate through relay
    //! pt. False: the chain could not be composed, and the candidate must be dropped.
    bool ForTag(size_t pt, const CaTblPolicy& relay, CaTblRelayedRange& rr, double t,
                uint32_t consumer);
    //! One decision, either leg. verdict < 0 means the composition was refused.
    void Record(double t, uint32_t consumer, int leg, const CaTblRelayedRange& rr,
                double inheritedVarM2, int verdict, double sigmaEffM,
                const LegTerms::Out* terms = nullptr);
    //! A short relay's step FUSED a relayed range: the depth it now carries, and why.
    void LogFuse(double t, uint32_t relayId, uint32_t viaId, CaTblTier rt, uint32_t newDepth,
                 uint32_t maxRelayDepth, double trP, double inherited, double budgetM2);
    void Report(std::ostream& os, const std::vector<CaTblTagState>& tags,
                double budgetM2, uint32_t maxRelayDepth) const;

    //! --subjectRelayChain (N.15, 2026-10-03). A measured subject's own fix set: three or
    //! more direct ranges re-ground it at depth 0 and make it a fix-holder this cycle.
    void SubjectHolds(size_t pt);
    //! What a subject's exchanges with the holders left: the depth it would carry, the
    //! holder that set it, and the leg it inherits along.
    struct SubjectPick
    {
        size_t used = 0;
        size_t tried = 0;
        uint32_t newDepth = 0;
        uint32_t viaId = 0;
        double bestProj = -1.0;
        double bestUx = 0.0;
        double bestUy = 0.0;
    };
    //! A subject short of three on its active tier ranges, on that tier, every node that
    //! holds a fix this cycle and is awake on it (\p awake, a tier bitmask per tag);
    //! admitted ranges are appended to \p meas. Charged to the subject's own account.
    SubjectPick RangeForSubject(size_t b,
                                std::vector<CaTblTagState>& tags,
                                uint32_t nTags,
                                CaTblTier tier,
                                const std::vector<uint8_t>& awake,
                                const TierPhy* phy,
                                LoRaPhysicsEngine& e868,
                                LoRa2p4PhysicsEngine& e2p4,
                                LoRaUWBPhysicsEngine& eUwb,
                                double t,
                                double dt,
                                Time now,
                                uint32_t maxRelayDepth,
                                double budgetM2,
                                RelayRound* round,
                                std::vector<CaTblRangeMeasurement>& meas);
    //! The subject stepped on \p meas (three or more, relayed ones among them): it now
    //! holds a fix at the depth the pick carried.
    void SubjectFused(size_t b, const CaTblTagState& B, const SubjectPick& pk, CaTblTier tier,
                      double t, uint32_t maxRelayDepth, double budgetM2);
    void ReportSubjects(std::ostream& os) const;
    uint64_t subjShort = 0;        //!< subjects short of three on both sets
    uint64_t subjTier1 = 0;        //!< of which were on 868 MHz, where A1 admits nothing
    uint64_t subjNoExchange = 0;   //!< of which could not reach three, so ranged no one
    uint64_t subjExchanges = 0;    //!< exchanges made (and charged)
    uint64_t subjLinked = 0;       //!< of which the engine delivered a range
    uint64_t subjAdmitted = 0;     //!< subjects that admitted at least one
    uint64_t subjFused = 0;        //!< of which reached three and fused
    uint64_t subjFusedByDepth[4] = {0, 0, 0, 0};
    uint64_t subjHeld = 0;         //!< subject-cycles re-grounded on direct ranges
    std::vector<uint8_t> awake;    //!< per tag, the tiers awake this cycle (bit = tier)
    void ClearAwake(size_t n)
    {
        awake.assign(n, 0);
    }

    std::vector<uint32_t> depth;          //!< hops between the estimate and infrastructure
    bool hopProvenance = false;           //!< NW-1: a short relay's Step gets its fused depth
    LegTerms* legTerms = nullptr;         //!< NW-1 step 3: null unless --hopVarianceTerms
    bool legCols = false;                 //!< the log carries the leg terms' columns
    std::vector<double> inheritedVarM2;   //!< projected variance that estimate leans on
    std::vector<double> upUx;             //!< direction of the upstream leg it came along
    std::vector<double> upUy;
    std::vector<char> fixThisCycle;       //!< stepped this cycle on >= 3 measurements
    std::vector<char> deferred;
    std::vector<size_t> addressed;
    std::vector<std::vector<CaTblRangeMeasurement>> held;
    std::ofstream log;

    uint64_t exchanges = 0;        //!< relay-to-relay exchanges made (and charged)
    uint64_t linked = 0;           //!< of which the engine delivered a range
    uint64_t shortSteps = 0;       //!< short relays stepped after the fix-holders
    uint64_t shortNoExchange = 0;  //!< of which could not reach three, so ranged no one
    uint64_t shortAdmitted = 0;    //!< of which admitted at least one relayed range
    uint64_t shortFused = 0;       //!< of which then had >= 3, so the filter FUSED them
    uint64_t shortByAnchors[3] = {0, 0, 0}; //!< short relays by direct anchor ranges 0/1/2
    uint64_t holderCycles[3] = {0, 0, 0};   //!< cycles by fix-holders present 0/1/2+
    uint64_t composeRefused[2] = {0, 0};
    uint64_t verdicts[2][6] = {{0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}};
    uint64_t offered[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};  //!< [leg][min(depth,3)]
    uint64_t admitted[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
    uint64_t a2[2][3] = {{0, 0, 0}, {0, 0, 0}};             //!< [leg][CaTblRelayA2Reason]
    //! [leg][min(depth,3)][CaTblRelayA2Reason] -- the done-when is about depth 2, and a
    //! total over depths would let depth-1 budget refusals stand in for it.
    uint64_t a2ByDepth[2][4][3] = {};
};

__attribute__((noinline))
RelayChain::RelayChain(size_t n, const std::string& logPath, bool withLegCols)
    : depth(n, 0),
      legCols(withLegCols),
      inheritedVarM2(n, 0.0),
      upUx(n, 0.0),
      upUy(n, 0.0),
      fixThisCycle(n, 0),
      deferred(n, 0),
      addressed(n, 0),
      held(n)
{
    log.open(logPath.c_str());
    if (!log.is_open())
    {
        std::cerr << "FATAL: cannot open " << logPath
                  << " for writing -- does --outdir exist?\n";
        std::exit(1);
    }
    log << "t,consumerId,relayId,leg,tier,hopDepth,maxRelayDepth,distM,carriedVarM2,"
           "inheritedVarM2,budgetM2,a2,verdict,sigmaRawM,sigmaEffM";
    // NW-1 step 3: the leg's readings and terms; sigmaRawM stays the engine's sigma, and
    // sigmaLegM is what C5 composed sigmaEffM from.
    log << (legCols ? ",legRssiDbm,legSnrDb,legRangeM,legN,legSDb,legSlopeDbPerM,legVarSigmaM,legScore,"
                      "legNlos,sigmaLegM\n"
                    : "\n");
}

__attribute__((noinline)) void
RelayChain::BeginCycle()
{
    std::fill(fixThisCycle.begin(), fixThisCycle.end(), 0);
    std::fill(deferred.begin(), deferred.end(), 0);
}

__attribute__((noinline)) bool
RelayChain::Defer(size_t pt, const std::vector<CaTblRangeMeasurement>& pmeas, size_t nAddr)
{
    if (pmeas.size() >= 3)
    {
        // Three direct anchor ranges: a fix on infrastructure alone. Re-grounded.
        fixThisCycle[pt] = 1;
        depth[pt] = 0;
        inheritedVarM2[pt] = 0.0;
        upUx[pt] = 0.0;
        upUy[pt] = 0.0;
        return false;
    }
    deferred[pt] = 1;
    held[pt] = pmeas;
    addressed[pt] = nAddr;
    ++shortByAnchors[pmeas.size()];
    return true;
}

__attribute__((noinline)) void
RelayChain::Record(double t, uint32_t consumer, int leg, const CaTblRelayedRange& rr,
                   double inherited, int verdict, double sigmaEffM,
                   const LegTerms::Out* terms)
{
    const unsigned d = std::min<unsigned>(3u, rr.hopDepth);
    ++offered[leg][d];
    const char* a2name = "not_reached";
    if (verdict < 0)
    {
        ++composeRefused[leg];
    }
    else
    {
        ++verdicts[leg][verdict];
        if (verdict == CA_TBL_RELAY_ADMIT)
        {
            ++admitted[leg][d];
        }
        // A1 is checked first; a candidate refused there never reached A2.
        if (verdict != CA_TBL_RELAY_REJECT_TIER)
        {
            const CaTblRelayA2Reason why = CaTblRelayA2(rr);
            ++a2[leg][why];
            ++a2ByDepth[leg][d][why];
            a2name = CaTblRelayA2ReasonName(why);
        }
    }
    log << std::fixed << std::setprecision(4) << t << ',' << consumer << ',' << rr.relayId
        << ',' << (leg == 0 ? "relay" : "tag") << ',' << static_cast<int>(rr.tier) << ','
        << static_cast<unsigned>(rr.hopDepth) << ',' << rr.maxRelayDepth << ','
        << std::setprecision(6) << rr.distToRelayM << ','
        << (rr.relayP[0][0] + rr.relayP[1][1]) << ',' << inherited
        << ',' << rr.relayVarBudgetM2 << ',' << a2name << ','
        << (verdict < 0 ? "COMPOSE_REFUSED"
                        : CaTblRelayVerdictName(static_cast<CaTblRelayVerdict>(verdict)))
        << ',' << (terms ? terms->sigmaRawM : rr.sigmaRangeM) << ',' << sigmaEffM;
    if (legCols && terms)
    {
        log << ',' << terms->rssiDbm << ',' << terms->snrDb << ',' << terms->rangeM << ','
            << terms->n << ','
            << terms->sDb << ',' << terms->slopeDbPerM << ',' << terms->varSigmaM << ','
            << terms->score << ',' << (terms->nlos ? 1 : 0) << ',' << terms->sigmaLegM;
    }
    else if (legCols)
    {
        log << ",,,,,,,,,,";
    }
    log << '\n';
}

__attribute__((noinline)) void
RelayChain::LogFuse(double t, uint32_t relayId, uint32_t viaId, CaTblTier rt,
                    uint32_t newDepth, uint32_t maxRelayDepth, double trP, double inherited,
                    double budgetM2)
{
    // Same columns as a decision row, so one reader takes both. consumerId is the relay
    // that fused, relayId the deepest holder it fused through, hopDepth the depth it now
    // carries, carriedVarM2 its own trace(P) after the step. No A2 and no sigma: this
    // row records a consequence, not an admission.
    log << std::fixed << std::setprecision(4) << t << ',' << relayId << ',' << viaId
        << ",fuse," << static_cast<int>(rt) << ',' << newDepth << ',' << maxRelayDepth
        << ",0.000000," << std::setprecision(6) << trP << ',' << inherited << ','
        << budgetM2 << ",-,FUSED,0.000000,0.000000" << (legCols ? ",,,,,,,,,,\n" : "\n");
}

__attribute__((noinline)) bool
RelayChain::ForTag(size_t pt, const CaTblPolicy& relay, CaTblRelayedRange& rr, double t,
                   uint32_t consumer)
{
    rr.hopDepth = static_cast<uint8_t>(std::min<uint32_t>(255u, depth[pt] + 1));
    if (inheritedVarM2[pt] > 0.0)
    {
        const double p[2][2] = {{relay.Pxx(), relay.Pxy()}, {relay.Pxy(), relay.Pyy()}};
        if (!CaTblComposeRelayChain(p, inheritedVarM2[pt], upUx[pt], upUy[pt], rr.relayP))
        {
            // Not passed on uncorrected: that is the exact understatement the
            // composition exists to prevent.
            Record(t, consumer, 1, rr, inheritedVarM2[pt], -1, 0.0);
            return false;
        }
    }
    return true;
}

__attribute__((noinline)) void
RelayChain::RangeShortRelays(std::vector<CaTblTagState>& tags,
                             uint32_t nTags,
                             CaTblTier rt,
                             const TierPhy* phy,
                             LoRaPhysicsEngine& e868,
                             LoRa2p4PhysicsEngine& e2p4,
                             LoRaUWBPhysicsEngine& eUwb,
                             double t,
                             double dt,
                             Time now,
                             uint32_t maxRelayDepth,
                             double budgetM2,
                             RelayRound* round)
{
    const int rti = static_cast<int>(rt);
    const size_t n = std::min<size_t>(nTags, tags.size());
    size_t holders = 0;
    for (size_t k = 1; k < n; ++k)
    {
        holders += fixThisCycle[k] ? 1 : 0;
    }
    ++holderCycles[std::min<size_t>(holders, 2)];
    for (size_t b = 1; b < n; ++b)
    {
        if (!deferred[b])
        {
            continue;
        }
        CaTblTagState& B = tags[b];
        std::vector<CaTblRangeMeasurement>& meas = held[b];
        std::set<uint16_t> used;
        uint32_t newDepth = 0;
        uint32_t viaId = 0; // the holder that set newDepth
        double bestProj = -1.0;
        double bestUx = 0.0;
        double bestUy = 0.0;
        size_t tried = 0;
        // Holders NOW, not at the top: a short relay stepped earlier in this loop may have
        // just become one.
        size_t reachable = 0;
        for (size_t c = 1; c < n; ++c)
        {
            reachable += (c != b && fixThisCycle[c]) ? 1 : 0;
        }
        // Even a clean sweep would leave the filter below three and coasting: no exchange.
        const bool canFix = (meas.size() + reachable >= 3);
        if (!canFix)
        {
            ++shortNoExchange;
        }
        for (size_t c = 1; c < n && canFix; ++c)
        {
            if (c == b || !fixThisCycle[c])
            {
                continue;
            }
            const CaTblTagState& C = tags[c];
            // B's own PRIOR estimate: it has not stepped this cycle and does not know where
            // it is. Its true position enters only the physics, which does not care.
            const double ddx = B.policy.EstX() - C.policy.EstX();
            const double ddy = B.policy.EstY() - C.policy.EstY();
            const double dist = std::sqrt(ddx * ddx + ddy * ddy);

            bool got = false;
            double r = 0.0;
            double sigRaw = 0.0;
            LegTerms::Obs obs; // NW-1 step 3
            obs.tier = rti;
            obs.txPowerDbm = phy[rti].txPowerDbm;
            // C5_INSTANT: the round's next relay-to-relay slot, after every relay's polls.
            double atS = t;
            const Time at = round ? round->NextChain(now, t, &atS) : now;
            if (rt == CA_TBL_TIER_2P4)
            {
                PhysicsResult2p4 res = e2p4.ProcessTransmission(
                    B.nodeId, C.nodeId, B.pos, C.pos, phy[rti].txPowerDbm, phy[rti].sf,
                    phy[rti].bandwidthHz, 1, at);
                got = res.packetReceived;
                r = res.estimatedRangeM;
                sigRaw = res.rangingSigmaM;
                obs.rssiDbm = res.measuredRssiDbm;
                obs.snrDb = res.snr_db;
            }
            else if (rt == CA_TBL_TIER_UWB)
            {
                PhysicsResultUWB res = eUwb.ProcessTransmission(
                    B.nodeId, C.nodeId, B.pos, C.pos, phy[rti].txPowerDbm, phy[rti].sf,
                    phy[rti].bandwidthHz, 1, at);
                got = res.packetReceived;
                r = res.estimatedRangeM;
                sigRaw = res.rangingSigmaM;
                obs.rssiDbm = res.measuredRssiDbm;
                obs.snrDb = res.snr_db;
                obs.truthNlos = res.isNLOS;
                obs.kFactor = res.sampledKFactor;
                obs.nlosDb = res.nlosDb;
            }
            else
            {
                PhysicsResult res = e868.ProcessTransmission(
                    B.nodeId, C.nodeId, B.pos, C.pos, phy[rti].txPowerDbm, phy[rti].sf,
                    phy[rti].bandwidthHz, 1, at);
                got = res.packetReceived;
                r = res.estimatedRangeM;
                sigRaw = res.rangingSigmaM;
            }
            ++tried;
            ++exchanges;
            // Charged on the short relay's own account, on --relayTier, admitted or not,
            // its governor armed from the instant the exchange took.
            B.energy.AccountRangingExchange(rt, 1, got ? 1 : 0, atS);
            if (!got || !std::isfinite(r) || r <= 0.0 || dist <= 0.0)
            {
                continue;
            }
            ++linked;

            CaTblRelayedRange rr;
            rr.relayId = static_cast<uint16_t>(C.nodeId);
            rr.tier = rt;
            rr.hopDepth = static_cast<uint8_t>(std::min<uint32_t>(255u, depth[c] + 1));
            rr.maxRelayDepth = maxRelayDepth;
            rr.relayVarBudgetM2 = budgetM2;
            rr.sigmaRangeM = sigRaw;
            rr.relayP[0][0] = C.policy.Pxx();
            rr.relayP[0][1] = C.policy.Pxy();
            rr.relayP[1][0] = C.policy.Pxy();
            rr.relayP[1][1] = C.policy.Pyy();
            // C stepped earlier in this same cycle, so its covariance is current.
            rr.relayAgeS = 0.0;
            rr.relayFixIntervalS = dt;
            rr.ux = ddx / dist;
            rr.uy = ddy / dist;
            rr.distToRelayM = dist;
            if (inheritedVarM2[c] > 0.0)
            {
                const double p[2][2] = {{C.policy.Pxx(), C.policy.Pxy()},
                                        {C.policy.Pxy(), C.policy.Pyy()}};
                if (!CaTblComposeRelayChain(p, inheritedVarM2[c], upUx[c], upUy[c],
                                            rr.relayP))
                {
                    Record(t, B.nodeId, 0, rr, inheritedVarM2[c], -1, 0.0);
                    continue;
                }
            }
            LegTerms::Out terms;
            if (legTerms)
            {
                terms = legTerms->Apply(B.nodeId, C.nodeId, obs, r, sigRaw);
                rr.sigmaRangeM = terms.sigmaLegM; // C5 composes sigma_eff from it
            }
            double sigEff = 0.0;
            const CaTblRelayVerdict v = CaTblAdmitRelayedRange(rr, used, &sigEff);
            Record(t, B.nodeId, 0, rr, inheritedVarM2[c], static_cast<int>(v),
                   (v == CA_TBL_RELAY_ADMIT) ? sigEff : 0.0, legTerms ? &terms : nullptr);
            if (v != CA_TBL_RELAY_ADMIT)
            {
                continue;
            }
            // The PSEUDO-ANCHOR: the holder's ESTIMATE, priced by the inflated sigma.
            CaTblRangeMeasurement m;
            m.anchorX = C.policy.EstX();
            m.anchorY = C.policy.EstY();
            m.rangeM = r;
            m.nlos = false;
            m.nlosPropagation = false;
            m.hiddenChannelActive = false;
            m.sigmaM = sigEff;
            m.hopDepth = rr.hopDepth; // NW-1 (read only under --hopProvenance)
            meas.push_back(m);
            used.insert(rr.relayId);
            if (depth[c] + 1 > newDepth)
            {
                newDepth = depth[c] + 1;
                viaId = C.nodeId;
            }
            const double proj = sigEff * sigEff - sigRaw * sigRaw;
            if (proj > bestProj)
            {
                bestProj = proj;
                bestUx = rr.ux;
                bestUy = rr.uy;
            }
        }
        uint32_t hopArg = 0; // NW-1: the deepest leg in the set it fuses
        if (hopProvenance && meas.size() >= 3)
        {
            for (size_t k = 0; k < meas.size(); ++k)
            {
                hopArg = std::max<uint32_t>(hopArg, meas[k].hopDepth);
            }
        }
        B.policy.Step(dt, meas, hopArg, static_cast<int32_t>(meas.size()),
                      static_cast<int32_t>(addressed[b] + tried), 0);
        ++B.steps;
        ++shortSteps;
        if (!used.empty())
        {
            ++shortAdmitted;
        }
        // Step() fuses only at three or more; below that it coasted and the admitted
        // ranges were discarded, so B's estimate leans on no one and its depth stays.
        // held[b] started below three, so reaching three here means a relayed range is
        // among them.
        if (meas.size() >= 3 && !used.empty())
        {
            fixThisCycle[b] = 1;
            ++shortFused;
            depth[b] = newDepth;
            inheritedVarM2[b] = std::max(0.0, bestProj);
            upUx[b] = bestUx;
            upUy[b] = bestUy;
            LogFuse(t, B.nodeId, viaId, rt, newDepth, maxRelayDepth,
                    B.policy.Pxx() + B.policy.Pyy(), inheritedVarM2[b], budgetM2);
        }
    }
}

__attribute__((noinline)) void
RelayChain::SubjectHolds(size_t pt)
{
    fixThisCycle[pt] = 1;
    depth[pt] = 0;
    inheritedVarM2[pt] = 0.0;
    upUx[pt] = 0.0;
    upUy[pt] = 0.0;
    ++subjHeld;
}

/**
 * --subjectRelayChain. The short-relay exchange of RangeShortRelays, for a measured subject:
 * the same physics call, the same composition of the holder's covariance with what it
 * inherited, the same A1-A5 admission and the same pseudo-anchor. What differs, and why:
 * - the tier is the subject's ACTIVE tier, not --relayTier: a subject escalates, and it
 *   transmits on one tier a cycle (sect 6), which pays for the exchange;
 * - a holder must be awake on that tier this cycle (its active tier, or 868 MHz while a
 *   hybrid arm keeps the floor): a sleeping radio does not answer (NW-0's rule);
 * - holders are the nodes stepped EARLIER this cycle with a fix: plain relays (they step
 *   first) and lower-id subjects. Tag 0 steps after every subject, so it is never one.
 * Kept separate from RangeShortRelays, which is unchanged, so no default path moves.
 */
__attribute__((noinline)) RelayChain::SubjectPick
RelayChain::RangeForSubject(size_t b,
                            std::vector<CaTblTagState>& tags,
                            uint32_t nTags,
                            CaTblTier tier,
                            const std::vector<uint8_t>& awake,
                            const TierPhy* phy,
                            LoRaPhysicsEngine& e868,
                            LoRa2p4PhysicsEngine& e2p4,
                            LoRaUWBPhysicsEngine& eUwb,
                            double t,
                            double dt,
                            Time now,
                            uint32_t maxRelayDepth,
                            double budgetM2,
                            RelayRound* round,
                            std::vector<CaTblRangeMeasurement>& meas)
{
    SubjectPick pk;
    const int ti = static_cast<int>(tier);
    const size_t n = std::min<size_t>(nTags, tags.size());
    const uint8_t bit = static_cast<uint8_t>(1u << ti);
    CaTblTagState& B = tags[b];
    ++subjShort;
    if (tier == CA_TBL_TIER_868)
    {
        // A1 refuses every relayed range on tier 1 (a correlated shadowing field), so an
        // exchange on 868 MHz could buy nothing and is not made, as one that cannot reach
        // three is not.
        ++subjTier1;
        return pk;
    }
    size_t reachable = 0;
    for (size_t c = 1; c < n; ++c)
    {
        reachable += (c != b && fixThisCycle[c] && (awake[c] & bit)) ? 1 : 0;
    }
    if (meas.size() + reachable < 3)
    {
        ++subjNoExchange;
        return pk;
    }
    std::set<uint16_t> used;
    for (size_t c = 1; c < n; ++c)
    {
        if (c == b || !fixThisCycle[c] || !(awake[c] & bit))
        {
            continue;
        }
        const CaTblTagState& C = tags[c];
        const double ddx = B.policy.EstX() - C.policy.EstX();
        const double ddy = B.policy.EstY() - C.policy.EstY();
        const double dist = std::sqrt(ddx * ddx + ddy * ddy);
        bool got = false;
        double r = 0.0;
        double sigRaw = 0.0;
        LegTerms::Obs obs; // NW-1 step 3
        obs.tier = ti;
        obs.txPowerDbm = phy[ti].txPowerDbm;
        double atS = t;
        const Time at = round ? round->NextChain(now, t, &atS) : now;
        if (tier == CA_TBL_TIER_2P4)
        {
            PhysicsResult2p4 res = e2p4.ProcessTransmission(
                B.nodeId, C.nodeId, B.pos, C.pos, phy[ti].txPowerDbm, phy[ti].sf,
                phy[ti].bandwidthHz, 1, at);
            got = res.packetReceived;
            r = res.estimatedRangeM;
            sigRaw = res.rangingSigmaM;
            obs.rssiDbm = res.measuredRssiDbm;
            obs.snrDb = res.snr_db;
        }
        else if (tier == CA_TBL_TIER_UWB)
        {
            PhysicsResultUWB res = eUwb.ProcessTransmission(
                B.nodeId, C.nodeId, B.pos, C.pos, phy[ti].txPowerDbm, phy[ti].sf,
                phy[ti].bandwidthHz, 1, at);
            got = res.packetReceived;
            r = res.estimatedRangeM;
            sigRaw = res.rangingSigmaM;
            obs.rssiDbm = res.measuredRssiDbm;
            obs.snrDb = res.snr_db;
            obs.truthNlos = res.isNLOS;
            obs.kFactor = res.sampledKFactor;
            obs.nlosDb = res.nlosDb;
        }
        else
        {
            PhysicsResult res = e868.ProcessTransmission(
                B.nodeId, C.nodeId, B.pos, C.pos, phy[ti].txPowerDbm, phy[ti].sf,
                phy[ti].bandwidthHz, 1, at);
            got = res.packetReceived;
            r = res.estimatedRangeM;
            sigRaw = res.rangingSigmaM;
        }
        ++pk.tried;
        ++subjExchanges;
        // On the subject's own account, on the tier it is transmitting on this cycle.
        B.energy.AccountRangingExchange(tier, 1, got ? 1 : 0, atS);
        if (!got || !std::isfinite(r) || r <= 0.0 || dist <= 0.0)
        {
            continue;
        }
        ++subjLinked;
        CaTblRelayedRange rr;
        rr.relayId = static_cast<uint16_t>(C.nodeId);
        rr.tier = tier;
        rr.hopDepth = static_cast<uint8_t>(std::min<uint32_t>(255u, depth[c] + 1));
        rr.maxRelayDepth = maxRelayDepth;
        rr.relayVarBudgetM2 = budgetM2;
        rr.sigmaRangeM = sigRaw;
        rr.relayP[0][0] = C.policy.Pxx();
        rr.relayP[0][1] = C.policy.Pxy();
        rr.relayP[1][0] = C.policy.Pxy();
        rr.relayP[1][1] = C.policy.Pyy();
        rr.relayAgeS = 0.0; // C stepped earlier this cycle
        rr.relayFixIntervalS = dt;
        rr.ux = ddx / dist;
        rr.uy = ddy / dist;
        rr.distToRelayM = dist;
        if (inheritedVarM2[c] > 0.0)
        {
            const double p[2][2] = {{C.policy.Pxx(), C.policy.Pxy()},
                                    {C.policy.Pxy(), C.policy.Pyy()}};
            if (!CaTblComposeRelayChain(p, inheritedVarM2[c], upUx[c], upUy[c], rr.relayP))
            {
                Record(t, B.nodeId, 0, rr, inheritedVarM2[c], -1, 0.0);
                continue;
            }
        }
        LegTerms::Out terms;
        if (legTerms)
        {
            terms = legTerms->Apply(B.nodeId, C.nodeId, obs, r, sigRaw);
            rr.sigmaRangeM = terms.sigmaLegM;
        }
        double sigEff = 0.0;
        const CaTblRelayVerdict v = CaTblAdmitRelayedRange(rr, used, &sigEff);
        Record(t, B.nodeId, 0, rr, inheritedVarM2[c], static_cast<int>(v),
               (v == CA_TBL_RELAY_ADMIT) ? sigEff : 0.0, legTerms ? &terms : nullptr);
        if (v != CA_TBL_RELAY_ADMIT)
        {
            continue;
        }
        CaTblRangeMeasurement m;
        m.anchorX = C.policy.EstX();
        m.anchorY = C.policy.EstY();
        m.rangeM = r;
        m.nlos = false;
        m.nlosPropagation = false;
        m.hiddenChannelActive = false;
        m.sigmaM = sigEff;
        m.hopDepth = rr.hopDepth;
        m.anchorId = static_cast<uint16_t>(C.nodeId);
        m.sourceKind = CaTblSourceKindOf(static_cast<uint16_t>(C.nodeId));
        meas.push_back(m);
        used.insert(rr.relayId);
        if (depth[c] + 1 > pk.newDepth)
        {
            pk.newDepth = depth[c] + 1;
            pk.viaId = C.nodeId;
        }
        const double proj = sigEff * sigEff - sigRaw * sigRaw;
        if (proj > pk.bestProj)
        {
            pk.bestProj = proj;
            pk.bestUx = rr.ux;
            pk.bestUy = rr.uy;
        }
    }
    pk.used = used.size();
    if (pk.used > 0)
    {
        ++subjAdmitted;
    }
    return pk;
}

__attribute__((noinline)) void
RelayChain::SubjectFused(size_t b, const CaTblTagState& B, const SubjectPick& pk, CaTblTier tier,
                         double t, uint32_t maxRelayDepth, double budgetM2)
{
    fixThisCycle[b] = 1;
    ++subjFused;
    ++subjFusedByDepth[std::min<uint32_t>(3u, pk.newDepth)];
    depth[b] = pk.newDepth;
    inheritedVarM2[b] = std::max(0.0, pk.bestProj);
    upUx[b] = pk.bestUx;
    upUy[b] = pk.bestUy;
    LogFuse(t, B.nodeId, pk.viaId, tier, pk.newDepth, maxRelayDepth,
            B.policy.Pxx() + B.policy.Pyy(), inheritedVarM2[b], budgetM2);
}

__attribute__((noinline)) void
RelayChain::ReportSubjects(std::ostream& os) const
{
    os << "    L.1.2 subject chain: subject-cycles held on direct ranges " << subjHeld
       << ", short " << subjShort << " (on 868 MHz, where A1 admits nothing, " << subjTier1
       << "; no exchange possible " << subjNoExchange << "), exchanges " << subjExchanges << " (ranged " << subjLinked << "), admitted "
       << subjAdmitted << ", fused " << subjFused << " (at depth 1/2/3+ " << subjFusedByDepth[1]
       << "/" << subjFusedByDepth[2] << "/" << subjFusedByDepth[3] << ")\n";
}

__attribute__((noinline)) void
RelayChain::Report(std::ostream& os, const std::vector<CaTblTagState>& tags, double budgetM2,
                   uint32_t maxRelayDepth) const
{
    // Formatted into its own stream: std::fixed set on std::cout would outlive this call
    // and reformat every arm printed after it.
    std::ostringstream o;
    o << "    L.1.2 relay chain: relay-to-relay exchanges " << exchanges << " (ranged "
      << linked << "), short relays stepped " << shortSteps << " (no exchange possible "
      << shortNoExchange << ", admitted a relayed range " << shortAdmitted
      << ", fused it and held a fix " << shortFused << "), ceiling " << maxRelayDepth
      << ", budget " << std::fixed << std::setprecision(1) << budgetM2 << " m^2\n"
      << "      short relays by direct anchor ranges 0/1/2: " << shortByAnchors[0] << "/"
      << shortByAnchors[1] << "/" << shortByAnchors[2]
      << "; cycles by fix-holders present 0/1/2+: " << holderCycles[0] << "/"
      << holderCycles[1] << "/" << holderCycles[2] << "\n";
    for (int leg = 0; leg < 2; ++leg)
    {
        o << "      leg " << (leg == 0 ? "relay->relay" : "tag->relay  ")
          << ": offered by depth 1/2/3+ " << offered[leg][1] << "/" << offered[leg][2] << "/"
          << offered[leg][3] << ", admitted " << admitted[leg][1] << "/" << admitted[leg][2]
          << "/" << admitted[leg][3] << "; ADMIT " << verdicts[leg][0] << " A1 "
          << verdicts[leg][1] << " A2 " << verdicts[leg][2] << " A3 " << verdicts[leg][3]
          << " A4 " << verdicts[leg][4] << " A5 " << verdicts[leg][5] << " compose-refused "
          << composeRefused[leg] << "\n";
        for (int d = 1; d < 4; ++d)
        {
            o << "        A2 at depth " << d << (d == 3 ? "+" : "") << ": pass "
              << a2ByDepth[leg][d][CA_TBL_RELAY_A2_PASS] << " ceiling "
              << a2ByDepth[leg][d][CA_TBL_RELAY_A2_CEILING] << " budget "
              << a2ByDepth[leg][d][CA_TBL_RELAY_A2_BUDGET] << "\n";
        }
    }
    // Per tier, per relay: the sect 6 check. A relay ranges only on --relayTier (a
    // fixed-tier measured node, C5), so every other tier must read exactly zero.
    for (size_t k = 1; k < tags.size(); ++k)
    {
        o << "      relay node " << tags[k].nodeId << ": depth " << depth[k] << ", inherited "
          << std::setprecision(3) << inheritedVarM2[k] << " m^2, energy 868 "
          << std::setprecision(6) << tags[k].energy.GetTierJ(CA_TBL_TIER_868) << " J, 2.4 "
          << tags[k].energy.GetTierJ(CA_TBL_TIER_2P4) << " J, UWB "
          << tags[k].energy.GetTierJ(CA_TBL_TIER_UWB) << " J\n";
    }
    os << o.str();
}

/**
 * L.1.1 -- ROUTED POSITION FIXES (Network sect 2-3), behind --fixRouting.
 *
 * WHAT WAS MISSING. CaTblRoutingTable, its ETX baseline, the confidence metric and
 * Divergences() existed with a passing suite and ZERO scenario call sites: nothing ever
 * built a table, and no frame was ever routed on one.
 *
 * WHO ROUTES. The relays -- the filtered peer field nodes, ids 2+, that --relayedRanging
 * gives a filter and an energy account. They are FIXED-TIER nodes on --relayTier (C5),
 * which is what lets a relay forward a frame without waking a second radio. The measured
 * tag (id 1) does not take part: its tier changes cycle to cycle, and a frame on a tier it
 * did not transmit on would charge a tier outside that cycle's transmission (sect 6).
 * Finders carry GNSS and are anchors, not routers; they are neither sinks nor next hops.
 * NW-2 (--fixRoutingSubjects) lifts that: one router per tier, the measured tag included,
 * and each cycle a node takes part only in the router of the tier it ranged on (part[]),
 * which is what keeps the sect 6 reason above intact for nodes whose tier moves.
 *
 * WHERE TO. Every gateway is on the backhaul the edge-server path already assumes, so a
 * fix delivered to ANY gateway has reached the network. The destination is therefore the
 * gateway BACKBONE, one table id (kBackbone) no node carries, and a route's next hop is
 * either a gateway (direct: the frame arrives) or a relay (the frame is carried on). "A
 * next hop that is not the destination" is a next hop that is not a gateway.
 *
 * WHAT A ROUTE IS BUILT FROM -- links the relay has actually heard, never the map.
 *  - Relay -> gateway: the gateway's DISCOVERY beacons as this relay's MAC delivered them
 *    on --relayTier (a callback on the MAC, bound only under this flag), and only while
 *    the gateway is in the relay's one-hop neighbour table.
 *  - Relay -> relay: the MAC has no tag-to-tag link class (EnginePhyProvider, limitation
 *    2), so a relay holding a route ADVERTISES it: one broadcast per cycle on --relayTier,
 *    evaluated at every other relay by the tier's engine. Only reception and received
 *    power are read, never estimatedRangeM, so this is a link and not a relayed range.
 *    The advertisement is discovery gossip: one hop, never routed.
 * Each directed link keeps its last kLinkWindow frames. From them: the delivery ratio d
 * (frames heard against the sender's mean period), so ETX per hop = 1/d^2 -- the reverse
 * direction is not observed and reciprocity is ASSUMED, ours; the NLOS score, the fraction
 * of those frames flagged NLOS under --nlosSource (the flag the filter uses); RSSI
 * variance, dB^2; and relay jitter, the standard deviation of the frames' inter-arrival
 * times, seconds -- a node cannot see another node's forwarding queue, so it scores the
 * irregularity of the frames it hears from it, which also rises with every missed frame.
 * A link is usable after two frames. A route via relay C carries the link to C plus what
 * C advertised: the hop count, ETX and the three sums of the route C ITSELF WOULD TAKE,
 * so the cost of handing a frame to C is the cost of where C will send it. The row's pcs
 * field is left 0: neither metric reads it, and the confidence of a fix belongs to the
 * node that made it, not to the path that carried it.
 *
 * WHO CHOOSES. Each relay originates one ENTRY_PUSH carrying its own estimate per cycle
 * -- position-fix traffic by CaTblCarriesPositionFix(), so the table routes it as
 * CA_TBL_TRAFFIC_POSITION_FIX -- and every hop asks its own table ONCE:
 * SelectRoute(kBackbone, CONFIDENCE, POSITION_FIX, conf, &etx). Under --fixRouting the
 * frame goes to ETX's pick and the confidence pick is only counted; under
 * --fixRoutingConfidence it goes to the confidence pick. Divergences() therefore counts
 * the hops at which the two metrics disagreed about a frame that was actually routed.
 * path_cost = alpha*NLOS + beta*RSSI_var + gamma*jitter over the hops, among routes no
 * more than slack = 1 hop longer than the shortest (CaTblRoutingWeights defaults, OURS,
 * not cited; in these units the dB^2 variance term dominates at equal weights).
 *
 * WHEN. After the cycle's last ranging exchange, each frame in its OWN slot, kSlotS
 * apart, in the remainder of the cycle. Stamping them at the cycle instant would put
 * them inside the engines' interference registry with every ranging exchange of the
 * cycle -- the collision that keeps L.1.2's relay-to-relay exchanges from delivering. The
 * slot order is a collision-free schedule, OURS; the spec defines none.
 *
 * WHAT IS CHARGED, and the sect 6 invariant. Every frame goes on --relayTier, the one
 * radio a relay uses, inside the per-cycle loop: its sender pays the airtime at the TX
 * current and arms the duty governor from the slot it took (AccountFrameTx, after
 * CanTransmit()); a relay that receives a fix to carry on pays the airtime at the RX
 * current. Nothing else is touched -- not another tier, not the measured tag, not a
 * gateway. Advertisements are not charged at their receivers, as a relay is not charged
 * for the beacons it hears. ENTRY_PUSH carries no ACK (ca-tbl-header.cc), so a lost hop
 * is a lost report: no retransmission.
 *
 * Off, this is never constructed, no callback is bound, no frame is sent and no engine is
 * evaluated. On, every hop decision is written to fix_routing_<arm>_seed<N>.csv (G18).
 */
struct FixRouter
{
    static const uint16_t kBackbone = 0xFFFF; //!< the gateway backbone; no node carries it
    static const size_t kLinkWindow = 8;      //!< frames kept per directed link. OURS.
    static const uint8_t kMaxHops = 3;        //!< a frame is dropped past this. OURS.
    static const uint32_t kRouteBytes = 4;    //!< hop count, ETX and cost on an advert. OURS.
    static constexpr double kSlotS = 0.05;    //!< one frame per slot, > every tier's nominal
                                              //!< collision airtime (868 SF7: 20.5 ms). OURS.
    static constexpr double kRouteTtlCycles = 3.0; //!< a route not refreshed for 3 cycles
                                                   //!< is expired. OURS.

    struct Link
    {
        std::deque<double> at;
        std::deque<double> rssi;
        std::deque<char> nlos;
    };
    struct Advert
    {
        bool have = false;
        uint16_t nextHop = 0;
        uint8_t hops = 0;
        double etx = 0.0;
        double sumNlos = 0.0;
        double sumVar = 0.0;
        double sumJit = 0.0;
    };
    struct Heard
    {
        bool received = false;
        double rxPowerDbm = 0.0;
        bool nlos = false;
    };

    FixRouter(const std::vector<CaTblTagState>& tags,
              uint32_t nTags,
              CaTblTier tier,
              bool confActive,
              const std::vector<AnchorDef>& gateways,
              double beaconPeriodS,
              double cycleS,
              CaTblNlosSource src,
              const TierPhy* ph,
              const std::string& logPath,
              const CaTblRoutingWeights& w = CaTblRoutingWeights(),
              bool withTag0 = false,
              bool withTermCols = false);
    //! MAC delivery callback on each relay's --relayTier radio.
    void OnRx(uint16_t rxId, const CaTblHeader& hdr, const CaTblLinkOutcome& out);
    void Cycle(std::vector<CaTblTagState>& tags,
               const std::vector<std::vector<CaTblMac*>>& peerMacs,
               LoRaPhysicsEngine& e868,
               LoRa2p4PhysicsEngine& e2p4,
               LoRaUWBPhysicsEngine& eUwb,
               double t,
               Time now);
    void Report(std::ostream& os, const std::vector<CaTblTagState>& tags) const;

    Heard Eval(uint16_t txId, uint16_t rxId, const Vector& txPos, const Vector& rxPos,
               LoRaPhysicsEngine& e868, LoRa2p4PhysicsEngine& e2p4,
               LoRaUWBPhysicsEngine& eUwb, Time at, uint8_t hops);
    void Note(size_t rx, uint16_t tx, double atS, double rxPowerDbm, bool nlos);
    //! Fills hop count 1, ETX and the three terms for the link rx <- tx. False: unusable.
    bool LinkRoute(size_t rx, uint16_t tx, double nominalS, CaTblRoute& r) const;
    //! Next free slot instant in this cycle, or < 0 when the cycle is full.
    double NextSlot(double t);
    void Row(double t, uint16_t origin, uint32_t hop, uint16_t sender, bool have,
             const CaTblRoute& etx, const CaTblRoute& conf, const CaTblRoutingTable& tbl,
             uint16_t chosen, bool chosenIsGw, const char* outcome);
    //! NW-2: tag 0's position is the scenario's, not tags[0].pos.
    const Vector& PosOf(const std::vector<CaTblTagState>& tags, size_t k) const
    {
        return (k == 0) ? tag0Pos : tags[k].pos;
    }

    CaTblTier rt;
    bool confidenceActive;
    CaTblNlosSource nlosSrc;
    const TierPhy* phy;
    double beaconMeanS; //!< a gateway's mean beacon interval on rt
    double dt;
    double fixAirS;     //!< one ENTRY_PUSH of one entry on rt
    double advertAirS;  //!< one DISCOVERY plus the route summary on rt
    size_t n;           //!< tags.size(); index 0, the measured tag, routes only under NW-2
    // NW-2 (--fixRoutingSubjects). One router per tier; a node takes part in a cycle only
    // on the router of the tier it is on that cycle (sect 6). Without the flag first = 1
    // and every node takes part, as before.
    size_t first = 1;              //!< 0 when the measured tag routes
    std::vector<char> part;        //!< this cycle's participants, by tags index
    CaTblMac* tag0Mac = nullptr;   //!< the measured tag's radio on rt
    Vector tag0Pos;                //!< the measured tag's position this cycle
    bool termCols = false;         //!< rows carry the confidence route's three sums
    CaTblRoutingWeights weights;
    uint64_t offTier = 0;          //!< hops whose next hop was on another tier this cycle
    uint64_t partSum = 0;          //!< participants summed over cycles
    uint64_t cycles = 0;
    std::map<uint16_t, Vector> gwPos;
    std::map<uint16_t, size_t> relayIdx; //!< node id -> tags index
    std::vector<CaTblRoutingTable> tables;
    std::map<std::pair<size_t, uint16_t>, Link> links;
    uint32_t round = 0;
    uint32_t slot = 0;
    double cycleEndS = 0.0;
    std::ofstream log;

    uint64_t beaconsNoted = 0;
    uint64_t directInstalled = 0;
    uint64_t viaInstalled = 0;
    uint64_t directUnusable = 0;
    uint64_t adverts = 0;
    uint64_t advertsHeard = 0;
    uint64_t splitHorizon = 0;
    uint64_t originated = 0;
    uint64_t delivered[4] = {0, 0, 0, 0}; //!< [hops, 3 = 3+]
    uint64_t forwarded = 0;
    uint64_t lostOnAir = 0;
    uint64_t noRouteOrigin = 0;
    uint64_t noRouteMid = 0;
    uint64_t loops = 0;
    uint64_t ttl = 0;
    uint64_t duty = 0;
    uint64_t noSlot = 0;
    uint64_t selections = 0;
    uint64_t diverged = 0;
    uint64_t chosenGw = 0;
    uint64_t chosenRelay = 0;
    uint64_t etxRelay = 0;
    uint64_t confRelay = 0;
    uint64_t divergedCarried = 0;   //!< diverged hops the frame actually crossed
    std::vector<double> txAirS;     //!< per tag: router airtime charged at TX
    std::vector<double> rxAirS;     //!< per tag: router airtime charged at RX
};

__attribute__((noinline))
FixRouter::FixRouter(const std::vector<CaTblTagState>& tags,
                     uint32_t nTags,
                     CaTblTier tier,
                     bool confActive,
                     const std::vector<AnchorDef>& gateways,
                     double beaconPeriodS,
                     double cycleS,
                     CaTblNlosSource src,
                     const TierPhy* ph,
                     const std::string& logPath,
                     const CaTblRoutingWeights& w,
                     bool withTag0,
                     bool withTermCols)
    : rt(tier),
      confidenceActive(confActive),
      nlosSrc(src),
      phy(ph),
      // A gateway beacons every period plus U(0, period/4) -- the jitter every
      // BeaconDriver in this file is built with -- so its mean interval is 1.125 periods.
      beaconMeanS(beaconPeriodS * 1.125),
      dt(cycleS),
      n(std::min<size_t>(nTags, tags.size())),
      tables(std::min<size_t>(nTags, tags.size())),
      txAirS(tags.size(), 0.0),
      rxAirS(tags.size(), 0.0)
{
    const int rti = static_cast<int>(rt);
    const uint32_t fixBytes = CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(CA_TBL_MSG_ENTRY_PUSH, 1);
    const uint32_t advBytes =
        CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(CA_TBL_MSG_DISCOVERY, 1) + kRouteBytes;
    if (rt == CA_TBL_TIER_UWB)
    {
        fixAirS = CaTblEnergyModel::UwbFrameTimeS(fixBytes);
        advertAirS = CaTblEnergyModel::UwbFrameTimeS(advBytes);
    }
    else
    {
        fixAirS = CaTblEnergyModel::LoRaTimeOnAirS(phy[rti].sf, phy[rti].bandwidthHz, fixBytes, 1);
        advertAirS =
            CaTblEnergyModel::LoRaTimeOnAirS(phy[rti].sf, phy[rti].bandwidthHz, advBytes, 1);
    }
    for (size_t g = 0; g < gateways.size(); ++g)
    {
        gwPos[static_cast<uint16_t>(gateways[g].id)] = gateways[g].pos;
    }
    first = withTag0 ? 0 : 1;
    termCols = withTermCols;
    weights = w;
    part.assign(n, 1);
    for (size_t k = 0; k < tables.size(); ++k)
    {
        tables[k].SetWeights(w); // NW-2: --routeAlpha/Beta/Gamma/Slack, defaults 1,1,1,1
    }
    for (size_t k = first; k < n; ++k)
    {
        relayIdx[static_cast<uint16_t>(tags[k].nodeId)] = k;
    }
    log.open(logPath.c_str());
    if (!log.is_open())
    {
        std::cerr << "FATAL: cannot open " << logPath
                  << " for writing -- does --outdir exist?\n";
        std::exit(1);
    }
    log << "t,originId,hop,senderId,etxNext,etxHops,etxCost,confNext,confHops,confCost,"
           "metric,chosenNext,chosenClass,diverged,outcome"
        << (termCols ? ",confNlos,confVar,confJit\n" : "\n");
}

__attribute__((noinline)) void
FixRouter::Note(size_t rx, uint16_t tx, double atS, double rxPowerDbm, bool nlos)
{
    Link& l = links[std::make_pair(rx, tx)];
    l.at.push_back(atS);
    l.rssi.push_back(rxPowerDbm);
    l.nlos.push_back(nlos ? 1 : 0);
    while (l.at.size() > kLinkWindow)
    {
        l.at.pop_front();
        l.rssi.pop_front();
        l.nlos.pop_front();
    }
}

__attribute__((noinline)) void
FixRouter::OnRx(uint16_t rxId, const CaTblHeader& hdr, const CaTblLinkOutcome& out)
{
    // A gateway's own beacon, heard first-hand on the routing tier. A relayed copy names
    // the gateway as its origin but says nothing about THIS relay's link to it.
    if (hdr.GetHopsTraversed() != 0 || hdr.GetMsgType() != CA_TBL_MSG_DISCOVERY ||
        static_cast<CaTblTier>(hdr.GetTier()) != rt || gwPos.count(hdr.GetOriginId()) == 0)
    {
        return;
    }
    std::map<uint16_t, size_t>::const_iterator it = relayIdx.find(rxId);
    if (it == relayIdx.end())
    {
        return;
    }
    Note(it->second, hdr.GetOriginId(), Simulator::Now().GetSeconds(), out.rxPowerDbm,
         out.nlos);
    ++beaconsNoted;
}

__attribute__((noinline)) bool
FixRouter::LinkRoute(size_t rx, uint16_t tx, double nominalS, CaTblRoute& r) const
{
    std::map<std::pair<size_t, uint16_t>, Link>::const_iterator it =
        links.find(std::make_pair(rx, tx));
    if (it == links.end() || it->second.at.size() < 2)
    {
        return false;
    }
    const Link& l = it->second;
    const size_t k = l.at.size();
    const double span = l.at.back() - l.at.front();
    const double d =
        (span > 0.0) ? std::min(1.0, static_cast<double>(k - 1) * nominalS / span) : 1.0;
    double mean = 0.0;
    double nl = 0.0;
    for (size_t i = 0; i < k; ++i)
    {
        mean += l.rssi[i];
        nl += l.nlos[i];
    }
    mean /= static_cast<double>(k);
    double var = 0.0;
    for (size_t i = 0; i < k; ++i)
    {
        var += (l.rssi[i] - mean) * (l.rssi[i] - mean);
    }
    var /= static_cast<double>(k - 1);
    double gapMean = span / static_cast<double>(k - 1);
    double jit = 0.0;
    for (size_t i = 1; i < k; ++i)
    {
        const double g = (l.at[i] - l.at[i - 1]) - gapMean;
        jit += g * g;
    }
    jit = std::sqrt(jit / static_cast<double>(k - 1));
    r.hopCount = 1;
    r.etx = 1.0 / (d * d);
    r.sumNlos = nl / static_cast<double>(k);
    r.sumRssiVar = var;
    r.sumJitter = jit;
    return true;
}

__attribute__((noinline)) FixRouter::Heard
FixRouter::Eval(uint16_t txId, uint16_t rxId, const Vector& txPos, const Vector& rxPos,
                LoRaPhysicsEngine& e868, LoRa2p4PhysicsEngine& e2p4,
                LoRaUWBPhysicsEngine& eUwb, Time at, uint8_t hops)
{
    const int rti = static_cast<int>(rt);
    Heard h;
    if (rt == CA_TBL_TIER_2P4)
    {
        PhysicsResult2p4 r = e2p4.ProcessTransmission(txId, rxId, txPos, rxPos,
                                                      phy[rti].txPowerDbm, phy[rti].sf,
                                                      phy[rti].bandwidthHz, hops, at);
        h.received = r.packetReceived;
        h.rxPowerDbm = r.rxPowerDbm;
        h.nlos = CaTblNlosFlag(nlosSrc, r.nlosDb, r.isNLOS, r.hiddenChannelState);
    }
    else if (rt == CA_TBL_TIER_UWB)
    {
        PhysicsResultUWB r = eUwb.ProcessTransmission(txId, rxId, txPos, rxPos,
                                                      phy[rti].txPowerDbm, phy[rti].sf,
                                                      phy[rti].bandwidthHz, hops, at);
        h.received = r.packetReceived;
        h.rxPowerDbm = r.rxPowerDbm;
        h.nlos = CaTblNlosFlag(nlosSrc, r.nlosDb, r.isNLOS, r.hiddenChannelState);
    }
    else
    {
        PhysicsResult r = e868.ProcessTransmission(txId, rxId, txPos, rxPos,
                                                   phy[rti].txPowerDbm, phy[rti].sf,
                                                   phy[rti].bandwidthHz, hops, at);
        h.received = r.packetReceived;
        h.rxPowerDbm = r.rxPowerDbm;
        h.nlos = CaTblNlosFlag(nlosSrc, r.nlosDb, r.isNLOS, r.hiddenChannelState);
    }
    return h;
}

__attribute__((noinline)) double
FixRouter::NextSlot(double t)
{
    ++slot;
    const double s = t + static_cast<double>(slot) * kSlotS;
    return (s < cycleEndS) ? s : -1.0;
}

__attribute__((noinline)) void
FixRouter::Row(double t, uint16_t origin, uint32_t hop, uint16_t sender, bool have,
               const CaTblRoute& etx, const CaTblRoute& conf, const CaTblRoutingTable& tbl,
               uint16_t chosen, bool chosenIsGw, const char* outcome)
{
    log << std::fixed << std::setprecision(4) << t << ',' << origin << ',' << hop << ','
        << sender << ',';
    if (!have)
    {
        log << "0,0,0,0,0,0," << (confidenceActive ? "confidence" : "etx") << ",0,none,0,"
            << outcome << (termCols ? ",0,0,0\n" : "\n");
        return;
    }
    log << etx.nextHopId << ',' << static_cast<unsigned>(etx.hopCount) << ','
        << std::setprecision(6) << tbl.Cost(etx, CA_TBL_ROUTE_ETX) << ',' << conf.nextHopId
        << ',' << static_cast<unsigned>(conf.hopCount) << ','
        << tbl.Cost(conf, CA_TBL_ROUTE_CONFIDENCE) << ','
        << (confidenceActive ? "confidence" : "etx") << ',' << chosen << ','
        << (chosenIsGw ? "gateway" : "relay") << ','
        << (etx.nextHopId != conf.nextHopId ? 1 : 0) << ',' << outcome;
    if (termCols)
    {
        log << ',' << conf.sumNlos << ',' << conf.sumRssiVar << ',' << conf.sumJitter;
    }
    log << '\n';
}

__attribute__((noinline)) void
FixRouter::Cycle(std::vector<CaTblTagState>& tags,
                 const std::vector<std::vector<CaTblMac*>>& peerMacs,
                 LoRaPhysicsEngine& e868,
                 LoRa2p4PhysicsEngine& e2p4,
                 LoRaUWBPhysicsEngine& eUwb,
                 double t,
                 Time now)
{
    ++round;
    slot = 0;
    cycleEndS = t + dt;
    ++cycles;
    for (size_t k = first; k < n; ++k)
    {
        partSum += part[k] ? 1 : 0;
    }
    const uint16_t seq = static_cast<uint16_t>(round);
    const int rti = static_cast<int>(rt);
    const CaTblRouteMetric active =
        confidenceActive ? CA_TBL_ROUTE_CONFIDENCE : CA_TBL_ROUTE_ETX;
    // The header's definition, not a local one: widening what counts as position-fix
    // traffic widens the routing claim (ca-tbl-header.cc, CaTblCarriesPositionFix).
    const CaTblTrafficClass traffic = CaTblCarriesPositionFix(CA_TBL_MSG_ENTRY_PUSH)
                                          ? CA_TBL_TRAFFIC_POSITION_FIX
                                          : CA_TBL_TRAFFIC_GOSSIP;

    // 1. Routes not refreshed recently name next hops that may be gone.
    for (size_t b = first; b < n; ++b)
    {
        tables[b].Expire(t, kRouteTtlCycles * dt);
    }

    // 2. Direct routes: gateways in the relay's own one-hop table on rt, priced from the
    //    beacons its MAC delivered.
    for (size_t b = first; b < n; ++b)
    {
        if (!part[b])
        {
            continue; // NW-2: on another tier this cycle
        }
        CaTblMac* mm = (b == 0) ? tag0Mac : nullptr;
        if (b > 0)
        {
            const size_t pi = b - 1;
            if (pi >= peerMacs.size() || static_cast<size_t>(rti) >= peerMacs[pi].size() ||
                peerMacs[pi][static_cast<size_t>(rti)] == nullptr)
            {
                continue;
            }
            mm = peerMacs[pi][static_cast<size_t>(rti)];
        }
        if (mm == nullptr)
        {
            continue;
        }
        const std::vector<CaTblNeighbour> nb = mm->GetNeighbours(rt, t);
        for (size_t k = 0; k < nb.size(); ++k)
        {
            if (gwPos.count(nb[k].id) == 0)
            {
                continue;
            }
            CaTblRoute r;
            if (!LinkRoute(b, nb[k].id, beaconMeanS, r))
            {
                ++directUnusable;
                continue;
            }
            r.nodeId = kBackbone;
            r.nextHopId = nb[k].id;
            r.lastUpdatedS = t;
            r.seqNo = seq;
            r.valid = true;
            r.pathConfidencePenalty = tables[b].Cost(r, CA_TBL_ROUTE_CONFIDENCE);
            tables[b].Update(r);
            ++directInstalled;
        }
    }

    // 3. Advertisements. Snapshot first, so the order relays advertise in cannot let a
    //    route learned this round be re-advertised in the same round.
    std::vector<Advert> adv(n);
    for (size_t c = first; c < n; ++c)
    {
        CaTblRoute sel;
        if (tables[c].PeekRoute(kBackbone, active, traffic, sel))
        {
            adv[c].have = true;
            adv[c].nextHop = sel.nextHopId;
            adv[c].hops = sel.hopCount;
            adv[c].etx = sel.etx;
            adv[c].sumNlos = sel.sumNlos;
            adv[c].sumVar = sel.sumRssiVar;
            adv[c].sumJit = sel.sumJitter;
        }
    }
    for (size_t c = first; c < n; ++c)
    {
        if (!part[c] || !adv[c].have || adv[c].hops >= kMaxHops)
        {
            continue;
        }
        const double ts = NextSlot(t);
        if (ts < 0.0)
        {
            ++noSlot;
            continue;
        }
        CaTblEnergyModel& E = tags[c].energy;
        if (!E.CanTransmit(rt, ts))
        {
            ++duty;
            continue;
        }
        E.AccountFrameTx(rt, ts, advertAirS);
        txAirS[c] += advertAirS;
        ++adverts;
        const Time at = now + Seconds(ts - t);
        const uint16_t cId = static_cast<uint16_t>(tags[c].nodeId);
        for (size_t b = first; b < n; ++b)
        {
            if (b == c || !part[b])
            {
                continue;
            }
            const Heard h = Eval(cId, static_cast<uint16_t>(tags[b].nodeId), PosOf(tags, c),
                                 PosOf(tags, b), e868, e2p4, eUwb, at, 0);
            if (!h.received)
            {
                continue;
            }
            ++advertsHeard;
            Note(b, cId, ts, h.rxPowerDbm, h.nlos);
            // Split horizon: C's route runs through B, so B handing a frame to C would
            // hand it straight back.
            if (adv[c].nextHop == tags[b].nodeId)
            {
                ++splitHorizon;
                continue;
            }
            CaTblRoute r;
            // A relay advertises once a cycle while it holds a route: nominal period dt.
            if (!LinkRoute(b, cId, dt, r))
            {
                continue;
            }
            r.nodeId = kBackbone;
            r.nextHopId = cId;
            r.hopCount = static_cast<uint8_t>(adv[c].hops + 1);
            r.etx += adv[c].etx;
            r.sumNlos += adv[c].sumNlos;
            r.sumRssiVar += adv[c].sumVar;
            r.sumJitter += adv[c].sumJit;
            r.lastUpdatedS = t;
            r.seqNo = seq;
            r.valid = true;
            r.pathConfidencePenalty = tables[b].Cost(r, CA_TBL_ROUTE_CONFIDENCE);
            tables[b].Update(r);
            ++viaInstalled;
        }
    }

    // 4. Each relay reports its own estimate, hop by hop, each hop choosing once.
    for (size_t o = first; o < n; ++o)
    {
        if (!part[o])
        {
            continue;
        }
        ++originated;
        const uint16_t originId = static_cast<uint16_t>(tags[o].nodeId);
        size_t cur = o;
        std::set<uint16_t> visited;
        visited.insert(originId);
        for (uint32_t hop = 0;; ++hop)
        {
            const uint16_t sender = static_cast<uint16_t>(tags[cur].nodeId);
            CaTblRoute conf;
            CaTblRoute etx;
            if (!tables[cur].SelectRoute(kBackbone, CA_TBL_ROUTE_CONFIDENCE, traffic, conf,
                                         &etx))
            {
                (hop == 0 ? noRouteOrigin : noRouteMid)++;
                Row(t, originId, hop, sender, false, etx, conf, tables[cur], 0, false,
                    "noroute");
                break;
            }
            ++selections;
            const bool div = (conf.nextHopId != etx.nextHopId);
            diverged += div ? 1 : 0;
            etxRelay += gwPos.count(etx.nextHopId) ? 0 : 1;
            confRelay += gwPos.count(conf.nextHopId) ? 0 : 1;
            const CaTblRoute& ch = confidenceActive ? conf : etx;
            const bool isGw = gwPos.count(ch.nextHopId) != 0;
            (isGw ? chosenGw : chosenRelay)++;
            std::map<uint16_t, size_t>::const_iterator nx = relayIdx.find(ch.nextHopId);
            if (!isGw && (nx == relayIdx.end() || visited.count(ch.nextHopId) != 0))
            {
                ++loops;
                Row(t, originId, hop, sender, true, etx, conf, tables[cur], ch.nextHopId,
                    isGw, "loop");
                break;
            }
            if (!isGw && !part[nx->second])
            {
                // NW-2: the route was learned on this tier, and its next hop has since
                // moved to another. Not sent: it would not be listening.
                ++offTier;
                Row(t, originId, hop, sender, true, etx, conf, tables[cur], ch.nextHopId,
                    isGw, "offtier");
                break;
            }
            const double ts = NextSlot(t);
            if (ts < 0.0)
            {
                ++noSlot;
                Row(t, originId, hop, sender, true, etx, conf, tables[cur], ch.nextHopId,
                    isGw, "noslot");
                break;
            }
            CaTblEnergyModel& E = tags[cur].energy;
            if (!E.CanTransmit(rt, ts))
            {
                ++duty;
                Row(t, originId, hop, sender, true, etx, conf, tables[cur], ch.nextHopId,
                    isGw, "duty");
                break;
            }
            E.AccountFrameTx(rt, ts, fixAirS);
            txAirS[cur] += fixAirS;
            const Vector& rxPos = isGw ? gwPos[ch.nextHopId] : PosOf(tags, nx->second);
            const Heard h = Eval(sender, ch.nextHopId, PosOf(tags, cur), rxPos, e868, e2p4,
                                 eUwb, now + Seconds(ts - t),
                                 static_cast<uint8_t>(std::min<uint32_t>(255u, hop)));
            if (!h.received)
            {
                ++lostOnAir;
                Row(t, originId, hop, sender, true, etx, conf, tables[cur], ch.nextHopId,
                    isGw, "lost");
                break;
            }
            divergedCarried += div ? 1 : 0;
            if (isGw)
            {
                ++delivered[std::min<uint32_t>(3u, hop + 1)];
                Row(t, originId, hop, sender, true, etx, conf, tables[cur], ch.nextHopId,
                    isGw, "delivered");
                break;
            }
            tags[nx->second].energy.AccountListen(rt, fixAirS);
            rxAirS[nx->second] += fixAirS;
            ++forwarded;
            Row(t, originId, hop, sender, true, etx, conf, tables[cur], ch.nextHopId, isGw,
                "forwarded");
            if (hop + 1 >= kMaxHops)
            {
                ++ttl;
                break;
            }
            visited.insert(ch.nextHopId);
            cur = nx->second;
        }
    }
}

__attribute__((noinline)) void
FixRouter::Report(std::ostream& os, const std::vector<CaTblTagState>& tags) const
{
    std::ostringstream o;
    uint64_t tblSel = 0;
    uint64_t tblDiv = 0;
    for (size_t k = first; k < n; ++k)
    {
        tblSel += tables[k].Selections();
        tblDiv += tables[k].Divergences();
    }
    o << "    L.1.1 fix routing on " << CaTblTierName(rt) << ", selector "
      << (confidenceActive ? "CONFIDENCE" : "ETX (confidence scored)") << ": beacons noted "
      << beaconsNoted << ", routes installed direct " << directInstalled << " / via a relay "
      << viaInstalled << " (gateway links not yet usable " << directUnusable
      << "), adverts sent " << adverts << " heard " << advertsHeard << " split-horizon "
      << splitHorizon << "\n"
      << "      fixes originated " << originated << ": delivered by hops 1/2/3+ "
      << delivered[1] << "/" << delivered[2] << "/" << delivered[3] << ", forwarded "
      << forwarded << ", lost on air " << lostOnAir << ", no route at origin "
      << noRouteOrigin << " / mid-path " << noRouteMid << ", loop " << loops << ", ttl "
      << ttl << ", duty " << duty << ", no slot " << noSlot << "\n"
      << "      selections " << selections << " (tables " << tblSel << "), divergences "
      << diverged << " (tables " << tblDiv << "); next hop chosen gateway " << chosenGw
      << " / relay " << chosenRelay << "; ETX would pick a relay " << etxRelay
      << ", confidence " << confRelay << "; diverged hops crossed " << divergedCarried
      << "\n";
    if (termCols)
    {
        // NW-2: the weights the confidence cost used, and who took part.
        o << "      NW-2 weights alpha " << weights.alphaNlos << " beta " << weights.betaRssiVar
          << " gamma " << weights.gammaJitter << " slack "
          << static_cast<unsigned>(weights.slack) << "; measured tag routes "
          << (first == 0 ? "yes" : "no") << "; participants " << partSum << " over "
          << cycles << " cycles; next hop off this tier " << offTier << "\n";
    }
    // Per relay, per tier: the sect 6 check. Every router frame is on rt, so the two
    // other tiers must read what the relay's ranging alone put there -- exactly 0.
    for (size_t k = first; k < n; ++k)
    {
        o << "      router node " << tags[k].nodeId << ": routes " << tables[k].Size()
          << ", selections " << tables[k].Selections() << ", divergences "
          << tables[k].Divergences() << ", router airtime tx " << std::fixed
          << std::setprecision(6) << txAirS[k] << " s rx " << rxAirS[k]
          << " s; energy 868 " << tags[k].energy.GetTierJ(CA_TBL_TIER_868) << " J, 2.4 "
          << tags[k].energy.GetTierJ(CA_TBL_TIER_2P4) << " J, UWB "
          << tags[k].energy.GetTierJ(CA_TBL_TIER_UWB) << " J\n";
    }
    os << o.str();
}

/**
 * \brief Drives the SHS engine from a trajectory, by synthesising the gait an IMU sees.
 *
 * The engine samples at 100 Hz and expects trunk accelerations and yaw rate; the
 * scenario knows only where the walker is once per cycle. This fills the gap the same
 * way the engine's own example does -- a periodic bounce and anterior-posterior swing at
 * a walking cadence, plus the centripetal term from turning.
 *
 * SIMPLIFICATIONS, and they are real: trunk lean during turns is not modelled (the
 * example ramps it, this holds it at zero), and cadence is fixed rather than scaling
 * with speed. Both make the synthetic gait cleaner than a real one, so step detection
 * here is easier than in the field. Recorded in docs/DESIGN_DECISIONS.md.
 */
struct ShsDriver
{
    lorawan::ShsEngine* engine = nullptr;
    double prevHeading = 0.0;
    bool have = false;
    double cadenceHz = 1.8;
    double bounceAmp = 6.0;
    double apAmp = 3.0;
    bool seeded = false;

    void Advance(double tStart, double dt, const Vector& from, const Vector& to)
    {
        if (engine == nullptr || dt <= 0.0)
        {
            return;
        }
        const double dx = to.x - from.x, dy = to.y - from.y;
        const double disp = std::sqrt(dx * dx + dy * dy);
        const double speed = disp / dt;
        const double heading = (disp > 1e-6) ? std::atan2(dy, dx) : prevHeading;
        // Seed on the first cycle with real motion: before that the node has no
        // heading to state, and stating zero is a claim that it faces +x.
        if (!seeded && disp > 1e-6)
        {
            engine->SetInitialState(from.x, from.y, heading);
            seeded = true;
            prevHeading = heading;
        }
        double omegaZ = 0.0;
        if (have)
        {
            double d = heading - prevHeading;
            while (d > M_PI) d -= 2.0 * M_PI;
            while (d < -M_PI) d += 2.0 * M_PI;
            omegaZ = d / dt;
        }
        prevHeading = heading;
        have = true;

        const double sampleDt = 1.0 / 100.0; // ImuSensorParams::sampleRateHz
        // Cadence must carry the speed, or the engine integrates a distance
        // proportional to elapsed time instead of to how far the node actually went.
        // Nominal step length is the Weinberg model at this bounce amplitude and the
        // engine's fallback k_w; the clamp is the step detector's own passband, so a
        // speed the model cannot represent is under-counted rather than rescaled.
        const double nominalStepM = 0.7 * std::pow(bounceAmp, 0.25);
        const double wantCadence = (nominalStepM > 1e-6) ? (speed / nominalStepM) : 0.0;
        const double effCadence = std::min(3.0, std::max(0.5, wantCadence));
        const double gaitPeriod = 1.0 / effCadence;
        const bool resting = (speed < 0.05) || (wantCadence < 0.5);
        const int n = static_cast<int>(dt / sampleDt);
        for (int k = 0; k < n; ++k)
        {
            const double t = tStart + k * sampleDt;
            const double phase = std::fmod(t, gaitPeriod) / gaitPeriod;
            double bounce = (phase < 0.4) ? bounceAmp * std::sin(M_PI * phase / 0.4) : 0.0;
            double ap = apAmp * std::sin(2.0 * M_PI * phase);
            if (resting)
            {
                bounce = 0.0;
                ap = 0.0;
            }
            const double lateral = speed * omegaZ;
            const double vertical = 9.80665 + bounce;
            engine->Tick(t, sampleDt, ap, vertical, lateral, omegaZ, 0.0);
        }
    }
};

/**
 * \brief --tier1Tdoa: the gateways timestamp the node's tier-1 broadcast, the time
 *        differences are solved as hyperbolae, and the fix is fused beside the RSSI
 *        ranges of the same broadcast (Physical sect 1.1; TECH_DEBT J.2, L.2.1).
 *
 * WHAT MOVES AND WHAT DOES NOT. The energy model has always billed tier 1 as one
 * broadcast that the anchors timestamp -- and, since --tier1ChargeReplies became the
 * default, a reply from every anchor that answered -- while the code localised by RSSI
 * inversion alone. This builds the observable that bill describes. It adds NO frame and
 * NO charge. Every stamp is taken off the broadcast rangeTier() already made and billed.
 * It reaches the estimator either on the reply the energy model already charges for that
 * gateway (estimator on the node), or over the backhaul with the one downlink
 * AccountEdgeServerDownlink() charges (--gatewayEdgeServer). A configuration that bills
 * neither is refused in main(). So the sect 6 invariant holds by construction: nothing
 * here transmits, and nothing here is charged.
 *
 * WHO STAMPS. Only the surveyed tier-1 gateways: the one immobile class, with a
 * GNSS-surveyed position and a PPS-disciplined clock. A finder carries GNSS too, but its
 * position reaches the node as a digest entry that can be minutes old (see the
 * pseudo-anchor treatment in rangeTier()). Its stamps would need that age folded into a
 * timing variance, which is not modelled, so finders stay RSSI-only. Our choice, not
 * cited. Field nodes are GPS-denied and cannot stamp at all.
 *
 * THE STAMP, for each gateway that decoded the broadcast:
 *   t_k = t_tx + |p - a_k| / c + e_pps + e_toa + tau_nlos
 *   e_pps    ~ N(0, --tdoaPpsSigmaNs). The default, 20 ns, is the u-blox NEO/LEA-M8T
 *              timing-module data sheet's time-pulse accuracy (UBX-15025193 R10,
 *              Table 1: <= 20 ns clear sky, <= 500 ns indoor, both 1-sigma). The spec
 *              LIMIT is taken as the sigma, which is the pessimistic reading of a
 *              "<=". It is drawn independently per broadcast. A real PPS residual
 *              wanders slowly, and that per-gateway persistence is NOT modelled (a
 *              stated limit); nor is the gateway's own timestamp-counter quantisation.
 *   e_toa    ~ N(0, sigma_t). sigma_t is the one-way ToA Cramer-Rao bound for a LoRa
 *              chirp at this link's in-band SNR, SF and bandwidth
 *              (CaTblTdoaArrivalSigmaS): the bound and the one-symbol dechirp gain that
 *              tier 2's time-of-flight model already uses. A ToA bound is the right one
 *              HERE, because this IS a time-of-arrival estimator and RSSI inversion is
 *              not (TECH_DEBT 2.5). It is a bound: the low-SNR threshold effect is not
 *              modelled, and one symbol rather than the whole preamble makes it
 *              conservative.
 *   tau_nlos ~ Exp(mean = the link's rms delay spread) when the propagation state says
 *              the link is NLOS, else 0: a blocked direct path makes the first
 *              detectable arrival late by the order of the channel's spread. The
 *              exponential form is OURS, not cited. Its scale is the engine's own
 *              delay-spread output for that link.
 * t_tx is common to every stamp and cancels in the differences, so nothing reads the
 * node's clock. The solver is given the surveyed positions; survey error is not modelled
 * (metre-level after a survey-in, far below the timing noise). The solver is 2-D while
 * the stamps come from the 3-D geometry (15 m masts), a model error under a metre beyond
 * 100 m that stays inside the residual.
 *
 * THE SIGMA THE NETWORK ASSIGNS a stamp is what a gateway can know: the PPS
 * specification, the bound at the SNR it measured, and one more term when the configured
 * NLOS flag tags the link. That flag is --nlosSource, the same flag the RSSI path
 * inflates on, and the term is the second moment of the excess delay, 2 * spread^2. It
 * treats an unknown positive bias as variance, the way the RSSI path does. The gateway
 * never sees the realised excess delay. Our choice, not cited.
 *
 * RANDOMNESS. Two dedicated streams: seed*7919 + 1400017 (normal) and + 1400029
 * (exponential). Both are passed as the Stream attribute AT CONSTRUCTION, so not one
 * automatic stream index is consumed. The engines' samplers, and every draw on every
 * link, are exactly where they are without the flag. Off, this object does not exist.
 *
 * WHEN IT SOLVES: on a cycle whose fix set is tier 1 with >= 3 ranges (the active tier
 * is 868, or the tier-1 floor supplied the fix). Those are exactly the cycles on which
 * Step() fuses tier-1 ranges, and they are the denominator of every count below. With
 * four or more gateways Chan's closed form seeds Foy's iteration, and the RSSI
 * multilateration of those same ranges seeds it when Chan refuses or fails. With exactly
 * three, both exact roots are found in closed form and the ranges themselves choose
 * between them, or the solve is refused as AMBIGUOUS (ca-tbl-tdoa.h, step 2): the first
 * draft let the seed choose silently and took the mirror root 450-743 m from the truth
 * under a sigma near 85 m. An OK solve is queued on the policy
 * (SetAuxPositionFix). Step() fuses it straight after the range update, under the
 * cycle's integrity verdict: the COMBINED tier-1 fix the spec describes. Every such cycle
 * is one row of tdoa_<arm>_seed<N>.csv, refused solves included. Only the measured tag
 * does this; relays and peers stay RSSI-only.
 */
struct Tier1Tdoa
{
    Tier1Tdoa(uint32_t seed,
              double ppsSigmaNs,
              const TierPhy& t1,
              const std::vector<AnchorDef>& gateways,
              bool chargeReplies,
              bool edgeServer,
              const std::string& logPath)
        : ppsSigmaS(ppsSigmaNs * 1e-9),
          sf(t1.sf),
          bwHz(t1.bandwidthHz),
          billReplies(chargeReplies && !edgeServer),
          billDownlink(edgeServer)
    {
        gauss = CreateObjectWithAttributes<NormalRandomVariable>(
            "Stream", IntegerValue(static_cast<int64_t>(seed) * 7919 + 1400017),
            "Mean", DoubleValue(0.0),
            "Variance", DoubleValue(1.0));
        expo = CreateObjectWithAttributes<ExponentialRandomVariable>(
            "Stream", IntegerValue(static_cast<int64_t>(seed) * 7919 + 1400029),
            "Mean", DoubleValue(1.0));
        for (size_t k = 0; k < gateways.size(); ++k)
        {
            surveyed.insert(static_cast<uint16_t>(gateways[k].id));
        }
        log.open(logPath.c_str());
        log << "t,cycle,nRanges,nStamps,repliesBilled,downlinkBilled,verdict,seededBy,"
               "iterations,refAnchor,tdoaX,tdoaY,rxx,rxy,ryy,chi2,chanOk,trueX,trueY,"
               "tdoaErrM,rssiOk,rssiErrM,fused,rejected,estErrM,nRoots,tieChosen,tieOther\n";
        log << std::setprecision(10);
    }

    void BeginCycle()
    {
        stamps.clear();
        repliesThisCycle = 0;
        pending = false;
    }

    //! A gateway decoded this cycle's tier-1 broadcast: take its arrival stamp.
    void Stamp(uint16_t anchorId,
               const Vector& anchorPos,
               const Vector& nodePos,
               double t,
               double snrDbInBand,
               bool nlosTruth,
               double delaySpreadNs,
               bool nlosFlag)
    {
        if (surveyed.count(anchorId) == 0)
        {
            return; // a finder or a peer: no surveyed position to stamp against
        }
        const double sigToaS = CaTblTdoaArrivalSigmaS(bwHz, sf, snrDbInBand);
        if (!std::isfinite(sigToaS))
        {
            ++unstampable;
            return;
        }
        // The engine's own distance, floor included (lora-physics-engine.cc: 1 m).
        const double d = std::max(1.0, CalculateDistance(nodePos, anchorPos));
        const double spreadS =
            (std::isfinite(delaySpreadNs) && delaySpreadNs > 0.0) ? delaySpreadNs * 1e-9 : 0.0;
        const double ePps = ppsSigmaS * gauss->GetValue();
        const double eToa = sigToaS * gauss->GetValue();
        double excess = 0.0;
        if (nlosTruth && spreadS > 0.0)
        {
            excess = spreadS * expo->GetValue();
            ++nlosDelayed;
        }
        CaTblTdoaObs o;
        o.anchorId = anchorId;
        o.x = anchorPos.x;
        o.y = anchorPos.y;
        o.tArrS = t + d / kLightMps + ePps + eToa + excess;
        const double nlosVar = nlosFlag ? 2.0 * spreadS * spreadS : 0.0;
        o.sigmaS = std::sqrt(ppsSigmaS * ppsSigmaS + sigToaS * sigToaS + nlosVar);
        stamps.push_back(o);
        ++stampsTaken;
        ++stampsThisExchange;
    }

    /** The tier-1 exchange closed with this many replies -- the count
     *  AccountRangingExchange() was just handed, so the count it billed.
     *
     *  THE BILLING CHECK, per exchange rather than per run, because a run total can
     *  balance while individual exchanges do not. A stamp reaches the node's estimator
     *  on its gateway's reply, so an exchange may never yield more stamps than billed
     *  replies. The stamp test in rangeTier() is the reply test, so the excess counted
     *  here is zero by construction; it is counted anyway, because a later edit to
     *  either test would break it silently. Under --gatewayEdgeServer the stamps go over
     *  the backhaul and no reply is billed, so nothing is compared. */
    void NoteExchange(size_t replies)
    {
        repliesThisCycle = replies;
        ++exchanges;
        repliesOffered += replies;
        stampsOnExchanges += stampsThisExchange;
        if (billReplies && stampsThisExchange > replies)
        {
            stampsBeyondReplies += stampsThisExchange - replies;
        }
        stampsThisExchange = 0;
    }

    //! This cycle's fix set is tier 1 with >= 3 ranges: solve, and queue an OK fix.
    void Solve(CaTblPolicy& policy, const std::vector<CaTblRangeMeasurement>& meas)
    {
        pending = true;
        ++fixCycles;
        nRanges = meas.size();
        stampsOnFixCycles += stamps.size();
        rssiOk = CaTblPolicy::RawMultilateration(meas, policy.EstX(), policy.EstY(), rssiX,
                                                 rssiY);
        const double sx = rssiOk ? rssiX : policy.EstX();
        const double sy = rssiOk ? rssiY : policy.EstY();
        // The same ranges, weighted as Step() weights them (NLOS-tagged links inflated),
        // settle a three-gateway solve's choice between its two exact roots.
        hints.clear();
        const double infl = policy.GetConfig().nlosSigmaInflation;
        for (size_t k = 0; k < meas.size(); ++k)
        {
            CaTblTdoaRangeHint h;
            h.x = meas[k].anchorX;
            h.y = meas[k].anchorY;
            h.rangeM = meas[k].rangeM;
            h.sigmaM = meas[k].sigmaM * (meas[k].nlos ? infl : 1.0);
            hints.push_back(h);
        }
        res = CaTblSolveTdoa(stamps, sx, sy, CaTblTdoaConfig(), hints);
        ++verdicts[static_cast<int>(res.verdict)];
        if (res.verdict != CA_TBL_TDOA_OK)
        {
            return;
        }
        ++seeded[static_cast<int>(res.seededBy)];
        stampsUsed += stamps.size();
        repliesOnOk += billReplies ? repliesThisCycle : 0;
        downlinksOnOk += billDownlink ? 1 : 0;
        policy.SetAuxPositionFix(res.x, res.y, res.rxx, res.rxy, res.ryy);
    }

    //! After Step(): write the cycle's row. Only cycles that solved have one.
    void EndCycle(double t, uint64_t cycle, const Vector& truth, bool fused, bool rejected,
                  double estX, double estY)
    {
        if (!pending)
        {
            return;
        }
        const bool ok = (res.verdict == CA_TBL_TDOA_OK);
        fusedCount += fused ? 1 : 0;
        const double te = ok ? std::hypot(res.x - truth.x, res.y - truth.y) : -1.0;
        const double re = rssiOk ? std::hypot(rssiX - truth.x, rssiY - truth.y) : -1.0;
        log << t << "," << cycle << "," << nRanges << "," << stamps.size() << ","
            << (billReplies ? repliesThisCycle : 0) << "," << (billDownlink ? 1 : 0) << ","
            << CaTblTdoaVerdictName(res.verdict) << "," << CaTblTdoaSeedName(res.seededBy)
            << "," << res.iterations << "," << res.refAnchorId << "," << res.x << ","
            << res.y << "," << res.rxx << "," << res.rxy << "," << res.ryy << ","
            << res.chi2 << "," << (res.chanOk ? 1 : 0) << "," << truth.x << "," << truth.y
            << "," << te << "," << (rssiOk ? 1 : 0) << "," << re << "," << (fused ? 1 : 0)
            << "," << (rejected ? 1 : 0) << "," << std::hypot(estX - truth.x, estY - truth.y)
            << "," << res.nRoots << "," << res.tieChi2Chosen << "," << res.tieChi2Other
            << "\n";
        pending = false;
    }

    //! Per arm, to stdout: counts, their denominator, and the billing each stamp rode on.
    void Report(std::ostream& os, const CaTblPolicy& policy, const TierEnergyAccount& t1)
    {
        log.close();
        std::ostringstream o;
        o << "    TDoA tier 1: fix cycles " << fixCycles << " (tier-1 set >= 3 ranges), stamps "
          << stampsOnFixCycles << " on them (" << stampsTaken << " taken, " << nlosDelayed
          << " NLOS-delayed, " << unstampable << " unstampable); solves OK "
          << verdicts[CA_TBL_TDOA_OK] << " (chan " << seeded[CA_TBL_TDOA_SEED_CHAN] << ", rssi "
          << seeded[CA_TBL_TDOA_SEED_CALLER] << ", root " << seeded[CA_TBL_TDOA_SEED_ROOT]
          << "), refused TOO_FEW " << verdicts[CA_TBL_TDOA_TOO_FEW] << " COLLINEAR "
          << verdicts[CA_TBL_TDOA_COLLINEAR] << " SINGULAR " << verdicts[CA_TBL_TDOA_SINGULAR]
          << " NO_CONVERGE " << verdicts[CA_TBL_TDOA_NO_CONVERGE] << " BAD_INPUT "
          << verdicts[CA_TBL_TDOA_BAD_INPUT] << " NO_ROOT " << verdicts[CA_TBL_TDOA_NO_ROOT]
          << " AMBIGUOUS " << verdicts[CA_TBL_TDOA_AMBIGUOUS] << " DIVERGED "
          << verdicts[CA_TBL_TDOA_DIVERGED] << "; fused " << fusedCount << " (policy "
          << policy.GetAuxFusedCount() << ")\n";
        o << "    TDoA billing: " << stampsUsed << " stamps in OK solves, replies billed on "
          << "those cycles " << repliesOnOk << ", downlinks " << downlinksOnOk
          << "; node tier-1 exchanges " << exchanges << " (one billed broadcast each), "
          << "stamps " << stampsOnExchanges << " against replies " << repliesOffered
          << (billReplies ? " billed" : " not billed (edge server)")
          << ", stamps beyond a billed reply " << stampsBeyondReplies
          << "; tier-1 over the run: tx " << t1.txFrames << ", rx " << t1.rxFrames
          << ", exchanges " << t1.rangingExchanges << "\n";
        if (!log)
        {
            o << "    WARNING: tdoa log failed to flush\n";
        }
        os << o.str();
    }

    static constexpr double kLightMps = 299792458.0; // exact, SI definition

    Ptr<NormalRandomVariable> gauss;
    Ptr<ExponentialRandomVariable> expo;
    std::set<uint16_t> surveyed;
    double ppsSigmaS;
    uint8_t sf;
    double bwHz;
    bool billReplies;
    bool billDownlink;
    std::ofstream log;

    std::vector<CaTblTdoaObs> stamps;
    size_t repliesThisCycle = 0;
    bool pending = false;
    size_t nRanges = 0;
    CaTblTdoaResult res;
    bool rssiOk = false;
    double rssiX = 0.0;
    double rssiY = 0.0;

    uint64_t fixCycles = 0;         //!< denominator: cycles whose fix set is tier 1, >= 3
    uint64_t stampsTaken = 0;       //!< over every tier-1 exchange of the run
    uint64_t stampsOnFixCycles = 0; //!< of which on a fix cycle, offered to the solver
    uint64_t nlosDelayed = 0;       //!< stamps that drew an excess delay
    uint64_t unstampable = 0;       //!< gateway replies with no finite ToA bound
    uint64_t verdicts[CA_TBL_TDOA_VERDICT_COUNT] = {};
    uint64_t seeded[CA_TBL_TDOA_SEED_COUNT] = {};
    std::vector<CaTblTdoaRangeHint> hints;
    uint64_t stampsUsed = 0;        //!< stamps in OK solves
    uint64_t repliesOnOk = 0;       //!< tier-1 replies billed on OK-solve cycles
    uint64_t downlinksOnOk = 0;     //!< edge-server downlinks on OK-solve cycles
    uint64_t fusedCount = 0;        //!< OK fixes Step() fused
    size_t stampsThisExchange = 0;  //!< stamps since the last NoteExchange()
    uint64_t exchanges = 0;         //!< the node's tier-1 exchanges (rangeTier)
    uint64_t repliesOffered = 0;    //!< replies AccountRangingExchange() was handed
    uint64_t stampsOnExchanges = 0; //!< stamps, summed per exchange
    uint64_t stampsBeyondReplies = 0; //!< stamps an exchange had no billed reply for
};

/**
 * \brief --nlosClassifier: every link the measured node ranges gets the tier-specific
 *        NLOS_score of Physical sect 1.3 / 2.5, and every score is checked against the
 *        channel model's own LOS/NLOS draw (TECH_DEBT L.2.2; K.8 "the NLOS term is dead").
 *
 * THE CIRCULARITY, first, because it bounds everything below: the classifier can only
 * rediscover NLOS the channel model created. The UWB CIR is synthesised FROM the engine's
 * link state (its LOS/NLOS draw, K-factor, obstruction loss, SNR), and the 2.4 GHz and
 * 868 MHz scores read RSSI and range values the same engine derived from the same draw.
 * The confusion matrix therefore measures how much of the model's NLOS decision survives
 * into what a receiver may read -- not how the detectors would do on a real channel.
 *
 * WHAT IS SCORED: exactly the links rangeTier() pushes into a measurement set -- decoded,
 * finite, positive range -- on every tier, active or floor. A frame that was not decoded
 * has no CIR and no RSSI reading, so it is not scored; it is COUNTED, by its modelled
 * truth, as a lost link. That count is the denominator the confusion matrix leaves out,
 * and on UWB it is most of the story. Outdoors the engine's NLOS probability at UWB
 * distances is ~0.1% (1 - exp(-d / 8000 m) in RURAL_FORESTED); where it is not negligible
 * (--envFacility 5/6/7, scales 30-100 m) the UWB NLOS penalty -- both NLOS terms carry
 * +9.64 dB at 7.99 GHz -- exceeds UWB's margin. MEASURED at seed 7 over the default and
 * legacy topologies, --envFacility 1/5/6/7 and UWB triads of 2-10 m: 6 NLOS UWB links
 * polled, 6 lost, 0 decoded. So the CIR classifier's TP and FN cells are empty in every
 * scenario run tried, by the channel model rather than by the classifier.
 *
 * THE TRUTH is PhysicsResult*::isNLOS -- the propagation layer's own determination, not
 * the --nlosSource tag. The verdict is score >= --nlosScoreThreshold.
 *
 * SECT 6. Nothing here transmits and nothing here is charged. Every score reads a frame
 * rangeTier() already made and AccountRangingExchange() already billed.
 *
 * RANDOMNESS: only the UWB synthesis draws, from one stream set at construction,
 * seed*7919 + 1400053, so no automatic stream index is consumed and no engine draw moves.
 * With --pcsNlosClassifier off the scores are attached to the measurements and read by
 * nothing that decides, so every other output of the run is byte-identical to the flag off.
 *
 * OUTPUT: nlos_links_<arm>_seed<N>.csv, one row per scored-or-unscored decoded link, and
 * nlos_summary_<arm>_seed<N>.csv: the confusion matrix per tier, the lost links by truth,
 * and the PCS term's behaviour over the run's fix cycles.
 */
struct NlosScoring
{
    // The detector constants. OURS, not cited: a window of 8 readings per 868 link, a
    // value from the third reading; a robust reference over the tier's last 200 values,
    // scoring from the 20th; and the z-to-score ramp 1 -> 3 (within one robust sigma of
    // the typical link is noise; three is certainly not).
    static constexpr uint32_t kVarWindow = 8;
    static constexpr uint32_t kVarMinReadings = 3;
    static constexpr uint32_t kRefPool = 200;
    static constexpr uint32_t kRefMinPool = 20;
    static constexpr double kZLo = 1.0;
    static constexpr double kZHi = 3.0;
    static constexpr int kCols = 30;

    NlosScoring(uint32_t seed,
                double threshold,
                const LoRa2p4PhysicsConfig& c2p4,
                const LoRaUWBPhysicsConfig& cUwb,
                const std::string& linkPath,
                const std::string& summaryPath)
        : thr(threshold),
          cir(static_cast<int64_t>(seed) * 7919 + 1400053, CaTblCirConfig()),
          var868(kVarWindow, kVarMinReadings),
          ref868(kRefPool, kRefMinPool),
          ref2p4(kRefPool, kRefMinPool),
          prop2p4(c2p4.propagation),
          envUwb(cUwb.environment),
          accumGainUwbDb(10.0 * std::log10(std::max(cUwb.ranging.uwbPreambleSymbols, 1.0))),
          summaryFile(summaryPath)
    {
        log.open(linkPath.c_str());
        log << "t,cycle,tier,anchor,distM,truthNlos,nlosDb,kFactor,snrDb,score,verdict,"
               "cirSnrAccDb,cirDirectPow,cirLeNs,cirPeakNs,cirALe,cirAPeak,cirNoise,"
               "cirClusters,cirRays,tofRangeM,predRssiDbm,measRssiDbm,excessDb,winN,sRssiDb,"
               "sSnrDb,refMedian,refSigma,z\n";
    }

    //! Tier 1: variability of the link's RSSI/SNR over its consecutive readings, against
    //! the typical variability of the node's recent 868 links.
    double Score868(uint16_t anchorId, double t, uint64_t cycle, double distM,
                    const PhysicsResult& r)
    {
        const CaTblLinkVariability::Result v = var868.Push(anchorId, r.measuredRssiDbm, r.snr_db);
        double score = -1.0;
        std::vector<std::string> f = Head(t, cycle, "868", anchorId, distM, r.isNLOS, r.nlosDb,
                                          r.sampledKFactor, r.snr_db);
        f[22] = Num(r.measuredRssiDbm);
        f[24] = Num(v.n);
        if (v.valid)
        {
            const double s = std::max(v.sRssiDb, v.sSnrDb);
            double z = 0.0;
            double med = 0.0;
            double sig = 0.0;
            if (ref868.Score(s, z, med, sig))
            {
                score = CaTblNlosRamp(z, kZLo, kZHi);
                f[27] = Num(med);
                f[28] = Num(sig);
                f[29] = Num(z);
            }
            ref868.Add(s);
            f[25] = Num(v.sRssiDb);
            f[26] = Num(v.sSnrDb);
        }
        Finish(f, 0, r.isNLOS, score);
        return score;
    }

    //! Tier 2: the loss the RSSI shows beyond what the nominal path-loss model predicts at
    //! the ToF distance, against the typical residual of the node's recent 2.4 GHz links.
    double Score2p4(uint16_t anchorId, double t, uint64_t cycle, double distM, double rangeM,
                    double txPowerDbm, const PhysicsResult2p4& r)
    {
        const CaTblTofRssiResidual e =
            CaTblTofRssiExcess(rangeM, r.measuredRssiDbm, txPowerDbm, prop2p4);
        double score = -1.0;
        std::vector<std::string> f = Head(t, cycle, "2.4", anchorId, distM, r.isNLOS, r.nlosDb,
                                          r.sampledKFactor, r.snr_db);
        f[20] = Num(rangeM);
        f[22] = Num(r.measuredRssiDbm);
        if (e.valid)
        {
            f[21] = Num(e.predRssiDbm);
            f[23] = Num(e.excessDb);
            double z = 0.0;
            double med = 0.0;
            double sig = 0.0;
            if (ref2p4.Score(e.excessDb, z, med, sig))
            {
                score = CaTblNlosRamp(z, kZLo, kZHi); // one-sided: z < 0 scores 0
                f[27] = Num(med);
                f[28] = Num(sig);
                f[29] = Num(z);
            }
            ref2p4.Add(e.excessDb);
        }
        Finish(f, 1, r.isNLOS, score);
        return score;
    }

    //! Tier 3: synthesise the CIR from the link state and run the leading-edge test on it.
    double ScoreUwb(uint16_t anchorId, double t, uint64_t cycle, double distM,
                    const PhysicsResultUWB& r)
    {
        const double snrAcc = r.snr_db + accumGainUwbDb;
        const CaTblCirResult c = cir.Classify(envUwb, r.isNLOS, r.sampledKFactor, r.nlosDb, snrAcc);
        std::vector<std::string> f = Head(t, cycle, "UWB", anchorId, distM, r.isNLOS, r.nlosDb,
                                          r.sampledKFactor, r.snr_db);
        f[11] = Num(snrAcc);
        f[12] = Num(c.directPower);
        f[13] = c.detected ? Num(c.leDelayNs) : "";
        f[14] = Num(c.peakDelayNs);
        f[15] = Num(c.aLeadingEdge);
        f[16] = Num(c.aPeak);
        f[17] = Num(c.noiseSigma);
        f[18] = Num(c.nClusters);
        f[19] = Num(c.nRays);
        Finish(f, 2, r.isNLOS, c.score);
        return c.score;
    }

    //! A polled link that was not decoded: no CIR, no reading, nothing to score.
    void NoteLost(int tier, bool truthNlos) { ++lost[tier][truthNlos ? 1 : 0]; }

    //! After Step(): the PCS term the classifier produced this cycle, fed or not.
    void NoteStep(const CaTblDecision& d)
    {
        if (!d.hadFix)
        {
            return;
        }
        ++pcsFixCycles;
        if (d.nNlosScored == 0 || d.nlosScoreTerm < 0.0)
        {
            return;
        }
        const double v = d.nlosScoreTerm;
        ++pcsTermCycles;
        pcsFedCycles += d.nlosScoreFed ? 1 : 0;
        pcsSum += v;
        pcsMin = std::min(pcsMin, v);
        pcsMax = std::max(pcsMax, v);
        pcsDistinct.insert(v);
        if (havePrev && v != prevTerm)
        {
            ++pcsChanges;
        }
        prevTerm = v;
        havePrev = true;
        if (v > 0.0 && v < 1.0)
        {
            ++pcsInterior;
        }
        // The term the boolean tag would have supplied on the same set, for comparison.
        const double tag = (d.nAnchorsUsed > 0)
                               ? static_cast<double>(d.nAnchorsUsed - d.nNlos) / d.nAnchorsUsed
                               : 0.0;
        if (std::fabs(v - tag) > 1e-12)
        {
            ++pcsDiffersFromTag;
        }
    }

    void Report(std::ostream& os)
    {
        log.close();
        static const char* names[3] = {"868", "2.4", "UWB"};
        static const char* detectors[3] = {"rssi_snr_variability", "tof_vs_rssi", "cir_leading_edge"};
        std::ofstream s(summaryFile.c_str());
        s << std::setprecision(10);
        s << "kind,tier,detector,threshold,tp,fp,fn,tn,unscoredLos,unscoredNlos,lostLos,"
             "lostNlos,meanScoreLos,meanScoreNlos,fixCycles,termCycles,fedCycles,distinct,"
             "changes,interior,min,max,mean,differsFromTag\n";
        std::ostringstream o;
        o << "    NLOS classifier (threshold " << thr << "; truth = the engine's isNLOS):\n";
        for (int k = 0; k < 3; ++k)
        {
            const double mLos = nScored[k][0] ? scoreSum[k][0] / nScored[k][0] : -1.0;
            const double mNlos = nScored[k][1] ? scoreSum[k][1] / nScored[k][1] : -1.0;
            s << "confusion," << names[k] << "," << detectors[k] << "," << thr << "," << tp[k]
              << "," << fp[k] << "," << fn[k] << "," << tn[k] << "," << unscored[k][0] << ","
              << unscored[k][1] << "," << lost[k][0] << "," << lost[k][1] << "," << mLos << ","
              << mNlos << ",,,,,,,,,,\n";
            o << "      " << names[k] << " " << detectors[k] << ": tp " << tp[k] << " fp " << fp[k]
              << " fn " << fn[k] << " tn " << tn[k] << "; unscored LOS " << unscored[k][0]
              << " NLOS " << unscored[k][1] << "; lost before any reading LOS " << lost[k][0]
              << " NLOS " << lost[k][1] << "; mean score LOS " << mLos << " NLOS " << mNlos
              << "\n";
        }
        const double mean = pcsTermCycles ? pcsSum / pcsTermCycles : -1.0;
        const double lo = pcsTermCycles ? pcsMin : -1.0;
        const double hi = pcsTermCycles ? pcsMax : -1.0;
        s << "pcs,,,,,,,,,,,,,," << pcsFixCycles << "," << pcsTermCycles << "," << pcsFedCycles
          << "," << pcsDistinct.size() << "," << pcsChanges << "," << pcsInterior << "," << lo
          << "," << hi << "," << mean << "," << pcsDiffersFromTag << "\n";
        o << "    NLOS PCS term 3 = mean(1 - NLOS_score): fix cycles " << pcsFixCycles
          << ", with a scored link " << pcsTermCycles << ", fed to PCS " << pcsFedCycles
          << "; distinct values " << pcsDistinct.size() << ", changes " << pcsChanges
          << ", strictly inside (0,1) " << pcsInterior << ", range [" << lo << ", " << hi
          << "], mean " << mean << "; differs from the tag's LOS fraction on "
          << pcsDiffersFromTag << "\n";
        if (!log || !s)
        {
            o << "    WARNING: an NLOS classifier file failed to flush\n";
        }
        os << o.str();
    }

    double thr;
    CaTblCirSynth cir;
    CaTblLinkVariability var868;
    CaTblRobustRef ref868;
    CaTblRobustRef ref2p4;
    PropagationParams prop2p4;
    EnvironmentClass envUwb;
    double accumGainUwbDb;
    std::string summaryFile;
    std::ofstream log;

    uint64_t tp[3] = {};
    uint64_t fp[3] = {};
    uint64_t fn[3] = {};
    uint64_t tn[3] = {};
    uint64_t unscored[3][2] = {};
    uint64_t lost[3][2] = {};
    uint64_t nScored[3][2] = {};
    double scoreSum[3][2] = {};

    uint64_t pcsFixCycles = 0;      //!< denominator: Step() cycles with a fix
    uint64_t pcsTermCycles = 0;     //!< of which at least one link carried a score
    uint64_t pcsFedCycles = 0;      //!< of which the term replaced the tag inside PCS
    uint64_t pcsChanges = 0;        //!< consecutive term cycles whose values differ
    uint64_t pcsInterior = 0;       //!< term strictly between 0 and 1
    uint64_t pcsDiffersFromTag = 0; //!< term != the tag's LOS fraction on the same set
    double pcsSum = 0.0;
    double pcsMin = std::numeric_limits<double>::infinity();
    double pcsMax = -std::numeric_limits<double>::infinity();
    std::set<double> pcsDistinct;
    double prevTerm = 0.0;
    bool havePrev = false;

  private:
    void Tally(int tier, bool truthNlos, double score)
    {
        const int tr = truthNlos ? 1 : 0;
        if (score < 0.0)
        {
            ++unscored[tier][tr];
            return;
        }
        ++nScored[tier][tr];
        scoreSum[tier][tr] += score;
        const bool verdict = (score >= thr);
        if (verdict && truthNlos)
        {
            ++tp[tier];
        }
        else if (verdict)
        {
            ++fp[tier];
        }
        else if (truthNlos)
        {
            ++fn[tier];
        }
        else
        {
            ++tn[tier];
        }
    }

    template <typename T>
    static std::string Num(T v)
    {
        std::ostringstream o;
        o << std::setprecision(10) << v;
        return o.str();
    }

    //! The ten columns every row has; the verdict and score go in at Finish().
    std::vector<std::string> Head(double t, uint64_t cycle, const char* tier, uint16_t anchorId,
                                  double distM, bool truthNlos, double nlosDb, double k,
                                  double snrDb)
    {
        std::vector<std::string> f(kCols);
        f[0] = Num(t);
        f[1] = Num(cycle);
        f[2] = tier;
        f[3] = Num(anchorId);
        f[4] = Num(distM);
        f[5] = truthNlos ? "1" : "0";
        f[6] = Num(nlosDb);
        f[7] = Num(k);
        f[8] = Num(snrDb);
        return f;
    }

    void Finish(std::vector<std::string>& f, int tier, bool truthNlos, double score)
    {
        Tally(tier, truthNlos, score);
        f[9] = Num(score);
        f[10] = (score >= 0.0) ? ((score >= thr) ? "1" : "0") : "-1";
        for (int i = 0; i < kCols; ++i)
        {
            log << (i ? "," : "") << f[i];
        }
        log << "\n";
    }
};

/**
 * \brief L.2.5: write every proxy check of the run to proxy_checks_<arm>_seed<N>.csv and
 * summarise the held entries' ages on stdout.
 *
 * WHAT IT IS FOR. "The proxy refuses at the spec bound" was recorded as 7 refusals and 0
 * answers, and a count cannot say whether the bound missed by five seconds or by five
 * hundred -- which is the whole question when deciding whether 120 s is too tight. The
 * age is the observable; the refusal is only its comparison against one number.
 *
 * Called only under --queryProxy, and reads the MACs after the simulator has stopped, so
 * it cannot move behaviour. A function of its own rather than a block in RunArm, and kept
 * out of line, because a local added to RunArm has already once shifted floating-point
 * codegen there by one ulp with the feature off.
 */
static void __attribute__((noinline))
WriteProxyChecks(const std::deque<CaTblMac>& macs,
                 const std::string& traceDir,
                 CaTblPolicyMode mode,
                 uint32_t seed,
                 double boundS)
{
    static const char* const kClass[] = {"UNASSIGNED", "RESCUER", "TARGET", "ANCHOR"};
    std::ostringstream fn;
    fn << traceDir << "/proxy_checks_" << CaTblPolicyModeName(mode) << "_seed" << seed
       << ".csv";
    std::ofstream out(fn.str().c_str());
    if (out.is_open())
    {
        out << "t,queryId,nodeId,identityClass,tier,manifestId,held,answered,entryNodeId,"
               "entryTimeS,ageS,entryX,entryY,entryVarX,entryVarY,hopCount,seqNo\n";
    }
    uint64_t checks = 0, held = 0, answered = 0;
    uint64_t checksByTier[CA_TBL_TIER_COUNT] = {0, 0, 0};
    uint64_t heldByTier[CA_TBL_TIER_COUNT] = {0, 0, 0};
    std::vector<double> ages;
    std::set<uint32_t> queriesSeen;
    // The query side of the denominator. A proxy can only be asked by a query that
    // arrives, and a flood that never left its originator looks, from here, exactly
    // like a network that holds nothing -- so what the radios did with QUERY frames is
    // printed beside the checks. Summed over every radio, relays included.
    CaTblMsgCounters qc;
    for (size_t i = 0; i < macs.size(); ++i)
    {
        const CaTblMsgCounters& c = macs[i].GetCounters(CA_TBL_MSG_QUERY);
        qc.sent += c.sent;
        qc.rebroadcast += c.rebroadcast;
        qc.received += c.received;
        qc.droppedDutyCycle += c.droppedDutyCycle;
        qc.droppedCadExhausted += c.droppedCadExhausted;
        qc.lostCollision += c.lostCollision;
    }
    for (size_t i = 0; i < macs.size(); ++i)
    {
        const std::vector<CaTblProxyCheck>& pc = macs[i].GetProxyChecks();
        const uint8_t cls = static_cast<uint8_t>(macs[i].GetIdentityClass());
        for (size_t k = 0; k < pc.size(); ++k)
        {
            const CaTblProxyCheck& r = pc[k];
            ++checks;
            queriesSeen.insert(r.queryId);
            if (r.tier < CA_TBL_TIER_COUNT)
            {
                ++checksByTier[r.tier];
                heldByTier[r.tier] += r.held ? 1 : 0;
            }
            if (r.held)
            {
                ++held;
                ages.push_back(r.tS - r.entry.timestampS);
            }
            answered += r.answered ? 1 : 0;
            if (!out.is_open())
            {
                continue;
            }
            out << std::fixed << std::setprecision(6) << r.tS << ',' << r.queryId << ','
                << macs[i].GetNodeId() << ',' << (cls < 4 ? kClass[cls] : "?") << ','
                << int(r.tier) << ','
                << int(r.manifestId) << ',' << (r.held ? 1 : 0) << ','
                << (r.answered ? 1 : 0) << ',';
            if (r.held)
            {
                const CaTblTableEntry& e = r.entry;
                out << e.nodeId << ',' << e.timestampS << ',' << (r.tS - e.timestampS) << ','
                    << std::setprecision(3) << e.x << ',' << e.y << ',' << e.varX << ','
                    << e.varY << ',' << int(e.hopCount) << ',' << e.seqNo << '\n';
            }
            else
            {
                out << ",,,,,,,,\n";
            }
        }
    }
    out.close();

    // Nearest-rank quantiles over the HELD entries only: an age exists only where an
    // entry does, and the checks that held nothing are reported beside them rather than
    // folded in as zeros or infinities.
    std::sort(ages.begin(), ages.end());
    auto q = [&ages](double p) {
        if (ages.empty())
        {
            return -1.0;
        }
        size_t r = static_cast<size_t>(std::ceil(p * static_cast<double>(ages.size())));
        r = (r == 0) ? 1 : r;
        return ages[r - 1];
    };
    auto within = [&ages](double s) {
        return static_cast<uint64_t>(std::upper_bound(ages.begin(), ages.end(), s) -
                                     ages.begin());
    };
    std::ostringstream s;
    s << std::fixed << std::setprecision(1);
    s << "    NET4 queries (all radios): transmitted " << qc.sent << " (relayed "
      << qc.rebroadcast << "), received " << qc.received << ", refused by duty "
      << qc.droppedDutyCycle << ", CAD-exhausted " << qc.droppedCadExhausted
      << ", lost to collision " << qc.lostCollision << "\n";
    s << "    NET4 proxy checks: " << checks << " arrivals of " << queriesSeen.size()
      << " distinct queries at a radio not the target named, " << held
      << " held a matching entry (by the query's tier label: 868 " << heldByTier[0] << "/"
      << checksByTier[0] << ", 2.4 " << heldByTier[1] << "/" << checksByTier[1] << ", UWB "
      << heldByTier[2] << "/" << checksByTier[2] << "), " << answered
      << " inside the bound of " << boundS << " s\n";
    if (!ages.empty())
    {
        s << "    NET4 held-entry age at query (s): p10 " << q(0.10) << ", p25 " << q(0.25)
          << ", p50 " << q(0.50) << ", p75 " << q(0.75) << ", p90 " << q(0.90) << ", max "
          << ages.back() << "; within 30/60/120/300/600 s: " << within(30.0) << "/"
          << within(60.0) << "/" << within(120.0) << "/" << within(300.0) << "/"
          << within(600.0) << " of " << ages.size() << "\n";
    }
    std::cout << s.str();
}

/**
 * \brief Every flag the spec-conformance programme adds, carried into RunArm as ONE
 *        parameter (docs/SPEC_CONFORMANCE_PLAN.md rule 7).
 *
 * RunArm already takes 127 positional parameters, and HANDOFF_2026_09_09 sect 3 records
 * what that costs: a new one has to sit in the same position in the signature and in the
 * call, and a comma expression there is silent. New flags go in here instead, each with
 * the value that reproduces the build before it.
 */
struct SpecOptions
{
    //! DC-6. Explicit per-tier PCS thresholds; NaN = keep whatever the form set.
    //! pcsThrDown[CA_TBL_TIER_868] has no flag and stays NaN: nothing is below 868 MHz,
    //! so the policy's 868 MHz de-escalation threshold decides nothing.
    double pcsThrUp[CA_TBL_TIER_COUNT] = {std::numeric_limits<double>::quiet_NaN(),
                                          std::numeric_limits<double>::quiet_NaN(),
                                          std::numeric_limits<double>::quiet_NaN()};
    double pcsThrDown[CA_TBL_TIER_COUNT] = {std::numeric_limits<double>::quiet_NaN(),
                                            std::numeric_limits<double>::quiet_NaN(),
                                            std::numeric_limits<double>::quiet_NaN()};
    //! DC-7. --suspectPcsFactor; NaN = not given (the policy's own 0.5). Read only under
    //! --suspectRecovery=downweight, and refused anywhere else.
    double suspectPcsFactor = std::numeric_limits<double>::quiet_NaN();
    //! DC-8. --integrityNlosFromScore: the integrity check's eligibility from the mean
    //! NLOS_score rather than the tag. Needs --nlosClassifier.
    bool integrityNlosFromScore = false;
    //! DC-10. --entryVelocity: table entries carry the producer's velocity.
    bool entryVelocity = false;
    //! MC-1. --handshakeFixes: the escalation handshake as MAC sect 8 specifies it.
    bool handshakeFixes = false;
    //! MC-2. --triggeredUpdates: large-correction and last-gasp TRIGGERED_UPDATEs.
    bool triggeredUpdates = false;
    //! N.3 / row 55. --subjectTriggeredUpdates: subjects 2..N send the large correction too.
    bool subjectTriggeredUpdates = false;
    //! N.3 / row 55. --subjectBatteryDeplete: subjects 2..N run --batteryDepleteJ's fault too.
    bool subjectBatteryDeplete = false;
    //! N.21. --subjectFinderPseudoAnchor: a subject's finder range uses the finder's report.
    bool subjectFinderPseudoAnchor = false;
    //! N.21. --finderPseudoAnchorStrict: no finder range ever uses the true position.
    bool finderPseudoAnchorStrict = false;
    //! N.1. --silenceStopsRelay: a silenced radio relays no other node's frame.
    bool silenceStopsRelay = false;
    //! M.9. --reportRecedingPushes: count L.1.5's RECEDING eager push.
    bool reportRecedingPushes = false;
    //! N.6. --halfDuplex: 0 off, 1 count only, 2 enforce.
    uint32_t halfDuplex = 0;
    //! N.20. --deadBatteryNoDraw: a node whose battery has run out is not billed for
    //! listening or sleeping.
    bool deadBatteryNoDraw = false;
    //! MC-2. --triggeredUpdateK: a large correction is a move of more than k sigma.
    double triggeredUpdateK = 1.0;
    //! SY-1. --edgeUplinkRanges: bill the uplink of tier-2/3 ranges to the gateways.
    bool edgeUplinkRanges = false;
    //! N.3 / row 55. --subjectEdgeUplink: subjects 2..N bill it too.
    bool subjectEdgeUplink = false;
    //! SY-5. --injectTxPowerDropDb / --injectAntennaLossDb, over the fault window.
    double injectTxPowerDropDb = 0.0;
    double injectAntennaLossDb = 0.0;
    //! DC-11. --sparseInStep: Cases A/B solved inside Step() against the propagated prior.
    bool sparseInStep = false;
    //! DC-1. --imuPredict: the prediction takes a synthetic-IMU displacement.
    bool imuPredict = false;
    //! DC-4. --caseD: a single range as a constraint on the propagated state.
    bool caseD = false;
    //! DC-13. --entryPrediction: held entries propagated as x + v age, P + Q_src age^2.
    bool entryPrediction = false;
    //! DC-9. --imuWitnessSource: "" or "shs" (the SHS absolute track), or "model".
    std::string imuWitnessSource;
    //! NW-0. --meshLinks: "" or "star" (field node to anchor only), or "all".
    std::string meshLinks;
    //! NW-8. --reversePathHop: with --reversePath, the reverse path is hop by hop.
    bool reversePathHop = false;
    //! NW-7. --grantGatedUwb: an escalation into UWB commits only on a delivered grant.
    bool grantGatedUwb = false;
    //! NW-9. --queryV2: QUERY and RESPONSE carry what the spec says.
    bool queryV2 = false;
    //! NW-9. --identityQueryManifests: the list a v2 query asks for (empty = the target).
    std::vector<uint8_t> identityQueryManifests;
    //! NW-4. --queryTriggeredEscalation, and --identityQueryUrgent (queries sent URGENT).
    bool queryTriggeredEscalation = false;
    bool identityQueryUrgent = false;
    //! NW-5. --arbitrationV2: arbitration as Network sect 5 specifies.
    bool arbitrationV2 = false;
    //! NW-6. --gossipQueue: anchors gossip their queue as well as their grants.
    bool gossipQueue = false;
    //! --arbSlotHoldS: how long an awarded UWB slot is held (CaTblMacConfig::arbSlotHoldS).
    double arbSlotHoldS = 10.0;
    //! NW-1. --hopProvenance: the hop count of a fused set reaches Step() and the RESULT.
    bool hopProvenance = false;
    //! N.15. --subjectRelayChain: measured subjects take part in the L.1.2 relay chain.
    bool subjectRelayChain = false;
    //! N.14. --fuseFloorActive: an active tier short of three is fused with the floor.
    bool fuseFloorActive = false;
    //! NW-1 step 3. --hopVarianceTerms: per-hop variability and NLOS enter a leg's sigma.
    bool hopVarianceTerms = false;
    //! NW-2. --fixRoutingSubjects: every measured subject routes its fixes on its own tier.
    bool fixRoutingSubjects = false;
    //! DC-3. --caseCRelative: Case C's relative shape, assembled at the base station.
    bool caseCRelative = false;
    //! NW-2. The confidence cost's weights and slack (CaTblRoutingWeights), OURS.
    double routeAlpha = 1.0;
    double routeBeta = 1.0;
    double routeGamma = 1.0;
    uint32_t routeSlack = 1;
    //! TA-1a. --sessionOverMesh: the base station's session is fed by delivered RESPONSEs.
    bool sessionOverMesh = false;
    double sessionStalenessS = 120.0;
    double sessionTimeoutS = 600.0;
    //! TA-3. --sessionPreciseOnGrant: PRECISE is a confirmed grant in a delivered entry.
    bool sessionPreciseOnGrant = false;
    //! TA-1b. --finderSessions: every finder runs its own search for its assigned subject.
    bool finderSessions = false;
    //! TA-1b. --finderRequesterEscalation: a finder's session asks for UWB as the base does.
    bool finderRequesterEscalation = false;
    //! TA-1b. --finderCancelM: a finder this close to its target cancels its session.
    double finderCancelM = 2.0;
    //! TA-2. --requesterEscalation: the base station asks for UWB on a target's behalf.
    bool requesterEscalation = false;
    //! MS-1. --measuredSubjects: field nodes localised by the full pipeline (1 = tag 0).
    uint32_t measuredSubjects = 1;
    //! MS-1. --finderTasking: "" or "tag0" (every finder searches for tag 0), or
    //! "roundrobin" (finder k for subject (k mod N) + 1).
    std::string finderTasking;
    //! NW-3. --queryTierBinding: "" (not given) and "first" bind every tier's query driver
    //! to the first anchor MAC -- gateway 101's 868 MHz radio; "per-tier" binds each to
    //! that tier's radio. Given explicitly, either value prints the binding.
    std::string queryTierBinding;
};

/**
 * \brief DC-6. The last two steps of resolving a run's PCS thresholds: --pcsHysteresisBand,
 *        then the explicit per-tier values, so an explicit value always wins.
 *
 * RunArm applies it after the form's own thresholds (v2's are derived). main() applies it
 * to a probe config built the same way, to refuse an inverted pair and to print what was
 * resolved -- one function, so the refusal and the run cannot disagree about the values.
 */
static void
ResolvePcsThresholds(CaTblPolicyConfig& pcfg, double pcsHysteresisBand, const SpecOptions& spec)
{
    // A6 / CAL-03 / CAL-04. Applied LAST so it composes with either score:
    // ApplyV2Thresholds() has already run for v2, and the band is expressed
    // RELATIVE to whatever thresholdUp ended up being. The BAND, not the two
    // absolute thresholds, is the swept quantity -- 868 MHz and 2.4 GHz carry a
    // 0.20 gap today and UWB 0.25, so one number moves the hysteresis without also
    // silently moving the operating point, which sweeping thresholdDown alone
    // would do.
    if (pcsHysteresisBand >= 0.0)
    {
        for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
        {
            pcfg.pcsThresholdDown[k] = pcfg.pcsThresholdUp[k] + pcsHysteresisBand;
        }
    }
    // NaN (the default) leaves each untouched.
    for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
    {
        if (std::isfinite(spec.pcsThrUp[k]))
        {
            pcfg.pcsThresholdUp[k] = spec.pcsThrUp[k];
        }
        if (std::isfinite(spec.pcsThrDown[k]))
        {
            pcfg.pcsThresholdDown[k] = spec.pcsThrDown[k];
        }
    }
}

ArmResult
RunArm(CaTblPolicyMode mode,
       uint32_t seed,
       double durationS,
       double dt,
       EnvironmentClass envWide,
       EnvironmentClass envFacility,
       VegetationType vegWide,
       VegetationType vegFacility,
       const Deployment& dep,
       const TierPhy phy[CA_TBL_TIER_COUNT],
       double walkSpeedMps,
       double dwellS,
       const double* beaconPeriodS,
       double nbrTtlMult,
       double searchDutyFrac,
       const std::string& traceDir,
       bool writeTrace,
       int fastDemoteMode,
       bool mahalUseR,
       CaTblSuspectRecovery suspectRecovery,
       double suspectInflateFactor,
       uint32_t suspectRecoveryN,
       bool escalationRequiresEvidence,
       bool escalationRequiresEvidenceT2,
       double t2PredictRadiusM,
       uint32_t t2PredictMinAnchors,
       bool uwbEvidenceFromNeighbours,
       int multiStepMode,
       double demoteStalenessS,
       CaTblNlosSource nlosSource,
       bool suspectForcesEscalation,
       CaTblPcsForm pcsForm,
       double pcsW1,
       double pcsW2,
       double pcsW3,
       double pcsGateFloor,
       double pcsHysteresisBand,
       uint32_t nDeescalateOverride,
       bool sendHandshakeFrames,
       bool deferEscalationOnDutyLimit,
       bool intraCycleHandshakeDefer,
       uint32_t nTags,
       bool relayedRanging,
       uint32_t relayTier,
       uint32_t maxRelayDepth,
       double relayVarBudgetM2,
       bool vizEventsOn,
       double vizTablePeriodS,
       bool gwRidge,
       double gwRidgeY,
       double gwArcDeg,
       double gwArcCentreDeg,
       double trailDeviateSide,
       double trailDeviateAtS,
       double trailStopAfterS,
       uint32_t nFinders,
       uint32_t finderMode,
       double finderStaleSteerBoundS,
       double searchLaunchOnLossS,
       double searchLaunchSigmaM,
       double searchLaunchAtS,
       double maxTurnRateRadS,
       double shsTrustedFixK,
       bool imuFusion,
       bool imuIdeal,
       bool queryProxy,
       bool tableFuseCI,
       bool digestSummary,
       double batteryDepleteJ,
       bool sessionLayer,
       bool mobilityFromRange,
       bool lifecycleTracking,
       bool batchSearch,
       bool sparseCaseA,
       double sparseCaseAMarginM,
       uint32_t injectSilentId,
       double injectSilentStartS,
       double injectSilentEndS,
       double queryStalenessBoundS,
       bool reversePath,
       bool shsStepBounds,
       bool shsSnapPosition,
       double finderSpeedMps,
       double finderDigestPeriodS,
       double finderPosSigmaM,
       bool finderPseudoAnchor,
       bool bidirectionalDigest,
       bool tier1ChargeReplies,
       bool gatewayEdgeServer,
       double processNoiseStdPos,
       double processNoiseStdVel,
       double finderDispersal,
       bool uwbCoverageIncludesFinders,
       uint32_t hopProvenanceForm,
       double hopPenalty,
       bool imuCrossCheck,
       double imuDriftMps,
       double imuConsistencyK,
       double imuSigmaFloorM,
       bool corroborationCheck,
       double corroborationK,
       double corroborationRangeSigmaM,
       double identityQueryPeriodS,
       uint32_t identityQueryTarget,
       bool uwbArbitration,
       double arbBatteryCapJ,
       bool arbitrateDutyBudget,
       double fixedVarThresholdM2,
       double pcsSigmaTargetM,
       bool floorOnDemand,
       uint32_t floorSkipK,
       uint32_t floorHeartbeatN,
       double injectRangeBiasM,
       double injectBiasFraction,
       uint32_t injectFaultAnchors,
       double anchorGrantGossipS,
       bool relayChain,
       bool fixRouting,
       bool fixRoutingConfidence,
       bool tier1Tdoa,
       double tdoaPpsSigmaNs,
       bool nlosClassifier,
       bool pcsNlosClassifier,
       double nlosScoreThreshold,
       bool relaySlots,
       bool edgeDownlinkOnFixTier,
       const SpecOptions& spec)
{
    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(1);
    // WITHOUT THIS LINE NO TWO ARMS SHARE A CHANNEL REALISATION, AND NONE EVER DID.
    //
    // LoRaDistributionSampler's constructor takes a uint32_t seed and never uses it
    // (lora-distribution-sampler.cc:8). It creates five CreateObject<...
    // RandomVariable>() objects with no SetStream() call anywhere in the file, so
    // each one takes the next automatic stream index. RngSeedManager::SetSeed()
    // does NOT reset that counter -- ResetNextStreamIndex() is a separate call
    // (rng-seed-manager.cc:71 vs 104) -- so arm 6's engines begin where arm 5's
    // engines stopped, and every arm draws from different substreams.
    //
    // Measured: ca_tbl_full and ca_tbl_no_integrity, identical EKF state and
    // identical poll counts, reached 334 vs 336 engine evaluations by t=2.5 -- the
    // first cycle, before either arm's policy had executed a single decision. At
    // t=7.5 the same six anchors returned 6 decodes on one arm and 2 on the other.
    //
    // This invalidated every cross-arm comparison the project has made, including
    // the integrity ablation, whose two arms differ by 8.1% RMSE while the check
    // itself fires zero times. Resetting the index makes the automatic assignments
    // identical per arm; the explicit streams below are far above the automatic
    // range and cannot collide with them.
    //
    // NOT SUFFICIENT ON ITS OWN. Two file-scope caches also survive across arms --
    // g_fadingComponents (lora-fading-submodule.cc:6) and g_foliageDynamicCache
    // (lora-foliage-submodule.cc:8) -- and the second stores a Time, which
    // Simulator::Destroy() resets to zero underneath it.
    RngSeedManager::ResetNextStreamIndex();

    CA_TBL_MAKE_CONFIGS(envWide, envFacility, seed, vegWide, vegFacility, cfg868, cfg2p4, cfgUwb)
    if (g_sy3Given)
    {
        // SY-3. Read back from the configs the engines are about to be built from, so the
        // sensitivity offset is observed where it acts, not where it was computed.
        std::ostringstream line; // its own stream: cout's precision is left alone
        line << "    SY-3 engine sensitivity: 868 MHz SF" << int(phy[0].sf) << " "
             << std::setprecision(9)
             << cfg868.receiver.sensitivityDbm[std::min(5, std::max(0, int(phy[0].sf) - 7))]
             << " dBm; 2.4 GHz SF" << int(phy[1].sf) << " "
             << cfg2p4.receiver.sensitivityDbm[std::min(7, std::max(0, int(phy[1].sf) - 5))]
             << " dBm\n";
        std::cout << line.str();
    }

    LoRaPhysicsEngine engine868;
    engine868.Initialize(cfg868);
    LoRa2p4PhysicsEngine engine2p4;
    engine2p4.Initialize(cfg2p4);
    LoRaUWBPhysicsEngine engineUwb;
    engineUwb.Initialize(cfgUwb);

    // C2. All per-tag state lives here now. tags[0] is the tag this arm has always
    // measured; tags[1..] are additional MEASURED targets, each with its own filter,
    // confidence score, energy account and statistics.
    //
    // The aliases below are what let every line of setup and cycle code stay exactly
    // as it was: they bind the old names to tag 0's members. Without them this would
    // be a rename across roughly 750 lines, which is not a diff anyone can review.
    std::vector<CaTblTagState> tags(std::max<uint32_t>(1, nTags));
    for (CaTblTagState& ts : tags)
    {
        ApplyRadioBasis(ts.energy); // SY-2: every field node carries the same radios
    }
    tags[0].nodeId = 1;
    const uint32_t nodeId = tags[0].nodeId;
    CaTblEnergyModel& energy = tags[0].energy;
    CaTblPolicy& policy = tags[0].policy;
    CaTblMac** nodeMac = tags[0].nodeMac;
    // Setup-scope aliases to tag 0, so every line below is unchanged.
    CaTblTagState& TS0 = tags[0];
    auto& beaconToaS = TS0.beaconToaS;
    auto& lastRx = TS0.lastRx;
    auto& lastNonEmptyAtS = TS0.lastNonEmptyAtS;
    auto& nbrSum = TS0.nbrSum;
    auto& blindCycles = TS0.blindCycles;
    auto& floorHealthyStreak = TS0.floorHealthyStreak;
    auto& cycleIndex = TS0.cycleIndex;
    auto& floorRangedCycles = TS0.floorRangedCycles;
    auto& handshakeDeferrals = TS0.handshakeDeferrals;
    auto& handshakeIntraCycleDefers = TS0.handshakeIntraCycleDefers;
    auto& handshakeIntraCycleWaitS = TS0.handshakeIntraCycleWaitS;
    auto& handshakeDeferralDelayS = TS0.handshakeDeferralDelayS;
    auto& handshakePreemptions = TS0.handshakePreemptions;
    auto& escalationsDeferredAway = TS0.escalationsDeferredAway;
    auto& pendingHandshake = TS0.pendingHandshake;
    auto& pendingHandshakeTier = TS0.pendingHandshakeTier;
    auto& directEngineCalls = TS0.directEngineCalls;
    auto& errors = TS0.errors;
    auto& errorsFixed = TS0.errorsFixed;
    auto& steps = TS0.steps;
    auto& tp = TS0.tp;
    auto& fp = TS0.fp;
    auto& fn = TS0.fn;
    auto& tn = TS0.tn;
    auto& inCov = TS0.inCov;
    auto& staleSteerRefusals = TS0.staleSteerRefusals;
    auto& lostContactSinceS = TS0.lostContactSinceS;
    auto& lowConfSinceS = TS0.lowConfSinceS;
    auto& partyDeployed = TS0.partyDeployed;
    auto& deployedAtS = TS0.deployedAtS;
    auto& lastReportedSigmaM = TS0.lastReportedSigmaM;
    auto& lastReportHadFix = TS0.lastReportHadFix;
    auto& deployCause = TS0.deployCause;
    auto& batteryDead = TS0.batteryDead;
    auto& batteryDeadAtS = TS0.batteryDeadAtS;
    auto& lifecycle = TS0.lifecycle;
    auto& sessions = TS0.sessions;
    auto& sessionId = TS0.sessionId;
    auto& renderCounts = TS0.renderCounts;
    auto& caseAAttempts = TS0.caseAAttempts;
    auto& caseSolved = TS0.caseSolved;
    auto& prevMeanRangeM = TS0.prevMeanRangeM;
    auto& mobCounts = TS0.mobCounts;
    auto& caseAAdmitted = TS0.caseAAdmitted;
    auto& caseARefused = TS0.caseARefused;

    // Each tier's anchors are registered only on that tier's engine. Registering
    // all anchors on all engines would put UWB anchors kilometres out of range
    // into the 868 solver's geometry and vice versa.
    for (size_t i = 0; i < dep.anchors[CA_TBL_TIER_868].size(); ++i)
    {
        engine868.RegisterGateway(dep.anchors[CA_TBL_TIER_868][i].id,
                                  dep.anchors[CA_TBL_TIER_868][i].pos,
                                  true);
    }
    for (size_t i = 0; i < dep.anchors[CA_TBL_TIER_2P4].size(); ++i)
    {
        engine2p4.RegisterGateway(dep.anchors[CA_TBL_TIER_2P4][i].id,
                                  dep.anchors[CA_TBL_TIER_2P4][i].pos,
                                  true);
    }
    for (size_t i = 0; i < dep.anchors[CA_TBL_TIER_UWB].size(); ++i)
    {
        engineUwb.RegisterGateway(dep.anchors[CA_TBL_TIER_UWB][i].id,
                                  dep.anchors[CA_TBL_TIER_UWB][i].pos,
                                  true);
    }

    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(static_cast<int64_t>(seed) * 7919 + 13);

    Tourist walker(dep.facilities, walkSpeedMps, dwellS, dep.boxHalfM, dep.dwellRadiusM, rng);
    if (trailDeviateAtS > 0.0)
    {
        // The trail sits just off the ridge, so the walker starts inside good coverage
        // and walks out of it. Starting on the ridge itself would put the subject at the
        // one place the collinear geometry is least bad, and the deviation would then
        // conflate leaving the trail with leaving the only good conditioning.
        walker.SetTrail(gwRidgeY * dep.boxHalfM - dep.boxHalfM * 0.08, trailDeviateAtS,
                        trailStopAfterS, trailDeviateSide);
    }
    // The SUBJECT is the one the SHS rides on, so this is the walker whose
    // continuity decides whether a yaw integrator can track it at all.
    walker.SetMaxTurnRate(maxTurnRateRadS);
    // LAST KNOWN POSITION. With a base camp the subject starts AT it and walks
    // out, which is what a search actually begins from and what gives the finders
    // something to converge on. The RNG draw in the constructor happens either
    // way, so the stream does not move.
    if (dep.hasBaseCamp)
    {
        if (walkSpeedMps > 0.0 || dep.facilities.empty())
        {
            walker.SetPosition(dep.baseCampX, dep.baseCampY);
        }
        else
        {
            // A STILL subject cannot walk out of the camp, so placing it there
            // would park it among the gateways for the whole run -- which is what
            // invalidated K9's three still cells. A real stationary casualty was
            // SEEN at the camp, walked out, and then stopped, so it starts at the
            // first tour site, which the keep-out already puts beyond the
            // infrastructure.
            walker.SetPosition(dep.facilities[0].x, dep.facilities[0].y);
        }
    }
    tags[0].walker = &walker;
    Vector pos = walker.Position();
    engine868.RegisterNode(nodeId, pos);
    engine2p4.RegisterNode(nodeId, pos);
    engineUwb.RegisterNode(nodeId, pos);

    for (int t = 0; t < CA_TBL_TIER_COUNT; ++t)
    {
        energy.SetLoRaPhy(static_cast<CaTblTier>(t), phy[t].sf, phy[t].bandwidthHz, 1);
        energy.SetTier1ChargeReplies(tier1ChargeReplies);
        energy.SetEdgeServer(gatewayEdgeServer);
    }

    CaTblPolicyConfig pcfg;
    pcfg.mode = mode;
    pcfg.mahalUseR = mahalUseR;
    pcfg.suspectRecovery = suspectRecovery;
    pcfg.suspectInflateFactor = suspectInflateFactor;
    pcfg.suspectRecoveryN = suspectRecoveryN;
    if (std::isfinite(spec.suspectPcsFactor))
    {
        pcfg.suspectPcsFactor = spec.suspectPcsFactor;
    }
    if (spec.integrityNlosFromScore)
    {
        pcfg.integrityNlosFromScore = true;
        pcfg.integrityNlosScoreThreshold = nlosScoreThreshold;
    }
    if (spec.sparseInStep)
    {
        pcfg.sparseInStep = true;
        pcfg.sparseMarginM = sparseCaseAMarginM;
    }
    if (spec.caseD)
    {
        pcfg.caseD = true;
    }
    if (spec.grantGatedUwb)
    {
        pcfg.grantGatedUwb = true; // NW-7
    }
    if (spec.queryTriggeredEscalation)
    {
        pcfg.queryTriggeredEscalation = true; // NW-4
    }
    pcfg.escalationRequiresEvidence = escalationRequiresEvidence;
    pcfg.escalationRequiresEvidenceT2 = escalationRequiresEvidenceT2;
    pcfg.t2PredictRadiusM = t2PredictRadiusM;
    pcfg.t2PredictMinAnchors = t2PredictMinAnchors;
    pcfg.uwbEvidenceFromNeighbours = uwbEvidenceFromNeighbours;
    // The map the predicate needs. The node must already hold anchor coordinates to
    // multilaterate at all, so supplying them grants no information it lacks; an
    // empty vector disables the UWB half of the predicate by construction.
    for (const AnchorDef& a : dep.anchors[CA_TBL_TIER_UWB])
    {
        pcfg.uwbAnchorX.push_back(a.pos.x);
        pcfg.uwbAnchorY.push_back(a.pos.y);
    }
    pcfg.suspectForcesEscalation = suspectForcesEscalation;
    pcfg.pcsForm = pcsForm;
    pcfg.processNoiseStdPos = processNoiseStdPos;
    pcfg.processNoiseStdVel = processNoiseStdVel;
    // Non-positive means leave the built-in default, matching the sentinel
    // convention fixedVarThresholdM2 and pcsSigmaTargetM already use below.
    // Zero is a weight since 2026-09-30 (DC-6): LOOCV's answer is (3, 0, 0), and "> 0"
    // silently turned a requested 0 into the hand-set default. The sentinel is -1.
    if (pcsW1 >= 0.0) { pcfg.pcsW1 = pcsW1; }
    if (pcsW2 >= 0.0) { pcfg.pcsW2 = pcsW2; }
    if (pcsW3 >= 0.0) { pcfg.pcsW3 = pcsW3; }
    if (pcsGateFloor > 0.0) { pcfg.pcsGateFloor = pcsGateFloor; }
    // --pcsNlosClassifier. Only the measured node's policy: its measurements are the only
    // ones scored, so a relay's policy would find no score and fall back to the tag anyway.
    pcfg.pcsNlosFromScore = pcsNlosClassifier;
    // Operating-point knobs for the Pareto sweep. Both express the SAME physical
    // quantity -- how much position uncertainty is tolerated before energy is
    // spent -- so the two decision rules can be swept over a common grid and
    // compared as fronts rather than as two arbitrarily-placed points.
    if (fixedVarThresholdM2 > 0.0)
    {
        pcfg.fixedVarThresholdM2 = fixedVarThresholdM2;
    }
    if (pcsSigmaTargetM > 0.0)
    {
        // traceRef IS the target, as a variance. Setting it here keeps the single
        // source of truth: the v2 threshold stays derived at 0.0 and the operating
        // point moves by moving the specification, not by hand-placing a threshold.
        pcfg.traceRef = pcsSigmaTargetM * pcsSigmaTargetM;
    }
    if (pcsForm == CA_TBL_PCS_V2)
    {
        // Pre-registered thresholds, derived from traceRef -- see
        // CaTblPolicyConfig::ApplyV2Thresholds() and
        // report/PCS_REDESIGN_RESULTS.md §0.2/§0.3. The legacy 1.55/1.70/2.30 are
        // deliberately NOT carried onto a differently-scaled score.
        pcfg.ApplyV2Thresholds();
    }
    // fastDemoteMode: 0 = nobody, 1 = the proposed arms only, 2 = every dynamic arm.
    // The three-way split exists because the verdict depends on how the baseline is
    // credited: fixed_threshold is ALSO burning ~98% of its energy on blind UWB
    // discovery at boxHalf 3000, so giving the fix only to the proposed system and
    // giving it to both are different experiments. Report the matrix, do not choose.
    if (fastDemoteMode == 2)
    {
        pcfg.fastDemoteOnZeroNeighbours = true;
    }
    else if (fastDemoteMode == 1)
    {
        pcfg.fastDemoteOnZeroNeighbours =
            (mode == CA_TBL_MODE_FULL || mode == CA_TBL_MODE_NO_INTEGRITY);
    }
    // Separate axis from fastDemote deliberately: demoting SOONER and demoting
    // FURTHER are different mechanisms, and fast demotion already turned out to have
    // opposite signs at the two geometries and to favour the baseline. Conflating
    // them into one switch would make that unattributable.
    if (multiStepMode == 2)
    {
        pcfg.multiStepDemote = true;
    }
    else if (multiStepMode == 1)
    {
        pcfg.multiStepDemote =
            (mode == CA_TBL_MODE_FULL || mode == CA_TBL_MODE_NO_INTEGRITY);
    }
    // The band, then DC-6's explicit per-tier thresholds; see ResolvePcsThresholds.
    ResolvePcsThresholds(pcfg, pcsHysteresisBand, spec);
    if (nDeescalateOverride > 0) { pcfg.nDeescalate = nDeescalateOverride; }
    pcfg.hopProvenanceForm = hopProvenanceForm;
    pcfg.hopPenalty = hopPenalty;
    pcfg.imuCrossCheck = imuCrossCheck;
    if (spec.imuWitnessSource == "model")
    {
        pcfg.witnessCounts = true; // DC-9
    }
    pcfg.corroborationCheck = corroborationCheck;
    policy.Configure(pcfg);
    // Seeded at the true start position with a deliberately loose covariance:
    // the tag knows roughly where it deployed, not exactly.
    // THE FILTER STARTS WHERE THE SUBJECT WAS LAST SEEN, NOT AT THE TRUTH.
    //
    // With a base camp the last known position is the camp. For a MOVING subject
    // that is also where it starts, so this is identical to seeding at truth and
    // nothing changes. For a STILL subject, which is placed out at a tour site,
    // seeding at truth would hand it perfect knowledge of a position it has never
    // had coverage at -- and it would then report rmse 0.0000 for the whole run
    // without ever receiving a measurement, which is what the still-cell check
    // caught.
    //
    // The prior must be WIDE or the fix is worse than the defect: a filter seeded
    // kilometres from truth with a 100 m std is grossly overconfident, and the
    // Mahalanobis gate would reject every measurement that could correct it. So
    // the prior is how far the subject could plausibly have gone since it was
    // last seen, floored at the historical 100 m.
    double initX = pos.x;
    double initY = pos.y;
    double initStdM = 100.0;
    if (dep.hasBaseCamp)
    {
        initX = dep.baseCampX;
        initY = dep.baseCampY;
        const double away = std::hypot(pos.x - initX, pos.y - initY);
        initStdM = std::max(100.0, away);
    }
    policy.Initialize(initX, initY, initStdM, 5.0);

    // ---- S4 MAC substrate ---------------------------------------------------
    //
    // Until now CaTblPhyProvider had exactly one implementation, StubPhy in the
    // packet validator, and ca_tbl_causal_sim never instantiated the MAC at all --
    // it called ProcessTransmission() directly in a sequential loop and never
    // entered the discrete-event scheduler. Claims 4, 5 and 6 were therefore
    // "substrate done, not wired", and the ranging poll addressed every anchor in
    // the deployment because there was no protocol layer to tell it which anchors
    // existed.
    //
    // WHAT THIS CHANGES ABOUT COMPARABILITY -- read before comparing to any table
    // taken before this commit. The MAC issues real beacon transmissions, and each
    // one calls into the physics engines. Those calls draw from the engines' RNG
    // streams. Every channel realisation therefore differs from the pre-MAC runs,
    // on every arm, for a reason that has nothing to do with the policy. The
    // arm_summary.csv produced after this change is a NEW baseline, not a delta
    // against the old one. Determinism from a seed is preserved (and must be
    // re-verified); comparability with the previous numbers is not.
    //
    // Energy is deliberately NOT charged through the MAC. CaTblMac only ever calls
    // AccountCad() and NoteDeferred(); it does not charge TX or RX joules. Leaving
    // SetEnergyModel() unset on every MAC here keeps all energy in the explicit
    // AccountRangingExchange / AccountEscalationHandshake / AccountListen path, so
    // the ONLY intended changes to the energy axis are (a) the poll addresses the
    // neighbour table instead of the deployment and (b) beacon reception is now
    // charged. If CAD were also charged by the MAC it would be double-counted
    // against rangeTier()'s own AccountCad(), and the energy delta would stop being
    // attributable to the gating -- which is the entire point of the change.
    Ptr<UniformRandomVariable> macRng = CreateObject<UniformRandomVariable>();
    // Explicit stream, set before first use, so no global stream index is consumed
    // and the engines' and Tourist's existing streams are not shifted underneath
    // them. Same value on every arm: arms must differ in how many draws they
    // consume, not in which sequence they consume from.
    macRng->SetStream(static_cast<int64_t>(seed) * 7919 + 104729);

    EnginePhyProvider provider(engine868,
                               engine2p4,
                               engineUwb,
                               phy,
                               static_cast<uint16_t>(nodeId));
    provider.SetNlosSource(nlosSource);
    provider.SetNodePosition(pos);

    // One channel per tier. CaTblMac holds a single channel pointer, so a node
    // that must exist on all three tiers needs one MAC per tier -- which it needs
    // anyway, because CaTblMacConfig carries a single sf/bandwidth pair and a
    // shared MAC would compute a 2.4 GHz frame's time-on-air at 125 kHz: 51.5 ms
    // instead of 8.0 ms, a factor of 6.4 straight into the duty-cycle governor and
    // the collision overlap test.
    CaTblChannel chan[CA_TBL_TIER_COUNT];

    // Per-frame event log for the visualiser. Opened only when asked for, and the
    // channel treats it as write-only, so with the flag off no stream exists, nothing
    // is written and no branch differs. It records the SAME events the aggregate
    // counters already count -- itemised, so an animation can show them one at a time
    // rather than as a total.
    std::ofstream vizEvents;
    std::ofstream vizNodes;
    // PER-NODE TABLE SNAPSHOTS. Two files, because they are two different things and
    // conflating them is the mistake the header of CaTblTableEntry warns about:
    //
    //   nbrtable  -- what this node has PERSONALLY HEARD. Strictly one-hop, and it is
    //                what drives the poll set, so admitting a far-away peer here costs
    //                a receive timeout every cycle.
    //   loctable  -- what this node KNOWS, from any source and any number of hops.
    //                Nothing is polled from it; it is knowledge, not a work list.
    //
    // Sampled on a period rather than every cycle. A 600 s run with ten nodes each
    // holding a dozen neighbours is ~80k rows at every cycle, and an inspector cannot
    // show more than a few frames per second anyway. The period is stated in the file
    // so a reader knows the sampling rather than assuming continuity.
    std::ofstream vizNbrTable;
    std::ofstream vizLocTable;
    double nextTableDumpS = 0.0;
    if (vizEventsOn && vizTablePeriodS > 0.0)
    {
        std::ostringstream bfn;
        bfn << traceDir << "/nbrtable_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        vizNbrTable.open(bfn.str().c_str());
        if (vizNbrTable.is_open())
        {
            vizNbrTable << "t,nodeId,tier,peerId,lastHeardS,rxPowerDbm,snrDb,heardCount\n";
        }
        std::ostringstream lfn;
        lfn << traceDir << "/loctable_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        vizLocTable.open(lfn.str().c_str());
        if (vizLocTable.is_open())
        {
            vizLocTable << "t,nodeId,entryId,x,y,varX,varY,covXY,entryTimeS,ageS,seqNo,"
                           "hopCount,manifestId,freshness\n";
        }
    }
    if (vizEventsOn)
    {
        std::ostringstream vfn;
        vfn << traceDir << "/events_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        vizEvents.open(vfn.str().c_str());
        if (vizEvents.is_open())
        {
            vizEvents << "t,tier,txId,rxId,msgType,outcome,rxPowerDbm,payload\n";
        }
        std::ostringstream nfn;
        nfn << traceDir << "/nodes_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        vizNodes.open(nfn.str().c_str());
        if (vizNodes.is_open())
        {
            vizNodes << "t,nodeId,x,y\n";
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                chan[ti].SetEventLog(&vizEvents);
            }
        }
    }
    // L.1.4. Declared BEFORE macs so it is destroyed AFTER them: every gateway MAC holds
    // a pointer to a coordinator and to the log stream this owns. Null unless the flag
    // is on, and while null nothing below constructs, draws or transmits anything.
    std::unique_ptr<GrantGossip> gossip;
    if (anchorGrantGossipS > 0.0)
    {
        std::ostringstream gfn;
        gfn << traceDir << "/grant_verdicts_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        gossip.reset(new GrantGossip(anchorGrantGossipS, dt, gfn.str(), phy));
        gossip->windowS = spec.arbSlotHoldS;
    }
    // L.1.1. Declared before macs for the same reason: each relay's routing-tier MAC holds
    // a callback into it. Built below, once the relays have their ids; null unless
    // --fixRouting, and while null no callback is bound and nothing is sent or evaluated.
    std::unique_ptr<FixRouter> router;
    std::unique_ptr<FixRouter> tierRouters[CA_TBL_TIER_COUNT]; // NW-2, null without the flag
    std::deque<CaTblMac> macs; // deque: push_back must not invalidate held pointers
    std::deque<BeaconDriver> beacons;
    std::map<uint16_t, Vector> anchorPosById;

    for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
    {
        chan[ti].SetPhyProvider(&provider);

        CaTblMacConfig mc;
        mc.queryProxy = queryProxy;
        mc.tableFuseCI = tableFuseCI;
        mc.digestSummary = digestSummary;
        mc.queryStalenessBoundS = queryStalenessBoundS;
        mc.reversePath = reversePath;
        mc.reversePathHop = spec.reversePathHop;
        mc.queryV2 = spec.queryV2;
        mc.silenceStopsRelay = spec.silenceStopsRelay; // N.1
        mc.halfDuplex = static_cast<uint8_t>(spec.halfDuplex); // N.6
        mc.arbitrationV2 = spec.arbitrationV2;
        mc.queryTriggeredEscalation = spec.queryTriggeredEscalation;
        mc.nodeId = static_cast<uint16_t>(nodeId);
        mc.identityClass = CA_TBL_ID_TARGET;
        // C3. The measured tag is manifest 1; peers are 2, 3, ... Nothing read
        // this field before C3, so setting it explicitly changes no behaviour --
        // and the null control confirms that rather than assuming it.
        mc.targetManifestId = 1;
        mc.sendHandshakeFrames = sendHandshakeFrames;
        mc.handshakeFixes = spec.handshakeFixes;
        mc.discoveryOneHop = (spec.meshLinks == "all"); // NW-0
        mc.isAccessPoint = false; // field node: WPC Table-II 2.5% cap
        mc.sf = phy[ti].sf;
        mc.bandwidthHz = phy[ti].bandwidthHz;
        mc.txPowerDbm = phy[ti].txPowerDbm;
        mc.neighbourTtlS = beaconPeriodS[ti] * nbrTtlMult;
        macs.emplace_back();
        nodeMac[ti] = &macs.back();
        nodeMac[ti]->Configure(mc);
        nodeMac[ti]->SetRng(macRng);
        chan[ti].Register(nodeMac[ti]);
    }

    for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
    {
        const CaTblTier tier = static_cast<CaTblTier>(ti);
        for (size_t a = 0; a < dep.anchors[ti].size(); ++a)
        {
            const AnchorDef& ad = dep.anchors[ti][a];
            const uint16_t aid = static_cast<uint16_t>(ad.id);
            provider.RegisterAnchor(tier, aid, ad.pos);
            anchorPosById[aid] = ad.pos;

            CaTblMacConfig mc;
            mc.queryProxy = queryProxy;
            mc.tableFuseCI = tableFuseCI;
            mc.digestSummary = digestSummary;
            mc.queryStalenessBoundS = queryStalenessBoundS;
            mc.reversePath = reversePath;
            mc.reversePathHop = spec.reversePathHop;
            mc.queryV2 = spec.queryV2;
            mc.silenceStopsRelay = spec.silenceStopsRelay; // N.1
            mc.halfDuplex = static_cast<uint8_t>(spec.halfDuplex); // N.6
            mc.arbitrationV2 = spec.arbitrationV2;
            mc.queryTriggeredEscalation = spec.queryTriggeredEscalation;
            mc.gossipQueue = spec.gossipQueue;
            mc.nodeId = aid;
            mc.identityClass = CA_TBL_ID_ANCHOR;
            // C4. Only anchors arbitrate -- a target has no slot to award.
            mc.uwbArbitration = uwbArbitration;
            mc.arbSlotHoldS = spec.arbSlotHoldS;
            mc.sendHandshakeFrames = sendHandshakeFrames;
            mc.handshakeFixes = spec.handshakeFixes;
            mc.discoveryOneHop = (spec.meshLinks == "all"); // NW-0
            // Anchors are network access points under WPC G.S.R. 853(E) Table-II,
            // so their 865-868 MHz cap is 10%, not 2.5%. At the default 10 s
            // tier-1 beacon period one gateway occupies 51.5 ms / 10 s = 0.51%,
            // comfortably compliant; nine of them occupy 4.6% of the CHANNEL,
            // which is a contention statement, not a regulatory one.
            mc.isAccessPoint = true;
            mc.sf = phy[ti].sf;
            mc.bandwidthHz = phy[ti].bandwidthHz;
            mc.txPowerDbm = phy[ti].txPowerDbm;
            mc.neighbourTtlS = beaconPeriodS[ti] * nbrTtlMult;
            macs.emplace_back();
            CaTblMac* am = &macs.back();
            am->Configure(mc);
            am->SetRng(macRng);
            // A gateway is surveyed and a finder has GPS, so each can state its own
            // position with small uncertainty. This is what the lost node corroborates
            // AGAINST -- without it there are no witnesses, only its own opinion.
            am->PublishSelfEntry(ad.pos.x, ad.pos.y, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0);
            chan[ti].Register(am);
            if (gossip)
            {
                gossip->AttachAnchor(aid, tier, am, provider);
            }

            BeaconDriver bd;
            bd.mac = am;
            bd.tier = tier;
            bd.periodS = beaconPeriodS[ti];
            bd.jitterMaxS = beaconPeriodS[ti] * 0.25;
            bd.untilS = durationS;
            bd.rng = macRng;
            beacons.push_back(bd);
        }
    }
    // ---- C2-peer: additional targets, as protocol peers -------------------
    //
    // Constructed only when nTags > 1. At nTags == 1 this block does nothing at all:
    // no Tourist, no MAC, no beacon, and critically NO DRAW from macRng -- which is
    // what makes the default byte-identical rather than merely close.
    //
    // A peer is a real MAC on the real channel with a real trajectory. It beacons, so
    // anchors discover it; it contends, so it costs the medium what it costs; and it
    // carries a DISTINCT targetManifestId, which is what an identity-scoped query
    // discriminates on. By default it has no CaTblPolicy and no CaTblEnergyModel: it is
    // an addressing and contention peer, NOT a second measured subject, and its energy
    // is deliberately not accounted. Anything that reports peer energy is wrong.
    // EXCEPT under --relayedRanging (C5, below): then each peer is also a RELAY with a
    // configured CaTblPolicy and an energy model charged for its own ranging -- a
    // fixed-tier measured node that never escalates, not a full subject (this comment
    // omitted that until 2026-09-30; making peers full subjects is
    // docs/SPEC_CONFORMANCE_PLAN.md MS-1).
    std::deque<Tourist> peerWalkers;
    std::vector<uint32_t> peerIds;
    // One MAC per tier per peer, recorded AT CONSTRUCTION. CaTblMac exposes no
    // GetTier(), so a later search cannot tell a peer's three MACs apart -- and
    // a search by node id alone matched the measured tag first, which is how
    // every synthetic request ended up attributed to target 1.
    std::vector<std::vector<CaTblMac*>> peerMacs;
    for (uint32_t pt = 1; pt < nTags; ++pt)
    {
        const uint32_t pid = nodeId + pt;
        // Own stream, so the peer's motion does not shift the measured tag's walker
        // draws. The offset is arbitrary and large; it only has to not collide with
        // seed*7919+13 or seed*7919+104729.
        Ptr<UniformRandomVariable> prng = CreateObject<UniformRandomVariable>();
        prng->SetStream(static_cast<int64_t>(seed) * 7919 + 500003 + pt);
        peerWalkers.emplace_back(dep.facilities, walkSpeedMps, dwellS,
                                 dep.boxHalfM, dep.dwellRadiusM, prng);
        const Vector ppos = peerWalkers.back().Position();
        engine868.RegisterNode(pid, ppos);
        engine2p4.RegisterNode(pid, ppos);
        engineUwb.RegisterNode(pid, ppos);
        // The engines were always told about the peer; the PROVIDER was not, and
        // the provider is what decides whether a frame has an evaluable link.
        // Without this line the three calls above are inert for delivery.
        provider.RegisterPeerNode(static_cast<uint16_t>(pid), ppos);
        peerIds.push_back(pid);
        peerMacs.push_back(std::vector<CaTblMac*>());

        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            CaTblMacConfig pmc;
            pmc.queryProxy = queryProxy;
            pmc.tableFuseCI = tableFuseCI;
            pmc.digestSummary = digestSummary;
            pmc.queryStalenessBoundS = queryStalenessBoundS;
            pmc.reversePath = reversePath;
            pmc.reversePathHop = spec.reversePathHop;
            pmc.queryV2 = spec.queryV2;
            pmc.silenceStopsRelay = spec.silenceStopsRelay; // N.1
            pmc.halfDuplex = static_cast<uint8_t>(spec.halfDuplex); // N.6
            pmc.arbitrationV2 = spec.arbitrationV2;
            pmc.queryTriggeredEscalation = spec.queryTriggeredEscalation;
            pmc.nodeId = static_cast<uint16_t>(pid);
            pmc.identityClass = CA_TBL_ID_TARGET;
            // THE POINT OF THE PEER. A distinct manifest id is what makes an
            // identity-scoped query answerable: without it every target matches
            // every query and C3 has nothing to discriminate.
            pmc.targetManifestId = static_cast<uint8_t>(1 + pt);
            pmc.sendHandshakeFrames = sendHandshakeFrames;
            pmc.handshakeFixes = spec.handshakeFixes;
            pmc.discoveryOneHop = (spec.meshLinks == "all"); // NW-0
            pmc.isAccessPoint = false; // field node: WPC Table-II 2.5% cap
            pmc.sf = phy[ti].sf;
            pmc.bandwidthHz = phy[ti].bandwidthHz;
            pmc.txPowerDbm = phy[ti].txPowerDbm;
            pmc.neighbourTtlS = beaconPeriodS[ti] * nbrTtlMult;
            macs.emplace_back();
            CaTblMac* pm = &macs.back();
            peerMacs.back().push_back(pm);
            pm->Configure(pmc);
            pm->SetRng(macRng);
            chan[ti].Register(pm);

            BeaconDriver pbd;
            pbd.mac = pm;
            pbd.tier = static_cast<CaTblTier>(ti);
            pbd.periodS = beaconPeriodS[ti];
            pbd.jitterMaxS = beaconPeriodS[ti] * 0.25;
            pbd.untilS = durationS;
            pbd.rng = macRng;
            beacons.push_back(pbd);
        }
    }

    // ---- C3: identity-scoped queries from the anchors ----------------------
    //
    // Off unless identityQueryPeriodS > 0, and then ONE driver per tier on the first
    // anchor of that tier -- not every anchor. The question being measured is whether
    // the RIGHT target answers, not how much query traffic the medium carries, and
    // every anchor querying would confound the two.
    // The deque MUST live at RunArm scope. QueryDriver objects are scheduled into
    // the simulator and Fire() runs long after this block; confining them to the
    // branch destroys them while the scheduler still holds their addresses, which is
    // a use-after-free that would most likely have appeared to work.
    //
    // The cost is a trace divergence with queries OFF: a deque local to RunArm
    // changes the stack frame even when unused, and under -march=native that alone
    // changes floating-point codegen. Measured: 1942 trace lines move by 1 ulp on
    // estXhp, estYhp, meanSigmaM, meanSigmaRawM and one mahalanobis, while
    // arm_summary.csv stays byte-identical. Correctness first.
    std::deque<QueryDriver> queries;
    // Lives at RunArm scope for the same reason queries does: the scheduler
    // holds these addresses long after the block that creates them.
    std::deque<PeerRequestDriver> peerReqs;
    if (identityQueryPeriodS > 0.0)
    {
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            for (size_t m = 0; m < macs.size(); ++m)
            {
                if (macs[m].GetIdentityClass() != CA_TBL_ID_ANCHOR)
                {
                    continue;
                }
                // NW-3. per-tier: the first anchor MAC ON THIS TIER'S CHANNEL, the radio
                // the query claims to use. Otherwise the first anchor MAC of all -- gateway
                // 101's 868 MHz radio for every tier -- which is what every C3 result used:
                // a "2.4 GHz" query sent with 868 MHz's SF and bandwidth, charged to its
                // duty budget and heard by 868 MHz radios.
                if (spec.queryTierBinding == "per-tier" && macs[m].GetChannel() != &chan[ti])
                {
                    continue;
                }
                QueryDriver qd;
                qd.mac = &macs[m];
                qd.tier = static_cast<CaTblTier>(ti);
                qd.periodS = identityQueryPeriodS;
                qd.jitterMaxS = identityQueryPeriodS * 0.25;
                qd.untilS = durationS;
                qd.manifestId = static_cast<uint8_t>(identityQueryTarget);
                qd.rng = macRng;
                if (spec.queryV2)
                {
                    qd.v2 = true;
                    qd.manifests = spec.identityQueryManifests.empty()
                                       ? std::vector<uint8_t>(1, qd.manifestId)
                                       : spec.identityQueryManifests;
                    qd.priority = spec.identityQueryUrgent ? 1 : 0;
                }
                queries.push_back(qd);
                break;
            }
        }
    }

    // C4. Peer UWB requests, so the arbiter has competing claims to resolve. Only
    // when arbitration is on AND a peer exists -- otherwise there is nothing to
    // arbitrate and this would just be extra traffic.
    if (uwbArbitration && nTags > 1 && spec.measuredSubjects <= 1)
    {
        for (size_t pi = 0; pi < peerMacs.size(); ++pi)
        {
            for (size_t ti = 0; ti < peerMacs[pi].size(); ++ti)
            {
                {
                    PeerRequestDriver rd;
                    rd.mac = peerMacs[pi][ti];
                    rd.tier = static_cast<CaTblTier>(ti);
                    rd.periodS = 30.0;
                    rd.jitterMaxS = 7.5;
                    rd.untilS = durationS;
                    // STATED CONSTANTS, not measurements. A peer has no filter and no
                    // energy account, so there is nothing to derive these from. Chosen
                    // mid-range so the peer neither always wins nor always loses --
                    // an arbiter that always resolves the same way is not exercised
                    // either.
                    rd.degradation = 128;
                    rd.battery = 128;
                    rd.rng = macRng;
                    peerReqs.push_back(rd);
                }
            }
        }
    }

    // C5. A RELAY MUST BE A MEASURED SUBJECT, or it has nothing to relay.
    //
    // A relayed measurement treats the relay as a PSEUDO-ANCHOR: the range is used
    // against the relay's own position ESTIMATE, with the variance inflated by
    // u^T P_B u. Both halves need the relay to run a filter. A peer with no
    // CaTblPolicy has no x_hat and no P_B, so there is nothing to put on the wire --
    // which is what blocked C5, and it was the link budget that was never the issue.
    //
    // The relay is a FIXED-TIER measured node: it ranges, filters and is charged, but
    // does not escalate. Stated limitation, not an approximation of one -- escalation
    // for peer tags is the remaining half of C2, and a relay that escalated would earn
    // its P_B on a different tier between one report and the next, which the error
    // model does not cover.
    if (relayedRanging)
    {
        for (uint32_t pt = 1; pt < nTags && pt < tags.size(); ++pt)
        {
            const size_t pi = pt - 1;
            if (pi >= peerWalkers.size())
            {
                break;
            }
            tags[pt].nodeId = peerIds[pi];
            tags[pt].walker = &peerWalkers[pi];
            tags[pt].pos = peerWalkers[pi].Position();
            // The SAME configuration as the measured tag. A relay filtered under
            // different settings would report a P_B not comparable with the covariance
            // the consumer's own filter carries, and admission test A3 compares
            // exactly those two quantities.
            tags[pt].policy.Configure(pcfg);
            tags[pt].policy.Initialize(tags[pt].pos.x, tags[pt].pos.y, 100.0, 5.0);
        }
    }
    // L.1.2. Null unless --relayChain, and while null the relay loop below is unchanged.
    std::unique_ptr<RelayChain> chain;
    std::unique_ptr<LegTerms> legTerms; // NW-1 step 3: null unless --hopVarianceTerms
    if (relayChain)
    {
        std::ostringstream cfn;
        cfn << traceDir << "/relay_chain_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        chain.reset(new RelayChain(tags.size(), cfn.str(), spec.hopVarianceTerms));
        chain->hopProvenance = spec.hopProvenance;
        if (spec.hopVarianceTerms)
        {
            legTerms.reset(new LegTerms(seed, nlosScoreThreshold, pcfg.nlosSigmaInflation,
                                        engine2p4.GetConfig(), engineUwb.GetConfig()));
            chain->legTerms = legTerms.get();
        }
    }
    // N.14. Null unless --fuseFloorActive.
    std::unique_ptr<FuseFloorLog> fuseFloorLog;
    if (spec.fuseFloorActive)
    {
        std::ostringstream ffn;
        ffn << traceDir << "/fusefloor_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        fuseFloorLog.reset(new FuseFloorLog(ffn.str()));
    }
    // C5_INSTANT. Null unless --relaySlots, and while null every relay exchange is at the
    // cycle's instant and charged at its time, exactly as before.
    std::unique_ptr<RelayRound> relayRound;
    if (relaySlots)
    {
        relayRound.reset(new RelayRound(static_cast<uint32_t>(tags.size()), relayChain,
                                        FixRouter::kSlotS));
    }
    if (fixRouting)
    {
        const CaTblTier rrt = static_cast<CaTblTier>(
            std::min<uint32_t>(CA_TBL_TIER_COUNT - 1, relayTier));
        std::ostringstream rfn;
        rfn << traceDir << "/fix_routing_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        CaTblRoutingWeights rw; // NW-2
        rw.alphaNlos = spec.routeAlpha;
        rw.betaRssiVar = spec.routeBeta;
        rw.gammaJitter = spec.routeGamma;
        rw.slack = static_cast<uint8_t>(spec.routeSlack);
        const bool rwSet = spec.routeAlpha != 1.0 || spec.routeBeta != 1.0 ||
                           spec.routeGamma != 1.0 || spec.routeSlack != 1;
        if (!spec.fixRoutingSubjects)
        {
            router.reset(new FixRouter(tags, nTags, rrt, fixRoutingConfidence,
                                       dep.anchors[rrt], beaconPeriodS[rrt], dt, nlosSource,
                                       phy, rfn.str(), rw, false, rwSet));
            for (size_t pi = 0; pi < peerMacs.size(); ++pi)
            {
                if (static_cast<size_t>(rrt) < peerMacs[pi].size() &&
                    peerMacs[pi][static_cast<size_t>(rrt)] != nullptr)
                {
                    peerMacs[pi][static_cast<size_t>(rrt)]->SetRxCallback(
                        MakeCallback(&FixRouter::OnRx, router.get()));
                }
            }
        }
        else
        {
            // NW-2. One router per tier, the measured tag included. Each cycle a node takes
            // part only in the router of the tier it ranged on, so a routed frame is on the
            // one tier that node transmits on (sect 6); a relay that is not a subject stays
            // on --relayTier. Each tier's radio of every node feeds that tier's router.
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                const CaTblTier tt = static_cast<CaTblTier>(ti);
                std::ostringstream tfn;
                tfn << traceDir << "/fix_routing_" << CaTblPolicyModeName(mode) << "_seed"
                    << seed << "_tier" << ti << ".csv";
                tierRouters[ti].reset(new FixRouter(tags, nTags, tt, fixRoutingConfidence,
                                                    dep.anchors[tt], beaconPeriodS[tt], dt,
                                                    nlosSource, phy, tfn.str(), rw, true, true));
                tierRouters[ti]->tag0Mac = tags[0].nodeMac[ti];
                tags[0].nodeMac[ti]->SetRxCallback(
                    MakeCallback(&FixRouter::OnRx, tierRouters[ti].get()));
                for (size_t pi = 0; pi < peerMacs.size(); ++pi)
                {
                    if (static_cast<size_t>(ti) < peerMacs[pi].size() &&
                        peerMacs[pi][static_cast<size_t>(ti)] != nullptr)
                    {
                        peerMacs[pi][static_cast<size_t>(ti)]->SetRxCallback(
                            MakeCallback(&FixRouter::OnRx, tierRouters[ti].get()));
                    }
                }
            }
        }
    }

    // ---- FINDERS: moving nodes carrying all three radios ---------------------
    //
    // Registered on every tier exactly like a gateway, because that is what they are --
    // a gateway whose position changes. They beacon, so the lost node discovers them
    // through the ordinary neighbour table and ranges to them through the ordinary
    // poll set; no ranging or energy code knows they are special.
    //
    // isAccessPoint is FALSE: a finder is a person carrying a radio, not installed
    // infrastructure, so it takes the 2.5% WPC Table-II field cap and not the 10%
    // access-point cap. Getting that backwards would hand searchers four times the
    // airtime budget they are entitled to.
    std::deque<Tourist> finderWalkers;
    std::vector<uint32_t> finderIds;
    std::vector<Vector> finderPos;
    std::vector<CaTblMac*> finderMac; // tier-0 MAC per finder, for its table
    std::vector<std::vector<CaTblMac*>> finderMacAll; // [finder][tier], to republish
    // WHAT A FINDER BELIEVES ABOUT ITSELF, as distinct from where it is.
    //
    // A searcher is not GPS-denied -- that is the casualty's problem, and the whole
    // premise of the deployment -- but it is not surveyed infrastructure either. It
    // knows where it is to within its receiver's error, and every position it
    // publishes or navigates by carries that error. Modelled as a CONSTANT per-finder
    // offset rather than fresh noise each cycle, because GNSS error over a 600 s walk
    // is dominated by slowly varying terms (geometry, multipath, ionosphere) and
    // white noise would wash out under averaging and understate the effect.
    //
    // Drawn from its own stream, so enabling this does not displace the shared RNG
    // stream and move every unrelated fading realisation with it (TECH_DEBT sect 1.1).
    std::vector<Vector> finderGpsOffset;
    const double finderPosVar =
        (finderPosSigmaM > 0.0) ? (finderPosSigmaM * finderPosSigmaM) : 4.0;
    for (uint32_t fi = 0; fi < nFinders; ++fi)
    {
        const uint32_t fid = 501 + fi;
        Ptr<UniformRandomVariable> frng = CreateObject<UniformRandomVariable>();
        frng->SetStream(static_cast<int64_t>(seed) * 7919 + 900007 + fi);
        // The SEARCH PARTY's speed, not the subject's. They are separate because
        // a stationary casualty does not imply stationary searchers -- and while
        // they shared one flag, --speed=0 pinned all four finders to the spot and
        // every still cell measured a paralysed search.
        finderWalkers.emplace_back(dep.facilities, finderSpeedMps, dwellS,
                                   dep.boxHalfM, dep.dwellRadiusM, frng);
        finderWalkers.back().SetMaxTurnRate(maxTurnRateRadS);
        // A search party sets out FROM somewhere. Without this the finders spawn in
        // the middle of the search area, which hands them the answer at t = 0 and
        // flatters the finder-hop mechanism the base-camp scenario exists to test.
        // The draw above is consumed either way, so the RNG stream does not move.
        if (dep.hasBaseCamp)
        {
            finderWalkers.back().SetPosition(dep.baseCampX, dep.baseCampY);
        }
        const Vector fpos = finderWalkers.back().Position();
        finderIds.push_back(fid);
        finderPos.push_back(fpos);
        Vector goff(0.0, 0.0, 0.0);
        if (finderPosSigmaM > 0.0)
        {
            Ptr<NormalRandomVariable> grng = CreateObject<NormalRandomVariable>();
            grng->SetAttribute("Mean", DoubleValue(0.0));
            grng->SetAttribute("Variance",
                               DoubleValue(finderPosSigmaM * finderPosSigmaM));
            grng->SetStream(static_cast<int64_t>(seed) * 7919 + 910007 + fi);
            goff.x = grng->GetValue();
            goff.y = grng->GetValue();
        }
        finderGpsOffset.push_back(goff);
        finderMacAll.push_back(std::vector<CaTblMac*>(CA_TBL_TIER_COUNT, nullptr));

        engine868.RegisterNode(fid, fpos);
        engine2p4.RegisterNode(fid, fpos);
        engineUwb.RegisterNode(fid, fpos);

        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            const CaTblTier tier = static_cast<CaTblTier>(ti);
            provider.RegisterAnchor(tier, static_cast<uint16_t>(fid), fpos);
            anchorPosById[static_cast<uint16_t>(fid)] = fpos;

            CaTblMacConfig fc;
            fc.queryProxy = queryProxy;
            fc.queryStalenessBoundS = queryStalenessBoundS;
            fc.reversePath = reversePath;
            fc.reversePathHop = spec.reversePathHop;
            fc.queryV2 = spec.queryV2;
            fc.silenceStopsRelay = spec.silenceStopsRelay; // N.1
            fc.halfDuplex = static_cast<uint8_t>(spec.halfDuplex); // N.6
            fc.arbitrationV2 = spec.arbitrationV2;
            fc.queryTriggeredEscalation = spec.queryTriggeredEscalation;
            fc.gossipQueue = spec.gossipQueue;
            fc.nodeId = static_cast<uint16_t>(fid);
            fc.identityClass = CA_TBL_ID_RESCUER;
            fc.isAccessPoint = false;
            fc.uwbArbitration = uwbArbitration;
            fc.arbSlotHoldS = spec.arbSlotHoldS;
            fc.sendHandshakeFrames = sendHandshakeFrames;
            fc.handshakeFixes = spec.handshakeFixes;
            fc.discoveryOneHop = (spec.meshLinks == "all"); // NW-0
            fc.sf = phy[ti].sf;
            fc.bandwidthHz = phy[ti].bandwidthHz;
            fc.txPowerDbm = phy[ti].txPowerDbm;
            fc.neighbourTtlS = beaconPeriodS[ti] * nbrTtlMult;
            macs.emplace_back();
            CaTblMac* fm = &macs.back();
            fm->Configure(fc);
            fm->SetRng(macRng);
            // What it BELIEVES, with the variance that belief actually carries.
            // Publishing the true position under a non-zero variance would claim an
            // uncertainty the reported number does not have, which is the silent
            // perfect-knowledge assumption this flag exists to remove.
            fm->PublishSelfEntry(fpos.x + goff.x, fpos.y + goff.y, finderPosVar,
                                 finderPosVar, 0.0, 0.0, 0.0, 0.0);
            if (gossip && spec.gossipQueue && spec.meshLinks == "all")
            {
                gossip->AttachFinder(static_cast<uint16_t>(fid), tier, fm); // NW-6
            }
            chan[ti].Register(fm);
            finderMacAll.back()[ti] = fm;
            if (ti == 0)
            {
                finderMac.push_back(fm);
            }

            BeaconDriver fb;
            fb.mac = fm;
            fb.tier = tier;
            fb.periodS = beaconPeriodS[ti];
            fb.jitterMaxS = beaconPeriodS[ti] * 0.25;
            fb.untilS = durationS;
            fb.rng = macRng;
            beacons.push_back(fb);
        }
    }

    // Application sect 4. A rescuer opens the search before anything runs, which is what
    // makes time-to-first-fix meaningful: the clock starts when the request was made,
    // not when the system happened to succeed.
    if (sessionLayer && spec.sessionOverMesh)
    {
        // TA-1a. The base station (requester 0) searches for what its queries ask: the
        // --identityQueryManifests list (a batch) or --identityQueryTarget alone.
        sessions.SetStalenessBound(spec.sessionStalenessS);
        sessions.SetSessionTimeout(spec.sessionTimeoutS);
        sessions.SetPreciseOnGrant(spec.sessionPreciseOnGrant); // TA-3
        const std::vector<uint8_t> targets =
            spec.identityQueryManifests.empty()
                ? std::vector<uint8_t>(1, static_cast<uint8_t>(identityQueryTarget))
                : spec.identityQueryManifests;
        sessionId = (targets.size() > 1)
                        ? sessions.BatchSearch(targets, 0u, CA_TBL_PRIORITY_ROUTINE, 0.0)
                        : sessions.SearchAndLocalize(targets[0], 0u, CA_TBL_PRIORITY_ROUTINE,
                                                     0.0);
    }
    else if (sessionLayer)
    {
        // Manifest id 1 is the measured tag's, set at mc.targetManifestId above. The
        // requester id is the base station, which is not a simulated node -- nothing is
        // routed to it here, so any non-colliding id serves.
        if (batchSearch)
        {
            // Application sect 5. ONE session over N targets -- one flood, one
            // reverse-path cache, one staleness clock, and every escalation request
            // landing in the SAME arbiter queue. That last part is the point: it is the
            // only configuration in which U(r) scoring does observable work, so a
            // single-target run proves nothing about arbitration.
            std::vector<uint8_t> targets;
            for (uint32_t k = 0; k < nTags; ++k)
            {
                targets.push_back(static_cast<uint8_t>(1 + k));
            }
            sessionId = sessions.BatchSearch(targets, 1u, CA_TBL_PRIORITY_ROUTINE, 0.0);
        }
        else
        {
            sessionId = sessions.SearchAndLocalize(1u, 1u, CA_TBL_PRIORITY_ROUTINE, 0.0);
        }
    }

    // Scheduled only after every push_back, so the addresses are final.
    for (size_t r = 0; r < peerReqs.size(); ++r)
    {
        Simulator::Schedule(Seconds(macRng->GetValue(0.0, peerReqs[r].periodS)),
                            &PeerRequestDriver::Fire,
                            &peerReqs[r]);
    }
    for (size_t q = 0; q < queries.size(); ++q)
    {
        Simulator::Schedule(Seconds(macRng->GetValue(0.0, queries[q].periodS)),
                            &QueryDriver::Fire,
                            &queries[q]);
    }
    for (size_t b = 0; b < beacons.size(); ++b)
    {
        Simulator::Schedule(Seconds(macRng->GetValue(0.0, beacons[b].periodS)),
                            &BeaconDriver::Fire,
                            &beacons[b]);
    }

    // Airtime of one DISCOVERY beacon per tier, for the listen charge below.
    for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
    {
        const uint32_t payload =
            CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(CA_TBL_MSG_DISCOVERY, 1);
        beaconToaS[ti] = (ti == static_cast<int>(CA_TBL_TIER_UWB))
                             ? CaTblEnergyModel::UwbFrameTimeS(payload)
                             : CaTblEnergyModel::LoRaTimeOnAirS(phy[ti].sf,
                                                                phy[ti].bandwidthHz,
                                                                payload,
                                                                1);
    }

    // Last simulated time at which each tier's neighbour table was non-empty.
    // This SURVIVES ClearNeighbours(), and that is the entire point: the live table
    // answers "can I range against this tier right now", which is false for any
    // sleeping tier by construction. Multi-step demotion needs the different
    // question "was anything ever there", and reading the live table for it would
    // make every sleeping tier look absent and collapse every demotion to 868.
    // -1.0 means "never seen populated".
    // Floor-ranging-on-demand bookkeeping. floorHealthyStreak counts consecutive
    // cycles on which the ACTIVE tier delivered a solvable set; cycleIndex drives
    // the optional heartbeat. Both are inert unless --floorOnDemand is set.
    // Deterministic fault-window start: seed-derived, mid-run, so the window never
    // straddles the EKF's initial convergence. Same for every arm at a given seed.
    const double biasStartS =
        durationS * (0.25 + 0.5 * static_cast<double>(seed % 7) / 7.0);
    // Escalation handshakes suppressed by the tier-1 duty-cycle governor, and the
    // total time the node had to wait for the budget to refill. The regulation
    // delaying a handoff is a cyber-physical coupling, not an accounting detail:
    // it converts a legal constraint into position error.
    // A3. Handshakes that did NOT slip to the next cycle because the tier-1
    // budget refilled before this one ended. Deliberately NOT an arm_summary
    // column: adding one changes the CSV header for every run and breaks
    // byte-identity against baseline_v7. Printed instead.
    //! Escalations rolled back because the handshake could not be sent. Distinct
    //! from handshakeDeferrals, which also counts deferred DEMOTION handshakes --
    //! those defer the charge but never the transition.
    //! Cycles where ranging on the CAPPED tier was pre-empted so a pending
    //! handshake could use the duty budget. The cost is this cycle's tier-1
    //! measurements.
    //! Set when an escalation is deferred this cycle; the NEXT cycle will
    //! pre-empt ranging to send the handshake (if arbitrateDutyBudget is on).

    // Divergence instrument. ca_tbl_full and ca_tbl_no_integrity are identical
    // through t=5.0 and differ from t=7.5 onward, with suspect=0 on every cycle of
    // both -- so the integrity check cannot be the cause, and the policy is
    // provably mode-independent when it does not fire. The two arms polled the SAME
    // six anchors at t=7.5 and got 6 decodes versus 2.
    //
    // Two arms reading one sampler diverge only if one consumes a draw the other
    // does not. These counters say whether that is what happened. If the cumulative
    // counts are equal at the cycle where nAnchors first differs, the sampler is
    // NOT the mechanism and the fault is per-link cached state inside the engines.
    // If they differ, the extra consumer is between here and there.

    std::ofstream trace;
    if (writeTrace)
    {
        std::ostringstream fn;
        fn << traceDir << "/trace_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        trace.open(fn.str().c_str());
        // CHECKED, and aborts rather than degrading. An unchecked failure here makes
        // every later `trace << ...` a silent no-op, so the run completes and emits a
        // summary while the per-cycle evidence is absent -- and the verifier's
        // staleness gates, the null-ablation diff and every divergence analysis read
        // exactly these files. A partially written trace set is worse than none,
        // which is why this exits instead of continuing with the remaining arms.
        if (!trace.is_open())
        {
            std::cerr << "FATAL: cannot open " << fn.str()
                      << " for writing -- does --outdir exist?\n";
            std::exit(1);
        }
        // fixTier / usedFloor expose the concurrent-tier fallback. activeTier is what
        // the policy selected; fixTier is which radio actually produced the fix. On a
        // hybrid arm they differ whenever the escalated tier returned fewer than three
        // ranges, and the fraction of steps with usedFloor=1 is the measure of how
        // often escalation was attempted into no coverage.
        trace << "t,arm,seed,activeTier,nextTier,fixTier,usedFloor,trueX,trueY,estX,estY,errM,"
              << "pcs,gdop,traceSigma,mahalanobis,nAnchors,nNlos,"
              // nNlos is the OPERATIVE count -- the one that drove R, the LOS
              // fraction and the all-LOS gate, under whichever --nlosSource was
              // selected. The next two are the underlying populations, emitted
              // unconditionally so the legacy flag's composition is a
              // measurement rather than an argument: under legacy,
              // nNlos == |propagation UNION hidden|, and the gap between
              // nNlosPropagation and nHiddenChannel is the entire defect.
              << "nNlosPropagation,nHiddenChannel,hadFix,"
              << "escalated,deescalated,suspect,rejected,"
              << "vis868,vis2p4,visUwb,shouldUwb,cumJ,"
              // nbr* is the size of the one-hop neighbour table on each tier at the
              // moment the poll was issued. Read it against vis*: vis is geometric
              // ground truth ("could this tier have solved here"), nbr is what the
              // node had actually DISCOVERED. The gap between them is the cost of
              // discovery, and it is a quantity the simulation could not previously
              // express at all -- the poll addressed every anchor unconditionally,
              // so the node behaved as if it had perfect prior knowledge of a
              // deployment map nobody gave it.
              << "nbr868,nbr2p4,nbrUwb,polled,blind,"
              // Divergence instrument -- see the counters in RunArm(). directCalls
              // is cumulative ProcessTransmission() calls from the ranging poll,
              // macEvals is cumulative calls from the MAC/channel layer. estXhp and
              // estYhp are the same numbers as estX/estY at 12 significant figures,
              // because estX prints at 4 decimals and a divergence of 1e-5 m is
              // invisible there -- which is precisely how a divergence that began
              // earlier could look like one that began at t=7.5.
              << "directCalls,macEvals,estXhp,estYhp,meanSigmaM,meanSigmaRawM,"
              // sigmaGeoM is the exact sqrt(trace((J^T R^-1 J)^-1)), PCS v2's
              // second input. Negative under the legacy form, where it is never
              // computed. Emitted so §0.4a's per-term variance decomposition can
              // be done from the trace without re-deriving the solve.
              << "sigmaGeoM,"
              // P1 diagnostics. floorRanged separates "the floor was skipped"
              // from "the floor ran and found nothing"; nRangesEkf is how many
              // ranges actually entered the EKF this cycle. Together they make
              // the coverage channel and the accuracy channel separable in the
              // data rather than argued about afterwards -- see
              // PREDICTION_P1_v2.md falsifier 4.
              << "floorRanged,nRangesEkf\n";
    }

    // C4. The two U(r) terms this tag contributes to an anchor's arbitration.
    // Held rather than recomputed at the handshake site, so the PCS history they
    // derive from is the one the policy just used.
    //
    // C2: PER TAG, and that is not bookkeeping. With a second MEASURED target both
    // tags issue their own ESC_REQUEST and the anchor arbitrates between them. A
    // single shared deque would score both requests off whichever tag wrote to it
    // last, which would look exactly like arbitration and would not be one.
    // The remaining per-cycle counters moved into CaTblTagState with everything else.
    // C5 admission tallies, for stdout. Counted per verdict rather than as a single
    // reject total, because "the measurement was refused" and "it was refused for the
    // reason the model says it should be" are different claims, and only the second
    // one is evidence the admission test is doing its job.
    double relayedVarThisCycle = 0.0;
    // The reverse half of the anti-entropy exchange, measured rather than assumed.
    // A mechanism that depends on a node HOLDING an entry has to be checked against
    // whether the entry arrived -- that is the class of error that made the 51 J
    // finder result an artefact, and nothing in the gate suite tests for it.
    uint64_t pseudoAnchorSubs = 0;
    uint64_t casualtyDigests = 0;
    double finderEntryAgeSum = 0.0;
    uint64_t finderEntryAgeN = 0;
    uint64_t finderEntryHeldSum = 0;
    uint64_t finderEntryCycles = 0;
    double relayedFracSum = 0.0;
    uint64_t relayedFracN = 0;
    uint64_t relayAdmitted = 0;
    uint64_t relayRejTier = 0;
    uint64_t relayRejDepth = 0;
    uint64_t relayRejGeom = 0;
    uint64_t relayRejStale = 0;
    uint64_t relayRejDup = 0;
    double relaySigmaEffSum = 0.0;
    double relaySigmaRawSum = 0.0;

    // Independent-witness state for the switching-point integrity check.
    std::vector<std::pair<uint16_t, double>> rangedThisCycle;
    // --tier1Tdoa. Null unless the flag is on, and while null no line below changes.
    std::unique_ptr<Tier1Tdoa> tdoa;
    if (tier1Tdoa)
    {
        std::ostringstream tfn;
        tfn << traceDir << "/tdoa_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        tdoa.reset(new Tier1Tdoa(seed, tdoaPpsSigmaNs, phy[CA_TBL_TIER_868],
                                 dep.anchors[CA_TBL_TIER_868], energy.GetTier1ChargeReplies(),
                                 energy.GetEdgeServer(), tfn.str()));
    }
    // --nlosClassifier. Null unless the flag is on, and while null no line below changes.
    std::unique_ptr<NlosScoring> nlosScorer;
    if (nlosClassifier)
    {
        std::ostringstream lfn;
        std::ostringstream sfn;
        lfn << traceDir << "/nlos_links_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        sfn << traceDir << "/nlos_summary_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        nlosScorer.reset(new NlosScoring(seed, nlosScoreThreshold, engine2p4.GetConfig(),
                                         engineUwb.GetConfig(), lfn.str(), sfn.str()));
    }
    // --integrityNlosFromScore (DC-8). One row per fix cycle the integrity check evaluated:
    // what its eligibility was read from, and the verdict. Only the arm that runs the check.
    std::unique_ptr<std::ofstream> integrityLog;
    if (spec.integrityNlosFromScore && mode == CA_TBL_MODE_FULL)
    {
        std::ostringstream ifn;
        ifn << traceDir << "/integrity_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        integrityLog.reset(new std::ofstream(ifn.str().c_str()));
        *integrityLog << "t,fixTier,nRanges,nNlosTag,nScored,meanScore,basis,eligible,"
                         "mahalanobis,suspect\n";
    }
    // Rival systems, scored from the same measurements. See the accumulation
    // block beside the EKF error for what each one is. Declared HERE, in RunArm,
    // because that is where they are used -- `errors` above is a member of the
    // per-tag state reached through a reference, not a local.
    std::vector<double> errorsMlat;
    std::vector<double> errorsWcl;
    std::vector<double> errorsProx;
    std::vector<double> errorsLkp;
    double blMlatX = 0.0, blMlatY = 0.0;
    double blWclX = 0.0, blWclY = 0.0;
    double blProxX = 0.0, blProxY = 0.0;
    double blLkpX = 0.0, blLkpY = 0.0;
    bool blMlatHave = false, blWclHave = false, blProxHave = false;
    uint64_t mlatSolved = 0;
    Vector prevTruePos = pos;
    // DC-10 (--entryVelocity). Each finder's last published position and time, for the
    // velocity its GNSS receiver would report, and the entries_ record of what every
    // producer published and every node holds. Untouched without the flag.
    std::vector<Vector> finderPubPrev(nFinders);
    std::vector<double> finderPubPrevT(nFinders, -1.0);
    // SY-5. Per-run state of the link-budget faults, null without them.
    struct LinkFaultState
    {
        double activeDb = 0.0; //!< drop + loss this cycle; 0 outside the window
        double skew = 1.0;     //!< the factor a tier-1 range reads long by this cycle
        double n = 0.0;
        uint64_t rangingLinks = 0;
        uint64_t skewedLinks = 0;
        double windowStartS = 0.0;
        double windowEndS = 0.0;
    };
    std::unique_ptr<LinkFaultState> linkFault;
    if (spec.injectTxPowerDropDb > 0.0 || spec.injectAntennaLossDb > 0.0)
    {
        linkFault.reset(new LinkFaultState());
        linkFault->n = cfg868.propagation.pathLossExponent.mean;
    }
    // SY-1. Per-run tally of --edgeUplinkRanges, null without it: frames, bytes and the
    // joules they were billed, per tier.
    struct EdgeUplinkTally
    {
        uint64_t frames[CA_TBL_TIER_COUNT] = {0, 0, 0};
        uint64_t records[CA_TBL_TIER_COUNT] = {0, 0, 0};
        double airS[CA_TBL_TIER_COUNT] = {0.0, 0.0, 0.0};
    };
    std::unique_ptr<EdgeUplinkTally> edgeUp;
    if (spec.edgeUplinkRanges)
    {
        edgeUp.reset(new EdgeUplinkTally());
    }
    // N.3 / row 55. --subjectEdgeUplink: the same tally per subject (index = tag), null without.
    std::unique_ptr<std::vector<EdgeUplinkTally>> edgeUpS;
    if (spec.subjectEdgeUplink)
    {
        edgeUpS.reset(new std::vector<EdgeUplinkTally>(tags.size()));
    }
    // DC-1. Per-run state of --imuPredict, null without it.
    struct ImuPredictState
    {
        CaTblImuDeadReckoner rk;
        bool havePrev = false;
        double prevX = 0.0;
        double prevY = 0.0;
        double dx = 0.0;
        double dy = 0.0;
        double ux = 0.0;
        double uy = 0.0;
        double qu[3] = {0.0, 0.0, 0.0};
        uint64_t cycles = 0;
        uint64_t resets = 0;
        std::ofstream log;
    };
    std::unique_ptr<ImuPredictState> imuPred;
    if (spec.imuPredict)
    {
        imuPred.reset(new ImuPredictState());
        // Its own stream (plan sect 0 rule 5): seed*7919 + 1400071, the measured node being
        // subject 0 (MS-1 adds 97 per subject).
        imuPred->rk.Configure(CaTblImuParams(), static_cast<int64_t>(seed) * 7919 + 1400071);
        std::ostringstream ifn;
        ifn << traceDir << "/imu_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        imuPred->log.open(ifn.str().c_str());
        imuPred->log << "t,dxTrue,dyTrue,ux,uy,psi,quxx,quxy,quyy,sigXX,sigXY,sigYY,accX,accY,"
                        "elapsedS,imuPredicted,hadFix,rejected,reset,estX,estY\n";
    }
    // MS-1. Per-run state of --measuredSubjects > 1, null without it.
    struct SubjectExtra
    {
        std::unique_ptr<std::ofstream> trace;
        std::unique_ptr<CaTblImuDeadReckoner> rk;
        double prevX = 0.0;
        double prevY = 0.0;
        uint64_t requests = 0;
        uint64_t deferredAway = 0;
        uint64_t floorCycles = 0;
        double meshBilledAir[CA_TBL_TIER_COUNT] = {0.0, 0.0, 0.0};
        uint64_t meshBilledFrames[CA_TBL_TIER_COUNT] = {0, 0, 0};
    };
    struct SubjectsState
    {
        std::vector<SubjectExtra> extra;
    };
    std::unique_ptr<SubjectsState> subj;
    if (spec.measuredSubjects > 1)
    {
        subj.reset(new SubjectsState());
    }
    // NW-2. The tier each subject ranged on this cycle (the measured tag's is `active`).
    std::vector<int> subjCycleTier(tags.size(), -1);
    // DC-3. --caseCRelative. Physical sect 2.4, Case C: a subject with no anchor range this
    // cycle ranges the peer subjects its active radio has heard (the C5 path: a direct
    // engine exchange, billed on the active tier, initiator pays). The ranges ride its
    // next entry, 3 bytes a range (OURS, billed as the extra airtime), and reach the base
    // station when a RESPONSE carrying that entry is delivered to a gateway (TA-1a),
    // within kCaseCWindowS (OURS). Each
    // cycle's delivered ranges are assembled there once its window closes: a connected
    // set of 3+ gets its relative shape by classical MDS (missing pairs completed by
    // shortest path, OURS, counted), a pair its distance. NEVER fused into a filter.
    struct CaseCRange
    {
        uint32_t from = 0;
        uint32_t to = 0;
        double rangeM = 0.0;
        double trueM = 0.0;
        CaTblTier tier = CA_TBL_TIER_868;
    };
    struct CaseCPending
    {
        uint32_t node = 0;
        double t = 0.0;
        std::vector<CaseCRange> r;
        std::map<uint32_t, Vector> truth; // member -> true position at t, for the log
    };
    static constexpr double kCaseCWindowS = 10.0;
    std::deque<CaseCPending> caseCPending;
    std::map<double, std::vector<CaseCPending>> caseCAtBase; // cycle t -> delivered sets
    std::unique_ptr<std::ofstream> caseCRangeLog;
    std::unique_ptr<std::ofstream> caseCShapeLog;
    uint64_t caseCCycles = 0, caseCLonely = 0, caseCTried = 0, caseCGot = 0;
    uint64_t caseCDelivered = 0, caseCDropped = 0, caseCPairs = 0, caseCShapes = 0;
    uint64_t caseCCompleted = 0, caseCFused = 0;
    double caseCCarryS = 0.0;
    if (spec.caseCRelative)
    {
        std::ostringstream a;
        std::ostringstream b;
        a << traceDir << "/case_c_ranges_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        b << traceDir << "/case_c_relative_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        caseCRangeLog.reset(new std::ofstream(a.str().c_str()));
        caseCShapeLog.reset(new std::ofstream(b.str().c_str()));
        *caseCRangeLog << "t,nodeId,peerId,tier,rangeM,trueM,deliveredAt\n";
        *caseCShapeLog << "t,k,members,measuredPairs,completedPairs,cohesionM,stressM,offsets,truth\n";
    }
    auto caseCPeers = [&](size_t self, const Vector& selfPos, CaTblEnergyModel& en, CaTblMac* mac,
                          CaTblTier tier, double tNow, Time at, double atSec) {
        ++caseCCycles;
        const uint32_t selfId = tags[self].nodeId;
        std::vector<size_t> peers;
        const std::vector<CaTblNeighbour> nb = mac->GetNeighbours(tier, tNow);
        for (size_t k = 0; k < nb.size(); ++k)
        {
            for (size_t q = 0; q < spec.measuredSubjects && q < tags.size(); ++q)
            {
                if (q != self && tags[q].nodeId == nb[k].id)
                {
                    peers.push_back(q);
                }
            }
        }
        if (peers.empty())
        {
            ++caseCLonely;
            return;
        }
        const int ti = static_cast<int>(tier);
        CaseCPending p;
        p.node = selfId;
        p.t = tNow;
        p.truth[selfId] = selfPos;
        for (size_t i = 0; i < peers.size(); ++i)
        {
            const size_t q = peers[i];
            const Vector& qp = (q == 0) ? pos : tags[q].pos;
            const uint16_t qid = static_cast<uint16_t>(tags[q].nodeId);
            bool got = false;
            double rg = 0.0;
            if (tier == CA_TBL_TIER_2P4)
            {
                PhysicsResult2p4 r = engine2p4.ProcessTransmission(
                    static_cast<uint16_t>(selfId), qid, selfPos, qp, phy[ti].txPowerDbm, phy[ti].sf,
                    phy[ti].bandwidthHz, 0, at);
                got = r.packetReceived;
                rg = r.estimatedRangeM;
            }
            else if (tier == CA_TBL_TIER_UWB)
            {
                PhysicsResultUWB r = engineUwb.ProcessTransmission(
                    static_cast<uint16_t>(selfId), qid, selfPos, qp, phy[ti].txPowerDbm, phy[ti].sf,
                    phy[ti].bandwidthHz, 0, at);
                got = r.packetReceived;
                rg = r.estimatedRangeM;
            }
            else
            {
                PhysicsResult r = engine868.ProcessTransmission(
                    static_cast<uint16_t>(selfId), qid, selfPos, qp, phy[ti].txPowerDbm, phy[ti].sf,
                    phy[ti].bandwidthHz, 0, at);
                got = r.packetReceived;
                rg = r.estimatedRangeM;
            }
            ++caseCTried;
            if (got && std::isfinite(rg) && rg > 0.0)
            {
                CaseCRange cr;
                cr.from = selfId;
                cr.to = qid;
                cr.rangeM = rg;
                cr.trueM = CalculateDistance(selfPos, qp);
                cr.tier = tier;
                p.r.push_back(cr);
                p.truth[qid] = qp;
                ++caseCGot;
            }
        }
        en.AccountRangingExchange(tier, static_cast<uint32_t>(peers.size()),
                                  static_cast<uint32_t>(p.r.size()), atSec);
        if (p.r.empty())
        {
            return;
        }
        // Carried on the next entry: the extra airtime of 3 bytes a range on this tier.
        const uint32_t base = CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(CA_TBL_MSG_ENTRY_PUSH, 1);
        const uint32_t more = base + 3u * static_cast<uint32_t>(p.r.size());
        const double extra =
            (tier == CA_TBL_TIER_UWB)
                ? CaTblEnergyModel::UwbFrameTimeS(more) - CaTblEnergyModel::UwbFrameTimeS(base)
                : CaTblEnergyModel::LoRaTimeOnAirS(phy[ti].sf, phy[ti].bandwidthHz, more, 1) -
                      CaTblEnergyModel::LoRaTimeOnAirS(phy[ti].sf, phy[ti].bandwidthHz, base, 1);
        en.AccountFrameTx(tier, atSec, extra);
        caseCCarryS += extra;
        caseCPending.push_back(p);
    };
    // MS-1. Subjects range in their own slots of a round BEFORE tag 0's instant, as relays
    // do under --relaySlots (M.1): all at one instant, they collide in the engines'
    // interference model -- measured, tag 0's static_868 fixes fell 212 -> 2 at five
    // subjects. The relay round is reused when it exists.
    std::unique_ptr<RelayRound> subjRoundOwn;
    uint64_t tag0RequestFrames = 0; // MS-1. ESC_REQUEST frames tag 0 sent, for its row
    // NW-1. --hopProvenance: one row per tag-0 cycle with the depth of what it fused.
    std::unique_ptr<std::ofstream> hopLog;
    uint64_t hopFixesByDepth[4] = {0, 0, 0, 0};
    if (spec.hopProvenance)
    {
        std::ostringstream hfn;
        hfn << traceDir << "/hops_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        hopLog.reset(new std::ofstream(hfn.str().c_str()));
        *hopLog << "t,nMeas,nRelayed,maxHop,usedFloor,hadFix,rejected,hopCountAtFix\n";
    }
    // NW-5. --arbitrationV2: the arbiters (every gateway's and finder's 2.4 GHz radio, the
    // radio a UWB request arrives on), by node id for the range evidence the scenario
    // reports, and arb_<arm>_seed<N>.csv.
    std::map<uint16_t, CaTblMac*> arbMacById;
    std::unique_ptr<std::ofstream> arbLog;
    std::vector<uint8_t> pendQT(tags.size(), 0); // NW-4: a deferred request's bit
    if (spec.arbitrationV2)
    {
        std::ostringstream afn;
        afn << traceDir << "/arb_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        arbLog.reset(new std::ofstream(afn.str().c_str()));
        *arbLog << "t,arbiter,requester,event,u,tDeg,rem,tBatt,tStale,qt,queuePos,evidenceAgeS\n";
        for (size_t m = 0; m < macs.size(); ++m)
        {
            const CaTblIdentityClass ic = macs[m].GetIdentityClass();
            if (macs[m].GetChannel() == &chan[CA_TBL_TIER_2P4] &&
                (ic == CA_TBL_ID_ANCHOR || ic == CA_TBL_ID_RESCUER))
            {
                arbMacById[macs[m].GetNodeId()] = &macs[m];
                macs[m].SetArbLog(arbLog.get());
            }
        }
    }
    std::map<uint32_t, std::pair<uint16_t, double>> reachSeen; // TA-2: node -> (arbiter, t)
    auto noteUwbEvidence = [&](uint16_t anchorId, uint32_t who, CaTblTier tier, double rangeM,
                               double tS) {
        // NW-5: a UWB exchange, or a 2.4 GHz range under the UWB r50, is evidence for the
        // ranged anchor that this node is within its UWB reach.
        if (arbMacById.empty() ||
            !(tier == CA_TBL_TIER_UWB || (tier == CA_TBL_TIER_2P4 && rangeM < 29.5)))
        {
            return;
        }
        std::map<uint16_t, CaTblMac*>::iterator am = arbMacById.find(anchorId);
        if (am != arbMacById.end())
        {
            am->second->NoteUwbEvidence(static_cast<uint16_t>(who), tS);
            reachSeen[who] = std::make_pair(anchorId, tS); // TA-2: the node knows it too
        }
    };
    auto reachOf = [&](uint32_t who, double tS) -> uint16_t {
        // TA-2: the arbiter this node last had reach evidence with, if within 10 s.
        std::map<uint32_t, std::pair<uint16_t, double>>::const_iterator r = reachSeen.find(who);
        return (r != reachSeen.end() && tS - r->second.second <= 10.0) ? r->second.first : 0;
    };
    // NW-9. --queryV2: every answer a requester receives (responses_*), every decision a
    // node takes on a v2 QUERY (queryv2_*), and each tag's last PCS / integrity verdict /
    // granting anchor, which its published entry carries.
    std::unique_ptr<std::ofstream> respLog;
    std::unique_ptr<std::ofstream> qv2Log;
    std::vector<double> v2Pcs(tags.size(), 0.0);
    std::vector<uint8_t> v2Suspect(tags.size(), 0);
    std::vector<uint16_t> v2GrantedBy(tags.size(), 0);
    std::vector<uint8_t> v2FixTier(tags.size(), 0xFF); // TA-1: the last fix's tier
    std::vector<uint8_t> v2FixHop(tags.size(), 0);     // TA-1: and its hop count
    // TA-1a. The base station's session log and the answers it has already taken.
    std::unique_ptr<std::ofstream> sessLog;
    std::set<std::tuple<uint32_t, uint16_t, uint8_t>> baseSeen;
    uint64_t baseResults = 0;
    std::map<uint32_t, double> caseCBaseT; // DC-3: newest entry of each node the base holds
    // TA-2. The base station's own escalation requests: one row each in reqesc_*, at most
    // one per target per slot hold.
    std::unique_ptr<std::ofstream> reqLog;
    std::map<uint16_t, double> reqLastS;
    uint64_t reqCounts[4] = {0, 0, 0, 0}; // granted, queued, refused, unroutable
    if (spec.requesterEscalation)
    {
        std::ostringstream qfn2;
        qfn2 << traceDir << "/reqesc_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        reqLog.reset(new std::ofstream(qfn2.str().c_str()));
        *reqLog << "t,target,manifest,pcs,threshold,arbiter,entryT,qt,outcome\n";
    }
    const double reqThreshold = pcfg.pcsThresholdUp[CA_TBL_TIER_2P4];
    // TA-1b. One session manager per finder (a rescuer's handheld), its session over the
    // subject it is assigned -- round-robin, finder k for subject (k mod N) + 1, the rule
    // --finderTasking uses (OURS) -- and what each has taken and when it was cancelled.
    struct FinderSearch
    {
        CaTblSessionManager mgr;
        uint32_t sid = 0;
        uint8_t target = 0;
        std::set<std::tuple<uint32_t, uint16_t, uint8_t>> seen;
        uint64_t results = 0;
        double cancelledAtS = -1.0;
        double cancelledAtM = -1.0;
        std::map<uint16_t, double> reqLastS; // TA-1b requests: one per target per slot hold
    };
    // TA-1b. --finderRequesterEscalation: every finder's request, in finderreq_*.
    std::unique_ptr<std::ofstream> finderReqLog;
    uint64_t finderReq[6] = {0, 0, 0, 0, 0, 0}; // own, named, unroutable; granted, queued, refused
    if (spec.finderRequesterEscalation)
    {
        std::ostringstream ffn;
        ffn << traceDir << "/finderreq_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        finderReqLog.reset(new std::ofstream(ffn.str().c_str()));
        *finderReqLog << "t,finderId,target,manifest,pcs,threshold,route,arbiter,entryT,qt,outcome\n";
    }
    std::deque<FinderSearch> finderSearch; // deque: the callbacks hold pointers
    if (spec.finderSessions)
    {
        for (size_t fi = 0; fi < finderMacAll.size(); ++fi)
        {
            finderSearch.emplace_back();
            FinderSearch& fs = finderSearch.back();
            fs.target = static_cast<uint8_t>(1 + fi % std::max<uint32_t>(1u, spec.measuredSubjects));
            fs.mgr.SetStalenessBound(spec.sessionStalenessS);
            fs.mgr.SetSessionTimeout(spec.sessionTimeoutS);
            fs.mgr.SetPreciseOnGrant(spec.sessionPreciseOnGrant);
            fs.sid = fs.mgr.SearchAndLocalize(fs.target, static_cast<uint16_t>(finderIds[fi]),
                                              CA_TBL_PRIORITY_ROUTINE, 0.0);
        }
    }
    // TA-1b. Each finder asks for its own target from its own tier-1 radio, on the identity
    // query period. Kept apart from `queries`, whose drivers NW-3 reports.
    std::deque<QueryDriver> finderQueries;
    for (size_t fi = 0; fi < finderSearch.size(); ++fi)
    {
        QueryDriver qd;
        qd.mac = finderMacAll[fi][CA_TBL_TIER_868];
        qd.tier = CA_TBL_TIER_868;
        qd.periodS = identityQueryPeriodS;
        qd.jitterMaxS = identityQueryPeriodS * 0.25;
        qd.untilS = durationS;
        qd.manifestId = finderSearch[fi].target;
        qd.rng = macRng;
        qd.v2 = true;
        qd.manifests = std::vector<uint8_t>(1, finderSearch[fi].target);
        qd.priority = spec.identityQueryUrgent ? 1 : 0;
        finderQueries.push_back(qd);
    }
    for (size_t q = 0; q < finderQueries.size(); ++q)
    {
        Simulator::Schedule(Seconds(macRng->GetValue(0.0, finderQueries[q].periodS)),
                            &QueryDriver::Fire,
                            &finderQueries[q]);
    }
    if (spec.sessionOverMesh) // TA-1b's finder rows ride the same log
    {
        std::ostringstream sfn;
        sfn << traceDir << "/session_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        sessLog.reset(new std::ofstream(sfn.str().c_str()));
        *sessLog << "t,requesterId,sessionId,event,manifest,queryId,answererId,entryNode,"
                    "tierSource,pcs,suspect,hopCountAtFix,stateBefore,stateAfter,uwbActive,"
                    "grantedBy,distM\n";
    }
    if (spec.queryV2)
    {
        std::ostringstream rfn, qfn;
        rfn << traceDir << "/responses_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        qfn << traceDir << "/queryv2_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        respLog.reset(new std::ofstream(rfn.str().c_str()));
        qv2Log.reset(new std::ofstream(qfn.str().c_str()));
        *respLog << "t,requesterId,tier,queryId,manifest,answererId,entryNode,entryT,x,y,seq,hop,"
                    "pcsQ,integrity,batteryQ,degradationQ,uwbActive,grantedBy,tierSource,fixHop\n";
        *qv2Log << "t,node,tier,queryId,what,manifest,left\n";
        for (size_t m = 0; m < macs.size(); ++m)
        {
            macs[m].SetQueryLog(qv2Log.get());
            const uint16_t rid = macs[m].GetNodeId();
            std::ofstream* out = respLog.get();
            const bool atBase =
                spec.sessionOverMesh && macs[m].GetIdentityClass() == CA_TBL_ID_ANCHOR;
            FinderSearch* atFinder = nullptr; // TA-1b
            if (spec.finderSessions && macs[m].GetIdentityClass() == CA_TBL_ID_RESCUER)
            {
                for (size_t fi = 0; fi < finderIds.size() && fi < finderSearch.size(); ++fi)
                {
                    if (finderIds[fi] == rid)
                    {
                        atFinder = &finderSearch[fi];
                    }
                }
            }
            macs[m].SetResponseCallback([out, rid, atBase, atFinder, &sessions, &sessionId,
                                         &sessLog, &baseSeen, &baseResults, &renderCounts,
                                         &caseCBaseT, &spec, &reqLog, &reqLastS, &reqCounts, &arbMacById,
                                         &finderReqLog, &finderReq,
                                         reqThreshold](const CaTblHeader& h) {
                const CaTblTableEntry& e = h.GetSelfEntry();
                *out << std::fixed << std::setprecision(6) << Simulator::Now().GetSeconds()
                     << ',' << rid << ',' << static_cast<int>(h.GetTier()) << ','
                     << h.GetRespQueryId() << ',' << static_cast<int>(h.GetRespManifest()) << ','
                     << h.GetRespAnswererId() << ',' << (h.HasSelfEntry() ? e.nodeId : 0) << ','
                     << e.timestampS << ',' << std::setprecision(4) << e.x << ',' << e.y << ','
                     << e.seqNo << ',' << static_cast<int>(e.hopCount) << ','
                     << static_cast<int>(e.pcsQ) << ',' << static_cast<int>(e.integrityFlag)
                     << ',' << static_cast<int>(e.batteryQ) << ','
                     << static_cast<int>(e.degradationQ) << ',' << (e.uwbActive ? 1 : 0) << ','
                     << e.grantedBy << ',' << static_cast<int>(e.tierSource) << ','
                     << static_cast<int>(e.fixHop) << '\n';
                if (!h.HasSelfEntry() || !e.valid)
                {
                    return;
                }
                // TA-1b. A finder's own session takes the answers that reached IT as the
                // requester (its queries come back over the reverse path), once per answer.
                if (atFinder != nullptr && h.GetRespRequesterId() == rid)
                {
                    const std::tuple<uint32_t, uint16_t, uint8_t> fkey(
                        h.GetRespQueryId(), h.GetRespAnswererId(), h.GetRespManifest());
                    if (atFinder->seen.insert(fkey).second)
                    {
                        ++atFinder->results;
                        CaTblResult fr;
                        fr.targetManifestId = h.GetRespManifest();
                        fr.x = e.x;
                        fr.y = e.y;
                        fr.varX = e.varX;
                        fr.varY = e.varY;
                        fr.covXY = e.covXY;
                        fr.tierSource = (e.tierSource < CA_TBL_TIER_COUNT)
                                            ? static_cast<CaTblTier>(e.tierSource)
                                            : CA_TBL_TIER_868;
                        fr.pcs = static_cast<double>(e.pcsQ) / 255.0;
                        fr.hopCountAtFix = e.fixHop;
                        fr.suspect = (e.integrityFlag != 0);
                        fr.timestampS = e.timestampS;
                        fr.uwbGranted = e.uwbActive && e.grantedBy != 0;
                        const double nowF = Simulator::Now().GetSeconds();
                        const CaTblSessionState fb =
                            atFinder->mgr.Sessions().at(atFinder->sid).State();
                        atFinder->mgr.OnResult(fr, nowF);
                        const CaTblSessionState fa =
                            atFinder->mgr.Sessions().at(atFinder->sid).State();
                        if (sessLog)
                        {
                            *sessLog << std::fixed << std::setprecision(6) << nowF << ',' << rid
                                     << ',' << atFinder->sid << ",result,"
                                     << static_cast<int>(fr.targetManifestId) << ','
                                     << h.GetRespQueryId() << ',' << h.GetRespAnswererId() << ','
                                     << e.nodeId << ',' << static_cast<int>(e.tierSource) << ','
                                     << std::setprecision(4) << fr.pcs << ','
                                     << (fr.suspect ? 1 : 0) << ','
                                     << static_cast<int>(fr.hopCountAtFix) << ','
                                     << CaTblSessionStateName(fb) << ','
                                     << CaTblSessionStateName(fa) << ','
                                     << (e.uwbActive ? 1 : 0) << ',' << e.grantedBy
                                     << ",-1\n";
                        }
                        // TA-1b. Transport sect 4 at a rescuer's handheld, as TA-2 at the base:
                        // a clean result below the threshold, not on UWB, at most one request
                        // per target per slot hold. A finder is itself a UWB arbiter (NW-5):
                        // holding reach evidence of the target, it scores the request in its
                        // own queue, no frame needed ("own"). Otherwise the request goes to the
                        // arbiter the target's entry names, as TA-2's does ("named"; instant and
                        // unbilled, N.10), and with none it is unroutable and not sent. OURS.
                        if (spec.finderRequesterEscalation && !fr.suspect && !e.uwbActive &&
                            fr.pcs < reqThreshold)
                        {
                            std::map<uint16_t, double>::iterator fl =
                                atFinder->reqLastS.find(e.nodeId);
                            if (fl == atFinder->reqLastS.end() ||
                                nowF - fl->second >= spec.arbSlotHoldS)
                            {
                                atFinder->reqLastS[e.nodeId] = nowF;
                                CaTblMac* best = nullptr;
                                uint16_t bestId = 0;
                                const char* route = "unroutable";
                                std::map<uint16_t, CaTblMac*>::const_iterator own =
                                    arbMacById.find(rid);
                                if (own != arbMacById.end() &&
                                    own->second->HasUwbEvidence(e.nodeId, nowF))
                                {
                                    best = own->second;
                                    bestId = rid;
                                    route = "own";
                                    ++finderReq[0];
                                }
                                else
                                {
                                    std::map<uint16_t, CaTblMac*>::const_iterator nm =
                                        arbMacById.find(e.reachArbiter);
                                    if (e.reachArbiter != 0 && nm != arbMacById.end())
                                    {
                                        best = nm->second;
                                        bestId = e.reachArbiter;
                                        route = "named";
                                        ++finderReq[1];
                                    }
                                    else
                                    {
                                        ++finderReq[2];
                                    }
                                }
                                const uint8_t fqt = (spec.identityQueryUrgent && !e.uwbActive) ? 1 : 0;
                                const char* outcome = "unroutable";
                                if (best != nullptr)
                                {
                                    const int v = best->SubmitRequestOnBehalf(
                                        e.nodeId, e.degradationQ, e.batteryQ, fqt, CA_TBL_TIER_2P4);
                                    outcome = (v == 1) ? "granted" : ((v == 0) ? "queued" : "refused");
                                    ++finderReq[(v == 1) ? 3 : ((v == 0) ? 4 : 5)];
                                }
                                *finderReqLog << std::fixed << std::setprecision(6) << nowF << ','
                                              << rid << ',' << e.nodeId << ','
                                              << static_cast<int>(fr.targetManifestId) << ','
                                              << std::setprecision(4) << fr.pcs << ','
                                              << reqThreshold << ',' << route << ',' << bestId
                                              << ',' << std::setprecision(6) << e.timestampS << ','
                                              << static_cast<int>(fqt) << ',' << outcome << '\n';
                            }
                        }
                    }
                }
                // TA-1a. A RESPONSE delivered to ANY gateway reaches the base station over
                // the backhaul (unbilled, as for the edge estimator), once per answer; the
                // RESULT is built from the delivered entry, never from the policy.
                if (!atBase)
                {
                    return;
                }
                const std::tuple<uint32_t, uint16_t, uint8_t> key(
                    h.GetRespQueryId(), h.GetRespAnswererId(), h.GetRespManifest());
                if (!baseSeen.insert(key).second)
                {
                    return;
                }
                ++baseResults;
                {
                    // DC-3: what the base station now holds of this node, and so of the peer
                    // ranges that rode its entry.
                    std::map<uint32_t, double>::iterator cb = caseCBaseT.find(e.nodeId);
                    if (cb == caseCBaseT.end() || e.timestampS > cb->second)
                    {
                        caseCBaseT[e.nodeId] = e.timestampS;
                    }
                }
                CaTblResult res;
                res.targetManifestId = h.GetRespManifest();
                res.x = e.x;
                res.y = e.y;
                res.varX = e.varX;
                res.varY = e.varY;
                res.covXY = e.covXY;
                res.tierSource = (e.tierSource < CA_TBL_TIER_COUNT)
                                     ? static_cast<CaTblTier>(e.tierSource)
                                     : CA_TBL_TIER_868;
                res.pcs = static_cast<double>(e.pcsQ) / 255.0;
                res.hopCountAtFix = e.fixHop;
                res.suspect = (e.integrityFlag != 0);
                res.timestampS = e.timestampS;
                res.uwbGranted = e.uwbActive && e.grantedBy != 0; // TA-3
                const double nowS = Simulator::Now().GetSeconds();
                std::map<uint32_t, CaTblSession>::const_iterator si =
                    sessions.Sessions().find(sessionId);
                const CaTblSessionState before =
                    (si != sessions.Sessions().end()) ? si->second.State() : CA_TBL_SESSION_CLOSED;
                sessions.OnResult(res, nowS);
                si = sessions.Sessions().find(sessionId);
                const CaTblSessionState after =
                    (si != sessions.Sessions().end()) ? si->second.State() : CA_TBL_SESSION_CLOSED;
                ++renderCounts[CaTblRenderFor(res)];
                *sessLog << std::fixed << std::setprecision(6) << nowS << ",0," << sessionId
                         << ",result," << static_cast<int>(res.targetManifestId) << ','
                         << h.GetRespQueryId() << ',' << h.GetRespAnswererId() << ','
                         << e.nodeId << ',' << static_cast<int>(e.tierSource) << ','
                         << std::setprecision(4) << res.pcs << ',' << (res.suspect ? 1 : 0)
                         << ',' << static_cast<int>(res.hopCountAtFix) << ','
                         << CaTblSessionStateName(before) << ',' << CaTblSessionStateName(after)
                         << ',' << (e.uwbActive ? 1 : 0) << ',' << e.grantedBy << ",-1\n";
                // TA-2. Transport sect 4: on a RESULT with PCS below the threshold and a CLEAN
                // verdict, the requester asks for UWB for that target -- at the arbiter
                // with UWB reach of it, never at "the nearest gateway", which under this
                // topology offers UWB only to something beside it. The target's entry names
                // the arbiter it last had reach evidence with (within 10 s of publishing);
                // with none, the request is unroutable and not sent. Delivered to that
                // arbiter over backhaul and mesh, modelled as instant and unbilled (OURS).
                // At most one per target per slot hold.
                if (!spec.requesterEscalation || res.suspect || e.uwbActive ||
                    !(res.pcs < reqThreshold))
                {
                    return;
                }
                std::map<uint16_t, double>::iterator lr = reqLastS.find(e.nodeId);
                if (lr != reqLastS.end() && nowS - lr->second < spec.arbSlotHoldS)
                {
                    return;
                }
                reqLastS[e.nodeId] = nowS;
                CaTblMac* best = nullptr;
                const uint16_t bestId = e.reachArbiter;
                std::map<uint16_t, CaTblMac*>::const_iterator am = arbMacById.find(bestId);
                if (bestId != 0 && am != arbMacById.end())
                {
                    best = am->second;
                }
                const uint8_t qt = (spec.identityQueryUrgent && !e.uwbActive) ? 1 : 0;
                const char* outcome = "unroutable";
                if (best != nullptr)
                {
                    const int v = best->SubmitRequestOnBehalf(e.nodeId, e.degradationQ, e.batteryQ,
                                                              qt, CA_TBL_TIER_2P4);
                    outcome = (v == 1) ? "granted" : ((v == 0) ? "queued" : "refused");
                    ++reqCounts[(v == 1) ? 0 : ((v == 0) ? 1 : 2)];
                }
                else
                {
                    ++reqCounts[3];
                }
                *reqLog << std::fixed << std::setprecision(6) << nowS << ',' << e.nodeId << ','
                        << static_cast<int>(res.targetManifestId) << ',' << std::setprecision(4)
                        << res.pcs << ',' << reqThreshold << ',' << bestId << ','
                        << std::setprecision(6) << e.timestampS << ',' << static_cast<int>(qt)
                        << ',' << outcome << '\n';
            });
        }
    }
    // NW-7. --grantGatedUwb: every request, verdict and cancellation, per subject, in
    // grants_<arm>_seed<N>.csv, so a pending window is on disk exactly.
    std::unique_ptr<std::ofstream> grantLog;
    if (spec.grantGatedUwb)
    {
        std::ostringstream gfn;
        gfn << traceDir << "/grants_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        grantLog.reset(new std::ofstream(gfn.str().c_str()));
        *grantLog << "t,subjectId,event,anchorId\n";
    }
    auto grantNote = [&grantLog](double atS, uint32_t sid, const char* ev, uint16_t anchor) {
        if (grantLog)
        {
            *grantLog << std::fixed << std::setprecision(6) << atS << ',' << sid << ',' << ev
                      << ',' << anchor << '\n';
        }
    };
    // MS-1. --finderTasking=roundrobin: finder k searches for subject (k mod N) + 1, and
    // every cycle of every finder is a row of tasking_<arm>_seed<N>.csv.
    const bool taskRR = (spec.finderTasking == "roundrobin");
    std::unique_ptr<std::ofstream> taskLog;
    std::vector<uint64_t> taskSweep, taskConverge, taskBlind;
    if (taskRR)
    {
        std::ostringstream tfn;
        tfn << traceDir << "/tasking_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        taskLog.reset(new std::ofstream(tfn.str().c_str()));
        *taskLog << "t,finderId,targetId,phase,entryX,entryY,entryVarX,entryVarY,entryT,goalX,"
                    "goalY\n";
        taskSweep.assign(nFinders, 0);
        taskConverge.assign(nFinders, 0);
        taskBlind.assign(nFinders, 0);
    }
    // NW-0. Per-run state of --meshLinks=all, null without it.
    struct MeshState
    {
        double billedAir[CA_TBL_TIER_COUNT] = {0.0, 0.0, 0.0};
        uint64_t billedFrames[CA_TBL_TIER_COUNT] = {0, 0, 0};
        uint64_t peersSkipped = 0;
    };
    std::unique_ptr<MeshState> mesh;
    if (spec.meshLinks == "all")
    {
        mesh.reset(new MeshState());
        provider.SetMeshAll(true);
        // NW-0. The first slice belongs to the starting tier; a hybrid arm keeps 868 MHz
        // awake whenever it is off 868.
        const CaTblTier t0 = policy.GetActiveTier();
        const bool hybrid0 = (mode == CA_TBL_MODE_FULL || mode == CA_TBL_MODE_NO_INTEGRITY ||
                              mode == CA_TBL_MODE_FIXED_THRESHOLD);
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            if (nodeMac[ti] != nullptr)
            {
                nodeMac[ti]->SetAsleep(!(ti == static_cast<int>(t0) ||
                                         (hybrid0 && t0 != CA_TBL_TIER_868 &&
                                          ti == CA_TBL_TIER_868)));
            }
        }
    }
    // DC-9. Per-run state of --imuWitnessSource=model, null without it: the witness track,
    // anchored at the last fused fix and advanced by the synthetic IMU's u.
    struct ImuWitnessState
    {
        bool anchored = false;
        double wx = 0.0;
        double wy = 0.0;
        bool haveFix = false;
        double fx = 0.0;
        double fy = 0.0;
        double rxx = 0.0;
        double ryy = 0.0;
        double sigTrace = 0.0;
        double disagree = 0.0;
        double tol = 0.0;
        uint64_t evals = 0;
        uint64_t refusals = 0;
        uint64_t absorbed = 0;
        std::ofstream log;
    };
    std::unique_ptr<ImuWitnessState> witness;
    if (spec.imuWitnessSource == "model")
    {
        witness.reset(new ImuWitnessState());
        std::ostringstream wfn;
        wfn << traceDir << "/witness_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        witness->log.open(wfn.str().c_str());
        witness->log << "t,anchored,wx,wy,haveFix,fixX,fixY,rxx,ryy,sigTrace,disagree,tol,"
                        "evaluated,refused,absorbed,estX,estY\n";
    }
    // DC-13. Per-run state of --entryPrediction, null without it.
    struct EntryPredState
    {
        uint64_t subs = 0;
        uint64_t moved = 0;
        std::ofstream log;
    };
    std::unique_ptr<EntryPredState> entryPred;
    if (spec.entryPrediction)
    {
        entryPred.reset(new EntryPredState());
        std::ostringstream efn2;
        efn2 << traceDir << "/entrypred_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        entryPred->log.open(efn2.str().c_str());
        entryPred->log << "t,anchorId,seqNo,ageS,x0,y0,vx,vy,xPred,yPred,varAdd,varAddLegacy\n";
    }
    // DC-4. Per-run state of --caseD, null without it.
    struct CaseDState
    {
        uint64_t cycles = 0;
        uint64_t applied = 0;
        uint64_t refused = 0;
        uint64_t wanted = 0;
        uint64_t escalated = 0;
        uint64_t demoted = 0;
        std::ofstream log;
    };
    std::unique_ptr<CaseDState> caseDst;
    if (spec.caseD)
    {
        caseDst.reset(new CaseDState());
        std::ostringstream cfn;
        cfn << traceDir << "/cased_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        caseDst->log.open(cfn.str().c_str());
        caseDst->log << "t,activeTier,anchorId,sourceKind,nis,applied,traceSigma,pcs,"
                        "wantedEscalation,escalated,deescalated,nextTier\n";
    }
    // DC-11. Per-run state of --sparseInStep, null without it.
    struct SparseInStepState
    {
        bool twoNoFloor = false;
        double prevX = 0.0;
        double prevY = 0.0;
        uint64_t attempts = 0;
        uint64_t caseA = 0;
        uint64_t caseB = 0;
        uint64_t refused[4] = {0, 0, 0, 0};
        uint64_t fullWithRelays = 0;
        std::ofstream log;
    };
    std::unique_ptr<SparseInStepState> sparse;
    if (spec.sparseInStep)
    {
        sparse.reset(new SparseInStepState());
        std::ostringstream sfn;
        sfn << traceDir << "/sparse_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        sparse->log.open(sfn.str().c_str());
        sparse->log << "t,nMeas,verdict,fixKind,priorX,priorY,prevX,prevY,x,y,varM2,estX,estY,"
                       "rangeUpdates,positionFuses,priorSource\n";
    }
    // MC-2. Per-run state of --triggeredUpdates, null without it.
    struct TriggeredUpdateState
    {
        double preX = 0.0;
        double preY = 0.0;
        double preVx = 0.0;
        double preVy = 0.0;
        double preTraceP = 0.0;
        uint64_t nLarge = 0;
        uint64_t nGasp = 0;
        uint64_t billedLarge = 0;
        uint64_t billedGasp = 0;
        std::ofstream log;
    };
    std::unique_ptr<TriggeredUpdateState> trig;
    // N.3 / row 55. The subjects' large corrections, one row each, null without the flag.
    std::unique_ptr<std::ofstream> trigS;
    uint64_t trigSLarge = 0, trigSBilled = 0, trigSGasp = 0;
    // N.21. --subjectFinderPseudoAnchor: one row per subject range to a finder, null without.
    std::unique_ptr<std::ofstream> pseudoS;
    uint64_t pseudoSubs = 0, pseudoNoEntry = 0, pseudoDigests = 0;
    // N.20. Radio-cycles (one per tier per cycle) a dead node was not billed for.
    uint64_t deadNoDrawT0 = 0, deadNoDrawS = 0, deadNoDrawHs = 0;
    // N.21 strict: tag 0's finder ranges, one row each, null without --finderPseudoAnchorStrict.
    std::unique_ptr<std::ofstream> strictLog;
    uint64_t strictReport = 0, strictDropped = 0, strictNoInflate = 0, strictNoInflateS = 0;
    if (spec.finderPseudoAnchorStrict)
    {
        std::ostringstream sfn2;
        sfn2 << traceDir << "/pseudo_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        strictLog.reset(new std::ofstream(sfn2.str().c_str()));
        *strictLog << "t,finderId,kind,usedX,usedY,trueX,trueY\n";
    }
    if (spec.subjectFinderPseudoAnchor)
    {
        std::ostringstream pfn;
        pfn << traceDir << "/subjpseudo_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        pseudoS.reset(new std::ofstream(pfn.str().c_str()));
        *pseudoS << "t,subjectId,finderId,kind,entryT,entryX,entryY,vx,vy,predX,predY,trueX,trueY,"
                    "entryVarX,entryVarY,ageVar,sigmaRaw,sigmaEff\n";
    }
    if (spec.subjectTriggeredUpdates)
    {
        std::ostringstream sfn;
        sfn << traceDir << "/subjtriggered_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        trigS.reset(new std::ofstream(sfn.str().c_str()));
        *trigS << "t,subjectId,kind,tier,moveM,threshM,estX,estY,predX,predY,totalJ,framesBilled\n";
    }
    if (spec.triggeredUpdates)
    {
        trig.reset(new TriggeredUpdateState());
        std::ostringstream tfn;
        tfn << traceDir << "/triggered_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        trig->log.open(tfn.str().c_str());
        trig->log << "t,kind,tier,moveM,threshM,estX,estY,predX,predY,totalJ,framesBilled\n";
    }
    std::unique_ptr<std::ofstream> entriesLog;
    if (spec.entryVelocity)
    {
        std::ostringstream efn;
        efn << traceDir << "/entries_" << CaTblPolicyModeName(mode) << "_seed" << seed
            << ".csv";
        entriesLog.reset(new std::ofstream(efn.str().c_str()));
        *entriesLog << "kind,t,holderId,entryId,seqNo,hopCount,timestampS,x,y,vx,vy,refVx,"
                       "refVy\n";
    }
    // One row of the record. kind "pub": a producer's own slot read back from its MAC,
    // with the value it was handed (refVx/refVy). kind "held": an entry of another node, as
    // this node's radios hold it (the union, newest sequence winning).
    auto entryRow = [&](const char* kind, double tNow, uint16_t holder,
                        const CaTblTableEntry& e, const std::string& ref) {
        *entriesLog << kind << ',' << std::fixed << std::setprecision(4) << tNow << ','
                    << holder << ',' << e.nodeId << ',' << e.seqNo << ','
                    << int(e.hopCount) << ',' << e.timestampS << ','
                    << std::setprecision(6) << e.x << ',' << e.y << ','
                    << std::setprecision(9) << e.vx << ',' << e.vy << ',' << ref << '\n';
    };
    auto refOf = [](double vx, double vy) {
        std::ostringstream r;
        r << std::fixed << std::setprecision(9) << vx << ',' << vy;
        return r.str();
    };
    // WHERE THE SUBJECT WAS LAST SEEN. With a base camp that is the camp itself,
    // because the walker starts there; without one it is wherever the run began.
    // The LKP baseline never updates it -- no direction is known, so the point
    // estimate cannot move, and only the uncertainty grows.
    if (dep.hasBaseCamp)
    {
        // The camp, NOT the walker's start. For a still subject those differ, and
        // seeding from the start would hand the no-system baseline the answer in
        // the one case where it would otherwise look perfect.
        blLkpX = dep.baseCampX;
        blLkpY = dep.baseCampY;
    }
    else
    {
        blLkpX = pos.x;
        blLkpY = pos.y;
    }
    double imuDisagreeM = 0.0;
    double imuTolM = 0.0;
    double lastTrustedFixS = 0.0;
    // Where the SHS was last calibrated, and whether it ever was.
    double lastTrustedX = 0.0;
    double lastTrustedY = 0.0;
    bool haveTrusted = false;
    uint64_t shsTrustedFixes = 0;
    uint64_t shsFixesSkipped = 0;
    uint64_t imuFusedCycles = 0;
    double tFirstFixS = -1.0;
    double tFirst100mS = -1.0;

    // The real step-and-heading system from IMU_SHS_V1.1, not a scale-error stub.
    lorawan::ShsEngineParams shsParams;
    // Anchor the dead-reckoned position at each clean fix. Without this the SHS
    // position error accumulates from seeding and is never reset, while the
    // fusion variance resets every trusted fix -- so the filter is handed a
    // 459 m position labelled 5.4 m. See --shsSnapPosition.
    shsParams.snapPositionToTrustedFix = shsSnapPosition;
    if (shsStepBounds)
    {
        // A walking human's step, not a fitted range. Refusing a calibration
        // outside it is the only thing standing between a sparse-fix arm and a
        // dead-reckoned position 1.9e6 m from the body.
        shsParams.stepLength.minPlausibleStepM = 0.25;
        shsParams.stepLength.maxPlausibleStepM = 1.10;
    }
    Ptr<NormalRandomVariable> shsRng = CreateObject<NormalRandomVariable>();
    // Own stream, set before first use: the inertial sensor model must not shift a
    // single draw the radio layer makes.
    shsRng->SetStream(static_cast<int64_t>(seed) * 7919 + 1300021);
    lorawan::ShsEngine shsEngine(shsParams, shsRng);
    // Seeded with the true deployment state, which is what the engine's own
    // documentation says removes the cold start: without it heading stays on a
    // fallback until two fixes happen to bracket a straight walk.
    // NOT seeded here: the deployment heading is not known until the node moves, and
    // seeding zero would assert it faces +x. ShsDriver seeds on the first moving cycle.
    ShsDriver shsDriver;
    // Ideal dead reckoner: anchored at the last clean fix, advanced by TRUE
    // displacement. A ceiling on inertial contribution, not a sensor model.
    double idealX = 0.0, idealY = 0.0;
    bool idealAnchored = false;
    // The position covariance the dead reckoner was anchored WITH. A dead
    // reckoner inherits its anchor's error and holds it constant, so this is a
    // floor on its accuracy that drift-since-anchor cannot see.
    double imuAnchorVarM2 = 0.0;
    shsDriver.engine = &shsEngine;

    auto& pcsHist = TS0.pcsHist;
    auto& pcsDegradation = TS0.pcsDegradation;
    auto& batteryUsedFrac = TS0.batteryUsedFrac;

    // Step count is fixed up front and time is read back from the scheduler rather
    // than accumulated in the loop variable. With an event layer running, the
    // authoritative clock is Simulator::Now(); keeping a parallel float counter is
    // how the two silently drift apart.
    if (subj)
    {
        // MS-1. Subjects 2..N: configured exactly as tag 0 -- the same policy config, the
        // same energy model setup, the same beacon airtimes -- each with its own trace
        // and, under --imuPredict, its own IMU on its own stream (+97 per subject).
        for (uint32_t pt = 1; pt < spec.measuredSubjects; ++pt)
        {
            const size_t pi = pt - 1;
            CaTblTagState& sb = tags[pt];
            sb.nodeId = peerIds[pi];
            sb.walker = &peerWalkers[pi];
            sb.pos = peerWalkers[pi].Position();
            sb.policy.Configure(pcfg);
            sb.policy.Initialize(sb.pos.x, sb.pos.y, 100.0, 5.0);
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                sb.energy.SetLoRaPhy(static_cast<CaTblTier>(ti), phy[ti].sf, phy[ti].bandwidthHz,
                                     1);
                sb.energy.SetTier1ChargeReplies(tier1ChargeReplies);
                sb.energy.SetEdgeServer(gatewayEdgeServer);
                sb.beaconToaS[ti] = beaconToaS[ti];
            }
            SubjectExtra ex;
            std::ostringstream sfn;
            sfn << traceDir << "/subjtrace_" << CaTblPolicyModeName(mode) << "_seed" << seed
                << "_subj" << sb.nodeId << ".csv";
            ex.trace.reset(new std::ofstream(sfn.str().c_str()));
            *ex.trace << "t,subjectId,activeTier,nextTier,fixTier,usedFloor,trueX,trueY,estX,"
                         "estY,errM,pcs,traceSigma,nAnchors,hadFix,escalated,deescalated,"
                         "cumJ\n";
            if (spec.imuPredict)
            {
                ex.rk.reset(new CaTblImuDeadReckoner());
                ex.rk->Configure(CaTblImuParams(), static_cast<int64_t>(seed) * 7919 + 1400071 +
                                                       97 * static_cast<int64_t>(pt));
            }
            ex.prevX = sb.pos.x;
            ex.prevY = sb.pos.y;
            subj->extra.push_back(std::move(ex));
        }
    }

    // NW-7. Each measured subject's radios report the fate of its own UWB request to its
    // own policy: a grant commits the change for the next cycle, a denial drops the request
    // (and its retries -- the policy asks afresh, with fresh terms, when it next wants UWB),
    // and exhausted retries time it out.
    if (spec.grantGatedUwb)
    {
        auto wire = [&grantNote, &v2GrantedBy](CaTblMac* mm, CaTblPolicy* po, uint32_t sid,
                                               size_t tagIdx) {
            if (mm == nullptr)
            {
                return;
            }
            mm->SetUwbGrantCallback([mm, po, sid, tagIdx, &grantNote, &v2GrantedBy](uint16_t anchor,
                                                                                int outcome) {
                const double nowS = Simulator::Now().GetSeconds();
                if (outcome == 2)
                {
                    grantNote(nowS, sid, "queued", anchor); // NW-5: still waiting
                }
                else if (outcome == 3)
                {
                    // TA-2: a grant a requester obtained for this node.
                    const bool took = po->AcceptGrantOnBehalf();
                    if (took)
                    {
                        v2GrantedBy[tagIdx] = anchor;
                    }
                    grantNote(nowS, sid, took ? "obo_accept" : "obo_refuse", anchor);
                }
                else if (outcome == 1)
                {
                    const bool took = po->ConfirmUwbGrant();
                    if (took)
                    {
                        v2GrantedBy[tagIdx] = anchor; // NW-9: the entry names its grantor
                    }
                    grantNote(nowS, sid, took ? "confirm" : "late", anchor);
                }
                else if (po->ClearUwbPending(outcome < 0))
                {
                    grantNote(nowS, sid, outcome < 0 ? "timeout" : "deny", anchor);
                    if (outcome == 0)
                    {
                        mm->CancelPendingRequests();
                    }
                }
            });
        };
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            wire(nodeMac[ti], &policy, nodeId, 0);
        }
        for (uint32_t pt = 1; subj && pt < spec.measuredSubjects; ++pt)
        {
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                wire(peerMacs[pt - 1][static_cast<size_t>(ti)], &tags[pt].policy, tags[pt].nodeId,
                     pt);
            }
        }
    }

    const uint64_t nSteps = static_cast<uint64_t>(std::floor(durationS / dt));
    for (uint64_t stepIdx = 0; stepIdx < nSteps; ++stepIdx)
    {
        walker.Step(dt);
        pos = walker.Position();

        // Peers move too, or they are stationary beacons rather than targets.
        // Position is pushed to every engine for the same reason the measured
        // tag's is: the engines key correlation and mobility state on node id.
        for (size_t pi = 0; pi < peerWalkers.size(); ++pi)
        {
            peerWalkers[pi].Step(dt);
            const Vector pp = peerWalkers[pi].Position();
            engine868.UpdateNodePosition(peerIds[pi], pp);
            engine2p4.UpdateNodePosition(peerIds[pi], pp);
            engineUwb.UpdateNodePosition(peerIds[pi], pp);
            provider.SetPeerPosition(static_cast<uint16_t>(peerIds[pi]), pp);
            if ((relayedRanging || subj) && (pi + 1) < tags.size())
            {
                tags[pi + 1].pos = pp; // MS-1: a subject's truth moves with its walker too
            }
        }

        // Every engine tracks position so its internal correlation/mobility
        // state stays continuous. This costs nothing physically -- no packet is
        // generated -- but keeps a tier's channel coherent across a gap in which
        // it was not selected. Without it, re-escalating to UWB after 200 s
        // would resume from a stale spatial correlation state.
        engine868.UpdateNodePosition(nodeId, pos);
        engine2p4.UpdateNodePosition(nodeId, pos);
        engineUwb.UpdateNodePosition(nodeId, pos);
        provider.SetNodePosition(pos);

        // Advance the discrete-event layer across this cycle BEFORE the ranging
        // decision that consumes its output: anchors beacon, CSMA backoff and
        // capture arbitration run at the node's receivers, and the one-hop
        // neighbour tables are updated. Simulator::Run() returns at the stop event
        // and may be re-entered -- Stop(delay) is relative to Now().
        Simulator::Stop(Seconds(dt));
        Simulator::Run();

        const Time now = Simulator::Now();
        const double t = now.GetSeconds();

        // L.1.4. Charge the gateways for the gossip the slice just carried, and on the
        // gossip period schedule the next round into the coming slice. Inside the loop,
        // like every other transmission this scenario decides to make.
        if (gossip)
        {
            gossip->Cycle(t, macRng);
        }

        if (vizNodes.is_open())
        {
            vizNodes << std::fixed << std::setprecision(2) << t << ',' << nodeId << ','
                     << pos.x << ',' << pos.y << '\n';
            for (uint32_t pt = 1; pt < nTags && pt < tags.size(); ++pt)
            {
                vizNodes << t << ',' << tags[pt].nodeId << ',' << tags[pt].pos.x << ','
                         << tags[pt].pos.y << '\n';
            }
        }

        // sect 49 FAULT INJECTION -- a node goes silent, then comes back.
        //
        // The node is NOT removed. It keeps its table and its position and resumes with
        // both intact. Removing it would test a different fault and would make the
        // return indistinguishable from a new node joining -- the distinction
        // CaTblNodeLifecycle exists to preserve.
        //
        // TRANSMIT SUPPRESSION ONLY. This is a node that cannot TALK: a failed transmit
        // chain, an exhausted duty budget, an amplifier browning out on a cold battery.
        // It still HEARS everything. That is why the finders still hold the casualty
        // entry across the outage -- not a bug in the injection, the fault that was
        // actually injected. An obstruction, which is deaf as well as mute, is a
        // different fault and is not this one.
        //
        // Applied every cycle rather than scheduled once, so the window is honoured
        // even if the run is shorter than it, and so the node is observably restored
        // rather than left silent by an event that never fired.
        if (injectSilentId != 0)
        {
            const bool inWindow = (t >= injectSilentStartS && t < injectSilentEndS);
            for (size_t mi = 0; mi < macs.size(); ++mi)
            {
                if (macs[mi].GetNodeId() == static_cast<uint16_t>(injectSilentId))
                {
                    macs[mi].SetSilent(inWindow);
                }
            }
        }

        // Snapshot every node's two tables. The lost node, the peers and the finders --
        // not the gateways, whose tables are large, static and not what a viewer clicks.
        if (vizNbrTable.is_open() && t >= nextTableDumpS)
        {
            nextTableDumpS = t + vizTablePeriodS;
            // ONE PASS PER NODE, NOT PER MAC. Each node owns three MACs, one per radio,
            // and iterating macs directly would emit every node three times and would
            // read one radio's table while calling it the union of three.
            std::set<uint16_t> seen;
            for (size_t mi = 0; mi < macs.size(); ++mi)
            {
                const uint16_t nid = macs[mi].GetNodeId();
                if (nid >= 101 && nid <= 199 || seen.count(nid))
                {
                    continue; // gateway, or already emitted for this node
                }
                seen.insert(nid);

                // NEIGHBOUR TABLE -- per radio, and kept per radio on purpose. Which
                // band heard a peer is the interesting part: a node audible on 868 and
                // not on UWB is the ordinary case and is what the tiering is about.
                for (size_t mj = 0; mj < macs.size(); ++mj)
                {
                    if (macs[mj].GetNodeId() != nid)
                    {
                        continue;
                    }
                    for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
                    {
                        const std::vector<CaTblNeighbour> nb =
                            macs[mj].GetNeighbours(static_cast<CaTblTier>(ti), t);
                        for (size_t k = 0; k < nb.size(); ++k)
                        {
                            vizNbrTable << std::fixed << std::setprecision(2) << t << ','
                                        << nid << ',' << ti << ',' << nb[k].id << ','
                                        << nb[k].lastHeardS << ',' << nb[k].lastRxPowerDbm
                                        << ',' << nb[k].lastSnrDb << ','
                                        << nb[k].heardCount << '\n';
                        }
                    }
                }

                // LOCATION TABLE -- the UNION across this node's three radios, newest
                // sequence winning, which is the same rule the pseudo-anchor lookup uses.
                // Knowledge is not tier-scoped even though the radios are.
                if (vizLocTable.is_open())
                {
                    std::map<uint16_t, CaTblTableEntry> merged;
                    for (size_t mj = 0; mj < macs.size(); ++mj)
                    {
                        if (macs[mj].GetNodeId() != nid)
                        {
                            continue;
                        }
                        const std::map<uint16_t, CaTblTableEntry>& tb = macs[mj].GetTable();
                        for (std::map<uint16_t, CaTblTableEntry>::const_iterator it =
                                 tb.begin(); it != tb.end(); ++it)
                        {
                            std::map<uint16_t, CaTblTableEntry>::iterator ex =
                                merged.find(it->first);
                            if (ex == merged.end() ||
                                CaTblSeqNewer(it->second.seqNo, ex->second.seqNo))
                            {
                                merged[it->first] = it->second;
                            }
                        }
                    }
                    for (std::map<uint16_t, CaTblTableEntry>::const_iterator it =
                             merged.begin(); it != merged.end(); ++it)
                    {
                        const CaTblTableEntry& e = it->second;
                        if (!e.valid)
                        {
                            continue;
                        }
                        const double age = t - e.timestampS;
                        CaTblFreshnessBounds fb;
                        vizLocTable << std::fixed << std::setprecision(2) << t << ','
                                    << nid << ',' << e.nodeId << ',' << e.x << ',' << e.y
                                    << ',' << e.varX << ',' << e.varY << ',' << e.covXY
                                    << ',' << e.timestampS << ',' << age << ','
                                    << e.seqNo << ',' << int(e.hopCount) << ','
                                    << int(e.targetManifestId) << ','
                                    << CaTblFreshnessName(CaTblClassifyFreshness(age, fb))
                                    << '\n';
                    }
                }
            }
        }

        // A searcher polls for entries. Broadcast, because a finder looking for a
        // casualty does not yet know which id it is looking for -- that is the whole
        // problem -- and the responder replies with its own entry regardless of who
        // asked. Charged like any other frame: this is airtime the dissemination costs,
        // and it should show up against the energy the finders save.
        if (finderDigestPeriodS > 0.0 && sendHandshakeFrames)
        {
            for (size_t fi = 0; fi < finderMac.size(); ++fi)
            {
                if (finderMac[fi] == nullptr)
                {
                    continue;
                }
                const double phase = static_cast<double>(fi) * 3.0; // stagger
                const double prev = t - dt;
                const double a = std::floor((prev - phase) / finderDigestPeriodS);
                const double b = std::floor((t - phase) / finderDigestPeriodS);
                if (b > a && t >= phase)
                {
                    finderMac[fi]->Send(CA_TBL_MSG_DIGEST, CA_TBL_TIER_868, 1, 0);
                }
            }
        }

        // PUBLISH. The lost node puts its current estimate and its own uncertainty
        // into its table, which is what an ENTRY_PUSH then carries to anyone who asks.
        // Published on every tier's MAC because any of them may be the one that gets
        // the airtime, and a node that only advertised on the tier it happens to be
        // using would go silent exactly when it escalated.
        if (spec.queryV2)
        {
            // NW-9: the entry carries the last decision's status.
            const bool onUwb = policy.GetActiveTier() == CA_TBL_TIER_UWB;
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (nodeMac[ti] != nullptr)
                {
                    nodeMac[ti]->SetSelfStatus(CaTblQuantise(v2Pcs[0]), v2Suspect[0],
                                               CaTblQuantise(1.0 - batteryUsedFrac),
                                               CaTblQuantise(pcsDegradation), onUwb,
                                               onUwb ? v2GrantedBy[0] : 0, v2FixTier[0],
                                               v2FixHop[0], reachOf(nodeId, t));
                }
            }
        }
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            if (nodeMac[ti] != nullptr)
            {
                nodeMac[ti]->PublishSelfEntry(policy.EstX(), policy.EstY(),
                                              policy.Pxx(), policy.Pyy(), policy.Pxy(),
                                              spec.entryVelocity ? policy.EstVx() : 0.0,
                                              spec.entryVelocity ? policy.EstVy() : 0.0, t);
            }
        }
        if (entriesLog && nodeMac[0] != nullptr)
        {
            CaTblTableEntry e;
            if (nodeMac[0]->GetTableEntry(nodeMac[0]->GetNodeId(), e))
            {
                entryRow("pub", t, nodeMac[0]->GetNodeId(), e,
                         refOf(policy.EstVx(), policy.EstVy()));
            }
        }

        // AND SO DOES EVERY PEER TARGET, for the same reason and none of its own.
        //
        // Peers published nowhere at all until a positive control refused to fire:
        // an identity-scoped query for a peer's manifest id could never be
        // answered, because the peer's position had never entered any table and no
        // proxy could stand in for an entry that did not exist. Every mechanism
        // reading the position table -- gossip, proxy answering, relay admission,
        // corroboration -- was structurally blind to the peer targets.
        //
        // What it believes and the uncertainty that belief carries, exactly as the
        // measured tag reports its own. Publishing a truer position than the peer
        // actually holds would hand every consumer a confidence nothing earned.
        for (uint32_t pt = 1; pt < nTags; ++pt)
        {
            const size_t pi = static_cast<size_t>(pt - 1);
            if (pi >= peerMacs.size())
            {
                continue;
            }
            for (size_t ti = 0; ti < peerMacs[pi].size(); ++ti)
            {
                if (peerMacs[pi][ti] == nullptr)
                {
                    continue;
                }
                if (spec.queryV2)
                {
                    // NW-9: a measured subject's status; a protocol peer has no decision.
                    const bool onUwb = tags[pt].policy.GetActiveTier() == CA_TBL_TIER_UWB;
                    peerMacs[pi][ti]->SetSelfStatus(
                        CaTblQuantise(v2Pcs[pt]), v2Suspect[pt],
                        CaTblQuantise(1.0 - tags[pt].batteryUsedFrac),
                        CaTblQuantise(tags[pt].pcsDegradation), onUwb,
                        onUwb ? v2GrantedBy[pt] : 0, v2FixTier[pt], v2FixHop[pt],
                        reachOf(tags[pt].nodeId, t));
                }
                peerMacs[pi][ti]->PublishSelfEntry(
                    tags[pt].policy.EstX(), tags[pt].policy.EstY(),
                    tags[pt].policy.Pxx(), tags[pt].policy.Pyy(),
                    tags[pt].policy.Pxy(),
                    spec.entryVelocity ? tags[pt].policy.EstVx() : 0.0,
                    spec.entryVelocity ? tags[pt].policy.EstVy() : 0.0, t);
            }
            if (entriesLog && !peerMacs[pi].empty() && peerMacs[pi][0] != nullptr)
            {
                CaTblTableEntry e;
                if (peerMacs[pi][0]->GetTableEntry(peerMacs[pi][0]->GetNodeId(), e))
                {
                    entryRow("pub", t, peerMacs[pi][0]->GetNodeId(), e,
                             refOf(tags[pt].policy.EstVx(), tags[pt].policy.EstVy()));
                }
            }
        }

        // A FINDER REPUBLISHES WHERE IT BELIEVES IT IS, EVERY CYCLE.
        //
        // It used to publish once, inside the CONSTRUCTION loop, at its spawn
        // position -- and then walk for the whole run. Nothing read the entry, so
        // nothing broke, but the moment anything does (the pseudo-anchor
        // substitution, or a rescuer's console) it would be handed a position stale
        // by however far the searcher had walked. A stale entry that looks fresh is
        // worse than no entry, because a consumer cannot tell the difference.
        //
        // Costs no airtime: PublishSelfEntry writes this node's own table slot and
        // bumps its sequence number. What goes on the air is the ENTRY_PUSH that a
        // digest already asks for, billed by entry count and not by content.
        auto republishFinder = [&](size_t fi, const Vector& truePos, double nowS) {
            if (fi >= finderMacAll.size())
            {
                return;
            }
            const Vector& off = finderGpsOffset[fi];
            // DC-10. The velocity a GNSS receiver reports: displacement over the time
            // since the last publication. The offset is constant, so it cancels. 0 on the
            // first publication, which has nothing to difference against.
            double fvx = 0.0;
            double fvy = 0.0;
            if (spec.entryVelocity)
            {
                if (finderPubPrevT[fi] >= 0.0 && nowS > finderPubPrevT[fi])
                {
                    fvx = (truePos.x - finderPubPrev[fi].x) / (nowS - finderPubPrevT[fi]);
                    fvy = (truePos.y - finderPubPrev[fi].y) / (nowS - finderPubPrevT[fi]);
                }
                finderPubPrev[fi] = truePos;
                finderPubPrevT[fi] = nowS;
            }
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (finderMacAll[fi][ti] != nullptr)
                {
                    finderMacAll[fi][ti]->PublishSelfEntry(truePos.x + off.x,
                                                           truePos.y + off.y,
                                                           finderPosVar, finderPosVar,
                                                           0.0, fvx, fvy, nowS);
                }
            }
            if (entriesLog && finderMacAll[fi][0] != nullptr)
            {
                CaTblTableEntry e;
                if (finderMacAll[fi][0]->GetTableEntry(finderMacAll[fi][0]->GetNodeId(), e))
                {
                    entryRow("pub", nowS, finderMacAll[fi][0]->GetNodeId(), e, refOf(fvx, fvy));
                }
            }
        };

        // Finders move. Their position is pushed to the engines (which key correlation
        // and mobility state on node id), to the provider (which computes the link) and
        // to anchorPosById (which the ranging poll reads for the far-end position). All
        // three, or a finder ranges from a position it has left.
        // THE ALERT. A party is not dispatched because someone stepped off a path -- that
        // is invisible to the network -- but because CONTACT WAS LOST and stayed lost.
        // Deviating is not detectable; going quiet is. That ordering matters for the
        // model as much as the story: the subject is already some way off by the time
        // anyone knows to look.
        //
        // Latched. A momentary re-contact does not recall searchers already walking,
        // which is how a real search behaves and what stops the party oscillating on a
        // marginal link.
        if (searchLaunchOnLossS > 0.0 && !partyDeployed)
        {
            bool heard = false;
            if (nodeMac[0] != nullptr)
            {
                const std::vector<CaTblNeighbour> nb =
                    nodeMac[0]->GetNeighbours(CA_TBL_TIER_868, t);
                for (size_t k = 0; k < nb.size(); ++k)
                {
                    if (nb[k].id >= 101 && nb[k].id <= 199) { heard = true; break; }
                }
            }
            if (heard) { lostContactSinceS = -1.0; }
            else if (lostContactSinceS < 0.0) { lostContactSinceS = t; }
            else if (t - lostContactSinceS >= searchLaunchOnLossS)
            {
                partyDeployed = true;
                deployedAtS = t;
                deployCause = "no gateway contact";
            }

            // SECOND TRIGGER: CONFIDENCE LOST, CONTACT INTACT.
            //
            // Measured, not assumed: at boxHalf 3000 with the ridge deployment the
            // walker still heard a gateway on 88% of cycles at 4.5 km, so the
            // contact-loss branch above cannot fire at any scale this scenario runs
            // at (BEHAVIOUR_ANOMALIES A.10). Waiting for silence would mean never
            // dispatching, which is the wrong demo and, given the link model, the
            // wrong search doctrine too.
            //
            // What actually degrades is not the link but the FIX. The ridge is
            // collinear, so a walker who leaves the line loses the geometry that
            // placed them. Measured on the seed-7 ridge run, posterior radial sigma
            // sqrt(traceSigma) is p50 21.9 m / p90 58.0 m while the walker is on the
            // trail and p50 326.6 m / p90 546.1 m once off it, and the real error
            // follows it (p50 7.7 m -> 124.9 m). The party is dispatched when the
            // network can still hear the subject but can no longer say where they
            // are, sustained over the same window. For a confidence-aware system
            // that is the more honest alarm: the trigger is the system's own stated
            // uncertainty, not an oracle reading the truth track.
            //
            // NOT sigmaGeoM, which is the better-founded quantity and was the first
            // choice. Its header says it is negative when the solve is degenerate OR
            // when the legacy PCS form never computed it -- and it is negative on
            // 720 of 720 cycles here, because the legacy form is what runs. A bound
            // on it would have deployed the party in the first window of every run
            // and looked exactly like a working alarm. Same shape as the three
            // earlier exact-zero-as-evidence defects in this project; caught this
            // time before it was believed.
            //
            // SIMPLIFICATION, recorded rather than hidden: this reads the subject's
            // last reported sigma directly instead of the copy that reaches base
            // camp through the digest path, so the alert is not delayed by
            // propagation. It is one cycle late (the decision for this cycle does
            // not exist yet) but not table-age late.
            if (!partyDeployed && searchLaunchSigmaM > 0.0)
            {
                const bool confident = lastReportHadFix && lastReportedSigmaM >= 0.0 &&
                                       lastReportedSigmaM <= searchLaunchSigmaM;
                // No fix at all is not "confidence unknown", it is confidence gone.
                if (confident) { lowConfSinceS = -1.0; }
                else if (lowConfSinceS < 0.0) { lowConfSinceS = t; }
                else if (t - lowConfSinceS >= searchLaunchOnLossS)
                {
                    partyDeployed = true;
                    deployedAtS = t;
                    deployCause = "position confidence lost";
                }
            }

            // THE OVERDUE REPORT. A party is dispatched because someone did not
            // arrive, not because a telemetry threshold tripped. Kept separate
            // from the sigma alarm rather than folded into it, because the two
            // are different claims: one is an operational decision with a known
            // time, the other is an inference from the network's own confidence.
            // A.10 records that on these deployments the second does not
            // separate, so the first is what the demo uses.
            if (!partyDeployed && searchLaunchAtS > 0.0 && t >= searchLaunchAtS)
            {
                partyDeployed = true;
                deployedAtS = t;
                deployCause = "reported overdue";
            }
        }

        auto writeTask = [&](size_t fi, uint16_t tid, const char* phase, bool found,
                             const CaTblTableEntry& e, double gx, double gy) {
            *taskLog << std::fixed << std::setprecision(4) << t << ',' << finderIds[fi] << ','
                     << tid << ',' << phase << ',';
            if (found)
            {
                *taskLog << e.x << ',' << e.y << ',' << std::setprecision(6) << e.varX << ','
                         << e.varY << ',' << std::setprecision(4) << e.timestampS;
            }
            else
            {
                *taskLog << ",,,,";
            }
            *taskLog << ',' << gx << ',' << gy << '\n';
        };
        for (size_t fi = 0; fi < finderWalkers.size(); ++fi)
        {
            Vector fp;
            // MS-1. The subject this finder searches for: tag 0, or under roundrobin
            // subject (fi mod N) + 1. The assignment rule is OURS.
            const uint32_t taskK =
                taskRR ? static_cast<uint32_t>(fi % spec.measuredSubjects) : 0u;
            const uint16_t taskId = static_cast<uint16_t>(tags[taskK].nodeId);
            CaTblTableEntry taskTe;
            bool taskFound = false;
            // No alert yet: the party is at base camp and has been told nothing. Standing
            // still is correct here, and it makes the moment of dispatch visible instead
            // of implicit.
            if ((searchLaunchOnLossS > 0.0 || searchLaunchAtS > 0.0) &&
                !partyDeployed)
            {
                fp = finderPos[fi];
                const uint16_t fid0 = static_cast<uint16_t>(finderIds[fi]);
                engine868.UpdateNodePosition(fid0, fp);
                engine2p4.UpdateNodePosition(fid0, fp);
                engineUwb.UpdateNodePosition(fid0, fp);
                provider.SetAnchorPosition(fid0, fp);
                anchorPosById[fid0] = fp;
                republishFinder(fi, fp, t);
                if (vizNodes.is_open())
                {
                    vizNodes << std::fixed << std::setprecision(2) << t << ',' << fid0
                             << ',' << fp.x << ',' << fp.y << '\n';
                }
                continue;
            }
            // MODE 2, PHASE 1: sweep outward from the last known position.
            //
            // Runs only while no entry is more informative than the sweep itself.
            // The casualty publishes its prior from the first cycle -- the camp,
            // with a sigma of order how far it could have walked -- so an entry
            // exists immediately and says nothing. Converging on it would re-park
            // the party at the camp, which is the deadlock being fixed.
            bool searching = false;
            if (finderMode == 2 && dep.hasBaseCamp)
            {
                double entrySigma = std::numeric_limits<double>::infinity();
                if (fi < finderMac.size() && finderMac[fi] != nullptr)
                {
                    CaTblTableEntry te;
                    if (finderMac[fi]->GetTableEntry(taskId, te) && te.valid)
                    {
                        entrySigma = std::sqrt(std::max(0.0, te.varX + te.varY));
                        if (taskLog)
                        {
                            taskTe = te;
                            taskFound = true;
                        }
                    }
                }
                // How far the subject could physically have gone since it was last
                // seen. Sweeping beyond that is sweeping ground it cannot occupy.
                // How far the SUBJECT could have gone bounds the area worth
                // sweeping; how fast the SEARCHERS move bounds how much of it
                // they have covered by now. Two different speeds, and using the
                // subject's for both made the radius zero whenever it was still.
                // The subject is somewhere inside the box. It may already have
                // been kilometres out at t=0 -- a still subject is PLACED there --
                // so elapsed time bounds how far the SEARCHERS have got, not how
                // far the subject might be. Bounding by the subject's speed made
                // this 1 m whenever it was stationary.
                //
                // 0.9 of the walking speed goes radial. At 0.5 a searcher reaches
                // 8.6 km in four hours against a subject 11.6 km out and could
                // never arrive; at 0.9 it reaches 15.5 km. That trades area
                // coverage for reach, and the trade is real rather than a tuning
                // choice: four searchers in four hours cannot both sweep an
                // annulus densely and cross 11.6 km.
                const double swept = 0.9 * finderSpeedMps * t;
                const double radius =
                    std::max(1.0, std::min(dep.boxHalfM, swept));
                if (entrySigma > radius)
                {
                    searching = true;
                    const double n = static_cast<double>(
                        std::max<size_t>(1, finderWalkers.size()));
                    // Archimedean sweep: radius grows with time, bearing advances
                    // so the tangential component stays about half the walking
                    // speed, and finder i is offset by 2*pi*i/n so the party
                    // covers the annulus instead of one bearing.
                    const double theta =
                        2.0 * M_PI * static_cast<double>(fi) / n
                        + ((radius > 1.0) ? (0.4 * finderSpeedMps * t / radius) : 0.0);
                    fp = Vector(dep.baseCampX + radius * std::cos(theta),
                                dep.baseCampY + radius * std::sin(theta),
                                0.0);
                    finderWalkers[fi].SetPosition(fp.x, fp.y);
                    if (taskLog)
                    {
                        ++taskSweep[fi];
                        writeTask(fi, taskId, "sweep", taskFound, taskTe, fp.x, fp.y);
                    }
                }
            }
            if (!searching && (finderMode == 1 || finderMode == 2))
            {
                // Converge on the ESTIMATE, which is all a searcher is ever given.
                // Stand off once inside standoff metres rather than walking onto the
                // casualty, so the finder stays put and keeps the short-range link up
                // instead of oscillating across the estimate every cycle.
                // What this finder was TOLD, over the air. If the casualty's
                // entry has not reached it, it has nowhere to go and holds position:
                // that is the honest behaviour, and it makes dissemination failure
                // visible as searchers standing still rather than as a silent success.
                double gx = 0.0, gy = 0.0;
                bool known = false;
                if (fi < finderMac.size() && finderMac[fi] != nullptr)
                {
                    CaTblTableEntry te;
                    if (finderMac[fi]->GetTableEntry(taskId, te))
                    {
                        gx = te.x;
                        gy = te.y;
                        known = true;
                        if (taskLog)
                        {
                            taskTe = te;
                            taskFound = true;
                        }
                        // A STALE ENTRY IS NOT KNOWLEDGE. An entry, once received,
                        // persists forever, so a finder that heard the subject ONCE
                        // converges on that position for the rest of the run. Measured
                        // at boxHalf 3000: entries 377 s old and 1029 m wrong, with the
                        // party walking confidently towards where the subject used to be.
                        //
                        // sect 23 exists for exactly this decision and nothing was asking
                        // it. Past the bound the entry may still be REPORTED -- that is
                        // the proxy responder's call, and a stale position is still worth
                        // showing an operator -- but it must not STEER. A searcher walking
                        // to a six-minute-old fix does worse than sweeping, because
                        // sweeping at least covers ground nobody has looked at.
                        //
                        // Off by default: it changes where searchers go, so it is a
                        // mechanism and not an observation.
                        if (finderStaleSteerBoundS > 0.0 &&
                            (t - te.timestampS) > finderStaleSteerBoundS)
                        {
                            known = false;
                            ++staleSteerRefusals;
                        }

                        // SPREAD OVER THE REPORTED UNCERTAINTY, DO NOT STACK ON THE
                        // POINT.
                        //
                        // Every finder walking to the same coordinate is four searchers
                        // covering one spot. That is the worst allocation exactly when
                        // it matters: the reported position is 36 m wrong and UWB
                        // reaches 13.5 m, so a stack of finders at the centroid is a
                        // stack of finders all equally out of range, and tier 3 is
                        // never reached at all. Measured before this: cycles_uwb 0.
                        //
                        // The entry already carries the casualty's OWN covariance --
                        // varX, varY -- because that is what an anti-entropy entry is
                        // for. So the search region is known to the searcher without
                        // anyone being told anything they could not have. Finder i is
                        // placed on the ellipse at angle 2*pi*i/n, scaled by
                        // finderDispersal sigmas.
                        //
                        // AXIS-ALIGNED, and that is a simplification worth naming: the
                        // entry carries covXY too, so the true ellipse is rotated. Using
                        // sqrt(varX) and sqrt(varY) ignores that rotation. It is the
                        // conservative direction -- a wrongly oriented spread still
                        // covers the region, just less efficiently -- and it avoids an
                        // eigen decomposition on a 2x2 for a search pattern that is
                        // heuristic anyway.
                        //
                        // MS-1: under roundrobin the spread is over the finders tasked
                        // to the same subject, finder fi being the (fi / N)-th of them.
                        const uint32_t grpN =
                            taskRR ? (nFinders - taskK + spec.measuredSubjects - 1) /
                                         spec.measuredSubjects
                                   : nFinders;
                        const size_t grpI = taskRR ? fi / spec.measuredSubjects : fi;
                        if (finderDispersal > 0.0 && grpN > 1)
                        {
                            const double sx = std::sqrt(std::max(te.varX, 0.0));
                            const double sy = std::sqrt(std::max(te.varY, 0.0));
                            const double th = 2.0 * M_PI * static_cast<double>(grpI) /
                                              static_cast<double>(grpN);
                            gx += finderDispersal * sx * std::cos(th);
                            gy += finderDispersal * sy * std::sin(th);
                        }
                    }
                }
                if (taskLog)
                {
                    if (known)
                    {
                        ++taskConverge[fi];
                    }
                    else
                    {
                        ++taskBlind[fi];
                    }
                    writeTask(fi, taskId, known ? "converge" : "blind", taskFound, taskTe,
                              known ? gx : std::numeric_limits<double>::quiet_NaN(),
                              known ? gy : std::numeric_limits<double>::quiet_NaN());
                }
                if (!known)
                {
                    finderPos[fi] = finderPos[fi];
                    fp = finderPos[fi];
                    engine868.UpdateNodePosition(finderIds[fi], fp);
                    engine2p4.UpdateNodePosition(finderIds[fi], fp);
                    engineUwb.UpdateNodePosition(finderIds[fi], fp);
                    provider.SetAnchorPosition(static_cast<uint16_t>(finderIds[fi]), fp);
                    anchorPosById[static_cast<uint16_t>(finderIds[fi])] = fp;
                    republishFinder(fi, fp, t);
                    continue;
                }
                Vector c = finderPos[fi];
                // Steers from where it BELIEVES it is, and walks in the real world.
                // A GPS offset therefore bends the approach: the finder closes on a
                // point displaced by its own error, which is the honest consequence
                // and the reason the offset is not merely cosmetic.
                const Vector bel(c.x + finderGpsOffset[fi].x,
                                 c.y + finderGpsOffset[fi].y, c.z);
                const double dx = gx - bel.x, dy = gy - bel.y;
                const double dist = std::sqrt(dx * dx + dy * dy);
                const double standoff = 8.0;
                const double stepM = walkSpeedMps * dt;
                if (dist > standoff && dist > 1e-9)
                {
                    const double f = std::min(1.0, stepM / dist);
                    c.x += dx * f;
                    c.y += dy * f;
                }
                finderPos[fi] = c;
                fp = c;
            }
            else
            {
                finderWalkers[fi].Step(dt);
                fp = finderWalkers[fi].Position();
                finderPos[fi] = fp;
            }
            const uint16_t fid = static_cast<uint16_t>(finderIds[fi]);
            engine868.UpdateNodePosition(fid, fp);
            engine2p4.UpdateNodePosition(fid, fp);
            engineUwb.UpdateNodePosition(fid, fp);
            provider.SetAnchorPosition(fid, fp);
            anchorPosById[fid] = fp;
            republishFinder(fi, fp, t);
            if (vizNodes.is_open())
            {
                vizNodes << std::fixed << std::setprecision(2) << t << ',' << fid << ','
                         << fp.x << ',' << fp.y << '\n';
            }
        }

        // C5. Each relay's own ranging and filter for this cycle.
        //
        // The engine calls are the SAME calls the measured tag makes -- same
        // ProcessTransmission, same tier parameters, same neighbour-table poll set --
        // so a relay's fix comes from the modelled physics rather than a stand-in.
        // Energy is charged to ITS OWN account, which keeps the §6 invariant per tag:
        // exactly one tier transmits per cycle for this tag and only that tier is
        // charged on this tag's account. Nothing here touches the measured tag's
        // energy, and with relayedRanging off none of it runs at all.
        if (relayedRanging)
        {
            const CaTblTier rt = static_cast<CaTblTier>(
                std::min<uint32_t>(CA_TBL_TIER_COUNT - 1, relayTier));
            const int rti = static_cast<int>(rt);
            if (chain)
            {
                chain->BeginCycle();
            }
            if (relayRound)
            {
                relayRound->BeginCycle();
            }
            for (uint32_t pt = 1; pt < nTags && pt < tags.size(); ++pt)
            {
                const size_t pi = pt - 1;
                if (pi >= peerMacs.size() ||
                    static_cast<size_t>(rti) >= peerMacs[pi].size())
                {
                    continue;
                }
                // MS-1: a relay that is also a measured subject is ranged, charged and
                // stepped once, by the subject pipeline below -- twice would break sect 6.
                if (subj && pt < spec.measuredSubjects)
                {
                    continue;
                }
                const std::vector<CaTblNeighbour> nb =
                    peerMacs[pi][static_cast<size_t>(rti)]->GetNeighbours(rt, t);
                // C5_INSTANT: under --relaySlots this relay polls in its own slot of the
                // round, before every relay-to-relay exchange and the measured tag.
                const Time at = relayRound ? relayRound->Burst(now, pt) : now;
                uint64_t polled = 0;
                std::vector<CaTblRangeMeasurement> pmeas;
                for (size_t a = 0; a < nb.size(); ++a)
                {
                    const uint16_t anchorId = nb[a].id;
                    std::map<uint16_t, Vector>::const_iterator ap =
                        anchorPosById.find(anchorId);
                    if (ap == anchorPosById.end())
                    {
                        continue;
                    }
                    const Vector& anchorPos = ap->second;
                    bool received = false;
                    double range = 0.0;
                    double sigmaM = 0.0;
                    bool nlosProp = false;
                    bool hiddenActive = false;
                    ++polled;
                    if (rt == CA_TBL_TIER_868)
                    {
                        PhysicsResult r = engine868.ProcessTransmission(
                            tags[pt].nodeId, anchorId, tags[pt].pos, anchorPos,
                            phy[rti].txPowerDbm, phy[rti].sf, phy[rti].bandwidthHz, 0,
                            at);
                        received = r.packetReceived;
                        range = r.estimatedRangeM;
                        sigmaM = r.rangingSigmaM;
                        nlosProp = r.isNLOS;
                        hiddenActive = (r.hiddenChannelState > 0);
                    }
                    else if (rt == CA_TBL_TIER_2P4)
                    {
                        PhysicsResult2p4 r = engine2p4.ProcessTransmission(
                            tags[pt].nodeId, anchorId, tags[pt].pos, anchorPos,
                            phy[rti].txPowerDbm, phy[rti].sf, phy[rti].bandwidthHz, 0,
                            at);
                        received = r.packetReceived;
                        range = r.estimatedRangeM;
                        sigmaM = r.rangingSigmaM;
                        nlosProp = r.isNLOS;
                        hiddenActive = (r.hiddenChannelState > 0);
                    }
                    else
                    {
                        PhysicsResultUWB r = engineUwb.ProcessTransmission(
                            tags[pt].nodeId, anchorId, tags[pt].pos, anchorPos,
                            phy[rti].txPowerDbm, phy[rti].sf, phy[rti].bandwidthHz, 0,
                            at);
                        received = r.packetReceived;
                        range = r.estimatedRangeM;
                        sigmaM = r.rangingSigmaM;
                        nlosProp = r.isNLOS;
                        hiddenActive = (r.hiddenChannelState > 0);
                    }
                    if (received && std::isfinite(range) && range > 0.0)
                    {
                        CaTblRangeMeasurement m;
                        m.anchorX = anchorPos.x;
                        m.anchorY = anchorPos.y;
                        m.rangeM = range;
                        m.nlos = nlosProp || hiddenActive;
                        m.nlosPropagation = nlosProp;
                        m.hiddenChannelActive = hiddenActive;
                        m.sigmaM = sigmaM;
                        // DC-11: who the range is to (rule 14's fields).
                        m.anchorId = anchorId;
                        m.sourceKind = CaTblSourceKindOf(anchorId);
                        pmeas.push_back(m);
                    }
                }
                // Charged on the relay's own account, RX timeouts of anchors that did
                // not answer included -- the same rule the measured tag pays -- with its
                // governor armed from the instant the burst took.
                if (relayRound && polled > 0)
                {
                    relayRound->polls += polled;
                    ++relayRound->bursts;
                }
                tags[pt].energy.AccountRangingExchange(
                    rt, static_cast<uint32_t>(nb.size()),
                    static_cast<uint32_t>(pmeas.size()),
                    relayRound ? relayRound->BurstS(t, pt) : t);
                // L.1.2: a relay short of three waits until every fix-holder has stepped.
                if (chain && chain->Defer(pt, pmeas, nb.size()))
                {
                    continue;
                }
                tags[pt].policy.Step(dt, pmeas, 0,
                                     static_cast<int32_t>(pmeas.size()),
                                     static_cast<int32_t>(nb.size()), 0);
                ++tags[pt].steps;
            }
            if (chain)
            {
                chain->RangeShortRelays(tags, nTags, rt, phy, engine868, engine2p4, engineUwb,
                                        t, dt, now, maxRelayDepth, relayVarBudgetM2,
                                        relayRound.get());
            }
        }

        // NW-5. Each arbiter re-scores its queue and gives a free slot to the argmax.
        for (std::map<uint16_t, CaTblMac*>::iterator am = arbMacById.begin();
             am != arbMacById.end(); ++am)
        {
            am->second->ArbitrationCycle();
        }
        if (subj)
        {
            // MS-1. SUBJECTS 2..N, each through the core of tag 0's cycle, on its own
            // account: rangeTier on its active tier (the poll addresses discovered
            // anchors with known positions), the tier-1 floor while a hybrid arm is off
            // 868, the same fix selection, the IMU control under --imuPredict, Step() with
            // its own evidence mask, the escalation handshake with its OWN terms, the
            // gateway downlink, and the listen/sleep billing. They step here, before tag 0
            // and at the point relays always stepped, in id order. Mechanisms outside this
            // core run for tag 0 only; main() warns when one is on.
            const bool hybridS = (mode == CA_TBL_MODE_FULL || mode == CA_TBL_MODE_NO_INTEGRITY ||
                                  mode == CA_TBL_MODE_FIXED_THRESHOLD);
            if (!relayRound && !subjRoundOwn)
            {
                subjRoundOwn.reset(new RelayRound(spec.measuredSubjects, false, FixRouter::kSlotS));
            }
            const RelayRound* sround = relayRound ? relayRound.get() : subjRoundOwn.get();
            if (chain && spec.subjectRelayChain)
            {
                // N.15: who can answer a subject's chain exchange this cycle. A plain relay
                // is a fixed-tier node on --relayTier; a subject sets its own below.
                chain->ClearAwake(tags.size());
                const uint32_t rtw = std::min<uint32_t>(CA_TBL_TIER_COUNT - 1, relayTier);
                for (size_t pr = spec.measuredSubjects; pr < tags.size() && pr < nTags; ++pr)
                {
                    chain->awake[pr] = static_cast<uint8_t>(1u << rtw);
                }
            }
            for (uint32_t pt = 1; pt < spec.measuredSubjects; ++pt)
            {
                const size_t pi = pt - 1;
                CaTblTagState& sb = tags[pt];
                SubjectExtra& ex = subj->extra[pi];
                const CaTblTier act = sb.policy.GetActiveTier();
                subjCycleTier[pt] = static_cast<int>(act); // NW-2
                const bool floorOn = hybridS && act != CA_TBL_TIER_868;
                const Time atS = sround->Burst(now, pt);
                const double atSec = sround->BurstS(t, pt);
                size_t addressedActive = 0;
                // N.21. A subject asks the finders' digest question on its active tier, as tag 0
                // does (THE EXCHANGE RUNS BOTH WAYS): the answers land in the table its ranging
                // reads. Unbilled, as tag 0's question is: the MACs carry no energy model.
                // STAGGERED, not at tag 0's phase: the finders broadcast at phase 0 and tag 0
                // asks at half a period, and five nodes asking at one instant drowned every
                // answer -- tag 0 held 0.0 finder entries a cycle instead of 1.9 (seed 7, 5 km).
                // Subject k of N asks at (0.5 + k / (2N)) of the period (OURS).
                if (pseudoS && bidirectionalDigest && sendHandshakeFrames && finderDigestPeriodS > 0.0 &&
                    peerMacs[pi][static_cast<size_t>(act)] != nullptr &&
                    !(spec.subjectBatteryDeplete && sb.batteryDead))
                {
                    const double phase =
                        finderDigestPeriodS *
                        (0.5 + static_cast<double>(pt) / (2.0 * static_cast<double>(spec.measuredSubjects)));
                    const double ka = std::floor((t - dt - phase) / finderDigestPeriodS);
                    const double kb = std::floor((t - phase) / finderDigestPeriodS);
                    if (kb > ka && t >= phase)
                    {
                        peerMacs[pi][static_cast<size_t>(act)]->Send(CA_TBL_MSG_DIGEST, act, 1, 0);
                        ++pseudoDigests;
                    }
                }
                auto rangeS = [&](CaTblTier tier) {
                    std::vector<CaTblRangeMeasurement> out;
                    size_t droppedS = 0; // N.21: finder replies whose position the subject lacks
                    if (spec.subjectBatteryDeplete && sb.batteryDead)
                    {
                        return out; // N.3: a radio with no power makes no exchange, as tag 0's
                    }
                    CaTblMac* mm = peerMacs[pi][static_cast<size_t>(tier)];
                    std::vector<CaTblNeighbour> nb = mm->GetNeighbours(tier, t);
                    std::vector<CaTblNeighbour> keep;
                    for (size_t k = 0; k < nb.size(); ++k)
                    {
                        if (nb[k].id >= 101 &&
                            anchorPosById.find(nb[k].id) != anchorPosById.end())
                        {
                            keep.push_back(nb[k]);
                        }
                    }
                    sb.energy.AccountCad(tier);
                    if (tier == act)
                    {
                        addressedActive = keep.size();
                    }
                    if (keep.empty())
                    {
                        return out;
                    }
                    const int ti = static_cast<int>(tier);
                    for (size_t a = 0; a < keep.size(); ++a)
                    {
                        const uint16_t aid = keep[a].id;
                        const Vector& ap = anchorPosById.find(aid)->second;
                        bool rcv = false;
                        double range = 0.0, sigma = 0.0;
                        bool nlosP = false, hid = false, nlosF = false;
                        if (tier == CA_TBL_TIER_868)
                        {
                            PhysicsResult r = engine868.ProcessTransmission(
                                sb.nodeId, aid, sb.pos, ap, phy[ti].txPowerDbm, phy[ti].sf,
                                phy[ti].bandwidthHz, 0, atS);
                            rcv = r.packetReceived;
                            range = r.estimatedRangeM;
                            sigma = r.rangingSigmaM;
                            nlosP = r.isNLOS;
                            hid = (r.hiddenChannelState > 0);
                            nlosF = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS,
                                                  r.hiddenChannelState);
                        }
                        else if (tier == CA_TBL_TIER_2P4)
                        {
                            PhysicsResult2p4 r = engine2p4.ProcessTransmission(
                                sb.nodeId, aid, sb.pos, ap, phy[ti].txPowerDbm, phy[ti].sf,
                                phy[ti].bandwidthHz, 0, atS);
                            rcv = r.packetReceived;
                            range = r.estimatedRangeM;
                            sigma = r.rangingSigmaM;
                            nlosP = r.isNLOS;
                            hid = (r.hiddenChannelState > 0);
                            nlosF = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS,
                                                  r.hiddenChannelState);
                        }
                        else
                        {
                            PhysicsResultUWB r = engineUwb.ProcessTransmission(
                                sb.nodeId, aid, sb.pos, ap, phy[ti].txPowerDbm, phy[ti].sf,
                                phy[ti].bandwidthHz, 0, atS);
                            rcv = r.packetReceived;
                            range = r.estimatedRangeM;
                            sigma = r.rangingSigmaM;
                            nlosP = r.isNLOS;
                            hid = (r.hiddenChannelState > 0);
                            nlosF = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS,
                                                  r.hiddenChannelState);
                        }
                        if (rcv && std::isfinite(range) && range > 0.0)
                        {
                            CaTblRangeMeasurement m;
                            m.anchorX = ap.x;
                            m.anchorY = ap.y;
                            // N.21. A FINDER IS A PSEUDO-ANCHOR for a subject too: the position
                            // it reported (the freshest entry the subject's radios hold), carried
                            // forward under --entryPrediction, its variance and age projected
                            // into sigma -- tag 0's treatment, line for line. With no entry held
                            // the subject does not know where the finder is, so the range is NOT
                            // used (tag 0's code keeps the true position there; N.21). The reply
                            // was received, and is billed as one.
                            bool dropS = false;
                            if (pseudoS && aid >= 501)
                            {
                                CaTblTableEntry fe;
                                bool feOk = false;
                                for (size_t ti2 = 0; ti2 < peerMacs[pi].size(); ++ti2)
                                {
                                    CaTblTableEntry e2;
                                    if (peerMacs[pi][ti2] != nullptr &&
                                        peerMacs[pi][ti2]->GetTableEntry(aid, e2) && e2.valid &&
                                        (!feOk || e2.timestampS > fe.timestampS))
                                    {
                                        fe = e2;
                                        feOk = true;
                                    }
                                }
                                if (!feOk)
                                {
                                    ++pseudoNoEntry;
                                    dropS = true;
                                    *pseudoS << std::fixed << std::setprecision(4) << t << ','
                                             << sb.nodeId << ',' << aid << ",dropped,,,,,,,," << ap.x
                                             << ',' << ap.y << ",,,," << sigma << ",\n";
                                }
                                else
                                {
                                    const double ageP = std::max(0.0, t - fe.timestampS);
                                    double px = fe.x, py = fe.y, ageVar;
                                    if (spec.entryPrediction)
                                    {
                                        px += fe.vx * ageP;
                                        py += fe.vy * ageP;
                                        const double sv = 0.5 * ageP; // DC-13's GNSS-velocity Q
                                        ageVar = sv * sv;
                                    }
                                    else
                                    {
                                        const double travel = walkSpeedMps * ageP;
                                        ageVar = travel * travel;
                                    }
                                    const double ux = sb.policy.EstX() - px;
                                    const double uy = sb.policy.EstY() - py;
                                    const double un = std::sqrt(ux * ux + uy * uy);
                                    if (un <= 1e-6 && spec.finderPseudoAnchorStrict)
                                    {
                                        m.anchorX = px; // N.21 strict: the report, uninflated
                                        m.anchorY = py;
                                        ++strictNoInflateS;
                                    }
                                    if (un > 1e-6)
                                    {
                                        const double fp2[2][2] = {{fe.varX + ageVar, fe.covXY},
                                                                  {fe.covXY, fe.varY + ageVar}};
                                        const double proj =
                                            CaTblProjectRelayCovariance(fp2, ux / un, uy / un);
                                        if (proj < 0.0 && spec.finderPseudoAnchorStrict)
                                        {
                                            m.anchorX = px; // N.21 strict: the report, uninflated
                                            m.anchorY = py;
                                            ++strictNoInflateS;
                                        }
                                        if (proj >= 0.0)
                                        {
                                            const double s0 = sigma;
                                            m.anchorX = px;
                                            m.anchorY = py;
                                            sigma = std::sqrt(sigma * sigma + proj);
                                            ++pseudoSubs;
                                            *pseudoS << std::fixed << std::setprecision(4) << t << ','
                                                     << sb.nodeId << ',' << aid << ",report,"
                                                     << fe.timestampS << ',' << fe.x << ',' << fe.y << ','
                                                     << std::setprecision(6) << fe.vx << ',' << fe.vy
                                                     << ',' << std::setprecision(4) << px << ',' << py
                                                     << ',' << ap.x << ',' << ap.y << ','
                                                     << std::setprecision(6) << fe.varX << ',' << fe.varY
                                                     << ',' << ageVar << ',' << s0 << ',' << sigma << '\n';
                                        }
                                    }
                                }
                            }
                            m.rangeM = range;
                            m.nlos = nlosF;
                            m.nlosPropagation = nlosP;
                            m.hiddenChannelActive = hid;
                            m.sigmaM = sigma;
                            m.anchorId = aid;
                            m.sourceKind = CaTblSourceKindOf(aid);
                            m.fromFloor = (tier != act);
                            noteUwbEvidence(aid, sb.nodeId, tier, range, t); // NW-5
                            if (dropS)
                            {
                                ++droppedS; // N.21: received, billed, not used
                            }
                            else
                            {
                                out.push_back(m);
                            }
                        }
                    }
                    sb.energy.AccountRangingExchange(tier, static_cast<uint32_t>(keep.size()),
                                                     static_cast<uint32_t>(out.size() + droppedS),
                                                     atSec);
                    // N.3 / row 55. SY-1's uplink, as tag 0's: one frame on the tier that
                    // ranged, 9 B + 6 B per answered anchor (OURS), billed to the subject.
                    if (edgeUpS && gatewayEdgeServer && tier != CA_TBL_TIER_868 && !out.empty())
                    {
                        const uint32_t ub = CA_TBL_MAC_HEADER_BYTES +
                                            6u * static_cast<uint32_t>(out.size());
                        const double ua =
                            (tier == CA_TBL_TIER_UWB)
                                ? CaTblEnergyModel::UwbFrameTimeS(ub)
                                : CaTblEnergyModel::LoRaTimeOnAirS(phy[tier].sf,
                                                                   phy[tier].bandwidthHz, ub, 1);
                        sb.energy.AccountFrameTx(tier, atSec, ua);
                        EdgeUplinkTally& eu = (*edgeUpS)[pt];
                        ++eu.frames[tier];
                        eu.records[tier] += out.size();
                        eu.airS[tier] += ua;
                    }
                    return out;
                };
                // A handshake the duty budget pushed out of the previous cycle pre-empts this
                // cycle's ranging on its tier, as tag 0's does (arbitrateDutyBudget).
                const bool preempt = sb.pendingHandshake && arbitrateDutyBudget;
                const CaTblTier pendT = sb.pendingHandshakeTier;
                bool sQT = false; // NW-4: whether the request about to go follows a query
                auto sendRequest = [&](CaTblTier on, double atT) {
                    if (spec.subjectBatteryDeplete && sb.batteryDead)
                    {
                        return; // N.3: nothing is sent, so nothing is billed
                    }
                    sb.energy.AccountEscalationHandshake(on, atT);
                    if (sendHandshakeFrames)
                    {
                        CaTblMac* mm = peerMacs[pi][static_cast<size_t>(on)];
                        mm->SetRequestTerms(
                            CaTblQuantise(sb.pcsDegradation),
                            CaTblQuantise(ArbBatteryTerm(
                                spec.arbitrationV2,
                                CaTblBatteryUsedFrac(sb.energy.GetTotalJ(), arbBatteryCapJ))));
                        if (spec.handshakeFixes)
                        {
                            mm->SetRequestTargetTier(CA_TBL_TIER_UWB);
                        }
                        mm->SetRequestQueryTriggered(sQT);
                        mm->Send(CA_TBL_MSG_ESC_REQUEST, on, 0, 0);
                        ++ex.requests;
                    }
                };
                if (preempt)
                {
                    sQT = pendQT[pt] != 0;
                    sendRequest(pendT, atSec);
                    ++sb.handshakePreemptions;
                    sb.pendingHandshake = false;
                }
                std::vector<CaTblRangeMeasurement> actM;
                if (preempt && act == pendT)
                {
                    sb.energy.AccountCad(act);
                }
                else
                {
                    actM = rangeS(act);
                }
                std::vector<CaTblRangeMeasurement> floorM;
                if (floorOn)
                {
                    if (preempt && pendT == CA_TBL_TIER_868)
                    {
                        sb.energy.AccountCad(CA_TBL_TIER_868);
                    }
                    else
                    {
                        floorM = rangeS(CA_TBL_TIER_868);
                        ++ex.floorCycles;
                    }
                }
                if (spec.caseCRelative && actM.empty() && floorM.empty() &&
                    !(spec.subjectBatteryDeplete && sb.batteryDead) &&
                    !(preempt && act == pendT))
                {
                    caseCPeers(pt, sb.pos, sb.energy, peerMacs[pi][static_cast<size_t>(act)], act,
                               t, atS, atSec); // DC-3: no anchor range this cycle
                }
                const bool actSolved = actM.size() >= 3;
                const bool usedFloorS = !actSolved && floorM.size() >= 3;
                // N.14, as tag 0's: short active set plus floor, fused (labelled active).
                std::vector<CaTblRangeMeasurement> fusedS;
                const bool fuseS = spec.fuseFloorActive && actM.size() < 3 && !floorM.empty() &&
                                   !(actM.empty() && floorM.size() >= 3);
                if (fuseS)
                {
                    fusedS = actM;
                    fusedS.insert(fusedS.end(), floorM.begin(), floorM.end());
                }
                // N.15 (--subjectRelayChain). Three or more direct ranges on either set
                // make the subject a fix-holder at depth 0. Short of three on both, it
                // ranges the holders awake on its active tier, and the relayed ranges join
                // the active set ONLY when they complete three: the sparse cases never take
                // a relayed range (OURS), and the floor's 868 MHz ranges are never mixed with
                // another tier's (the fix selection's rule). Exchanges that leave it short
                // are billed and their ranges discarded.
                std::vector<CaTblRangeMeasurement> chainM;
                RelayChain::SubjectPick chainPk;
                bool chainUsed = false;
                if (chain && spec.subjectRelayChain)
                {
                    chain->awake[pt] = static_cast<uint8_t>((1u << static_cast<int>(act)) |
                                                            (floorOn ? 1u : 0u));
                    if (actSolved || usedFloorS || (fuseS && fusedS.size() >= 3))
                    {
                        chain->SubjectHolds(pt);
                    }
                    else if (!(preempt && act == pendT) && !(spec.subjectBatteryDeplete && sb.batteryDead))
                    {
                        chainM = fuseS ? fusedS : actM; // N.14: relays complete the union
                        chainPk = chain->RangeForSubject(
                            pt, tags, nTags, act, chain->awake, phy, engine868, engine2p4,
                            engineUwb, t, dt, now, maxRelayDepth, relayVarBudgetM2,
                            relayRound.get(), chainM);
                        chainUsed = chainPk.used > 0 && chainM.size() >= 3;
                    }
                }
                const std::vector<CaTblRangeMeasurement>& measS =
                    chainUsed ? chainM : (fuseS ? fusedS : (usedFloorS ? floorM : actM));
                const CaTblTier fixTierS =
                    (fuseS && !chainUsed) ? (actM.empty() ? CA_TBL_TIER_868 : act)
                                          : (usedFloorS ? CA_TBL_TIER_868 : act);
                if (ex.rk)
                {
                    double ux, uy, qu[3];
                    ex.rk->Step(sb.pos.x - ex.prevX, sb.pos.y - ex.prevY, dt, ux, uy, qu);
                    sb.policy.SetImuControl(ux, uy, qu[0], qu[1], qu[2]);
                }
                ex.prevX = sb.pos.x;
                ex.prevY = sb.pos.y;
                uint32_t maskS = 0;
                for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
                {
                    if (sb.lastNonEmptyAtS[k] >= 0.0 && (t - sb.lastNonEmptyAtS[k]) <= demoteStalenessS)
                    {
                        maskS |= (1u << k);
                    }
                }
                if (spec.queryTriggeredEscalation)
                {
                    for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
                    {
                        if (peerMacs[pi][static_cast<size_t>(ti)]->TakeUrgentQuery())
                        {
                            sb.policy.RequestQueryEscalation(); // NW-4
                        }
                    }
                }
                // N.3 / row 55. MC-2's state carried into this cycle, before Step() moves it.
                const double tuX = sb.policy.EstX(), tuY = sb.policy.EstY();
                const double tuVx = sb.policy.EstVx(), tuVy = sb.policy.EstVy();
                const double tuTr = sb.policy.Pxx() + sb.policy.Pyy();
                const uint64_t covBeforeS = sb.policy.GetUwbCoverageClears(); // NW-7
                uint32_t hopS = 0; // NW-1, under --subjectRelayChain: the set's deepest leg
                if (chainUsed && spec.hopProvenance)
                {
                    for (size_t k = 0; k < chainM.size(); ++k)
                    {
                        hopS = std::max<uint32_t>(hopS, chainM[k].hopDepth);
                    }
                }
                CaTblDecision ds = sb.policy.Step(
                    dt, measS, hopS, static_cast<int32_t>(chainUsed ? chainM.size() : actM.size()),
                    static_cast<int32_t>(addressedActive + chainPk.tried), maskS);
                if (chainUsed)
                {
                    chain->SubjectFused(pt, sb, chainPk, act, t, maxRelayDepth, relayVarBudgetM2);
                }
                if (fuseS && !chainUsed)
                {
                    fuseFloorLog->Row(t, sb.nodeId, act, actM.size(), floorM.size(), ds, fixTierS);
                }
                ++sb.steps;
                v2Pcs[pt] = ds.pcs; // NW-9
                v2Suspect[pt] = ds.suspect ? 1 : 0;
                if (ds.hadFix)
                {
                    v2FixTier[pt] = static_cast<uint8_t>(fixTierS); // TA-1
                }
                if (grantLog)
                {
                    if (sb.policy.GetUwbCoverageClears() > covBeforeS)
                    {
                        grantNote(t, sb.nodeId, "coverage", 0);
                    }
                    if (ds.uwbPending)
                    {
                        grantNote(t, sb.nodeId, "request", 0);
                    }
                }
                if (ex.rk && ds.hadFix && !ds.measurementRejected)
                {
                    ex.rk->ResetAccumulation();
                }
                const double errS = std::hypot(ds.estX - sb.pos.x, ds.estY - sb.pos.y);
                sb.errors.push_back(errS);
                if (ds.hadFix)
                {
                    sb.errorsFixed.push_back(errS);
                }
                // The escalation handshake, as tag 0's: --handshakeFixes signals only an
                // escalation into UWB; a refused duty budget defers the escalation away.
                if (spec.handshakeFixes
                        ? (ds.escalated && (ds.nextTier == CA_TBL_TIER_UWB || ds.uwbPending))
                        : (ds.escalated || ds.deescalated))
                {
                    sQT = ds.queryTriggered;
                    if (sb.energy.CanTransmit(act, t))
                    {
                        sendRequest(act, t);
                    }
                    else if (intraCycleHandshakeDefer && sb.energy.GetNextAllowedTxS(act) < t + dt)
                    {
                        const double tDefer = sb.energy.GetNextAllowedTxS(act);
                        sendRequest(act, tDefer);
                        ++sb.handshakeIntraCycleDefers;
                        sb.handshakeIntraCycleWaitS += (tDefer - t);
                    }
                    else
                    {
                        sb.energy.NoteDeferred(act);
                        ++sb.handshakeDeferrals;
                        if (deferEscalationOnDutyLimit && ds.escalated)
                        {
                            if (arbitrateDutyBudget)
                            {
                                // The tier change commits; the handshake pre-empts the
                                // next cycle's ranging on this (capped) tier.
                                sb.pendingHandshake = true;
                                sb.pendingHandshakeTier = act;
                                pendQT[pt] = ds.queryTriggered ? 1 : 0;
                            }
                            else
                            {
                                sb.policy.AbortTierChange(act);
                                ds.escalated = false;
                                ds.nextTier = act;
                                ++ex.deferredAway;
                            }
                        }
                    }
                }
                if (gatewayEdgeServer && ds.hadFix)
                {
                    sb.energy.AccountEdgeServerDownlink(edgeDownlinkOnFixTier ? fixTierS : act);
                }
                // N.3 / row 55. MC-2's LARGE CORRECTION, as tag 0's: the posterior moved more
                // than k sqrt(trace P) of the previous posterior from the constant-velocity
                // prediction. The corrected entry is published and pushed on the tier the
                // subject transmitted on this cycle, with the redundant class's repeats,
                // billed to the subject (sect 6). Subjects have no battery depletion, so no
                // last gasp (N.3).
                if (trigS && tuTr > 0.0 && std::isfinite(tuTr) && !sb.batteryDead &&
                    peerMacs[pi][static_cast<size_t>(act)] != nullptr)
                {
                    const double pxS = tuX + tuVx * dt, pyS = tuY + tuVy * dt;
                    const double moveS = std::hypot(sb.policy.EstX() - pxS, sb.policy.EstY() - pyS);
                    const double thrS = spec.triggeredUpdateK * std::sqrt(tuTr);
                    if (moveS > thrS)
                    {
                        for (size_t ti = 0; ti < peerMacs[pi].size(); ++ti)
                        {
                            if (peerMacs[pi][ti] != nullptr)
                            {
                                peerMacs[pi][ti]->PublishSelfEntry(
                                    sb.policy.EstX(), sb.policy.EstY(), sb.policy.Pxx(),
                                    sb.policy.Pyy(), sb.policy.Pxy(),
                                    spec.entryVelocity ? sb.policy.EstVx() : 0.0,
                                    spec.entryVelocity ? sb.policy.EstVy() : 0.0, t);
                            }
                        }
                        CaTblMac* tm = peerMacs[pi][static_cast<size_t>(act)];
                        tm->SendTriggeredUpdate(act, 1);
                        const uint32_t frames = 3; // CaTblMacConfig::triggeredUpdateRepeats
                        sb.energy.AccountFrameTx(
                            act, t, frames * tm->FrameAirtimeS(CA_TBL_MSG_TRIGGERED_UPDATE, act, 1));
                        ++trigSLarge;
                        trigSBilled += frames;
                        *trigS << std::fixed << std::setprecision(4) << t << ',' << sb.nodeId
                               << ",large_correction," << static_cast<int>(act) << ',' << moveS << ','
                               << thrS << ',' << sb.policy.EstX() << ',' << sb.policy.EstY() << ','
                               << pxS << ',' << pyS << ',' << std::setprecision(9)
                               << sb.energy.GetTotalJ() << ',' << frames << '\n';
                    }
                }
                // N.3 / row 55. --batteryDepleteJ's fault, as tag 0's: the cycle the subject's
                // budget is crossed, its radios fall silent (after one unrepeated LAST GASP on
                // its active radio under --subjectTriggeredUpdates, queued first and billed),
                // and from the next cycle it ranges, signals and relays nothing it originates.
                if (spec.subjectBatteryDeplete && !sb.batteryDead &&
                    sb.energy.GetTotalJ() >= batteryDepleteJ)
                {
                    sb.batteryDead = true;
                    sb.batteryDeadAtS = t;
                    CaTblMac* gm = peerMacs[pi][static_cast<size_t>(act)];
                    if (trigS && gm != nullptr)
                    {
                        gm->SendTriggeredUpdate(act, 2);
                        sb.energy.AccountFrameTx(act, t,
                                                 gm->FrameAirtimeS(CA_TBL_MSG_TRIGGERED_UPDATE, act, 1));
                        ++trigSGasp;
                        *trigS << std::fixed << std::setprecision(4) << t << ',' << sb.nodeId
                               << ",last_gasp," << static_cast<int>(act) << ",,," << sb.policy.EstX()
                               << ',' << sb.policy.EstY() << ",,," << std::setprecision(9)
                               << sb.energy.GetTotalJ() << ",1\n";
                    }
                    for (size_t ti = 0; ti < peerMacs[pi].size(); ++ti)
                    {
                        if (peerMacs[pi][ti] != nullptr)
                        {
                            peerMacs[pi][ti]->SetSilent(true);
                        }
                    }
                }
                // The C4 terms, as tag 0's end-of-cycle update.
                sb.pcsDegradation = CaTblDegradationWith(sb.pcsHist, ds.pcs, sb.pcsDegradation);
                sb.pcsHist.push_back(ds.pcs);
                while (sb.pcsHist.size() > 5)
                {
                    sb.pcsHist.pop_front();
                }
                sb.batteryUsedFrac = CaTblBatteryUsedFrac(sb.energy.GetTotalJ(), arbBatteryCapJ);
                // Listen and sleep, as tag 0's.
                for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
                {
                    const CaTblTier tr = static_cast<CaTblTier>(k);
                    CaTblMac* mm = peerMacs[pi][static_cast<size_t>(k)];
                    const bool awake = (tr == act) || (floorOn && tr == CA_TBL_TIER_868);
                    if (mesh)
                    {
                        const double air = mm->GetRebroadcastAirtimeS();
                        const uint64_t fr = mm->GetRebroadcastFrames();
                        if (fr > ex.meshBilledFrames[k])
                        {
                            sb.energy.AccountTx(tr, air - ex.meshBilledAir[k],
                                                fr - ex.meshBilledFrames[k]);
                            ex.meshBilledAir[k] = air;
                            ex.meshBilledFrames[k] = fr;
                        }
                    }
                    const uint64_t rxNow = mm->GetTotals().received;
                    if (spec.deadBatteryNoDraw && sb.batteryDead)
                    {
                        // N.20: a dead radio draws nothing -- no reception, no search
                        // window, no sleep current.
                        ++deadNoDrawS;
                    }
                    else if (awake)
                    {
                        const uint64_t delivered = rxNow - sb.lastRx[k];
                        if (delivered > 0)
                        {
                            sb.energy.AccountListen(tr, static_cast<double>(delivered) *
                                                            sb.beaconToaS[k]);
                        }
                        if (mm->GetNeighbourCount(tr, t) == 0)
                        {
                            const double window = std::min(dt, beaconPeriodS[k]) * searchDutyFrac;
                            if (window > 0.0)
                            {
                                sb.energy.AccountListen(tr, window);
                            }
                        }
                        else
                        {
                            sb.lastNonEmptyAtS[k] = t;
                        }
                    }
                    else
                    {
                        sb.energy.AccountSleep(tr, dt);
                        mm->ClearNeighbours();
                    }
                    sb.lastRx[k] = rxNow;
                    if (mesh)
                    {
                        const bool floorNext = hybridS && ds.nextTier != CA_TBL_TIER_868;
                        mm->SetAsleep(!(k == static_cast<int>(ds.nextTier) ||
                                        (floorNext && k == CA_TBL_TIER_868)));
                    }
                }
                *ex.trace << std::fixed << std::setprecision(4) << t << ',' << sb.nodeId << ','
                          << CaTblTierName(ds.activeTier) << ',' << CaTblTierName(ds.nextTier)
                          << ',' << CaTblTierName(fixTierS) << ',' << (usedFloorS ? 1 : 0) << ','
                          << sb.pos.x << ',' << sb.pos.y << ',' << ds.estX << ',' << ds.estY << ','
                          << errS << ',' << ds.pcs << ',' << ds.traceSigma << ','
                          << ds.nAnchorsUsed << ',' << (ds.hadFix ? 1 : 0) << ','
                          << (ds.escalated ? 1 : 0) << ',' << (ds.deescalated ? 1 : 0) << ','
                          << std::setprecision(9) << sb.energy.GetTotalJ() << '\n';
            }
        }

        const CaTblTier active = policy.GetActiveTier();

        // ONE NODE, THREE RADIOS, ONE BODY OF KNOWLEDGE.
        //
        // Each tier owns its own CaTblMac and therefore its own table, so an entry
        // learned on 868 was invisible to a lookup made while up on UWB. That is an
        // artefact of the interface split, not a property of the node: the header
        // calls the shared table "knowledge, not a work list", and knowledge does not
        // stop being true because a different radio heard it. Reading the union is
        // free -- it is a read, nothing goes on the air -- and it is what a single
        // node with three interfaces actually knows.
        //
        // Freshest wins, by the reporter's own timestamp rather than by arrival: two
        // radios may both hold an entry for the same node from different digests, and
        // the useful one is the later OBSERVATION, not the later delivery.
        //
        // The one-hop neighbour table is deliberately NOT unioned. It drives the poll
        // set, and merging it across tiers would have the node calling peers a
        // different radio heard -- burning a receive timeout every cycle on a link
        // that cannot close. That is the distinction the header draws, kept.
        auto freshestEntry = [&](uint16_t id, CaTblTableEntry& out) -> bool {
            bool found = false;
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                CaTblTableEntry e;
                if (nodeMac[ti] != nullptr && nodeMac[ti]->GetTableEntry(id, e) &&
                    e.valid && (!found || e.timestampS > out.timestampS))
                {
                    out = e;
                    found = true;
                }
            }
            return found;
        };

        // THE EXCHANGE RUNS BOTH WAYS.
        //
        // Only searchers used to ask. A finder broadcasts a digest, every node that
        // hears it answers with its own entry, and that is why 4 of 4 finders learn
        // where the casualty is -- and why the casualty learns nothing about them.
        // The responder half has always been identity-agnostic (ca-tbl-mac.cc:909),
        // so nothing new goes on the wire here: the casualty simply asks the
        // question its searchers were already asking.
        //
        // ON THE ACTIVE TIER, and that is not a detail. Sending on 868 while the
        // node is up on UWB would charge a tier outside this cycle's transmission
        // and break the one invariant the energy axis rests on (CLAUDE.md sect 6).
        // Asking on the tier already transmitting is the same discipline C5 uses for
        // a relayed exchange. It also happens to be the only choice that WORKS: each
        // tier owns its own CaTblMac and therefore its own table, so an entry learned
        // on 868 is invisible to a UWB ranging lookup. Asking on the active tier puts
        // the answer in exactly the table the ranging path will read.
        //
        // Phase-offset by half a period so the casualty's question does not land in
        // lockstep with the finders' every time and contend with the answers.
        if (bidirectionalDigest && sendHandshakeFrames && finderDigestPeriodS > 0.0 &&
            nodeMac[active] != nullptr)
        {
            const double phase = finderDigestPeriodS * 0.5;
            const double prevT = t - dt;
            const double ka = std::floor((prevT - phase) / finderDigestPeriodS);
            const double kb = std::floor((t - phase) / finderDigestPeriodS);
            if (kb > ka && t >= phase)
            {
                nodeMac[active]->Send(CA_TBL_MSG_DIGEST, active, 1, 0);
                ++casualtyDigests;
            }
        }

        // Did the answers arrive? Counted every cycle against the table the ranging
        // path actually reads, so "the mechanism fired" and "the entry was there to
        // fire on" are separate observations.
        if (!finderIds.empty())
        {
            uint32_t held = 0;
            for (size_t fi = 0; fi < finderIds.size(); ++fi)
            {
                CaTblTableEntry fe;
                if (freshestEntry(static_cast<uint16_t>(finderIds[fi]), fe))
                {
                    ++held;
                    finderEntryAgeSum += (t - fe.timestampS);
                    ++finderEntryAgeN;
                }
            }
            finderEntryHeldSum += held;
            ++finderEntryCycles;
        }

        // ---- Which radios this arm physically has ---------------------------
        //
        // Tier 1 runs CONTINUOUSLY on the hybrid arms. The design specifies a field
        // node that "runs tier 1 continuously and escalates to tier 2 or 3 only when
        // its position confidence justifies the energy", i.e. tier 1 is a coverage
        // FLOOR, not one of three mutually exclusive choices. The previous loop
        // ranged against the active tier only, with no within-cycle fallback, and the
        // consequence was measured: static_868 recorded 0 nofix cycles out of 1440
        // while ca_tbl_full recorded 1070. A hybrid arm cannot fix less often than
        // the pure tier-1 arm if tier 1 is always on. It was dead-reckoning 74% of
        // steps, and its RMSE was therefore not a localisation result.
        //
        // The static arms do NOT get this. static_2p4 and static_uwb are single-band
        // baselines -- the point of the comparison is that they have one radio. Giving
        // them a concurrent 868 radio would make them hybrids and destroy claim 1's
        // meaning. static_868's active tier is already tier 1.
        const bool hybridArm = (mode == CA_TBL_MODE_FULL ||
                                mode == CA_TBL_MODE_NO_INTEGRITY ||
                                mode == CA_TBL_MODE_FIXED_THRESHOLD);
        // WAKEFULNESS, not ranging. This flag gates `wasAwake` below, which decides
        // whether the tier-1 radio listens (and is charged for it) and -- critically
        // -- whether its neighbour table survives. A tier that is not awake has
        // ClearNeighbours() called on it every cycle. Left unconditional
        // deliberately: see FloorRangingWanted().
        // S7-lite fault window: a contiguous fraction of the run, placed
        // deterministically from the seed so it is reproducible and identical across
        // arms at a given seed -- the arms must see the SAME fault or the ablation
        // measures the fault placement rather than the gate.
        const bool biasActive =
            (injectBiasFraction > 0.0) &&
            (t >= biasStartS) && (t < biasStartS + injectBiasFraction * durationS);
        if (linkFault)
        {
            // SY-5. "Statistically clean, physically wrong": the node's own link budget
            // drops by the injected amount over the SAME seeded window as the range bias,
            // and nothing in the per-link statistics announces it. Tier 1 is RSSI
            // inversion against the NOMINAL transmit power, so a drop of D dB reads as D
            // dB of extra path loss and the range comes out long by 10^(D / (10 n)), n the
            // 868 MHz path-loss exponent -- the single-slope form the ranging submodule
            // uses for its own loss bias. Tiers 2 and 3 range by time of flight: there the
            // drop costs SNR and links, not bias. Magnitudes are OURS; no draw is taken.
            linkFault->windowStartS = biasStartS;
            linkFault->windowEndS = biasStartS + injectBiasFraction * durationS;
            linkFault->activeDb =
                biasActive ? spec.injectTxPowerDropDb + spec.injectAntennaLossDb : 0.0;
            linkFault->skew =
                biasActive ? std::pow(10.0, linkFault->activeDb / (10.0 * linkFault->n)) : 1.0;
            provider.SetNodeFaultDb(biasActive ? spec.injectTxPowerDropDb : 0.0,
                                    biasActive ? spec.injectAntennaLossDb : 0.0);
        }

        const bool runFloor = hybridArm && (active != CA_TBL_TIER_868);

        // FLOOR RANGING ON DEMAND -- the P1 mechanism, off by default.
        //
        // MEASURED (report/ROOTCAUSE_ENVELOPE.md Q1, boxHalf=1500, 20 seeds):
        // runFloor is true on 99.9% of cycles and j_868 = 15.35 J = 54.5% of arm
        // energy, while the floor supplies the fix on 1.16% of cycles and tier 1 is
        // the ACTIVE tier on 1.9 of 1440. It is not a defect -- it is an unexamined
        // design choice whose cost had never been measured.
        //
        // WHY RANGING AND NOT THE WHOLE FLOOR. Suppressing runFloor itself would put
        // tier 1 to sleep, and the sleep branch calls ClearNeighbours() outright --
        // not TTL expiry, an immediate wipe. The next cycle that needed the floor
        // would find an empty table, rangeTier() would return early without polling,
        // and the floor would deliver zero ranges exactly when required. The cost is
        // separable and only the larger part is recoverable: at boxHalf=1500,
        // tx_868 = 12.095 J (78.8%) is the ranging exchange and rx_868 = 3.183 J
        // (20.7%) is the listening that keeps the table warm. So the radio stays
        // awake and only the exchange is conditional.
        //
        // REACTIVE, NOT PREDICTIVE. activeMeas is populated before this is consulted
        // in both branches above, so the rule reads THIS cycle's outcome: range the
        // floor iff the active tier has already failed to deliver a solvable set.
        // A shortfall is therefore covered on the cycle it occurs, with zero added
        // coasting and no prediction to be wrong about.
        //
        // K = 1 is primary. Under a reactive rule with the table kept warm there is
        // no cold resume to bridge and no oscillation to damp, so hysteresis has no
        // named benefit; K > 1 is exposed only as a sensitivity check. Heartbeat
        // defaults OFF for the same reason -- with the radio awake it no longer
        // serves table warming, and its remaining purpose (bounding EKF drift) is
        // real but unquantified, so it must not carry the claim.
        const uint32_t floorHealthyRun = floorHealthyStreak;
        auto FloorRangingWanted = [&](size_t activeDelivered) -> bool {
            if (!floorOnDemand)
            {
                return true; // pre-existing behaviour, byte-identical
            }
            if (activeDelivered < 3)
            {
                return true; // reactive: the active tier did not solve this cycle
            }
            if (floorHeartbeatN > 0 && (cycleIndex % floorHeartbeatN) == 0)
            {
                return true;
            }
            return floorHealthyRun + 1 < floorSkipK; // K = 1 => never blocks
        };
        bool floorRanged = false;

        // Range against one tier and return its measurement set. Charges CAD and the
        // ranging exchange, including the RX-timeout cost of anchors that did not
        // answer -- which is why escalating into a tier that is out of range is
        // expensive rather than merely useless.
        auto rangeTier = [&](CaTblTier tier) {
            std::vector<CaTblRangeMeasurement> out;
            size_t droppedT = 0; // N.21 strict: finder replies whose position tag 0 lacks
            // A DEAD BATTERY STOPS THE RANGING, NOT JUST THE CHATTER.
            //
            // SetSilent() suppresses MAC frames -- beacons, digests, handshakes -- and
            // ranging does not go through them: rangeTier calls the engine directly.
            // Silencing alone therefore left a "dead" node still ranging and still
            // fixing, which is worse than not modelling the fault at all, because the
            // run reports a depletion that changed almost nothing. Measured before this:
            // fixCycles 479 -> 474 across a 528 s outage, and totalJ kept climbing past
            // the budget that was supposed to have ended it.
            //
            // A ranging exchange is a transmission. A radio with no power does not make
            // one, so no exchange forms, no energy is charged, and the filter coasts --
            // which is the whole consequence the fault exists to expose.
            if (batteryDead)
            {
                return out;
            }
            rangedThisCycle.clear();

            // THE POLL SET IS THE ONE-HOP NEIGHBOUR TABLE, NOT THE DEPLOYMENT.
            //
            // This one substitution is the §6 fix. Previously every anchor of the
            // tier was addressed every cycle -- including, on UWB, the 15 anchors
            // sited at the three facilities the node was not at, kilometres away.
            // The energy model correctly charges the full rxTimeoutS for each
            // non-responder (0.93 mJ on UWB, about 3x a SUCCESSFUL exchange), so
            // total energy tracked anchor count: 9 / 12 / 20 anchors gave
            // 12.19 / 16.44 / 28.35 J. totalJ is the denominator of every
            // accuracy-per-joule figure, so claims 1 and 2 both rested on it.
            //
            // A node cannot address a peer it has never heard from. It ranges
            // against what it has discovered, which is what this now does.
            std::vector<CaTblNeighbour> nb = nodeMac[tier]->GetNeighbours(tier, t);
            if (mesh)
            {
                // NW-0. Under mesh links a peer's frames reach the node and the peer enters
                // the neighbour table -- but the poll is to ANCHORS (gateways and finders)
                // with known positions. operator[] below would otherwise range a peer as a
                // free "anchor" at (0, 0, 0). Peers are ranged only through C5 (relays).
                std::vector<CaTblNeighbour> keep;
                for (size_t k = 0; k < nb.size(); ++k)
                {
                    if (nb[k].id >= 101 && anchorPosById.find(nb[k].id) != anchorPosById.end())
                    {
                        keep.push_back(nb[k]);
                    }
                    else
                    {
                        ++mesh->peersSkipped;
                    }
                }
                nb.swap(keep);
            }

            energy.AccountCad(tier);

            if (nb.empty())
            {
                // Nothing discovered on this tier this cycle: no poll is issued and
                // no RX timeouts are burnt, because none were physically incurred.
                // Note the incentive this creates and do NOT tune it away -- it
                // makes escalating into undiscovered space cheap-but-useless
                // rather than expensive-and-useless, which weakens the energy
                // penalty the hysteresis parameters were supposed to be earning
                // their keep against. Whether the policy still behaves is now an
                // empirical question with a different answer than before.
                return out;
            }
            out.reserve(nb.size());

            for (size_t a = 0; a < nb.size(); ++a)
            {
                const uint16_t anchorId = nb[a].id;
                const Vector& anchorPos = anchorPosById[anchorId];
                ++directEngineCalls;

                bool received = false;
                double range = 0.0;
                bool nlos = false;
                // Both candidate populations are captured on every link whichever
                // source is operative, so the trace can compare them directly.
                bool nlosProp = false;
                bool hiddenActive = false;
                double sigmaM = 0.0;
                // --nlosClassifier: this link's tier-specific NLOS_score, or -1 (unscored,
                // and always -1 with the flag off). Scored only on the links pushed below,
                // i.e. under the same received/finite/positive test; a polled link that
                // fails it is counted as lost by its modelled truth.
                double nlosScore = -1.0;

                if (tier == CA_TBL_TIER_868)
                {
                    PhysicsResult r = engine868.ProcessTransmission(
                        nodeId,
                        anchorId,
                        pos,
                        anchorPos,
                        (linkFault && linkFault->activeDb > 0.0)
                            ? phy[0].txPowerDbm - linkFault->activeDb
                            : phy[0].txPowerDbm,
                        phy[0].sf,
                        phy[0].bandwidthHz,
                        0,
                        now);
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    if (linkFault && linkFault->activeDb > 0.0)
                    {
                        ++linkFault->rangingLinks;
                        if (received && std::isfinite(range) && range > 0.0)
                        {
                            range *= linkFault->skew; // SY-5: the RSSI skew, see above
                            ++linkFault->skewedLinks;
                        }
                    }
                    nlos = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
                    nlosProp = r.isNLOS;
                    hiddenActive = (r.hiddenChannelState > 0);
                    sigmaM = r.rangingSigmaM;
                    // --tier1Tdoa: a gateway that decoded this broadcast stamps it, on
                    // exactly the links whose reply is billed below (the same test).
                    if (tdoa && received && std::isfinite(range) && range > 0.0)
                    {
                        tdoa->Stamp(anchorId, anchorPos, pos, t, r.finalSNRDb, r.isNLOS,
                                    r.delaySpreadNs, nlos);
                    }
                    if (nlosScorer)
                    {
                        if (received && std::isfinite(range) && range > 0.0)
                        {
                            nlosScore = nlosScorer->Score868(anchorId, t, cycleIndex,
                                                             CalculateDistance(pos, anchorPos), r);
                        }
                        else
                        {
                            nlosScorer->NoteLost(0, r.isNLOS);
                        }
                    }
                }
                else if (tier == CA_TBL_TIER_2P4)
                {
                    PhysicsResult2p4 r = engine2p4.ProcessTransmission(
                        nodeId,
                        anchorId,
                        pos,
                        anchorPos,
                        (linkFault && linkFault->activeDb > 0.0)
                            ? phy[1].txPowerDbm - linkFault->activeDb
                            : phy[1].txPowerDbm,
                        phy[1].sf,
                        phy[1].bandwidthHz,
                        0,
                        now);
                    if (linkFault && linkFault->activeDb > 0.0)
                    {
                        ++linkFault->rangingLinks;
                    }
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    nlos = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
                    nlosProp = r.isNLOS;
                    hiddenActive = (r.hiddenChannelState > 0);
                    sigmaM = r.rangingSigmaM;

                    // S7-lite: COMMON-MODE RANGE BIAS on tier 2, the minimal viable
                    // test of the integrity gate's actual claim.
                    //
                    // The gate claims to catch a fix that is STATISTICALLY CLEAN but
                    // PHYSICALLY WRONG. Every fault already in the model announces
                    // itself through the NLOS flag or through sigma, and `suspect`
                    // requires ALL-LOS -- so a fault that trips the NLOS flag is
                    // invisible to the gate by construction and would test nothing.
                    //
                    // This bias therefore does NOT touch nlos, nlosProp or sigmaM. It
                    // shifts every tier-2 range by the same amount, which is exactly
                    // the failure a filter-free multilateration check should see as a
                    // continuity violation while the per-link statistics stay healthy.
                    // Common-mode rather than per-link because an independent
                    // per-link error is what sigma already models; a shared offset is
                    // what it does not.
                    // F1 vs F3. injectFaultAnchors = 0 means COMMON-MODE (every tier-2
                    // link, h = m). >0 means a SINGLE-ANCHOR fault (h = 1), which is the
                    // canonical RAIM model and the physically realistic one: an anchor
                    // displaced, drifting, detuned, or with a blocked path. The faulted
                    // anchor is chosen deterministically from the seed so it is
                    // reproducible and identical across arms -- the ablation must see the
                    // same fault, or it measures fault placement rather than the detector.
                    if (injectRangeBiasM != 0.0 && biasActive)
                    {
                        const bool hitThisAnchor =
                            (injectFaultAnchors == 0) ||
                            ((anchorId % 97u) % 3u == (seed % 3u));
                        if (hitThisAnchor)
                        {
                            range += injectRangeBiasM;
                        }
                    }
                    // Scored on the range as the node has it, injected bias included:
                    // the cross-check reads the observable, not the engine's truth.
                    if (nlosScorer)
                    {
                        if (received && std::isfinite(range) && range > 0.0)
                        {
                            nlosScore = nlosScorer->Score2p4(anchorId, t, cycleIndex,
                                                             CalculateDistance(pos, anchorPos),
                                                             range, phy[1].txPowerDbm, r);
                        }
                        else
                        {
                            nlosScorer->NoteLost(1, r.isNLOS);
                        }
                    }
                }
                else
                {
                    PhysicsResultUWB r = engineUwb.ProcessTransmission(
                        nodeId,
                        anchorId,
                        pos,
                        anchorPos,
                        (linkFault && linkFault->activeDb > 0.0)
                            ? phy[2].txPowerDbm - linkFault->activeDb
                            : phy[2].txPowerDbm,
                        phy[2].sf,
                        phy[2].bandwidthHz,
                        0,
                        now);
                    if (linkFault && linkFault->activeDb > 0.0)
                    {
                        ++linkFault->rangingLinks;
                    }
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    nlos = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
                    nlosProp = r.isNLOS;
                    hiddenActive = (r.hiddenChannelState > 0);
                    sigmaM = r.rangingSigmaM;
                    if (nlosScorer)
                    {
                        if (received && std::isfinite(range) && range > 0.0)
                        {
                            nlosScore = nlosScorer->ScoreUwb(anchorId, t, cycleIndex,
                                                             CalculateDistance(pos, anchorPos), r);
                        }
                        else
                        {
                            nlosScorer->NoteLost(2, r.isNLOS);
                        }
                    }
                }

                if (received && std::isfinite(range) && range > 0.0)
                {
                    rangedThisCycle.push_back(std::make_pair(anchorId, range));
                    CaTblRangeMeasurement m;
                    m.anchorX = anchorPos.x;
                    m.anchorY = anchorPos.y;
                    // A FINDER IS A PSEUDO-ANCHOR. It knows where it is to within its
                    // own GPS error, so the filter gets the position it REPORTS and a
                    // variance inflated by that position's uncertainty along the range
                    // direction -- the same treatment, and the same formula, C5 applies
                    // to any relayed measurement. Handing over its true position would
                    // credit it with knowledge no node has.
                    //
                    // The physics above is untouched: the radio link is physical and
                    // does not care what anyone believes about where they are.
                    bool dropT = false;   // N.21 strict: no entry held, so not used
                    bool subbedT = false; // N.21 strict: the report replaced the truth
                    if (finderPseudoAnchor && anchorId >= 501)
                    {
                        CaTblTableEntry fe;
                        if (!freshestEntry(anchorId, fe))
                        {
                            dropT = spec.finderPseudoAnchorStrict;
                        }
                        else
                        {
                            // DC-13. Under --entryPrediction the entry is first carried
                            // forward to now: position + velocity x age (DC-10's velocity),
                            // and a source-type Q in place of the walk-speed bound below.
                            // For a finder (the only kind substituted here) Q is the
                            // error of a GNSS-derived velocity, 0.5 m/s, OURS: DC-10 takes
                            // it as displacement over the publication interval, which a
                            // walker's turns corrupt well beyond the GNSS fixes' own error.
                            // Gateways are surveyed and never substituted (Q = 0).
                            double fePredVar = -1.0;
                            if (entryPred)
                            {
                                const double ageP = std::max(0.0, t - fe.timestampS);
                                const double x0 = fe.x, y0 = fe.y;
                                fe.x += fe.vx * ageP;
                                fe.y += fe.vy * ageP;
                                const double sv = 0.5 * ageP;
                                fePredVar = sv * sv;
                                const double legacy = walkSpeedMps * ageP;
                                ++entryPred->subs;
                                if (fe.x != x0 || fe.y != y0)
                                {
                                    ++entryPred->moved;
                                }
                                entryPred->log << std::fixed << std::setprecision(4) << t << ','
                                               << anchorId << ',' << fe.seqNo << ',' << ageP
                                               << ',' << std::setprecision(6) << x0 << ','
                                               << y0 << ',' << std::setprecision(9) << fe.vx
                                               << ',' << fe.vy << ',' << std::setprecision(6)
                                               << fe.x << ',' << fe.y << ',' << fePredVar
                                               << ',' << legacy * legacy << '\n';
                            }
                            const double ux = policy.EstX() - fe.x;
                            const double uy = policy.EstY() - fe.y;
                            const double un = std::sqrt(ux * ux + uy * uy);
                            if (un > 1e-6)
                            {
                                // AGE IS PART OF THE UNCERTAINTY, and leaving it out
                                // is not conservative -- it is wrong in the dangerous
                                // direction. The entry carries the variance the
                                // reporter had WHEN IT REPORTED, a couple of metres of
                                // GNSS error. The position is as old as the last
                                // digest that reached us, and a searcher walks the
                                // whole time. Handing the filter a several-minute-old
                                // point under a two-metre variance tells it to trust
                                // an anchor that is hundreds of metres from where it
                                // now stands. Measured before this was added: 238
                                // substitutions on entries averaging 242 s old drove
                                // rmse from 7.4 m to 86.7 m.
                                //
                                // Bounded by how far the reporter COULD have gone:
                                // walkSpeedMps is the searcher speed this deployment
                                // assumes, so speed x age is a travel bound and its
                                // square is the variance that bound implies. Isotropic
                                // because a bound carries no direction -- the entry's
                                // velocity fields are not populated, and inventing a
                                // heading from a stale fix would claim more than is
                                // known.
                                //
                                // This also makes A3 do its job. The admission test
                                // refuses a relay whose position uncertainty is large
                                // against the range being measured, and staleness is
                                // exactly such an uncertainty -- so an entry too old
                                // to use is now rejected by the existing guard rather
                                // than needing a staleness cutoff of its own.
                                const double ageS = std::max(0.0, t - fe.timestampS);
                                const double travel = walkSpeedMps * ageS;
                                const double ageVar = (fePredVar >= 0.0) ? fePredVar
                                                                         : travel * travel;
                                const double fp2[2][2] = {{fe.varX + ageVar, fe.covXY},
                                                          {fe.covXY, fe.varY + ageVar}};
                                const double proj =
                                    CaTblProjectRelayCovariance(fp2, ux / un, uy / un);
                                if (proj >= 0.0)
                                {
                                    m.anchorX = fe.x;
                                    m.anchorY = fe.y;
                                    const double inflated =
                                        std::sqrt(sigmaM * sigmaM + proj);
                                    relayedVarThisCycle += inflated * inflated;
                                    sigmaM = inflated;
                                    ++pseudoAnchorSubs;
                                    subbedT = true;
                                }
                            }
                            // N.21 strict: where the variance could not be projected (the
                            // estimate on the report itself), the report is still the
                            // position, uninflated -- never the truth.
                            if (spec.finderPseudoAnchorStrict && !subbedT)
                            {
                                m.anchorX = fe.x;
                                m.anchorY = fe.y;
                                ++strictNoInflate;
                            }
                        }
                        if (strictLog)
                        {
                            *strictLog << std::fixed << std::setprecision(4) << t << ',' << anchorId
                                       << ',' << (dropT ? "dropped" : (subbedT ? "report" : "report_noinflate"))
                                       << ',' << (dropT ? anchorPos.x : m.anchorX) << ','
                                       << (dropT ? anchorPos.y : m.anchorY) << ',' << anchorPos.x << ','
                                       << anchorPos.y << '\n';
                            if (dropT)
                            {
                                ++strictDropped;
                            }
                            else if (subbedT)
                            {
                                ++strictReport;
                            }
                        }
                    }
                    m.rangeM = range;
                    m.nlos = nlos;
                    m.nlosPropagation = nlosProp;
                    m.hiddenChannelActive = hiddenActive;
                    m.sigmaM = sigmaM;
                    m.nlosScore = nlosScore;
                    // DC-11: who the range is to, and whether this is the floor's set
                    // (the floor is the only tier ranged other than the active one).
                    m.anchorId = anchorId;
                    m.sourceKind = CaTblSourceKindOf(anchorId);
                    m.fromFloor = (tier != active);
                    noteUwbEvidence(anchorId, nodeId, tier, range, t); // NW-5
                    if (dropT)
                    {
                        ++droppedT; // received and billed, not used
                    }
                    else
                    {
                        out.push_back(m);
                    }
                }
            }

            // nAnchorsAddressed is now the DISCOVERED count. An entry that has gone
            // stale within its TTL but is no longer reachable still costs a full RX
            // timeout here, which is correct and is the price of the TTL being
            // finite rather than clairvoyant.
            energy.AccountRangingExchange(tier,
                                          static_cast<uint32_t>(nb.size()),
                                          static_cast<uint32_t>(out.size() + droppedT),
                                          t);
            // SY-1 (TECH_DEBT L.2.3). With the estimator on the gateways, a tier-2 or
            // tier-3 range has to REACH them, and nothing billed that. Tiers 2 and 3 range
            // by two-way time of flight, which leaves the range at the initiator -- the
            // node (Poucet's START/ACK/DATA_REPLY, docx Physical 1.3) -- so the node sends
            // it up: one frame on the tier that just ranged, carrying one record per
            // answered anchor. A record is anchor id (2 B) + range (2 B, decimetres) +
            // sigma (1 B) + NLOS score (1 B) = 6 B, OURS, shaped on the table entry's
            // fields. The 868 MHz exchange needs no uplink: the gateways measured it.
            // The tier transmitted this cycle, so CLAUDE.md sect 6 holds.
            if (edgeUp && gatewayEdgeServer && tier != CA_TBL_TIER_868 && !out.empty())
            {
                const uint32_t bytes = CA_TBL_MAC_HEADER_BYTES +
                                       6u * static_cast<uint32_t>(out.size());
                const double airS =
                    (tier == CA_TBL_TIER_UWB)
                        ? CaTblEnergyModel::UwbFrameTimeS(bytes)
                        : CaTblEnergyModel::LoRaTimeOnAirS(phy[tier].sf, phy[tier].bandwidthHz,
                                                           bytes, 1);
                energy.AccountFrameTx(tier, t, airS);
                ++edgeUp->frames[tier];
                edgeUp->records[tier] += out.size();
                edgeUp->airS[tier] += airS;
            }
            if (tdoa && tier == CA_TBL_TIER_868)
            {
                tdoa->NoteExchange(out.size());
            }
            return out;
        };

        // ---- Run the radios this arm has, in this cycle ---------------------
        // How many anchors the active tier ADDRESSED, as distinct from how many
        // answered. Zero means the node had discovered nothing on that tier and did
        // not transmit at all -- a state that could not exist before, because the
        // poll was unconditional.
        const size_t activeMeasAddressed = nodeMac[active]->GetNeighbourCount(active, t);

        // ---- ARBITRATION: pre-empt ranging for a pending handshake ----------
        //
        // When the previous cycle's escalation was deferred because the tier-1
        // duty budget was exhausted, and arbitrateDutyBudget is on, this cycle
        // gives up its ranging exchange on the tier whose budget the handshake
        // needs.  The freed budget means CanTransmit() returns true when Step()
        // re-decides to escalate, and the normal handshake flow commits the
        // tier change.
        //
        // Rationale (derivable, not asserted): a node escalates precisely because
        // the current tier's fix quality is inadequate.  Spending this cycle's
        // one transmission on another inadequate fix, rather than on the handoff
        // that would end the inadequacy, is strictly dominated.  The arbiter's
        // ordering falls out of the policy's own premise.
        std::vector<CaTblRangeMeasurement> activeMeas;
        std::vector<CaTblRangeMeasurement> floorMeas;
        // RESET BEFORE THE FIRST CONTRIBUTOR, not between two of them.
        //
        // This accumulator has two sources: the pseudo-anchor substitution inside
        // rangeTier(), and the C5 relayed exchange below. It used to be cleared
        // between them -- after the ranging calls and before the relay block -- which
        // was correct while C5 was the only contributor and silently wrong once
        // 937a55a added the first. Every substitution's variance was zeroed before
        // policy.SetRelayedVarFraction() could read it, so the share was always 0.00
        // and the hop-provenance term had no input no matter how often it fired.
        //
        // MEASURED: 1513 substitutions in a 240-cycle run, and identical output with
        // --hopProvenanceForm 0 and 1. A mechanism that cannot change its own output
        // is not enabled, whatever its flag says.
        relayedVarThisCycle = 0.0;
        // --tier1Tdoa: stamps belong to this cycle's tier-1 exchange and to no other.
        if (tdoa)
        {
            tdoa->BeginCycle();
        }

        if (pendingHandshake && arbitrateDutyBudget)
        {
            // Pre-empt: the tier change committed last cycle but the handshake
            // energy was not charged (the tier-1 budget was exhausted by ranging).
            // This cycle surrenders the ranging exchange on pendingHandshakeTier
            // -- the CAPPED tier being left, where the handshake must be sent --
            // and charges the handshake there instead.
            //
            // IT MUST BE THAT TIER AND NOT THE ACTIVE ONE. Once the tier change
            // has committed, `active` is the NEW, higher tier, and tiers 2 and 3
            // carry no duty-cycle cap in this model, so surrendering their
            // ranging frees nothing the handshake can use.
            //
            // MEASURED, before this was keyed correctly (results/v6_ladder,
            // seed 7): excess tier-1 airtime over static_868 equalled
            // handshakePreemptions x 41.22 ms to four decimal places on all three
            // escalating arms -- the handshake was charged strictly ON TOP OF a
            // full tier-1 ranging load. ca_tbl_full reached duty_868 = 2.5080%
            // against the WPC G.S.R. 853(E) Table-II 2.5% cap while holding tier 1
            // for only 362 of 1440 cycles, and the verifier failed the run. The
            // arbiter was sacrificing an uncapped transmission to make room on
            // the capped one.
            //
            // The cost is real and is still charged: the node loses this cycle's
            // tier-1 measurements. It keeps the new tier's, which is the point --
            // the tier-1 fix is the inadequate one the escalation exists to
            // escape, so it is the correct thing to surrender under the rationale
            // above.
            if (spec.deadBatteryNoDraw && batteryDead)
            {
                ++deadNoDrawHs; // N.20: a dead radio pays for no handshake
            }
            else
            {
                energy.AccountEscalationHandshake(pendingHandshakeTier, t);
            }
            // C1: the frame ITSELF, in addition to the charge above. The
            // accounting already sizes the handshake correctly; what it has
            // never done is contend for the medium, pass CAD, or meet the
            // duty governor as a frame. Adding rather than replacing keeps
            // the energy axis fixed, so a change here is attributable to
            // contention and to nothing else.
            if (sendHandshakeFrames && nodeMac[pendingHandshakeTier] != nullptr)
            {
                if (spec.handshakeFixes)
                {
                    // MC-1. This send carried no terms at all -- 0/0, whatever the node's
                    // state -- because SetRequestTerms was called at the other two sites
                    // only. pcsDegradation is current here: last updated at the end of
                    // the previous cycle, and no PCS has been computed since. Under the
                    // flag a pending handshake exists only for an escalation into UWB.
                    nodeMac[pendingHandshakeTier]->SetRequestTerms(
                        CaTblQuantise(pcsDegradation),
                        CaTblQuantise(ArbBatteryTerm(
                            spec.arbitrationV2,
                            CaTblBatteryUsedFrac(energy.GetTotalJ(), arbBatteryCapJ))));
                    nodeMac[pendingHandshakeTier]->SetRequestTargetTier(CA_TBL_TIER_UWB);
                }
                nodeMac[pendingHandshakeTier]->SetRequestQueryTriggered(pendQT[0] != 0); // NW-4
                nodeMac[pendingHandshakeTier]->Send(
                    CA_TBL_MSG_ESC_REQUEST, pendingHandshakeTier, 0, 0);
                ++tag0RequestFrames;
            }
            ++handshakePreemptions;
            pendingHandshake = false;

            // Ranging proceeds on every tier EXCEPT the one surrendering its slot.
            // rangeTier() charges CAD itself, so charge it directly for the
            // skipped tier: the node still listens even when it does not poll.
            if (active != pendingHandshakeTier)
            {
                activeMeas = rangeTier(active);
            }
            else
            {
                energy.AccountCad(active);
            }
            if (runFloor && FloorRangingWanted(activeMeas.size()))
            {
                if (pendingHandshakeTier != CA_TBL_TIER_868)
                {
                    floorMeas = rangeTier(CA_TBL_TIER_868);
                    floorRanged = true;
                }
                else
                {
                    energy.AccountCad(CA_TBL_TIER_868);
                }
            }
        }
        else
        {
            pendingHandshake = false;  // consumed or stale
            activeMeas = rangeTier(active);
            if (runFloor && FloorRangingWanted(activeMeas.size()))
            {
                floorMeas = rangeTier(CA_TBL_TIER_868);
                floorRanged = true;
            }
        }

        // Fix selection: prefer the escalated tier when it solved, fall back to the
        // tier-1 floor otherwise. Preference rather than fusion is deliberate -- it
        // keeps each cycle's fix attributable to exactly one tier, which the ablation
        // between arms depends on. Cross-tier fusion would produce a better number and
        // a worse experiment.
        // sect 7 CASE A. Two ranges are not nothing. Multilateration needs three
        // anchors and below that the estimator coasted; two circles cut the plane to two
        // points and the prior chooses between them.
        //
        // TRIED ON THE ACTIVE TIER, NOT THE FLOOR. The floor is 868 MHz and almost
        // always has three anchors already -- runFloor is true on 99.9% of cycles and
        // static_868 records zero no-fix cycles. The tier short of anchors is the one
        // the node escalated TO, tier 3 especially: UWB reaches ~13.5 m, so three
        // neighbours at once is rare and two is markedly less rare. That is where a
        // two-anchor solve is worth having, and it is the open question in TECH_DEBT H.1.
        //
        // FUSED AS A POSITION, NOT AS RANGES. Case A has already consumed both ranges to
        // produce the point; handing the same two ranges to the filter afterwards would
        // count one measurement twice.
        //
        // LAST RESORT, AND THIS WAS MEASURED THE WRONG WAY FIRST. Gating only on
        // "the active tier has two" let a two-anchor solve win the cycle from the
        // TIER-1 FLOOR, which usually has three anchors and solves better. Measured:
        // noFix 1 -> 13, rmse 16.88 -> 23.57 m, tier-3 cycles 5 -> 0. Case A must
        // replace COASTING, which is what the estimator does when nothing solves --
        // not replace a fallback that was already working.
        bool caseAUsed = false;
        if (activeMeas.size() >= 3)
        {
            // CASE D -- the ordinary overdetermined solve. Counted so the sparse cases
            // have a denominator: "Case A fired 8 times" means nothing without knowing
            // how often the normal path ran. NOT the spec's Case D (Physical 2.4D, one
            // range plus the IMU), which is --caseD (DC-4) and counted on its own DC-4
            // line; the label is kept so stdout stays comparable with every earlier run.
            ++caseSolved[0];
        }
        else if (floorMeas.size() >= 3)
        {
            // THE FIFTH BUCKET, AND IT IS THE ONE THAT WAS MISSING. The active tier came
            // up short but the TIER-1 FLOOR solved, so the cycle is localised and is
            // none of D, A, B or C: not D because the active tier had fewer than three,
            // not A or B because those only run once the floor has failed, not C because
            // the node was not unlocalisable.
            //
            // Without it the census summed to ~404-421 of 480 and looked like a
            // partition with cycles missing. It was never a partition; it was four
            // categories with a hole where the commonest outcome lived.
            ++caseSolved[4];
        }
        else if (activeMeas.size() == 2 && !sparseCaseA && !spec.sparseInStep)
        {
            // Two ranges, no floor, and sparse solving disabled: the estimator coasts.
            // Unlocalisable in the same sense as C, and counted there so the census does
            // not silently depend on whether --sparseCaseA happens to be on.
            ++caseSolved[3];
        }
        else if (activeMeas.size() == 0 || activeMeas.size() == 1)
        {
            // CASE C -- no usable absolute geometry. CaTblSolveCaseC() always refuses by
            // construction: with no surveyed anchor there is no absolute frame, and the
            // peer set fixes a SHAPE that can be translated and rotated freely without
            // contradicting any measurement. Counted rather than called, because calling
            // a function whose answer is known is theatre; what matters is that the
            // cycle is CLASSIFIED as unlocalisable rather than silently coasting.
            ++caseSolved[3];
        }
        if (spec.caseCRelative && floorMeas.empty() &&
            std::none_of(activeMeas.begin(), activeMeas.end(),
                         [](const CaTblRangeMeasurement& m) { return m.anchorId >= 101; }))
        {
            caseCPeers(0, pos, energy, nodeMac[static_cast<int>(active)], active, t, now, t); // DC-3
        }
        if (sparseCaseA && activeMeas.size() == 2 && floorMeas.size() < 3)
        {
            ++caseAAttempts;
            // A OR B IS DECIDED BY PROVENANCE, NOT BY GEOMETRY. The arithmetic is the
            // same two-circle intersection either way; what differs is what the second
            // reference IS. A surveyed gateway is known infrastructure. A finder or peer
            // is a node reporting its own estimate, and a fix that leaned on one is not
            // the evidence two gateways give -- so it must not be reported as though it
            // were. Ids below 500 are gateways; 500 and above are finders and peers.
            // Provenance is read from the POSITION, because CaTblRangeMeasurement does
            // not carry the id and rangedThisCycle holds the floor's ids by this point
            // (the floor is ranged after the active tier). A reference standing exactly
            // where a surveyed gateway stands IS that gateway.
            auto isSurveyed = [&](double ax, double ay) {
                for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
                {
                    for (size_t k = 0; k < dep.anchors[ti].size(); ++k)
                    {
                        const Vector& gp = dep.anchors[ti][k].pos;
                        if (std::fabs(gp.x - ax) < 0.01 && std::fabs(gp.y - ay) < 0.01)
                        {
                            return true;
                        }
                    }
                }
                return false;
            };
            const bool bothSurveyed = isSurveyed(activeMeas[0].anchorX, activeMeas[0].anchorY) &&
                                      isSurveyed(activeMeas[1].anchorX, activeMeas[1].anchorY);
            const CaTblSparseResult sr =
                bothSurveyed
                    ? CaTblSolveCaseA(activeMeas[0].anchorX, activeMeas[0].anchorY,
                                      activeMeas[0].rangeM,
                                      activeMeas[1].anchorX, activeMeas[1].anchorY,
                                      activeMeas[1].rangeM,
                                      policy.EstX(), policy.EstY(),
                                      policy.Pxx(), policy.Pxy(), policy.Pyy(),
                                      sparseCaseAMarginM)
                    : CaTblSolveCaseB(activeMeas[0].anchorX, activeMeas[0].anchorY,
                                      activeMeas[0].rangeM,
                                      activeMeas[1].anchorX, activeMeas[1].anchorY,
                                      activeMeas[1].rangeM,
                                      policy.EstX(), policy.EstY(),
                                      policy.Pxx(), policy.Pxy(), policy.Pyy(),
                                      sparseCaseAMarginM);
            if (sr.verdict == CA_TBL_SPARSE_OK)
            {
                // Conservative on purpose. A proper propagation would carry the geometry
                // through; a point standing on two circles is worse conditioned than a
                // three-anchor solve of the same sigmas, so the honest direction to err
                // is wide.
                const double s0 = activeMeas[0].sigmaM;
                const double s1 = activeMeas[1].sigmaM;
                policy.FusePosition(sr.x, sr.y, 2.0 * (s0 * s0 + s1 * s1));
                caseAUsed = true;
                ++caseAAdmitted;
                ++caseSolved[(sr.solvedBy == CA_TBL_CASE_B) ? 2 : 1];
            }
            else if (sr.verdict < 4)
            {
                ++caseARefused[sr.verdict];
                // A REFUSED SPARSE SOLVE IS AN UNLOCALISABLE CYCLE, and it belongs in C.
                // Leaving it uncounted is what made the census incomplete at 412 of 480:
                // the two-range cycles that Case A/B declined fell through every branch.
                // Counting the refusal as C is not bookkeeping convenience -- the node
                // genuinely ended the cycle without a position, which is what C means.
                ++caseSolved[3];
            }
        }

        if (sparse)
        {
            // DC-11. Classified after Step(), which now does the solving; noted before
            // the relays below append to activeMeas.
            sparse->twoNoFloor = (activeMeas.size() == 2 && floorMeas.size() < 3);
            sparse->prevX = policy.EstX();
            sparse->prevY = policy.EstY();
        }
        const bool activeSolved = (activeMeas.size() >= 3) || caseAUsed;
        const bool usedFloor = (!activeSolved && floorMeas.size() >= 3);

        // Streak bookkeeping for --floorSkipK. Reset on any shortfall so a single
        // failure re-arms the floor immediately; K = 1 makes this inert.
        floorHealthyStreak = activeSolved ? (floorHealthyStreak + 1) : 0;
        if (floorRanged)
        {
            ++floorRangedCycles;
        }
        // C5. Form a RELAYED measurement against each relay, admit it, and let the
        // admitted ones into this cycle's measurement set.
        //
        // THE §6 INVARIANT IS THE CONSTRAINT HERE, not an afterthought. Exactly one
        // tier transmits per cycle and only that tier is charged. So a relayed
        // exchange is formed ONLY when the relay's tier is already the active tier --
        // then it is one more exchange on a tier that was transmitting anyway, and it
        // is charged there. If the relay sat on a different tier, forming this would
        // make two tiers transmit in one cycle and the energy axis would stop meaning
        // anything.
        //
        // A1 also excludes tier 1, so this can only ever engage on 2.4 GHz or UWB.
        // Both conditions are checked rather than assumed, and the A1 rejection is
        // still counted when relayTier is forced to 0, because a condition that is
        // never exercised is not evidence of anything.
        if (relayedRanging && !usedFloor)
        {
            const CaTblTier rt = static_cast<CaTblTier>(
                std::min<uint32_t>(CA_TBL_TIER_COUNT - 1, relayTier));
            if (rt == active)
            {
                std::set<uint16_t> relaysUsedThisUpdate;
                for (uint32_t pt = 1; pt < nTags && pt < tags.size(); ++pt)
                {
                    const double bx = tags[pt].policy.EstX();
                    const double by = tags[pt].policy.EstY();
                    // The tag's OWN prior estimate, not its true position: a node does
                    // not know where it is, and using the truth here would leak the
                    // answer into the geometry test and the range direction.
                    const double ax = policy.EstX();
                    const double ay = policy.EstY();
                    const double ddx = ax - bx;
                    const double ddy = ay - by;
                    const double dist = std::sqrt(ddx * ddx + ddy * ddy);

                    // The tag-to-tag link. ProcessTransmission auto-registers an
                    // unknown receiver as a gateway with fineTimestamp=false, which is
                    // the correct hardware class for a tag.
                    double rAB = 0.0;
                    double sigRaw = 0.0;
                    bool got = false;
                    const int ati = static_cast<int>(active);
                    LegTerms::Obs obs; // NW-1 step 3
                    obs.tier = ati;
                    obs.txPowerDbm = phy[ati].txPowerDbm;
                    if (active == CA_TBL_TIER_2P4)
                    {
                        PhysicsResult2p4 r = engine2p4.ProcessTransmission(
                            nodeId, static_cast<uint16_t>(tags[pt].nodeId), pos,
                            tags[pt].pos, phy[ati].txPowerDbm, phy[ati].sf,
                            phy[ati].bandwidthHz, 1, now);
                        got = r.packetReceived;
                        rAB = r.estimatedRangeM;
                        sigRaw = r.rangingSigmaM;
                        obs.rssiDbm = r.measuredRssiDbm;
                        obs.snrDb = r.snr_db;
                    }
                    else if (active == CA_TBL_TIER_UWB)
                    {
                        PhysicsResultUWB r = engineUwb.ProcessTransmission(
                            nodeId, static_cast<uint16_t>(tags[pt].nodeId), pos,
                            tags[pt].pos, phy[ati].txPowerDbm, phy[ati].sf,
                            phy[ati].bandwidthHz, 1, now);
                        got = r.packetReceived;
                        rAB = r.estimatedRangeM;
                        sigRaw = r.rangingSigmaM;
                        obs.rssiDbm = r.measuredRssiDbm;
                        obs.snrDb = r.snr_db;
                        obs.truthNlos = r.isNLOS;
                        obs.kFactor = r.sampledKFactor;
                        obs.nlosDb = r.nlosDb;
                    }
                    else
                    {
                        PhysicsResult r = engine868.ProcessTransmission(
                            nodeId, static_cast<uint16_t>(tags[pt].nodeId), pos,
                            tags[pt].pos, phy[ati].txPowerDbm, phy[ati].sf,
                            phy[ati].bandwidthHz, 1, now);
                        got = r.packetReceived;
                        rAB = r.estimatedRangeM;
                        sigRaw = r.rangingSigmaM;
                    }

                    // Charged on the active tier, whether or not the measurement is
                    // ultimately admitted. The energy was spent on the exchange; a
                    // later software decision to discard the result does not refund it.
                    energy.AccountRangingExchange(active, 1, got ? 1 : 0, t);

                    if (!got || !std::isfinite(rAB) || rAB <= 0.0 || dist <= 0.0)
                    {
                        continue;
                    }

                    CaTblRelayedRange rr;
                    rr.relayId = static_cast<uint16_t>(tags[pt].nodeId);
                    rr.tier = active;
                    // Depth is the relay's own hop count plus one. A relay whose
                    // position came from infrastructure is at depth 1; one whose
                    // position came from another relay is deeper, and carries the
                    // uncertainty it inherited in its covariance.
                    rr.hopDepth = 1;
                    rr.maxRelayDepth = maxRelayDepth;
                    rr.relayVarBudgetM2 = relayVarBudgetM2;
                    rr.sigmaRangeM = sigRaw;
                    rr.relayP[0][0] = tags[pt].policy.Pxx();
                    rr.relayP[0][1] = tags[pt].policy.Pxy();
                    rr.relayP[1][0] = tags[pt].policy.Pxy();
                    rr.relayP[1][1] = tags[pt].policy.Pyy();
                    // The relay filtered this same cycle, so its covariance is current.
                    rr.relayAgeS = 0.0;
                    rr.relayFixIntervalS = dt;
                    rr.ux = ddx / dist;
                    rr.uy = ddy / dist;
                    rr.distToRelayM = dist;
                    // L.1.2: the relay's own depth + 1, and its covariance composed with
                    // what it inherited. Without --relayChain every relay is depth 0.
                    if (chain && !chain->ForTag(pt, tags[pt].policy, rr, t, nodeId))
                    {
                        continue;
                    }

                    LegTerms::Out terms;
                    if (legTerms)
                    {
                        terms = legTerms->Apply(nodeId, tags[pt].nodeId, obs, rAB, sigRaw);
                        rr.sigmaRangeM = terms.sigmaLegM; // C5 composes sigma_eff from it
                    }
                    double sigmaEff = 0.0;
                    const CaTblRelayVerdict v =
                        CaTblAdmitRelayedRange(rr, relaysUsedThisUpdate, &sigmaEff);
                    if (chain)
                    {
                        chain->Record(t, nodeId, 1, rr, chain->inheritedVarM2[pt],
                                      static_cast<int>(v),
                                      (v == CA_TBL_RELAY_ADMIT) ? sigmaEff : 0.0,
                                      legTerms ? &terms : nullptr);
                    }
                    switch (v)
                    {
                    case CA_TBL_RELAY_ADMIT:
                        break;
                    case CA_TBL_RELAY_REJECT_TIER:
                        ++relayRejTier;
                        continue;
                    case CA_TBL_RELAY_REJECT_DEPTH:
                        ++relayRejDepth;
                        continue;
                    case CA_TBL_RELAY_REJECT_GEOMETRY:
                        ++relayRejGeom;
                        continue;
                    case CA_TBL_RELAY_REJECT_STALE:
                        ++relayRejStale;
                        continue;
                    default:
                        ++relayRejDup;
                        continue;
                    }

                    // The PSEUDO-ANCHOR. The anchor position is the relay's ESTIMATE,
                    // not its true position, and the inflated sigma is what pays for
                    // that estimate being wrong. Passing the true position with the
                    // inflated sigma would be the same category error in the opposite
                    // direction: a measurement that is better than the model claims.
                    CaTblRangeMeasurement m;
                    m.anchorX = bx;
                    m.anchorY = by;
                    m.rangeM = rAB;
                    m.nlos = false;
                    m.nlosPropagation = false;
                    m.hiddenChannelActive = false;
                    m.sigmaM = sigmaEff;
                    m.anchorId = rr.relayId; // DC-11
                    m.sourceKind = CA_TBL_SRC_RELAY;
                    m.hopDepth = rr.hopDepth;
                    activeMeas.push_back(m);

                    relaysUsedThisUpdate.insert(rr.relayId);
                    relayedVarThisCycle += sigmaEff * sigmaEff;
                    ++relayAdmitted;
                    relaySigmaEffSum += sigmaEff;
                    relaySigmaRawSum += sigRaw;
                }
            }
        }

        // sect 11.11 MOBILITY STATE, driven at last. CA_TBL_MOB_RECEDING existed in the
        // taxonomy and the receding branch of StartHandshake was written, and the
        // scenario never once called SetMobilityState -- so the eager-push path was dead
        // code that looked implemented.
        //
        // THE SIGNAL IS THE ONE THE NODE ACTUALLY HAS: the trend in its own mean measured
        // range. A node cannot know whether it is walking away from a gateway; it can
        // know that everything it ranges against is getting further off, which is the
        // same conclusion reached from evidence rather than from truth. Reading the
        // walker's true velocity would be the flattering shortcut and would not survive
        // being asked where a real node gets that number.
        //
        // The band is 5% of the previous mean rather than an absolute metre count,
        // because the same 20 m of drift means something different at 100 m and at
        // 2 km. Below it the state is STATIONARY, which is the honest reading of a
        // change smaller than the ranging noise.
        if (mobilityFromRange)
        {
            const std::vector<CaTblRangeMeasurement>& mm =
                activeMeas.empty() ? floorMeas : activeMeas;
            if (!mm.empty())
            {
                double sum = 0.0;
                for (size_t k = 0; k < mm.size(); ++k)
                {
                    sum += mm[k].rangeM;
                }
                const double mean = sum / static_cast<double>(mm.size());
                CaTblMobilityState ms = CA_TBL_MOB_STATIONARY;
                if (prevMeanRangeM > 0.0)
                {
                    const double band = 0.05 * prevMeanRangeM;
                    if (mean > prevMeanRangeM + band)
                    {
                        ms = CA_TBL_MOB_RECEDING;
                    }
                    else if (mean < prevMeanRangeM - band)
                    {
                        ms = CA_TBL_MOB_APPROACHING;
                    }
                }
                ++mobCounts[static_cast<int>(ms)];
                for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
                {
                    if (nodeMac[ti] != nullptr)
                    {
                        nodeMac[ti]->SetMobilityState(ms);
                    }
                }
                prevMeanRangeM = mean;
            }
        }

        // N.14 (--fuseFloorActive). The active tier came up short of three and the floor
        // ranged: the fix set is the active tier's ranges (relays included) WITH the floor's,
        // and the sparse cases run on that union in Step(). Otherwise a 1-2 range UWB or
        // 2.4 GHz set is discarded whenever the floor solves, and two floor ranges are
        // discarded whenever the node is off 868 MHz. Floor alone at three or more is the
        // old path. A union with active ranges is labelled with the ACTIVE tier (OURS):
        // the cycle used them, and they are the precise ones.
        std::vector<CaTblRangeMeasurement> fusedFA;
        const bool fuseFA = spec.fuseFloorActive && activeMeas.size() < 3 && !floorMeas.empty() &&
                            !(activeMeas.empty() && floorMeas.size() >= 3);
        if (fuseFA)
        {
            fusedFA = activeMeas;
            fusedFA.insert(fusedFA.end(), floorMeas.begin(), floorMeas.end());
        }
        const std::vector<CaTblRangeMeasurement>& meas =
            fuseFA ? fusedFA : (usedFloor ? floorMeas : activeMeas);
        const CaTblTier fixTier =
            fuseFA ? (activeMeas.empty() ? CA_TBL_TIER_868 : active)
                   : (usedFloor ? CA_TBL_TIER_868 : active);

        // --tier1Tdoa: the gateways in this tier-1 fix set stamped the same broadcast.
        // Solved here and queued, so Step() fuses it beside the ranges it came with.
        if (tdoa && fixTier == CA_TBL_TIER_868 && meas.size() >= 3)
        {
            tdoa->Solve(policy, meas);
        }

        // activeMeas.size() is passed separately from meas so the policy can tell
        // "the active tier delivered" from "we got a fix from somewhere". Without it
        // the tier-1 fallback hides every coverage failure from the de-escalation
        // logic and the policy latches on the top tier.
        // Evidence that a tier EXISTS here, as distinct from being reachable right
        // now. Built from lastNonEmptyAtS, which survives ClearNeighbours(), bounded
        // by demoteStalenessS. Tier 1 is always awake and is the floor, so it never
        // needs evidence.
        uint32_t tierEvidenceMask = 0;
        for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
        {
            if (lastNonEmptyAtS[k] >= 0.0 && (t - lastNonEmptyAtS[k]) <= demoteStalenessS)
            {
                tierEvidenceMask |= (1u << k);
            }
        }

        // WITNESS 1 -- the body, through a real step-and-heading system.
        //
        // The engine is ticked at its own 100 Hz sample rate across this cycle from the
        // gait the walker's motion implies, then asked where it thinks the node is. The
        // disagreement between that and the filter is the evidence.
        //
        // The tolerance grows with TIME SINCE THE LAST TRUSTED FIX, which is the SHS
        // error model: heading drift and step-length error accumulate while the engine
        // runs unaided, and a strapdown-style tolerance on distance walked would be the
        // wrong shape entirely. The engine's own header refuses to be wired to a
        // strapdown-derived check for exactly this reason.
        {
            shsDriver.Advance(t - dt, dt, prevTruePos, pos);
            if (imuIdeal && idealAnchored)
            {
                // Perfect relative motion from an imperfect anchor. The anchor's
                // error is retained, not compounded -- that is the difference
                // between a perfect IMU and an oracle.
                idealX += pos.x - prevTruePos.x;
                idealY += pos.y - prevTruePos.y;
            }
            prevTruePos = pos;
            const double sdx = shsEngine.GetX() - policy.EstX();
            const double sdy = shsEngine.GetY() - policy.EstY();
            imuDisagreeM = std::sqrt(sdx * sdx + sdy * sdy);
            const double unaidedS = std::max(0.0, t - lastTrustedFixS);
            imuTolM = imuConsistencyK *
                      std::max(imuSigmaFloorM, std::sqrt(std::max(policy.Pxx() + policy.Pyy(), 0.0))) +
                      imuDriftMps * unaidedS;
        }

        // WITNESS 2 -- the neighbours. Does the range actually measured to a node agree
        // with where that node says it is, given where this node thinks IT is?
        int32_t corrOk = 0, corrTotal = 0;
        if (corroborationCheck && nodeMac[active] != nullptr)
        {
            const double sigMine = std::sqrt(std::max((policy.Pxx() + policy.Pyy()), 0.0));
            for (size_t k = 0; k < rangedThisCycle.size(); ++k)
            {
                CaTblTableEntry te;
                if (!nodeMac[active]->GetTableEntry(rangedThisCycle[k].first, te))
                {
                    continue; // never heard where it claims to be: not a witness
                }
                ++corrTotal;
                const double ex = policy.EstX() - te.x, ey = policy.EstY() - te.y;
                const double predicted = std::sqrt(ex * ex + ey * ey);
                const double resid = std::fabs(rangedThisCycle[k].second - predicted);
                const double sigTheirs = std::sqrt(std::max(te.varX + te.varY, 0.0));
                const double tol = corroborationK *
                                   std::sqrt(sigMine * sigMine + sigTheirs * sigTheirs +
                                             corroborationRangeSigmaM * corroborationRangeSigmaM);
                if (resid <= tol)
                {
                    ++corrOk;
                }
            }
        }
        // Share of this cycle's measurement variance that arrived through a relay.
        // Over the set actually used, so a floor fallback -- which contains no relayed
        // measurement at all -- correctly reports zero rather than inheriting the
        // active set's share.
        {
            double totVar = 0.0;
            for (size_t mi = 0; mi < meas.size(); ++mi)
            {
                totVar += meas[mi].sigmaM * meas[mi].sigmaM;
            }
            const double rvf =
                (totVar > 0.0) ? std::min(1.0, relayedVarThisCycle / totVar) : 0.0;
            if (rvf > 0.0)
            {
                relayedFracSum += rvf;
                ++relayedFracN;
            }
            policy.SetRelayedVarFraction(rvf);
        }

        policy.SetCrossCheck(imuDisagreeM, imuCrossCheck ? imuTolM : 0.0,
                             corrOk, corrTotal);

        if (imuPred)
        {
            // DC-1. What an IMU-based dead-reckoner reports for the walker's true
            // displacement this cycle, queued as Step()'s control input.
            imuPred->dx = imuPred->havePrev ? pos.x - imuPred->prevX : 0.0;
            imuPred->dy = imuPred->havePrev ? pos.y - imuPred->prevY : 0.0;
            imuPred->rk.Step(imuPred->dx, imuPred->dy, dt, imuPred->ux, imuPred->uy, imuPred->qu);
            policy.SetImuControl(imuPred->ux, imuPred->uy, imuPred->qu[0], imuPred->qu[1],
                                 imuPred->qu[2]);
            imuPred->prevX = pos.x;
            imuPred->prevY = pos.y;
            imuPred->havePrev = true;
        }
        if (witness && imuPred)
        {
            // DC-9. The witness under --imuWitnessSource=model: the synthetic IMU's track,
            // anchored at the last fused fix and advanced by u, against THIS cycle's raw
            // radio fix -- multilateration of the fix set alone, which never saw u. The EKF
            // estimate already contains u under --imuPredict, so comparing the track with
            // the estimate would compare the IMU with itself (the plan's review finding).
            // Tolerance K sqrt(trace(R_fix) + trace(Sigma_acc)), K = --imuConsistencyK: the
            // fix's own covariance plus the IMU's accumulated error since the anchor.
            if (witness->anchored)
            {
                witness->wx += imuPred->ux;
                witness->wy += imuPred->uy;
            }
            witness->haveFix = false;
            witness->tol = 0.0;
            witness->disagree = 0.0;
            if (witness->anchored && meas.size() >= 3)
            {
                std::vector<double> mv(meas.size());
                for (size_t mi = 0; mi < meas.size(); ++mi)
                {
                    const double sg = (meas[mi].sigmaM > 0.0) ? meas[mi].sigmaM : 1.0;
                    mv[mi] = sg * sg;
                }
                double rxy = 0.0;
                witness->haveFix = CaTblPolicy::RawMultilaterationWeighted(
                    meas, mv, policy.EstX(), policy.EstY(), witness->fx, witness->fy,
                    witness->rxx, rxy, witness->ryy);
                if (witness->haveFix)
                {
                    const double* sa = imuPred->rk.SigmaAcc();
                    witness->sigTrace = sa[0] + sa[2];
                    witness->disagree =
                        std::hypot(witness->wx - witness->fx, witness->wy - witness->fy);
                    witness->tol = imuConsistencyK *
                                   std::sqrt(std::max(0.0, witness->rxx + witness->ryy +
                                                               witness->sigTrace));
                }
            }
            policy.SetCrossCheck(witness->disagree, witness->tol, corrOk, corrTotal);
        }
        if (trig)
        {
            // MC-2. The filter's state carried into this cycle, before Step() moves it.
            trig->preX = policy.EstX();
            trig->preY = policy.EstY();
            trig->preVx = policy.EstVx();
            trig->preVy = policy.EstVy();
            trig->preTraceP = policy.Pxx() + policy.Pyy();
        }
        if (spec.queryTriggeredEscalation)
        {
            // NW-4: an URGENT query that named tag 0 arms the policy's hook.
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (nodeMac[ti] != nullptr && nodeMac[ti]->TakeUrgentQuery())
                {
                    policy.RequestQueryEscalation();
                }
            }
        }
        const uint64_t covBefore = policy.GetUwbCoverageClears(); // NW-7
        // NW-1. The deepest leg in the set Step() fuses: relayed ranges carry their depth
        // (C5, and the chain's composition), direct ones 0. The tier-1 floor's set is
        // direct by construction.
        uint32_t hopArg = 0;
        uint32_t nRelayedLegs = 0;
        if (spec.hopProvenance)
        {
            for (size_t k = 0; k < meas.size(); ++k)
            {
                hopArg = std::max<uint32_t>(hopArg, meas[k].hopDepth);
                nRelayedLegs += (meas[k].hopDepth > 0) ? 1u : 0u;
            }
        }
        CaTblDecision d = policy.Step(dt,
                                            meas,
                                            hopArg,
                                            static_cast<int32_t>(activeMeas.size()),
                                            static_cast<int32_t>(activeMeasAddressed),
                                            tierEvidenceMask);
        if (fuseFA)
        {
            fuseFloorLog->Row(t, nodeId, active, activeMeas.size(), floorMeas.size(), d, fixTier);
        }
        // NW-1: a fix that fused the set carries its depth; anything else carries none.
        const uint32_t hopCountAtFix =
            (d.hadFix && !d.measurementRejected && meas.size() >= 3) ? hopArg : 0u;
        if (hopLog)
        {
            *hopLog << std::fixed << std::setprecision(4) << t << ',' << meas.size() << ','
                    << nRelayedLegs << ',' << hopArg << ',' << (usedFloor ? 1 : 0) << ','
                    << (d.hadFix ? 1 : 0) << ',' << (d.measurementRejected ? 1 : 0) << ','
                    << hopCountAtFix << '\n';
            ++hopFixesByDepth[std::min<uint32_t>(3u, hopCountAtFix)];
        }
        v2Pcs[0] = d.pcs; // NW-9
        v2Suspect[0] = d.suspect ? 1 : 0;
        if (d.hadFix)
        {
            v2FixTier[0] = static_cast<uint8_t>(fixTier); // TA-1
            v2FixHop[0] = static_cast<uint8_t>(std::min<uint32_t>(255u, hopCountAtFix));
        }
        if (grantLog)
        {
            if (policy.GetUwbCoverageClears() > covBefore)
            {
                grantNote(t, nodeId, "coverage", 0);
            }
            if (d.uwbPending)
            {
                grantNote(t, nodeId, "request", 0);
            }
        }
        if (mesh)
        {
            // NW-0. Which of the node's radios are awake for the coming slice. The slice
            // runs BEFORE the next cycle's ranging (Simulator::Run at the loop head), so it
            // belongs to the tier that cycle will use -- d.nextTier -- plus 868 MHz while a
            // hybrid arm keeps the floor awake: the same attribution the listen billing
            // below makes. A radio asleep for the slice neither receives nor relays
            // (CLAUDE.md sect 6). Gating on THIS cycle's tier instead would keep a newly
            // chosen radio asleep through the slice in which it should discover neighbours.
            const bool floorNext = hybridArm && d.nextTier != CA_TBL_TIER_868;
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (nodeMac[ti] != nullptr)
                {
                    nodeMac[ti]->SetAsleep(!(ti == static_cast<int>(d.nextTier) ||
                                             (floorNext && ti == CA_TBL_TIER_868)));
                }
            }
        }
        if (witness)
        {
            // DC-9. The cycle's witness, then the anchor moves to a fused fix.
            witness->evals += d.witnessEvaluated ? 1 : 0;
            witness->refusals += d.witnessRefused ? 1 : 0;
            witness->absorbed += d.witnessAbsorbed ? 1 : 0;
            witness->log << std::fixed << std::setprecision(4) << t << ','
                         << (witness->anchored ? 1 : 0) << ',' << std::setprecision(6)
                         << witness->wx << ',' << witness->wy << ','
                         << (witness->haveFix ? 1 : 0) << ',' << witness->fx << ','
                         << witness->fy << ',' << witness->rxx << ',' << witness->ryy << ','
                         << witness->sigTrace << ',' << witness->disagree << ','
                         << witness->tol << ',' << (d.witnessEvaluated ? 1 : 0) << ','
                         << (d.witnessRefused ? 1 : 0) << ',' << (d.witnessAbsorbed ? 1 : 0)
                         << ',' << d.estX << ',' << d.estY << '\n';
            if (d.hadFix && !d.measurementRejected)
            {
                witness->anchored = true;
                witness->wx = d.estX;
                witness->wy = d.estY;
            }
        }
        if (caseDst && d.fixKind == CA_TBL_FIX_CASE_D)
        {
            // DC-4. Every Case D cycle, with what the decision did with it.
            ++caseDst->cycles;
            caseDst->applied += d.caseDApplied ? 1 : 0;
            caseDst->refused += d.caseDApplied ? 0 : 1;
            caseDst->wanted += d.wantedEscalation ? 1 : 0;
            caseDst->escalated += d.escalated ? 1 : 0;
            caseDst->demoted += d.deescalated ? 1 : 0;
            caseDst->log << std::fixed << std::setprecision(4) << t << ','
                         << static_cast<int>(d.activeTier) << ',' << meas[0].anchorId << ','
                         << static_cast<int>(meas[0].sourceKind) << ',' << std::setprecision(6)
                         << d.caseDNis << ',' << (d.caseDApplied ? 1 : 0) << ','
                         << d.traceSigma << ',' << d.pcs << ',' << (d.wantedEscalation ? 1 : 0)
                         << ',' << (d.escalated ? 1 : 0) << ',' << (d.deescalated ? 1 : 0)
                         << ',' << static_cast<int>(d.nextTier) << '\n';
        }
        if (imuPred)
        {
            // DC-1. A fused fix restarts the accumulated position error; a rejected one
            // was not fused and does not.
            const double* sa = imuPred->rk.SigmaAcc();
            const bool reset = d.hadFix && !d.measurementRejected;
            imuPred->log << std::fixed << std::setprecision(6) << t << ',' << imuPred->dx << ','
                         << imuPred->dy << ',' << imuPred->ux << ',' << imuPred->uy << ','
                         << std::setprecision(9) << imuPred->rk.Heading() << ','
                         << std::setprecision(9) << imuPred->qu[0] << ',' << imuPred->qu[1] << ','
                         << imuPred->qu[2] << ',' << sa[0] << ',' << sa[1] << ',' << sa[2] << ','
                         << std::setprecision(6) << imuPred->rk.AccX() << ','
                         << imuPred->rk.AccY() << ',' << imuPred->rk.ElapsedS() << ','
                         << (d.imuPredicted ? 1 : 0) << ',' << (d.hadFix ? 1 : 0) << ','
                         << (d.measurementRejected ? 1 : 0) << ',' << (reset ? 1 : 0) << ','
                         << d.estX << ',' << d.estY << '\n';
            ++imuPred->cycles;
            if (reset)
            {
                imuPred->rk.ResetAccumulation();
                ++imuPred->resets;
            }
        }
        if (sparse && sparse->twoNoFloor)
        {
            // DC-11. The census of a two-range cycle, now that Step() has decided it:
            // solved as A or B, refused (C), or -- relays having made the set three or
            // more -- the ordinary solve (bucket 0).
            if (d.sparseVerdict != 255)
            {
                ++sparse->attempts;
                ++caseAAttempts;
                if (d.fixKind == CA_TBL_FIX_CASE_A || d.fixKind == CA_TBL_FIX_CASE_B)
                {
                    ++caseAAdmitted;
                    const bool isB = (d.fixKind == CA_TBL_FIX_CASE_B);
                    ++caseSolved[isB ? 2 : 1];
                    ++(isB ? sparse->caseB : sparse->caseA);
                }
                else if (d.sparseVerdict < 4)
                {
                    ++caseARefused[d.sparseVerdict];
                    ++sparse->refused[d.sparseVerdict];
                    ++caseSolved[3];
                }
            }
            else if (d.hadFix)
            {
                ++caseSolved[0];
                ++sparse->fullWithRelays;
            }
            else
            {
                ++caseSolved[3];
            }
        }
        if (sparse && d.sparseVerdict != 255)
        {
            sparse->log << std::fixed << std::setprecision(4) << t << ',' << meas.size() << ','
                        << int(d.sparseVerdict) << ',' << int(d.fixKind) << ',' << d.sparsePriorX
                        << ',' << d.sparsePriorY << ',' << sparse->prevX << ',' << sparse->prevY
                        << ',' << d.sparseX << ',' << d.sparseY << ',' << d.sparseVarM2 << ','
                        << d.estX << ',' << d.estY << ',' << d.rangeUpdates << ','
                        << d.positionFuses << ',' << int(d.sparsePriorSource) << '\n';
        }
        if (tdoa)
        {
            tdoa->EndCycle(t, cycleIndex, pos, policy.AuxFusedLast(), d.measurementRejected,
                           d.estX, d.estY);
        }
        if (nlosScorer)
        {
            nlosScorer->NoteStep(d);
        }
        if (integrityLog && d.integrityBasis != 0)
        {
            *integrityLog << std::fixed << std::setprecision(4) << t << ","
                          << CaTblTierName(fixTier) << "," << meas.size() << "," << d.nNlos
                          << "," << d.nNlosScored << "," << std::setprecision(9)
                          << d.integrityMeanScore << "," << static_cast<unsigned>(d.integrityBasis)
                          << "," << (d.integrityEligible ? 1 : 0) << "," << std::setprecision(4)
                          << d.mahalanobis << "," << (d.suspect ? 1 : 0) << "\n";
        }

        // What the subject would have reported this cycle, read by the alert test at
        // the top of the NEXT cycle: posterior radial sigma, sqrt of the covariance
        // trace. Guarded against a negative trace so a filter fault reads as lost
        // confidence rather than as a NaN that compares false against every bound.
        lastReportHadFix = d.hadFix;
        lastReportedSigmaM =
            (d.traceSigma >= 0.0) ? std::sqrt(d.traceSigma) : -1.0;

        // ---- INERTIAL DEAD RECKONING FILLS THE GAP -------------------------
        //
        // With no radio fix this cycle the filter would coast on constant
        // velocity, while the SHS holds a dead-reckoned position nobody reads.
        // Hand it over as a measurement instead.
        //
        // Its uncertainty grows with time since the last TRUSTED fix -- the SHS
        // error model, where drift accumulates with UNAIDED TIME rather than with
        // distance walked -- reusing the two constants the consistency check
        // already uses, so the fusion and the check state one error model between
        // them rather than two.
        //
        // NOT the check's max(sigmaFloor, sqrt(trace P)) form. That check asks
        // whether two estimates disagree by more than their COMBINED uncertainty,
        // so it must carry the filter's covariance. A measurement variance must
        // describe the SHS ALONE -- folding P into R would count the filter's own
        // uncertainty on both sides of its own update.
        //
        // Never on a cycle that HAS a fix: a radio measurement beats dead
        // reckoning whenever there is one. This fills gaps, it does not compete.
        if (imuFusion && !d.hadFix && shsDriver.seeded)
        {
            const double unaidedFuseS = std::max(0.0, t - lastTrustedFixS);
            const double sigShs = imuSigmaFloorM + imuDriftMps * unaidedFuseS;
            // Independent by construction: the error the anchor was placed with,
            // plus what the engine has accumulated since. They add as variances.
            const double fuseVarM2 = imuAnchorVarM2 + sigShs * sigShs;
            if (fuseVarM2 > 0.0)
            {
                const double fx = imuIdeal ? idealX : shsEngine.GetX();
                const double fy = imuIdeal ? idealY : shsEngine.GetY();
                policy.FusePosition(fx, fy, fuseVarM2);
                ++imuFusedCycles;
            }
        }

        // The engine takes the integrity verdict back: a fix the check calls SUSPECT
        // must not be used to reset inertial drift, or a bad fix would launder itself
        // into the one estimate that was independent of it.
        // THE ANSWER HAS TO COME BACK. With the estimator on the gateways the node
        // does not compute this fix -- it is sent one -- and that is a frame it
        // must be awake to receive. Only a cycle that produced a fix generates a
        // downlink: a network with nothing to say sends nothing.
        //
        // ON THE RADIO THE FIX CAME FROM (TECH_DEBT M.10, decided 2026-09-30). The
        // gateways that computed a floor-solved fix are the ones that heard the node's
        // 868 MHz broadcast this cycle; the escalated tier is the link that has just
        // failed to reach three anchors. So the answer comes back on 868 MHz and is
        // billed there -- 0.9042 mJ at the defaults, not the 0.2005 mJ a 2.4 GHz frame
        // costs. fixTier differs from active only on a hybrid arm's floor-solved cycle,
        // where 868 has already transmitted (rangeTier() billed the broadcast), so this
        // charges no radio that stayed silent (sect 6). The downlink shares the fix's
        // radio because that is the link known to have closed to three gateways this
        // cycle: our choice, not cited. --edgeDownlinkTier=active keeps the old bill on
        // the active tier.
        if (gatewayEdgeServer && d.hadFix)
        {
            energy.AccountEdgeServerDownlink(edgeDownlinkOnFixTier ? fixTier : active);
        }

        // sect 49 BATTERY DEPLETION. The measured tag has a finite budget; when it is
        // spent the radio stops transmitting and does not come back.
        //
        // IT REUSES THE SILENCE MECHANISM DELIBERATELY. A flat battery and a failed
        // transmit chain look identical from the network's side -- the node stops being
        // heard and goes on existing -- so modelling them with two different mechanisms
        // would be inventing a distinction the network cannot observe. What differs is
        // only the CAUSE and whether it can end, and a depleted battery cannot: there is
        // no return window here, unlike --injectSilentId.
        //
        // THE NODE KEEPS RUNNING ITS FILTER. That is not an oversight. A device with a
        // dead radio still knows where it thinks it is; it simply cannot tell anyone,
        // and its estimate degrades from that moment because no measurement arrives to
        // correct it. Stopping the estimator instead would model a node that switched
        // OFF, which is a different failure and would hide the drift this one causes.
        // MC-2. LARGE CORRECTION (MAC sect 3 and 8: "large correction or dying battery").
        // The posterior moved further from the constant-velocity prediction than
        // k * sqrt(trace P) of the covariance carried into the cycle -- the uncertainty
        // the entry the mesh holds was published with -- so that entry is now wrong by
        // more than it claims: push the corrected one at once, on the radio this cycle
        // already used, instead of waiting for a digest. k is --triggeredUpdateK
        // (default 1, OURS). The plan's draft said 3; measured, a Kalman update moves
        // the mean by about one prior sigma at most (max 1.05 sigma over 359 cycles at
        // seed 7), so k = 3 never fires. P is the previous posterior, before this
        // cycle's process noise -- also OURS: it is smaller than the predicted
        // covariance, so the trigger fires a little more often than against it.
        if (trig && !batteryDead && nodeMac[active] != nullptr && trig->preTraceP > 0.0 &&
            std::isfinite(trig->preTraceP))
        {
            const double kTrig = spec.triggeredUpdateK;
            const double moveM = std::hypot(policy.EstX() - (trig->preX + trig->preVx * dt),
                                            policy.EstY() - (trig->preY + trig->preVy * dt));
            const double threshM = kTrig * std::sqrt(trig->preTraceP);
            if (moveM > threshM)
            {
                // The entry the frame carries is the corrected one.
                for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
                {
                    if (nodeMac[ti] != nullptr)
                    {
                        nodeMac[ti]->PublishSelfEntry(
                            policy.EstX(), policy.EstY(), policy.Pxx(), policy.Pyy(),
                            policy.Pxy(), spec.entryVelocity ? policy.EstVx() : 0.0,
                            spec.entryVelocity ? policy.EstVy() : 0.0, t);
                    }
                }
                nodeMac[active]->SendTriggeredUpdate(active, 1);
                // Billed to the node on the tier it rides, which transmitted this cycle
                // (CLAUDE.md sect 6): the first frame and its jittered repeats, as
                // attempted. A repeat the duty governor then refuses is still billed;
                // the MC-2 line prints frames billed against frames transmitted.
                const uint32_t frames = 3; // CaTblMacConfig::triggeredUpdateRepeats
                energy.AccountFrameTx(active, t,
                                      frames * nodeMac[active]->FrameAirtimeS(
                                                   CA_TBL_MSG_TRIGGERED_UPDATE, active, 1));
                ++trig->nLarge;
                trig->billedLarge += frames;
                trig->log << std::fixed << std::setprecision(4) << t << ",large_correction,"
                          << static_cast<int>(active) << ',' << moveM << ',' << threshM << ','
                          << policy.EstX() << ',' << policy.EstY() << ','
                          << (trig->preX + trig->preVx * dt) << ','
                          << (trig->preY + trig->preVy * dt) << ',' << std::setprecision(9)
                          << energy.GetTotalJ() << ',' << frames << '\n';
            }
        }

        if (batteryDepleteJ > 0.0 && !batteryDead && energy.GetTotalJ() >= batteryDepleteJ)
        {
            batteryDead = true;
            batteryDeadAtS = t;
            if (trig && nodeMac[active] != nullptr)
            {
                // MC-2. LAST GASP: one unrepeated TRIGGERED_UPDATE on the active radio,
                // queued BEFORE the radios are silenced below. SetSilent() gates Send()
                // only, so the queued frame leaves after CAD and nothing after it does.
                // Billed like any frame on the tier that transmitted this cycle; the
                // battery overshoots its budget by this one frame, which is stated.
                nodeMac[active]->SendTriggeredUpdate(active, 2);
                energy.AccountFrameTx(
                    active, t, nodeMac[active]->FrameAirtimeS(CA_TBL_MSG_TRIGGERED_UPDATE,
                                                              active, 1));
                ++trig->nGasp;
                trig->billedGasp += 1;
                trig->log << std::fixed << std::setprecision(4) << t << ",last_gasp,"
                          << static_cast<int>(active) << ",,," << policy.EstX() << ','
                          << policy.EstY() << ",,," << std::setprecision(9)
                          << energy.GetTotalJ() << ",1\n";
            }
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (nodeMac[ti] != nullptr)
                {
                    nodeMac[ti]->SetSilent(true);
                }
            }
        }

        // TRANSPORT sect 2 / APPLICATION sect 3-5. The layer above the MAC, driven by the
        // running simulator rather than only by its unit suite.
        //
        // A RESULT CARRIES ITS LINEAGE, not just coordinates -- tier, PCS, hop count,
        // the integrity verdict, and the covariance itself so an ellipse comes from the
        // filter rather than a stylised radius. Surfacing the SUSPECT verdict is the
        // entire reason the integrity check exists, and CaTblRenderFor lets it override
        // tier and PCS: a sub-metre UWB fix that failed the Mahalanobis gate is the
        // dangerous case precisely because its tier would otherwise earn it the most
        // confident marker on the screen.
        // sect 24-27. Contact is what the tag ACTUALLY HEARD this cycle, taken from the
        // per-tier neighbour tables rather than from the deployment: a node that exists
        // and cannot be heard is exactly the case this tracks, and reading the
        // deployment would report everyone present forever.
        if (lifecycleTracking)
        {
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (nodeMac[ti] == nullptr)
                {
                    continue;
                }
                const std::vector<CaTblNeighbour> nb =
                    nodeMac[ti]->GetNeighbours(static_cast<CaTblTier>(ti), t);
                for (size_t k = 0; k < nb.size(); ++k)
                {
                    lifecycle.NoteContact(nb[k].id, t);
                }
            }
            // Tick AFTER the contacts, so a node heard this cycle is not aged out by the
            // same call that should have refreshed it.
            lifecycle.Tick(t);
        }

        if (sessionLayer)
        {
            if (sessLog)
            {
                // TA-1a: a Tick can move the session (stale, timed out); recorded too.
                std::map<uint32_t, CaTblSession>::const_iterator si =
                    sessions.Sessions().find(sessionId);
                const CaTblSessionState before = (si != sessions.Sessions().end())
                                                     ? si->second.State()
                                                     : CA_TBL_SESSION_CLOSED;
                sessions.Tick(t);
                si = sessions.Sessions().find(sessionId);
                const CaTblSessionState after = (si != sessions.Sessions().end())
                                                    ? si->second.State()
                                                    : CA_TBL_SESSION_CLOSED;
                if (after != before)
                {
                    *sessLog << std::fixed << std::setprecision(6) << t << ",0," << sessionId
                             << ",tick,0,0,0,0,255,0,0,0," << CaTblSessionStateName(before)
                             << ',' << CaTblSessionStateName(after) << ",0,0,-1\n";
                }
            }
            else
            {
                sessions.Tick(t);
            }
            for (size_t fi = 0; fi < finderSearch.size(); ++fi)
            {
                // TA-1b / TA-3: a finder's own session ages like the base station's, and is
                // CANCELLED (closed) once the finder has physically reached its target --
                // within --finderCancelM (2 m), true positions (OURS): Transport sect 2's
                // explicit cancel.
                FinderSearch& fs = finderSearch[fi];
                const CaTblSessionState fb = fs.mgr.Sessions().at(fs.sid).State();
                fs.mgr.Tick(t);
                CaTblSessionState fa = fs.mgr.Sessions().at(fs.sid).State();
                const uint32_t k = static_cast<uint32_t>(fs.target) - 1u;
                const Vector tp = (k == 0) ? pos : ((k < tags.size()) ? tags[k].pos : pos);
                const double dm = std::hypot(finderPos[fi].x - tp.x, finderPos[fi].y - tp.y);
                const char* ev = "tick";
                if (fs.cancelledAtS < 0.0 && fa != CA_TBL_SESSION_CLOSED && dm < spec.finderCancelM)
                {
                    fs.mgr.Close(fs.sid, t);
                    fs.cancelledAtS = t;
                    fs.cancelledAtM = dm;
                    fa = fs.mgr.Sessions().at(fs.sid).State();
                    ev = "cancel";
                }
                if (fa != fb && sessLog)
                {
                    *sessLog << std::fixed << std::setprecision(6) << t << ',' << finderIds[fi]
                             << ',' << fs.sid << ',' << ev << ',' << static_cast<int>(fs.target)
                             << ",0,0,0,255,0,0,0," << CaTblSessionStateName(fb) << ','
                             << CaTblSessionStateName(fa) << ",0,0," << std::setprecision(4)
                             << dm << '\n';
                }
            }
            if (d.hadFix && !spec.sessionOverMesh)
            {
                CaTblResult res;
                res.targetManifestId = 1u;
                res.x = policy.EstX();
                res.y = policy.EstY();
                res.varX = policy.Pxx();
                res.varY = policy.Pyy();
                res.covXY = policy.Pxy();
                res.tierSource = fixTier;
                res.pcs = d.pcs;
                res.hopCountAtFix = spec.hopProvenance ? hopCountAtFix : 0u; // NW-1
                res.suspect = d.suspect;
                res.timestampS = t;
                sessions.OnResult(res, t);
                ++renderCounts[CaTblRenderFor(res)];
            }
        }
        if (d.hadFix && !d.suspect)
        {
            // ONLY IF THE FIX SAYS SOMETHING ABOUT MOTION.
            //
            // OnTrustedFix calibrates step length and heading from the
            // displacement between consecutive fixes. The node walks ~3 m per
            // cycle and the fixes carry ~167 m of error, so at every cycle that
            // displacement is about 1 part signal to 56 parts noise, and the
            // heading it implies is noise. Measured: arms that fed every fix ran
            // 5-18 km off, while the one arm that never fed any stayed at 20-42 m.
            //
            // So wait until the node has actually moved, relative to how well its
            // position is known. The threshold scales with the filter's own
            // covariance rather than with a chosen constant, and skipping simply
            // leaves the SHS dead-reckoning, which is its job.
            const double sigPos =
                std::sqrt(std::max(0.0, policy.Pxx() + policy.Pyy()));
            const double moved =
                std::hypot(policy.EstX() - lastTrustedX,
                           policy.EstY() - lastTrustedY);
            const bool informative =
                (shsTrustedFixK <= 0.0) || !haveTrusted ||
                (moved > shsTrustedFixK * sigPos);
            if (informative)
            {
                shsEngine.OnTrustedFix(t, policy.EstX(), policy.EstY(), true);
                idealX = policy.EstX();
                idealY = policy.EstY();
                idealAnchored = true;
                // Recorded at the instant of anchoring, not read later: the
                // filter's covariance moves every cycle, and what this engine
                // inherited is the value it had HERE.
                imuAnchorVarM2 = policy.Pxx() + policy.Pyy();
                lastTrustedX = policy.EstX();
                lastTrustedY = policy.EstY();
                haveTrusted = true;
                ++shsTrustedFixes;
            }
            else
            {
                ++shsFixesSkipped;
            }
            lastTrustedFixS = t;
        }

        // MC-1. Under --handshakeFixes only an escalation INTO UWB is signalled: MAC sect 8
        // asks for an ACK'd "tier escalation request (UWB specifically)", and a demotion
        // is never deferred (Architecture sect 9.4), so it needs no one's permission. By
        // default 868 -> 2.4 GHz and every de-escalation were signalled and charged too.
        // Under the flag those send nothing and are charged nothing, so energy and duty
        // move.
        // NW-7: under --grantGatedUwb the escalation into UWB is the pending request.
        if (spec.handshakeFixes
                ? (d.escalated && (d.nextTier == CA_TBL_TIER_UWB || d.uwbPending))
                : (d.escalated || d.deescalated))
        {
            // THE GOVERNOR IS ADVISORY -- THE CALLER MUST CONSULT IT.
            //
            // This call site previously charged the handshake unconditionally.
            // CaTblEnergyModel does not enforce the duty cycle; it exposes
            // CanTransmit()/NoteDeferred() and expects the caller to ask. So the
            // request/grant round-trip burnt tier-1 airtime outside the regulatory
            // budget, and duty_868 reached 0.02511 against the WPC G.S.R. 853(E)
            // Table-II cap of 0.025 -- about 0.4 s of excess airtime over 3600 s,
            // a handful of SF7 frames, which is what tier-1 escalations at the
            // observed rate come to.
            //
            // The consequence is not only the compliance number. An escalation that
            // cannot be signalled must WAIT for the budget to refill, so the
            // regulation propagates directly into handoff latency and therefore
            // into position error. That coupling -- a legal constraint reaching the
            // localisation result -- only exists once the governor is actually
            // consulted here. The deferral count is emitted so its size is a
            // measurement rather than an assumption.
            //
            // NOTE: this changes arm_summary.csv for every arm that escalates, and
            // is therefore a NEW BASELINE (results/baseline_v4), not a delta against
            // baseline_v3 -- treated exactly as the MAC change was. Determinism from
            // a seed is preserved and re-verified; comparability with the previous
            // numbers is not. Static arms never escalate and are unchanged, which is
            // itself a check on the scope of this fix.
            if (energy.CanTransmit(active, t))
            {
                if (spec.deadBatteryNoDraw && batteryDead)
                {
                    ++deadNoDrawHs; // N.20: a dead radio pays for no handshake
                }
                else
                {
                    energy.AccountEscalationHandshake(active, t);
                }
                // C1: see the note at the pre-emption site above. Charge
                // unchanged, frame added.
                if (sendHandshakeFrames && nodeMac[active] != nullptr)
                {
                    // C4. Fill the two terms the anchor cannot see: T_deg is this
                    // node's own confidence degradation, T_batt how much of the stated
                    // budget it has spent. Quantised to a byte over [0,1].
                    if (spec.handshakeFixes)
                    {
                        // MC-1. This cycle's terms, not the previous cycle's: the
                        // end-of-cycle update that refreshes them runs after this send.
                        nodeMac[active]->SetRequestTerms(
                            CaTblQuantise(CaTblDegradationWith(pcsHist, d.pcs, pcsDegradation)),
                            CaTblQuantise(ArbBatteryTerm(
                                spec.arbitrationV2,
                                CaTblBatteryUsedFrac(energy.GetTotalJ(), arbBatteryCapJ))));
                        nodeMac[active]->SetRequestTargetTier(CA_TBL_TIER_UWB);
                        nodeMac[active]->SetRequestQueryTriggered(d.queryTriggered); // NW-4
                    }
                    else
                    {
                        nodeMac[active]->SetRequestTerms(CaTblQuantise(pcsDegradation),
                                                         CaTblQuantise(batteryUsedFrac));
                    }
                    nodeMac[active]->Send(CA_TBL_MSG_ESC_REQUEST, active, 0, 0);
                    ++tag0RequestFrames;
                }
            }
            else if (intraCycleHandshakeDefer &&
                     energy.GetNextAllowedTxS(active) < t + dt)
            {
                // A3. The budget refills BEFORE this cycle ends, so the handshake does
                // not have to slip into the next one and cost it a ranging slot.
                //
                // The arithmetic is the whole argument. One SF7/125 kHz/CR4-5 frame is
                // 56.58 ms of airtime, which at the 2.5% WPC G.S.R. 853(E) Table-II cap
                // arms 2.263 s of off-time. Against dt = 2.5 s that refill lands inside
                // the same cycle, with 0.237 s to spare -- so the pre-emption the
                // arbiter performs is avoidable at any dt above about 2.32 s, and below
                // that this branch simply does not fire because the condition fails.
                //
                // It is charged AT THE REFILL TIME, not at t. The wait is real: the
                // handoff is later by that much, and the latency is accumulated so it
                // can be reported rather than assumed away. What is NOT paid is the
                // next cycle's ranging exchange.
                const double tDefer = energy.GetNextAllowedTxS(active);
                if (spec.deadBatteryNoDraw && batteryDead)
                {
                    ++deadNoDrawHs; // N.20: a dead radio pays for no handshake
                }
                else
                {
                    energy.AccountEscalationHandshake(active, tDefer);
                }
                ++handshakeIntraCycleDefers;
                handshakeIntraCycleWaitS += (tDefer - t);
                if (sendHandshakeFrames && nodeMac[active] != nullptr)
                {
                    // C4. Fill the two terms the anchor cannot see: T_deg is this
                    // node's own confidence degradation, T_batt how much of the stated
                    // budget it has spent. Quantised to a byte over [0,1].
                    if (spec.handshakeFixes)
                    {
                        // MC-1. This cycle's terms, not the previous cycle's: the
                        // end-of-cycle update that refreshes them runs after this send.
                        nodeMac[active]->SetRequestTerms(
                            CaTblQuantise(CaTblDegradationWith(pcsHist, d.pcs, pcsDegradation)),
                            CaTblQuantise(ArbBatteryTerm(
                                spec.arbitrationV2,
                                CaTblBatteryUsedFrac(energy.GetTotalJ(), arbBatteryCapJ))));
                        nodeMac[active]->SetRequestTargetTier(CA_TBL_TIER_UWB);
                        nodeMac[active]->SetRequestQueryTriggered(d.queryTriggered); // NW-4
                    }
                    else
                    {
                        nodeMac[active]->SetRequestTerms(CaTblQuantise(pcsDegradation),
                                                         CaTblQuantise(batteryUsedFrac));
                    }
                    nodeMac[active]->Send(CA_TBL_MSG_ESC_REQUEST, active, 0, 0);
                    ++tag0RequestFrames;
                }
            }
            else
            {
                energy.NoteDeferred(active);
                ++handshakeDeferrals;
                handshakeDeferralDelayS += std::max(0.0, energy.GetNextAllowedTxS(active) - t);

                // MEASURED: 100% of fixed_threshold's 200 escalations are deferred,
                // against 1 of 193 for the ca_tbl arms. The asymmetry is structural,
                // not statistical -- fixed_threshold escalates 868 -> 2.4 every time,
                // so its handshake is charged on TIER 1, whose 2.5% WPC budget the
                // ranging exchange has already spent this cycle. The ca_tbl arms
                // mostly escalate from tier 2/3, which carry no duty limit.
                //
                // So an escalation handshake DEPARTING TIER 1 essentially never fits
                // the regulatory budget at dt = 2.5 s. Gating alone therefore trades
                // one error for another: the airtime becomes compliant, but the
                // handshake stops being charged at all and its energy is understated.
                //
                // Deferring the escalation itself is the physically coherent
                // resolution -- a handoff that cannot be signalled has not happened,
                // so the node stays put and retries -- and it is what produces the
                // cyber-physical coupling worth measuring: a legal airtime constraint
                // propagating into handoff latency and therefore into position error.
                //
                // DEFAULT ON. Leaving it off is not the conservative choice it looks
                // like. With gating on but deferral off the tier change still commits
                // inside Step(), so an arm performs the escalation and pays for none
                // of it -- and the size of that free ride is correlated with which arm
                // is under test: 100% of fixed_threshold's 200 handshakes go unpaid
                // against 1 of ca_tbl's 193, because fixed_threshold always departs
                // tier 1. The bias therefore runs in the BASELINE's favour, which is
                // exactly as wrong as the opposite and is the first thing a reviewer
                // finds: the baseline's handoffs are free, so of course it wins on
                // energy. Deferring removes the bias by making the escalation not
                // happen rather than happen for free.
                //
                // Demotion is never deferred: a node must be able to abandon a tier it
                // cannot reach.
                if (deferEscalationOnDutyLimit && d.escalated)
                {
                    if (arbitrateDutyBudget)
                    {
                        // ARBITRATED: the tier change COMMITS now -- the node
                        // moves to the new tier -- but the handshake energy
                        // cannot be charged this cycle (the budget is spent on
                        // ranging). The next cycle will pre-empt the ranging
                        // exchange ON THIS TIER -- the capped one -- so the
                        // handshake frame fits the budget, and the energy is
                        // charged there.
                        //
                        // This implements the access-category priority the MAC
                        // spec already defines (CW_high above CW_normal) in the
                        // duty-cycle governor. The cost is real: the next cycle
                        // loses its tier-1 measurements.
                        pendingHandshake = true;
                        pendingHandshakeTier = active;
                        pendQT[0] = d.queryTriggered ? 1 : 0; // NW-4
                        // DO NOT abort the tier change -- it commits.
                        // d.escalated stays true, d.nextTier stays as Step() set it.
                        ++handshakeDeferrals;
                        handshakeDeferralDelayS += dt;  // cost: one cycle of delay
                    }
                    else
                    {
                        // v5 behaviour: defer the escalation entirely.
                        policy.AbortTierChange(active);
                        ++escalationsDeferredAway;
                        d.escalated = false;
                        d.nextTier = active;
                    }
                }
            }
        }

        // §2.9 handshake, on the tier being LEFT -- the request/grant exchange has
        // to ride a link that still exists. This is claim 4's "custom handshake"
        // actually executing on the wire for the first time: Beacon -> Digest ->
        // targeted Entry-Push, with the access category and therefore the
        // contention window selected per message type.
        //
        // StartHandshake() branches on mobility state, and the RECEDING branch -- skip
        // the digest, one-shot urgent push at CW_high -- is the interesting half. At
        // defaults nothing sets mobility state, so every node reports STATIONARY and
        // only the stationary path runs. Under --mobilityFromRange (50a229a) the state
        // is driven from the node's own mean-range trend and the RECEDING branch does
        // run: TRIGGERED_UPDATE frames appear in events_* (TECH_DEBT L.1.5, M.9). This
        // comment said "the receding branch never runs" until 2026-09-30.
        if (d.escalated)
        {
            const std::vector<CaTblNeighbour> peers = nodeMac[active]->GetNeighbours(active, t);
            if (!peers.empty())
            {
                nodeMac[active]->StartHandshake(active, peers[0].id, 4);
            }
        }

        // Radios that transmitted this cycle are not also charged sleep. On a hybrid
        // arm that is two radios, and that cost is the honest price of the
        // architecture: the comparison against a single-band baseline is only fair if
        // the hybrid pays for the second radio it is using.
        for (int tierIdx = 0; tierIdx < CA_TBL_TIER_COUNT; ++tierIdx)
        {
            const CaTblTier tr = static_cast<CaTblTier>(tierIdx);
            const bool wasAwake = (tr == active) || (runFloor && tr == CA_TBL_TIER_868);
            if (mesh && nodeMac[tierIdx] != nullptr)
            {
                // NW-0. Under mesh links the node relays others' floods on its awake radios;
                // each such frame is a transmission on that tier, billed to the node.
                const double air = nodeMac[tierIdx]->GetRebroadcastAirtimeS();
                const uint64_t fr = nodeMac[tierIdx]->GetRebroadcastFrames();
                if (fr > mesh->billedFrames[tierIdx])
                {
                    energy.AccountTx(tr, air - mesh->billedAir[tierIdx],
                                     fr - mesh->billedFrames[tierIdx]);
                    mesh->billedAir[tierIdx] = air;
                    mesh->billedFrames[tierIdx] = fr;
                }
            }
            const uint64_t rxNow = nodeMac[tierIdx]->GetTotals().received;

            if (spec.deadBatteryNoDraw && batteryDead)
            {
                // N.20: a dead radio draws nothing -- no reception, no search window, no
                // sleep current. Its relays are refused (--silenceStopsRelay, required), and
                // any frame already past CAD when it died is still billed above.
                ++deadNoDrawT0;
            }
            else if (wasAwake)
            {
                // Discovery listening. SCOPE LIMIT, and it is a real one: this
                // charges RECEPTION, not SEARCHING. A physical receiver also burns
                // current through windows in which no beacon arrives, and modelling
                // that honestly needs a wake-up schedule the design document does
                // not specify. Discovery energy here is therefore a LOWER BOUND,
                // and the only search cost charged is the single AccountCad() probe
                // in rangeTier(). Do not present the post-gating energy figure as
                // the true cost of discovery -- present it as the floor.
                const uint64_t delivered = rxNow - lastRx[tierIdx];
                if (delivered > 0)
                {
                    energy.AccountListen(tr,
                                         static_cast<double>(delivered) * beaconToaS[tierIdx]);
                }

                // ---- SEARCH: the receiver is open and nothing is arriving -----
                //
                // This is the defect the first MAC-wired run exposed, and it is
                // worth stating in full because it is the same inversion the S2.5
                // rewrite was supposed to have killed.
                //
                // With the poll gated on the neighbour table, a UWB radio that is
                // awake and out of coverage issued no poll, and AccountCad() is a
                // no-op on UWB because DW3000 has no CAD primitive. It was
                // therefore charged EXACTLY ZERO on the 1305 of 1440 cycles it
                // spent finding nothing -- and static_uwb fell from 28.35 J, the
                // most expensive arm, to 0.7618 J, the cheapest. Verifier check 12
                // caught it and named the mechanism: "a radio that never ranges is
                // never charged, which inverts the axis".
                //
                // Charging the window is not a correction factor, it is the
                // consistency requirement. Discovery in this model is
                // deterministic-on-reception: if a beacon is transmitted in range,
                // the node hears it. Guaranteed detection requires the receiver to
                // be open across a full beacon period. Charging less than that buys
                // certain discovery at partial cost, which is not a cheaper model,
                // it is an inconsistent one.
                //
                // The consequence is large and it is a RESULT, not a problem: at
                // 94 mA and 3.3 V a 2.0 s UWB window is 0.620 J, so an always-on
                // UWB arm out of coverage costs roughly 60x the 868 arm rather than
                // 1/18th of it. That is precisely the argument for triggering the
                // precision tier from a cheaper one instead of leaving it on, i.e.
                // it is the project's own thesis appearing in the energy column.
                //
                // searchDutyFrac exists so the bracket is reportable rather than
                // asserted: 0.0 reproduces the zero-charge run exactly, 1.0 is the
                // consistent model, and the span between them is how much of
                // static_uwb's energy is a modelling assumption. Sweep it and
                // publish the curve; do not quote a single number.
                if (nodeMac[tierIdx]->GetNeighbourCount(tr, t) == 0)
                {
                    const double window =
                        std::min(dt, beaconPeriodS[tierIdx]) * searchDutyFrac;
                    if (window > 0.0)
                    {
                        energy.AccountListen(tr, window);
                    }
                }
            }
            else
            {
                energy.AccountSleep(tr, dt);
                // A radio in deep sleep hears nothing, so its neighbour table does
                // not survive the gap. Keeping it would hand the policy free
                // knowledge of a tier it had switched off and make re-escalation
                // instantaneous, when in reality it costs a discovery window. That
                // window is the mechanism by which escalation has latency at all
                // now, and it is measured rather than asserted: see the blind
                // column in the trace.
                nodeMac[tierIdx]->ClearNeighbours();
            }
            lastRx[tierIdx] = rxNow;
        }

        for (int tierIdx = 0; tierIdx < CA_TBL_TIER_COUNT; ++tierIdx)
        {
            const size_t n =
                nodeMac[tierIdx]->GetNeighbourCount(static_cast<CaTblTier>(tierIdx), t);
            nbrSum[tierIdx] += static_cast<double>(n);
            if (n > 0)
            {
                lastNonEmptyAtS[tierIdx] = t;
            }
        }
        const uint32_t polledCount = static_cast<uint32_t>(activeMeasAddressed);
        const bool blind = (polledCount == 0);
        if (blind)
        {
            ++blindCycles;
        }
        ++cycleIndex;

        // ---- Ground-truth coverage labels and trigger confusion matrix -----
        const uint32_t vis868 = VisibleCount(dep, CA_TBL_TIER_868, pos);
        const uint32_t vis2p4 = VisibleCount(dep, CA_TBL_TIER_2P4, pos);
        uint32_t visUwb = VisibleCount(dep, CA_TBL_TIER_UWB, pos);
        // THE GROUND-TRUTH LABEL COULD NOT SEE THE ONLY SOURCE OF UWB.
        //
        // VisibleCount() reads dep.anchors, the STATIC deployment. Under the tri-band
        // correction tier 3 comes from FINDERS, which are not in that list, so visUwb
        // was structurally 0 and shouldUwb structurally false. Every escalation to UWB
        // was therefore scored a FALSE POSITIVE and trig_f1 was 0.000 by construction:
        // measured 0 tp / 21 fp / 0 fn / 459 tn on a run where the node genuinely had
        // three UWB neighbours on 12 cycles.
        //
        // The comment at the top of this file records this exact symptom occurring
        // once before -- "VisibleCount() returned 0 at every step of every arm, visUwb
        // was identically zero, and trigF1 was therefore 0.000 on all six arms" -- and
        // it was fixed then by widening the label radius. This is the same symptom from
        // a different cause: the radius is fine, the POPULATION is wrong.
        //
        // Off by default because it changes a reported column and every published
        // trigger figure was computed without it. It changes no decision: shouldUwb
        // feeds the confusion matrix and nothing else.
        if (uwbCoverageIncludesFinders)
        {
            const double rUwb = dep.nominalRangeM[CA_TBL_TIER_UWB];
            for (size_t fi = 0; fi < finderPos.size(); ++fi)
            {
                const double dx = finderPos[fi].x - pos.x;
                const double dy = finderPos[fi].y - pos.y;
                if (std::sqrt(dx * dx + dy * dy) <= rUwb)
                {
                    ++visUwb;
                }
            }
        }
        const bool shouldUwb = (visUwb >= 3);
        const bool onUwb = (active == CA_TBL_TIER_UWB);
        if (shouldUwb)
        {
            ++inCov;
            if (onUwb)
            {
                ++tp;
            }
            else
            {
                ++fn;
            }
        }
        else
        {
            if (onUwb)
            {
                ++fp;
            }
            else
            {
                ++tn;
            }
        }

        // C4. T_deg from the slope of the last five PCS values, T_batt from energy
        // spent against a STATED capacity. Harness form is 1 - exp(-k*max(0,-slope));
        // k = 1.0 here and is OURS, not calibrated -- calibrating it is part of
        // CAL-09, which this mechanism exists to produce.
        pcsHist.push_back(d.pcs);
        while (pcsHist.size() > 5)
        {
            pcsHist.pop_front();
        }
        if (pcsHist.size() >= 3)
        {
            const size_t np = pcsHist.size();
            double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
            for (size_t i = 0; i < np; ++i)
            {
                const double x = static_cast<double>(i);
                sx += x;
                sy += pcsHist[i];
                sxx += x * x;
                sxy += x * pcsHist[i];
            }
            const double den = static_cast<double>(np) * sxx - sx * sx;
            const double slope =
                (den != 0.0) ? ((static_cast<double>(np) * sxy - sx * sy) / den) : 0.0;
            pcsDegradation = 1.0 - std::exp(-std::max(0.0, -slope));
        }
        batteryUsedFrac = (arbBatteryCapJ > 0.0)
                              ? std::min(1.0, energy.GetTotalJ() / arbBatteryCapJ)
                              : 0.0;

        const double err = std::hypot(d.estX - pos.x, d.estY - pos.y);
        errors.push_back(err);

        // ---- RIVAL SYSTEMS, same measurements, same cycle --------------------
        // Each holds its previous estimate when it cannot solve, exactly as the
        // EKF coasts. Charging them for silence would manufacture the result.
        {
            double mx = 0.0, my = 0.0;
            if (BaselineMultilaterate(meas, mx, my))
            {
                blMlatX = mx;
                blMlatY = my;
                blMlatHave = true;
                ++mlatSolved;
            }
            if (blMlatHave)
            {
                errorsMlat.push_back(std::hypot(blMlatX - pos.x, blMlatY - pos.y));
            }

            // Weighted centroid: 1/range, so a near anchor pulls harder. One
            // anchor is enough, which is the point of a range-free method.
            double wsum = 0.0, wx = 0.0, wy = 0.0;
            double bestR = -1.0, bestX = 0.0, bestY = 0.0;
            for (size_t i = 0; i < meas.size(); ++i)
            {
                const double r = std::max(1.0, meas[i].rangeM);
                const double w = 1.0 / r;
                wsum += w;
                wx += w * meas[i].anchorX;
                wy += w * meas[i].anchorY;
                if (bestR < 0.0 || meas[i].rangeM < bestR)
                {
                    bestR = meas[i].rangeM;
                    bestX = meas[i].anchorX;
                    bestY = meas[i].anchorY;
                }
            }
            if (wsum > 0.0)
            {
                blWclX = wx / wsum;
                blWclY = wy / wsum;
                blWclHave = true;
            }
            if (blWclHave)
            {
                errorsWcl.push_back(std::hypot(blWclX - pos.x, blWclY - pos.y));
            }
            if (bestR >= 0.0)
            {
                blProxX = bestX;
                blProxY = bestY;
                blProxHave = true;
            }
            if (blProxHave)
            {
                errorsProx.push_back(std::hypot(blProxX - pos.x, blProxY - pos.y));
            }

            // Last known position: no direction is known, so the estimate never
            // moves. This is "no system" and it is NOT blind search -- a real
            // party extrapolates from where the subject was last seen.
            errorsLkp.push_back(std::hypot(blLkpX - pos.x, blLkpY - pos.y));
        }
        if (d.hadFix)
        {
            errorsFixed.push_back(err);
            // FIRST, not best: a search party is dispatched on the first usable
            // answer, not on the best one the run eventually produces.
            if (tFirstFixS < 0.0)
            {
                tFirstFixS = t;
            }
            if (tFirst100mS < 0.0 && err <= 100.0)
            {
                tFirst100mS = t;
            }
        }
        steps++;

        // DC-10: what every node holds of every other node, once per cycle.
        if (entriesLog)
        {
            std::map<uint16_t, std::map<uint16_t, CaTblTableEntry>> held;
            for (size_t mi = 0; mi < macs.size(); ++mi)
            {
                const uint16_t nid = macs[mi].GetNodeId();
                const std::map<uint16_t, CaTblTableEntry>& tb = macs[mi].GetTable();
                for (std::map<uint16_t, CaTblTableEntry>::const_iterator it = tb.begin();
                     it != tb.end(); ++it)
                {
                    if (!it->second.valid || it->first == nid)
                    {
                        continue;
                    }
                    std::map<uint16_t, CaTblTableEntry>& m = held[nid];
                    std::map<uint16_t, CaTblTableEntry>::iterator ex = m.find(it->first);
                    if (ex == m.end() || CaTblSeqNewer(it->second.seqNo, ex->second.seqNo))
                    {
                        m[it->first] = it->second;
                    }
                }
            }
            for (const auto& h : held)
            {
                for (const auto& en : h.second)
                {
                    entryRow("held", t, h.first, en.second, ",");
                }
            }
        }

        if (writeTrace)
        {
            trace << std::fixed << std::setprecision(4) << t << "," << CaTblPolicyModeName(mode)
                  << "," << seed << "," << CaTblTierName(d.activeTier) << ","
                  << CaTblTierName(d.nextTier) << "," << CaTblTierName(fixTier) << ","
                  << (usedFloor ? 1 : 0) << "," << pos.x << "," << pos.y << "," << d.estX
                  << "," << d.estY << "," << err << "," << d.pcs << "," << d.gdop << ","
                  << d.traceSigma << "," << d.mahalanobis << "," << d.nAnchorsUsed << ","
                  << d.nNlos << "," << d.nNlosPropagation << "," << d.nHiddenChannel
                  << "," << (d.hadFix ? 1 : 0) << "," << (d.escalated ? 1 : 0) << ","
                  << (d.deescalated ? 1 : 0) << "," << (d.suspect ? 1 : 0) << ","
                  << (d.measurementRejected ? 1 : 0) << "," << vis868 << "," << vis2p4 << ","
                  << visUwb << "," << (shouldUwb ? 1 : 0) << "," << std::setprecision(9)
                  << energy.GetTotalJ() << ","
                  << nodeMac[0]->GetNeighbourCount(CA_TBL_TIER_868, t) << ","
                  << nodeMac[1]->GetNeighbourCount(CA_TBL_TIER_2P4, t) << ","
                  << nodeMac[2]->GetNeighbourCount(CA_TBL_TIER_UWB, t) << ","
                  << polledCount << "," << (blind ? 1 : 0) << "," << directEngineCalls << ","
                  << provider.GetEvaluations() << "," << std::setprecision(12) << d.estX << ","
                  << d.estY << "," << d.meanSigmaM << "," << d.meanSigmaRawM << ","
                  << d.sigmaGeoM << "," << (floorRanged ? 1 : 0) << ","
                  << meas.size() << "\n";
        }

        // DC-3. Ranges reach the base station on the measuring node's entry: once a RESPONSE
        // delivered to a gateway (TA-1a) carried an entry of that node stamped after the
        // cycle they were taken in. Gateway tables never merge a field node's entry in
        // this model, so the answer to the base station's query is the only way there.
        if (spec.caseCRelative)
        {
            for (std::deque<CaseCPending>::iterator it = caseCPending.begin();
                 it != caseCPending.end();)
            {
                std::map<uint32_t, double>::const_iterator cb = caseCBaseT.find(it->node);
                bool there = cb != caseCBaseT.end() && cb->second > it->t + 1e-6;
                const bool late = (t - it->t) > kCaseCWindowS;
                if (!late && !there)
                {
                    ++it;
                    continue;
                }
                there = there && !late; // an answer after the window is too late for it
                for (size_t k = 0; k < it->r.size(); ++k)
                {
                    *caseCRangeLog << std::fixed << std::setprecision(4) << it->t << ','
                                   << it->r[k].from << ',' << it->r[k].to << ','
                                   << static_cast<int>(it->r[k].tier) << ',' << std::setprecision(6)
                                   << it->r[k].rangeM << ',' << it->r[k].trueM << ','
                                   << std::setprecision(4) << (there ? t : -1.0) << '\n';
                }
                if (there)
                {
                    ++caseCDelivered;
                    caseCAtBase[it->t].push_back(*it);
                }
                else
                {
                    ++caseCDropped;
                }
                it = caseCPending.erase(it);
            }
            // A cycle's window has closed: assemble what reached the base station for it.
            for (std::map<double, std::vector<CaseCPending>>::iterator it = caseCAtBase.begin();
                 it != caseCAtBase.end() && (t - it->first) > kCaseCWindowS;)
            {
                std::map<std::pair<uint32_t, uint32_t>, std::vector<double>> pr;
                std::map<uint32_t, Vector> truth;
                for (size_t a = 0; a < it->second.size(); ++a)
                {
                    const CaseCPending& cp = it->second[a];
                    for (size_t k = 0; k < cp.r.size(); ++k)
                    {
                        const uint32_t u = std::min(cp.r[k].from, cp.r[k].to);
                        const uint32_t v = std::max(cp.r[k].from, cp.r[k].to);
                        pr[std::make_pair(u, v)].push_back(cp.r[k].rangeM);
                    }
                    for (std::map<uint32_t, Vector>::const_iterator tt = cp.truth.begin();
                         tt != cp.truth.end(); ++tt)
                    {
                        truth[tt->first] = tt->second;
                    }
                }
                // Connected sets over the measured pairs.
                std::map<uint32_t, uint32_t> root;
                std::function<uint32_t(uint32_t)> find = [&](uint32_t x) {
                    while (root[x] != x)
                    {
                        x = root[x] = root[root[x]];
                    }
                    return x;
                };
                for (std::map<std::pair<uint32_t, uint32_t>, std::vector<double>>::const_iterator
                         e = pr.begin();
                     e != pr.end(); ++e)
                {
                    if (!root.count(e->first.first)) root[e->first.first] = e->first.first;
                    if (!root.count(e->first.second)) root[e->first.second] = e->first.second;
                    root[find(e->first.first)] = find(e->first.second);
                }
                std::map<uint32_t, std::vector<uint32_t>> sets;
                for (std::map<uint32_t, uint32_t>::const_iterator r = root.begin(); r != root.end(); ++r)
                {
                    sets[find(r->first)].push_back(r->first);
                }
                for (std::map<uint32_t, std::vector<uint32_t>>::const_iterator st = sets.begin();
                     st != sets.end(); ++st)
                {
                    const std::vector<uint32_t>& mem = st->second; // ascending ids
                    const size_t kk = mem.size();
                    const double inf = std::numeric_limits<double>::infinity();
                    std::vector<std::vector<double>> dm(kk, std::vector<double>(kk, inf));
                    size_t measured = 0;
                    for (size_t i = 0; i < kk; ++i)
                    {
                        dm[i][i] = 0.0;
                        for (size_t j = i + 1; j < kk; ++j)
                        {
                            std::map<std::pair<uint32_t, uint32_t>, std::vector<double>>::const_iterator
                                e = pr.find(std::make_pair(mem[i], mem[j]));
                            if (e != pr.end())
                            {
                                double m = 0.0;
                                for (size_t q = 0; q < e->second.size(); ++q)
                                {
                                    m += e->second[q] / static_cast<double>(e->second.size());
                                }
                                dm[i][j] = dm[j][i] = m;
                                ++measured;
                            }
                        }
                    }
                    size_t completed = 0;
                    for (size_t i = 0; i < kk; ++i)
                    {
                        for (size_t j = i + 1; j < kk; ++j)
                        {
                            completed += std::isinf(dm[i][j]) ? 1 : 0;
                        }
                    }
                    for (size_t m = 0; m < kk; ++m) // Floyd: a missing pair by shortest path
                    {
                        for (size_t i = 0; i < kk; ++i)
                        {
                            for (size_t j = 0; j < kk; ++j)
                            {
                                dm[i][j] = std::min(dm[i][j], dm[i][m] + dm[m][j]);
                            }
                        }
                    }
                    std::ostringstream ids;
                    std::ostringstream off;
                    std::ostringstream tru;
                    for (size_t i = 0; i < kk; ++i)
                    {
                        ids << (i ? ";" : "") << mem[i];
                        const Vector tv = truth.count(mem[i]) ? truth[mem[i]] : Vector(0, 0, 0);
                        tru << (i ? ";" : "") << std::fixed << std::setprecision(3) << tv.x << ':' << tv.y;
                    }
                    double cohesion = 0.0;
                    double stress = 0.0;
                    if (kk >= 3)
                    {
                        const CaTblRelativeShape sh = CaTblRelativeShapeMds(dm);
                        stress = sh.stressM;
                        for (size_t i = 0; i < kk; ++i)
                        {
                            off << (i ? ";" : "") << std::fixed << std::setprecision(3)
                                << (sh.ok ? sh.x[i] : 0.0) << ':' << (sh.ok ? sh.y[i] : 0.0);
                            for (size_t j = i + 1; sh.ok && j < kk; ++j)
                            {
                                cohesion = std::max(cohesion, std::hypot(sh.x[i] - sh.x[j], sh.y[i] - sh.y[j]));
                            }
                        }
                        ++caseCShapes;
                        caseCCompleted += completed;
                    }
                    else
                    {
                        cohesion = dm[0][1]; // a pair: its distance, and nothing else
                        off << "-";
                        ++caseCPairs;
                    }
                    *caseCShapeLog << std::fixed << std::setprecision(4) << it->first << ',' << kk
                                   << ',' << ids.str() << ',' << measured << ',' << completed << ','
                                   << std::setprecision(6) << cohesion << ',' << stress << ','
                                   << off.str() << ',' << tru.str() << '\n';
                }
                it = caseCAtBase.erase(it);
            }
        }

        // L.1.1. The relays' position fixes, routed. LAST in the cycle on purpose: every
        // frame takes its own slot after the cycle instant, so no ranging exchange of this
        // cycle can be evaluated after one of them.
        if (router)
        {
            router->Cycle(tags, peerMacs, engine868, engine2p4, engineUwb, t, now);
        }
        if (tierRouters[0])
        {
            // NW-2. Who is on which tier this cycle: the measured tag on `active`, a subject
            // on the tier it ranged on, every other relay on --relayTier.
            const int rrtI = static_cast<int>(std::min<uint32_t>(CA_TBL_TIER_COUNT - 1, relayTier));
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                FixRouter& fr = *tierRouters[ti];
                fr.tag0Pos = pos;
                for (size_t k = 0; k < fr.n; ++k)
                {
                    const int tk = (k == 0) ? static_cast<int>(active)
                                            : (subjCycleTier[k] >= 0 ? subjCycleTier[k] : rrtI);
                    fr.part[k] = (tk == ti) ? 1 : 0;
                }
                fr.Cycle(tags, peerMacs, engine868, engine2p4, engineUwb, t, now);
            }
        }
    }

    if (writeTrace)
    {
        trace.close();
    }

    ArmResult res;
    res.arm = CaTblPolicyModeName(mode);
    res.seed = seed;
    res.steps = steps;
    res.durationS = durationS;

    double sumSq = 0.0;
    for (size_t i = 0; i < errors.size(); ++i)
    {
        sumSq += errors[i] * errors[i];
    }
    res.rmseM = errors.empty() ? 0.0 : std::sqrt(sumSq / static_cast<double>(errors.size()));
    res.cep50M = Percentile(errors, 0.50);
    res.cep95M = Percentile(errors, 0.95);
    res.maxErrM = errors.empty() ? 0.0 : *std::max_element(errors.begin(), errors.end());

    // Rival systems. Same measurements, same cycles, same seed -- so a paired
    // comparison against the EKF columns above has nothing else moving in it.
    auto rmseOf = [](const std::vector<double>& v) {
        if (v.empty())
        {
            return 0.0;
        }
        double q = 0.0;
        for (size_t i = 0; i < v.size(); ++i)
        {
            q += v[i] * v[i];
        }
        return std::sqrt(q / static_cast<double>(v.size()));
    };
    res.rmseMlatM = rmseOf(errorsMlat);
    res.cep95MlatM = Percentile(errorsMlat, 0.95);
    res.rmseWclM = rmseOf(errorsWcl);
    res.cep95WclM = Percentile(errorsWcl, 0.95);
    res.rmseProxM = rmseOf(errorsProx);
    res.cep95ProxM = Percentile(errorsProx, 0.95);
    res.rmseLkpM = rmseOf(errorsLkp);
    res.cep95LkpM = Percentile(errorsLkp, 0.95);
    res.mlatSolvedCycles = mlatSolved;

    double sumSqFixed = 0.0;
    for (size_t i = 0; i < errorsFixed.size(); ++i)
    {
        sumSqFixed += errorsFixed[i] * errorsFixed[i];
    }
    res.rmseFixedM =
        errorsFixed.empty() ? 0.0
                            : std::sqrt(sumSqFixed / static_cast<double>(errorsFixed.size()));
    res.cep50FixedM = Percentile(errorsFixed, 0.50);
    res.meanErrM =
        errors.empty() ? 0.0
                       : std::accumulate(errors.begin(), errors.end(), 0.0) /
                             static_cast<double>(errors.size());
    res.meanErrFixedM =
        errorsFixed.empty()
            ? 0.0
            : std::accumulate(errorsFixed.begin(), errorsFixed.end(), 0.0) /
                  static_cast<double>(errorsFixed.size());
    res.tFirstFixS = tFirstFixS;
    res.tFirst100mS = tFirst100mS;
    res.fixCycles = static_cast<uint64_t>(errorsFixed.size());

    res.totalJ = energy.GetTotalJ();
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        const CaTblTier tt = static_cast<CaTblTier>(i);
        const TierEnergyAccount& acc = energy.GetAccount(tt);
        res.tierJ[i] = energy.GetTierJ(tt);
        res.tierCycles[i] = policy.GetTierCycles(tt);
        res.airtimeS[i] = acc.txTimeS;
        res.txJ[i] = acc.txJ;
        res.rxJ[i] = acc.rxJ;
        res.cadJ[i] = acc.cadJ;
        res.idleJ[i] = acc.idleJ;
        res.sleepJ[i] = acc.sleepJ;
        res.meanNeighbours[i] =
            (steps > 0) ? nbrSum[i] / static_cast<double>(steps) : 0.0;
    }
    res.blindCycles = blindCycles;
    res.neighbourInsertions = nodeMac[0]->GetNeighbourInsertions() +
                              nodeMac[1]->GetNeighbourInsertions() +
                              nodeMac[2]->GetNeighbourInsertions();
    res.macTx = 0;
    res.macCollisions = 0;
    res.macCaptures = 0;
    res.macDeliveries = 0;
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        res.macTx += chan[i].GetTotalTransmissions();
        res.macCollisions += chan[i].GetTotalCollisions();
        res.macCaptures += chan[i].GetTotalCaptures();
        res.macDeliveries += chan[i].GetTotalDeliveries();
    }
    if (subj)
    {
        // MS-1. One subject_summary.csv row per measured subject. Subject 1 is tag 0 and
        // its row is built from the very numbers arm_summary.csv prints, in its format, so
        // the two can be compared column for column.
        auto row = [&](uint32_t id, uint64_t st, const std::vector<double>& er,
                       uint64_t fixc, const CaTblEnergyModel& en, const CaTblPolicy& po,
                       uint64_t req) {
            double q = 0.0;
            for (size_t i = 0; i < er.size(); ++i)
            {
                q += er[i] * er[i];
            }
            std::ostringstream o;
            o << CaTblPolicyModeName(mode) << ',' << seed << ',' << id << ',' << st << ','
              << std::fixed << std::setprecision(4)
              << (er.empty() ? 0.0 : std::sqrt(q / static_cast<double>(er.size()))) << ','
              << Percentile(er, 0.50) << ',' << Percentile(er, 0.95) << ','
              << (er.empty() ? 0.0 : *std::max_element(er.begin(), er.end())) << ',' << fixc
              << ',' << std::setprecision(9) << en.GetTotalJ();
            for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
            {
                o << ',' << en.GetTierJ(static_cast<CaTblTier>(k));
            }
            for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
            {
                o << ',' << po.GetTierCycles(static_cast<CaTblTier>(k));
            }
            for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
            {
                o << ',' << en.GetAccount(static_cast<CaTblTier>(k)).txJ;
            }
            for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
            {
                o << ',' << en.GetAccount(static_cast<CaTblTier>(k)).sleepJ;
            }
            o << ',' << po.GetEscalationCount() << ',' << po.GetDeescalationCount() << ','
              << po.GetNoFixCount() << ',' << req;
            return o.str();
        };
        res.subjectRows.push_back(row(static_cast<uint32_t>(nodeId), steps, errors,
                                      static_cast<uint64_t>(errorsFixed.size()), energy, policy,
                                      tag0RequestFrames));
        for (uint32_t pt = 1; pt < spec.measuredSubjects; ++pt)
        {
            const CaTblTagState& sb = tags[pt];
            res.subjectRows.push_back(row(sb.nodeId, sb.steps, sb.errors,
                                          static_cast<uint64_t>(sb.errorsFixed.size()),
                                          sb.energy, sb.policy, subj->extra[pt - 1].requests));
        }
        std::ostringstream line; // its own stream: cout's precision is left alone
        line << "    MS-1 subjects: " << spec.measuredSubjects << ";";
        for (uint32_t pt = 1; pt < spec.measuredSubjects; ++pt)
        {
            const CaTblTagState& sb = tags[pt];
            const SubjectExtra& ex = subj->extra[pt - 1];
            line << " subj " << sb.nodeId << " steps " << sb.steps << " fixes "
                 << sb.errorsFixed.size() << " esc " << sb.policy.GetEscalationCount()
                 << " requests " << ex.requests << " pre-empted " << sb.handshakePreemptions
                 << " deferred-away " << ex.deferredAway << ";";
        }
        std::cout << line.str() << "\n";
    }
    if (grantLog)
    {
        grantLog->close(); // NW-7: before the digest, and before the MACs can call back
        grantLog.reset();
    }
    if (sessLog)
    {
        // TA-1a. Closed before the digest; then what the base station's session saw.
        sessLog->close();
        sessLog.reset();
        std::map<uint32_t, CaTblSession>::const_iterator si = sessions.Sessions().find(sessionId);
        std::ostringstream line;
        line << "    TA-1 base-station session (" << CaTblPolicyModeName(mode)
             << "): answers delivered " << baseResults << ", path "
             << (si != sessions.Sessions().end() ? si->second.Path().c_str() : "none")
             << "; first coarse/precise (s):";
        if (si != sessions.Sessions().end())
        {
            const std::vector<uint8_t>& tg = si->second.Targets();
            for (size_t k = 0; k < tg.size(); ++k)
            {
                line << " m" << static_cast<int>(tg[k]) << " " << std::fixed
                     << std::setprecision(1) << si->second.TimeToCoarse(tg[k]) << "/"
                     << si->second.TimeToPrecise(tg[k]);
            }
            if (tg.size() > 1)
            {
                // TA-4: the batch's spread in time-to-precision, undefined below two.
                const double spread = sessions.BatchPrecisionSpread(sessionId);
                line << "; batch precision spread ";
                if (spread < 0.0)
                {
                    line << "UNDEFINED";
                }
                else
                {
                    line << std::fixed << std::setprecision(1) << spread << " s";
                }
            }
        }
        std::cout << line.str() << "\n";
    }
    if (finderReqLog)
    {
        // TA-1b. Closed before the digest.
        finderReqLog->close();
        finderReqLog.reset();
        std::cout << "    TA-1b finder requester escalation (" << CaTblPolicyModeName(mode)
                  << "): requests own-arbiter " << finderReq[0] << ", named " << finderReq[1]
                  << ", unroutable " << finderReq[2] << "; granted " << finderReq[3]
                  << ", queued " << finderReq[4] << ", refused " << finderReq[5] << "\n";
    }
    if (reqLog)
    {
        // TA-2. Closed before the digest; what was asked and what the targets did.
        reqLog->close();
        reqLog.reset();
        uint64_t acc = 0, ref = 0;
        for (uint32_t pt = 0; pt < spec.measuredSubjects && pt < tags.size(); ++pt)
        {
            acc += tags[pt].policy.GetOboAccepted();
            ref += tags[pt].policy.GetOboRefused();
        }
        std::cout << "    TA-2 requester escalation (" << CaTblPolicyModeName(mode)
                  << "): granted " << reqCounts[0] << ", queued " << reqCounts[1] << ", refused "
                  << reqCounts[2] << ", unroutable " << reqCounts[3] << "; at the targets accepted "
                  << acc << ", refused " << ref << "\n";
    }
    for (size_t fi = 0; fi < finderSearch.size(); ++fi)
    {
        // TA-1b. One line per finder's own search.
        const FinderSearch& fs = finderSearch[fi];
        const CaTblSession& se = fs.mgr.Sessions().at(fs.sid);
        std::ostringstream line;
        line << "    TA-1b finder session (" << CaTblPolicyModeName(mode) << "): finder "
             << finderIds[fi] << " target m" << static_cast<int>(fs.target) << ", answers "
             << fs.results << ", path " << (se.Path().empty() ? "SEARCHING" : se.Path())
             << ", first coarse/precise (s) " << std::fixed << std::setprecision(1)
             << se.TimeToCoarse(fs.target) << "/" << se.TimeToPrecise(fs.target) << ", cancelled "
             << fs.cancelledAtS;
        std::cout << line.str() << "\n";
    }
    if (hopLog)
    {
        // NW-1. Closed before the digest.
        hopLog->close();
        hopLog.reset();
        std::cout << "    NW-1 hop provenance (" << CaTblPolicyModeName(mode)
                  << "): fixes by depth of the fused set 0:" << hopFixesByDepth[0] << " 1:"
                  << hopFixesByDepth[1] << " 2:" << hopFixesByDepth[2] << " 3+:"
                  << hopFixesByDepth[3] << "\n";
    }
    if (spec.gossipQueue && gossip)
    {
        // NW-6. Queue items gossiped and heard, double-queued requests detected, and queued
        // requests given up to a higher-id anchor.
        uint64_t sent = 0, heard = 0, yields = 0, doubled = 0;
        for (size_t m = 0; m < macs.size(); ++m)
        {
            sent += macs[m].GetQueueItemsSent();
            heard += macs[m].GetQueueItemsHeard();
            yields += macs[m].GetQueueYields();
        }
        for (std::map<uint16_t, CaTblGrantCoordinator*>::const_iterator gc = gossip->byId.begin();
             gc != gossip->byId.end(); ++gc)
        {
            doubled += gc->second->DoubleQueued();
        }
        std::cout << "    NW-6 queue gossip (" << CaTblPolicyModeName(mode) << "): queue items sent "
                  << sent << ", heard " << heard << ", double-queued " << doubled
                  << ", queued requests yielded to a higher id " << yields << " ("
                  << gossip->finder1.size() << " finders gossiping)\n";
    }
    if (arbLog)
    {
        // NW-5. Unhooked and closed before the digest; then the arbiters' sums.
        uint64_t g = 0, pre = 0, aged = 0, q = 0, oor = 0, ex = 0, gFinder = 0;
        for (std::map<uint16_t, CaTblMac*>::iterator am = arbMacById.begin();
             am != arbMacById.end(); ++am)
        {
            am->second->SetArbLog(nullptr);
            g += am->second->GetArbGrants();
            pre += am->second->GetArbPreemptions();
            aged += am->second->GetArbAgedGrants();
            q += am->second->GetArbQueued();
            oor += am->second->GetArbOutOfRange();
            ex += am->second->GetArbExpired();
            if (am->first >= 501)
            {
                gFinder += am->second->GetArbGrants();
            }
        }
        arbLog->close();
        arbLog.reset();
        std::cout << "    NW-5 arbitration (" << CaTblPolicyModeName(mode) << "): grants " << g
                  << " (by finders " << gFinder << "), pre-emptions " << pre
                  << ", from the queue " << aged << ", queued " << q << ", out of range " << oor
                  << ", dropped from the queue " << ex << "\n";
    }
    if (spec.queryTriggeredEscalation)
    {
        // NW-4. URGENT queries latched by every MAC; the hook's counts, tag 0 and subjects.
        uint64_t latched = 0;
        for (size_t m = 0; m < macs.size(); ++m)
        {
            latched += macs[m].GetUrgentQueries();
        }
        uint64_t a[5] = {0, 0, 0, 0, 0};
        for (uint32_t pt = 0; pt < spec.measuredSubjects && pt < tags.size(); ++pt)
        {
            const CaTblPolicy& po = tags[pt].policy;
            a[0] += po.GetQueryEscRequests();
            a[1] += po.GetQueryEscFired();
            a[2] += po.GetQueryEscalations();
            a[3] += po.GetQueryEscSuppressed();
            a[4] += po.GetQueryEscIgnored();
        }
        std::cout << "    NW-4 query-triggered escalation (" << CaTblPolicyModeName(mode)
                  << "): URGENT queries latched " << latched << "; hook armed " << a[0]
                  << ", fired " << a[1] << ", escalations " << a[2] << ", suppressed " << a[3]
                  << ", ignored " << a[4] << "\n";
    }
    if (spec.queryV2)
    {
        // NW-9. Logs closed and unhooked before the digest; then the sums over every MAC.
        uint64_t an = 0, px = 0, sp = 0, rd = 0, hd = 0, en = 0;
        for (size_t m = 0; m < macs.size(); ++m)
        {
            macs[m].SetQueryLog(nullptr);
            macs[m].SetResponseCallback(nullptr);
            an += macs[m].GetV2Answered();
            px += macs[m].GetV2Proxied();
            sp += macs[m].GetV2Suppressed();
            rd += macs[m].GetV2Reduced();
            hd += macs[m].GetV2ResponsesHeard();
            en += macs[m].GetV2EntriesHeard();
        }
        respLog->close();
        qv2Log->close();
        std::cout << "    NW-9 queries v2 (" << CaTblPolicyModeName(mode) << "): answered as target "
                  << an << ", by proxy " << px << ", rebroadcasts suppressed " << sp
                  << ", lists reduced " << rd << ", answers heard by requesters " << hd
                  << " (with an entry " << en << ")\n";
    }
    if (spec.grantGatedUwb)
    {
        // NW-7. Tag 0, and subjects 2..N summed.
        auto put = [](std::ostringstream& o, const CaTblPolicy& po) {
            o << "requests " << po.GetUwbRequests() << ", confirmed " << po.GetUwbConfirmed()
              << ", denied " << po.GetUwbDenied() << ", timed out " << po.GetUwbTimeouts()
              << ", late grants " << po.GetUwbLate() << ", aborted " << po.GetUwbAborted()
              << ", cancelled by coverage loss " << po.GetUwbCoverageClears()
              << ", cycles waiting " << po.GetUwbWaitCycles();
        };
        std::ostringstream line;
        line << "    NW-7 grant-gated UWB (" << CaTblPolicyModeName(mode) << "), tag 0: ";
        put(line, policy);
        std::cout << line.str() << "\n";
        if (subj)
        {
            uint64_t a[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            for (uint32_t pt = 1; pt < spec.measuredSubjects; ++pt)
            {
                const CaTblPolicy& po = tags[pt].policy;
                a[0] += po.GetUwbRequests();
                a[1] += po.GetUwbConfirmed();
                a[2] += po.GetUwbDenied();
                a[3] += po.GetUwbTimeouts();
                a[4] += po.GetUwbLate();
                a[5] += po.GetUwbAborted();
                a[6] += po.GetUwbCoverageClears();
                a[7] += po.GetUwbWaitCycles();
            }
            std::cout << "    NW-7 grant-gated UWB (" << CaTblPolicyModeName(mode)
                      << "), subjects 2.." << spec.measuredSubjects
                      << ": requests " << a[0] << ", confirmed " << a[1] << ", denied " << a[2]
                      << ", timed out " << a[3] << ", late grants " << a[4] << ", aborted "
                      << a[5] << ", cancelled by coverage loss " << a[6] << ", cycles waiting "
                      << a[7] << "\n";
        }
    }
    if (taskLog)
    {
        taskLog->close();
        std::ostringstream line;
        line << "    MS-1 tasking (roundrobin):";
        for (size_t fi = 0; fi < taskSweep.size(); ++fi)
        {
            line << " finder " << finderIds[fi] << " -> subject "
                 << tags[fi % spec.measuredSubjects].nodeId << " sweep " << taskSweep[fi]
                 << " converge " << taskConverge[fi] << " blind " << taskBlind[fi] << ";";
        }
        std::cout << line.str() << "\n";
    }
    res.escalations = policy.GetEscalationCount();
    res.deescalations = policy.GetDeescalationCount();
    res.suspects = policy.GetSuspectCount();
    res.noFixCycles = policy.GetNoFixCount();
    // A3 counter, to stdout rather than to arm_summary.csv: a new column changes
    // the CSV header for every run and breaks byte-identity against baseline_v7.
    if (handshakeIntraCycleDefers > 0)
    {
        std::cout << "    A3 intra-cycle handshake deferrals: "
                  << handshakeIntraCycleDefers << ", mean wait "
                  << std::fixed << std::setprecision(3)
                  << (handshakeIntraCycleWaitS
                      / static_cast<double>(handshakeIntraCycleDefers))
                  << " s -- each is a next-cycle ranging exchange NOT pre-empted\n";
    }
    // C4. Arbiter outcomes, summed over the ANCHOR MACs -- the arbiter runs on the
    // anchors, not on the tag. Per-target grant counts are printed so a fairness delta
    // is measurable rather than asserted. To stdout and not arm_summary.csv: a new
    // column changes the CSV header for every run and breaks byte-identity.
    {
        uint64_t g = 0, dn = 0, pe = 0;
        std::map<uint16_t, uint64_t> byTarget;
        for (std::deque<CaTblMac>::const_iterator mi = macs.begin(); mi != macs.end(); ++mi)
        {
            if (mi->GetIdentityClass() != CA_TBL_ID_ANCHOR)
            {
                continue;
            }
            g += mi->GetArbGrants();
            dn += mi->GetArbDenials();
            pe += mi->GetArbPreemptions();
            const std::map<uint16_t, uint64_t>& t = mi->GetGrantsByTarget();
            for (std::map<uint16_t, uint64_t>::const_iterator ti = t.begin(); ti != t.end(); ++ti)
            {
                byTarget[ti->first] += ti->second;
            }
        }
        if (g + dn > 0)
        {
            std::cout << "    C4 arbitration: granted " << g << ", denied " << dn
                      << ", pre-empted " << pe << "\n";
            for (std::map<uint16_t, uint64_t>::const_iterator ti = byTarget.begin();
                 ti != byTarget.end();
                 ++ti)
            {
                std::cout << "        target " << ti->first << ": " << ti->second
                          << " grants\n";
            }
        }
    }
    // SY-5. The fault window and what it touched. Printed only under the flags.
    if (linkFault)
    {
        std::ostringstream line; // its own stream: cout's precision is left alone
        line << "    SY-5 link fault: window [" << std::setprecision(9) << linkFault->windowStartS
             << ", " << linkFault->windowEndS << ") s, tx-power drop " << spec.injectTxPowerDropDb
             << " dB, antenna loss " << spec.injectAntennaLossDb << " dB, n "
             << linkFault->n << ", tier-1 skew x"
             << std::pow(10.0, (spec.injectTxPowerDropDb + spec.injectAntennaLossDb) /
                                   (10.0 * linkFault->n))
             << "; ranging links attenuated " << linkFault->rangingLinks
             << ", tier-1 ranges skewed " << linkFault->skewedLinks << "; MAC links attenuated "
             << "tx " << provider.GetFaultTxLinks() << " rx " << provider.GetFaultRxLinks()
             << "\n";
        std::cout << line.str();
    }
    // SY-1. The uplink frames billed, per tier. Printed only under the flag.
    if (edgeUp)
    {
        std::ostringstream line; // its own stream: cout's precision is left alone
        line << "    SY-1 edge uplink: 2.4 GHz frames " << edgeUp->frames[CA_TBL_TIER_2P4]
             << " records " << edgeUp->records[CA_TBL_TIER_2P4] << " airtime "
             << std::setprecision(9) << edgeUp->airS[CA_TBL_TIER_2P4] << " s; UWB frames "
             << edgeUp->frames[CA_TBL_TIER_UWB] << " records "
             << edgeUp->records[CA_TBL_TIER_UWB] << " airtime "
             << edgeUp->airS[CA_TBL_TIER_UWB] << " s\n";
        std::cout << line.str();
    }
    // N.3 / row 55. The subjects' uplink frames, per subject and tier. Only under the flag.
    if (edgeUpS)
    {
        std::ostringstream line;
        line << "    SY-1 edge uplink (subjects, " << CaTblPolicyModeName(mode) << "):";
        for (uint32_t pt = 1; pt < spec.measuredSubjects && pt < edgeUpS->size(); ++pt)
        {
            const EdgeUplinkTally& eu = (*edgeUpS)[pt];
            line << " subject " << tags[pt].nodeId << " 2.4 GHz frames "
                 << eu.frames[CA_TBL_TIER_2P4] << " records " << eu.records[CA_TBL_TIER_2P4]
                 << " airtime " << std::setprecision(9) << eu.airS[CA_TBL_TIER_2P4]
                 << " s, UWB frames " << eu.frames[CA_TBL_TIER_UWB] << " records "
                 << eu.records[CA_TBL_TIER_UWB] << " airtime " << eu.airS[CA_TBL_TIER_UWB] << " s;";
        }
        std::cout << line.str() << "\n";
    }
    // NW-0. The mesh's cost and its guards. Printed only under the flag.
    if (mesh)
    {
        uint64_t asleep = 0;
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            if (nodeMac[ti] != nullptr)
            {
                asleep += nodeMac[ti]->GetAsleepRefused();
            }
        }
        std::ostringstream line; // its own stream: cout's precision is left alone
        line << "    NW-0 mesh: link evaluations " << provider.GetEvaluations() << " (mesh pairs "
             << provider.GetMeshEvaluations() << "), peers kept out of poll sets "
             << mesh->peersSkipped << ", receptions refused asleep " << asleep
             << ", node rebroadcasts billed " << (mesh->billedFrames[0] + mesh->billedFrames[1] +
                                                  mesh->billedFrames[2])
             << " frames " << std::setprecision(9)
             << (mesh->billedAir[0] + mesh->billedAir[1] + mesh->billedAir[2]) << " s\n";
        std::cout << line.str();
    }
    // DC-9. The model witness's counters, split. Printed only under the flag.
    if (witness)
    {
        std::cout << "    DC-9 witness (model): evaluations " << witness->evals << ", refusals "
                  << witness->refusals << ", absorbed by the evidence gate " << witness->absorbed
                  << "\n";
    }
    // DC-13. Entries carried forward before use. Printed only under the flag.
    if (entryPred)
    {
        std::cout << "    DC-13 entry prediction: substitutions " << entryPred->subs
                  << ", moved by their velocity " << entryPred->moved << "\n";
    }
    // DC-4. The single-range cycles and what they did. Printed only under the flag.
    if (caseDst)
    {
        std::cout << "    DC-4 Case D: cycles " << caseDst->cycles << ", updates applied "
                  << caseDst->applied << ", refused by NIS " << caseDst->refused
                  << "; wanted escalation " << caseDst->wanted << ", escalated "
                  << caseDst->escalated << ", demoted " << caseDst->demoted << "\n";
    }
    // DC-1. The run's IMU errors and how often the accumulation restarted. Under the flag.
    if (imuPred)
    {
        std::ostringstream line; // its own stream: cout's precision is left alone
        line << "    DC-1 IMU: bias " << std::setprecision(9) << imuPred->rk.Bias()
             << " rad/s, scale " << imuPred->rk.Scale() << ", cycles " << imuPred->cycles
             << ", resets " << imuPred->resets << "\n";
        std::cout << line.str();
    }
    // DC-11. The two-range cycles Step() decided. Printed only under the flag.
    if (sparse)
    {
        std::cout << "    DC-11 sparse in Step: attempts " << sparse->attempts << ", case A "
                  << sparse->caseA << ", case B " << sparse->caseB << ", refused degenerate "
                  << sparse->refused[1] << " ambiguous " << sparse->refused[2] << " no-prior "
                  << sparse->refused[3] << "; two-range cycles made full by relays "
                  << sparse->fullWithRelays << "\n";
    }
    // MC-2. Triggers, frames billed and transmitted by the measured node, and receptions
    // of them anywhere in the mesh, per kind. Printed only under the flag.
    if (trig)
    {
        uint64_t sentFrames = 0, heardLarge = 0, heardGasp = 0;
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            if (nodeMac[ti] != nullptr)
            {
                sentFrames += nodeMac[ti]->GetTrigFramesSent();
            }
        }
        for (std::deque<CaTblMac>::const_iterator mi = macs.begin(); mi != macs.end(); ++mi)
        {
            heardLarge += mi->GetTrigHeard(1);
            heardGasp += mi->GetTrigHeard(2);
        }
        std::cout << "    MC-2 triggered updates: large correction " << trig->nLarge
                  << " (heard " << heardLarge << "), last gasp " << trig->nGasp << " (heard "
                  << heardGasp << "); frames billed " << (trig->billedLarge + trig->billedGasp)
                  << ", transmitted " << sentFrames << "\n";
    }
    if (strictLog)
    {
        // N.21 strict. Closed before the digest.
        strictLog->close();
        strictLog.reset();
        std::cout << "    N.21 strict pseudo-anchors (" << CaTblPolicyModeName(mode)
                  << "): tag 0 finder ranges on reports " << strictReport << ", dropped for want of a "
                  << "position " << strictDropped << ", reports used uninflated " << strictNoInflate
                  << "; subjects' reports used uninflated " << strictNoInflateS << "\n";
    }
    if (pseudoS)
    {
        // N.21. Closed before the digest.
        pseudoS->close();
        pseudoS.reset();
        std::cout << "    N.21 finder pseudo-anchors (subjects, " << CaTblPolicyModeName(mode)
                  << "): digests asked " << pseudoDigests << ", reported positions used " << pseudoSubs
                  << ", dropped for want of a position " << pseudoNoEntry << "\n";
    }
    if (trigS)
    {
        // N.3 / row 55. Closed before the digest.
        trigS->close();
        trigS.reset();
        std::cout << "    MC-2 triggered updates (subjects, " << CaTblPolicyModeName(mode)
                  << "): large correction " << trigSLarge << ", frames billed " << trigSBilled
                  << (spec.subjectBatteryDeplete ? ", last gasp " : "")
                  << (spec.subjectBatteryDeplete ? std::to_string(trigSGasp) : std::string())
                  << "\n";
    }
    // MC-1. The handshake's outcomes, summed over every MAC. Printed only under the flag.
    if (spec.handshakeFixes)
    {
        uint64_t reqSent = 0, reqRetx = 0, skipped = 0, gs = 0, ds = 0, gh = 0, dh = 0;
        uint64_t settled = 0, am = 0, au = 0;
        for (std::deque<CaTblMac>::const_iterator mi = macs.begin(); mi != macs.end(); ++mi)
        {
            const CaTblMsgCounters& rc = mi->GetCounters(CA_TBL_MSG_ESC_REQUEST);
            reqSent += rc.sent;
            reqRetx += rc.retransmissions;
            skipped += mi->GetHsRequestAcksSkipped();
            gs += mi->GetHsGrantsSent();
            ds += mi->GetHsDeniesSent();
            gh += mi->GetHsGrantsHeard();
            dh += mi->GetHsDeniesHeard();
            settled += mi->GetHsRequestsAnswered();
            am += mi->GetHsAcksMatched();
            au += mi->GetHsAcksUnmatched();
        }
        std::cout << "    MC-1 handshake: ESC_REQUEST sent " << reqSent << " (retransmitted "
                  << reqRetx << "), bare ACKs to a request withheld " << skipped
                  << "; GRANT sent granted " << gs << " denied " << ds << "; heard granted "
                  << gh << " denied " << dh << ", requests settled by a grant " << settled
                  << "; ACKs matched by id " << am << ", unmatched " << au << "\n";
    }
    // L.1.4. Per-gateway gossip traffic, tie-break verdicts and the gossip's own energy.
    // Stdout for the same byte-identity reason as C4 above; the per-verdict record is
    // grant_verdicts_<arm>_seed<N>.csv, which exists only when the flag is on.
    if (gossip)
    {
        gossip->Report(std::cout, durationS);
    }

    // C3. Queries answered and correctly ignored. To stdout rather than
    // arm_summary.csv: a new column changes the CSV header for every run and breaks
    // byte-identity against baseline_v7.
    //
    // SUMMED OVER EVERY MAC, NOT JUST THE MEASURED TAG'S THREE. It used to sum only
    // nodeMac[], which is the measured tag -- so a query addressed to manifest id 2,
    // which belongs to a PEER tag, could only ever be counted as IGNORED. The answer,
    // if it came, was invisible. Every run reported "answered 0" and the zero was a
    // property of the counter rather than of the system.
    {
        uint64_t qa = 0, qi = 0;
        for (size_t i = 0; i < macs.size(); ++i)
        {
            qa += macs[i].GetQueriesAnswered();
            qi += macs[i].GetQueriesIgnored();
        }
        if (qa + qi > 0)
        {
            std::cout << "    C3 identity queries: answered " << qa
                      << ", correctly ignored " << qi << "\n";
        }
    }
    // NW-3. Which radio each tier's query driver actually used, and what its queries cost
    // there. Printed only when --queryTierBinding is given, either value.
    if (!spec.queryTierBinding.empty())
    {
        for (size_t q = 0; q < queries.size(); ++q)
        {
            const CaTblMac* qm = queries[q].mac;
            int radio = -1;
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (qm->GetChannel() == &chan[ti])
                {
                    radio = ti;
                }
            }
            const CaTblMsgCounters& qc = qm->GetCounters(CA_TBL_MSG_QUERY);
            std::ostringstream line; // its own stream: cout's precision is left alone
            line << "    NW-3 query driver tier " << static_cast<int>(queries[q].tier)
                 << ": node " << qm->GetNodeId() << " radio " << radio << ", QUERY sent "
                 << qc.sent << " airtime " << std::setprecision(9) << qc.airtimeS << " s ("
                 << spec.queryTierBinding << ")\n";
            std::cout << line.str();
        }
    }

    // Net sect 4 / Trans sect 1. The counters existed and NOTHING PRINTED THEM, so the
    // positive control for --queryProxy and --reversePath had no observable to check --
    // rule 9, in miniature: the check for a firing was written before anything wrote the
    // firing down. Summed over every MAC in the run, tags and infrastructure alike,
    // because a proxy answer is by definition given by a node that is NOT the target.
    {
        uint64_t proxyStale = 0, uni = 0, flood = 0, proxyOk = 0;
        uint64_t digPush = 0, digNoOp = 0;
        for (size_t i = 0; i < macs.size(); ++i)
        {
            proxyOk += macs[i].GetProxyResponses();
            proxyStale += macs[i].GetProxyRefusedStale();
            uni += macs[i].GetResponsesUnicast();
            flood += macs[i].GetResponsesFlooded();
            digPush += macs[i].GetDigestPushes();
            digNoOp += macs[i].GetDigestNoOps();
        }
        uint64_t suppressed = 0;
        for (size_t i = 0; i < macs.size(); ++i)
        {
            suppressed += macs[i].GetSuppressedTx();
        }
        if (suppressed > 0)
        {
            std::cout << "    FAULT sect 49: " << suppressed
                      << " frames suppressed while node " << injectSilentId
                      << " was silent\n";
        }
        if (spec.deadBatteryNoDraw)
        {
            std::cout << "    N.20 dead radios (" << CaTblPolicyModeName(mode)
                      << "): radio-cycles unbilled, tag 0 " << deadNoDrawT0 << ", subjects "
                      << deadNoDrawS << "; tag 0 escalation handshakes unbilled " << deadNoDrawHs
                      << "\n";
        }
        if (spec.halfDuplex != 0)
        {
            // N.6. Summed over every radio of every node.
            uint64_t ov = 0, dfr = 0, deaf = 0;
            for (size_t i = 0; i < macs.size(); ++i)
            {
                ov += macs[i].GetSelfOverlapTx();
                dfr += macs[i].GetHalfDuplexDeferred();
                deaf += macs[i].GetDeafRx();
            }
            std::cout << "    N.6 half-duplex (" << (spec.halfDuplex == 1 ? "count" : "enforce")
                      << ", " << CaTblPolicyModeName(mode) << "): frames due over the radio's own "
                      << ov << ", sent back to CAD " << dfr << ", receptions while transmitting "
                      << deaf << (spec.halfDuplex == 1 ? " (kept)" : " (lost)") << "\n";
        }
        if (spec.silenceStopsRelay)
        {
            // N.1. Other nodes' frames the silenced radios refused at CAD.
            uint64_t refusedRelays = 0;
            for (size_t i = 0; i < macs.size(); ++i)
            {
                refusedRelays += macs[i].GetSilentRelayRefused();
            }
            std::cout << "    N.1 silenced radios (" << CaTblPolicyModeName(mode)
                      << "): relays refused " << refusedRelays << "\n";
        }
        if (spec.subjectBatteryDeplete)
        {
            // N.3 / row 55. One line per subject whose budget ran out.
            for (uint32_t pt = 1; pt < spec.measuredSubjects && pt < tags.size(); ++pt)
            {
                if (tags[pt].batteryDeadAtS >= 0.0)
                {
                    std::cout << "    FAULT sect 49 (subject " << tags[pt].nodeId
                              << "): battery exhausted at t=" << std::fixed << std::setprecision(1)
                              << tags[pt].batteryDeadAtS << " s (" << batteryDepleteJ
                              << " J); radio silent for the remaining "
                              << (durationS - tags[pt].batteryDeadAtS) << " s\n";
                }
            }
        }
        if (batteryDeadAtS >= 0.0)
        {
            std::cout << "    FAULT sect 49: battery exhausted at t=" << std::fixed
                      << std::setprecision(1) << batteryDeadAtS << " s ("
                      << batteryDepleteJ << " J); radio silent for the remaining "
                      << (durationS - batteryDeadAtS) << " s\n";
        }
        if (deployedAtS >= 0.0)
        {
            std::cout << "    ALERT: " << deployCause << " for " << std::fixed
                      << std::setprecision(0) << searchLaunchOnLossS
                      << " s -- party deployed at t=" << std::setprecision(1)
                      << deployedAtS << " s\n";
        }
        else if (searchLaunchOnLossS > 0.0)
        {
            std::cout << "    ALERT: never triggered -- party never deployed"
                      << (searchLaunchSigmaM > 0.0
                              ? " (contact held and the fix stayed inside the sigma bound)"
                              : " (contact never lost; --searchLaunchSigmaM is off)")
                      << "\n";
        }
        if (staleSteerRefusals > 0)
        {
            std::cout << "    SEARCH: " << staleSteerRefusals
                      << " refusals to steer on an entry past the staleness bound\n";
        }
        if (mobilityFromRange)
        {
            std::cout << "    sect 11.11 MOBILITY: stationary " << mobCounts[0]
                      << ", approaching " << mobCounts[1] << ", receding " << mobCounts[2]
                      << "\n";
        }
        if (spec.reportRecedingPushes)
        {
            // M.9. Summed over every radio of every node.
            uint64_t started = 0, sent = 0;
            for (size_t i = 0; i < macs.size(); ++i)
            {
                started += macs[i].GetRecedingPushesStarted();
                sent += macs[i].GetRecedingPushesSent();
            }
            std::cout << "    M.9 RECEDING pushes (" << CaTblPolicyModeName(mode)
                      << "): handshakes on the RECEDING branch " << started
                      << ", push frames transmitted " << sent
                      << " (the redundant class repeats each up to 3 times)\n";
        }
        if (lifecycleTracking)
        {
            std::cout << "    sect 24-27 LIFECYCLE: tracked "
                      << lifecycle.CountIn(CA_TBL_NODE_TRACKED) << ", degraded "
                      << lifecycle.CountIn(CA_TBL_NODE_DEGRADED) << ", lost "
                      << lifecycle.CountIn(CA_TBL_NODE_LOST) << "; loss events "
                      << lifecycle.TotalLossEvents() << ", reacquisitions "
                      << lifecycle.TotalReacquisitions() << "\n";
        }
        if (sessionLayer)
        {
            const CaTblSession* sess = nullptr;
            std::map<uint32_t, CaTblSession>::const_iterator si =
                sessions.Sessions().find(sessionId);
            if (si != sessions.Sessions().end())
            {
                sess = &si->second;
            }
            std::cout << "    SESSION: path "
                      << (sess ? sess->Path().c_str() : "none") << ", ever precise "
                      << (sess ? (sess->EverPrecise() ? "YES" : "NO") : "-")
                      << ", final state "
                      << (sess ? CaTblSessionStateName(sess->State()) : "none")
                      << ", renders VERIFY " << renderCounts[CA_TBL_RENDER_VERIFY]
                      << " / HIGH " << renderCounts[CA_TBL_RENDER_HIGH]
                      << " / MEDIUM " << renderCounts[CA_TBL_RENDER_MEDIUM]
                      << " / COARSE " << renderCounts[CA_TBL_RENDER_COARSE] << "\n";
            if (batchSearch)
            {
                const double spread = sessions.BatchPrecisionSpread(sessionId);
                std::cout << "    APPLICATION sect 5 batch spread: ";
                if (spread < 0.0)
                {
                    // Undefined below two precise targets, and reported as undefined.
                    // Calling it zero would claim a batch finished together when most of
                    // it was never found -- a lie in the direction that flatters.
                    std::cout << "UNDEFINED (fewer than two targets reached precise)\n";
                }
                else
                {
                    std::cout << std::fixed << std::setprecision(1) << spread << " s\n";
                }
            }
        }
        if (sparseCaseA || caseSolved[0] > 0)
        {
            std::cout << "    sect 7 CASES: D(3+ anchors) " << caseSolved[0]
                      << ", A(2 gateways) " << caseSolved[1]
                      << ", B(anchor+peer) " << caseSolved[2]
                      << ", C(unlocalisable) " << caseSolved[3] << ", floor-solved "
                      << caseSolved[4] << "\n";
            const uint64_t caseTotal = caseSolved[0] + caseSolved[1] + caseSolved[2] +
                                       caseSolved[3] + caseSolved[4];
            // Printed so the census can be CHECKED rather than trusted. If these two
            // disagree the breakdown has a hole, which is exactly the defect this line
            // exists to make visible.
            std::cout << "    sect 7 CASES: classified " << caseTotal << " of " << steps
                      << " cycles" << ((caseTotal == steps) ? " (complete)" : " (INCOMPLETE)")
                      << "\n";
        }
        if (caseAAttempts > 0)
        {
            std::cout << "    sect 7 CASE A/B: attempts " << caseAAttempts << ", admitted "
                      << caseAAdmitted << ", refused ambiguous "
                      << caseARefused[CA_TBL_SPARSE_AMBIGUOUS] << ", degenerate "
                      << caseARefused[CA_TBL_SPARSE_DEGENERATE] << ", no-prior "
                      << caseARefused[CA_TBL_SPARSE_NO_PRIOR] << "\n";
        }
        if (digPush + digNoOp > 0)
        {
            std::cout << "    ANTI-ENTROPY: digest pushes " << digPush
                      << ", digests answered with nothing (asker already current) "
                      << digNoOp << "\n";
        }
        if (proxyStale + uni + flood + proxyOk > 0)
        {
            std::cout << "    NET4/TRANS1: proxy answered " << proxyOk << ", proxy refused stale "
                      << proxyStale << ", responses unicast " << uni << ", flooded "
                      << flood << "\n";
        }
        if (spec.reversePathHop)
        {
            // NW-8. Summed over every MAC: a hop is sent by the answerer and by each
            // forwarder, and only the requester counts a delivery.
            uint64_t hops = 0, fwd = 0, fl = 0, expd = 0, del = 0, dup = 0;
            for (size_t i = 0; i < macs.size(); ++i)
            {
                hops += macs[i].GetRpUnicastHops();
                fwd += macs[i].GetRpForwarded();
                fl += macs[i].GetRpFloods();
                expd += macs[i].GetRpExpired();
                del += macs[i].GetRpDelivered();
                dup += macs[i].GetRpDuplicates();
            }
            std::cout << "    NW-8 reverse path: unicast hops " << hops << ", forwarded " << fwd
                      << ", floods " << fl << ", expired entries " << expd
                      << ", delivered to the requester " << del << ", duplicates absorbed "
                      << dup << "\n";
        }
        if (queryProxy)
        {
            WriteProxyChecks(macs, traceDir, mode, seed, queryStalenessBoundS);
        }
    }
    if (imuCrossCheck || corroborationCheck)
    {
        std::cout << "    SHS: engine at (" << std::fixed << std::setprecision(1)
                  << shsEngine.GetX() << ", " << shsEngine.GetY() << ") vs true ("
                  << pos.x << ", " << pos.y << "), error "
                  << std::sqrt((shsEngine.GetX() - pos.x) * (shsEngine.GetX() - pos.x) +
                               (shsEngine.GetY() - pos.y) * (shsEngine.GetY() - pos.y))
                  << " m; last disagreement " << imuDisagreeM << " m against tolerance "
                  << imuTolM << " m\n";
        if (imuFusion)
        {
            std::cout << "    SHS FUSION: dead-reckoned position fused on "
                      << imuFusedCycles << " no-fix cycles\n";
        }
        std::cout << "    INTEGRITY: escalations refused -- IMU inconsistency "
                  << policy.GetImuRejections() << ", weak corroboration "
                  << policy.GetCorroborationRejections() << "\n";
    }

    if (nFinders > 0)
    {
        uint32_t knowing = 0;
        double sumAge = 0.0, sumErr = 0.0;
        for (size_t fi = 0; fi < finderMac.size(); ++fi)
        {
            CaTblTableEntry te;
            if (finderMac[fi] != nullptr &&
                finderMac[fi]->GetTableEntry(static_cast<uint16_t>(nodeId), te))
            {
                ++knowing;
                sumAge += (durationS - te.timestampS);
                sumErr += std::sqrt((te.x - pos.x) * (te.x - pos.x) +
                                    (te.y - pos.y) * (te.y - pos.y));
            }
        }
        std::cout << "    TABLE: " << knowing << "/" << finderMac.size()
                  << " finders hold the casualty entry";
        if (knowing > 0)
        {
            std::cout << ", mean age " << std::fixed << std::setprecision(1)
                      << (sumAge / knowing) << " s, mean error "
                      << (sumErr / knowing) << " m vs truth";
        }
        std::cout << "\n";
        uint64_t merges = 0;
        for (size_t m = 0; m < macs.size(); ++m)
        {
            merges += macs[m].GetTableMerges();
        }
        std::cout << "    TABLE: " << merges << " entry merges network-wide\n";
        if (tableFuseCI)
        {
            uint64_t fuses = 0; // P6 (G24): only under the flag, so no default line moves
            for (size_t m = 0; m < macs.size(); ++m)
            {
                fuses += macs[m].GetTableFuses();
            }
            std::cout << "    TABLE CI: " << fuses << " covariance-intersection fuses network-wide\n";
        }
        if (bidirectionalDigest || pseudoAnchorSubs > 0)
        {
            std::cout << "    TABLE(reverse): casualty sent " << casualtyDigests
                      << " digests, holds "
                      << (finderEntryCycles > 0
                              ? static_cast<double>(finderEntryHeldSum) /
                                    static_cast<double>(finderEntryCycles)
                              : 0.0)
                      << " finder entries per cycle (of " << finderIds.size() << ")";
            if (finderEntryAgeN > 0)
            {
                std::cout << ", mean age " << std::fixed << std::setprecision(1)
                          << (finderEntryAgeSum / static_cast<double>(finderEntryAgeN))
                          << " s";
            }
            std::cout << "\n    TABLE(reverse): " << pseudoAnchorSubs
                      << " pseudo-anchor substitutions";
            // The provenance term's actual input, reported where the substitution is
            // reported. The C5 block below prints the same quantity but only when
            // --relayedRanging is on, and the pseudo-anchor path feeds the accumulator
            // without it -- so the share was being computed and never shown.
            if (relayedFracN > 0)
            {
                std::cout << ", relayed variance share "
                          << (relayedFracSum / static_cast<double>(relayedFracN))
                          << " over " << relayedFracN << " cycles";
            }
            std::cout << "\n";
        }
    }

    if (relayedRanging)
    {
        if (relayedFracN > 0)
        {
            std::cout << "    C5 relayed variance share: mean "
                      << (relayedFracSum / static_cast<double>(relayedFracN))
                      << " over " << relayedFracN << " cycles that had any\n";
        }
        std::cout << "    C5 relayed measurements: admitted " << relayAdmitted
                  << ", rejected A1(tier) " << relayRejTier << ", A2(depth) "
                  << relayRejDepth << ", A3(geometry) " << relayRejGeom
                  << ", A4(stale) " << relayRejStale << ", A5(duplicate) "
                  << relayRejDup << "\n";
        if (relayAdmitted > 0)
        {
            std::cout << "        mean sigma: raw "
                      << (relaySigmaRawSum / static_cast<double>(relayAdmitted))
                      << " m -> inflated "
                      << (relaySigmaEffSum / static_cast<double>(relayAdmitted))
                      << " m -- the relay position uncertainty, priced in\n";
        }
    }

    // C5 relay state, to stdout rather than to arm_summary.csv: a new column changes
    // the CSV header for every run and breaks byte-identity against baseline_v7.
    //
    // WHY THIS IS PRINTED AT ALL. Rule 4 -- an exact zero is a control-flow claim, not
    // a measurement. Without this, "the relay filter runs" would be an assertion about
    // code that produces no observable, and a relay whose Step() silently never ran
    // would look exactly like one that ran and converged. trace(P) is the quantity that
    // distinguishes them: an initialised-but-never-updated filter keeps its seeded
    // covariance, so a trace that has MOVED off 2 * 100^2 is evidence the update path
    // executed.
    if (relayedRanging)
    {
        for (size_t rt2 = 1; rt2 < tags.size(); ++rt2)
        {
            const double trP = tags[rt2].policy.Pxx() + tags[rt2].policy.Pyy();
            const double ex = tags[rt2].policy.EstX();
            const double ey = tags[rt2].policy.EstY();
            const double dx = ex - tags[rt2].pos.x;
            const double dy = ey - tags[rt2].pos.y;
            std::cout << "    C5 relay node " << tags[rt2].nodeId << ": steps "
                      << tags[rt2].steps << ", trace(P) " << std::fixed
                      << std::setprecision(3) << trP << " m^2 (seeded 20000), Pxy "
                      << tags[rt2].policy.Pxy() << ", err "
                      << std::sqrt(dx * dx + dy * dy) << " m, energy "
                      << tags[rt2].energy.GetTotalJ() << " J\n";
        }
    }
    if (chain)
    {
        chain->Report(std::cout, tags, relayVarBudgetM2, maxRelayDepth);
        if (spec.subjectRelayChain)
        {
            chain->ReportSubjects(std::cout);
        }
    }
    if (fuseFloorLog)
    {
        fuseFloorLog->Report(std::cout, CaTblPolicyModeName(mode));
    }
    if (legTerms)
    {
        legTerms->Report(std::cout, CaTblPolicyModeName(mode));
    }
    if (spec.caseCRelative)
    {
        std::cout << "  DC-3 Case C relative (" << CaTblPolicyModeName(mode)
                  << "): 0-anchor node-cycles " << caseCCycles << " (no peer heard " << caseCLonely
                  << "); peer exchanges " << caseCTried << ", ranged " << caseCGot
                  << " (carried on entries, " << std::fixed << std::setprecision(6) << caseCCarryS
                  << " s billed); reached the base station " << caseCDelivered << ", dropped "
                  << caseCDropped << ", still pending " << caseCPending.size()
                  << "; shapes: pairs " << caseCPairs << ", 3+ " << caseCShapes
                  << " (completed pairs " << caseCCompleted << "); fused into a filter "
                  << caseCFused << "\n";
    }
    if (relayRound)
    {
        relayRound->Report(std::cout, dt);
    }
    for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
    {
        if (tierRouters[ti])
        {
            tierRouters[ti]->Report(std::cout, tags); // NW-2
        }
    }
    if (router)
    {
        router->Report(std::cout, tags);
    }
    if (tdoa)
    {
        tdoa->Report(std::cout, policy, energy.GetAccount(CA_TBL_TIER_868));
    }
    if (spec.integrityNlosFromScore && mode == CA_TBL_MODE_FULL)
    {
        // DC-8. The measured node's policy, the only one whose links are scored.
        std::cout << "    DC-8 integrity eligibility from NLOS_score: "
                  << policy.GetIntegrityScoreEligible() << " eligible and "
                  << policy.GetIntegrityScoreIneligible()
                  << " not, of fix cycles with a scored link; "
                  << policy.GetIntegrityTagFallback()
                  << " with none fell back to the tag; " << policy.GetSuspectCount()
                  << " suspects\n";
    }
    if (suspectRecovery == CA_TBL_RECOVERY_DOWNWEIGHT)
    {
        // DC-7. The measured node's policy; relays run the same config and are not counted.
        std::cout << "    DC-7 downweighted suspect fixes: " << policy.GetDownweightedFixes()
                  << " of " << policy.GetSuspectCount() << " suspects (fused at R x "
                  << pcfg.suspectInflateFactor << ", PCS x " << pcfg.suspectPcsFactor
                  << ")\n";
    }
    if (nlosScorer)
    {
        nlosScorer->Report(std::cout);
    }
    res.handshakeDeferrals = handshakeDeferrals;
    res.handshakeDeferralDelayS = handshakeDeferralDelayS;
    res.escalationsDeferredAway = escalationsDeferredAway;
    res.handshakePreemptions = handshakePreemptions;
    res.tp = tp;
    res.fp = fp;
    res.fn = fn;
    res.tn = tn;
    res.cyclesInUwbCoverage = inCov;
    res.reinitFallback = policy.GetReinitFallbackCount();

    // Must run while macs/beacons are still alive: it cancels the events that hold
    // raw pointers into them. Destroying the deques first would leave the scheduler
    // holding dangling this-pointers.
    Simulator::Destroy();

    return res;
}

} // namespace

/** Append a sha256 of every CSV this run produced to run_id.txt.
 *
 * WHY CONTENT AND NOT TIMESTAMPS. The sidecar's job is to say which run produced
 * the files beside it, and until now nothing could answer that. A `git reset
 * --hard` rewrote results/causal/run_id.txt with content describing an Aug 5
 * build while the traces next to it came from an Aug 9 one -- and because the
 * reset wrote the file, its mtime was NEWER than the traces. Every ordering-based
 * freshness check passes that. What was wrong was the content, and the traces
 * carried no stamp to compare against, so "does this sidecar belong" was not
 * merely unchecked, it was unanswerable.
 *
 * A digest makes it answerable exactly. Three failures close at once: a reverted
 * or foreign sidecar, a run that died after some outputs and before others, and
 * results edited after the fact -- none of which any timestamp gate can see.
 *
 * ORDERING IS LOAD-BEARING. This must run after the last stream is closed, so it
 * is a second append rather than part of the startup write. That also makes the
 * block's presence a completeness signal for free: a run that dies mid-way leaves
 * a sidecar with no digest, which is exactly the state a reader should distrust.
 */
static bool
AppendOutputDigest(const std::string& outdir, const std::string& patterns)
{
    // ONLY THIS RUN'S OWN OUTPUTS. The first version globbed *.csv, which swept in
    // probe_range_sweep.csv and rho_summary.csv left in results/causal by earlier,
    // unrelated runs -- so the sidecar vouched for files it never wrote, and any
    // consumer that legitimately held a subset (the G5 fixture copies the summary
    // and traces, not the strays) was reported as missing recorded outputs. A
    // digest that covers more than the run produced is not stricter, it is wrong in
    // a direction that manufactures failures.
    //
    // sha256sum's own format ("<hash>  <name>") is kept verbatim so the block can
    // be checked with `sha256sum -c` directly, without the verifier being the only
    // thing that can read it.
    const std::string cmd = "cd '" + outdir + "' 2>/dev/null && sha256sum " + patterns +
                            " 2>/dev/null | LC_ALL=C sort -k2";
    std::string out;
    if (FILE* fp = popen(cmd.c_str(), "r"))
    {
        char buf[512];
        while (fgets(buf, sizeof(buf), fp))
        {
            out += buf;
        }
        pclose(fp);
    }
    if (out.empty())
    {
        std::cerr << "FATAL: produced no output files to digest in " << outdir << "\n";
        return false;
    }
    std::ofstream rid((outdir + "/run_id.txt").c_str(), std::ios::app);
    if (!rid)
    {
        std::cerr << "FATAL: cannot append the output digest to " << outdir
                  << "/run_id.txt -- the sidecar cannot vouch for these results\n";
        return false;
    }
    rid << "digest sha256\n" << out;
    rid.close();
    if (!rid)
    {
        std::cerr << "FATAL: the output digest failed to flush to " << outdir
                  << "/run_id.txt\n";
        return false;
    }
    return true;
}

/**
 * P6. --specProfile: the spec preset (docs/SPEC_CONFORMANCE_PLAN.md sect 3.1). Each row is a
 * flag the preset sets, the value, and the docx section or owner decision it serves. The
 * radio basis is the LR1121 since SY-2 (2026-10-03). The geometry is GEO-01 stage 2's registered FALLBACK
 * (report/GEO01_RESULTS.md): no candidate met the SAR criterion. The PCS is CAL-02's
 * calibrated form, adopted by its registered rule (report/CAL02_RESULTS.md).
 * --fuseFloorActive (N.14), adopted by FUSE-01's registered rule on 2026-10-04, is left out
 * since FUSE-01b: with the subjects' finder oracle closed (N.21) it failed the same rule on
 * tag 0's endgame CEP95 (report/FUSE01B_RESULTS.md). The routing
 * weights are CAMP-SPEC's registered sweep pick (0,0,1,1), adopted by owner decision
 * 2026-10-03 (report/CAMP_SPEC_RESULTS.md sect 3); a weight the preset applied is not
 * refused when --fixRouting is turned off explicitly, it is simply unused.
 * --sparseCaseA, in the plan's table, is left out: --sparseInStep replaces it (FATAL
 * together). The UWB channel model stays legacy (P0's read-out). --hopVarianceTerms, in the
 * plan's table and in the preset until 2026-10-03, is left out by owner decision: its RSSI
 * variability, converted to range, swamps a relayed leg's sigma (64-83 m against 3.5 m;
 * TECH_DEBT N.11). It stays a flag, and the single-hop comparator ran with it off.
 */
struct CaTblSpecSetting
{
    const char* name;
    const char* value;
    const char* why;
};

static const CaTblSpecSetting kSpecProfile[] = {
    {"tier1Tdoa", "true", "Physical 1.1: tier 1 is a TDoA exchange"},
    {"nlosClassifier", "true", "Physical 1.3 / 2.5: per-tier NLOS scores"},
    {"pcsNlosClassifier", "true", "Physical 2.5: PCS term 3 from the classifier"},
    {"nlosSource", "propagation", "Physical 2.5: NLOS keyed on propagation, not the Markov tag"},
    {"terrainPerTierFreq", "true", "physics fix D-4: per-tier terrain wavelength"},
    {"imuPredict", "true", "Physical 2.2: IMU-driven prediction"},
    {"sparseInStep", "true", "Physical 2.4: sparse cases solved inside Step()"},
    {"caseD", "true", "Physical 2.4 Case D: one range plus the IMU"},
    {"caseCRelative", "true", "Physical 2.4 Case C: relative shape at the base station"},
    {"suspectRecovery", "downweight", "Physical 2.6: a SUSPECT fix is downweighted"},
    {"suspectForcesEscalation", "false", "Physical 2.6: SUSPECT does not force escalation"},
    {"integrityNlosFromScore", "true", "Physical 2.6: integrity eligibility from the score"},
    {"imuCrossCheck", "true", "Physical 2.6: IMU cross-check"},
    {"corroborationCheck", "true", "Physical 2.6: corroboration check"},
    {"imuWitnessSource", "model", "Physical 2.6: the witness is the IMU model"},
    {"entryVelocity", "true", "Physical 2.1: entries carry velocity"},
    {"entryPrediction", "true", "Physical 2.2: per-entry prediction"},
    {"pcsForm", "calibrated", "decision 3: PCS from CAL-02 (report/CAL02_RESULTS.md), adopted by its registered rule"},
    {"pcsW1", "0.0", "CAL-02 fit.json"},
    {"pcsW2", "1.920195", "CAL-02 fit.json"},
    {"pcsW3", "1.079805", "CAL-02 fit.json"},
    {"pcsThrUp868", "3.000016", "CAL-02 fit.json"},
    {"pcsThrUp2p4", "2.023835", "CAL-02 fit.json"},
    {"pcsThrUpUwb", "1.501074", "CAL-02 fit.json"},
    {"pcsThrDown2p4", "2.3611", "CAL-02 fit.json"},
    {"pcsThrDownUwb", "1.681203", "CAL-02 fit.json"},
    {"escalationRequiresEvidence", "true", "decision 7: the evidence gate stays on"},
    {"uwbEvidenceFromNeighbours", "true", "MS-3: the gate predicts from the awake radio"},
    {"uwbCoverageIncludesFinders", "true", "MS-3: the trigger label counts finders"},
    {"sendHandshakeFrames", "true", "MAC 8: the handshake is on the air"},
    {"handshakeFixes", "true", "MAC 8: the specified handshake"},
    {"triggeredUpdates", "true", "MAC 7: triggered updates"},
    {"mobilityFromRange", "true", "MAC 11.11: mobility state from range trend"},
    {"bidirectionalDigest", "true", "MAC 6: bidirectional digest"},
    {"digestSummary", "true", "MAC 6: digest summary"},
    {"tableFuseCI", "true", "Physical 2.7: covariance-intersection table merge"},
    {"meshLinks", "all", "throughout: any two radios can exchange frames"},
    {"relayedRanging", "true", "Network 2: relayed ranging"},
    {"relayChain", "true", "Network 2: relays chain"},
    {"maxRelayDepth", "2", "Network 2: depth 2"},
    {"relayVarBudget", "24336", "Network 2: A2's budget, (156 m)^2 as G17 (OURS); 0 makes the chain inert"},
    {"subjectRelayChain", "true", "Network 2: subjects in the relay chain, so depth 2 exists with five subjects (N.18; owner decision 2026-10-03)"},
    {"hopProvenance", "true", "Physical 2.5 / Transport 3: hop count carried"},
    {"fixRouting", "true", "Network 2: routing tables"},
    {"fixRoutingConfidence", "true", "Network 3: confidence-weighted routing"},
    {"fixRoutingSubjects", "true", "Network 3: every subject routes on its own tier"},
    {"routeAlpha", "0", "Network 3: CAMP-SPEC's registered sweep pick (0,0,1,1), adopted by owner decision 2026-10-03"},
    {"routeBeta", "0", "Network 3: CAMP-SPEC's registered sweep pick (0,0,1,1), adopted by owner decision 2026-10-03"},
    {"routeGamma", "1", "Network 3: CAMP-SPEC's registered sweep pick (0,0,1,1), adopted by owner decision 2026-10-03"},
    {"routeSlack", "1", "Network 3: CAMP-SPEC's registered sweep pick (0,0,1,1), adopted by owner decision 2026-10-03"},
    {"queryV2", "true", "Network 4: QUERY/RESPONSE contents"},
    {"queryTierBinding", "per-tier", "Network 4: queries per tier"},
    {"queryProxy", "true", "Network 4: proxy answers"},
    {"reversePath", "true", "Transport 1: reverse-path RESPONSE"},
    {"reversePathHop", "true", "Transport 1: hop by hop"},
    {"queryTriggeredEscalation", "true", "Network 4: query-triggered escalation"},
    {"uwbArbitration", "true", "Network 5: UWB arbitration"},
    {"arbitrationV2", "true", "Network 5: U(r) with a queue"},
    {"anchorGrantGossipS", "5", "Network 6: anchors gossip grants"},
    {"gossipQueue", "true", "Network 6: and their queues"},
    {"grantGatedUwb", "true", "MAC 8 / Transport 4: escalation only when granted"},
    {"sessionLayer", "true", "Transport 2: sessions"},
    {"sessionOverMesh", "true", "Transport 2: fed by delivered RESPONSEs"},
    {"batchSearch", "true", "Transport 5: batch search"},
    {"finderSessions", "true", "decision 8: finders search too"},
    {"finderRequesterEscalation", "true", "decision 8 / Transport 4: a finder's session drives escalation too (TA-1b)"},
    {"identityQueryUrgent", "true", "Transport 4: the batch session is URGENT"},
    {"identityQueryManifests", "1,2,3,4,5", "Transport 5: the batch is the five subjects"},
    {"identityQueryPeriodS", "30", "Transport 2: the session's query period (OURS)"},
    {"requesterEscalation", "true", "Transport 4: the requester drives escalation"},
    {"sessionPreciseOnGrant", "true", "Transport 2: LOCATED_PRECISE on a confirmed grant"},
    {"vizEvents", "true", "rule 13: events_ written and digested"},
    {"digestVizOutputs", "true", "rule 13 / M.4: the visualiser's outputs digested, which the row above claimed before they were"},
    {"edgeUplinkRanges", "true", "final plan: the gateway edge server's uplink billed"},
    {"subjectEdgeUplink", "true", "final plan / decision 4: every subject's uplink billed too (row 55)"},
    {"radioBasis", "lr1121", "hardware: tiers 1 and 2 on one LR1121 (SY-2; DS.LR1121.W.APP Rev 2.0)"},
    {"measuredSubjects", "5", "decision 4: all five field nodes are measured"},
    {"nTags", "5", "decision 4: five field nodes"},
    {"boxHalf", "5000", "the spec's 5 km search area"},
    {"nGateways", "8", "GEO-01 stage 2: N8_ring, the registered FALLBACK (no geometry met A >= 0.95)"},
    {"nFinders", "4", "decision 6: four finders"},
    {"finderMode", "2", "decision 6: the two-phase search"},
    {"finderDispersal", "1.0", "MS-1: finders spread over the subjects"},
    {"finderPseudoAnchor", "true", "MS-1: finders as pseudo-anchors"},
    {"subjectFinderPseudoAnchor", "true", "MS-1 / N.21: for the subjects too -- no node ranges against a finder's true position"},
    {"finderPseudoAnchorStrict", "true", "MS-1 / N.21: tag 0 too -- a finder range with no report behind it is dropped, never solved against the truth"},
    {"finderDigestPeriodS", "20", "MS-1: finders gossip digests"},
    {"finderTasking", "roundrobin", "MS-1: finders tasked per subject"},
};

int
main(int argc, char* argv[])
{
    uint32_t envInt = 1;           // ENV_RURAL_FORESTED -- the wide-area SAR scenario
    // 255 is a SENTINEL meaning "follow envInt", not an EnvironmentClass.
    // Tiers 2 and 3 used to default to ENV_RURAL_OPEN because their anchors sat
    // at facility sites -- true of the three-population model, and false since
    // the tri-band correction, which put all three radios on the same node. A
    // node standing in canopy is in canopy on every band it carries.
    uint32_t envFacilityInt = 255;
    double durationS = 3600.0;
    // 2.5 s, not 1.0 s. WPC G.S.R. 853(E) Table-II caps a field node at a 2.5%
    // transmit duty cycle in the 865-867 MHz band. Tier-1 airtime is ~0.056 s per
    // cycle at SF7/125 kHz, so dt = 1.0 s is a 5.6% duty cycle -- illegal, and every
    // arm previously ran at it. The minimum compliant period is 0.056/0.025 = 2.24 s;
    // 2.5 s gives 2.24% with margin for the ToA variation across payload sizes.
    //
    // This is a CONSTRAINT the architecture has to state, not a tuning knob: it caps
    // the achievable tier-1 fix rate at 0.4 Hz and it more than halves the number of
    // cycles in a 3600 s run (3600 -> 1440), which changes energy totals and fix
    // counts on every arm. Results taken at dt = 1.0 are not comparable to these.
    double dt = 2.5;
    uint32_t seedStart = 7;
    uint32_t seedCount = 1;
    std::string outdir = "results/causal";
    bool writeTrace = true;
    bool probe = false;
    // Defaults false: the probe's default output stays byte-identical to every
    // run that preceded the D2 fix. See DEFECTS.md D2.
    bool traceDiagnostic = false;
    // All four defaults below are set from results/causal/probe_range_sweep.csv,
    // not chosen. See BuildDeployment() for the derivation of each.
    double boxHalfM = 1000.0;      // 868 r80 = 1000 m; 3x3 grid pitch 900 m
    // 0.0 = off. See --baseCampFrac.
    double baseCampFrac = 0.0;
    double subjectKeepOutM = 0.0;
    double maxTurnRateRadS = 0.0;
    // 0 = feed every fix, which is what every run before 2026-09-08 did.
    double shsTrustedFixKArg = 0.0;
    bool imuFusionArg = false;
    bool imuIdealArg = false;
    bool queryProxyArg = false;
    bool tableFuseCIArg = false;
    bool digestSummaryArg = false;
    double batteryDepleteJArg = 0.0;
    bool sessionLayerArg = false;
    bool mobilityFromRangeArg = false;
    bool lifecycleTrackingArg = false;
    bool batchSearchArg = false;
    bool sparseCaseAArg = false;
    double sparseCaseAMarginMArg = 1.0;
    uint32_t injectSilentIdArg = 0;
    double injectSilentStartSArg = 0.0;
    double injectSilentEndSArg = 0.0;
    double queryStalenessBoundSArg = 120.0;
    bool reversePathArg = false;
    bool shsStepBoundsArg = false;
    bool shsSnapPositionArg = false;
    double uwbTriadRadiusM = 10.0; // UWB r80 = 10 m
    double t2TriadRadiusM = 150.0; // 2p4 r80 >= 900 m, so siting-limited not range-limited
    uint32_t uwbAnchorsPerSite = 5;
    uint32_t t2AnchorsPerSite = 3;
    double walkSpeedMps = 1.2;
    double dwellS = 90.0;
    uint32_t vegWideInt = static_cast<uint32_t>(VEG_DENSE_FOREST);
    uint32_t vegFacilityInt = 255; // sentinel: follow vegWideInt

    CommandLine cmd;
    cmd.AddValue("env",
                 "EnvironmentClass for the wide-area 868 tier (0=RuralOpen "
                 "1=RuralForested 2=Suburban 3=UrbanModerate 4=UrbanDense)",
                 envInt);
    cmd.AddValue("envFacility",
                 "EnvironmentClass for the 2.4 GHz and UWB tiers. Default 255 = "
                 "FOLLOW --env, because under the tri-band topology those radios "
                 "sit on the same nodes as the 868 tier and a node standing in "
                 "canopy is in canopy on every band it carries. It used to default "
                 "to ENV_RURAL_OPEN, from the three-population model in which tiers "
                 "2 and 3 were separate anchors at facility sites. That split gave "
                 "2.4 GHz enough relief to OUT-RANGE 868 against 27 dB of link "
                 "budget in tier 1's favour -- see report/ENV_SPLIT_DEFECT.md. "
                 "Pass 0 with --vegFacility=1 to reproduce a pre-2026-09-08 run.",
                 envFacilityInt);
    // ---- Anchor beacon cadence ---------------------------------------------
    //
    // Per tier, and NOT a free choice on tier 1. Tier-1 airtime is rationed by
    // WPC G.S.R. 853(E) Table-II (10% for an access point), and one SF7/125 kHz
    // beacon is 51.5 ms, so a 10 s period puts each gateway at 0.51% duty and the
    // nine-gateway grid at 4.6% CHANNEL occupancy. Dropping tier 1 to the tier-2
    // period would put the grid at 23% occupancy and make CAD deferral, not path
    // loss, the thing tier-1 discovery is measuring.
    //
    // Tiers 2 and 3 are unrationed ISM/UWB, and they need to be fast: the node
    // crosses the measured UWB r80 of ~13.5 m in about 9 s at the default 1.5 m/s
    // walking speed, so a discovery period comparable to that would mean the node
    // has left before it has finished discovering. 2 s is ~4.5 discovery
    // opportunities per transit.
    //
    // These three numbers are OURS. They are bracketed by regulation and by
    // transit time, not determined by them. Sweep them -- the trade between
    // discovery latency and beacon-collision load is a result.
    double beacon868S = 10.0;
    double beacon2p4S = 2.0;
    double beaconUwbS = 2.0;
    double nbrTtlMult = 3.0;
    // Fraction of a full beacon period the receiver is held open on an awake tier
    // that has discovered nothing. 1.0 is the only value consistent with discovery
    // being deterministic-on-reception; 0.0 reproduces the zero-charge run that
    // inverted the energy axis. Sweep it and report the bracket -- the span is how
    // much of an out-of-coverage arm's energy is a modelling assumption rather than
    // a measurement.
    double searchDutyFrac = 1.0;
    std::string nlosSourceArg = "legacy";
    std::string fastDemote = "off";
    bool mahalUseR = false;
    std::string suspectRecoveryArg = "none";
    double suspectInflateFactor = 4.0;
    uint32_t suspectRecoveryN = 5;
    // TRUE by default since 2026-09-08. K3 measured it worth 323.75 J of a
    // 339 J arm -- 95% -- at 20 of 20 seeds, p < 0.0001, with no significant
    // accuracy change and a zero noise floor. Pass false to reproduce a run
    // taken before that date.
    bool escalationRequiresEvidence = true;
    // Default OFF and default-off it stays until it has a gate with a negative
    // fixture (CLAUDE.md 4.4). The mechanism is new even though the flag name is old.
    bool escalationRequiresEvidenceT2Arg = false;
    double t2PredictRadiusMArg = 156.0;
    uint32_t t2PredictMinAnchorsArg = 3;
    bool uwbEvidenceFromNeighboursArg = false;
    bool suspectForcesEscalation = true;
    std::string pcsFormArg = "legacy";
    double pcsW1Arg = -1.0;
    double pcsW2Arg = -1.0;
    double pcsW3Arg = -1.0;
    double pcsGateFloorArg = -1.0;
    double pcsHysteresisBandArg = -1.0;
    // DC-6. Explicit per-tier PCS thresholds and the calibrated form. NaN = not given.
    SpecOptions spec;
    uint32_t nDeescalateArg = 0;
    bool sendHandshakeFramesArg = false;
    bool terrainPerTierFreqArg = false;
    bool intraCycleHandshakeDeferArg = false;
    uint32_t nTagsArg = 1;
    bool relayedRangingArg = false;
    uint32_t relayTierArg = 1;
    uint32_t maxRelayDepthArg = 1;
    double relayVarBudgetM2Arg = 0.0;
    bool relayChainArg = false; // L.1.2, off: every relay stays at depth 0
    // C5_INSTANT. "auto" (the default since 2026-09-30): the relays' round has instants of its
    // own in every run with a relay to schedule. "false" is every relayed run before that.
    std::string relaySlotsSpec = "auto";
    bool relaySlotsArg = false; // resolved from relaySlotsSpec after cmd.Parse()
    bool fixRoutingArg = false;           // L.1.1, off: no table is built, nothing routed
    bool fixRoutingConfidenceArg = false; // L.1.1, off: ETX routes, confidence only scored
    bool tier1TdoaArg = false;            // L.2.1, off: tier 1 localises by RSSI alone
    double tdoaPpsSigmaNsArg = 20.0;      // u-blox NEO/LEA-M8T time pulse, <= 20 ns 1-sigma
    bool nlosClassifierArg = false;       // L.2.2, off: no CIR, no score, nothing written
    bool pcsNlosClassifierArg = false;    // L.2.2, off: PCS term 3 reads the tag, as always
    double nlosScoreThresholdArg = 0.5;   // ours: the midpoint of every score's range
    std::string uwbNlosModelSpec = "legacy"; // M.2: resolved after cmd.Parse()
    std::string radioBasisArg = "sx";        // SY-2: resolved after cmd.Parse()
    bool vizEventsArg = false;
    bool digestVizOutputsArg = false; // M.4
    double vizTablePeriodSArg = 0.0;
    bool gwRidgeArg = false;
    double gwArcDegArg = 360.0;
    double gwArcCentreDegArg = 90.0;
    double trailDeviateSideArg = -1.0;
    double gwRidgeYArg = 0.55;
    double trailDeviateAtSArg = 0.0;
    double trailStopAfterSArg = 0.0;
    uint32_t nFindersArg = 0;
    uint32_t finderModeArg = 1;
    double finderStaleSteerBoundSArg = 0.0;
    double searchLaunchOnLossSArg = 0.0;
    double searchLaunchSigmaMArg = 0.0;
    double searchLaunchAtSArg = 0.0;
    // Sentinel: negative means follow --speed, which is what every run before
    // 2026-09-08 did, so the default is byte-identical.
    double finderSpeedMpsArg = -1.0;
    double finderDigestPeriodSArg = 30.0;
    double finderPosSigmaMArg = 0.0;
    bool finderPseudoAnchorArg = false;
    bool bidirectionalDigestArg = false;
    // TRUE since 2026-09-08. Tier 1 was billed as one broadcast -- a TDoA shape,
    // right when the NETWORK localises -- while CaTblPolicy runs on the FIELD
    // NODE and consumes a range per anchor, so the replies have to come back and
    // nothing charged for them. TECH_DEBT J.2 recommends billing what tier 1
    // actually does, which is RSSI inversion with replies. Roughly doubles
    // tier-1 energy. Pass false to reproduce anything measured before.
    bool tier1ChargeRepliesArg = true;
    // TRUE since 2026-09-29 (TECH_DEBT L.2.3). The spec puts the estimator on the
    // gateways; this was the one CONTRADICTION row in MASTER_PLAN 2.1. With it on,
    // tier 1 is billed as the broadcast plus one downlink per fix, so the reply
    // billing above applies only when this is false. B3 C6 measured the change as
    // energy-only at defaults. Pass false to reproduce anything measured before,
    // results/regen27_* included.
    bool gatewayEdgeServerArg = true;
    // SY-3. 0 = not given: the literal phy[] defaults stand.
    uint32_t sf868Arg = 0;
    double bw868Arg = 0.0;
    uint32_t sf2p4Arg = 0;
    double bw2p4Arg = 0.0;
    // "fix" since 2026-09-30 (TECH_DEBT M.10, owner decision): the downlink that carries
    // the gateways' fix back is billed on the radio whose measurements produced it, so a
    // floor-solved fix is an 868 MHz downlink. "active" is the old bill -- the downlink
    // on the policy's active tier -- and reproduces the build before it byte-for-byte,
    // B3 C6's results/regen27_edge/edge included. Read only with --gatewayEdgeServer=true.
    std::string edgeDownlinkTierSpec = "fix";
    bool edgeDownlinkOnFixTierArg = true; // resolved from edgeDownlinkTierSpec after Parse()
    // The values ca-tbl-policy.cc already sets, so passing nothing is
    // byte-identical to every run taken before this flag existed.
    double processNoiseStdPosArg = 1.0;
    double processNoiseStdVelArg = 0.5;
    double finderDispersalArg = 0.0;
    bool uwbCoverageIncludesFindersArg = false;
    uint32_t hopProvenanceFormArg = 0;
    double hopPenaltyArg = 0.0;
    bool imuCrossCheckArg = false;
    double imuDriftMpsArg = 0.15;
    double imuConsistencyKArg = 3.0;
    double imuSigmaFloorMArg = 5.0;
    bool corroborationCheckArg = false;
    double corroborationKArg = 3.0;
    double corroborationRangeSigmaMArg = 30.0;
    uint32_t sitingModeArg = 0;
    uint32_t topologyArg = 0;
    // MS-2. legacy = the --gw* flags as before.
    std::string gwLayoutArg = "legacy";
    double gwLayoutFracArg = 0.9;
    double gwClusterRadiusMArg = 300.0;
    uint32_t gwStarArmsArg = 3;
    double gwStarSpacingMArg = 1500.0;
    std::string gwLayoutFileArg;
    uint32_t nGatewaysArg = 0;
    double gwRingFracArg = 0.6;
    double identityQueryPeriodSArg = 0.0;
    uint32_t identityQueryTargetArg = 2;
    bool uwbArbitrationArg = false;
    double arbBatteryCapJArg = 1000.0;
    double anchorGrantGossipSArg = 0.0; // L.1.4, 0 = off: no coordinator, no gossip
    // DEFAULT TRUE. See the note at the deferral site: with gating on but deferral
    // off, an arm still performs the escalation while paying for none of it, and
    // the size of that free ride is correlated with which arm is under test --
    // 100% of fixed_threshold's handshakes against 1% of ca_tbl's. That bias runs
    // in the BASELINE's favour, which feels conservative and is not: it makes the
    // baseline's handoffs free. "No signal, no handoff" removes the bias by making
    // the escalation not happen rather than happen for free.
    bool deferEscalationOnDutyLimit = true;
    // When an escalation handshake is deferred by the duty-cycle governor, the
    // next cycle pre-empts its ranging exchange and uses the freed budget for
    // the handshake instead. Implements the access-category priority the MAC
    // spec already defines (CW_high > CW_normal) in the duty-cycle governor.
    bool arbitrateDutyBudget = true;
    // <= 0 means "leave the CaTblPolicyConfig default untouched", so the flags are
    // inert unless set and baseline reproduction does not depend on their values.
    double fixedVarThresholdM2 = -1.0;
    double pcsSigmaTargetM = -1.0;
    double injectRangeBiasM = 0.0;    // S7-lite, default off
    double injectBiasFraction = 0.0;  // S7-lite, default off
    uint32_t injectFaultAnchors = 0;  // 0 = common-mode (F3); >0 = single anchor (F1)
    bool floorOnDemand = false;   // default preserves the pre-existing baseline
    uint32_t floorSkipK = 1;      // K = 1 primary: pure reactive, no hysteresis
    uint32_t floorHeartbeatN = 0; // 0 = off; the radio stays awake regardless
    std::string armsArg = "all";
    std::string multiStepDemote = "off";
    // Gateways per side of the tier-1 grid. 3 reproduces the historical 3x3.
    // Density is gwGrid^2 / (2*boxHalf/1000)^2 per km^2; the derivation in
    // BuildDeployment() puts the requirement at ~2.25/km^2, i.e. gwGrid = 3 at
    // boxHalf 1000. Hold density constant with gwGrid = 3*boxHalf/1000.
    uint32_t gwGrid = 3;
    // OURS AND UNCITED, like neighbourTtlS. How long a tier stays "known populated"
    // after its radio sleeps. A new free parameter -- sweep it, do not choose it.
    double demoteStalenessS = 60.0;
    cmd.AddValue("duration", "Simulated seconds per arm", durationS);
    cmd.AddValue("dt", "Cycle period in seconds", dt);
    cmd.AddValue("seed", "Single seed (alias for seedStart)", seedStart);
    cmd.AddValue("seedStart", "First seed", seedStart);
    cmd.AddValue("seedCount", "Number of consecutive seeds to run", seedCount);
    cmd.AddValue("outdir", "Output directory (must already exist)", outdir);
    cmd.AddValue("trace", "Write per-step trace CSVs", writeTrace);
    cmd.AddValue("probe", "Run the pre-flight range sweep and exit", probe);
    cmd.AddValue("traceDiagnostic",
                 "Emit extra diagnostic columns (mean_snr_all_db). Default false so the "
                 "default output is byte-identical to pre-D2-fix runs.",
                 traceDiagnostic);
    cmd.AddValue("boxHalf", "Half-width of the deployment box, metres", boxHalfM);
    cmd.AddValue("baseCampFrac",
                 "Put the infrastructure at a BASE CAMP at (f*boxHalf, f*boxHalf) and "
                 "start the finders there, leaving the casualty touring the centre. "
                 "0.0 = off, which is every run before 2026-09-08. WHY IT EXISTS: at "
                 "0.0 the casualty starts within 0.2*boxHalf of the centre and tours "
                 "corners at 0.47*boxHalf while a 3x3 gateway grid sits at 0.9*boxHalf "
                 "WITH ONE AT THE ORIGIN -- so it is permanently inside the footprint "
                 "and the searchers begin beside it. That is asset tracking in a "
                 "covered yard, and the condition the architecture exists for -- no "
                 "single band spanning the mission -- never occurs. At 0.9 with "
                 "boxHalf 3000 the camp is at (2700,2700) and the tour corners are "
                 "1824-5813 m away, mostly beyond 868 r80 of 1515 m, so reaching the "
                 "casualty needs a searcher to walk out. Finders move with the camp: "
                 "leaving them central would put the searchers in the search area at "
                 "t=0 and flatter the very mechanism this is meant to test.",
                 baseCampFrac);
    cmd.AddValue("shsSnapPosition",
                 "Anchor the SHS position at each CLEAN trusted fix. FALSE by default. "
                 "REQUIRED BY --imuFusion, and the two are only correct together: the "
                 "fusion variance is imuSigmaFloor + imuDrift * (t - lastTrustedFix), "
                 "which asserts the position error is as old as the last trusted fix. "
                 "A trusted fix calibrates GAIT and does not touch position, so without "
                 "snapping that error accumulates from seeding and is never reset -- "
                 "measured 459 m of SHS error while the variance claimed 5.4 m, and the "
                 "filter duly followed it (cep95 276.6 -> 1499.8 on ca_tbl_full). "
                 "Snapped, the SHS contributes the dead-reckoned DISPLACEMENT since "
                 "the last fix, which is what an inertial system is for.",
                 shsSnapPositionArg);
    cmd.AddValue("shsStepBounds",
                 "Refuse a step-length calibration implying a step outside 0.25-1.10 m. "
                 "FALSE by default so earlier results reproduce. k_w = chord / |sum q u| is "
                 "unbounded above: the numerator is a radio chord, the denominator a gait "
                 "quantity that does not grow when the chord is mostly fix error. Where "
                 "fixes are SPARSE -- which is where --imuFusion is most needed -- the fit "
                 "runs away: measured 1.9e6 m of SHS error on an arm holding 7 fixes in "
                 "1440 cycles, against 13 m on an arm that never calibrated. The bound is "
                 "on step length rather than k_w because step length is the quantity with "
                 "a physiological range. Turn this on with --imuFusion.",
                 shsStepBoundsArg);
    cmd.AddValue("batteryDepleteJ",
                 "sect 49: joules after which the measured tag's radio goes permanently "
                 "silent. 0 = off (infinite battery, the pre-existing assumption). Reuses "
                 "the silence mechanism because a flat battery and a failed transmit chain "
                 "are INDISTINGUISHABLE from the network's side; what differs is that this "
                 "one cannot end. The filter keeps running -- a device with a dead radio "
                 "still knows where it thinks it is, it just cannot tell anyone, and the "
                 "estimate degrades from that moment because no measurement arrives.",
                 batteryDepleteJArg);
    cmd.AddValue("mobilityFromRange",
                 "sect 11.11: drive the MAC mobility state from the trend in the node's "
                 "own mean measured range, so the RECEDING eager-push branch of "
                 "StartHandshake can execute. FALSE by default. Until now "
                 "SetMobilityState was never called by the scenario and that branch was "
                 "dead code that looked implemented. The signal is evidence the node "
                 "actually has -- everything it ranges against getting further off -- not "
                 "the walker's true velocity, which no real node knows.",
                 mobilityFromRangeArg);
    cmd.AddValue("lifecycleTracking",
                 "sect 24-27: track every node the tag hears through UNSEEN / TRACKED / "
                 "DEGRADED / LOST / REACQUIRED. FALSE by default. Driven from the "
                 "neighbour tables, not the deployment -- a node that exists and cannot be "
                 "heard is exactly what this tracks, and reading the deployment would "
                 "report everyone present forever. Observation only; changes no decision.",
                 lifecycleTrackingArg);
    cmd.AddValue("batchSearch",
                 "Application sect 5: open ONE session over all nTags targets instead of "
                 "one per target. FALSE by default. One flood, one staleness clock, and "
                 "every escalation request in the SAME arbiter queue -- the only "
                 "configuration where U(r) scoring does observable work.",
                 batchSearchArg);
    cmd.AddValue("sessionLayer",
                 "Drive the Transport/Application layer from the run: a session opened at "
                 "t=0, every fix delivered to it as a RESULT carrying its lineage, and the "
                 "render rule applied. FALSE by default. Adds no frames and changes no "
                 "decision -- it consumes what the policy already produced -- so it is "
                 "observation, not mechanism.",
                 sessionLayerArg);
    cmd.AddValue("sparseCaseA",
                 "sect 7: solve a fix from TWO anchors when the active tier has only two, "
                 "by two-circle intersection with the prior choosing the branch. FALSE by "
                 "default. Below three anchors the estimator coasts; this is the cheapest "
                 "lever on tier-3 reachability, since UWB reaches ~13.5 m so three "
                 "neighbours at once is rare and two is markedly less rare. REFUSED rather "
                 "than guessed when the prior cannot separate the candidates -- a coin "
                 "flip between points tens of metres apart is worse than coasting.",
                 sparseCaseAArg);
    cmd.AddValue("sparseCaseAMargin",
                 "sect 7: Mahalanobis separation the two candidates must have before one "
                 "is accepted -- in Mahalanobis-distance units, not metres, whatever the "
                 "variable's M suffix says. Larger refuses more.",
                 sparseCaseAMarginMArg);
    cmd.AddValue("injectSilentId",
                 "sect 49: node id whose TRANSMIT goes silent for a window, then returns. "
                 "0 = off. The node is not removed and still HEARS everything -- this is a "
                 "node that cannot talk (failed transmit chain, exhausted duty budget, "
                 "amplifier browning out on a cold battery), NOT one behind an obstruction, "
                 "which would be deaf as well as mute. Removing it would make the return "
                 "indistinguishable from a new node joining.",
                 injectSilentIdArg);
    cmd.AddValue("injectSilentStartS", "sect 49: when the silence begins.", injectSilentStartSArg);
    cmd.AddValue("injectSilentEndS", "sect 49: when the node comes back.", injectSilentEndSArg);
    cmd.AddValue("digestSummary",
                 "Carry the {node_id, seq_no} anti-entropy summary in a DIGEST, and "
                 "answer it. FALSE by default. The airtime has been billed since S4 while "
                 "the frame carried nothing, so a responder could not tell what the asker "
                 "was missing and pushed its own entry regardless -- a request for one "
                 "specific thing dressed as a request for everything. With the summary "
                 "the responder pushes what the asker actually lacks, or stays silent.",
                 digestSummaryArg);
    cmd.AddValue("tableFuseCI",
                 "Fuse a table entry whose sequence number EQUALS the held one by "
                 "Covariance Intersection, instead of discarding it. FALSE by default. "
                 "The spec's table rule has three branches and the code had two -- equal "
                 "was falling into the discard path, and equal is the interesting case: "
                 "the same report arriving by a second mesh route, seen through different "
                 "NLOS. CI rather than a Kalman fuse because the two copies share an "
                 "unidentifiable upstream relay, and fusing them as independent returns a "
                 "covariance smaller than either input -- which is how a mesh talks itself "
                 "into a wrong position with high confidence.",
                 tableFuseCIArg);
    cmd.AddValue("queryProxy",
                 "Network Layer 4: answer a QUERY on behalf of a FRESH table entry, not "
                 "only when this node IS the target. FALSE by default. The spec calls "
                 "this the case that makes an identity-scoped query useful at all: the "
                 "target may be out of the requester radio range while its position "
                 "is not, because the mesh already gossiped it here. Costs one wire "
                 "byte per entry for target_manifest_id, charged only while this is "
                 "on. An entry older than --queryStalenessBoundS is refused and counted, "
                 "so a search that found only stale knowledge is distinguishable from "
                 "one that found none.",
                 queryProxyArg);
    cmd.AddValue("queryStalenessBoundS",
                 "Age beyond which a table entry may not be proxied (Network 4). 120 s "
                 "by default: the spec's bound, kept because it is now MEASURED rather "
                 "than inherited (L.2.5, results/l25_proxy_age_20260929, seeds 7/11/13). "
                 "It does not bind for the casualty: four finders digesting every 20 s "
                 "keep the entry a proxy holds when a query arrives p50 11.5 s, max "
                 "68.1 s old in the arc searcher regime (265/265 inside 120 s) and p50 "
                 "17.2 s in the grid-2 5 km sparse regime (37/39 inside). What it does "
                 "refuse is an entry about a node nobody polls -- a peer's is p50 493 s "
                 "and 385 s old, because an entry reaches a holder only as its subject's "
                 "answer to that holder's own digest. Raising the bound would answer from "
                 "those, and age has a measured price: an estimate held 120 s is 143 m "
                 "from a casualty walking 1.2 m/s, against 4.5 m fresh in the searcher "
                 "regime. So it is not a "
                 "tuning knob for accuracy: it is the claim that a position this old is "
                 "no longer something the mesh has reason to believe.",
                 queryStalenessBoundSArg);
    cmd.AddValue("reversePath",
                 "Transport Layer 1: cache which neighbour a QUERY arrived from and "
                 "unicast the RESPONSE back along that path instead of flooding it. "
                 "FALSE by default. Borrowed from AODV/DSR reactive routing. The cache "
                 "is bounded and time-windowed on the same discipline as the packet_id "
                 "dedup cache, and a miss FALLS BACK TO FLOOD -- so an eviction costs "
                 "airtime, never the answer. Unicast and flooded responses are "
                 "counted separately because the spec asks for the airtime saving as a "
                 "number rather than a claim.",
                 reversePathArg);
    cmd.AddValue("reversePathHop",
                 "With --reversePath, make the reverse path real (Transport sect 1; "
                 "docs/SPEC_CONFORMANCE_PLAN.md NW-8). FALSE by default, which keeps "
                 "--reversePath as it has always been: the cache holds the QUERY's "
                 "originator, and its hit and miss branches send the same one-hop frame. "
                 "TRUE: each node caches the PREVIOUS HOP the query came from (the channel's "
                 "transmitter), the RESPONSE is unicast to it and every node on the way "
                 "forwards it to its own cached previous hop, the 300 s window is honoured, "
                 "and a miss FLOODS the RESPONSE with the default hop limit. The RESPONSE "
                 "carries its query_id, requester and answerer (8 bytes, billed); a copy "
                 "retransmitted after a lost ACK is absorbed, not forwarded again. One NW-8 "
                 "line. "
                 "Needs --reversePath and --identityQueryPeriodS > 0. A multi-hop path needs "
                 "--meshLinks=all; under the star model every path is one hop.",
                 spec.reversePathHop);
    cmd.AddValue("grantGatedUwb",
                 "An escalation into UWB commits only on a delivered grant (MAC sect 8; "
                 "Transport sect 4 'ON UWB grant confirmed'; docs/SPEC_CONFORMANCE_PLAN.md "
                 "NW-7). FALSE by default, which is every result so far: the tier changes "
                 "inside Step() and nothing at the node reads ESC_GRANT, so a node can believe "
                 "it holds a UWB slot no anchor gave it. TRUE: the escalation is a pending "
                 "request (the ESC_REQUEST still goes out); the node stays on 2.4 GHz, frozen "
                 "-- no further escalation, no healthy demotion, though coverage loss still "
                 "demotes and cancels -- until a GRANT(granted) answering its own request "
                 "arrives, and moves to UWB at the next cycle. A denial drops the request and "
                 "its retries; exhausted retries time it out. 868 -> 2.4 GHz is unchanged; "
                 "static arms are exempt. Applies to every measured subject. Under it the "
                 "trace's escalated flag on 2.4 GHz marks the request, and the escalation "
                 "count the grant. Writes grants_<arm>_seed<N>.csv (every request, confirm, "
                 "deny, timeout, late grant and coverage cancellation, per subject), digested, "
                 "and one or two NW-7 lines. Needs --handshakeFixes.",
                 spec.grantGatedUwb);
    cmd.AddValue("queryV2",
                 "QUERY and RESPONSE carry what the spec says (Network sect 4; "
                 "docs/SPEC_CONFORMANCE_PLAN.md NW-9). FALSE by default, which is every result "
                 "so far: a QUERY carries one manifest id, a RESPONSE carries no entry and no "
                 "query id, nothing at the requester handles it, and a target that answered "
                 "still rebroadcasts. TRUE: a QUERY carries its query id, manifest list, "
                 "requester and priority; a node answers EVERY listed manifest it can -- as "
                 "the target, or by proxy from a fresh entry -- each with a RESPONSE carrying "
                 "the entry (with the producer's PCS, integrity verdict, battery, degradation, "
                 "UWB state and granting anchor), the manifest and the query id, billed 12 "
                 "bytes over the entry; it rebroadcasts with the answered manifests removed, "
                 "and not at all if it answered every one. Writes responses_<arm>_seed<N>.csv "
                 "(every answer a requester receives) and queryv2_<arm>_seed<N>.csv (every "
                 "decision on a query), digested, and one NW-9 line. Needs "
                 "--identityQueryPeriodS > 0.",
                 spec.queryV2);
    cmd.AddValue("queryTriggeredEscalation",
                 "An URGENT query for a node's own manifest escalates it toward UWB whatever "
                 "its PCS (Network sect 4: 'a QUERY match with priority == URGENT can itself "
                 "trigger UWB tier escalation ... bypassing the normal PCS threshold-crossing "
                 "trigger'; docs/SPEC_CONFORMANCE_PLAN.md NW-4). FALSE by default, which is "
                 "every result so far: no path from a QUERY to escalation. TRUE: the MAC "
                 "latches the query, the policy forces the escalation decision one tier per "
                 "cycle until UWB is reached or asked for -- before the coast and the blind "
                 "demotion that precede the decision -- while the suppress-only "
                 "preconditions still apply and a suppression is counted; the ESC_REQUEST "
                 "carries is_query_triggered, which the --arbitrationV2 arbiter scores. Static "
                 "arms ignore it. One NW-4 line. Needs --queryV2 and, until the session "
                 "requester (TA-2) issues URGENT queries, --identityQueryUrgent.",
                 spec.queryTriggeredEscalation);
    cmd.AddValue("identityQueryUrgent",
                 "Under --queryV2, send the identity queries at URGENT priority (1) instead of "
                 "ROUTINE (0). The byte mapping is OURS. FALSE by default.",
                 spec.identityQueryUrgent);
    cmd.AddValue("arbitrationV2",
                 "UWB arbitration as Network sect 5 specifies (docs/SPEC_CONFORMANCE_PLAN.md "
                 "NW-5). FALSE by default, which keeps the C4 arbiter: T_batt is the fraction "
                 "SPENT, T_query is the access category (a constant), a denied request is "
                 "forgotten, only gateways arbitrate, and hearing a 2.4 GHz request counts as "
                 "UWB reach. TRUE: the request carries the battery REMAINING, scored "
                 "min(1/max(rem, 0.05), 20) x 0.05 (the 0.05 is OURS); T_query is the "
                 "request's query-triggered bit (NW-4); an arbiter considers a request only "
                 "with its own evidence of UWB reach -- a UWB exchange, or a 2.4 GHz range "
                 "under 29.5 m, within 10 s (OURS) -- else denies it with queue position 0; a "
                 "request that cannot have the slot is queued and told its position, and each "
                 "cycle a free slot goes to the queue's argmax, staleness aging by the time "
                 "waited; an entry older than 30 s (OURS) or out of range is dropped with a "
                 "final denial; finders arbitrate their own UWB radio too (our mapping of the "
                 "spec's vehicle-mounted anchors). Under --grantGatedUwb a queued request "
                 "keeps the node waiting. Writes arb_<arm>_seed<N>.csv, digested, and one "
                 "NW-5 line. Needs --uwbArbitration and --handshakeFixes.",
                 spec.arbitrationV2);
    cmd.AddValue("sessionOverMesh",
                 "The session layer becomes a real search (Transport sect 2: 'a rescuer's "
                 "handheld/base station issues a search'; 'ON any RESPONSE received for this "
                 "session: result = extract_position_and_provenance(RESPONSE)'; "
                 "docs/SPEC_CONFORMANCE_PLAN.md TA-1a). FALSE by default, which is every "
                 "--sessionLayer result so far: the session is opened by the measured tag for "
                 "itself and fed its own EKF estimate every cycle. TRUE: the BASE STATION "
                 "(requester 0, behind the gateway backhaul) opens one session over what its "
                 "identity queries ask (a batch under --identityQueryManifests), and is fed "
                 "only by RESPONSEs delivered to any gateway -- once per answer -- with the "
                 "RESULT built from the delivered entry: position, Sigma, the producer's fix "
                 "tier and hop count, PCS (its byte, so saturating; TECH_DEBT N.7) and "
                 "integrity verdict. Writes session_<arm>_seed<N>.csv (every result and every "
                 "state change), digested, and one TA-1 line. Needs --sessionLayer and "
                 "--queryV2. Finder sessions (TA-1b) are not built.",
                 spec.sessionOverMesh);
    cmd.AddValue("finderSessions",
                 "Every finder runs its own search (Transport sect 2: 'a rescuer's handheld "
                 "... issues a search'; docs/SPEC_CONFORMANCE_PLAN.md TA-1b), beside the base "
                 "station's (--sessionOverMesh). FALSE by default. TRUE: finder k opens a "
                 "session for subject (k mod N) + 1 (N = --measuredSubjects; the rule OURS, "
                 "as --finderTasking's), queries for it from its own tier-1 radio on the "
                 "identity query period (URGENT under --identityQueryUrgent), and is fed only "
                 "by the answers that reach it as the requester -- over the reverse path to a "
                 "moving node. The session ages like the base station's and is CANCELLED "
                 "once the finder is within --finderCancelM of its target (true positions, "
                 "OURS): the spec's explicit cancel. Rows tagged with the finder id in session_*, and "
                 "one TA-1b line per finder. Needs --sessionOverMesh and --nFinders > 0.",
                 spec.finderSessions);
    cmd.AddValue("requesterEscalation",
                 "The requester drives escalation (Transport sect 4: on a RESULT with PCS below "
                 "the threshold and integrity CLEAN, it submits an escalation request for the "
                 "target; docs/SPEC_CONFORMANCE_PLAN.md TA-2). FALSE by default: only a node's "
                 "own PCS decision asks. TRUE, under --sessionOverMesh: the base station asks "
                 "for UWB for a target whose delivered entry is clean, not on UWB and below "
                 "the 2.4 GHz up threshold -- at the arbiter the target's entry names as the "
                 "one it last had UWB-reach evidence with (within 10 s), never 'the nearest "
                 "gateway'; with none it is unroutable. The request reaches the arbiter instantly and unbilled (OURS) "
                 "and is scored like any other (marked query-triggered when the queries are "
                 "URGENT); a grant goes to the target, which takes it 2.4 GHz -> UWB. At most "
                 "one per target per slot hold. Writes reqesc_<arm>_seed<N>.csv, digested, and "
                 "one TA-2 line. Needs --sessionOverMesh, --arbitrationV2 and --grantGatedUwb.",
                 spec.requesterEscalation);
    cmd.AddValue("finderCancelM",
                 "Under --finderSessions, how close (m, true positions) a finder must come to "
                 "its target to cancel its session as 'target physically reached'. 2 (default) "
                 "is OURS. With finders standing off 8 m from the estimate it rarely fires; a "
                 "larger value exercises the cancel.",
                 spec.finderCancelM);
    cmd.AddValue("sessionPreciseOnGrant",
                 "Under --sessionOverMesh, the session becomes LOCATED_PRECISE on a confirmed "
                 "UWB grant (Transport sect 4: 'ON UWB grant confirmed: session.state = "
                 "LOCATED_PRECISE'; docs/SPEC_CONFORMANCE_PLAN.md TA-3), learned the only way "
                 "the requester can learn it: a delivered entry saying its producer is on UWB "
                 "with a grantor (NW-7, NW-9) -- one delivery after the fact, which is "
                 "realistic. A result from a UWB fix without a grant is coarse. FALSE by "
                 "default: a UWB fix is precise. Needs --sessionOverMesh and --grantGatedUwb.",
                 spec.sessionPreciseOnGrant);
    cmd.AddValue("sessionStalenessS",
                 "Under --sessionOverMesh, seconds without a result after which a session "
                 "goes STALE (the session manager's default, 120).",
                 spec.sessionStalenessS);
    cmd.AddValue("sessionTimeoutS",
                 "Under --sessionOverMesh, seconds after which a session with no result "
                 "times out (the session manager's default, 600).",
                 spec.sessionTimeoutS);
    cmd.AddValue("finderRequesterEscalation",
                 "A finder's own session drives escalation, as the base station's does "
                 "(Transport sect 4; docs/SPEC_CONFORMANCE_PLAN.md TA-1b; TECH_DEBT N.10). FALSE by "
                 "default: only the base station asks (--requesterEscalation). TRUE: a finder whose "
                 "session receives a clean result below the 2.4 GHz up-threshold for a target not "
                 "on UWB asks, at most once per target per --arbSlotHoldS. A finder is itself a UWB "
                 "arbiter (NW-5): holding reach evidence of the target it scores the request in its "
                 "own queue ('own', no frame); else the request goes to the arbiter the target's "
                 "entry names, as TA-2's ('named', instant and unbilled); else it is unroutable "
                 "and not sent (OURS). Writes finderreq_<arm>_seed<N>.csv (digested) and one TA-1b "
                 "line. Needs --finderSessions and --requesterEscalation.",
                 spec.finderRequesterEscalation);
    cmd.AddValue("fuseFloorActive",
                 "Fuse an active tier short of three with the 868 MHz floor (TECH_DEBT N.14; owner "
                 "decision 2026-10-03: built behind a flag, adopted only by a registered campaign). "
                 "FALSE by default: the fix set is the active tier's when it has three or more "
                 "ranges, else the floor's when IT has three or more, and only then the active "
                 "tier's 1-2 ranges for the sparse cases -- so a UWB or 2.4 GHz range is discarded "
                 "whenever the floor solves, and two floor ranges are discarded whenever the node "
                 "is off 868 MHz. TRUE: when the active tier has fewer than three ranges and the "
                 "floor ranged, the fix set is the union (the active tier's ranges, relays "
                 "included, then the floor's), and Step()'s sparse cases act on it below three; "
                 "the floor alone at three or more is the old path. A union holding active ranges "
                 "is labelled with the active tier in the traces (OURS: the cycle used them, and "
                 "they are the precise ones), which overrides the rule that each cycle's fix "
                 "comes from one tier. Tag 0 and every measured subject; under "
                 "--subjectRelayChain a union of three is a fix on direct ranges, and relays may "
                 "complete a shorter one. Writes fusefloor_<arm>_seed<N>.csv (digested) and one "
                 "N.14 line. Needs --sparseInStep.",
                 spec.fuseFloorActive);
    cmd.AddValue("subjectRelayChain",
                 "Measured subjects take part in the relay chain (L.1.2; TECH_DEBT N.3 and N.15). "
                 "FALSE by default: a subject steps through the MS-1 pipeline, never ranges "
                 "another node, and stays at depth 0, so a range through it is depth 1 and depth "
                 "2 cannot arise when every field node is a subject (G24's zero under the "
                 "preset). TRUE: a subject with three or more direct ranges (active tier or "
                 "floor) holds a fix at depth 0; one short of three on both ranges, on its active "
                 "tier and billed there (never on 868 MHz, where A1 admits no relayed range), "
                 "every node that holds a fix this cycle and is awake on "
                 "that tier (plain relays, which step first, and lower-id subjects; tag 0 steps "
                 "last and never is one), with the chain's composition and A1-A5 admission, and "
                 "the admitted ranges join its active set only if they complete three (OURS: the "
                 "sparse cases never take a relayed range, and the floor's ranges are not mixed "
                 "with another tier's). Fused, it carries the deepest holder's depth + 1, and tag "
                 "0's relayed range through it is one deeper. relay_chain_* gains the subjects' "
                 "rows and fuses; one L.1.2 subject-chain line. Needs --relayChain and "
                 "--measuredSubjects > 1.",
                 spec.subjectRelayChain);
    cmd.AddValue("hopProvenance",
                 "The hop count of what a fix fused reaches the decision and the RESULT "
                 "(Physical sect 2.5 folds hop-decay into trace(Sigma); Transport sect 3's "
                 "RESULT carries hop_count_at_fix; docs/SPEC_CONFORMANCE_PLAN.md NW-1). FALSE "
                 "by default, which is every result so far: Step() is passed a literal 0 and "
                 "the RESULT says 0. TRUE: the measured tag's Step() and every short relay's "
                 "get the deepest leg of the set they fuse (relayed ranges carry their depth, "
                 "direct ones 0), and a fix that fused it carries it as hop_count_at_fix. "
                 "hopWeight stays 0 -- the spec's PCS has no hop term and hop enters through "
                 "Sigma, which C5's admission already inflates -- so no decision moves; what "
                 "moves is that the count is carried. Writes hops_<arm>_seed<N>.csv, "
                 "digested, and one NW-1 line. Needs --relayedRanging.",
                 spec.hopProvenance);
    cmd.AddValue("hopVarianceTerms",
                 "Per-hop RSSI/SNR variance and per-hop NLOS likelihood enter a relayed "
                 "range's sigma (the final plan's item 1; Physical sect 2.5 folds them into "
                 "trace(Sigma); docs/SPEC_CONFORMANCE_PLAN.md NW-1 step 3). FALSE by default: "
                 "a leg's sigma is the engine's, and C5 adds the relay's projected variance. "
                 "TRUE: every relay leg that reaches admission, on either leg of the chain, "
                 "adds the von Neumann sigma of its RSSI/SNR readings (window 8, three "
                 "readings, keyed by initiator and relay) converted to range by the tier's "
                 "nominal path-loss slope at the range (OURS: tiers 2 and 3 range by time of "
                 "flight), sigma_leg = sqrt(sigma^2 + sigma_var^2); a leg the tier's NLOS score "
                 "puts at or above --nlosScoreThreshold (2.4 GHz: ToF/RSSI excess against a "
                 "pool of legs; UWB: the CIR leading-edge test) is multiplied by "
                 "nlosSigmaInflation, as a direct NLOS link is; C5 composes sigma_eff from "
                 "sigma_leg. relay_chain_* gains the leg's readings and terms, and one NW-1 "
                 "line. Needs --relayedRanging, --relayChain and --nlosClassifier.",
                 spec.hopVarianceTerms);
    cmd.AddValue("fixRoutingSubjects",
                 "Every measured subject routes its own position-fix traffic (Network sect 3; "
                 "docs/SPEC_CONFORMANCE_PLAN.md NW-2). FALSE by default: --fixRouting routes "
                 "the relays' fixes on --relayTier and the measured tag never routes. TRUE: one "
                 "router per tier, the measured tag included; each cycle a node takes part -- "
                 "learns gateway links, advertises, originates its ENTRY_PUSH, forwards -- only "
                 "in the router of the tier it ranged on, so a routed frame is on the one tier "
                 "that node transmits on (sect 6); a relay that is not a subject stays on "
                 "--relayTier. A route whose next hop has since moved tier is not sent "
                 "(counted). Writes fix_routing_<arm>_seed<N>_tier<k>.csv per tier, with the "
                 "confidence route's three sums, digested, and an NW-2 line per router. Needs "
                 "--fixRouting and --measuredSubjects > 1; and --fixRouting with "
                 "--measuredSubjects > 1 is refused without it (TECH_DEBT N.12).",
                 spec.fixRoutingSubjects);
    bool specProfileArg = false;
    {
        std::ostringstream h;
        h << "The spec preset (docs/SPEC_CONFORMANCE_PLAN.md sect 3.1, owner decision 1). FALSE by "
             "default: every run is as before. TRUE: each setting below is applied unless the "
             "same flag is given explicitly, which always wins; spec_profile.txt (digested) lists "
             "what was set and what was kept, and a NOTE goes to stderr. FATAL with --topology=1. "
             "The geometry is GEO-01's registered "
             "fallback (no candidate met the criterion); the PCS is CAL-02's calibrated form; "
             "--fuseFloorActive is left out (FUSE-01b failed FUSE-01's adoption rule, "
             "report/FUSE01B_RESULTS.md); "
             "the routing weights are CAMP-SPEC's sweep pick (0,0,1,1), unused (not refused) "
             "when --fixRouting is turned off explicitly, as --subjectRelayChain is when "
             "--relayChain is. --hopVarianceTerms is left out "
             "(owner decision, TECH_DEBT N.11). Settings:";
        for (const CaTblSpecSetting& ss : kSpecProfile)
        {
            h << " --" << ss.name << "=" << ss.value << " (" << ss.why << ");";
        }
        cmd.AddValue("specProfile", h.str(), specProfileArg);
    }
    cmd.AddValue("caseCRelative",
                 "Case C's relative shape (Physical sect 2.4: 0 anchors, 2+ peers -- "
                 "relative-shape-only reconstruction, never an absolute position; "
                 "docs/SPEC_CONFORMANCE_PLAN.md DC-3). FALSE by default: a 0-anchor cycle is "
                 "counted as Case C and coasts. TRUE: a measured subject with no anchor range "
                 "this cycle (active tier and floor) ranges each peer subject its active radio "
                 "has heard, on that tier, billed there (initiator pays); the ranges ride its "
                 "next entry at 3 bytes a range (OURS, billed as the extra airtime) and reach "
                 "the base station when a RESPONSE carrying that entry is delivered to a gateway "
                 "(--sessionOverMesh), within 10 s (OURS). "
                 "Once a cycle's window closes the base station reconstructs each connected set "
                 "of 3+ by classical MDS (missing pairs by shortest path, OURS, counted) and a "
                 "pair as its distance. Nothing is fused into any filter. Writes "
                 "case_c_ranges_<arm>_seed<N>.csv and case_c_relative_<arm>_seed<N>.csv, "
                 "digested, and one DC-3 line. Needs --measuredSubjects > 1, --meshLinks=all and "
                 "--sessionOverMesh.",
                 spec.caseCRelative);
    cmd.AddValue("routeAlpha",
                 "Weight of the NLOS term in --fixRouting's confidence cost (Network sect 3, "
                 "path_cost = alpha*NLOS + beta*RSSI_var + gamma*jitter; NW-2). 1 is every result "
                 "so far, OURS; the spec gives none and says tune empirically. Needs --fixRouting.",
                 spec.routeAlpha);
    cmd.AddValue("routeBeta",
                 "Weight of the RSSI-variance term (dB^2) in the confidence cost; 1 (OURS). At "
                 "equal weights this term dominates. Needs --fixRouting.",
                 spec.routeBeta);
    cmd.AddValue("routeGamma",
                 "Weight of the relay-jitter term (s) in the confidence cost; 1 (OURS). Needs "
                 "--fixRouting.",
                 spec.routeGamma);
    cmd.AddValue("routeSlack",
                 "Extra hops over the shortest route the confidence cost may take (Network sect "
                 "3: start at 1, tune empirically); 1 (OURS), at most 3 (FixRouter's hop limit). "
                 "Needs --fixRouting.",
                 spec.routeSlack);
    cmd.AddValue("arbSlotHoldS",
                 "Seconds an awarded UWB slot is held before an arbiter treats it as free "
                 "(CaTblMacConfig::arbSlotHoldS). 10 (default) is the C4 harness constant "
                 "every result so far used. Longer holds make requests contend for the slot "
                 "-- queueing under --arbitrationV2, and its gossip under --gossipQueue -- "
                 "which at 10 s is rare. Read only with --uwbArbitration.",
                 spec.arbSlotHoldS);
    cmd.AddValue("gossipQueue",
                 "Anchors gossip their queue as well as their grants (Network sect 6: "
                 "'anchors periodically gossip their current Q (request queue) and active "
                 "grants'; docs/SPEC_CONFORMANCE_PLAN.md NW-6). FALSE by default: the gossip "
                 "carries grants only. TRUE: it carries the --arbitrationV2 queue too (4 bytes "
                 "an item, layout OURS, billed); a heard queue is stored, and the lower-id "
                 "tie-break extends to queues -- an arbiter drops, with a final denial, a "
                 "queued request a higher-id anchor queues or has granted, refuses to queue "
                 "one, and frees a slot whose grant it has yielded. Under --meshLinks=all the "
                 "finders gossip too (their links to gateways are the mesh's), unbilled like "
                 "everything a finder does. One NW-6 line. Needs --anchorGrantGossipS > 0 and "
                 "--arbitrationV2.",
                 spec.gossipQueue);
    std::string identityQueryManifestsArg;
    cmd.AddValue("identityQueryManifests",
                 "Under --queryV2, the comma-separated manifest list each query asks for "
                 "(e.g. 1,2,3: a batch for a missing team). Empty (default) asks for "
                 "--identityQueryTarget alone. Each id is 1..255.",
                 identityQueryManifestsArg);
    cmd.AddValue("imuIdeal",
                 "Replace the SHS with a PERFECT dead reckoner for --imuFusion. FALSE by "
                 "default. A CEILING, NOT A SYSTEM RESULT: the engine is anchored at the "
                 "last clean trusted fix and advances by the true displacement, so any "
                 "figure under it is an upper bound on what a flawless IMU could add to "
                 "this architecture and must be reported as a bound. Not error-free: "
                 "the anchor is the FILTER estimate and carries that fix error, held "
                 "constant rather than compounded, which is what separates a perfect "
                 "inertial system from an oracle. Exists because the real SHS gait "
                 "calibration diverges where fixes are sparse -- the regime fusion is "
                 "for -- and that defect is recorded as debt, not fixed.",
                 imuIdealArg);
    cmd.AddValue("imuFusion",
                 "Fuse the SHS position into the filter on cycles with NO radio fix. "
                 "FALSE by default. THE MISSING LINK: the SHS maintains an absolute "
                 "position and nothing consumed it -- its only use was a disagreement "
                 "magnitude vetoing escalation, so the data flow ran filter -> SHS and "
                 "never back. Meanwhile the filter coasts through 30 percent of cycles "
                 "in the best deployment tested and 99.6 percent with a stationary "
                 "subject, and measured 868 error degrades with that gap -- 183 m at "
                 "no gap, 1205 m after 32-60 blind cycles. Measurement variance is "
                 "(imuSigmaFloor + imuDrift * unaided time)^2: the SHS error model, "
                 "drift accumulating with UNAIDED TIME rather than distance walked. "
                 "Needs --shsTrustedFixK, or the SHS is calibrated on noise and fusing "
                 "it makes things worse.",
                 imuFusionArg);
    cmd.AddValue("shsTrustedFixK",
                 "Offer a fix to the SHS only when the node has moved more than K "
                 "position-sigmas since the last one. 0 (default) feeds every fix, "
                 "which is what every run before 2026-09-08 did. WHY: OnTrustedFix "
                 "CALIBRATES step length and heading from the displacement between "
                 "consecutive fixes, and the node walks ~3 m per cycle while the "
                 "fixes carry ~167 m of error -- about 1 part signal to 56 parts "
                 "noise, so the implied heading is noise and the engine dead-reckons "
                 "on it for kilometres. Measured: arms feeding every fix ran 5-18 km "
                 "off while the one arm that fed none stayed at 20-42 m. K around 2-3 "
                 "makes calibration sparse and informative.",
                 shsTrustedFixKArg);
    cmd.AddValue("maxTurnRate",
                 "Bound the walkers' heading change, rad/s. 0.0 (default) is unbounded, "
                 "which is every run before 2026-09-08. WHY IT EXISTS: the SHS reaches "
                 "~330 m of final error and DESIGN_DECISIONS section 8 names the cause -- "
                 "the walker changes heading discontinuously while the engine "
                 "integrates yaw rate. The travel leg already clamps to 0.35 rad/s; the "
                 "dwell did not, and reversed heading INSTANTANEOUSLY on reaching the "
                 "dwell radius. A yaw integrator cannot follow that. 0.35 matches the "
                 "travel leg. THIS CHANGES THE MOBILITY, so with it on no earlier "
                 "number is comparable.",
                 maxTurnRateRadS);
    cmd.AddValue("subjectKeepOutM",
                 "With a base camp, drop any tour site within this many metres of it, "
                 "so the subject does not wander back into coverage. 0.0 = off. The "
                 "subject also STARTS at the camp when a camp exists -- the last "
                 "known position, which is what a search begins from and what the "
                 "finders converge on. WHAT THIS MAKES TESTABLE: with the subject "
                 "outside gateway range every single-band arm goes blind, and the only "
                 "path to a fix is a finder walking within range. Finders are "
                 "registered on all three tiers, so three within 868's 1515 m give a "
                 "tier-1 fix with ZERO gateways -- mobile anchors, not merely "
                 "refinement -- and the error should then collapse as they close to "
                 "156 m for tier 2 and to contact for tier 3. Two sites are always "
                 "retained; a walker with fewer has no tour.",
                 subjectKeepOutM);
    cmd.AddValue("uwbTriad", "UWB triad radius at each facility, metres", uwbTriadRadiusM);
    cmd.AddValue("t2Triad", "2.4 GHz triad radius at each facility, metres", t2TriadRadiusM);
    cmd.AddValue("uwbAnchors", "UWB anchors per facility site (>=3)", uwbAnchorsPerSite);
    cmd.AddValue("t2Anchors", "2.4 GHz anchors per facility site (>=3)", t2AnchorsPerSite);
    cmd.AddValue("speed", "Walking speed, m/s", walkSpeedMps);
    cmd.AddValue("dwell", "Mean dwell time at a facility, seconds", dwellS);
    cmd.AddValue("vegWide",
                 "VegetationType for the wide-area 868 tier (0=none 1=sparse 2=light "
                 "3=dense 4=tropical 5=crop 6=orchard)",
                 vegWideInt);
    cmd.AddValue("vegFacility",
                 "VegetationType for the 2.4 GHz and UWB tiers. Default 255 = "
                 "FOLLOW --vegWide, for the same reason --envFacility does. The "
                 "old default was VEG_SPARSE_SHRUBS against tier 1's "
                 "VEG_DENSE_FOREST, which is 3.2 dB of ITU-R P.833 foliage "
                 "loss against 13.8 dB at the modelled median depth -- 10.6 dB "
                 "handed to tier 2 on links through the same trees.",
                 vegFacilityInt);
    cmd.AddValue("beacon868", "Anchor DISCOVERY beacon period on tier 1, seconds", beacon868S);
    cmd.AddValue("beacon2p4", "Anchor DISCOVERY beacon period on tier 2, seconds", beacon2p4S);
    cmd.AddValue("beaconUwb", "Anchor DISCOVERY beacon period on tier 3, seconds", beaconUwbS);
    cmd.AddValue("nbrTtlMult",
                 "Neighbour table TTL as a multiple of that tier's beacon period",
                 nbrTtlMult);
    cmd.AddValue("fastDemote",
                 "Demote on the first zero-neighbour cycle: off|proposed|all. "
                 "'proposed' applies it to ca_tbl_full/ca_tbl_no_integrity only; "
                 "'all' also gives it to fixed_threshold, which is the fair-baseline "
                 "control. off reproduces the pre-existing baseline byte-identically.",
                 fastDemote);
    cmd.AddValue("suspectRecovery",
                 "Response when the integrity check (the Mahalanobis one, not the "
                 "evidence gate) flags a fix: none|inflate|bounded|reinit|downweight. "
                 "none (default) is the pre-existing behaviour -- reject and coast, "
                 "which measurably locks the filter for up to 251 consecutive cycles "
                 "(22 with no fault injected at all). downweight is the spec's "
                 "response (Physical 2.6; docs/SPEC_CONFORMANCE_PLAN.md DC-7): the fix "
                 "is ACCEPTED at R x --suspectInflateFactor, stays flagged SUSPECT so "
                 "it cannot count toward a de-escalation, and its PCS is scaled by "
                 "--suspectPcsFactor. The spec never says a suspect fix forces "
                 "escalation; --suspectForcesEscalation still does unless set false.",
                 suspectRecoveryArg);
    cmd.AddValue("suspectInflateFactor",
                 "INFLATE: factor applied to the position covariance on rejection. "
                 "REINIT: multiplier on the raw fix covariance when reseeding. "
                 "DOWNWEIGHT: multiplier on R -- every range variance and the TDoA "
                 "fix's covariance -- when the suspect fix is fused. Fused at full R, a "
                 "persistent fault is absorbed into the prior within a cycle and the "
                 "check has nothing left to disagree with.",
                 suspectInflateFactor);
    cmd.AddValue("suspectRecoveryN",
                 "BOUNDED/REINIT: consecutive rejections before recovery fires. "
                 "Default 5 = the p90 of measured rejection-run lengths on F1.",
                 suspectRecoveryN);
    cmd.AddValue("suspectPcsFactor",
                 "DOWNWEIGHT only: what a suspect cycle's PCS is multiplied by, in "
                 "(0, 1]; under --pcsForm=v2, whose score can be negative, "
                 "log10(1/factor) is subtracted instead. Not set (nan) is 0.5, which is "
                 "OURS: the spec says 'downweight' and gives no magnitude. Refused "
                 "without --suspectRecovery=downweight, where it would do nothing.",
                 spec.suspectPcsFactor);
    cmd.AddValue("escalationRequiresEvidence",
                 "Escalate 2.4->UWB only when the node's own EKF estimate predicts >=3 "
                 "UWB anchors within uwbPredictRadiusM, behind an estimate-quality gate. "
                 "Suppress-only; never creates an escalation. Does NOT gate 868->2.4 -- "
                 "see escalationRequiresEvidenceT2 for why that one does not work. "
                 "TRUE by default: K3 measured it worth 323.75 J of a 339 J arm "
                 "at 20 of 20 seeds, p < 0.0001, with no significant accuracy "
                 "change -- without it the policy escalates to UWB 92 more times "
                 "per run, into a tier that obtained ZERO fix cycles in all "
                 "eleven cells of K1. THE CAVEAT: this was measured where tier 3 "
                 "never delivers, and the gate is suppress-only, so where tier 3 "
                 "DOES work suppression could cost. Pass false to reproduce a run "
                 "taken before 2026-09-08.",
                 escalationRequiresEvidence);
    cmd.AddValue("escalationRequiresEvidenceT2",
                 "Gate the 868->2.4 escalation on whether tier 2 could actually reach. "
                 "FALSE by default. The old version of this flag asked the MAC "
                 "neighbour cache and was BROKEN -- the tier-2 radio sleeps while "
                 "tier 1 is active, so the count was always 0 and enabling it latched "
                 "the node at the floor, 1440/1440 cycles on 868 with 0 escalations. "
                 "Rebuilt 2026-09-08 as a PREDICTOR symmetric with --uwbEvidenceFromNeighbours: "
                 "the ranges taken this cycle came from an AWAKE radio, so a neighbour "
                 "ranged closer than --t2PredictRadius is one 2.4 GHz would reach. "
                 "Geometry rather than a map, so it follows finders. WHY IT MATTERS: at "
                 "boxHalf 6000 the policy escalates 199 times into a tier with ZERO fix "
                 "cycles and pays ~18 J for it, which is the whole energy gap to "
                 "static_868.",
                 escalationRequiresEvidenceT2Arg);
    cmd.AddValue("t2PredictRadius",
                 "Radius for that predicate, metres. 156.0 = the probe r80 AFTER the "
                 "environment correction. The pre-correction figure was 909.5 m and is "
                 "5.8x too generous: a predicate at that radius admits escalations "
                 "into a tier that cannot reach, which is the failure it exists to "
                 "prevent.",
                 t2PredictRadiusMArg);
    cmd.AddValue("t2PredictMinAnchors",
                 "Neighbours inside that radius before escalation is allowed. 3, "
                 "because three is what a fix needs. The UWB predicate uses presence "
                 "rather than count because tier 3 is a contact indicator; tier 2 is a "
                 "localisation tier and one reachable anchor cannot solve.",
                 t2PredictMinAnchorsArg);
    cmd.AddValue("uwbEvidenceFromNeighbours",
                 "Take the UWB half of that gate from the LIVE neighbour table rather "
                 "than the static anchor map. FALSE by default. The map is a snapshot "
                 "taken at scenario start, and under the tri-band topology tier 3 comes "
                 "from FINDERS, which move and are never in it -- so the predicate asks "
                 "about a deployment this system no longer has. Presence, not count: a "
                 "suppress-only gate should ask whether the tier has anything here, not "
                 "whether a fix will solve. Symmetric with the 868->2.4 half, which has "
                 "always read the same live evidence.",
                 uwbEvidenceFromNeighboursArg);
    cmd.AddValue("mahalUseR",
                 "Use the correctly specified integrity gate: weighted "
                 "multilateration for the test point and S = P_prior + (J^T R^-1 J)^-1 "
                 "for the distance. false reproduces the pre-existing baseline.",
                 mahalUseR);
    cmd.AddValue("gwGrid",
                 "Tier-1 gateways per grid side (3 = the historical 3x3). Sweeping "
                 "--boxHalf at fixed gwGrid varies gateway DENSITY as well as tier-2 "
                 "coverage; use gwGrid=3*boxHalf/1000 to hold density constant.",
                 gwGrid);
    cmd.AddValue("multiStepDemote",
                 "On a blind demotion, drop to the highest tier known populated "
                 "rather than one step: off|proposed|all. Separate axis from "
                 "fastDemote. off reproduces the pre-existing baseline.",
                 multiStepDemote);
    cmd.AddValue("demoteStalenessS",
                 "How long a tier stays 'known populated' after sleeping, seconds. "
                 "OURS AND UNCITED -- sweep it.",
                 demoteStalenessS);
    cmd.AddValue("searchDuty",
                 "Receiver-open fraction of a beacon period while a tier is awake "
                 "with an empty neighbour table (1.0 = consistent, 0.0 = free search)",
                 searchDutyFrac);
    cmd.AddValue("arms",
                 "Comma-separated arms to run, or 'all' (default). Sound only "
                 "because RunArm() resets the RNG stream index per arm, so an "
                 "arm's channel realisation does not depend on which arms ran "
                 "before it; a filtered run's row is byte-identical to the full "
                 "run's. Exists for the operating-point sweep, where four of the "
                 "six arms are pure runtime.",
                 armsArg);
    cmd.AddValue("injectRangeBias",
                 "S7-lite: metres of COMMON-MODE range bias added to every tier-2 "
                 "range during the fault window. Deliberately does NOT set the NLOS "
                 "flag or inflate sigma -- `suspect` requires all-LOS, so a fault "
                 "that announces itself is invisible to the gate by construction and "
                 "would test nothing. 0 (default) disables.",
                 injectRangeBiasM);
    cmd.AddValue("injectFaultAnchors",
                 "S7 fault mode. 0 (default) = COMMON-MODE bias on every tier-2 link "
                 "(F3, h = m). 1 = SINGLE-ANCHOR bias (F1, h = 1), the canonical RAIM "
                 "fault model and the physically realistic one -- one anchor displaced, "
                 "drifting, detuned or blocked. The faulted anchor is seed-derived so it "
                 "is reproducible and identical across arms.",
                 injectFaultAnchors);
    cmd.AddValue("injectBiasFraction",
                 "S7-lite: fraction of the run over which the bias is active, as one "
                 "contiguous window whose start is seed-derived and identical across "
                 "arms so the ablation measures the gate and not the fault placement. "
                 "0 (default) disables.",
                 injectBiasFraction);
    cmd.AddValue("floorOnDemand",
                 "Range the tier-1 floor only on cycles where the active tier failed "
                 "to deliver >=3 ranges, instead of unconditionally. The tier-1 radio "
                 "stays AWAKE and listening either way -- suppressing that would "
                 "trigger ClearNeighbours() and leave the floor with an empty table "
                 "exactly when it is needed. Measured motivation: the unconditional "
                 "floor ranges on 99.9% of cycles, costs 54.5% of arm energy, and "
                 "supplies the fix on 1.16% (boxHalf=1500). Default false, so the "
                 "legacy envelope reproduces byte-identically.",
                 floorOnDemand);
    cmd.AddValue("floorSkipK",
                 "Consecutive healthy cycles required before floor ranging is "
                 "suppressed. 1 (default) is pure reactive and is the primary "
                 "configuration; >1 adds hysteresis whose benefit is unnamed and is "
                 "a sensitivity check only. Inert unless --floorOnDemand.",
                 floorSkipK);
    cmd.AddValue("floorHeartbeatN",
                 "Force floor ranging every N cycles regardless of health. 0 "
                 "(default) disables it. With the radio awake the neighbour table no "
                 "longer depends on this, so its only remaining purpose is bounding "
                 "EKF drift -- real but unquantified, so it does not carry the claim. "
                 "Inert unless --floorOnDemand.",
                 floorHeartbeatN);
    cmd.AddValue("fixedVarThreshold",
                 "fixed_threshold's escalation trigger, as a position VARIANCE in "
                 "m^2 (default 100.0, i.e. sigma = 10 m). Exposed so the literature "
                 "baseline can be swept across operating points: comparing it to "
                 "CA-TBL at one arbitrary point is unfair in whichever direction "
                 "the two constants happen to fall. <= 0 leaves the default.",
                 fixedVarThresholdM2);
    cmd.AddValue("pcsSigmaTargetM",
                 "PCS v2's target radial accuracy in metres (default 3.1623, i.e. "
                 "traceRef = 10 m^2). Moves the v2 operating point by moving the "
                 "SPECIFICATION rather than by hand-placing a threshold -- the "
                 "threshold stays derived at 0.0. The sweep counterpart of "
                 "--fixedVarThreshold, in the same physical quantity. <= 0 leaves "
                 "the default.",
                 pcsSigmaTargetM);
    cmd.AddValue("deferEscalationOnDutyLimit",
                 "If the escalation handshake cannot be transmitted within the "
                 "duty-cycle budget, defer the ESCALATION rather than merely "
                 "skipping its energy charge -- a handoff that cannot be signalled "
                 "has not happened. Measures the coupling from a regulatory airtime "
                 "limit into handoff latency and hence position error. Demotion is "
                 "never deferred. Default TRUE: with it false the tier change still "
                 "commits while the handshake goes uncharged, and the size of that "
                 "free ride is arm-correlated (100% of fixed_threshold's handshakes "
                 "vs 1% of ca_tbl's), biasing energy in the BASELINE's favour. Set "
                 "false to reach the biased mode for comparison.",
                 deferEscalationOnDutyLimit);
    cmd.AddValue("arbitrateDutyBudget",
                 "When an escalation handshake is deferred by the duty-cycle "
                 "governor, pre-empt the next cycle's ranging exchange ON THE "
                 "CAPPED TIER THE HANDSHAKE DEPARTS (tier 1, not the active "
                 "tier -- tiers 2/3 have no duty cap, so surrendering their "
                 "ranging frees nothing) and use the freed budget for the "
                 "handshake. The node loses that cycle's tier-1 measurements; "
                 "the cost is real and charged. Implements "
                 "the access-category priority the MAC spec already defines "
                 "(CW_high above CW_normal) in the duty-cycle governor — closing "
                 "an omission, not adding a new rule. Default TRUE.",
                 arbitrateDutyBudget);
    cmd.AddValue("pcsForm",
                 "Position Confidence Score functional form: legacy|v2. 'legacy' "
                 "is traceRef/(traceRef+traceSigma) + 1/max(gdop,1) + losFrac, "
                 "whose term 1 saturates, term 2 is inverted across tiers and "
                 "term 3 is a near-constant offset; it remains the default so "
                 "results/baseline_v4 reproduces byte-identically. 'v2' is "
                 "log10(sigma_ref / sqrt(sigma_post * sigma_geo)) -- decades of "
                 "position uncertainty from the 3.1623 m target traceRef already "
                 "specifies -- and carries its own pre-registered thresholds "
                 "(up 0.0, down log10(2), one target for all tiers). See "
                 "report/PCS_REDESIGN_RESULTS.md §0. Two more forms exist and this text "
                 "did not list them until 2026-09-30: 'weighted' is "
                 "pcsW1*t1 + pcsW2*t2 + pcsW3*t3 with the hand-set weights 2, 0.5, 0.5 "
                 "(--pcsW1..3), and 'gated' scores 0 whenever t1 < --pcsGateFloor and "
                 "the plain sum otherwise. Both reuse the legacy thresholds. "
                 "'calibrated' is the spec's form (docs/SPEC_CONFORMANCE_PLAN.md DC-5/6): "
                 "the weighted sum with --pcsW1..3, the three --pcsThrUp* and the two "
                 "--pcsThrDown* all given explicitly, the values a leave-one-out "
                 "calibration produced; it is refused unless all eight are given.",
                 pcsFormArg);
    cmd.AddValue("pcsW1",
                 "CA_TBL_PCS_WEIGHTED weight on the filter-uncertainty term. "
                 "Default 2.0; any value >= 0 is taken, 0 included (a requested 0 "
                 "was silently ignored until 2026-09-30). The defaults are "
                 "HAND-SET. CAL-01, the leave-one-out calibration, ran on "
                 "2026-09-28 and fitted (3, 0, 0) -- the trace alone -- which "
                 "--pcsForm=calibrated can now express. They are chosen for one property: "
                 "pcsW2 + pcsW3 = 1.0 sits below every pcsThresholdUp, so "
                 "term1 = 0 forces the score under the threshold whatever "
                 "geometry and LOS do. The unweighted LEGACY sum cannot do "
                 "that -- term2 + term3 alone reach 2.0 against 1.55 -- which "
                 "is the saturation defect this form repairs. Restoring the "
                 "veto is NOT the same as making the score informative; the "
                 "monotone-of-filter-statistic control decides that, and two "
                 "earlier forms failed it.",
                 pcsW1Arg);
    cmd.AddValue("pcsW2",
                 "CA_TBL_PCS_WEIGHTED weight on the geometry term "
                 "(1/max(gdop,1)). Default 0.5, held low deliberately: term 2 "
                 "is INVERTED across tiers -- GDOP is worse for the tightly "
                 "clustered facility triads than for spread gateways -- so it "
                 "rewards the coarse tier, and a large weight would re-import "
                 "that defect at full strength.",
                 pcsW2Arg);
    cmd.AddValue("pcsW3",
                 "CA_TBL_PCS_WEIGHTED weight on the LOS fraction. Default 0.5, "
                 "also held low: after the NLOS fix this term measured sd "
                 "0.0097 on tier 2 and was EXACTLY CONSTANT on tier 3, and a "
                 "near-constant term is an offset the threshold absorbs rather "
                 "than information.",
                 pcsW3Arg);
    cmd.AddValue("pcsGateFloor",
                 "CA_TBL_PCS_GATED veto floor on term 1. Default 0.10. OURS, "
                 "not physical: term1 = traceRef/(traceRef+traceSigma), so "
                 "0.10 is traceSigma = 9*traceRef, i.e. posterior radial sigma "
                 "three times the 3.1623 m target. Below that the filter is "
                 "not tracking to spec and the score should say so instead of "
                 "being carried by the other two terms.",
                 pcsGateFloorArg);
    cmd.AddValue("pcsHysteresisBand",
                 "Escalation/de-escalation hysteresis band: sets "
                 "pcsThresholdDown = pcsThresholdUp + this, on every tier. "
                 "Negative (default) leaves the built-in thresholds untouched "
                 "so the baseline reproduces byte-for-byte. This is "
                 "CAL-03/CAL-04, which the register defers to Phase 2.1 and "
                 "which has NEVER been swept, because until 2026-09-30 "
                 "pcsThresholdDown was not registered with AddValue at all "
                 "(it is now, per tier: --pcsThrDown2p4/--pcsThrDownUwb, which win over this). "
                 "The BAND is the swept quantity rather than the two absolute "
                 "thresholds: 868 MHz and 2.4 GHz carry a 0.20 gap today and UWB "
                 "0.25 (up 1.55/1.70/2.30, down 1.75/1.90/2.55; this text said 0.20 on "
                 "every tier until 2026-09-30), so one number moves the hysteresis "
                 "without silently also moving the operating point -- and sets UWB's "
                 "gap to the same band as the others.",
                 pcsHysteresisBandArg);
    {
        // DC-6: five explicit thresholds, registered in one loop so the help text is one
        // argument rather than five copies of it. There is no --pcsThrDown868: nothing is
        // below 868 MHz, so the policy's 868 MHz de-escalation threshold decides nothing,
        // and a flag that can only ever do nothing is TECH_DEBT I.4's defect class.
        static const char* kTierFlag[CA_TBL_TIER_COUNT] = {"868", "2p4", "Uwb"};
        static std::string upName[CA_TBL_TIER_COUNT];
        static std::string downName[CA_TBL_TIER_COUNT];
        for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
        {
            upName[k] = std::string("pcsThrUp") + kTierFlag[k];
            downName[k] = std::string("pcsThrDown") + kTierFlag[k];
            cmd.AddValue(upName[k],
                         "Explicit PCS escalation threshold for this tier: escalate when PCS "
                         "falls below it. Not set (nan, the default) keeps the form's own "
                         "(legacy/weighted/gated 1.55 / 1.70 / 2.30; v2 0.0). Applied after "
                         "--pcsHysteresisBand, so an explicit value wins. The thresholds the "
                         "spec asks for are the ones fitted with the weights by leave-one-out "
                         "calibration (docs/SPEC_CONFORMANCE_PLAN.md CAL-02); CAL-01 found the "
                         "old ones cannot serve a trace-only score, whose term 1 never "
                         "reaches 1.55 on a tier-1 fix. UWB's decides no escalation -- there "
                         "is no tier above -- but a score under it keeps a UWB node out of "
                         "the healthy-demotion count, so it is the UWB dwell's other half.",
                         spec.pcsThrUp[k]);
            if (k == CA_TBL_TIER_868)
            {
                continue;
            }
            cmd.AddValue(downName[k],
                         "Explicit PCS de-escalation threshold for this tier: demote after "
                         "--nDeescalate consecutive cycles above it. Not set (nan) keeps the "
                         "form's own (1.90 / 2.55; v2 log10 2), or --pcsHysteresisBand's. "
                         "Refused if the resolved pair is inverted, i.e. below the same "
                         "tier's escalation threshold, whichever of the two was given.",
                         spec.pcsThrDown[k]);
        }
    }
    cmd.AddValue("nDeescalate",
                 "Consecutive healthy cycles required before demoting a tier. "
                 "Zero (default) keeps the built-in 5. The other half of "
                 "CAL-04: the band sets how much better the score must get, "
                 "this sets for how long. 5 was itself never calibrated -- 141 "
                 "of 189 blind runs were exactly 5 cycles long, so the default "
                 "encodes an observation about one campaign, not a design "
                 "decision.",
                 nDeescalateArg);
    cmd.AddValue("sendHandshakeFrames",
                 "Whether the escalation handshake is TRANSMITTED as well as "
                 "charged. False (default) is the pre-existing behaviour and "
                 "must reproduce results/baseline_v7 byte-for-byte: "
                 "AccountEscalationHandshake() bills the airtime and the "
                 "energy, and no frame exists -- so the handshake has never "
                 "contended for the medium, never passed CAD, and never met "
                 "the duty governor AS A FRAME. Every duty-cycle finding in "
                 "the tree is therefore about a correctly-sized ACCOUNTING of "
                 "a handshake, not about one that contended. True adds the "
                 "frames and leaves the charge alone: adding rather than "
                 "replacing holds the energy axis fixed, so any change is "
                 "attributable to contention alone. It also avoids an energy "
                 "defect -- CaTblMac has no TX callback and SetEnergyModel() "
                 "is deliberately unset on every MAC, so a frame that "
                 "REPLACED the charge would transmit for free. Expect every "
                 "escalating arm to move when this is on; that is the "
                 "treatment, not a regression, and it needs a re-baseline.",
                 sendHandshakeFramesArg);
    cmd.AddValue("terrainPerTierFreq",
                 "Whether the terrain submodule uses each tier OWN carrier for "
                 "its Fresnel and diffraction geometry. False (default) keeps "
                 "the pre-existing behaviour, in which lora-terrain-submodule.cc "
                 "hardcoded 868 MHz and applied that wavelength to all three "
                 "tiers -- UWB Ch9 at 7.99 GHz included, where it is wrong by a "
                 "factor of 9.2. That is a defect, not a modelling choice, but "
                 "every result in the tree was measured under it, so it stays "
                 "the default and moves only when asked. True sets 2.4 GHz and "
                 "7.9872 GHz on tiers 2 and 3, the carriers Architecture.md 2.1 "
                 "records. NOTE that terrain is transcription-only and G11b is "
                 "permanently red, so this makes terrain LESS WRONG rather than "
                 "validated -- and both headline conclusions were already shown "
                 "invariant to terrain.",
                 terrainPerTierFreqArg);
    cmd.AddValue("intraCycleHandshakeDefer",
                 "Whether a handshake blocked by the tier-1 duty budget waits "
                 "for the budget to refill WITHIN this cycle instead of slipping "
                 "to the next one. False (default) keeps the existing behaviour, "
                 "in which the handshake is deferred and the arbiter then "
                 "pre-empts the next cycle ranging exchange to make room -- so a "
                 "tier-1 handoff costs a ranging slot. One SF7 frame is 56.58 ms, "
                 "which at the 2.5 percent WPC cap arms 2.263 s of off-time, and "
                 "that refill lands inside a 2.5 s cycle with 0.237 s to spare. "
                 "So the pre-emption is avoidable above dt of about 2.32 s, and "
                 "below it this simply never fires. The handshake is charged at "
                 "the refill time, not at the cycle start: the handoff really is "
                 "later, and that latency is accumulated and reported rather than "
                 "assumed away. What is not paid is the next cycle ranging.",
                 intraCycleHandshakeDeferArg);
    cmd.AddValue("gwRidge",
                 "Place gateways along a LINE (a ridge) instead of a ring or grid. FALSE "
                 "by default. This is how coverage is really deployed on a hiking route -- "
                 "repeaters follow the ridge because that is where the line of sight and "
                 "the access road are. It is BAD GEOMETRY on purpose: collinear anchors "
                 "give poor GDOP across the line, so a walker off the trail is far worse "
                 "localised than one on it, which is the weakness the scenario exists to "
                 "show. A ring would hide it.",
                 gwRidgeArg);
    cmd.AddValue("gwArcDeg",
                 "Angular sweep of the gateway ARC, degrees. 360 (DEFAULT) is the existing "
                 "full ring and reproduces it exactly. Below 360 the gateways follow a "
                 "curving access route -- a road or valley trail -- rather than closing "
                 "a ring around the search area. THIS IS THE HONEST MIDDLE between the "
                 "two deployments that are easy to build and misleading to measure: a "
                 "GRID puts anchors in the interior, which no team does and which "
                 "flatters geometry to 28 m CEP50; a RIDGE is collinear, so the solve is "
                 "near-singular off-axis and gives 1932 m. An arc has real 2-D spread, "
                 "so the solve is conditioned, but every anchor is on ONE SIDE, so "
                 "conditioning degrades as the subject leaves it in either direction. "
                 "Centred on north, so a trail beneath it can be strayed from either "
                 "way -- see --trailDeviateSide.",
                 gwArcDegArg);
    cmd.AddValue("gwLayout",
                 "Gateway placement shape: legacy|perimeter|cluster|star|lshape|file "
                 "(docs/SPEC_CONFORMANCE_PLAN.md MS-2, the shapes GEO-01 compares). legacy "
                 "(default) is every result so far: the grid, ring, arc or ridge the other "
                 "--gw* flags build. perimeter spaces --nGateways evenly round the square of "
                 "half-width --gwLayoutFrac x boxHalf; cluster rings them --gwClusterRadiusM "
                 "round the camp; star strings them --gwStarSpacingM apart along "
                 "--gwStarArms roads out of the camp, spread over 120 degrees towards the "
                 "box centre; lshape puts them on two perpendicular roads meeting at the "
                 "south-west corner; file reads x,y rows from --gwLayoutFile (the "
                 "optimiser's output). The camp is the base camp when --baseCampFrac gives "
                 "one, else the west midpoint of the frac square -- the trailhead side the "
                 "arc runs centre on (OURS). Every constant here is OURS. Prints one MS-2 "
                 "line with the coordinates. Needs --topology=0, and --nGateways > 0 except "
                 "for file.",
                 gwLayoutArg);
    cmd.AddValue("gwLayoutFrac",
                 "MS-2: the half-width of the square the perimeter and lshape layouts use, "
                 "as a fraction of boxHalf. 0.9 (default, OURS) is the inset the legacy "
                 "grid uses.",
                 gwLayoutFracArg);
    cmd.AddValue("gwClusterRadiusM",
                 "MS-2: radius of the cluster layout round the camp, metres. 300 (default, "
                 "OURS): a deployment a team sets up on foot from one vehicle.",
                 gwClusterRadiusMArg);
    cmd.AddValue("gwStarArms",
                 "MS-2: roads out of the camp in the star layout. 3 (default, OURS).",
                 gwStarArmsArg);
    cmd.AddValue("gwStarSpacingM",
                 "MS-2: spacing of gateways along each star road, metres. 1500 (default, "
                 "OURS): about the tier-1 r80 of 1515 m at the default environment.",
                 gwStarSpacingMArg);
    cmd.AddValue("gwLayoutFile",
                 "MS-2: the x,y CSV --gwLayout=file reads.",
                 gwLayoutFileArg);
    cmd.AddValue("gwArcCentreDeg",
                 "Bearing the gateway arc is centred on, degrees CCW from east. 90 "
                 "(DEFAULT) is north. Only meaningful with --gwArcDeg < 360. MEASURED "
                 "reason this exists: centred north, the subject starts at the box's "
                 "west edge OUTSIDE the arc's span, so on-trail error was 993 m and "
                 "off-trail 3.9 m -- the geometry got BETTER when the subject got lost, "
                 "and only because the searchers had arrived as pseudo-anchors. 180 "
                 "puts the containment at the trailhead the subject walks from, so "
                 "conditioning degrades with distance from access, which is what a "
                 "real deployment does and the direction the scenario needs.",
                 gwArcCentreDegArg);
    cmd.AddValue("trailDeviateSide",
                 "Which way the subject leaves the trail: -1 (DEFAULT) south, away from "
                 "an arc centred north; +1 north, through it. Default reproduces the "
                 "previous hard-coded southward deviation. NOT COSMETIC: with the "
                 "anchors on an arc the two sides are geometrically different. Inside "
                 "the arc the anchors subtend a wide angle and the fix stays "
                 "conditioned longer; outside it they collapse toward a single "
                 "bearing and the along-range direction goes soft first. A scenario "
                 "that can only stray one way tests one of those.",
                 trailDeviateSideArg);
    cmd.AddValue("gwRidgeY", "Ridge northing as a fraction of boxHalf.", gwRidgeYArg);
    cmd.AddValue("trailDeviateAtS",
                 "Seconds before the subject LEAVES the marked trail. 0 = off (the "
                 "ordinary wander). Before this the track is on the trail and entirely "
                 "predictable, which is what makes the deviation legible -- a viewer sees "
                 "the moment it stops being routine. After it the bearing is FIXED rather "
                 "than wandering: a person who has lost the path walks a line they believe "
                 "is right, and that makes the search geometric rather than diffusive.",
                 trailDeviateAtSArg);
    cmd.AddValue("trailStopAfterS",
                 "Seconds after deviating before the subject stops moving. 0 = keeps "
                 "walking. A stationary subject is the hard case -- K13 measured that no "
                 "steering helps once the contact rate is ~0 -- and it must be reachable.",
                 trailStopAfterSArg);
    cmd.AddValue("vizTablePeriodS",
                 "Seconds between per-node table snapshots for the visualiser. 0 = off. "
                 "Writes nbrtable_* (what a node has PERSONALLY HEARD, one-hop, drives "
                 "the poll set) and loctable_* (what it KNOWS, any source, any hop count, "
                 "nothing polled from it) as two files, because they are two different "
                 "things. Requires --vizEvents. Sampled rather than per-cycle: ten nodes "
                 "at every cycle is ~80k rows and an inspector cannot show that anyway.",
                 vizTablePeriodSArg);
    cmd.AddValue("digestVizOutputs",
                 "Digest the visualiser's outputs in run_id.txt (TECH_DEBT M.4). FALSE by "
                 "default: events_*, nodes_*, nbrtable_*, loctable_* and deployment.json are "
                 "written under --vizEvents but the sidecar carries no SHA-256 for them, though "
                 "house rule 4.5 asks for one per output file, and events_* is the only on-disk "
                 "evidence of some mechanisms (L.1.5's eager push, M.9). TRUE: all five are "
                 "added to the digest. Nothing else changes. Needs --vizEvents.",
                 digestVizOutputsArg);
    cmd.AddValue("reportRecedingPushes",
                 "Count L.1.5's RECEDING eager push (TECH_DEBT M.9). FALSE by default: under "
                 "--mobilityFromRange a node whose mean measured range is growing skips the "
                 "digest and pushes its entry once as a TRIGGERED_UPDATE, and nothing counts it; "
                 "its only trace is rows of events_*. TRUE: one M.9 line with the handshakes that "
                 "took the RECEDING branch and the pushes transmitted, summed over every radio. "
                 "A report only; nothing else changes. Needs --mobilityFromRange.",
                 spec.reportRecedingPushes);
    cmd.AddValue("vizEvents",
                 "Emit a per-frame event log (events_<arm>_seed<N>.csv) and the "
                 "deployment geometry (deployment.json), for the trace-driven "
                 "visualiser. FALSE by default. These are OUTPUTS ONLY -- the channel "
                 "writes to the stream and never reads it, and nothing in the "
                 "simulation reads either file back, so this cannot change a result. "
                 "It records the same events the aggregate counters already count, "
                 "itemised, so an animation can show them one at a time. Costs disk "
                 "and a little I/O; it is off by default because a 3600 s six-arm run "
                 "writes a lot of frames.",
                 vizEventsArg);
    cmd.AddValue("uwbCoverageIncludesFinders",
                 "Count finders when labelling ground-truth UWB coverage. FALSE by "
                 "default. VisibleCount() reads the STATIC deployment, and under the "
                 "tri-band correction tier 3 comes from finders, which are not in it -- "
                 "so visUwb is structurally 0, shouldUwb structurally false, every UWB "
                 "escalation is scored a false positive and trig_f1 is 0.000 by "
                 "construction. Measured: 0 tp / 21 fp / 0 fn / 459 tn on a run with "
                 "three UWB neighbours on 12 cycles. Changes a reported column and no "
                 "decision -- shouldUwb feeds the confusion matrix and nothing else.",
                 uwbCoverageIncludesFindersArg);
    cmd.AddValue("finderDispersal",
                 "Spread finders over the casualty's REPORTED position uncertainty "
                 "instead of stacking them on its centroid, in sigmas. 0.0 by default, "
                 "which is the pre-existing behaviour: every finder walks to the same "
                 "coordinate. That is the worst allocation precisely when it matters -- "
                 "the reported position is tens of metres wrong and UWB reaches ~13.5 m, "
                 "so a stack of finders at the centroid is a stack all equally out of "
                 "range and tier 3 is never reached. The search region comes from varX "
                 "and varY in the entry the finder already holds, so nothing is assumed "
                 "that a searcher could not have been told. Axis-aligned: covXY is "
                 "carried but not used, so a rotated ellipse is covered less efficiently "
                 "rather than wrongly.",
                 finderDispersalArg);
    cmd.AddValue("processNoiseStdPos",
                 "EKF process-noise std on position, metres. Default 1.0, the value "
                 "ca-tbl-policy.cc already set and the scenario never touched. "
                 "EXPOSED FOR A REASON: K2 measured 465 suspects in 1440 cycles, "
                 "32.3%, against the 1% chi-squared 0.99 = 9.21 gives on a "
                 "consistent filter, and ruled out corroboration and the forced "
                 "escalation. With 2.4 GHz corrected to r80 156 m the node coasts "
                 "far longer between fixes, and a Q tuned when coasting was short "
                 "leaves the covariance under-grown -- which is a Mahalanobis "
                 "distance that over-fires. Sweep this to test that; do NOT move "
                 "the 9.21 threshold, which is a distributional constant.",
                 processNoiseStdPosArg);
    cmd.AddValue("processNoiseStdVel",
                 "EKF process-noise std on velocity, m/s. Default 0.5, as above.",
                 processNoiseStdVelArg);
    cmd.AddValue("gatewayEdgeServer",
                 "Put the estimator on the GATEWAYS, where the brief puts it (ch10 "
                 "sect 10.1: estimation runs on the Anchor/Gateway Node, not the Field "
                 "Node). TRUE by default since 2026-09-29, for three reasons. It is the "
                 "specified design, and node-side estimation was the one CONTRADICTION "
                 "row in MASTER_PLAN 2.1. It is the exchange tier 1's bill always "
                 "described: the node broadcasts once, the gateways measure and report "
                 "over the BACKHAUL, the network computes the fix, and the node wakes "
                 "for ONE downlink per fix instead of a reply window per gateway. And at "
                 "defaults it moves energy and nothing else: in B3 (report/"
                 "B3_REGEN_RESULTS.md C6, 20 seeds x 3600 s) all seven accuracy columns "
                 "were byte-identical to node-side estimation on 20/20 seeds, and totalJ "
                 "fell 42.5% on static_868 and 32.9% on fixed_threshold and ca_tbl_full, "
                 "20/20 each. Those two hybrid figures were measured with the downlink "
                 "on the ACTIVE tier; since 2026-09-30 it is billed on the tier the fix "
                 "came from (--edgeDownlinkTier), which makes them smaller savings by an "
                 "amount not yet measured on 20 seeds. It is NOT a saving everywhere. "
                 "Tiers 2 and 3 range by "
                 "two-way time-of-flight because the endpoints share no clock, so that "
                 "exchange cannot move without the synchronisation TECH_DEBT J.2 records "
                 "as absent; there this only ADDS the downlink, and static_2p4 costs 0.2% "
                 "MORE on 20/20 seeds. static_uwb never produces a fix, so never takes "
                 "the path. Nor is it complete: getting a tier-2 or tier-3 range from the "
                 "exchange that measured it to the gateway's estimator is not billed "
                 "(TECH_DEBT L.2.3), which flatters this setting on those tiers and not "
                 "on tier 1. NO CLOCK SYNC IS INVOLVED: this is RSSI multilateration "
                 "performed network-side, which is what LoRaWAN geolocation does, and "
                 "the estimator itself does not change -- only where it runs, and so what "
                 "the node is billed. Energy is an INPUT in two places -- T_batt, which "
                 "--uwbArbitration scores between competing requests, and "
                 "--batteryDepleteJ, which stops the radio -- and there this setting can "
                 "move more than energy (with --uwbArbitration on and one requester it "
                 "did not). FALSE is node-side estimation, with the tier-1 replies billed "
                 "per --tier1ChargeReplies; it reproduces every result produced before "
                 "this default changed, results/regen27_* included, so pass it to replay "
                 "any earlier sidecar.",
                 gatewayEdgeServerArg);
    cmd.AddValue("edgeDownlinkTier",
                 "Which radio the gateways' downlink is billed on, fix or active. READ "
                 "ONLY WITH --gatewayEdgeServer=true. 'fix' by default since 2026-09-30 "
                 "(TECH_DEBT M.10, owner decision): the downlink goes on the radio whose "
                 "measurements produced the fix. That differs from the active tier only "
                 "on a hybrid arm's floor-solved cycle, where the escalated tier returned "
                 "fewer than three ranges and the 868 MHz floor supplied the fix. There "
                 "the answer comes back on 868 MHz, the link that has just reached three "
                 "gateways, and not on the escalated link that has just failed to. At the "
                 "defaults that is 0.9042 mJ per floor-solved fix instead of the 0.2005 mJ "
                 "of a 2.4 GHz frame, or the 0.3224 mJ of a UWB one (ca-tbl-energy-"
                 "model.cc: a 12-byte payload at SF7, 868 MHz at 125 kHz, 2.4 GHz at "
                 "800 kHz, one guarded receive window). Only energy moves at defaults, "
                 "and only on those cycles; --batteryDepleteJ and --uwbArbitration read "
                 "energy, so under them more can move. On every one of those cycles 868 "
                 "MHz has already transmitted this cycle's broadcast, so the bill charges "
                 "no radio that stayed silent (CLAUDE.md sect 6). Putting the answer on "
                 "the fix's radio is our choice, not cited. 'active' is the bill before "
                 "2026-09-30: the downlink on the policy's active tier even when the floor "
                 "supplied the fix, which charged the link that had just failed. With it "
                 "this build reproduces the build before the change byte-for-byte. B3 "
                 "C6's hybrid figures (results/regen27_edge/edge, -32.9%) were measured "
                 "under it, so add --edgeDownlinkTier=active to replay them. Any other "
                 "value is FATAL.",
                 edgeDownlinkTierSpec);
    cmd.AddValue("tier1ChargeReplies",
                 "Charge tier 1 for the downlink that carries its ranges back. READ ONLY "
                 "WITH --gatewayEdgeServer=false: since 2026-09-29 the estimator is on "
                 "the gateways by default, the ranges go over the backhaul, and there "
                 "are no replies to charge. TRUE by default since 2026-09-08; pass false "
                 "(with --gatewayEdgeServer=false) to reproduce anything measured "
                 "before that. What follows is the node-side case. Tier 1 is billed as "
                 "ONE BROADCAST AND NOTHING ELSE -- a TDoA shape, and the right cost "
                 "when the NETWORK localises. But CaTblPolicy runs on the FIELD NODE and "
                 "consumes a range per anchor per cycle, and nothing charges for those "
                 "ranges arriving: there is no per-beacon RX accounting and AccountListen "
                 "fires only when the neighbour table is empty. So tier 1's structural "
                 "energy advantage, which every accuracy-per-joule figure rests on, is "
                 "measured against an exchange one message short of closing. This "
                 "measures the gap; it did not decide which shape is right, because that "
                 "was the CONTRADICTION row in MASTER_PLAN 2.1 -- the spec puts the "
                 "estimator on the gateway and the code put it on the tag. "
                 "--gatewayEdgeServer's default now follows the spec.",
                 tier1ChargeRepliesArg);
    cmd.AddValue("bidirectionalDigest",
                 "Let the lost node ASK as well as answer, so the anti-entropy exchange "
                 "runs both ways. FALSE by default. Only searchers used to ask, which "
                 "is why 4 of 4 finders learn where the casualty is and the casualty "
                 "learns nothing about them -- and why --finderPseudoAnchor and the hop "
                 "provenance term were both built and both inert: neither has an entry "
                 "to act on. The digest goes out on the ACTIVE tier, because sending on "
                 "868 while up on UWB would charge a tier outside the cycle's "
                 "transmission and break the sect 6 invariant, and because each tier "
                 "owns its own table, so the answer must land in the one the ranging "
                 "path reads.",
                 bidirectionalDigestArg);
    cmd.AddValue("finderPosSigma",
                 "Metres of GNSS position error a finder carries, as a per-finder "
                 "constant offset. 0.0 by default, which is the PRE-EXISTING and "
                 "flattering assumption: a finder reports its position exactly. The "
                 "self-consistent setting is 2.0 -- the variance a finder entry has "
                 "always claimed is 4.0 m^2, while the position it carried was true, "
                 "so the claim and the number disagreed. A searcher is not GPS-denied "
                 "(that is the casualty's problem) but it is not surveyed "
                 "infrastructure either. Constant rather than per-cycle noise because "
                 "GNSS error over a 600 s walk is dominated by slowly varying terms; "
                 "white noise would average out and understate it. Drawn from its own "
                 "stream, so it displaces nothing else.",
                 finderPosSigmaMArg);
    cmd.AddValue("finderPseudoAnchor",
                 "Treat a finder as a PSEUDO-ANCHOR rather than a perfectly known one: "
                 "the filter gets the position the finder REPORTS and a variance "
                 "inflated by that position's own uncertainty along the range "
                 "direction. FALSE by default. With it off, a finder's TRUE position is "
                 "handed to the filter, which credits it with knowledge no node has and "
                 "flatters every result that depends on finders -- which, since finders "
                 "are what make tier 3 reachable at all, is most of them. The physics is "
                 "unaffected either way: the radio link is physical and does not care "
                 "what anyone believes.",
                 finderPseudoAnchorArg);
    cmd.AddValue("hopProvenanceForm",
                 "How hop provenance enters the confidence score. 0 (default) is the "
                 "legacy linear subtraction, kept so prior results reproduce and pinned "
                 "inert by hopWeight=0. 1 is the bounded multiplicative form the relayed-"
                 "ranging error model specifies: pcs /= (1 + hopPenalty * relayed "
                 "variance share). The legacy shape is UNBOUNDED BELOW, penalises a "
                 "COUNT rather than an uncertainty, and encodes nothing about shared "
                 "error -- all three of which the error model records as wrong.",
                 hopProvenanceFormArg);
    cmd.AddValue("hopPenalty",
                 "Strength of the bounded provenance penalty. 0 (default) disables it "
                 "whatever the form is set to. This is the ONE quantity in the "
                 "confidence score that the filter covariance cannot already contain: "
                 "the trace is the RESULT of a fix, provenance is HOW it was obtained, "
                 "and two fixes with the same trace and different provenance are equally "
                 "certain by the filter own account and not equally trustworthy.",
                 hopPenaltyArg);
    cmd.AddValue("imuCrossCheck",
                 "Refuse an escalation when the filter's POSITION disagrees with the "
                 "step-and-heading engine's dead-reckoned position by more than a "
                 "tolerance that grows with time since the last trusted fix. It compares "
                 "absolute positions, not displacements (this text said displacement "
                 "until 2026-09-30), so an SHS that has diverged (TECH_DEBT K.9) refuses "
                 "almost everything. --imuWitnessSource=model replaces the SHS track with "
                 "the synthetic IMU's, checked against the raw radio fix (DC-9). "
                 "FALSE by default so the check "
                 "can be run on against off, which is the comparison the project brief "
                 "asks for and one you cannot make if the mechanism is always on. "
                 "SUPPRESS-ONLY: it can withhold an escalation, never cause one, so it "
                 "cannot manufacture the behaviour it is policing.",
                 imuCrossCheckArg);
    cmd.AddValue("imuDriftMps",
                 "How fast the step-and-heading estimate is allowed to drift away "
                 "from the filter while running unaided, metres per second. Default "
                 "0.15. This sets the SHAPE of the consistency tolerance and it is the "
                 "SHS error model rather than a strapdown one: drift accumulates with "
                 "TIME SINCE THE LAST TRUSTED FIX, through heading error and step-length "
                 "miscalibration, not with distance walked. OURS, not cited.",
                 imuDriftMpsArg);
    cmd.AddValue("imuConsistencyK",
                 "How many filter sigmas of disagreement are tolerated before the "
                 "inertial witness refuses an escalation. Default 3. OURS.",
                 imuConsistencyKArg);
    cmd.AddValue("imuSigmaFloorM",
                 "Floor on the sigma term of that tolerance, metres, so a confident "
                 "filter cannot be tripped by a disagreement smaller than the inertial "
                 "system could ever resolve. Default 5.",
                 imuSigmaFloorMArg);
    cmd.AddValue("corroborationCheck",
                 "Refuse an escalation when too few neighbours corroborate this node's "
                 "estimate. For every node ranged this cycle whose own reported position "
                 "is held in the table, the measured range is compared against the "
                 "distance from this node's estimate to where that node SAYS it is. "
                 "FALSE by default. This is the independent witness: it uses residuals "
                 "and other nodes' reports, and NOT the filter's covariance, which is "
                 "the quantity the confidence signal is made of.",
                 corroborationCheckArg);
    cmd.AddValue("corroborationK",
                 "How many combined sigmas of range residual still count as agreement. "
                 "Default 3. OURS.",
                 corroborationKArg);
    cmd.AddValue("corroborationRangeSigmaM",
                 "Assumed ranging sigma in the corroboration tolerance, metres. Default "
                 "30, which is tier-1 scale -- the loosest tier, so the test is "
                 "deliberately reluctant to call a disagreement.",
                 corroborationRangeSigmaMArg);
    cmd.AddValue("finderDigestPeriodS",
                 "How often each finder broadcasts a digest asking for table entries, "
                 "seconds. Default 30. This is what makes the casualty's position reach "
                 "a searcher AT ALL: the responder side of the anti-entropy exchange has "
                 "always replied to a digest with its own entry, but nothing ever asked, "
                 "so nothing propagated. Set 0 to disable and watch the finders stand "
                 "still, which is what they should do when nobody has told them anything. "
                 "Requires --sendHandshakeFrames, since that is what gates the responder.",
                 finderDigestPeriodSArg);
    cmd.AddValue("finderSpeed",
                 "Walking speed of the SEARCHERS, m/s. Negative (default) means follow "
                 "--speed, which is what every run before 2026-09-08 did. THEY NEEDED "
                 "SEPARATING: --speed sets one variable that built both the subject's "
                 "walker and every finder's, so --speed=0 -- how a still casualty is "
                 "specified -- also pinned all four searchers to the spot, and every "
                 "still cell measured a paralysed search party rather than a search.",
                 finderSpeedMpsArg);
    cmd.AddValue("searchLaunchOnLoss",
                 "Seconds of NO GATEWAY CONTACT before the party deploys. 0 = off. A party "
                 "is not dispatched because someone stepped off a path -- that is invisible "
                 "to the network -- but because contact was lost and stayed lost. Latched: "
                 "a momentary re-contact does not recall searchers already walking.",
                 searchLaunchOnLossSArg);
    cmd.AddValue("searchLaunchAtS",
                 "Deploy the party at this time, as an OVERDUE REPORT. 0 = off. This "
                 "is how a real search starts: someone did not arrive. It exists because "
                 "neither telemetry trigger separates on the deployments tested -- "
                 "A.10 shows 868 reaches past 4.5 km so contact loss never occurs, and "
                 "on the arc the posterior sigma sits in an 80-190 m band all run with "
                 "no sustained degradation (on-trail p90 133, p99 148; sustained 60 s "
                 "crossings: 150 m at t=720, 175 m and above never). A bound inside "
                 "that band is an arbitrary number, not a threshold, and what sigma "
                 "measures there is tier-1 RSSI accuracy rather than geometry. The "
                 "party still holds at camp until dispatched, which is the behaviour "
                 "--searchLaunchSigmaM was a proxy for.",
                 searchLaunchAtSArg);
    cmd.AddValue("searchLaunchSigmaM",
                 "Radial position sigma (m) above which the network is treated as having "
                 "LOST THE SUBJECT even though it can still hear them; sustained for "
                 "--searchLaunchOnLoss seconds, it deploys the party. 0 = off. Needed "
                 "because contact loss is unreachable here: measured, the walker still "
                 "heard a ridge gateway on 88% of cycles at 4.5 km, so a silence-only "
                 "alarm never fires (BEHAVIOUR_ANOMALIES A.10). The bound is on the "
                 "system's own posterior radial sigma sqrt(traceSigma); a cycle with no "
                 "fix counts as lost outright. Measured on the ridge run: p90 58 m on "
                 "the trail against p50 327 m off it, so 100 is a bound the trail "
                 "clears and the deviation does not.",
                 searchLaunchSigmaMArg);
    cmd.AddValue("finderStaleSteerBound",
                 "Seconds past which a held entry is too old to STEER a searcher on. "
                 "0 = off, the pre-existing behaviour where an entry received once steers "
                 "forever. An entry persists after arrival, so a finder that heard the "
                 "subject once converges on that position for the rest of the run -- "
                 "measured at 377 s old and 1029 m wrong. Past the bound the entry may "
                 "still be REPORTED, which is the proxy responder's call; it just must not "
                 "steer, because walking to a six-minute-old fix does worse than sweeping "
                 "ground nobody has looked at.",
                 finderStaleSteerBoundSArg);
    cmd.AddValue("finderMode",
                 "2 = SEARCH THEN CONVERGE. Mode 1 converges on the casualty's "
                 "reported position and, with no entry, deliberately HOLDS POSITION "
                 "-- a good diagnostic and not a search. Measured consequence: a "
                 "stationary subject 11.6 km from the camp is never found, because "
                 "it is out of range so it can tell nobody where it is, the finders "
                 "have no entry so they do not move, and nothing comes into range. "
                 "Mode 2 sweeps an expanding spiral from the LAST KNOWN POSITION "
                 "until an entry is more informative than the sweep, then converges. "
                 "Radius is capped at how far the subject could have walked. Needs "
                 "--baseCampFrac. "
                 "How finders move. 1 (DEFAULT) = CONVERGE on the system's current "
                 "position ESTIMATE of the lost node, standing off 8 m once there. Not "
                 "the true position -- nobody knows that -- so a bad fix sends searchers "
                 "to the wrong place, and the cost of poor localisation shows up as "
                 "distance walked rather than only as a number. 0 = tour the facility "
                 "sites like any other field node, which is what a finder does before it "
                 "has been given anywhere to go. NOTE this is a MODELLING CHOICE not "
                 "specified by the project brief; see docs/DESIGN_DECISIONS.md.",
                 finderModeArg);
    cmd.AddValue("nFinders",
                 "Number of FINDERS: moving nodes carrying all three radios, which the "
                 "lost node discovers and ranges against exactly as it does a fixed "
                 "gateway. Default 0. This is the mechanism the architecture turns on: "
                 "UWB reaches tens of metres, so a FIXED gateway can offer tier 3 only "
                 "to something already beside it, and escalation to UWB becomes a "
                 "statement about a searcher having arrived rather than about the "
                 "casualty having wandered somewhere useful. Finders take the 2.5% WPC "
                 "Table-II field cap, NOT the 10% access-point cap -- a searcher is a "
                 "person carrying a radio, not installed infrastructure.",
                 nFindersArg);
    cmd.AddValue("nGateways",
                 "Place exactly N gateways evenly on a ring instead of a gwGrid x gwGrid "
                 "square array. 0 (default) keeps the grid. Exists because the project "
                 "brief asks for THREE gateways and three is not a square number, so the "
                 "grid cannot express the deployment at all -- its nearest options are "
                 "four and nine. At N=3 the ring is an equilateral triangle, which is "
                 "the minimum geometry for a 2-D trilateration fix and the best "
                 "conditioning three points can give the interior.",
                 nGatewaysArg);
    cmd.AddValue("gwRingFrac",
                 "Ring radius as a fraction of boxHalf when --nGateways is used. Default "
                 "0.6. A fraction rather than an absolute so the deployment scales with "
                 "the area the way the grid does, and the two stay comparable across "
                 "box sizes.",
                 gwRingFracArg);
    cmd.AddValue("topology",
                 "0 (DEFAULT) = TRI-BAND GATEWAYS, the correct model: every gateway is "
                 "one fixed node carrying all three radios -- 868 MHz, 2.4 GHz and UWB "
                 "-- so the tier is a choice of which radio to use, not of whom to talk "
                 "to. 1 = the legacy three-population model, in which tier 1, tier 2 and "
                 "tier 3 had SEPARATE anchors in different places. Legacy is retained "
                 "only so results produced before 2026-09-07 remain reproducible; it "
                 "does not describe the system. NOTE that 0 and 1 are different "
                 "deployments, so they do not share a baseline and no null control "
                 "relates them -- switching this is a change of scenario, not of "
                 "configuration.",
                 topologyArg);
    cmd.AddValue("sitingMode",
                 "Where tier-3 anchors go. 0 (default) CO-LOCATES them with tier 2 at "
                 "the four facility sites, which is the siting every existing result "
                 "was produced under and is deliberate: if the two refinement tiers "
                 "covered different ground, an escalation failure could be a siting "
                 "artefact rather than a decision error. 1 is COMPLEMENTARY siting -- "
                 "the facility square rotated 45 degrees, so tier 3 sits at the SAME "
                 "radius from the centre but in the GAPS between the tier-2 clusters. "
                 "Same radius is the point: moving tier 3 outward would confound "
                 "\"different place\" with \"further out\" and the comparison would "
                 "stop isolating siting. Tier 2 does not move in either mode, and "
                 "neither do the facilities themselves, because the walker dwells "
                 "there and moving them would change the mobility as well.",
                 sitingModeArg);
    cmd.AddValue("relayedRanging",
                 "Whether peer tags run their own filter, so that a relay has a "
                 "position estimate and a covariance to report. FALSE (default) "
                 "reproduces every existing result byte-for-byte, because with it "
                 "off not one additional engine evaluation is made and the shared "
                 "random stream is untouched. A relay is a PSEUDO-ANCHOR: its range "
                 "is used against its own estimate with the variance inflated by "
                 "u^T P_B u, so both halves need a filter on the relay. A peer "
                 "without one has nothing to put on the wire, which is what blocked "
                 "C5 -- not the link budget. The relay is modelled as a FIXED-TIER "
                 "measured node: it ranges, filters and is charged, but does not "
                 "escalate, because a relay that escalated would earn its P_B on a "
                 "different tier between one report and the next and the error model "
                 "does not cover that. With two or more tags, TRUE also turns on "
                 "--relaySlots by default since 2026-09-30, so the relays' exchanges get "
                 "instants of their own instead of colliding at the cycle's one instant; "
                 "--relaySlots=false restores the collision every relayed run before that "
                 "date was measured under.",
                 relayedRangingArg);
    cmd.AddValue("maxRelayDepth",
                 "Hard ceiling on how many hops a relayed measurement may have crossed. "
                 "1 (default) is the single-hop behaviour every result before 2026-09-08 "
                 "was measured under, and it made the mesh single-hop by construction: "
                 "a relay three kilometres out was invisible even with another node "
                 "standing between. Above 1 the UNCERTAINTY BUDGET is normally what "
                 "binds, not this -- see --relayVarBudget.",
                 maxRelayDepthArg);
    cmd.AddValue("relayVarBudget",
                 "Accumulated positional variance a relayed measurement may carry, "
                 "m^2. 0 (default) disables the budget and leaves depth bounded only "
                 "by --maxRelayDepth. This is the ADAPTIVE half: what decides whether "
                 "a relayed range is worth fusing is not how many hops it crossed but "
                 "how much uncertainty it picked up doing so, and a relay carries what "
                 "it inherited in its own covariance -- so the test needs no knowledge "
                 "of the chain length. As a scale, tier-2 r80 is 156 m, so a budget "
                 "of 156^2 admits a chain no more uncertain than one tier-2 hop.",
                 relayVarBudgetM2Arg);
    cmd.AddValue("relayChain",
                 "Let a relay that ranged fewer than three anchors this cycle range "
                 "against a relay that already holds a fix this cycle, so that a "
                 "relay's own position can itself be relayed and depth 2 exists "
                 "(Network sect 5, TECH_DEBT L.1.2). FALSE (default) is every result so "
                 "far: a relay ranges surveyed anchors and nothing else, so every relay "
                 "sits at depth 0, every relayed range is depth 1, and --maxRelayDepth "
                 "and --relayVarBudget can never bind at depth 2. It stays off by "
                 "default because each relay-to-relay exchange is one more engine "
                 "evaluation, and every added evaluation displaces the shared random "
                 "stream for every link in the run (rule 8), so no magnitude is "
                 "comparable across the two settings; off, nothing is constructed and "
                 "not one evaluation is added. On, the short relay steps after the "
                 "fix-holders, each range to one is admitted under A1-A5 at the "
                 "holder's depth + 1 with the holder's covariance composed with what it "
                 "inherited, and ONLY IF THE STEP FUSES THEM (three or more "
                 "measurements) does the relay take that depth -- below three the "
                 "filter coasts and uses none of them -- so a range from the measured "
                 "tag through it carries depth 2. The relays are the filtered peer "
                 "field nodes (ids 2+); a finder carries GNSS and is an anchor, never a "
                 "relay. The exchange rides --relayTier and is charged to the short "
                 "relay's own account, inside the per-cycle loop, and is not made at "
                 "all when anchors plus holders cannot reach three. KNOWN LIMIT: without "
                 "--relaySlots every relay exchange shares the cycle's one instant and "
                 "the engines treat them as simultaneous, which in every regime probed "
                 "has kept any chain from fusing -- see the RelayChain comment. Since "
                 "2026-09-30 --relaySlots is on by default in every relayed run, so this "
                 "limit binds only under an explicit --relaySlots=false; gates G17 and "
                 "G17b run at the default. REQUIRES "
                 "--relayedRanging=true and --nTags >= 3 (two relays): with fewer, no "
                 "relay has a filter or no relay has another to lean on, every count "
                 "would be a structural zero, and that combination is refused rather "
                 "than run. Set --maxRelayDepth=2 for depth 2 to be admissible at all.",
                 relayChainArg);
    cmd.AddValue("relaySlots",
                 "Give the relays' ranging round instants of its own inside each cycle "
                 "(C5_INSTANT). 'auto' (DEFAULT since 2026-09-30, the owner's decision on "
                 "TECH_DEBT M.1) turns it ON in every run that has a relay to schedule -- "
                 "--relayedRanging=true and --nTags >= 2 -- and leaves it off in every "
                 "other run, where nothing is constructed and the output is byte-identical "
                 "to the build before. 'true' asks for it and is refused where there is no "
                 "relay; 'false' turns it off, which is the old default, and reproduces the "
                 "build before the change byte-for-byte in relayed runs (13 configurations "
                 "checked, TECH_DEBT M.1). A bare --relaySlots means true, as it did when "
                 "this was a boolean. WHY ON: off, every exchange a cycle makes directly "
                 "is stamped at the cycle's one instant, and the engines' interference "
                 "registry decides overlap on time and transmitter alone, at the "
                 "interferer's own link power, so each relay's polls collide with the "
                 "other relays' and the measured tag's. That is an artefact of calling the "
                 "engines at one timestamp, not a channel effect the model means to have: "
                 "the code makes those exchanges one after another, and one stamp turns "
                 "the order of the calls into mutual interference. With only "
                 "--relayedRanging on, the measured tag's 3+-anchor cycles fall from "
                 "107/120 to 2/120 (static_2p4, seed 7, 300 s, --boxHalf=200, four tags), "
                 "relays after the first never hold a fix, and no --relayChain exchange "
                 "ever delivers; with the round the same run gives 103/120. HOW: relay pt "
                 "polls at now-(K-pt+1)*50 ms and the j-th "
                 "relay-to-relay exchange runs at now-(C-j)*50 ms, with C = (nTags-1)"
                 "(nTags-2) under --relayChain, else 0, and K = nTags-1+C -- all BEFORE "
                 "the measured tag, which still ranges at the instant and reads every "
                 "relay's estimate of this cycle, so the order is the order the code "
                 "already assumes and the instants rise as the calls are made. 50 ms is "
                 "L.1.1's frame slot, longer than every tier's nominal airtime at SF7; "
                 "slot, order and layout are OURS, the spec defines no schedule. Nothing "
                 "is added, removed or re-ordered and every charge is unchanged -- to the "
                 "relay that sent it, on --relayTier, inside the per-cycle loop -- with "
                 "its duty governor armed from its slot. WHAT IT GIVES UP: the round sits "
                 "in the slice the event layer has already run, so a MAC frame completing "
                 "in its last K*50 ms never meets it (RelayRound comment), and no relayed "
                 "magnitude compares across the two settings (rule 8). EVERY "
                 "--relayedRanging FIGURE RECORDED BEFORE 2026-09-30 WITHOUT "
                 "--relaySlots=true WAS MEASURED UNDER THE COLLISION, which "
                 "--relaySlots=false restores. A round that does not fit inside --dt -- at "
                 "the 2.5 s default, --nTags >= 9 under --relayChain or >= 51 without -- "
                 "is refused "
                 "under 'auto' as under 'true', rather than run under the collision "
                 "unannounced; --relaySlots=false runs it the old way.",
                 // A callback item and not a string one, so that a bare --relaySlots
                 // still parses: ns-3 refuses an empty value for a string, and the
                 // resolution after Parse() reads empty as true.
                 Callback<bool, const std::string&>([&relaySlotsSpec](const std::string& v) {
                     relaySlotsSpec = v;
                     return true;
                 }),
                 relaySlotsSpec);
    cmd.AddValue("fixRouting",
                 "Route each relay's position fix to the gateway backbone hop by hop, so a "
                 "relay that cannot reach a gateway on --relayTier hands its ENTRY_PUSH to "
                 "one that can (Network sect 2-3, TECH_DEBT L.1.1). Every relay keeps a "
                 "routing table built from links it has HEARD -- gateway beacons its own MAC "
                 "delivered, and a once-a-cycle route advertisement from every relay holding "
                 "a route -- and each hop chooses once, on ETX, while the confidence metric "
                 "(alpha*NLOS + beta*RSSI variance + gamma*jitter, slack 1; the weights are "
                 "OURS, not cited) is scored in the same call: Divergences() counts the hops "
                 "where the two disagreed about a frame that was actually routed. FALSE "
                 "(default) is every result so far, and it stays the default because every "
                 "advertisement and every hop is one more engine evaluation, which displaces "
                 "the shared random stream for every link in the run (rule 8), so no "
                 "magnitude is comparable across the two settings; off, nothing is "
                 "constructed, no MAC callback is bound and not one evaluation is added. The "
                 "routers are the filtered relays, ids 2+; the measured tag does not route, "
                 "because its tier moves between cycles and a frame on a tier it did not use "
                 "would break sect 6. Frames ride --relayTier inside the per-cycle loop, each "
                 "in its own slot after the cycle's ranging, charged to the relay that sends "
                 "(TX) or receives to forward (RX) and to no other tier or account. Every hop "
                 "is written to fix_routing_<arm>_seed<N>.csv. REQUIRES "
                 "--relayedRanging=true and --nTags >= 3 (two relays): with fewer no relay "
                 "has an account to charge or a neighbour to route through, and every count "
                 "would be a structural zero, so that combination is refused rather than run. "
                 "At --relayTier=0 the relay's own ranging broadcast spends its 2.5% tier-1 "
                 "budget every cycle, so nearly every routed frame is refused by the governor "
                 "and counted as such.",
                 fixRoutingArg);
    cmd.AddValue("fixRoutingConfidence",
                 "Make the confidence metric the ACTIVE selector for position-fix traffic "
                 "under --fixRouting, instead of only scoring it beside ETX. FALSE (default) "
                 "routes on ETX, the borrowed baseline the spec says to build and validate "
                 "first, so that the contributed metric is measured against something rather "
                 "than asserted over. Only ENTRY_PUSH is routed this way -- it carries a fix "
                 "(CaTblCarriesPositionFix) -- and route advertisements stay one-hop "
                 "discovery gossip, which is the traffic split the claim rests on. A relay "
                 "advertises the route its own selector would take, so under this flag the "
                 "cost of handing a frame to a neighbour is the confidence cost of where that "
                 "neighbour will send it. REQUIRES --fixRouting=true.",
                 fixRoutingConfidenceArg);
    cmd.AddValue("tier1Tdoa",
                 "Localise tier 1 the way the spec says (Physical sect 1.1): RSSI path loss "
                 "COMBINED with TDoA wherever three or more gateways hear the same "
                 "broadcast (TECH_DEBT J.2, L.2.1). Each surveyed gateway that decodes the "
                 "node's tier-1 broadcast stamps its arrival on a GPS-PPS-disciplined clock. "
                 "The stamp error is the PPS residual (--tdoaPpsSigmaNs), plus the one-way "
                 "ToA Cramer-Rao bound at that link's SNR, SF and bandwidth, plus an NLOS "
                 "excess delay scaled by the link's own delay spread. The differences are "
                 "solved by Chan's closed form (>= 4 gateways) refined by Foy's Taylor "
                 "iteration, which the cycle's RSSI fix seeds whenever Chan fails; three "
                 "gateways are solved exactly, and the cycle's RSSI ranges choose between "
                 "the two roots or the solve is refused. Collinear, singular and runaway "
                 "solutions are refused too. The fix goes to the EKF with "
                 "its full covariance as R, fused straight after that cycle's RSSI ranges. "
                 "FALSE (default) is every result so far, and it stays the default for "
                 "three reasons. On, the tier-1 observable changes, and with it every "
                 "tier-1 accuracy figure, the A4 sigma calibration and the escalation "
                 "decisions that read them (J.2: it invalidates every tier-1 result). The "
                 "PPS and NLOS terms are models, not measurements. And no result has yet "
                 "been registered on it. It adds no frame and no charge: the stamps ride "
                 "the broadcast and the replies the energy model already bills, so billing "
                 "and observable finally agree. For that reason it is REFUSED with "
                 "--tier1ChargeReplies=false unless --gatewayEdgeServer=true, since that "
                 "configuration would put stamps in the node's filter that no billed frame "
                 "carried. Its draws come from two dedicated streams set at construction, "
                 "so no link's draw moves; off, nothing is constructed. Every tier-1 fix "
                 "cycle is written to tdoa_<arm>_seed<N>.csv.",
                 tier1TdoaArg);
    cmd.AddValue("tdoaPpsSigmaNs",
                 "1-sigma GPS-PPS timing residual of a gateway's arrival stamp under "
                 "--tier1Tdoa, in ns. Default 20: the u-blox NEO/LEA-M8T timing-module data "
                 "sheet (UBX-15025193 R10, Table 1) gives time-pulse accuracy <= 20 ns "
                 "1-sigma in clear sky, and the limit is taken as the sigma. It is a cited "
                 "PRODUCT figure, not a measurement of this deployment. Every nanosecond "
                 "is 0.3 m of range difference, and J.2's 'the whole difficulty' is this "
                 "term, so sweep it rather than trust one value: the same table gives "
                 "<= 500 ns indoors, and a gateway under canopy sits between the two. "
                 "Drawn independently per broadcast; slow per-gateway wander is not "
                 "modelled. Read only under --tier1Tdoa.",
                 tdoaPpsSigmaNsArg);
    cmd.AddValue("nlosClassifier",
                 "Give every link the node ranges the tier-specific NLOS_score of the spec "
                 "(Physical sect 1.3, 2.5; TECH_DEBT L.2.2) and score it against the channel "
                 "model's own LOS/NLOS draw. UWB: a CIR is SYNTHESISED from the link's "
                 "modelled state -- IEEE 802.15.4a multipath (Molisch et al.; CM5/CM6 "
                 "outdoor, CM3/CM4 under --envFacility=6), a direct ray set by the engine's "
                 "Rician K and obstruction loss, accumulator noise at the link's SNR -- and "
                 "the leading-edge test gives 1 - A_leading_edge/A_peak. 2.4 GHz: the loss "
                 "the RSSI shows beyond what the nominal path-loss model predicts at the ToF "
                 "distance. 868 MHz: the RSSI/SNR variability over the link's last 8 "
                 "readings. Both are scored in robust sigmas (median, 1.4826*MAD) of the "
                 "node's own last 200 values on that tier, because the configured noise "
                 "was measured wrong by 4x on 868 and biased by 11 dB on 2.4. Writes "
                 "nlos_links_<arm>_seed<N>.csv and nlos_summary_<arm>_seed<N>.csv (a "
                 "confusion matrix per tier, lost links by truth, and the PCS term's "
                 "behaviour). CIRCULAR BY CONSTRUCTION, and that is why it is off: each "
                 "classifier can only rediscover the NLOS the channel model created, so a "
                 "confusion matrix here measures the synthesis, not the detector on a real "
                 "channel. On its own it decides nothing -- the scores reach the policy but "
                 "only --pcsNlosClassifier lets PCS read them -- and it adds no frame and no "
                 "charge. Its draws come from one dedicated stream set at construction, so "
                 "every other output is byte-identical to the flag off.",
                 nlosClassifierArg);
    cmd.AddValue("pcsNlosClassifier",
                 "Feed the classifier into PCS: term 3 becomes the spec's mean(1 - NLOS_score) "
                 "over the scored links of the fix set, instead of the fraction of links the "
                 "--nlosSource tag calls LOS (TECH_DEBT K.8: that tag fires on 75-78% of "
                 "links and pins the term). The LEGACY/WEIGHTED/GATED sums, weights and "
                 "thresholds are untouched, so the saturation defect is exactly as it was: "
                 "this changes the INPUT to w3, not the score's form. FALSE (default) is every "
                 "result so far, and it stays the default because on, PCS moves and every "
                 "escalation that reads it moves with it; the weights were never calibrated "
                 "for any term 3 (CAL-01), and K.8 orders the monotone control and CAL-01 "
                 "BEFORE live NLOS inputs. REQUIRES --nlosClassifier=true. REFUSED under "
                 "--pcsForm=v2, which has no NLOS term to feed.",
                 pcsNlosClassifierArg);
    cmd.AddValue("nlosScoreThreshold",
                 "NLOS verdict threshold for the confusion matrix: a link is called NLOS when "
                 "its score is >= this. Default 0.5, OURS and uncited: the midpoint of every "
                 "score's [0,1] range -- on UWB a leading edge at most half the peak amplitude "
                 "(6 dB down), on 2.4 GHz and 868 MHz a disagreement of two expected sigmas. "
                 "It moves the confusion matrix only; PCS reads the continuous score, never the "
                 "verdict. Sweep it for an operating curve rather than trusting one point. "
                 "Must lie in (0, 1).",
                 nlosScoreThresholdArg);
    cmd.AddValue("entryVelocity",
                 "Table entries carry the producer's velocity, as the spec's entry does "
                 "(Physical 2.1: {pos, Sigma, timestamp, seq_no, hop_count, source_type, "
                 "velocity_estimate}; docs/SPEC_CONFORMANCE_PLAN.md DC-10). A field node "
                 "publishes its EKF velocity; a finder the displacement over the time since "
                 "its last publication, which is what its GNSS receiver reports; a gateway "
                 "0, correctly. FALSE by default: the fields were billed on the wire and "
                 "published as 0, 0. Nothing reads them yet -- the RESPONSE contents "
                 "(NW-9), entry prediction (DC-13) and sessions (TA-1) will -- so turning it "
                 "on moves no other output. Writes entries_<arm>_seed<N>.csv: what each "
                 "producer published and what every node holds, once per cycle.",
                 spec.entryVelocity);
    cmd.AddValue("integrityNlosFromScore",
                 "The integrity check's eligibility from the classifier's score -- the "
                 "spec's condition, 'NLOS_score is LOW' (Physical 2.6; "
                 "docs/SPEC_CONFORMANCE_PLAN.md DC-8) -- instead of 'every link tagged "
                 "LOS'. A fix is eligible to be called SUSPECT when the mean NLOS_score over "
                 "its scored links is below --nlosScoreThreshold; a fix set with no scored "
                 "link falls back to the tag. FALSE by default, which reproduces every prior "
                 "result. Under the default tag, which fires on 75-78% of links (K.8), an "
                 "all-LOS set is so rare that the check had 0-9 eligible cycles a run (I.1), "
                 "so it could not fire. Needs --nlosClassifier=true. Writes "
                 "integrity_ca_tbl_full_seed<N>.csv, one row per fix cycle the check "
                 "evaluated.",
                 spec.integrityNlosFromScore);
    cmd.AddValue("uwbNlosModel",
                 "The UWB engine's LOS/NLOS state model, legacy or ieee802154a (TECH_DEBT M.2, "
                 "registered in report/PREREG_UWB_NLOS.md). LEGACY (default) is every UWB result "
                 "so far: an NLOS probability with kilometre scales outdoors (about 0.1 % at UWB "
                 "range) and a +9.64 dB-shifted attenuation that exceeds UWB's margin, so no "
                 "decoded UWB frame is ever NLOS and the CIR classifier's UWB matrix has no "
                 "positives. IEEE802154A draws the state from the 3GPP TR 38.901 InF-SL law, "
                 "P_NLOS = 1 - exp(-d_2D / 44.81 m) (d_clutter 10 m, r = 20 %), and charges an NLOS "
                 "draw the IEEE 802.15.4a NLOS-minus-LOS path loss (CM5/CM6 outdoor, CM3/CM4 under "
                 "--envFacility=6): about 27-37 dB outdoors over 1-20 m, deterministic in distance, "
                 "one sampler draw per evaluation and no new random variable. Only the UWB "
                 "engine's physics changes, but not only UWB outputs: which UWB frames the MAC "
                 "delivers changes, the shared MAC random stream is then consumed differently, "
                 "and every arm moves, static_868 included (measured at the change, NC1 flags). "
                 "So no magnitude compares across the two settings, only populations (rule 8). "
                 "It stays OFF by default whatever its registered runs show, for a reason "
                 "the source gives: 802.15.4a has no LOS/NLOS probability and no data for disaster "
                 "areas (its Remark 1), and its outdoor NLOS parameters are marked educated "
                 "guesses, so mapping it onto a ground-level SAR link is OURS. Any other value is "
                 "FATAL.",
                 uwbNlosModelSpec);
    cmd.AddValue("radioBasis",
                 "The radio parts the energy model is billed against (docs/SPEC_CONFORMANCE_PLAN.md "
                 "SY-2; the spec puts tiers 1 and 2 on one Semtech LR1121 die and tier 3 on a "
                 "DW3000). SX (default) is every result so far: an SX1262 at 868 MHz and an SX1280 "
                 "at 2.4 GHz. LR1121 bills both from DS.LR1121.W.APP Rev 2.0 (Dec 2023), Tables "
                 "3-4..3-6, VBAT 3.3 V, DC-DC: TX 28 mA at +14 dBm (LP PA) and 26 mA at 2.4 GHz "
                 "(the +13 dBm row for the scenario's 12.5 dBm, OURS); RX 7.8 and 7.5 mA "
                 "(RxBoosted, as the SX1262 profile); CAD at the RX current (the datasheet gives "
                 "none, OURS); idle 1.25 mA (STBY_RC); sleep 1.6 uA (RC-based RTC). UWB is "
                 "unchanged, and every timing field is kept from the SX profiles, so only currents "
                 "move: at defaults only energy columns change, while under --specProfile energy "
                 "feeds the escalation request's battery term and so arbitration. Every field "
                 "node's account and every gateway's gossip account take it; one SY-2 line. Any "
                 "other value is FATAL.",
                 radioBasisArg);
    cmd.AddValue("sf868",
                 "Tier-1 (868 MHz) spreading factor, SF7-SF12 (Physical 1.1: 'configurable "
                 "spreading factor'; docs/SPEC_CONFORMANCE_PLAN.md SY-3). Not given (0) is "
                 "SF7, which every result used: the fastest frame, so the most cycles fit "
                 "the 2.5% duty cap. A higher SF reaches further (the sensitivity table, "
                 "-123 dBm at SF7 to -137 dBm at SF12) at a longer time on air, so duty, "
                 "energy and every frame move. Prints one SY-3 line.",
                 sf868Arg);
    cmd.AddValue("bw868",
                 "Tier-1 bandwidth in Hz: 125000, 250000 or 500000 (Physical 1.1; SY-3). Not "
                 "given (0) is 125 kHz, the bandwidth the 868 MHz sensitivity table was "
                 "written for. Another value shifts that table by 10*log10(BW/125 kHz) dB "
                 "(noise bandwidth), so a wider channel is faster and hears less far.",
                 bw868Arg);
    cmd.AddValue("sf2p4",
                 "Tier-2 (2.4 GHz) spreading factor, SF5-SF12 (the SX1280 table; SY-3). Not "
                 "given (0) is SF7, every result so far.",
                 sf2p4Arg);
    cmd.AddValue("bw2p4",
                 "Tier-2 bandwidth in Hz: 203125, 406250, 812500 or 1625000 (SX1280), or "
                 "800000 (SY-3). Not given (0) is 800 kHz, which every result used and which "
                 "is not an SX1280 setting -- recorded, not changed. Another value shifts the "
                 "2.4 GHz sensitivity table by 10*log10(BW/800 kHz) dB.",
                 bw2p4Arg);
    cmd.AddValue("relayTier",
                 "Which tier a relay ranges on: 0=868, 1=2.4GHz, 2=UWB. Default 1. "
                 "NOT 0, and that is an admission condition rather than a preference "
                 "-- A1 excludes tier 1 from relayed ranging because its error is a "
                 "spatially correlated shadowing field, so two nearby tags' errors "
                 "are correlated before any relaying happens and the independence "
                 "the measurement equation assumes is absent in the medium. Setting "
                 "this to 0 produces relayed measurements that A1 will reject, which "
                 "is the intended behaviour and is worth being able to demonstrate.",
                 relayTierArg);
    cmd.AddValue("nTags",
                 "How many targets exist on the channel. 1 (default) is the "
                 "scenario as it has always been, and constructs nothing extra -- "
                 "no trajectory, no MAC, no beacon, and no draw from the shared "
                 "macRng, so the default is byte-identical by construction rather "
                 "than by measurement. Above 1, each additional target is a "
                 "PROTOCOL PEER: a real MAC on the real channel with a real "
                 "trajectory, which beacons, is discovered, contends, and carries "
                 "a distinct targetManifestId so an identity-scoped query has "
                 "something to discriminate on. A peer is NOT a second measured "
                 "subject -- it has no policy, no EKF and no energy model, and its "
                 "joules are not accounted -- except under --relayedRanging, where each "
                 "peer is also a fixed-tier relay with a policy and a ranging-only "
                 "energy account (C5). Reporting peer energy would be wrong. "
                 "Making a peer a measured system is the full per-tag refactor, "
                 "which is a separate and larger piece of work.",
                 nTagsArg);
    cmd.AddValue("identityQueryPeriodS",
                 "Seconds between identity-scoped QUERY floods from one anchor "
                 "per tier. 0 (default) issues none, which is the scenario as it "
                 "has always been. A query names a target manifest id and only "
                 "the target carrying that id answers -- that scoping is the "
                 "mechanism, and without it a query is a broadcast every target "
                 "answers. One anchor per tier rather than all of them, because "
                 "the question is whether the RIGHT target answers, not how much "
                 "query traffic the medium carries.",
                 identityQueryPeriodSArg);
    cmd.AddValue("identityQueryTarget",
                 "Which target manifest id the queries name. The measured tag is "
                 "1; peers created by --nTags are 2, 3 and so on. Default 2, so "
                 "with --nTags=2 the query names the PEER and the measured tag "
                 "must correctly ignore it -- which is the observation that shows "
                 "scoping works rather than that flooding works.",
                 identityQueryTargetArg);
    cmd.AddValue("injectTxPowerDropDb",
                 "Fault class: the measured node's transmit power drops by this many dB "
                 "over the --injectBiasFraction window, as a cold-sagged battery does (the "
                 "docx's 'cold-sagged battery RSSI skew'; exec plan Phase 5 'cold-soaking a "
                 "battery'; docs/SPEC_CONFORMANCE_PLAN.md SY-5). 0 (default) injects nothing. "
                 "Every link the node transmits on loses that much budget, and tier 1, which "
                 "inverts RSSI against the NOMINAL power, reads each range long by "
                 "10^(D/(10 n)) -- statistically clean and physically wrong, the case the "
                 "integrity check exists for. The magnitude is OURS; no random draw is taken. "
                 "Needs --injectBiasFraction > 0. Prints one SY-5 line.",
                 spec.injectTxPowerDropDb);
    cmd.AddValue("injectAntennaLossDb",
                 "Fault class: the measured node's antenna loses this many dB over the "
                 "--injectBiasFraction window, as one detuned by ice does (the docx's "
                 "'antenna detuning from ice'; 'deliberately covering an antenna'; SY-5). 0 "
                 "(default) injects nothing. Unlike --injectTxPowerDropDb it is reciprocal: "
                 "frames the node RECEIVES lose it too, so its neighbour tables thin. Tier 1 "
                 "reads long by the same rule. OURS in magnitude, no draw. Needs "
                 "--injectBiasFraction > 0.",
                 spec.injectAntennaLossDb);
    cmd.AddValue("measuredSubjects",
                 "How many field nodes the full pipeline localises (the spec's '5 field "
                 "nodes', a whole missing team; docs/SPEC_CONFORMANCE_PLAN.md MS-1). 1 "
                 "(default) is every result so far: tag 0 is measured, and the other --nTags "
                 "are protocol peers or fixed-tier relays. N > 1: tags 2..N run the core of "
                 "tag 0's cycle on their own accounts -- ranging on their active tier, the "
                 "tier-1 floor, the fix selection, Step() with its own escalation state "
                 "machine, the handshake with their OWN degradation and battery terms "
                 "(PeerRequestDriver's constant 128/128 requests retire), the gateway "
                 "downlink, listen/sleep billing, and under --imuPredict their own IMU. A "
                 "relay that is a subject is stepped once. Writes subject_summary.csv (one "
                 "row per arm, seed and subject; subject 1's row is tag 0's arm_summary row) "
                 "and subjtrace_<arm>_seed<N>_subj<id>.csv, both digested, and one MS-1 line. "
                 "Needs N <= --nTags. Tag-0-only mechanisms are warned about.",
                 spec.measuredSubjects);
    cmd.AddValue("finderTasking",
                 "Which subject each finder searches for: tag0|roundrobin (the spec's "
                 "rescuers searching for specific people; docs/SPEC_CONFORMANCE_PLAN.md MS-1). "
                 "Empty (default) or tag0 is every result so far: every finder steers on tag "
                 "0's entry. roundrobin: finder k (0-based) steers on subject (k mod N) + 1's "
                 "freshest entry, N = --measuredSubjects, and the --finderDispersal spread is "
                 "over the finders tasked to the same subject. The rule is OURS. Writes "
                 "tasking_<arm>_seed<N>.csv (every finder-cycle after dispatch: target, phase, "
                 "the entry steered on and the goal), digested, and one MS-1 tasking line. "
                 "Needs --measuredSubjects > 1, --nFinders > 0 and --finderMode 1 or 2.",
                 spec.finderTasking);
    cmd.AddValue("meshLinks",
                 "Which radio pairs can hear each other: star|all (the spec's 'sub-GHz LoRa "
                 "mesh'; docs/SPEC_CONFORMANCE_PLAN.md NW-0, the prerequisite of the network "
                 "and transport layer). Not given, or star, is every result so far: a link "
                 "exists only between a field node and an anchor, so a finder cannot reach a "
                 "gateway and a casualty cannot reach one through a peer -- the flood is "
                 "star-plus-anchor-reflection. all: every registered pair on a tier both "
                 "carry is a link (tag-tag, gateway-gateway, finder-gateway). Under it "
                 "DISCOVERY is one-hop (discovery is one-hop by definition, and flooding it "
                 "in a full mesh is a storm), the ranging poll addresses anchors only (a peer "
                 "would otherwise be ranged as an anchor at the origin), the node's sleeping "
                 "radios neither receive nor relay, and its relays are billed. About N^2 link "
                 "evaluations per frame. Prints one NW-0 line per arm.",
                 spec.meshLinks);
    cmd.AddValue("imuWitnessSource",
                 "Which IMU track the --imuCrossCheck witness compares (the final plan's "
                 "'IMU dead-reckoning consistency' before an escalation; "
                 "docs/SPEC_CONFORMANCE_PLAN.md DC-9): shs|model. Not given, or shs, is every "
                 "result so far: the SHS gait engine's absolute position, which diverges where "
                 "fixes are sparse (TECH_DEBT K.9), against the EKF estimate. model: the "
                 "synthetic IMU's track (--imuPredict), anchored at the last fused fix and "
                 "advanced by u, against the cycle's RAW radio fix -- the estimate already "
                 "contains u, so it would be the IMU checking itself -- with tolerance "
                 "K sqrt(trace R_fix + trace Sigma_acc), K = --imuConsistencyK. Splits the "
                 "witness's counters into evaluations, refusals, and refusals the evidence "
                 "gate would have made anyway (absorbed, never reported as a pass). Writes "
                 "witness_<arm>_seed<N>.csv (digested) and one DC-9 line per arm. model needs "
                 "--imuPredict and --imuCrossCheck.",
                 spec.imuWitnessSource);
    cmd.AddValue("entryPrediction",
                 "Carry a held entry forward before using it, as Physical 2.2 says (Q set by "
                 "source type: higher for IMU-drifting field nodes, lower for gateways; "
                 "docs/SPEC_CONFORMANCE_PLAN.md DC-13). False (default) is every result so "
                 "far: a finder's entry is used where it was reported, with its variance "
                 "inflated isotropically by (walk speed x age)^2, because no velocity was "
                 "populated. True, with --entryVelocity: the position is x + v x age and the "
                 "added variance (0.5 m/s x age)^2 for a finder's GNSS-derived velocity "
                 "(OURS). Gateways are surveyed and never substituted. Writes "
                 "entrypred_<arm>_seed<N>.csv (digested) and one DC-13 line per arm. Needs "
                 "--entryVelocity, without which every velocity is 0 and nothing moves.",
                 spec.entryPrediction);
    cmd.AddValue("caseD",
                 "Case D, single-range IMU-assisted tracking (Physical 2.4D; "
                 "docs/SPEC_CONFORMANCE_PLAN.md DC-4): when the final set holds exactly one "
                 "range, it is applied as a scalar measurement on the propagated state if "
                 "its innovation passes NIS <= 6.63 (chi^2, 1 dof, 0.99). The cycle is not a "
                 "position fix, but its covariance is live: PCS is computed from it, the "
                 "normal escalation decision runs (the evidence gate still applies), and it "
                 "does not count toward demotion (OURS). False (default) is every result so "
                 "far, where Step() discards a single range and coasts. Needs "
                 "--sparseInStep; without --imuPredict the propagation is constant-velocity. "
                 "Writes cased_<arm>_seed<N>.csv (digested) and one DC-4 line per arm. The "
                 "scenario's old census label 'CASE D' (the ordinary 3+ solve) is unrelated.",
                 spec.caseD);
    cmd.AddValue("imuPredict",
                 "Drive the filter's prediction with a synthetic IMU, as Physical 2.2 says "
                 "(x_k|k-1 = F x + B u_k; docs/SPEC_CONFORMANCE_PLAN.md DC-1). False (default) "
                 "is every result so far: constant velocity, Q alone. True: each cycle a "
                 "dead-reckoning error model turns the walker's true displacement into what an "
                 "IMU would report -- a heading error from a per-run gyro bias plus angle "
                 "random walk (MPU-6050, Rudyk 2020, already cited in lora-shs-parameters.h; "
                 "the BMI270 datasheet needs a PDF tool this machine lacks) and a per-run "
                 "step-length scale error of 2.66% (OURS: this project's SHS straight "
                 "traverse) -- and the filter predicts with it, its covariance the ACCUMULATED "
                 "error since the last fused fix, so drift is not understated. Its own random "
                 "stream. Writes imu_<arm>_seed<N>.csv (digested) and one DC-1 line per arm. "
                 "The SHS gait engine is untouched (owner decision 2).",
                 spec.imuPredict);
    cmd.AddValue("sparseInStep",
                 "Solve a two-range cycle inside Step(), after the prediction, against the "
                 "PROPAGATED prior, as Physical 2.4 says (docs/SPEC_CONFORMANCE_PLAN.md "
                 "DC-11). False (default) is every result so far: --sparseCaseA solves in the "
                 "scenario BEFORE Step(), against the previous cycle's posterior, fuses the "
                 "point one cycle early, and Step() then counts the cycle as a no-fix and "
                 "toward demotion -- and with relays appended it range-updates on the same "
                 "two ranges again (a double count). True: Case A when both ranges are to "
                 "surveyed gateways, Case B otherwise; the point is fused once, the ranges "
                 "never also as ranges; a solved cycle is a fix that does not count toward "
                 "demotion; with relays appended the set takes the full path. Margin: "
                 "--sparseCaseAMargin. Writes sparse_<arm>_seed<N>.csv (digested) and one "
                 "DC-11 line per arm. Replaces --sparseCaseA, so the two together are FATAL.",
                 spec.sparseInStep);
    cmd.AddValue("subjectEdgeUplink",
                 "Subjects 2..N bill the uplink of their tier-2/3 ranges, as --edgeUplinkRanges "
                 "bills tag 0's (SY-1; TECH_DEBT N.3, register row 55). FALSE by default: the "
                 "subjects' uplink is not billed, and a start-up WARNING says --edgeUplinkRanges "
                 "is tag 0's only. TRUE: each subject's ranging exchange on 2.4 GHz or UWB that "
                 "returns a range is followed by one frame on that tier, 9 B + 6 B per answered "
                 "anchor (OURS, as tag 0's), billed to the subject. Only energy moves at defaults. "
                 "One SY-1 subjects line. Needs --edgeUplinkRanges and --measuredSubjects > 1.",
                 spec.subjectEdgeUplink);
    cmd.AddValue("edgeUplinkRanges",
                 "Bill the uplink that carries a tier-2 or tier-3 range to the gateway "
                 "estimator (TECH_DEBT L.2.3; docs/SPEC_CONFORMANCE_PLAN.md SY-1). READ ONLY "
                 "WITH --gatewayEdgeServer=true. False (default) is every result so far: the "
                 "gateways fuse ranges the node measured by two-way time of flight, and the "
                 "frame that would bring each range to them is never billed, which flatters "
                 "the edge setting on tiers 2 and 3 and not on tier 1, whose gateways measure "
                 "the broadcast themselves. True bills one frame on the tier that ranged, per "
                 "cycle with at least one range: the 9-byte header plus 6 bytes per answered "
                 "anchor (OURS: id, decimetre range, sigma, NLOS score). At defaults only "
                 "energy moves, and only on tiers 2 and 3 (gate G35); --batteryDepleteJ and "
                 "--uwbArbitration read energy, so under them more can move. Prints one SY-1 "
                 "line per arm.",
                 spec.edgeUplinkRanges);
    cmd.AddValue("deadBatteryNoDraw",
                 "A node whose battery has run out draws nothing more (TECH_DEBT N.20). FALSE by "
                 "default: --batteryDepleteJ stops a dead node's ranging and silences its radios, "
                 "but the listen and sleep accounting does not read the battery, so it goes on "
                 "billing the dead node for reception, the neighbour-search window and sleep (and "
                 "tag 0 for escalation handshakes), and "
                 "every such run overstates what a dead node draws. TRUE: from the cycle its "
                 "budget runs out, tag 0 (and each subject under --subjectBatteryDeplete) is billed "
                 "for none of the three, nor tag 0 for the escalation handshakes its coasting "
                 "filter still asks for (their frames are already suppressed); a frame it transmitted (the last gasp, or a relay past CAD "
                 "at the moment of death) is still billed. One N.20 line. Needs --batteryDepleteJ "
                 "> 0 and --silenceStopsRelay (a dead radio that relayed while drawing nothing "
                 "would be billed for transmitting and not for the reception that fed it).",
                 spec.deadBatteryNoDraw);
    cmd.AddValue("halfDuplex",
                 "Make every radio half-duplex (TECH_DEBT N.6). 0 (default): unchanged -- a "
                 "radio's CAD never sees its own frame (CaTblChannel::IsBusy skips the "
                 "observer's transmissions), so a MAC with two frames queued can start the "
                 "second while the first is on air, the two then collide at every receiver, and "
                 "a radio receives other nodes' frames while transmitting. 1: COUNT ONLY -- "
                 "each case is counted and nothing else changes, so every output is "
                 "byte-identical to 0. 2: ENFORCE -- a frame whose turn comes while the radio's "
                 "own frame is on air goes back to CAD when it ends (a fresh CAD, billed), and a "
                 "frame that overlaps any of the receiving radio's own transmission is not "
                 "received (an events_ row with outcome halfduplex). Per radio: a node's three "
                 "radios are separate (the LR1121 serves tiers 1 and 2 from one chip; that is "
                 "not modelled). One N.6 line. Any other value is FATAL.",
                 spec.halfDuplex);
    cmd.AddValue("silenceStopsRelay",
                 "A silenced radio relays no other node's frame (TECH_DEBT N.1). FALSE by "
                 "default: a radio silenced by a spent battery (--batteryDepleteJ, "
                 "--subjectBatteryDeplete) or by --injectSilentId stops its own frames, since "
                 "SetSilent() gates Send(), but goes on relaying other nodes' floods and any "
                 "relay it had queued, so every such run overstates how much of the mesh the "
                 "fault removes. TRUE: the MAC refuses, at CAD, every frame another node "
                 "originated while the radio is silenced, and counts it; the node's own frames "
                 "are untouched, so a frame queued before the silence (MC-2's last gasp) still "
                 "leaves. One N.1 line. Needs --batteryDepleteJ > 0 or --injectSilentId.",
                 spec.silenceStopsRelay);
    cmd.AddValue("finderPseudoAnchorStrict",
                 "No finder range uses the finder's true position (TECH_DEBT N.21). FALSE by "
                 "default: under --finderPseudoAnchor tag 0 replaces a finder's true position with "
                 "its report when it holds an entry, and keeps the truth when it holds none, or "
                 "when the report's variance cannot be projected. TRUE: a finder range tag 0 holds "
                 "no entry for is dropped (received and billed, not used), and where the variance "
                 "cannot be projected the report is used uninflated; the same uninflated fallback "
                 "for subjects under --subjectFinderPseudoAnchor. Writes pseudo_<arm>_seed<N>.csv "
                 "(tag 0's finder ranges, digested) and one N.21 strict line. Needs "
                 "--finderPseudoAnchor.",
                 spec.finderPseudoAnchorStrict);
    cmd.AddValue("subjectFinderPseudoAnchor",
                 "A subject's range to a finder uses the finder's REPORTED position, as tag 0's does "
                 "under --finderPseudoAnchor (TECH_DEBT N.21). FALSE by default: subjects 2..N range "
                 "a finder against its TRUE position with the engine's sigma -- knowledge no node "
                 "has. TRUE: the freshest entry the subject's radios hold for that finder, carried "
                 "forward under --entryPrediction (DC-13's 0.5 m/s Q) or bounded by walk speed x age "
                 "otherwise, its covariance projected onto the range direction and added to sigma, "
                 "exactly as tag 0's. With no entry held the range is dropped (received and billed, "
                 "not used): the subject cannot know where the finder is. Tag 0's code keeps the "
                 "true position there, recorded in N.21. Under --bidirectionalDigest the subject "
                 "also asks the finders' digest question on its active tier, every finder digest "
                 "period, subject k of N at (0.5 + k/(2N)) of it (OURS: staggered off the finders' "
                 "broadcast and tag 0's question, which one shared instant drowned), so it can "
                 "hold their entries (unbilled, as tag 0's). Writes "
                 "subjpseudo_<arm>_seed<N>.csv (digested) and one N.21 line. Needs "
                 "--finderPseudoAnchor and --measuredSubjects > 1.",
                 spec.subjectFinderPseudoAnchor);
    cmd.AddValue("subjectBatteryDeplete",
                 "Subjects 2..N run --batteryDepleteJ's fault, as tag 0 does (FAULT sect 49; TECH_DEBT "
                 "N.3, register row 55). FALSE by default: only tag 0's battery runs out, and a "
                 "start-up WARNING says so. TRUE: the cycle a subject's energy reaches the budget, "
                 "its radios fall silent, after one unrepeated LAST GASP on its active radio under "
                 "--subjectTriggeredUpdates (queued first, billed, as tag 0's); from then on it "
                 "makes no ranging exchange, sends no request and ranges no peer or relay. A silent "
                 "radio still forwards other nodes' floods (TECH_DEBT N.1). One FAULT line per "
                 "subject. Needs --batteryDepleteJ > 0 and --measuredSubjects > 1.",
                 spec.subjectBatteryDeplete);
    cmd.AddValue("subjectTriggeredUpdates",
                 "Subjects 2..N send MC-2's LARGE CORRECTION, as --triggeredUpdates makes tag 0 send "
                 "it (MAC sect 3, 8; TECH_DEBT N.3, register row 55). FALSE by default: only tag 0 "
                 "does, and a start-up WARNING says so. TRUE: when a subject's posterior moves more "
                 "than --triggeredUpdateK sqrt(trace P) of its previous posterior from the "
                 "constant-velocity prediction, its corrected entry is published and pushed as a "
                 "TRIGGERED_UPDATE on the tier it transmitted on this cycle, with the redundant "
                 "class's three repeats, billed to it. No last gasp: subjects have no battery "
                 "depletion. Writes subjtriggered_<arm>_seed<N>.csv (digested) and one MC-2 subjects "
                 "line. Needs --triggeredUpdates and --measuredSubjects > 1.",
                 spec.subjectTriggeredUpdates);
    cmd.AddValue("triggeredUpdates",
                 "Send the spec's 'Triggered-update / last-gasp -- large correction or dying "
                 "battery' (MAC sect 3 and 8; docs/SPEC_CONFORMANCE_PLAN.md MC-2). False "
                 "(default) is every result so far, where the only TRIGGERED_UPDATE is "
                 "StartHandshake's RECEDING push. True adds two triggers for the measured "
                 "node. A LARGE CORRECTION: the posterior moved more than k x sqrt(trace P) "
                 "from the constant-velocity prediction in one cycle (k is "
                 "--triggeredUpdateK; it and the use of the previous posterior's P are OURS), "
                 "so the corrected entry is pushed at once on "
                 "the active radio, with the redundant class's 3 jittered repeats. A LAST GASP: "
                 "when --batteryDepleteJ is crossed, one unrepeated frame goes out before the "
                 "radio is silenced. Both are billed to the node on the tier that transmitted "
                 "this cycle. Writes triggered_<arm>_seed<N>.csv (digested) and one MC-2 line "
                 "per arm. Frames, energy and duty move.",
                 spec.triggeredUpdates);
    cmd.AddValue("triggeredUpdateK",
                 "Under --triggeredUpdates, how far the posterior must move from the "
                 "prediction, in units of sqrt(trace P) of the previous posterior, to count "
                 "as a large correction (MC-2). 1 (default, OURS): the entry the mesh holds "
                 "is then off by more than the uncertainty it was published with. Not 3, "
                 "the plan's draft: a Kalman update moves the mean by about one prior sigma "
                 "at most (max 1.05 sigma over 359 cycles at seed 7), so a 3-sigma trigger "
                 "never fires. FATAL unless > 0, and FATAL without --triggeredUpdates.",
                 spec.triggeredUpdateK);
    cmd.AddValue("handshakeFixes",
                 "The escalation handshake as the spec's MAC sect 8 describes it: 'Tier "
                 "escalation request (UWB specifically): ACK'd -- must know definitively "
                 "whether the grant was received' (docs/SPEC_CONFORMANCE_PLAN.md MC-1). False "
                 "(default) is every result so far, with five defects: an ACK clears its "
                 "receiver's OLDEST pending frame, whatever it acknowledged; an anchor ACKs a "
                 "request BEFORE arbitrating, so a denied request is settled and never retried; "
                 "the pre-emption send carries no terms (0/0); the other two sends carry the "
                 "previous cycle's terms; and 868 -> 2.4 GHz escalations and de-escalations "
                 "are signalled and charged as well. True fixes all five: ACKs name the frame "
                 "they acknowledge, an anchor answers a request only with a GRANT saying "
                 "granted or denied, a denial is retried on the ACK timeout, the terms are "
                 "this cycle's, and only escalations into UWB send a request -- so energy, "
                 "duty and every frame move. Prints one MC-1 line per arm. Needs "
                 "--sendHandshakeFrames=true, without which no handshake frame exists.",
                 spec.handshakeFixes);
    cmd.AddValue("queryTierBinding",
                 "Which radio each tier's identity-query driver sends on: first|per-tier "
                 "(docs/SPEC_CONFORMANCE_PLAN.md NW-3). Not given, or first, is the build "
                 "before: every tier's driver is bound to the first anchor MAC, gateway "
                 "101's 868 MHz radio, so a '2.4 GHz' query goes out with 868 MHz's SF and "
                 "bandwidth, is charged to that radio's duty budget, and is heard by 868 MHz "
                 "radios. per-tier binds each driver to the same gateway's radio on that "
                 "tier. Given explicitly, either value prints the binding on stdout. Needs "
                 "--identityQueryPeriodS > 0, where the drivers exist.",
                 spec.queryTierBinding);
    cmd.AddValue("uwbArbitration",
                 "Whether anchors arbitrate between competing UWB requests rather "
                 "than granting whoever asked. False (default) is the C1 "
                 "behaviour: every ESC_REQUEST is granted and contention alone "
                 "decides who lands. True scores U(r) = l1*T_deg + l2*T_batt + "
                 "l3*T_stale + l4*T_query, ported from the arbitration harness "
                 "rather than reinvented. T_deg and T_batt ride on the request "
                 "because an anchor cannot see another node PCS history or its "
                 "battery; T_stale and T_query it computes itself. NOTE that every "
                 "ESC_REQUEST carries the same access category, so T_query is a "
                 "constant across competing requests and cannot order them.",
                 uwbArbitrationArg);
    cmd.AddValue("arbBatteryCapJ",
                 "Joule budget T_batt is measured against: spent/cap, saturating "
                 "at 1. Default 1000 and it is OURS -- there is no battery model "
                 "in this simulator, so this is a stated reference rather than a "
                 "measured capacity. It sets how fast the battery term saturates "
                 "and so how much it can influence arbitration; sweep it before "
                 "trusting any conclusion that leans on T_batt.",
                 arbBatteryCapJArg);
    cmd.AddValue("anchorGrantGossipS",
                 "Seconds between rounds in which each gateway broadcasts the UWB "
                 "slots it holds to neighbouring gateways (Network sect 6), so that "
                 "two gateways holding overlapping grants for one field node resolve "
                 "it by the spec's rule: the LOWER node id defers. 0 (default) is "
                 "the scenario as it has always been -- no coordinator, no gossip, "
                 "every ESC_REQUEST granted by every gateway that hears it -- and it "
                 "has to stay the default because turning it on adds tier-1 frames "
                 "from every gateway, and every added frame displaces the shared "
                 "engine RNG for every link in the run (rule 8), so no magnitude is "
                 "comparable across the two settings; only which code paths ran is. "
                 "The gossip rides tier 1, 868 MHz, because it is the only radio "
                 "that closes a gateway-to-gateway link; it is charged to a "
                 "per-gateway tier-1 account reported on stdout, never to the field "
                 "node. REQUIRES --sendHandshakeFrames=true (without frames no "
                 "gateway ever grants, and every counter would be a structural "
                 "zero), so that combination is refused rather than run. Keep the "
                 "period BELOW the 10 s slot window: a grant that has ended is never "
                 "gossiped, so a longer period lets every double grant expire "
                 "unresolved and the mechanism cannot fire.",
                 anchorGrantGossipSArg);
    cmd.AddValue("suspectForcesEscalation",
                 "Whether a suspect fix also forces an escalation. true (default) "
                 "preserves the pre-existing baseline, in which `suspect` sets "
                 "wantEscalate unconditionally. That side effect dominates any "
                 "accuracy effect the gate has, so it makes the integrity check "
                 "unmeasurable AS an integrity check -- ca_tbl_full vs "
                 "ca_tbl_no_integrity then measures the escalation, not the "
                 "rejection. Set false to isolate the mechanism report §2.3/§10.5 "
                 "actually claim: a suspect fix is rejected and nothing else "
                 "happens.",
                 suspectForcesEscalation);
    cmd.AddValue("nlosSource",
                 "Which physical quantity the NLOS tag is built from: "
                 "legacy|propagation. 'legacy' is (nlosDb > 0 || hiddenChannelState "
                 "> 0), which tags ~75% of links because hiddenChannelState is a "
                 "4-state Markov FADING REGIME and not an obstruction indicator; it "
                 "remains the default so results/baseline_v3 reproduces "
                 "byte-identically. 'propagation' uses the engine's own NLOS "
                 "determination (PhysicsResult::isNLOS) alone. Both emit "
                 "nNlosPropagation and nHiddenChannel so the populations can be "
                 "compared. No mode consumes an RNG draw, so all share one channel "
                 "realisation. A 'severe' mode keyed on the Markov state was "
                 "deliberately NOT added -- see the CaTblNlosSource comment.",
                 nlosSourceArg);
    // P6. --specProfile is resolved BEFORE the parse: every spec setting the user did not
    // give explicitly is put ahead of the user's own arguments, so an explicit flag is
    // parsed later and always wins. The plan's "resolve after Parse" by a route that needs
    // no sentinel defaults. Off, argv goes to Parse unchanged.
    bool specProfileOn = false;
    std::set<std::string> explicitFlags;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a.rfind("--", 0) != 0)
        {
            continue;
        }
        const size_t eq = a.find('=');
        const std::string nm = a.substr(2, eq == std::string::npos ? std::string::npos : eq - 2);
        explicitFlags.insert(nm);
        if (nm == "specProfile")
        {
            const std::string v = (eq == std::string::npos) ? "true" : a.substr(eq + 1);
            specProfileOn = (v == "true" || v == "1");
        }
    }
    std::vector<std::string> specArgs;
    if (specProfileOn)
    {
        for (const CaTblSpecSetting& ss : kSpecProfile)
        {
            if (!explicitFlags.count(ss.name))
            {
                specArgs.push_back(std::string("--") + ss.name + "=" + ss.value);
            }
        }
    }
    std::vector<char*> argv2;
    argv2.push_back(argv[0]);
    for (size_t i = 0; i < specArgs.size(); ++i)
    {
        argv2.push_back(const_cast<char*>(specArgs[i].c_str()));
    }
    for (int i = 1; i < argc; ++i)
    {
        argv2.push_back(argv[i]);
    }
    cmd.Parse(static_cast<int>(argv2.size()), argv2.data());

    if (specProfileOn)
    {
        if (topologyArg == 1)
        {
            std::cerr << "FATAL: --specProfile with --topology=1: the legacy single-tier model "
                         "is never the spec\n";
            return 1;
        }
        std::ofstream sp((outdir + "/spec_profile.txt").c_str());
        if (!sp.is_open())
        {
            std::cerr << "FATAL: cannot open " << outdir
                      << "/spec_profile.txt for writing -- does --outdir exist?\n";
            return 1;
        }
        sp << "# --specProfile (docs/SPEC_CONFORMANCE_PLAN.md sect 3.1): what the preset set, and "
              "what the user gave explicitly instead\n"
              "# the geometry is GEO-01 stage 2's registered fallback; the PCS is CAL-02's "
              "calibrated form; the radio basis is the LR1121 (SY-2)\n";
        size_t kept = 0;
        for (const CaTblSpecSetting& ss : kSpecProfile)
        {
            if (explicitFlags.count(ss.name))
            {
                ++kept;
                sp << ss.name << ": explicit, the preset's " << ss.value << " not applied  # "
                   << ss.why << "\n";
            }
            else
            {
                sp << ss.name << "=" << ss.value << "  # " << ss.why << "\n";
            }
        }
        std::cerr << "NOTE: --specProfile set " << (sizeof(kSpecProfile) / sizeof(kSpecProfile[0]) - kept)
                  << " spec settings (" << kept << " given explicitly and kept); "
                  << outdir << "/spec_profile.txt lists them.\n";
    }

    g_terrainPerTierFreq = terrainPerTierFreqArg;

    // SY-2. --radioBasis is resolved here, before any energy account exists. The default
    // prints nothing, so no output moves with it.
    if (radioBasisArg == "lr1121")
    {
        g_radioBasisLr1121 = true;
        const RadioPowerProfile a = RadioPowerProfile::Lr1121_868();
        const RadioPowerProfile b = RadioPowerProfile::Lr1121_2p4();
        std::cout << "SY-2 radio basis lr1121 (DS.LR1121.W.APP Rev 2.0, Tables 3-4..3-6): 868 MHz "
                  << "tx " << a.txCurrentMa << " rx " << a.rxCurrentMa << " cad " << a.cadCurrentMa
                  << " idle " << a.idleCurrentMa << " sleep " << a.sleepCurrentMa << " mA; 2.4 GHz tx "
                  << b.txCurrentMa << " rx " << b.rxCurrentMa << " cad " << b.cadCurrentMa << " idle "
                  << b.idleCurrentMa << " sleep " << b.sleepCurrentMa
                  << " mA; UWB DW3000 unchanged; timing as the SX profiles\n";
    }
    else if (radioBasisArg != "sx")
    {
        std::cerr << "FATAL: --radioBasis must be sx or lr1121, got '" << radioBasisArg << "'\n";
        return 1;
    }

    // M.2. --uwbNlosModel is resolved here, before any engine config is built. Silent in
    // every case, so no output file, stdout or stderr line moves with the default.
    {
        std::string s = uwbNlosModelSpec;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (s == "legacy")
        {
            g_uwbNlosModel = UWB_NLOS_LEGACY;
        }
        else if (s == "ieee802154a")
        {
            g_uwbNlosModel = UWB_NLOS_IEEE802154A;
        }
        else
        {
            std::cerr << "FATAL: --uwbNlosModel must be legacy or ieee802154a, got '"
                      << uwbNlosModelSpec << "'\n";
            return 1;
        }
    }

    CaTblPcsForm pcsForm = CA_TBL_PCS_LEGACY;
    if (pcsFormArg == "v2")
    {
        pcsForm = CA_TBL_PCS_V2;
    }
    else if (pcsFormArg == "weighted")
    {
        pcsForm = CA_TBL_PCS_WEIGHTED;
    }
    else if (pcsFormArg == "gated")
    {
        pcsForm = CA_TBL_PCS_GATED;
    }
    else if (pcsFormArg == "calibrated")
    {
        // DC-6. The spec's PCS: the weighted sum, with weights AND thresholds given
        // explicitly -- the values a leave-one-out calibration produced (CAL-02). Refused
        // unless all eight are given: a half-calibrated score would silently borrow hand-set
        // values for the rest, which is the state the spec's calibration exists to end.
        pcsForm = CA_TBL_PCS_WEIGHTED;
        bool all = (pcsW1Arg >= 0.0) && (pcsW2Arg >= 0.0) && (pcsW3Arg >= 0.0);
        for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
        {
            all = all && std::isfinite(spec.pcsThrUp[k]) &&
                  (k == CA_TBL_TIER_868 || std::isfinite(spec.pcsThrDown[k]));
        }
        if (!all)
        {
            std::cerr << "FATAL: --pcsForm=calibrated needs --pcsW1, --pcsW2, --pcsW3 (>= 0), "
                      << "the three --pcsThrUp* and the two --pcsThrDown* given explicitly\n";
            return 1;
        }
    }
    else if (pcsFormArg != "legacy")
    {
        std::cout << "ERROR: --pcsForm must be 'legacy', 'v2', 'weighted', 'gated' or "
                  << "'calibrated', got '" << pcsFormArg << "'\n";
        return 1;
    }
    // DC-6. Resolve the thresholds exactly as RunArm will, on a probe config, whenever one
    // was given explicitly. An inverted pair (down below up) would let a tier escalate and
    // be counted healthy on the same score -- thrashing by construction -- and is refused
    // whichever of the two was given: --pcsThrUp868=2.0 alone inverts against the built-in
    // 1.75 just as surely. 868 MHz is exempt: its down threshold decides nothing. What was
    // resolved is printed, so a flag landing on the wrong tier is visible (gate G28).
    bool anyPcsThr = false;
    for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
    {
        anyPcsThr = anyPcsThr || std::isfinite(spec.pcsThrUp[k]) ||
                    std::isfinite(spec.pcsThrDown[k]);
    }
    if (anyPcsThr)
    {
        CaTblPolicyConfig probe;
        if (pcsForm == CA_TBL_PCS_V2)
        {
            probe.ApplyV2Thresholds();
        }
        ResolvePcsThresholds(probe, pcsHysteresisBandArg, spec);
        if (pcsW1Arg >= 0.0) { probe.pcsW1 = pcsW1Arg; }
        if (pcsW2Arg >= 0.0) { probe.pcsW2 = pcsW2Arg; }
        if (pcsW3Arg >= 0.0) { probe.pcsW3 = pcsW3Arg; }
        for (int k = CA_TBL_TIER_868 + 1; k < CA_TBL_TIER_COUNT; ++k)
        {
            if (probe.pcsThresholdDown[k] < probe.pcsThresholdUp[k])
            {
                std::cerr << "FATAL: PCS de-escalation threshold " << probe.pcsThresholdDown[k]
                          << " is below the escalation threshold " << probe.pcsThresholdUp[k]
                          << " on tier " << k << " (as resolved)\n";
                return 1;
            }
        }
        std::cerr << std::setprecision(17) << "NOTE: PCS form " << pcsFormArg
                  << " resolved: up 868/2p4/uwb " << probe.pcsThresholdUp[CA_TBL_TIER_868]
                  << " " << probe.pcsThresholdUp[CA_TBL_TIER_2P4] << " "
                  << probe.pcsThresholdUp[CA_TBL_TIER_UWB] << "; down 2p4/uwb "
                  << probe.pcsThresholdDown[CA_TBL_TIER_2P4] << " "
                  << probe.pcsThresholdDown[CA_TBL_TIER_UWB];
        if (pcsForm == CA_TBL_PCS_WEIGHTED)
        {
            std::cerr << "; weights " << probe.pcsW1 << " " << probe.pcsW2 << " "
                      << probe.pcsW3;
        }
        std::cerr << std::setprecision(6) << "\n";
    }

    CaTblNlosSource nlosSource = CA_TBL_NLOS_LEGACY;
    if (nlosSourceArg == "propagation")
    {
        nlosSource = CA_TBL_NLOS_PROPAGATION;
    }
    else if (nlosSourceArg != "legacy")
    {
        std::cout << "ERROR: --nlosSource must be 'legacy' or 'propagation', got '"
                  << nlosSourceArg << "'\n";
        return 1;
    }

    // L.1.4. Refused rather than run: without handshake frames no gateway ever grants,
    // so the tie-break would report zeros that are a property of the configuration and
    // not of the mechanism (TECH_DEBT I.4 -- four flags already fail silently this way).
    if (anchorGrantGossipSArg < 0.0 ||
        (anchorGrantGossipSArg > 0.0 && !sendHandshakeFramesArg))
    {
        std::cerr << "FATAL: --anchorGrantGossipS=" << anchorGrantGossipSArg
                  << " needs a value >= 0 and, when > 0, --sendHandshakeFrames=true\n";
        return 1;
    }
    if (!(spec.arbSlotHoldS > 0.0) ||
        (spec.arbSlotHoldS != CaTblMacConfig().arbSlotHoldS && !uwbArbitrationArg))
    {
        std::cerr << "FATAL: --arbSlotHoldS must be > 0, and is read only by the "
                     "--uwbArbitration arbiter\n";
        return 1;
    }
    if (anchorGrantGossipSArg >= spec.arbSlotHoldS)
    {
        std::cerr << "WARNING: --anchorGrantGossipS=" << anchorGrantGossipSArg
                  << " is not below the " << spec.arbSlotHoldS
                  << " s slot window; a double grant can expire before either anchor"
                  << " hears of it, and then the tie-break never fires\n";
    }
    // L.1.2. Refused rather than run: without relay filters, or with a single relay, no
    // relay can lean on another and every chain counter would be a structural zero.
    if (relayChainArg && (!relayedRangingArg || nTagsArg < 3))
    {
        std::cerr << "FATAL: --relayChain needs --relayedRanging=true and --nTags >= 3 (got "
                  << (relayedRangingArg ? "true" : "false") << ", " << nTagsArg << ")\n";
        return 1;
    }
    if (relayChainArg && maxRelayDepthArg < 2)
    {
        std::cerr << "WARNING: --relayChain with --maxRelayDepth=" << maxRelayDepthArg
                  << ": every depth-2 range will be refused by the ceiling, never by the"
                  << " budget\n";
    }
    // C5_INSTANT. --relaySlots is resolved here. 'auto', the default since 2026-09-30, is
    // on exactly when there is a relay to schedule, so it can never trip the refusal below,
    // and in a run without relayed ranging it is off and nothing is constructed. 'true'
    // and 'false' are the values the flag had as a boolean, spelled as ns-3 parses a bool;
    // a bare --relaySlots arrives empty and means true, as the boolean's toggle did.
    bool relaySlotsAuto = false;
    {
        std::string s = relaySlotsSpec;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (s == "auto")
        {
            relaySlotsAuto = true;
            relaySlotsArg = relayedRangingArg && nTagsArg >= 2;
        }
        else if (s.empty() || s == "true" || s == "t" || s == "1")
        {
            relaySlotsArg = true;
        }
        else if (s == "false" || s == "f" || s == "0")
        {
            relaySlotsArg = false;
        }
        else
        {
            std::cerr << "FATAL: --relaySlots must be auto, true or false, got '"
                      << relaySlotsSpec << "'\n";
            return 1;
        }
    }
    // What a refusal says when the round came from the default rather than the command line.
    const char* relaySlotsWhy =
        relaySlotsAuto ? " (--relaySlots=auto, on by default in relayed runs since 2026-09-30;"
                         " --relaySlots=false runs every relay exchange at the cycle instant,"
                         " as before)"
                       : "";
    // Refused rather than run: with no relay there is nothing to schedule and every count
    // would be a structural zero; and a round longer than the cycle would put its first
    // slots in the previous cycle, where they are not this cycle's exchanges.
    if (relaySlotsArg && (!relayedRangingArg || nTagsArg < 2))
    {
        std::cerr << "FATAL: --relaySlots schedules the relays' exchanges; it needs"
                  << " --relayedRanging=true and --nTags >= 2 (got "
                  << (relayedRangingArg ? "true" : "false") << ", " << nTagsArg << ")\n";
        return 1;
    }
    if (relaySlotsArg)
    {
        const double nr = static_cast<double>(nTagsArg - 1);
        const double k = nr + (relayChainArg ? nr * (nr - 1.0) : 0.0);
        if (!(k * FixRouter::kSlotS < dt))
        {
            std::cerr << "FATAL: --relaySlots needs " << k << " slots of "
                      << FixRouter::kSlotS * 1e3 << " ms at --nTags=" << nTagsArg
                      << (relayChainArg ? " with --relayChain" : "") << ", which does not"
                      << " fit inside a --dt=" << dt << " s cycle" << relaySlotsWhy << "\n";
            return 1;
        }
    }
    // The run says which it got, on stderr, so no output file moves: a relayed run whose
    // round came from the default is the case the 2026-09-30 change created, and its
    // invocation line in run_id.txt does not name the flag.
    if (relaySlotsAuto && relaySlotsArg)
    {
        std::cerr << "NOTE: --relaySlots=auto is on (the default for --relayedRanging runs"
                  << " since 2026-09-30); --relaySlots=false restores the collision relayed"
                  << " runs before it were measured under\n";
    }
    // L.1.1. Refused rather than run, on the same reasoning as --relayChain: without relay
    // filters there is no account to charge, and with one relay no route through another.
    if (fixRoutingConfidenceArg && !fixRoutingArg)
    {
        std::cerr << "FATAL: --fixRoutingConfidence selects the metric --fixRouting routes on;"
                  << " it needs --fixRouting=true\n";
        return 1;
    }
    if (fixRoutingArg && (!relayedRangingArg || nTagsArg < 3))
    {
        std::cerr << "FATAL: --fixRouting needs --relayedRanging=true and --nTags >= 3 (got "
                  << (relayedRangingArg ? "true" : "false") << ", " << nTagsArg << ")\n";
        return 1;
    }
    // DC-3.
    if (spec.caseCRelative &&
        (spec.measuredSubjects < 2 || spec.meshLinks != "all" || !spec.sessionOverMesh))
    {
        std::cerr << "FATAL: --caseCRelative needs --measuredSubjects > 1, --meshLinks=all and "
                     "--sessionOverMesh: peers must be subjects, a subject hears a peer only over "
                     "mesh links, and its ranges reach the base station only in its answers\n";
        return 1;
    }
    // NW-2.
    if (spec.fixRoutingSubjects && (!fixRoutingArg || spec.measuredSubjects < 2))
    {
        std::cerr << "FATAL: --fixRoutingSubjects needs --fixRouting and --measuredSubjects > 1: "
                     "without subjects there is nothing to route on its own tier\n";
        return 1;
    }
    {
        // A weight the preset applied (CAMP-SPEC's pick, owner decision 2026-10-03) is not
        // refused when --fixRouting is turned off explicitly: the user turned routing off,
        // and the preset's weights are simply unused. Only a weight given explicitly, or
        // set without the preset, is refused without --fixRouting.
        const bool presetWeights =
            specProfileOn && !explicitFlags.count("routeAlpha") && !explicitFlags.count("routeBeta") &&
            !explicitFlags.count("routeGamma") && !explicitFlags.count("routeSlack");
        const bool rwSet = !presetWeights &&
                           (spec.routeAlpha != 1.0 || spec.routeBeta != 1.0 ||
                            spec.routeGamma != 1.0 || spec.routeSlack != 1);
        if (rwSet && !fixRoutingArg)
        {
            std::cerr << "FATAL: --routeAlpha/--routeBeta/--routeGamma/--routeSlack weight "
                         "--fixRouting's confidence cost; they need --fixRouting\n";
            return 1;
        }
        if (!(spec.routeAlpha >= 0.0) || !(spec.routeBeta >= 0.0) || !(spec.routeGamma >= 0.0) ||
            !std::isfinite(spec.routeAlpha + spec.routeBeta + spec.routeGamma) ||
            spec.routeSlack > 3)
        {
            std::cerr << "FATAL: route weights must be finite and >= 0, and --routeSlack at "
                         "most 3 (FixRouter's hop limit)\n";
            return 1;
        }
    }
    // N.12. Refused since 2026-10-03 (owner decision), where it used to warn: without
    // --fixRoutingSubjects the router puts subjects 2..N on --relayTier whatever tier they
    // ranged on, a second tier transmitting in one cycle (sect 6; 671 of 1 204 frames in
    // G59's old control). No tracked result used the combination.
    if (fixRoutingArg && spec.measuredSubjects > 1 && !spec.fixRoutingSubjects)
    {
        std::cerr << "FATAL: --fixRouting with --measuredSubjects > 1 needs --fixRoutingSubjects: "
                     "without it subjects 2..N route on --relayTier whatever tier they ranged "
                     "on, a second tier transmitting in one cycle (sect 6; TECH_DEBT N.12)\n";
        return 1;
    }
    if (fixRoutingArg && relayTierArg == 0)
    {
        std::cerr << "WARNING: --fixRouting on --relayTier=0: each relay's ranging broadcast"
                  << " spends its 2.5% tier-1 budget every cycle, so routed frames will be"
                  << " refused by the duty governor\n";
    }
    // M.10. --edgeDownlinkTier is resolved here. Silent in every case, so no output file,
    // stdout or stderr line moves with it; under --gatewayEdgeServer=false it is never read.
    {
        std::string s = edgeDownlinkTierSpec;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (s == "fix")
        {
            edgeDownlinkOnFixTierArg = true;
        }
        else if (s == "active")
        {
            edgeDownlinkOnFixTierArg = false;
        }
        else
        {
            std::cerr << "FATAL: --edgeDownlinkTier must be fix or active, got '"
                      << edgeDownlinkTierSpec << "'\n";
            return 1;
        }
    }
    // L.2.1. Refused rather than run: with the estimator on the node and no reply billed,
    // the gateways' stamps would reach its filter on frames nobody paid for -- the very
    // billing/observable disagreement the flag exists to close.
    if (tier1TdoaArg && !tier1ChargeRepliesArg && !gatewayEdgeServerArg)
    {
        std::cerr << "FATAL: --tier1Tdoa needs the stamps to reach the estimator on a billed"
                  << " frame: keep --tier1ChargeReplies=true, or set --gatewayEdgeServer=true\n";
        return 1;
    }
    if (!(tdoaPpsSigmaNsArg >= 0.0) || !std::isfinite(tdoaPpsSigmaNsArg))
    {
        std::cerr << "FATAL: --tdoaPpsSigmaNs=" << tdoaPpsSigmaNsArg << " must be >= 0\n";
        return 1;
    }
    // L.2.2. A PCS input with no classifier behind it would silently fall back to the tag on
    // every cycle -- a flag that reads as on and does nothing.
    if (pcsNlosClassifierArg && !nlosClassifierArg)
    {
        std::cerr << "FATAL: --pcsNlosClassifier needs --nlosClassifier=true: without it no link"
                  << " carries a score and PCS would read the tag on every cycle\n";
        return 1;
    }
    // DC-8, for the same reason: with no classifier every fix set is unscored and the tag
    // would decide on every cycle.
    if (spec.integrityNlosFromScore && !nlosClassifierArg)
    {
        std::cerr << "FATAL: --integrityNlosFromScore needs --nlosClassifier=true: without it "
                  << "no link carries a score and the tag would decide on every cycle\n";
        return 1;
    }
    // MC-2. k is read only by the trigger, and k <= 0 would fire on every cycle.
    // M.9. Nothing takes the RECEDING branch without mobility state.
    if (spec.reportRecedingPushes && !mobilityFromRangeArg)
    {
        std::cerr << "FATAL: --reportRecedingPushes needs --mobilityFromRange: without it "
                     "every node reports STATIONARY and no push is ever made\n";
        return 1;
    }
    // N.20.
    if (spec.deadBatteryNoDraw && (!(batteryDepleteJArg > 0.0) || !spec.silenceStopsRelay))
    {
        std::cerr << "FATAL: --deadBatteryNoDraw needs --batteryDepleteJ > 0 (nothing dies "
                     "without it) and --silenceStopsRelay (a dead radio does not relay)\n";
        return 1;
    }
    // N.6.
    if (spec.halfDuplex > 2)
    {
        std::cerr << "FATAL: --halfDuplex is 0 (off), 1 (count only) or 2 (enforce)\n";
        return 1;
    }
    // N.1. Nothing is ever silenced without one of the two faults.
    if (spec.silenceStopsRelay && !(batteryDepleteJArg > 0.0) && injectSilentIdArg == 0)
    {
        std::cerr << "FATAL: --silenceStopsRelay needs --batteryDepleteJ > 0 or "
                     "--injectSilentId: without them no radio is ever silenced\n";
        return 1;
    }
    // M.4. Nothing to digest without the visualiser's files.
    if (digestVizOutputsArg && !vizEventsArg)
    {
        std::cerr << "FATAL: --digestVizOutputs needs --vizEvents: without it no visualiser "
                     "output is written\n";
        return 1;
    }
    // N.21 strict. Applied by the preset while the user turned --finderPseudoAnchor off
    // explicitly: unused, not refused (as the preset's other dependent settings).
    if (spec.finderPseudoAnchorStrict && specProfileOn &&
        !explicitFlags.count("finderPseudoAnchorStrict") && !finderPseudoAnchorArg)
    {
        spec.finderPseudoAnchorStrict = false;
    }
    if (spec.finderPseudoAnchorStrict && !finderPseudoAnchorArg)
    {
        std::cerr << "FATAL: --finderPseudoAnchorStrict needs --finderPseudoAnchor: it removes "
                     "the true-position fallback of the pseudo-anchor substitution\n";
        return 1;
    }
    // N.21. Applied by the preset while the user turned a prerequisite off explicitly:
    // unused, not refused (as the preset's other dependent settings).
    if (spec.subjectFinderPseudoAnchor && specProfileOn &&
        !explicitFlags.count("subjectFinderPseudoAnchor") &&
        (!finderPseudoAnchorArg || spec.measuredSubjects < 2))
    {
        spec.subjectFinderPseudoAnchor = false;
    }
    if (spec.subjectFinderPseudoAnchor && (!finderPseudoAnchorArg || spec.measuredSubjects < 2))
    {
        std::cerr << "FATAL: --subjectFinderPseudoAnchor needs --finderPseudoAnchor and "
                     "--measuredSubjects > 1: it gives subjects tag 0's finder treatment\n";
        return 1;
    }
    // N.3 / row 55.
    if (spec.subjectBatteryDeplete && (!(batteryDepleteJArg > 0.0) || spec.measuredSubjects < 2))
    {
        std::cerr << "FATAL: --subjectBatteryDeplete needs --batteryDepleteJ > 0 and "
                     "--measuredSubjects > 1: it extends tag 0's battery fault to the subjects\n";
        return 1;
    }
    if (spec.subjectTriggeredUpdates && (!spec.triggeredUpdates || spec.measuredSubjects < 2))
    {
        std::cerr << "FATAL: --subjectTriggeredUpdates needs --triggeredUpdates and "
                     "--measuredSubjects > 1: it extends tag 0's large correction to the subjects\n";
        return 1;
    }
    if (!(spec.triggeredUpdateK > 0.0) ||
        (spec.triggeredUpdateK != 1.0 && !spec.triggeredUpdates))
    {
        std::cerr << "FATAL: --triggeredUpdateK must be > 0 and needs --triggeredUpdates=true\n";
        return 1;
    }
    // MC-1. Every fix acts on a frame, and without --sendHandshakeFrames there is none.
    if (spec.handshakeFixes && !sendHandshakeFramesArg)
    {
        std::cerr << "FATAL: --handshakeFixes needs --sendHandshakeFrames=true: without it "
                  << "the handshake is only billed and there is no frame to fix\n";
        return 1;
    }
    // MS-1.
    if (spec.measuredSubjects < 1 || spec.measuredSubjects > nTagsArg)
    {
        std::cerr << "FATAL: --measuredSubjects=" << spec.measuredSubjects << " must be 1.."
                  << nTagsArg << " (--nTags): a subject is a field node\n";
        return 1;
    }
    if (spec.measuredSubjects > 1)
    {
        std::vector<std::string> only;
        if (tier1TdoaArg)
        {
            only.push_back("--tier1Tdoa");
        }
        if (nlosClassifierArg)
        {
            only.push_back("--nlosClassifier");
        }
        if (imuCrossCheckArg || imuFusionArg)
        {
            only.push_back("--imuCrossCheck/--imuFusion (SHS)");
        }
        if (sessionLayerArg)
        {
            only.push_back("--sessionLayer");
        }
        if (spec.triggeredUpdates && !spec.subjectTriggeredUpdates)
        {
            only.push_back("--triggeredUpdates");
        }
        else if (spec.triggeredUpdates && batteryDepleteJArg > 0.0 && !spec.subjectBatteryDeplete)
        {
            only.push_back("--triggeredUpdates' last gasp");
        }
        if (spec.edgeUplinkRanges && !spec.subjectEdgeUplink)
        {
            only.push_back("--edgeUplinkRanges");
        }
        if (batteryDepleteJArg > 0.0 && !spec.subjectBatteryDeplete)
        {
            only.push_back("--batteryDepleteJ");
        }
        if (spec.entryPrediction || spec.entryVelocity)
        {
            only.push_back("--entryVelocity/--entryPrediction");
        }
        if (spec.imuWitnessSource == "model")
        {
            only.push_back("--imuWitnessSource=model");
        }
        if (!only.empty())
        {
            std::cerr << "WARNING: --measuredSubjects > 1 runs these for subject 1 (tag 0) only:";
            for (size_t k = 0; k < only.size(); ++k)
            {
                std::cerr << " " << only[k];
            }
            std::cerr << "\n";
        }
    }
    if (!spec.finderTasking.empty() && spec.finderTasking != "tag0" &&
        spec.finderTasking != "roundrobin")
    {
        std::cerr << "FATAL: --finderTasking must be tag0 or roundrobin, got '"
                  << spec.finderTasking << "'\n";
        return 1;
    }
    if (spec.finderTasking == "roundrobin" &&
        (spec.measuredSubjects < 2 || nFindersArg == 0 ||
         (finderModeArg != 1 && finderModeArg != 2)))
    {
        std::cerr << "FATAL: --finderTasking=roundrobin needs --measuredSubjects > 1, "
                     "--nFinders > 0 and --finderMode 1 or 2: otherwise every finder already "
                     "searches for tag 0, or none steers on an entry\n";
        return 1;
    }
    // NW-9.
    if (!identityQueryManifestsArg.empty())
    {
        std::stringstream ss(identityQueryManifestsArg);
        std::string tok;
        while (std::getline(ss, tok, ','))
        {
            char* end = nullptr;
            const long v = std::strtol(tok.c_str(), &end, 10);
            if (tok.empty() || end == nullptr || *end != '\0' || v < 1 || v > 255)
            {
                std::cerr << "FATAL: --identityQueryManifests takes ids 1..255, got '"
                          << identityQueryManifestsArg << "'\n";
                return 1;
            }
            spec.identityQueryManifests.push_back(static_cast<uint8_t>(v));
        }
    }
    // TA-1a.
    if (spec.sessionOverMesh && (!sessionLayerArg || !spec.queryV2))
    {
        std::cerr << "FATAL: --sessionOverMesh needs --sessionLayer and --queryV2: there is no "
                     "session, or no RESPONSE carries an entry\n";
        return 1;
    }
    if (spec.finderSessions && (!spec.sessionOverMesh || nFindersArg == 0))
    {
        std::cerr << "FATAL: --finderSessions needs --sessionOverMesh and --nFinders > 0\n";
        return 1;
    }
    if (spec.requesterEscalation &&
        (!spec.sessionOverMesh || !spec.arbitrationV2 || !spec.grantGatedUwb))
    {
        std::cerr << "FATAL: --requesterEscalation needs --sessionOverMesh, --arbitrationV2 and "
                     "--grantGatedUwb: there is no requester, no arbiter to score it, or no "
                     "node that waits for a grant\n";
        return 1;
    }
    if (!(spec.finderCancelM > 0.0) || (spec.finderCancelM != 2.0 && !spec.finderSessions))
    {
        std::cerr << "FATAL: --finderCancelM must be > 0, and is read only under "
                     "--finderSessions\n";
        return 1;
    }
    if (spec.sessionPreciseOnGrant && (!spec.sessionOverMesh || !spec.grantGatedUwb))
    {
        std::cerr << "FATAL: --sessionPreciseOnGrant needs --sessionOverMesh and "
                     "--grantGatedUwb: only then does an entry name a confirmed grant\n";
        return 1;
    }
    if ((spec.sessionStalenessS != 120.0 || spec.sessionTimeoutS != 600.0) &&
        !spec.sessionOverMesh)
    {
        std::cerr << "FATAL: --sessionStalenessS and --sessionTimeoutS are read only under "
                     "--sessionOverMesh\n";
        return 1;
    }
    // N.15. Applied by the preset while the user turned the chain off explicitly (as
    // CAMP-SPEC's single-hop cells do): unused, not refused, as the preset's routing weights.
    if (spec.subjectRelayChain && specProfileOn && !explicitFlags.count("subjectRelayChain") &&
        (!relayChainArg || spec.measuredSubjects < 2))
    {
        spec.subjectRelayChain = false;
    }
    if (spec.subjectRelayChain && (!relayChainArg || spec.measuredSubjects < 2))
    {
        std::cerr << "FATAL: --subjectRelayChain needs --relayChain and --measuredSubjects > 1: "
                     "it puts measured subjects in the relay chain\n";
        return 1;
    }
    // NW-1 step 3.
    if (spec.hopVarianceTerms && (!relayedRangingArg || !relayChainArg || !nlosClassifierArg))
    {
        std::cerr << "FATAL: --hopVarianceTerms needs --relayedRanging, --relayChain (whose log "
                     "carries the terms) and --nlosClassifier (whose threshold says NLOS)\n";
        return 1;
    }
    // NW-1.
    if (spec.hopProvenance && !relayedRangingArg)
    {
        std::cerr << "FATAL: --hopProvenance needs --relayedRanging: without relays every "
                     "range is direct and every count is 0\n";
        return 1;
    }
    // NW-6.
    if (spec.gossipQueue && (!(anchorGrantGossipSArg > 0.0) || !spec.arbitrationV2))
    {
        std::cerr << "FATAL: --gossipQueue needs --anchorGrantGossipS > 0 and --arbitrationV2: "
                     "there is no gossip to ride, or no queue to gossip\n";
        return 1;
    }
    // NW-4 / NW-5.
    if (spec.queryTriggeredEscalation && (!spec.queryV2 || !spec.identityQueryUrgent))
    {
        std::cerr << "FATAL: --queryTriggeredEscalation needs --queryV2 and "
                     "--identityQueryUrgent: only a v2 QUERY carries a priority, and nothing "
                     "else sends an URGENT one yet\n";
        return 1;
    }
    if (spec.identityQueryUrgent && !spec.queryV2)
    {
        std::cerr << "FATAL: --identityQueryUrgent needs --queryV2\n";
        return 1;
    }
    if (spec.arbitrationV2 && (!uwbArbitrationArg || !spec.handshakeFixes))
    {
        std::cerr << "FATAL: --arbitrationV2 needs --uwbArbitration=true and "
                     "--handshakeFixes=true: it is an arbiter, and its denials and queue "
                     "positions ride MC-1's verdict\n";
        return 1;
    }
    if (spec.queryV2 && !(identityQueryPeriodSArg > 0.0))
    {
        std::cerr << "FATAL: --queryV2 needs --identityQueryPeriodS > 0: no query is sent\n";
        return 1;
    }
    if (!spec.identityQueryManifests.empty() && !spec.queryV2)
    {
        std::cerr << "FATAL: --identityQueryManifests needs --queryV2: only a v2 QUERY "
                     "carries a list\n";
        return 1;
    }
    // NW-7.
    if (spec.grantGatedUwb && !spec.handshakeFixes)
    {
        std::cerr << "FATAL: --grantGatedUwb needs --handshakeFixes=true: only then does a "
                     "GRANT carry its verdict and name the request it answers\n";
        return 1;
    }
    // NW-8.
    if (spec.reversePathHop && (!reversePathArg || !(identityQueryPeriodSArg > 0.0)))
    {
        std::cerr << "FATAL: --reversePathHop needs --reversePath=true and "
                     "--identityQueryPeriodS > 0: without them no QUERY is cached or no "
                     "RESPONSE is sent\n";
        return 1;
    }
    // NW-0.
    if (!spec.meshLinks.empty() && spec.meshLinks != "star" && spec.meshLinks != "all")
    {
        std::cerr << "FATAL: --meshLinks must be star or all, got '" << spec.meshLinks << "'\n";
        return 1;
    }
    // DC-9. The model witness needs the synthetic IMU and the witness itself.
    if (!spec.imuWitnessSource.empty() && spec.imuWitnessSource != "shs" &&
        spec.imuWitnessSource != "model")
    {
        std::cerr << "FATAL: --imuWitnessSource must be shs or model, got '"
                  << spec.imuWitnessSource << "'\n";
        return 1;
    }
    if (spec.imuWitnessSource == "model" && (!spec.imuPredict || !imuCrossCheckArg))
    {
        std::cerr << "FATAL: --imuWitnessSource=model needs --imuPredict=true (the track) and "
                  << "--imuCrossCheck=true (the witness)\n";
        return 1;
    }
    // DC-13. Without DC-10's velocities every entry would be carried forward by zero.
    if (spec.entryPrediction && !spec.entryVelocity)
    {
        std::cerr << "FATAL: --entryPrediction needs --entryVelocity=true: without it every "
                  << "entry's velocity is 0 and the prediction is the legacy path\n";
        return 1;
    }
    // DC-4. Case D runs in Step()'s restructured sparse path, which --sparseInStep turns on.
    if (spec.caseD && !spec.sparseInStep)
    {
        std::cerr << "FATAL: --caseD needs --sparseInStep=true: Case D is one of the sparse "
                  << "cases Step() resolves after the prediction\n";
        return 1;
    }
    if (spec.caseD && !spec.imuPredict)
    {
        std::cerr << "WARNING: --caseD without --imuPredict: the single range constrains a "
                  << "constant-velocity prediction, not an IMU-propagated one\n";
    }
    // TA-1b. Applied by the preset while the user turned a prerequisite off explicitly:
    // unused, not refused (as the preset's routing weights and --subjectRelayChain).
    if (spec.finderRequesterEscalation && specProfileOn &&
        !explicitFlags.count("finderRequesterEscalation") &&
        (!spec.finderSessions || !spec.requesterEscalation))
    {
        spec.finderRequesterEscalation = false;
    }
    if (spec.finderRequesterEscalation && (!spec.finderSessions || !spec.requesterEscalation))
    {
        std::cerr << "FATAL: --finderRequesterEscalation needs --finderSessions (whose results "
                     "drive it) and --requesterEscalation (its rule and arbiters)\n";
        return 1;
    }
    // N.14. The union's one- and two-range sets are solved by Step()'s sparse cases.
    if (spec.fuseFloorActive && !spec.sparseInStep)
    {
        std::cerr << "FATAL: --fuseFloorActive needs --sparseInStep: the sparse cases on the "
                     "fused set run inside Step()\n";
        return 1;
    }
    // DC-11. --sparseInStep replaces --sparseCaseA's pre-Step solve; both would solve twice.
    if (spec.sparseInStep && sparseCaseAArg)
    {
        std::cerr << "FATAL: --sparseInStep replaces --sparseCaseA's solve before Step(); "
                  << "give one or the other\n";
        return 1;
    }
    // SY-5. The faults ride the seeded window; without one they would never act.
    if (!(spec.injectTxPowerDropDb >= 0.0) || !(spec.injectAntennaLossDb >= 0.0) ||
        ((spec.injectTxPowerDropDb > 0.0 || spec.injectAntennaLossDb > 0.0) &&
         !(injectBiasFraction > 0.0)))
    {
        std::cerr << "FATAL: --injectTxPowerDropDb and --injectAntennaLossDb take a value >= 0 "
                  << "and, when > 0, need --injectBiasFraction > 0: the fault acts over that "
                  << "window\n";
        return 1;
    }
    // SY-1. Without the edge estimator the node fuses its own ranges and nothing goes up.
    // N.3 / row 55. Applied by the preset while the user turned a prerequisite off
    // explicitly: unused, not refused (as the preset's other dependent settings).
    if (spec.subjectEdgeUplink && specProfileOn && !explicitFlags.count("subjectEdgeUplink") &&
        (!spec.edgeUplinkRanges || spec.measuredSubjects < 2))
    {
        spec.subjectEdgeUplink = false;
    }
    if (spec.subjectEdgeUplink && (!spec.edgeUplinkRanges || spec.measuredSubjects < 2))
    {
        std::cerr << "FATAL: --subjectEdgeUplink needs --edgeUplinkRanges and --measuredSubjects "
                     "> 1: it extends tag 0's uplink bill to the subjects\n";
        return 1;
    }
    if (spec.edgeUplinkRanges && !gatewayEdgeServerArg)
    {
        std::cerr << "FATAL: --edgeUplinkRanges needs --gatewayEdgeServer=true: with the "
                  << "estimator on the node no range travels to a gateway\n";
        return 1;
    }
    if (!spec.queryTierBinding.empty() && spec.queryTierBinding != "first" &&
        spec.queryTierBinding != "per-tier")
    {
        std::cerr << "FATAL: --queryTierBinding must be first or per-tier, got '"
                  << spec.queryTierBinding << "'\n";
        return 1;
    }
    // NW-3. Without queries there is no driver to bind, and the flag would do nothing.
    if (!spec.queryTierBinding.empty() && !(identityQueryPeriodSArg > 0.0))
    {
        std::cerr << "FATAL: --queryTierBinding needs --identityQueryPeriodS > 0: without "
                  << "queries there is no driver to bind\n";
        return 1;
    }
    if (pcsNlosClassifierArg && pcsForm == CA_TBL_PCS_V2)
    {
        std::cerr << "FATAL: --pcsNlosClassifier has nothing to feed under --pcsForm=v2, which"
                  << " has no NLOS term\n";
        return 1;
    }
    if (!(nlosScoreThresholdArg > 0.0 && nlosScoreThresholdArg < 1.0))
    {
        std::cerr << "FATAL: --nlosScoreThreshold=" << nlosScoreThresholdArg
                  << " must lie in (0, 1)\n";
        return 1;
    }
    // C4 arbitration runs only where anchors receive ESC_REQUEST frames, and those exist
    // only under --sendHandshakeFrames (ca-tbl-mac.cc, the handshake block of DeliverRx).
    // Without them every arbitration counter is a structural zero -- a flag that reads as
    // on and does nothing, TECH_DEBT I.4's class. Refused rather than run (2026-09-30;
    // no tracked results/ sidecar uses --uwbArbitration).
    if (uwbArbitrationArg && !sendHandshakeFramesArg)
    {
        std::cerr << "FATAL: --uwbArbitration arbitrates ESC_REQUEST frames, which exist only"
                  << " under --sendHandshakeFrames=true\n";
        return 1;
    }
    // --imuFusion's own help text says it needs --shsTrustedFixK (and --shsSnapPosition's
    // says the two are only correct together); nothing checked it. A warning, not a
    // refusal: runs that used the combination exist, and --imuIdeal fuses the ideal
    // reckoner rather than the SHS, where neither applies.
    if (imuFusionArg && !imuIdealArg && (!(shsTrustedFixKArg > 0.0) || !shsSnapPositionArg))
    {
        std::cerr << "WARNING: --imuFusion without --shsTrustedFixK > 0 and --shsSnapPosition"
                  << " fuses an SHS calibrated on noise (TECH_DEBT K.9)\n";
    }

    // Run identifier, written as a SIDECAR rather than a CSV column. A column would
    // change arm_summary.csv and break byte-identity against results/baseline_v3,
    // which is the acceptance test protecting every commit in the NLOS chain.
    //
    // Identifies the INVOCATION, not the instant: hash of the full command line plus
    // the build stamp. No wall-clock read and no pid, deliberately -- this project
    // has already had one defect where a clock value reached a branch and shifted
    // the RNG stream whenever the OS preempted the thread.
    //
    // Two identical invocations share an id, which is correct: their outputs are
    // byte-identical, so mixing them is harmless. A --trace=false run has a
    // different command line and therefore a different id, which is exactly the
    // staleness case of v3 section 3.10 -- the one mtime only partially covers,
    // because a run-id survives cp and touch.
    {
        std::ostringstream inv;
        inv << "build " << __DATE__ << " " << __TIME__ << " |";
        for (int i = 0; i < argc; ++i)
        {
            inv << ' ' << argv[i];
        }
        const std::string invStr = inv.str();
        std::ostringstream rid;
        rid << std::hex << std::setw(16) << std::setfill('0')
            << std::hash<std::string>{}(invStr);
        // Source provenance. run_id previously hashed only the build stamp and argv,
        // which cannot distinguish a working-tree build from a committed one -- the
        // gap that hid remediation item 7 sitting uncommitted while the binary
        // already contained it. Recording the commit and a dirty count closes it.
        //
        // popen runs once at startup, before any RNG draw, and its result reaches no
        // branch that affects the simulation -- it is written to the sidecar and
        // nothing reads it back. That distinction matters here: this project has one
        // recorded defect where a value read at runtime reached a branch.
        auto shell = [](const char* cmd) {
            std::string out;
            FILE* fp = popen(cmd, "r");
            if (!fp)
            {
                return std::string("unknown");
            }
            char buf[256];
            while (fgets(buf, sizeof(buf), fp))
            {
                out += buf;
            }
            pclose(fp);
            while (!out.empty() && (out.back() == '\n' || out.back() == ' '))
            {
                out.pop_back();
            }
            return out.empty() ? std::string("unknown") : out;
        };
        const std::string head = shell("git rev-parse --short HEAD 2>/dev/null");
        // --untracked-files=no, and scoped to the directories that can affect a
        // build. The unscoped form reported dirty 430 on a tree with 4 tracked
        // modifications, because this repository carries hundreds of untracked
        // documents and result CSVs -- none of which can change the binary. A flag
        // that is always non-zero is a flag nobody reads.
        const std::string dirty = shell(
            "git status --porcelain --untracked-files=no -- src scratch 2>/dev/null | wc -l");

        std::ofstream ridFile((outdir + "/run_id.txt").c_str());
        // The guard used to be `if (ridFile)` with no else: on failure the sidecar was
        // silently absent, and the one verifier gate covering that path warns with a
        // pre-written excuse ("produced before the sidecar existed"), so absence was
        // indistinguishable from a legitimately old directory. Provenance that can
        // vanish without saying so is not provenance.
        if (!ridFile)
        {
            std::cerr << "FATAL: cannot open " << outdir
                      << "/run_id.txt for writing -- does --outdir exist?\n";
            return 1;
        }
        // The build profile decides whether NS_ASSERT and NS_LOG exist at all, so
        // two runs of the same source can behave differently and nothing in the
        // sidecar said which one this was. It is derivable from argv[0]'s
        // "-optimized" suffix, but derived-from-a-path is not the same as recorded,
        // and the README instructs a specific profile that the manifest now checks
        // this field against.
        const char* profile =
#if defined(NS3_BUILD_PROFILE_DEBUG)
            "debug";
#elif defined(NS3_BUILD_PROFILE_RELEASE)
            "release";
#elif defined(NS3_BUILD_PROFILE_OPTIMIZED)
            "optimized";
#else
            "unknown";
#endif
        {
            ridFile << rid.str() << "\n"
                    << invStr << "\n"
                    << "source " << head << " dirty " << dirty << " profile " << profile
                    << "\n";
        }
        ridFile.close();
        if (!ridFile)
        {
            std::cerr << "FATAL: run_id.txt failed to flush in " << outdir << "\n";
            return 1;
        }
    }

    CaTblSuspectRecovery suspectRecovery = CA_TBL_RECOVERY_NONE;
    if (suspectRecoveryArg == "inflate") { suspectRecovery = CA_TBL_RECOVERY_INFLATE; }
    else if (suspectRecoveryArg == "bounded") { suspectRecovery = CA_TBL_RECOVERY_BOUNDED; }
    else if (suspectRecoveryArg == "reinit") { suspectRecovery = CA_TBL_RECOVERY_REINIT; }
    else if (suspectRecoveryArg == "downweight") { suspectRecovery = CA_TBL_RECOVERY_DOWNWEIGHT; }
    else if (suspectRecoveryArg != "none")
    {
        std::cerr << "FATAL: --suspectRecovery=" << suspectRecoveryArg
                  << " is not one of none|inflate|bounded|reinit|downweight\n";
        return 1;
    }
    // DC-7. A PCS factor outside downweight would do nothing (TECH_DEBT I.4's class), and
    // one outside (0, 1] would not be a downweight.
    if (!std::isnan(spec.suspectPcsFactor)) // given, including +-inf
    {
        if (suspectRecovery != CA_TBL_RECOVERY_DOWNWEIGHT)
        {
            std::cerr << "FATAL: --suspectPcsFactor is read only under "
                      << "--suspectRecovery=downweight\n";
            return 1;
        }
        if (!(spec.suspectPcsFactor > 0.0 && spec.suspectPcsFactor <= 1.0))
        {
            std::cerr << "FATAL: --suspectPcsFactor must lie in (0, 1], got "
                      << spec.suspectPcsFactor << "\n";
            return 1;
        }
    }

    // Resolve the sentinels BEFORE the casts, so the value that reaches the
    // physics is also the value the sidecar prints. 255 is not an
    // EnvironmentClass and must never reach the cast.
    if (envFacilityInt == 255)
    {
        envFacilityInt = envInt;
    }
    if (vegFacilityInt == 255)
    {
        vegFacilityInt = vegWideInt;
    }
    const EnvironmentClass env = static_cast<EnvironmentClass>(envInt);
    const EnvironmentClass envFacility = static_cast<EnvironmentClass>(envFacilityInt);
    const VegetationType vegWide = static_cast<VegetationType>(vegWideInt);
    const VegetationType vegFacility = static_cast<VegetationType>(vegFacilityInt);

    TierPhy phy[CA_TBL_TIER_COUNT];
    phy[CA_TBL_TIER_868] = {7, 125e3, 14.0};
    phy[CA_TBL_TIER_2P4] = {7, 800e3, 12.5};
    // SY-3. Physical 1.1: "configurable spreading factor (SF7-SF12) and bandwidth
    // (125/250/500 kHz)". Applied only when a flag was given, so the defaults above stay
    // the literal values every result used.
    if (sf868Arg != 0 || bw868Arg != 0.0 || sf2p4Arg != 0 || bw2p4Arg != 0.0)
    {
        g_sy3Given = true;
        if (sf868Arg != 0 && (sf868Arg < 7 || sf868Arg > 12))
        {
            std::cerr << "FATAL: --sf868=" << sf868Arg << " is outside SF7-SF12, the range "
                      << "of the 868 MHz sensitivity table (and of Physical 1.1)\n";
            return 1;
        }
        if (bw868Arg != 0.0 && bw868Arg != 125e3 && bw868Arg != 250e3 && bw868Arg != 500e3)
        {
            std::cerr << "FATAL: --bw868=" << bw868Arg << " must be 125000, 250000 or 500000 Hz "
                      << "(Physical 1.1)\n";
            return 1;
        }
        if (sf2p4Arg != 0 && (sf2p4Arg < 5 || sf2p4Arg > 12))
        {
            std::cerr << "FATAL: --sf2p4=" << sf2p4Arg << " is outside SF5-SF12, the range of "
                      << "the 2.4 GHz sensitivity table\n";
            return 1;
        }
        if (bw2p4Arg != 0.0 && bw2p4Arg != 203125.0 && bw2p4Arg != 406250.0 &&
            bw2p4Arg != 800e3 && bw2p4Arg != 812500.0 && bw2p4Arg != 1625000.0)
        {
            std::cerr << "FATAL: --bw2p4=" << bw2p4Arg << " must be one of the SX1280 LoRa "
                      << "bandwidths 203125, 406250, 812500, 1625000 Hz, or 800000 (the "
                      << "default)\n";
            return 1;
        }
        if (sf868Arg != 0)
        {
            phy[CA_TBL_TIER_868].sf = static_cast<uint8_t>(sf868Arg);
        }
        if (bw868Arg != 0.0)
        {
            phy[CA_TBL_TIER_868].bandwidthHz = bw868Arg;
            g_sensShiftDb868 = 10.0 * std::log10(bw868Arg / 125e3);
        }
        if (sf2p4Arg != 0)
        {
            phy[CA_TBL_TIER_2P4].sf = static_cast<uint8_t>(sf2p4Arg);
        }
        if (bw2p4Arg != 0.0)
        {
            phy[CA_TBL_TIER_2P4].bandwidthHz = bw2p4Arg;
            g_sensShiftDb2p4 = 10.0 * std::log10(bw2p4Arg / 800e3);
        }
        std::ostringstream line;
        line << "  SY-3 PHY: 868 MHz SF" << int(phy[CA_TBL_TIER_868].sf) << " BW "
             << std::setprecision(9) << phy[CA_TBL_TIER_868].bandwidthHz
             << " Hz sensitivity offset " << g_sensShiftDb868 << " dB DISCOVERY time on air "
             << CaTblEnergyModel::LoRaTimeOnAirS(phy[CA_TBL_TIER_868].sf,
                                                 phy[CA_TBL_TIER_868].bandwidthHz,
                                                 CA_TBL_MAC_HEADER_BYTES +
                                                     CaTblPayloadBytes(CA_TBL_MSG_DISCOVERY, 1),
                                                 1)
             << " s; 2.4 GHz SF" << int(phy[CA_TBL_TIER_2P4].sf) << " BW "
             << phy[CA_TBL_TIER_2P4].bandwidthHz << " Hz sensitivity offset "
             << g_sensShiftDb2p4 << " dB DISCOVERY time on air "
             << CaTblEnergyModel::LoRaTimeOnAirS(phy[CA_TBL_TIER_2P4].sf,
                                                 phy[CA_TBL_TIER_2P4].bandwidthHz,
                                                 CA_TBL_MAC_HEADER_BYTES +
                                                     CaTblPayloadBytes(CA_TBL_MSG_DISCOVERY, 1),
                                                 1)
             << " s\n";
        std::cout << line.str();
    }
    phy[CA_TBL_TIER_UWB] = {7, 499.2e6, -16.47};
    // C5_INSTANT. The round's slots are disjoint in the registry only while the relay
    // tier's airtime is shorter than a slot; the engines register 2^sf / bw * 20 s
    // (lora-*-physics-engine.cc:~560), recomputed here from the same table.
    if (relaySlotsArg)
    {
        const TierPhy& rp = phy[std::min<uint32_t>(CA_TBL_TIER_COUNT - 1, relayTierArg)];
        const double airS = std::pow(2.0, rp.sf) / rp.bandwidthHz * 20.0;
        if (!(airS < FixRouter::kSlotS))
        {
            std::cerr << "FATAL: --relaySlots slot of " << FixRouter::kSlotS * 1e3
                      << " ms is not longer than the relay tier's airtime " << airS * 1e3
                      << " ms; its exchanges would overlap" << relaySlotsWhy << "\n";
            return 1;
        }
    }

    if (probe)
    {
        RunProbe(env, envFacility, seedStart, vegWide, vegFacility, phy, outdir, traceDiagnostic);
        return AppendOutputDigest(outdir, "probe_range_sweep.csv") ? 0 : 1;
    }

    if (uwbAnchorsPerSite < 3 || t2AnchorsPerSite < 3)
    {
        std::cout << "ERROR: at least 3 anchors per site are needed for a 2D fix.\n";
        return 1;
    }

    Deployment dep = BuildDeployment(gwGrid,
                                           boxHalfM,
                                           uwbTriadRadiusM,
                                           t2TriadRadiusM,
                                           uwbAnchorsPerSite,
                                           t2AnchorsPerSite,
                                           sitingModeArg,
                                           topologyArg,
                                           nGatewaysArg,
                                           gwRingFracArg,
                                     gwRidgeArg,
                                     gwRidgeYArg,
                                     gwArcDegArg,
                                     gwArcCentreDegArg);

    // BASE CAMP. Move the infrastructure to a corner and leave the casualty's tour
    // where it is, so the subject is OUTSIDE the gateway footprint for much of the
    // run and reaching it needs a searcher to walk out. Applied here rather than
    // inside BuildDeployment so every arm sees the identical deployment and the
    // legacy topology path is untouched.
    if (baseCampFrac > 0.0)
    {
        const double bcx = baseCampFrac * boxHalfM;
        const double bcy = baseCampFrac * boxHalfM;
        // A RIDGE IS ALREADY A DEPLOYMENT, so do not translate it. The corner-shift above
        // exists to put the subject outside a CENTRED footprint; a ridge is deliberately
        // placed and shifting it by three quarters of the box lands it off the map and
        // destroys the geometry the ridge scenario is about. In ridge mode the base camp
        // is what it actually is -- the trailhead the searchers set out FROM -- and the
        // gateways stay where they were put.
        if (!gwRidgeArg)
        {
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                for (size_t i = 0; i < dep.anchors[ti].size(); ++i)
                {
                    dep.anchors[ti][i].pos.x += bcx;
                    dep.anchors[ti][i].pos.y += bcy;
                }
            }
        }
        dep.hasBaseCamp = true;
        if (gwRidgeArg)
        {
            // The trailhead: west end of the ridge, on the trail the subject was walking.
            dep.baseCampX = -boxHalfM * 0.8;
            dep.baseCampY = gwRidgeYArg * boxHalfM - boxHalfM * 0.08;
        }
        else
        {
            dep.baseCampX = bcx;
            dep.baseCampY = bcy;
        }

        // KEEP-OUT. Once the subject leaves the camp it should not wander back
        // into coverage: "lost" means out of reach of the infrastructure, and a
        // tour that keeps re-entering the footprint is the enclosure scenario
        // again. Sites are dropped by distance from the camp, farthest kept.
        if (subjectKeepOutM > 0.0)
        {
            std::vector<std::pair<double, Vector>> byDist;
            for (size_t i = 0; i < dep.facilities.size(); ++i)
            {
                const double dx = dep.facilities[i].x - bcx;
                const double dy = dep.facilities[i].y - bcy;
                byDist.push_back({std::sqrt(dx * dx + dy * dy), dep.facilities[i]});
            }
            std::sort(byDist.begin(), byDist.end(),
                      [](const std::pair<double, Vector>& a,
                         const std::pair<double, Vector>& b) {
                          return a.first > b.first;
                      });
            std::vector<Vector> kept;
            for (size_t i = 0; i < byDist.size(); ++i)
            {
                // A walker with fewer than two targets has no tour, so the two
                // farthest are kept whatever the radius asks for.
                if (byDist[i].first >= subjectKeepOutM || kept.size() < 2)
                {
                    kept.push_back(byDist[i].second);
                }
            }
            std::cout << "  keep-out " << subjectKeepOutM << " m: "
                      << kept.size() << " of " << dep.facilities.size()
                      << " tour sites retained\n";
            dep.facilities = kept;
        }
    }

    // MS-2. A named placement shape replaces the gateways BuildDeployment placed, in every
    // tier list (topology 0: one tri-band node per gateway), keeping ids 101+. Applied
    // after the base-camp shift so the shapes' coordinates are absolute. The camp the
    // cluster and star grow from is the base camp when there is one, otherwise the west
    // midpoint of the frac square -- the trailhead side every arc run centres on (OURS).
    if (gwLayoutArg != "legacy")
    {
        if (topologyArg != 0)
        {
            std::cerr << "FATAL: --gwLayout needs --topology=0: the legacy topology's "
                      << "per-tier anchor rings are not gateways\n";
            return 1;
        }
        if (gwLayoutArg != "file" && nGatewaysArg == 0)
        {
            std::cerr << "FATAL: --gwLayout=" << gwLayoutArg << " needs --nGateways > 0\n";
            return 1;
        }
        if (!(gwLayoutFracArg > 0.0 && gwLayoutFracArg <= 1.0) || !(gwClusterRadiusMArg > 0.0) ||
            gwStarArmsArg == 0 || !(gwStarSpacingMArg > 0.0))
        {
            std::cerr << "FATAL: --gwLayoutFrac must be in (0, 1], --gwClusterRadiusM and "
                      << "--gwStarSpacingM > 0, --gwStarArms >= 1\n";
            return 1;
        }
        const double campX = dep.hasBaseCamp ? dep.baseCampX : -gwLayoutFracArg * boxHalfM;
        const double campY = dep.hasBaseCamp ? dep.baseCampY : 0.0;
        std::string why;
        const std::vector<Vector> pts =
            CaTblGatewayLayout(gwLayoutArg, nGatewaysArg, boxHalfM, gwLayoutFracArg, campX, campY,
                               gwClusterRadiusMArg, gwStarArmsArg, gwStarSpacingMArg,
                               gwLayoutFileArg, why);
        if (pts.empty())
        {
            std::cerr << "FATAL: " << why << "\n";
            return 1;
        }
        std::ostringstream line; // its own stream: cout's precision is left alone
        line << "  MS-2 layout " << gwLayoutArg << ": " << pts.size() << " gateways, camp ("
             << std::setprecision(9) << campX << ", " << campY << "):";
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            dep.anchors[ti].clear();
            for (size_t i = 0; i < pts.size(); ++i)
            {
                dep.anchors[ti].push_back({static_cast<uint32_t>(101 + i), pts[i]});
            }
        }
        for (size_t i = 0; i < pts.size(); ++i)
        {
            line << " " << (101 + i) << " " << pts[i].x << " " << pts[i].y;
        }
        std::cout << line.str() << "\n";
    }

    if (vizEventsArg)
    {
        // Geometry the animation needs and the trace does not carry. Written once, in
        // main, because the deployment is identical for every arm -- writing it per arm
        // would invite someone to believe otherwise.
        std::ofstream dj((outdir + "/deployment.json").c_str());
        if (dj.is_open())
        {
            dj << "{\n";
            dj << "  \"boxHalfM\": " << boxHalfM << ",\n";
            dj << "  \"dwellRadiusM\": " << dep.dwellRadiusM << ",\n";
            dj << "  \"envWide\": " << static_cast<int>(env) << ",\n";
            dj << "  \"envFacility\": " << static_cast<int>(envFacility) << ",\n";
            dj << "  \"facilities\": [";
            for (size_t i = 0; i < dep.facilities.size(); ++i)
            {
                dj << (i ? ", " : "") << "[" << dep.facilities[i].x << ", "
                   << dep.facilities[i].y << "]";
            }
            dj << "],\n";
            dj << "  \"anchors\": {\n";
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                dj << "    \"" << ti << "\": [";
                for (size_t i = 0; i < dep.anchors[ti].size(); ++i)
                {
                    dj << (i ? ", " : "") << "{\"id\": " << dep.anchors[ti][i].id
                       << ", \"x\": " << dep.anchors[ti][i].pos.x
                       << ", \"y\": " << dep.anchors[ti][i].pos.y << "}";
                }
                dj << "]" << (ti + 1 < CA_TBL_TIER_COUNT ? "," : "") << "\n";
            }
            dj << "  }\n}\n";
        }
    }

    const CaTblPolicyMode allArms[] = {CA_TBL_MODE_STATIC_868,
                                       CA_TBL_MODE_STATIC_2P4,
                                       CA_TBL_MODE_STATIC_UWB,
                                       CA_TBL_MODE_FIXED_THRESHOLD,
                                       CA_TBL_MODE_NO_INTEGRITY,
                                       CA_TBL_MODE_FULL};
    const int nAllArms = static_cast<int>(sizeof(allArms) / sizeof(allArms[0]));

    // ARM SELECTION. Safe because RunArm() calls ResetNextStreamIndex() before
    // building its engines, so an arm's channel realisation depends only on the
    // seed and not on which other arms ran before it -- that reset is the fix
    // documented at the top of RunArm(), and this filter is only sound because of
    // it. Verified empirically, not assumed: a filtered run's row for an arm is
    // byte-identical to the same arm's row in the full run.
    //
    // Exists for the operating-point sweep, where only fixed_threshold and
    // ca_tbl_no_integrity are needed and the four other arms -- static_uwb above
    // all, which polls a tier it cannot reach -- are most of the runtime.
    std::vector<CaTblPolicyMode> arms;
    if (armsArg.empty() || armsArg == "all")
    {
        arms.assign(allArms, allArms + nAllArms);
    }
    else
    {
        std::string rest = armsArg;
        while (!rest.empty())
        {
            const size_t comma = rest.find(',');
            const std::string name = rest.substr(0, comma);
            rest = (comma == std::string::npos) ? "" : rest.substr(comma + 1);
            bool found = false;
            for (int k = 0; k < nAllArms; ++k)
            {
                if (name == CaTblPolicyModeName(allArms[k]))
                {
                    arms.push_back(allArms[k]);
                    found = true;
                    break;
                }
            }
            if (!found && !name.empty())
            {
                std::cout << "ERROR: --arms: unknown arm '" << name << "'\n";
                return 1;
            }
        }
        if (arms.empty())
        {
            std::cout << "ERROR: --arms selected no arms\n";
            return 1;
        }
    }
    const int nArms = static_cast<int>(arms.size());

    const double beaconPeriodS[CA_TBL_TIER_COUNT] = {beacon868S, beacon2p4S, beaconUwbS};

    const std::string summaryPath = outdir + "/arm_summary.csv";
    // Accumulated across arms; announced on stderr after the loop if non-zero.
    uint64_t reinitFallbackTotal = 0;
    std::ofstream summary(summaryPath.c_str());
    // CHECKED, because it was not. --outdir "must already exist", and when it did
    // not, this stream failed to open, the run completed all 1440 cycles, the
    // program printed "Wrote <path>" from the unconditional line below and exited
    // 0 -- having written nothing at all. Found by the clone test, where
    // results/verify does not exist in a fresh checkout: RUN_EXIT=0, no assertion
    // fired, no file anywhere on the filesystem.
    //
    // A simulator that reports success while producing no output is the same
    // failure class as a check that passes because its command errored, and it is
    // worse, because a missing result is indistinguishable from a run never
    // launched.
    if (!summary)
    {
        std::cerr << "FATAL: cannot open " << summaryPath
                  << " for writing -- does the --outdir exist?\n";
        return 1;
    }
    // MS-1. subject_summary.csv, only when there is more than one subject.
    std::unique_ptr<std::ofstream> subjSummary;
    if (spec.measuredSubjects > 1)
    {
        subjSummary.reset(new std::ofstream((outdir + "/subject_summary.csv").c_str()));
        *subjSummary << "arm,seed,subjectId,steps,rmseM,cep50M,cep95M,maxErrM,fixCycles,"
                        "totalJ,j_868,j_2p4,j_uwb,cycles_868,cycles_2p4,cycles_uwb,tx_868,"
                        "tx_2p4,tx_uwb,sleep_868,sleep_2p4,sleep_uwb,escalations,"
                        "deescalations,noFixCycles,escRequests\n";
    }
    summary << "arm,seed,steps,durationS,rmseM,cep50M,cep95M,maxErrM,"
            << "rmseFixedM,cep50FixedM,fixCycles,totalJ,"
            << "j_868,j_2p4,j_uwb,cycles_868,cycles_2p4,cycles_uwb,"
            << "airtime_868,airtime_2p4,airtime_uwb,duty_868,"
            << "escalations,deescalations,suspects,noFixCycles,"
            << "handshakeDeferrals,handshakeDeferralDelayS,escalationsDeferredAway,handshakePreemptions,"
            << "trig_tp,trig_fp,trig_fn,trig_tn,trig_precision,trig_recall,trig_f1,"
            << "cyclesInUwbCoverage,accuracyPerJoule,"
            // Energy broken down by radio state, per tier. Emitted because the §6
            // blocking defect was not diagnosable from totalJ: "this radio is
            // expensive" lands in tx_*, "this tier is addressing too many anchors"
            // lands almost entirely in rx_*, and a single scalar cannot separate
            // them. It is also the check on whether the neighbour gate did
            // anything -- if rx_uwb has not fallen, it did not.
            << "tx_868,tx_2p4,tx_uwb,rx_868,rx_2p4,rx_uwb,"
            << "cad_868,cad_2p4,cad_uwb,idle_868,idle_2p4,idle_uwb,"
            << "sleep_868,sleep_2p4,sleep_uwb,"
            << "meanNbr_868,meanNbr_2p4,meanNbr_uwb,blindCycles,nbrInsertions,"
            << "mac_tx,mac_collisions,mac_captures,mac_deliveries,"
            // Rival systems, scored from the same measurements as the EKF columns
            // above. Appended rather than interleaved so every existing column
            // keeps its index and older readers still parse.
            << "rmse_mlat,cep95_mlat,rmse_wcl,cep95_wcl,"
            << "rmse_prox,cep95_prox,rmse_lkp,cep95_lkp,mlat_solved,"
            << "meanErrM,meanErrFixedM,tFirstFixS,tFirst100mS\n";

    std::cout << "CA-TBL causal simulation (S1 + S2 + S2.5)\n"
              << "  envWide=" << envInt << " vegWide=" << vegWideInt
              << "  envFacility=" << envFacilityInt << " vegFacility=" << vegFacilityInt << "\n"
              << "  duration=" << durationS << "s  dt=" << dt
              << "s  seeds=" << seedStart << ".." << (seedStart + seedCount - 1) << "\n"
              << "  deployment: box=" << (2.0 * boxHalfM) << "m  868 gateways="
              << dep.anchors[CA_TBL_TIER_868].size() << "  facilities=" << dep.facilities.size()
              << "  2p4 anchors=" << dep.anchors[CA_TBL_TIER_2P4].size()
              << "  uwb anchors=" << dep.anchors[CA_TBL_TIER_UWB].size() << "\n"
              << "  mobility: facility tour, " << walkSpeedMps << " m/s, dwell " << dwellS << "s\n\n";

    int multiStepMode = 0;
    if (multiStepDemote == "proposed")
    {
        multiStepMode = 1;
    }
    else if (multiStepDemote == "all")
    {
        multiStepMode = 2;
    }
    else if (multiStepDemote != "off")
    {
        std::cerr << "--multiStepDemote must be one of: off, proposed, all\n";
        return 1;
    }

    int fastDemoteMode = 0;
    if (fastDemote == "proposed")
    {
        fastDemoteMode = 1;
    }
    else if (fastDemote == "all")
    {
        fastDemoteMode = 2;
    }
    else if (fastDemote != "off")
    {
        std::cerr << "--fastDemote must be one of: off, proposed, all\n";
        return 1;
    }

    for (uint32_t s = 0; s < seedCount; ++s)
    {
        const uint32_t seed = seedStart + s;
        std::cout << "seed " << seed << "\n";
        std::cout << "  " << std::left << std::setw(22) << "arm" << std::right << std::setw(11)
                  << "rmse_m" << std::setw(11) << "rmseFix_m" << std::setw(11) << "cep50_m"
                  << std::setw(11) << "cep95_m" << std::setw(13) << "energy_J" << std::setw(8)
                  << "esc" << std::setw(8) << "susp" << std::setw(10) << "nofix" << std::setw(9)
                  << "trigF1" << std::setw(11) << "acc/J\n";

        for (int a = 0; a < nArms; ++a)
        {
            const ArmResult r = RunArm(arms[a],
                                       seed,
                                       durationS,
                                       dt,
                                       env,
                                       envFacility,
                                       vegWide,
                                       vegFacility,
                                       dep,
                                       phy,
                                       walkSpeedMps,
                                       dwellS,
                                       beaconPeriodS,
                                       nbrTtlMult,
                                       searchDutyFrac,
                                       outdir,
                                       writeTrace,
                                       fastDemoteMode,
                                       mahalUseR,
                                       suspectRecovery,
                                       suspectInflateFactor,
                                       suspectRecoveryN,
                                       escalationRequiresEvidence,
                                       escalationRequiresEvidenceT2Arg,
                                       t2PredictRadiusMArg,
                                       t2PredictMinAnchorsArg,
                                       uwbEvidenceFromNeighboursArg,
                                       multiStepMode,
                                       demoteStalenessS,
                                       nlosSource,
                                       suspectForcesEscalation,
                                       pcsForm,
                                       pcsW1Arg,
                                       pcsW2Arg,
                                       pcsW3Arg,
                                       pcsGateFloorArg,
                                       pcsHysteresisBandArg,
                                       nDeescalateArg,
                                       sendHandshakeFramesArg,
                                       deferEscalationOnDutyLimit,
                                       intraCycleHandshakeDeferArg,
                                       nTagsArg,
                                       relayedRangingArg,
                                       relayTierArg,
                                       maxRelayDepthArg,
                                       relayVarBudgetM2Arg,
                                       vizEventsArg,
                                       vizTablePeriodSArg,
                                       gwRidgeArg,
                                       gwRidgeYArg,
                                       gwArcDegArg,
                                       gwArcCentreDegArg,
                                       trailDeviateSideArg,
                                       trailDeviateAtSArg,
                                       trailStopAfterSArg,
                                       nFindersArg,
                                       finderModeArg,
                                       finderStaleSteerBoundSArg,
                                       searchLaunchOnLossSArg,
                                       searchLaunchSigmaMArg,
                                       searchLaunchAtSArg,
                                       maxTurnRateRadS,
                                       shsTrustedFixKArg,
                                       imuFusionArg,
                                       imuIdealArg,
                                       queryProxyArg,
                                       tableFuseCIArg,
                                       digestSummaryArg,
                                       batteryDepleteJArg,
                                       sessionLayerArg,
                                       mobilityFromRangeArg,
                                       lifecycleTrackingArg,
                                       batchSearchArg,
                                       sparseCaseAArg,
                                       sparseCaseAMarginMArg,
                                       injectSilentIdArg,
                                       injectSilentStartSArg,
                                       injectSilentEndSArg,
                                       queryStalenessBoundSArg,
                                       reversePathArg,
                                       shsStepBoundsArg,
                                       shsSnapPositionArg,
                                       (finderSpeedMpsArg >= 0.0)
                                           ? finderSpeedMpsArg
                                           : walkSpeedMps,
                                       finderDigestPeriodSArg,
                                       finderPosSigmaMArg,
                                       finderPseudoAnchorArg,
                                       bidirectionalDigestArg,
                                       tier1ChargeRepliesArg,
                                       gatewayEdgeServerArg,
                                       processNoiseStdPosArg,
                                       processNoiseStdVelArg,
                                       finderDispersalArg,
                                       uwbCoverageIncludesFindersArg,
                                       hopProvenanceFormArg,
                                       hopPenaltyArg,
                                       imuCrossCheckArg,
                                       imuDriftMpsArg,
                                       imuConsistencyKArg,
                                       imuSigmaFloorMArg,
                                       corroborationCheckArg,
                                       corroborationKArg,
                                       corroborationRangeSigmaMArg,
                                       identityQueryPeriodSArg,
                                       identityQueryTargetArg,
                                       uwbArbitrationArg,
                                       arbBatteryCapJArg,
                                       arbitrateDutyBudget,
                                       fixedVarThresholdM2,
                                       pcsSigmaTargetM,
                                       floorOnDemand,
                                       floorSkipK,
                                       floorHeartbeatN,
                                       injectRangeBiasM,
                                       injectBiasFraction,
                                       injectFaultAnchors,
                                       anchorGrantGossipSArg,
                                       relayChainArg,
                                       fixRoutingArg,
                                       fixRoutingConfidenceArg,
                                       tier1TdoaArg,
                                       tdoaPpsSigmaNsArg,
                                       nlosClassifierArg,
                                       pcsNlosClassifierArg,
                                       nlosScoreThresholdArg,
                                       relaySlotsArg,
                                       edgeDownlinkOnFixTierArg,
                                       spec);

            // The headline comparison metric. Higher is better: it rewards an
            // arm for being accurate AND cheap, which is exactly the trade the
            // architecture claims to win. RMSE alone always picks static_uwb;
            // joules alone always picks static_868. Neither is the claim.
            reinitFallbackTotal += r.reinitFallback;

            const double accPerJ =
                (r.rmseM > 0.0 && r.totalJ > 0.0) ? 1.0 / (r.rmseM * r.totalJ) : 0.0;

            const double prec =
                (r.tp + r.fp > 0) ? static_cast<double>(r.tp) / static_cast<double>(r.tp + r.fp) : 0.0;
            const double rec =
                (r.tp + r.fn > 0) ? static_cast<double>(r.tp) / static_cast<double>(r.tp + r.fn) : 0.0;
            const double f1 = (prec + rec > 0.0) ? 2.0 * prec * rec / (prec + rec) : 0.0;

            summary << r.arm << "," << r.seed << "," << r.steps << "," << std::fixed
                    << std::setprecision(4) << r.durationS << "," << r.rmseM << "," << r.cep50M
                    << "," << r.cep95M << "," << r.maxErrM << "," << r.rmseFixedM << ","
                    << r.cep50FixedM << "," << r.fixCycles << "," << std::setprecision(9)
                    << r.totalJ << "," << r.tierJ[0] << "," << r.tierJ[1] << "," << r.tierJ[2]
                    << "," << r.tierCycles[0] << "," << r.tierCycles[1] << "," << r.tierCycles[2]
                    << "," << r.airtimeS[0] << "," << r.airtimeS[1] << "," << r.airtimeS[2] << ","
                    << (r.durationS > 0.0 ? r.airtimeS[0] / r.durationS : 0.0) << ","
                    << r.escalations << "," << r.deescalations << "," << r.suspects << ","
                    << r.noFixCycles << "," << r.handshakeDeferrals << ","
                    << r.handshakeDeferralDelayS << "," << r.escalationsDeferredAway
                    << "," << r.handshakePreemptions << ","
                    << r.tp << "," << r.fp << "," << r.fn << "," << r.tn
                    << "," << std::setprecision(4) << prec << "," << rec << "," << f1 << ","
                    << r.cyclesInUwbCoverage << "," << std::setprecision(9) << accPerJ << ","
                    << r.txJ[0] << "," << r.txJ[1] << "," << r.txJ[2] << "," << r.rxJ[0] << ","
                    << r.rxJ[1] << "," << r.rxJ[2] << "," << r.cadJ[0] << "," << r.cadJ[1] << ","
                    << r.cadJ[2] << "," << r.idleJ[0] << "," << r.idleJ[1] << "," << r.idleJ[2]
                    << "," << r.sleepJ[0] << "," << r.sleepJ[1] << "," << r.sleepJ[2] << ","
                    << std::setprecision(4) << r.meanNeighbours[0] << "," << r.meanNeighbours[1]
                    << "," << r.meanNeighbours[2] << "," << r.blindCycles << ","
                    << r.neighbourInsertions << "," << r.macTx << "," << r.macCollisions << ","
                    << r.macCaptures << "," << r.macDeliveries << ","
                    << std::setprecision(4)
                    << r.rmseMlatM << "," << r.cep95MlatM << ","
                    << r.rmseWclM << "," << r.cep95WclM << ","
                    << r.rmseProxM << "," << r.cep95ProxM << ","
                    << r.rmseLkpM << "," << r.cep95LkpM << ","
                    << r.mlatSolvedCycles << "," << std::setprecision(4) << r.meanErrM << ","
                    << r.meanErrFixedM << "," << r.tFirstFixS << ","
                    << r.tFirst100mS << "\n";
            if (subjSummary)
            {
                // MS-1: this arm and seed's subjects, subject 1 (tag 0) first.
                for (size_t k = 0; k < r.subjectRows.size(); ++k)
                {
                    *subjSummary << r.subjectRows[k] << "\n";
                }
            }

            std::cout << "  " << std::left << std::setw(22) << r.arm << std::right << std::fixed
                      << std::setprecision(3) << std::setw(11) << r.rmseM << std::setw(11)
                      << r.rmseFixedM << std::setw(11) << r.cep50M << std::setw(11) << r.cep95M
                      << std::setprecision(4) << std::setw(13) << r.totalJ << std::setw(8)
                      << r.escalations << std::setw(8) << r.suspects << std::setw(10)
                      << r.noFixCycles << std::setprecision(3) << std::setw(9) << f1
                      << std::scientific << std::setprecision(3) << std::setw(11) << accPerJ
                      << std::defaultfloat << "\n";
        }
        std::cout << "\n";
    }

    // A condition that should never occur announces itself rather than being
    // averaged in. Silence means the REINIT fallback never fired.
    if (reinitFallbackTotal > 0)
    {
        std::cerr << "WARN reinit fallback fired on " << reinitFallbackTotal
                  << " cycle(s) -- fix covariance unavailable, P seeded from 100 m^2\n";
    }

    if (subjSummary)
    {
        subjSummary->close(); // MS-1: closed before the provenance digest is taken
    }
    summary.close();
    if (!summary)
    {
        std::cerr << "FATAL: " << summaryPath << " failed to flush -- the file is "
                  << "open-able but incomplete, which is the one state that looks "
                  << "like success to every reader downstream\n";
        return 1;
    }
    // Every stream that this run writes is now closed, which is the precondition
    // for the digest to describe the finished artefact rather than a partial one.
    // relay_chain_*.csv only under --relayChain and fix_routing_*.csv only under
    // --fixRouting, the only runs that write them.
    std::string digestPatterns = relayChainArg
                                     ? "arm_summary.csv trace_*.csv relay_chain_*.csv"
                                     : "arm_summary.csv trace_*.csv";
    if (fixRoutingArg)
    {
        digestPatterns += " fix_routing_*.csv";
    }
    if (tier1TdoaArg)
    {
        digestPatterns += " tdoa_*.csv";
    }
    if (nlosClassifierArg)
    {
        digestPatterns += " nlos_*.csv";
    }
    if (spec.integrityNlosFromScore)
    {
        digestPatterns += " integrity_*.csv";
    }
    if (spec.entryVelocity)
    {
        digestPatterns += " entries_*.csv";
    }
    if (spec.triggeredUpdates)
    {
        digestPatterns += " triggered_*.csv";
    }
    if (spec.subjectTriggeredUpdates)
    {
        digestPatterns += " subjtriggered_*.csv"; // N.3 / row 55
    }
    if (spec.subjectFinderPseudoAnchor)
    {
        digestPatterns += " subjpseudo_*.csv"; // N.21
    }
    if (spec.finderPseudoAnchorStrict)
    {
        digestPatterns += " pseudo_*.csv"; // N.21 strict
    }
    if (spec.sparseInStep)
    {
        digestPatterns += " sparse_*.csv";
    }
    if (spec.imuPredict)
    {
        digestPatterns += " imu_*.csv";
    }
    if (spec.caseD)
    {
        digestPatterns += " cased_*.csv";
    }
    if (spec.entryPrediction)
    {
        digestPatterns += " entrypred_*.csv";
    }
    if (spec.imuWitnessSource == "model")
    {
        digestPatterns += " witness_*.csv";
    }
    if (spec.measuredSubjects > 1)
    {
        digestPatterns += " subject_summary.csv subjtrace_*.csv";
    }
    if (spec.finderTasking == "roundrobin")
    {
        digestPatterns += " tasking_*.csv";
    }
    if (spec.grantGatedUwb)
    {
        digestPatterns += " grants_*.csv";
    }
    if (spec.hopProvenance)
    {
        digestPatterns += " hops_*.csv";
    }
    if (spec.caseCRelative)
    {
        digestPatterns += " case_c_*.csv"; // DC-3
    }
    if (specProfileOn)
    {
        digestPatterns += " spec_profile.txt"; // P6
    }
    if (spec.sessionOverMesh)
    {
        digestPatterns += " session_*.csv";
    }
    if (spec.requesterEscalation)
    {
        digestPatterns += " reqesc_*.csv";
    }
    if (spec.queryV2)
    {
        digestPatterns += " responses_*.csv queryv2_*.csv";
    }
    if (spec.arbitrationV2)
    {
        digestPatterns += " arb_*.csv";
    }
    if (spec.fuseFloorActive)
    {
        digestPatterns += " fusefloor_*.csv"; // N.14
    }
    if (spec.finderRequesterEscalation)
    {
        digestPatterns += " finderreq_*.csv"; // TA-1b
    }
    if (queryProxyArg)
    {
        digestPatterns += " proxy_checks_*.csv";
    }
    // grant_verdicts_*.csv only under --anchorGrantGossipS > 0, the only runs that build a
    // GrantGossip and so the only ones that write it. It is L.1.4's done-when evidence,
    // and until this line the sidecar did not vouch for it: provenance requires a
    // SHA-256 per output file (house rules sect 4.5).
    if (anchorGrantGossipSArg > 0.0)
    {
        digestPatterns += " grant_verdicts_*.csv";
    }
    // M.4. The visualiser's files; nbrtable_/loctable_ exist only with --vizTablePeriodS,
    // and a pattern with no match adds nothing.
    if (digestVizOutputsArg)
    {
        digestPatterns += " events_*.csv nodes_*.csv nbrtable_*.csv loctable_*.csv deployment.json";
    }
    if (!AppendOutputDigest(outdir, digestPatterns))
    {
        return 1;
    }
    std::cout << "Wrote " << summaryPath << "\n";
    if (writeTrace)
    {
        std::cout << "Wrote per-step traces to " << outdir << "/trace_<arm>_seed<N>.csv\n";
    }

    std::cout << "\nRead the summary this way:\n"
              << "  - Check noFixCycles FIRST. If it is a large fraction of steps for any\n"
              << "    arm, that arm is measuring dead reckoning, not localisation, and its\n"
              << "    RMSE means nothing. This is what invalidated the pre-S2.5 results.\n"
              << "  - static_uwb should now have the best RMSE and the WORST energy. If it\n"
              << "    is still the cheapest arm, it is still failing to range and the\n"
              << "    geometry is still wrong -- run --probe.\n"
              << "  - static_868 should have the worst RMSE among arms that get fixes.\n"
              << "  - cyclesInUwbCoverage should be a meaningful minority of steps (roughly\n"
              << "    10-30%). At 0 there is nothing to escalate to; at ~100% escalation is\n"
              << "    unconditionally correct and the policy is not being tested.\n"
              << "  - trig_f1 scores the tier choice against geometric ground truth.\n"
              << "    static_uwb gets high recall and low precision by construction;\n"
              << "    static_868 gets zero. The policy arms are the only interesting rows.\n"
              << "  - If ca_tbl_full does not beat fixed_threshold on accuracyPerJoule,\n"
              << "    the policy is not earning its complexity -- that is a real result,\n"
              << "    not a bug to tune away.\n";

    return 0;
}
