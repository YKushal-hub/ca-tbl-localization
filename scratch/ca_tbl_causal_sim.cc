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
#include <cmath>
#include <limits>
#include <deque>
#include <fstream>
#include <functional>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
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
                double gwRingFrac)
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
        for (uint32_t k = 0; k < nGateways; ++k)
        {
            const double th =
                2.0 * M_PI * static_cast<double>(k) / static_cast<double>(nGateways) + M_PI / 2.0;
            dep.anchors[CA_TBL_TIER_868].push_back(
                {id++, Vector(R * std::cos(th), R * std::sin(th), gwH)});
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
        if (txId == m_nodeId)
        {
            anchor = rxId;
        }
        else if (rxId == m_nodeId)
        {
            anchor = txId;
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

    void Fire()
    {
        mac->SendQuery(tier, manifestId);
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

/** Everything one arm produced, for the summary table. */
struct ArmResult
{
    std::string arm;
    uint32_t seed;
    double rmseM;
    double cep50M;
    double cep95M;
    double maxErrM;
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
          m_heading(0.0)
    {
        // Start off-site so the run begins in the tier-1-only regime.
        m_x = m_rng->GetValue(-boxHalfM * 0.2, boxHalfM * 0.2);
        m_y = m_rng->GetValue(-boxHalfM * 0.2, boxHalfM * 0.2);
        PickTarget();
    }

    void Step(double dt)
    {
        if (m_dwellLeft > 0.0)
        {
            // Milling about inside a site rather than standing perfectly still.
            m_dwellLeft -= dt;
            m_heading += m_rng->GetValue(-1.2, 1.2) * dt;
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
                m_heading += M_PI; // turn back inward rather than pressing the edge
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
    Ptr<UniformRandomVariable> m_rng;
    size_t m_target;
    double m_dwellLeft;
    double m_heading;
    double m_x;
    double m_y;
};

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
    if (g_terrainPerTierFreq)                                                                      \
    {                                                                                              \
        /* Carriers from Architecture.md 2.1: 868 MHz, 2.4 GHz ISM, UWB Ch9 7.99 GHz. */           \
        c2p4.terrain.frequencyHz = 2.4e9;                                                          \
        cUwb.terrain.frequencyHz = 7.9872e9;                                                       \
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
};

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
       bool vizEventsOn,
       uint32_t nFinders,
       uint32_t finderMode,
       double finderDigestPeriodS,
       double finderPosSigmaM,
       bool finderPseudoAnchor,
       bool bidirectionalDigest,
       bool tier1ChargeReplies,
       double finderDispersal,
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
       uint32_t injectFaultAnchors)
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
    tags[0].walker = &walker;
    Vector pos = walker.Position();
    engine868.RegisterNode(nodeId, pos);
    engine2p4.RegisterNode(nodeId, pos);
    engineUwb.RegisterNode(nodeId, pos);

    for (int t = 0; t < CA_TBL_TIER_COUNT; ++t)
    {
        energy.SetLoRaPhy(static_cast<CaTblTier>(t), phy[t].sf, phy[t].bandwidthHz, 1);
        energy.SetTier1ChargeReplies(tier1ChargeReplies);
    }

    CaTblPolicyConfig pcfg;
    pcfg.mode = mode;
    pcfg.mahalUseR = mahalUseR;
    pcfg.suspectRecovery = suspectRecovery;
    pcfg.suspectInflateFactor = suspectInflateFactor;
    pcfg.suspectRecoveryN = suspectRecoveryN;
    pcfg.escalationRequiresEvidence = escalationRequiresEvidence;
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
    // Non-positive means leave the built-in default, matching the sentinel
    // convention fixedVarThresholdM2 and pcsSigmaTargetM already use below.
    if (pcsW1 > 0.0) { pcfg.pcsW1 = pcsW1; }
    if (pcsW2 > 0.0) { pcfg.pcsW2 = pcsW2; }
    if (pcsW3 > 0.0) { pcfg.pcsW3 = pcsW3; }
    if (pcsGateFloor > 0.0) { pcfg.pcsGateFloor = pcsGateFloor; }
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
    // A6 / CAL-03 / CAL-04. Applied LAST so it composes with either score:
    // ApplyV2Thresholds() has already run for v2, and the band is expressed
    // RELATIVE to whatever thresholdUp ended up being. The BAND, not the two
    // absolute thresholds, is the swept quantity -- every tier carries the
    // same 0.20 gap today, so one number moves the hysteresis without also
    // silently moving the operating point, which sweeping thresholdDown
    // alone would do.
    if (pcsHysteresisBand >= 0.0)
    {
        for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
        {
            pcfg.pcsThresholdDown[k] = pcfg.pcsThresholdUp[k] + pcsHysteresisBand;
        }
    }
    if (nDeescalateOverride > 0) { pcfg.nDeescalate = nDeescalateOverride; }
    pcfg.hopProvenanceForm = hopProvenanceForm;
    pcfg.hopPenalty = hopPenalty;
    pcfg.imuCrossCheck = imuCrossCheck;
    pcfg.corroborationCheck = corroborationCheck;
    policy.Configure(pcfg);
    // Seeded at the true start position with a deliberately loose covariance:
    // the tag knows roughly where it deployed, not exactly.
    policy.Initialize(pos.x, pos.y, 100.0, 5.0);

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
    if (vizEventsOn)
    {
        std::ostringstream vfn;
        vfn << traceDir << "/events_" << CaTblPolicyModeName(mode) << "_seed" << seed << ".csv";
        vizEvents.open(vfn.str().c_str());
        if (vizEvents.is_open())
        {
            vizEvents << "t,tier,txId,rxId,msgType,outcome,rxPowerDbm\n";
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
    std::deque<CaTblMac> macs; // deque: push_back must not invalidate held pointers
    std::deque<BeaconDriver> beacons;
    std::map<uint16_t, Vector> anchorPosById;

    for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
    {
        chan[ti].SetPhyProvider(&provider);

        CaTblMacConfig mc;
        mc.nodeId = static_cast<uint16_t>(nodeId);
        mc.identityClass = CA_TBL_ID_TARGET;
        // C3. The measured tag is manifest 1; peers are 2, 3, ... Nothing read
        // this field before C3, so setting it explicitly changes no behaviour --
        // and the null control confirms that rather than assuming it.
        mc.targetManifestId = 1;
        mc.sendHandshakeFrames = sendHandshakeFrames;
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
            mc.nodeId = aid;
            mc.identityClass = CA_TBL_ID_ANCHOR;
            // C4. Only anchors arbitrate -- a target has no slot to award.
            mc.uwbArbitration = uwbArbitration;
            mc.sendHandshakeFrames = sendHandshakeFrames;
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
    // discriminates on. It has no CaTblPolicy and no CaTblEnergyModel: it is an
    // addressing and contention peer, NOT a second measured subject, and its energy
    // is deliberately not accounted. Anything that reports peer energy is wrong.
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
            pmc.nodeId = static_cast<uint16_t>(pid);
            pmc.identityClass = CA_TBL_ID_TARGET;
            // THE POINT OF THE PEER. A distinct manifest id is what makes an
            // identity-scoped query answerable: without it every target matches
            // every query and C3 has nothing to discriminate.
            pmc.targetManifestId = static_cast<uint8_t>(1 + pt);
            pmc.sendHandshakeFrames = sendHandshakeFrames;
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
                QueryDriver qd;
                qd.mac = &macs[m];
                qd.tier = static_cast<CaTblTier>(ti);
                qd.periodS = identityQueryPeriodS;
                qd.jitterMaxS = identityQueryPeriodS * 0.25;
                qd.untilS = durationS;
                qd.manifestId = static_cast<uint8_t>(identityQueryTarget);
                qd.rng = macRng;
                queries.push_back(qd);
                break;
            }
        }
    }

    // C4. Peer UWB requests, so the arbiter has competing claims to resolve. Only
    // when arbitration is on AND a peer exists -- otherwise there is nothing to
    // arbitrate and this would just be extra traffic.
    if (uwbArbitration && nTags > 1)
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
        finderWalkers.emplace_back(dep.facilities, walkSpeedMps, dwellS,
                                   dep.boxHalfM, dep.dwellRadiusM, frng);
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
            fc.nodeId = static_cast<uint16_t>(fid);
            fc.identityClass = CA_TBL_ID_RESCUER;
            fc.isAccessPoint = false;
            fc.uwbArbitration = uwbArbitration;
            fc.sendHandshakeFrames = sendHandshakeFrames;
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
    Vector prevTruePos = pos;
    double imuDisagreeM = 0.0;
    double imuTolM = 0.0;
    double lastTrustedFixS = 0.0;

    // The real step-and-heading system from IMU_SHS_V1.1, not a scale-error stub.
    lorawan::ShsEngineParams shsParams;
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
    shsDriver.engine = &shsEngine;

    auto& pcsHist = TS0.pcsHist;
    auto& pcsDegradation = TS0.pcsDegradation;
    auto& batteryUsedFrac = TS0.batteryUsedFrac;

    // Step count is fixed up front and time is read back from the scheduler rather
    // than accumulated in the loop variable. With an event layer running, the
    // authoritative clock is Simulator::Now(); keeping a parallel float counter is
    // how the two silently drift apart.
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
            if (relayedRanging && (pi + 1) < tags.size())
            {
                tags[pi + 1].pos = pp;
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
        for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
        {
            if (nodeMac[ti] != nullptr)
            {
                nodeMac[ti]->PublishSelfEntry(policy.EstX(), policy.EstY(),
                                              policy.Pxx(), policy.Pyy(), policy.Pxy(),
                                              0.0, 0.0, t);
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
            for (int ti = 0; ti < CA_TBL_TIER_COUNT; ++ti)
            {
                if (finderMacAll[fi][ti] != nullptr)
                {
                    finderMacAll[fi][ti]->PublishSelfEntry(truePos.x + off.x,
                                                           truePos.y + off.y,
                                                           finderPosVar, finderPosVar,
                                                           0.0, 0.0, 0.0, nowS);
                }
            }
        };

        // Finders move. Their position is pushed to the engines (which key correlation
        // and mobility state on node id), to the provider (which computes the link) and
        // to anchorPosById (which the ranging poll reads for the far-end position). All
        // three, or a finder ranges from a position it has left.
        for (size_t fi = 0; fi < finderWalkers.size(); ++fi)
        {
            Vector fp;
            if (finderMode == 1)
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
                    if (finderMac[fi]->GetTableEntry(static_cast<uint16_t>(nodeId), te))
                    {
                        gx = te.x;
                        gy = te.y;
                        known = true;

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
                        if (finderDispersal > 0.0 && nFinders > 1)
                        {
                            const double sx = std::sqrt(std::max(te.varX, 0.0));
                            const double sy = std::sqrt(std::max(te.varY, 0.0));
                            const double th = 2.0 * M_PI * static_cast<double>(fi) /
                                              static_cast<double>(nFinders);
                            gx += finderDispersal * sx * std::cos(th);
                            gy += finderDispersal * sy * std::sin(th);
                        }
                    }
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
            for (uint32_t pt = 1; pt < nTags && pt < tags.size(); ++pt)
            {
                const size_t pi = pt - 1;
                if (pi >= peerMacs.size() ||
                    static_cast<size_t>(rti) >= peerMacs[pi].size())
                {
                    continue;
                }
                const std::vector<CaTblNeighbour> nb =
                    peerMacs[pi][static_cast<size_t>(rti)]->GetNeighbours(rt, t);
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
                    if (rt == CA_TBL_TIER_868)
                    {
                        PhysicsResult r = engine868.ProcessTransmission(
                            tags[pt].nodeId, anchorId, tags[pt].pos, anchorPos,
                            phy[rti].txPowerDbm, phy[rti].sf, phy[rti].bandwidthHz, 0,
                            now);
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
                            now);
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
                            now);
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
                        pmeas.push_back(m);
                    }
                }
                // Charged on the relay's own account, RX timeouts of anchors that did
                // not answer included -- the same rule the measured tag pays.
                tags[pt].energy.AccountRangingExchange(
                    rt, static_cast<uint32_t>(nb.size()),
                    static_cast<uint32_t>(pmeas.size()), t);
                tags[pt].policy.Step(dt, pmeas, 0,
                                     static_cast<int32_t>(pmeas.size()),
                                     static_cast<int32_t>(nb.size()), 0);
                ++tags[pt].steps;
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
        // and break the one invariant the energy axis rests on (RULES.md sect 6).
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
            const std::vector<CaTblNeighbour> nb = nodeMac[tier]->GetNeighbours(tier, t);

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

                if (tier == CA_TBL_TIER_868)
                {
                    PhysicsResult r = engine868.ProcessTransmission(nodeId,
                                                                    anchorId,
                                                                    pos,
                                                                    anchorPos,
                                                                    phy[0].txPowerDbm,
                                                                    phy[0].sf,
                                                                    phy[0].bandwidthHz,
                                                                    0,
                                                                    now);
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    nlos = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
                    nlosProp = r.isNLOS;
                    hiddenActive = (r.hiddenChannelState > 0);
                    sigmaM = r.rangingSigmaM;
                }
                else if (tier == CA_TBL_TIER_2P4)
                {
                    PhysicsResult2p4 r = engine2p4.ProcessTransmission(nodeId,
                                                                       anchorId,
                                                                       pos,
                                                                       anchorPos,
                                                                       phy[1].txPowerDbm,
                                                                       phy[1].sf,
                                                                       phy[1].bandwidthHz,
                                                                       0,
                                                                       now);
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
                }
                else
                {
                    PhysicsResultUWB r = engineUwb.ProcessTransmission(nodeId,
                                                                       anchorId,
                                                                       pos,
                                                                       anchorPos,
                                                                       phy[2].txPowerDbm,
                                                                       phy[2].sf,
                                                                       phy[2].bandwidthHz,
                                                                       0,
                                                                       now);
                    received = r.packetReceived;
                    range = r.estimatedRangeM;
                    nlos = CaTblNlosFlag(nlosSource, r.nlosDb, r.isNLOS, r.hiddenChannelState);
                    nlosProp = r.isNLOS;
                    hiddenActive = (r.hiddenChannelState > 0);
                    sigmaM = r.rangingSigmaM;
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
                    if (finderPseudoAnchor && anchorId >= 501)
                    {
                        CaTblTableEntry fe;
                        if (freshestEntry(anchorId, fe))
                        {
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
                                const double ageVar = travel * travel;
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
                                }
                            }
                        }
                    }
                    m.rangeM = range;
                    m.nlos = nlos;
                    m.nlosPropagation = nlosProp;
                    m.hiddenChannelActive = hiddenActive;
                    m.sigmaM = sigmaM;
                    out.push_back(m);
                }
            }

            // nAnchorsAddressed is now the DISCOVERED count. An entry that has gone
            // stale within its TTL but is no longer reachable still costs a full RX
            // timeout here, which is correct and is the price of the TTL being
            // finite rather than clairvoyant.
            energy.AccountRangingExchange(tier,
                                          static_cast<uint32_t>(nb.size()),
                                          static_cast<uint32_t>(out.size()),
                                          t);
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
            energy.AccountEscalationHandshake(pendingHandshakeTier, t);
            // C1: the frame ITSELF, in addition to the charge above. The
            // accounting already sizes the handshake correctly; what it has
            // never done is contend for the medium, pass CAD, or meet the
            // duty governor as a frame. Adding rather than replacing keeps
            // the energy axis fixed, so a change here is attributable to
            // contention and to nothing else.
            if (sendHandshakeFrames && nodeMac[pendingHandshakeTier] != nullptr)
            {
                nodeMac[pendingHandshakeTier]->Send(
                    CA_TBL_MSG_ESC_REQUEST, pendingHandshakeTier, 0, 0);
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
        const bool activeSolved = (activeMeas.size() >= 3);
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
                    if (active == CA_TBL_TIER_2P4)
                    {
                        PhysicsResult2p4 r = engine2p4.ProcessTransmission(
                            nodeId, static_cast<uint16_t>(tags[pt].nodeId), pos,
                            tags[pt].pos, phy[ati].txPowerDbm, phy[ati].sf,
                            phy[ati].bandwidthHz, 1, now);
                        got = r.packetReceived;
                        rAB = r.estimatedRangeM;
                        sigRaw = r.rangingSigmaM;
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
                    rr.hopDepth = 1;
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

                    double sigmaEff = 0.0;
                    const CaTblRelayVerdict v =
                        CaTblAdmitRelayedRange(rr, relaysUsedThisUpdate, &sigmaEff);
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
                    activeMeas.push_back(m);

                    relaysUsedThisUpdate.insert(rr.relayId);
                    relayedVarThisCycle += sigmaEff * sigmaEff;
                    ++relayAdmitted;
                    relaySigmaEffSum += sigmaEff;
                    relaySigmaRawSum += sigRaw;
                }
            }
        }

        const std::vector<CaTblRangeMeasurement>& meas = usedFloor ? floorMeas : activeMeas;
        const CaTblTier fixTier = usedFloor ? CA_TBL_TIER_868 : active;

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

        CaTblDecision d = policy.Step(dt,
                                            meas,
                                            0,
                                            static_cast<int32_t>(activeMeas.size()),
                                            static_cast<int32_t>(activeMeasAddressed),
                                            tierEvidenceMask);

        // The engine takes the integrity verdict back: a fix the check calls SUSPECT
        // must not be used to reset inertial drift, or a bad fix would launder itself
        // into the one estimate that was independent of it.
        if (d.hadFix && !d.suspect)
        {
            shsEngine.OnTrustedFix(t, policy.EstX(), policy.EstY(), true);
            lastTrustedFixS = t;
        }

        if (d.escalated || d.deescalated)
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
                energy.AccountEscalationHandshake(active, t);
                // C1: see the note at the pre-emption site above. Charge
                // unchanged, frame added.
                if (sendHandshakeFrames && nodeMac[active] != nullptr)
                {
                    // C4. Fill the two terms the anchor cannot see: T_deg is this
                    // node's own confidence degradation, T_batt how much of the stated
                    // budget it has spent. Quantised to a byte over [0,1].
                    nodeMac[active]->SetRequestTerms(CaTblQuantise(pcsDegradation),
                                                     CaTblQuantise(batteryUsedFrac));
                    nodeMac[active]->Send(CA_TBL_MSG_ESC_REQUEST, active, 0, 0);
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
                energy.AccountEscalationHandshake(active, tDefer);
                ++handshakeIntraCycleDefers;
                handshakeIntraCycleWaitS += (tDefer - t);
                if (sendHandshakeFrames && nodeMac[active] != nullptr)
                {
                    // C4. Fill the two terms the anchor cannot see: T_deg is this
                    // node's own confidence degradation, T_batt how much of the stated
                    // budget it has spent. Quantised to a byte over [0,1].
                    nodeMac[active]->SetRequestTerms(CaTblQuantise(pcsDegradation),
                                                     CaTblQuantise(batteryUsedFrac));
                    nodeMac[active]->Send(CA_TBL_MSG_ESC_REQUEST, active, 0, 0);
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
        // HALF-TESTED, and say so. StartHandshake() branches on mobility state, and
        // the RECEDING branch -- skip the digest, one-shot urgent push at CW_high --
        // is the interesting half. Nothing here sets mobility state, so every node
        // reports STATIONARY and the receding branch never runs. Claim 4 is
        // exercised for its stationary path only until a mobility estimator exists.
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
            const uint64_t rxNow = nodeMac[tierIdx]->GetTotals().received;

            if (wasAwake)
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
        const uint32_t visUwb = VisibleCount(dep, CA_TBL_TIER_UWB, pos);
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
        if (d.hadFix)
        {
            errorsFixed.push_back(err);
        }
        steps++;

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

    double sumSqFixed = 0.0;
    for (size_t i = 0; i < errorsFixed.size(); ++i)
    {
        sumSqFixed += errorsFixed[i] * errorsFixed[i];
    }
    res.rmseFixedM =
        errorsFixed.empty() ? 0.0
                            : std::sqrt(sumSqFixed / static_cast<double>(errorsFixed.size()));
    res.cep50FixedM = Percentile(errorsFixed, 0.50);
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

    // C3. Queries answered and correctly ignored, summed over the tag's three
    // MACs. To stdout rather than arm_summary.csv: a new column changes the CSV
    // header for every run and breaks byte-identity against baseline_v7.
    {
        uint64_t qa = 0, qi = 0;
        for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
        {
            qa += nodeMac[i]->GetQueriesAnswered();
            qi += nodeMac[i]->GetQueriesIgnored();
        }
        if (qa + qi > 0)
        {
            std::cout << "    C3 identity queries: answered " << qa
                      << ", correctly ignored " << qi << "\n";
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

int
main(int argc, char* argv[])
{
    uint32_t envInt = 1;         // ENV_RURAL_FORESTED -- the wide-area SAR scenario
    uint32_t envFacilityInt = 0; // ENV_RURAL_OPEN -- facility sites are clearings
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
    double uwbTriadRadiusM = 10.0; // UWB r80 = 10 m
    double t2TriadRadiusM = 150.0; // 2p4 r80 >= 900 m, so siting-limited not range-limited
    uint32_t uwbAnchorsPerSite = 5;
    uint32_t t2AnchorsPerSite = 3;
    double walkSpeedMps = 1.2;
    double dwellS = 90.0;
    uint32_t vegWideInt = static_cast<uint32_t>(VEG_DENSE_FOREST);
    uint32_t vegFacilityInt = static_cast<uint32_t>(VEG_SPARSE_SHRUBS);

    CommandLine cmd;
    cmd.AddValue("env",
                 "EnvironmentClass for the wide-area 868 tier (0=RuralOpen "
                 "1=RuralForested 2=Suburban 3=UrbanModerate 4=UrbanDense)",
                 envInt);
    cmd.AddValue("envFacility",
                 "EnvironmentClass at facility sites, for the 2.4 GHz and UWB tiers",
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
    bool escalationRequiresEvidence = false;
    bool uwbEvidenceFromNeighboursArg = false;
    bool suspectForcesEscalation = true;
    std::string pcsFormArg = "legacy";
    double pcsW1Arg = -1.0;
    double pcsW2Arg = -1.0;
    double pcsW3Arg = -1.0;
    double pcsGateFloorArg = -1.0;
    double pcsHysteresisBandArg = -1.0;
    uint32_t nDeescalateArg = 0;
    bool sendHandshakeFramesArg = false;
    bool terrainPerTierFreqArg = false;
    bool intraCycleHandshakeDeferArg = false;
    uint32_t nTagsArg = 1;
    bool relayedRangingArg = false;
    uint32_t relayTierArg = 1;
    bool vizEventsArg = false;
    uint32_t nFindersArg = 0;
    uint32_t finderModeArg = 1;
    double finderDigestPeriodSArg = 30.0;
    double finderPosSigmaMArg = 0.0;
    bool finderPseudoAnchorArg = false;
    bool bidirectionalDigestArg = false;
    bool tier1ChargeRepliesArg = false;
    double finderDispersalArg = 0.0;
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
    uint32_t nGatewaysArg = 0;
    double gwRingFracArg = 0.6;
    double identityQueryPeriodSArg = 0.0;
    uint32_t identityQueryTargetArg = 2;
    bool uwbArbitrationArg = false;
    double arbBatteryCapJArg = 1000.0;
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
                 "VegetationType at facility sites, for the 2.4 GHz and UWB tiers",
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
                 "Response when the integrity gate rejects a fix: none|inflate|bounded|"
                 "reinit. none (default) is the pre-existing behaviour -- RevertToPrior "
                 "with no recovery, which measurably locks the filter for up to 251 "
                 "consecutive cycles (22 with no fault injected at all).",
                 suspectRecoveryArg);
    cmd.AddValue("suspectInflateFactor",
                 "INFLATE: factor applied to the position covariance on rejection. "
                 "REINIT: multiplier on the raw fix covariance when reseeding.",
                 suspectInflateFactor);
    cmd.AddValue("suspectRecoveryN",
                 "BOUNDED/REINIT: consecutive rejections before recovery fires. "
                 "Default 5 = the p90 of measured rejection-run lengths on F1.",
                 suspectRecoveryN);
    cmd.AddValue("escalationRequiresEvidence",
                 "Escalate 2.4->UWB only when the node's own EKF estimate predicts >=3 "
                 "UWB anchors within uwbPredictRadiusM, behind an estimate-quality gate. "
                 "Suppress-only; never creates an escalation. Does NOT gate 868->2.4 -- "
                 "see escalationRequiresEvidenceT2 for why that one does not work.",
                 escalationRequiresEvidence);
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
                 "report/PCS_REDESIGN_RESULTS.md §0.",
                 pcsFormArg);
    cmd.AddValue("pcsW1",
                 "CA_TBL_PCS_WEIGHTED weight on the filter-uncertainty term. "
                 "Default 2.0. The defaults are HAND-SET, not from the "
                 "leave-one-out cross-validation CAL-01 specifies -- that has "
                 "still never been run. They are chosen for one property: "
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
                 "which has NEVER been swept, because until now "
                 "pcsThresholdDown was not registered with AddValue at all. "
                 "The BAND is the swept quantity rather than the two absolute "
                 "thresholds: every tier carries the same 0.20 gap today, so "
                 "one number moves the hysteresis without silently also "
                 "moving the operating point.",
                 pcsHysteresisBandArg);
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
    cmd.AddValue("tier1ChargeReplies",
                 "Charge tier 1 for the downlink that carries its ranges back. FALSE by "
                 "default, which preserves every published number. Tier 1 is billed as "
                 "ONE BROADCAST AND NOTHING ELSE -- a TDoA shape, and the right cost "
                 "when the NETWORK localises. But CaTblPolicy runs on the FIELD NODE and "
                 "consumes a range per anchor per cycle, and nothing charges for those "
                 "ranges arriving: there is no per-beacon RX accounting and AccountListen "
                 "fires only when the neighbour table is empty. So tier 1's structural "
                 "energy advantage, which every accuracy-per-joule figure rests on, is "
                 "measured against an exchange one message short of closing. This "
                 "measures the gap; it does not decide which shape is right, because that "
                 "is the CONTRADICTION row in MASTER_PLAN 2.1 -- the spec puts the "
                 "estimator on the gateway and the code puts it on the tag.",
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
                 "Refuse an escalation when the filter's displacement disagrees with "
                 "the body's, measured by dead reckoning. FALSE by default so the check "
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
    cmd.AddValue("finderMode",
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
                 "does not cover that.",
                 relayedRangingArg);
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
                 "joules are not accounted. Reporting peer energy would be wrong. "
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
    cmd.Parse(argc, argv);

    g_terrainPerTierFreq = terrainPerTierFreqArg;

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
    else if (pcsFormArg != "legacy")
    {
        std::cout << "ERROR: --pcsForm must be 'legacy', 'v2', 'weighted' or "
                  << "'gated', got '" << pcsFormArg << "'\n";
        return 1;
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
    else if (suspectRecoveryArg != "none")
    {
        std::cerr << "FATAL: --suspectRecovery=" << suspectRecoveryArg
                  << " is not one of none|inflate|bounded|reinit\n";
        return 1;
    }

    const EnvironmentClass env = static_cast<EnvironmentClass>(envInt);
    const EnvironmentClass envFacility = static_cast<EnvironmentClass>(envFacilityInt);
    const VegetationType vegWide = static_cast<VegetationType>(vegWideInt);
    const VegetationType vegFacility = static_cast<VegetationType>(vegFacilityInt);

    TierPhy phy[CA_TBL_TIER_COUNT];
    phy[CA_TBL_TIER_868] = {7, 125e3, 14.0};
    phy[CA_TBL_TIER_2P4] = {7, 800e3, 12.5};
    phy[CA_TBL_TIER_UWB] = {7, 499.2e6, -16.47};

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

    const Deployment dep = BuildDeployment(gwGrid,
                                           boxHalfM,
                                           uwbTriadRadiusM,
                                           t2TriadRadiusM,
                                           uwbAnchorsPerSite,
                                           t2AnchorsPerSite,
                                           sitingModeArg,
                                           topologyArg,
                                           nGatewaysArg,
                                           gwRingFracArg);

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
            << "mac_tx,mac_collisions,mac_captures,mac_deliveries\n";

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
                                       vizEventsArg,
                                       nFindersArg,
                                       finderModeArg,
                                       finderDigestPeriodSArg,
                                       finderPosSigmaMArg,
                                       finderPseudoAnchorArg,
                                       bidirectionalDigestArg,
                                       tier1ChargeRepliesArg,
                                       finderDispersalArg,
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
                                       injectFaultAnchors);

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
                    << r.macCaptures << "," << r.macDeliveries << "\n";

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
    if (!AppendOutputDigest(outdir, "arm_summary.csv trace_*.csv"))
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
