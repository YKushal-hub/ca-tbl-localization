#ifndef CA_TBL_POLICY_H
#define CA_TBL_POLICY_H

/**
 * \file ca-tbl-policy.h
 * \brief CA-TBL Phase S1 -- the tier-selection policy, moved inside the simulation.
 *
 * Why this exists
 * ---------------
 * The previous pipeline generated 868 MHz, 2.4 GHz and UWB rows for every
 * timestep unconditionally, then let an offline Python script choose which
 * tier's rows to read. That made the escalation decision non-causal: UWB
 * receptions were computed even for steps where the policy never activated UWB.
 * Three consequences followed, all of them fatal to the claims being made:
 *
 *   1. Energy saved by *not* escalating could not be measured, because the
 *      unused radio had already "run".
 *   2. "The handoff is worth it" degenerated into post-hoc data selection
 *      rather than a control decision with consequences.
 *   3. Escalation latency and the cost of thrashing were invisible, so the
 *      hysteresis parameters could not be shown to do anything.
 *
 * This class is the fix. It owns the filter, the confidence score and the
 * escalation state machine, and it is stepped by the simulation so that its
 * output at time t determines which radio physically transmits at time t+1.
 *
 * Ported from scratch/python_handoff/{ekf_tracker,ca_tbl_fusion}.py. The Python
 * remains as a cross-check oracle: run both on the same trace and the position
 * estimates should agree to numerical tolerance. Keep it for that purpose.
 *
 * Filter note
 * -----------
 * The Python implementation performs a batch update with a dense n-by-n
 * innovation covariance. Here each range is applied as a sequential scalar
 * update. These are algebraically equivalent whenever R is diagonal, which it
 * is (per-anchor variances, no modelled cross-correlation), and the sequential
 * form avoids a general matrix inverse. This is a deliberate equivalence, not
 * an approximation -- if R ever gains off-diagonal terms, this must change.
 */

#include "ca-tbl-energy-model.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ns3
{

/** One anchor's range observation for a single cycle. */
struct CaTblRangeMeasurement
{
    double anchorX;
    double anchorY;
    double rangeM; //!< measured range (EstimatedRangeM from the physics engine)
    //! The OPERATIVE NLOS tag: what the R matrix, the LOS fraction in PCS and the
    //! integrity gate's all-LOS condition all key off. Which physical quantity it
    //! is built from is the caller's choice, selected by --nlosSource; see
    //! nlosPropagation / hiddenChannelActive below for the two candidate
    //! populations, which are always both recorded regardless of the selection.
    bool nlos;
    //! The propagation layer's own NLOS determination (PhysicsResult::isNLOS,
    //! i.e. nlosDb > 0.01) -- an obstruction between these two endpoints.
    bool nlosPropagation;
    //! Whether the link's 4-state Markov fading regime is nonzero.
    //!
    //! NOT an obstruction indicator, and recorded only so the two populations can
    //! be compared numerically rather than argued about. Under the fidelity mode
    //! this scenario actually runs (FIDELITY_ULTRA_FAST_STATISTICAL, set by
    //! ForScenario()), the state attenuates nothing, delays nothing and biases
    //! nothing: every coupling from it into the channel is gated behind
    //! FIDELITY_SDR_GRADE_REALISM. It is a free-running label. Keying measurement
    //! variance to it inflates noise on the basis of a variable that does not
    //! affect the measurement.
    bool hiddenChannelActive;
    //! Ranging 1-sigma in metres for this measurement, from the physics engine's
    //! rangingSigmaM. 0.0 means unavailable, in which case the policy falls back to
    //! the per-tier losSigmaM/nlosSigmaM constants.
    //!
    //! Per-measurement rather than per-tier because (a) the CRLB-derived sigma now
    //! varies with distance and SNR within a tier, and (b) a concurrent-tier arm
    //! submits ranges whose sigmas span four orders of magnitude, so a single
    //! tier-keyed constant cannot weight them.
    double sigmaM;
    //! The tier-specific NLOS_score of Physical sect 1.3 / 2.5, in [0,1], from
    //! ca-tbl-nlos-classifier.h; -1 when no classifier scored this link (the default,
    //! and always the case without --nlosClassifier). Read ONLY by the PCS NLOS term,
    //! and only under CaTblPolicyConfig::pcsNlosFromScore. R, the integrity check's
    //! all-LOS condition and every count above still key off the boolean `nlos`.
    double nlosScore;

    CaTblRangeMeasurement();
};

/** Which tier-selection strategy an experimental arm uses. */
enum CaTblPolicyMode
{
    CA_TBL_MODE_FULL = 0,          //!< PCS escalation + integrity check (the proposed system)
    CA_TBL_MODE_NO_INTEGRITY,      //!< PCS escalation only -- isolates the integrity contribution
    CA_TBL_MODE_FIXED_THRESHOLD,   //!< escalate on raw position variance -- the literature baseline
    CA_TBL_MODE_STATIC_868,        //!< never escalate
    CA_TBL_MODE_STATIC_2P4,        //!< pinned to tier 2
    CA_TBL_MODE_STATIC_UWB         //!< always-on UWB -- the accuracy ceiling / energy floor
};

const char* CaTblPolicyModeName(CaTblPolicyMode mode);

/**
 * \brief Which functional form the Position Confidence Score uses.
 *
 * See report/PCS_REDESIGN_RESULTS.md §0 for the pre-registration, which was
 * committed before any v2 run.
 */
enum CaTblPcsForm
{
    /**
     * traceRef/(traceRef+traceSigma) + 1/max(gdop,1) + nLos/nTotal.
     *
     * Three defects, all measured. Term 1 SATURATES, spanning 0.061 across tiers
     * 1-2 where the underlying uncertainty spans three orders of magnitude. Term 2
     * is INVERTED across tiers: GDOP is worse for the tightly clustered facility
     * triads (tiers 2/3) than for spread gateways (tier 1), so the geometry term
     * rewards the coarse tier. Term 3 was PINNED near 0 by the NLOS defect and is
     * pinned near 1 once that is fixed -- an offset either way.
     *
     * Retained as the default so results/baseline_v4 reproduces byte-identically.
     */
    CA_TBL_PCS_LEGACY = 0,
    /**
     * log10(sigma_ref / sqrt(sigma_post * sigma_geo)).
     *
     * How many DECADES of position uncertainty separate the current fix from the
     * stated accuracy target. Dimensionally a length ratio, unbounded in both
     * directions, so it retains resolution across the whole tier range instead of
     * saturating.
     *
     *   sigma_ref  = sqrt(traceRef) = 3.1623 m -- the SPECIFICATION, not a tuned
     *                constant; it predates this work and is documented as the
     *                target radial accuracy.
     *   sigma_post = sqrt(traceSigma)          -- EKF posterior radial sigma
     *   sigma_geo  = sqrt(trace((J^T R^-1 J)^-1)) -- this cycle's measurement-only
     *                radial sigma. The EXACT quantity, not the gdop*sigma
     *                approximation: RawMultilaterationWeighted() already returns
     *                it at no extra cost and no RNG draw, and GDOP is defined for
     *                equal per-range sigma, which is the assumption tier 1
     *                violates hardest.
     *
     * The LOS fraction is deliberately ABSENT. It was measured, after the NLOS fix,
     * at sd 0.0097 on tier 2 and EXACTLY CONSTANT on tier 3 -- a near-constant term
     * is an offset, and an offset compared against a threshold is absorbed by the
     * threshold. It is still emitted as a diagnostic so the exclusion stays visible.
     * See §0.1.1 of the pre-registration.
     */
    CA_TBL_PCS_V2,
    /**
     * w1*term1 + w2*term2 + w3*term3 -- the LEGACY terms carrying the weighting
     * the specification always asked for and the implementation never had.
     *
     * WHY THIS EXISTS. CAL-01 specifies weights determined by leave-one-out
     * cross-validation. That was never attempted; LEGACY summed the three terms
     * UNWEIGHTED, and that is the direct cause of the saturation defect. With
     * term2 in (0,1] and term3 in [0,1] their sum alone reaches 2.0 against a
     * threshold of 1.55, so term 1 -- the only term carrying filter uncertainty
     * -- CANNOT VETO. It can fail to contribute; it can never block an
     * escalation. A confidence score that cannot express its own uncertainty is
     * not a confidence score.
     *
     * THE DEFAULT WEIGHTS ARE OURS, NOT CALIBRATED, and are chosen for exactly
     * one property: w2 + w3 = 1.0 sits BELOW every pcsThresholdUp, so term1 = 0
     * forces the score under the threshold whatever the geometry and LOS terms
     * do. That restores the veto. It does NOT make the score informative -- that
     * is an empirical question the monotone-of-filter-statistic control decides,
     * and two prior forms have failed it. Report these as hand-set pending LOOCV.
     */
    CA_TBL_PCS_WEIGHTED,
    /**
     * LEGACY, except term 1 VETOES: below pcsGateFloor the score is forced to 0.
     *
     * The minimal repair of the same defect, and deliberately a DIFFERENT repair
     * from CA_TBL_PCS_WEIGHTED rather than a reparameterisation of it. WEIGHTED
     * rescales the terms and keeps the score continuous; GATED leaves the terms
     * alone and adds a discontinuity at the point where the uncertainty term has
     * saturated. If both behave the same way under the monotone control, that is
     * evidence about the CONCEPT rather than about either form -- which is the
     * question actually open after v2 failed.
     */
    CA_TBL_PCS_GATED
};

/** Tunables. Defaults reproduce the Python constants so results are comparable. */
/** Response policy when the integrity gate rejects a fix.
 *
 * MEASURED, and this enum exists because the default was never designed. A suspect
 * fix calls RevertToPrior() and skips the update (ca-tbl-policy.cc, the suspect branch
 * of Step()).
 *
 * CORRECTION 2026-09-30 (docs/SPEC_CONFORMANCE_PLAN.md sect 1.H item 1). This comment
 * used to say that RevertToPrior "restores the PRE-UPDATE covariance every cycle, so P
 * does not even grow across a rejection run -- the filter is not coasting, it is
 * stuck". Read against the code order that is wrong: RevertToPrior() runs before any
 * Update() of that cycle, when m_x / m_P already equal the post-Predict prior, so it is
 * a no-op and a rejected cycle is a plain coast -- P grows by Q each cycle. The lock-out
 * measured below is real; the mechanism given for it was not. It is the prior, not a
 * frozen covariance, that the gate keeps rejecting against. The gate's
 * reference is the prior, so once the prior is wrong every honest measurement
 * disagrees with it and the gate keeps firing:
 *
 *   F0 (NO FAULT INJECTED): unconditional suspect rate 0.0046, but
 *   P(suspect | suspect at previous cycle) = 0.409 -- an 89x jump -- and the filter
 *   can stay frozen for 22 consecutive cycles with nothing wrong.
 *   F1: 0.1135 -> 0.662, longest run 108 cycles. F3: 0.1665 -> 0.789, longest 251.
 *
 * The lock-out is therefore not the gate mishandling a fault; it freezes a healthy
 * filter off its own false positive.
 */
enum CaTblSuspectRecovery
{
    CA_TBL_RECOVERY_NONE = 0, //!< RevertToPrior only. The pre-existing behaviour.
    CA_TBL_RECOVERY_INFLATE,  //!< Scale P on rejection so the gate widens next cycle.
    CA_TBL_RECOVERY_BOUNDED,  //!< Force-accept after N consecutive rejections.
    CA_TBL_RECOVERY_REINIT,   //!< Discard the prior; reinitialise from the raw fix.
    /**
     * DC-7 (docs/SPEC_CONFORMANCE_PLAN.md): the spec's response, Physical 2.6 -- flag the
     * fix SUSPECT, do not let it alone justify a de-escalation, and "downweight in PCS
     * regardless of raw trace(Sigma)". The fix is ACCEPTED, not rejected: it is fused
     * with R x suspectInflateFactor (the TDoA half too), and the cycle's PCS is scaled by
     * suspectPcsFactor -- or, on v2's log scale, where a score can be negative and
     * scaling would raise it, lowered by log10(1/suspectPcsFactor). d.suspect stays
     * true, so the healthy-demotion count still refuses the cycle, and
     * suspectForcesEscalation still applies if left on.
     *
     * Why inflated and not full R: a persistent fault fused at full weight is absorbed
     * into the prior within a cycle, after which the check has nothing left to disagree
     * with. Inflating is OURS; the factor reuses INFLATE's 4.0.
     */
    CA_TBL_RECOVERY_DOWNWEIGHT
};

struct CaTblPolicyConfig
{
    CaTblPolicyMode mode;
    CaTblPcsForm pcsForm;

    double pcsThresholdUp[CA_TBL_TIER_COUNT];   //!< escalate when PCS falls below this
    double pcsThresholdDown[CA_TBL_TIER_COUNT]; //!< candidate for de-escalation above this
    uint32_t nDeescalate;                       //!< consecutive good cycles before demoting

    //! CA_TBL_PCS_WEIGHTED weights. HAND-SET, not calibrated -- CAL-01 asks for
    //! LOOCV and it has still not been done. Constrained so pcsW2 + pcsW3 falls
    //! below every pcsThresholdUp, which is what gives term 1 its veto back.
    double pcsW1;
    double pcsW2;
    double pcsW3;
    //! CA_TBL_PCS_GATED veto floor on term 1. OURS, not physical: the point below
    //! which the uncertainty term has saturated hard enough that the rest of the
    //! score should not be allowed to speak for it.
    double pcsGateFloor;
    //! PCS term 3 from the classifier (TECH_DEBT L.2.2, K.8). FALSE by default, and false
    //! reproduces every prior result. True: term 3 becomes the spec's (1 - NLOS_score),
    //! averaged over the links this cycle that carry a score (nlosScore >= 0), instead of
    //! the fraction of links the boolean tag calls LOS. With no scored link in the set it
    //! falls back to that fraction. Only the TERM changes: the LEGACY/WEIGHTED/GATED sums,
    //! their weights and thresholds are untouched, so the saturation defect (term2 + term3
    //! can reach 2.0 against 1.55) is exactly as it was. CA_TBL_PCS_V2 has no NLOS term,
    //! so the flag has nothing to feed there and the scenario refuses that combination.
    bool pcsNlosFromScore;
    //! Demote on the FIRST cycle where the active tier addressed zero anchors. A
    //! cycle with no anchors addressed is categorically different from one whose
    //! anchors failed to answer: it means the tier has nothing there at all, after a
    //! full discovery window. Measured 4.12 blind cycles per escalation against
    //! nDeescalate = 5, and 141 of 189 blind runs were exactly 5 cycles long -- the
    //! counter's signature. Default false preserves the pre-existing baseline.
    bool fastDemoteOnZeroNeighbours;
    //! On a blind demotion, drop to the highest tier BELOW the current one that is
    //! known to be populated, rather than stepping down one tier at a time. A node
    //! leaving a blind UWB tier otherwise spends further blind cycles at tier 2
    //! before reaching the floor.
    //!
    //! THE EVIDENCE MUST NOT COME FROM THE LIVE NEIGHBOUR TABLE. ClearNeighbours()
    //! runs on every sleeping tier every cycle, so for a node on UWB the tier-2
    //! table is empty because its radio is off, not because tier 2 is absent.
    //! Driving this off the live table would evaluate to "demote to 868" always --
    //! skipping tier 2 unconditionally and trading tier 2's 4.59 m median fix error
    //! for tier 1's 11.33 m at boxHalf 3000, i.e. the exact opposite of the intent.
    //! The caller therefore supplies a mask built from a per-tier last-non-empty
    //! timestamp that survives sleep. Reachability (can I range now?) stays with the
    //! live table; existence (was anything ever there?) comes from the timestamp.
    bool multiStepDemote;

    // ---- Switching-point integrity: cross-validating the CONFIDENCE SIGNAL --------
    //
    // Both default OFF, so the check can be measured on against off -- which is the
    // comparison the brief asks for, and it is not a comparison you can make if the
    // mechanism is always on.

    //! Refuse an escalation when the filter's displacement disagrees with the body's.
    /** Hop-provenance form. 0 = the legacy linear subtraction, kept so prior results
     *  reproduce. 1 = bounded multiplicative on the relayed VARIANCE FRACTION, which is
     *  what docs/RELAYED_RANGING_ERROR_MODEL.md section 5 specifies. */
    uint32_t hopProvenanceForm;
    //! Strength of the bounded penalty. 0 disables it whatever the form.
    double hopPenalty;

    bool imuCrossCheck;
    //! How many combined sigmas of disagreement are tolerated before the confidence
    //! signal is treated as untrustworthy. OURS, not cited.
    double imuConsistencyK;
    //! Floor on the sigma of the comparison, metres, so a stationary node with a tiny
    //! filter sigma does not trip on millimetre disagreements.
    double imuSigmaFloorM;

    //! Refuse an escalation when too few neighbours corroborate this node's estimate.
    bool corroborationCheck;
    //! Fraction of checkable neighbours that must agree. OURS, not cited.
    double corroborationFloor;
    //! Minimum number of neighbours that must be checkable before the test may refuse
    //! anything at all -- with one witness, "no corroboration" is not evidence.
    uint32_t corroborationMinWitnesses;

    /**
     * Whether a suspect fix also forces an escalation.
     *
     * When true (the default, and the pre-existing behaviour), `suspect` sets
     * `wantEscalate = true` unconditionally: the argument is that a distrusted fix
     * is itself grounds to escalate, because the coarse tier has stopped being
     * believable exactly when the precision tier earns its energy. That is
     * defensible as a POLICY.
     *
     * It also makes the integrity check unmeasurable AS AN INTEGRITY CHECK. The
     * gate's escalation side effect dominates any accuracy effect it has, so every
     * comparison of ca_tbl_full against ca_tbl_no_integrity is measuring the
     * escalation, not the rejection. The confound is small when the gate fires 5
     * times in 28,800 cycles; it is large now that the all-LOS precondition is
     * satisfiable and firings have risen by two orders of magnitude.
     *
     * Set false to isolate the mechanism the report actually claims in §2.3 and
     * §10.5: a suspect fix is REJECTED (the prior is kept) and nothing else
     * happens. With this flag the three arms separate cleanly --
     *
     *   PCS alone                 ca_tbl_no_integrity
     *   PCS + gate as rejector    ca_tbl_full, suspectForcesEscalation = false
     *   PCS + gate as trigger     ca_tbl_full, suspectForcesEscalation = true
     *
     * -- and §10.5 becomes measurable at all. Without it the gate's effect always
     * arrives mixed with an escalation it caused, however good the eventual S7
     * fault injection is.
     */
    bool suspectForcesEscalation;

    double mahalanobisThreshold; //!< chi-square gate, 2 DOF
    //! Use the correctly specified integrity gate: weighted multilateration for the
    //! test point, and S = P_prior_pos + (J^T R^-1 J)^-1 for the distance. Both
    //! defects are one mechanism -- a biased numerator against a denominator missing
    //! R -- so they are fixed under a single flag. Default false so the pre-existing
    //! baseline reproduces byte-identically.
    bool mahalUseR;

    /**
     * Escalate only into a tier the node has grounds to believe can deliver.
     *
     * THE DEFECT THIS CLOSES. `wantEscalate` is formed from PCS alone -- traceSigma,
     * GDOP and LOS fraction, all properties of the fix currently held -- and the
     * step-up that follows consults nothing about the destination. Measured at
     * boxHalf=1000: the arm escalates 2.4GHz->UWB 194 times with a mean of 0.08 UWB
     * anchors visible against the 3 required, spends 67.1% of the run on a tier that
     * cannot produce a fix, and takes 86.9% of the fixes made while nominally on UWB
     * from the 868 floor. 493.9 J of 511.1 J total is UWB energy buying nothing.
     *
     * That is a defect against claim 1 as written -- "escalation happens only when
     * position confidence justifies the energy" -- because escalating into zero
     * rangeable anchors cannot justify any energy. Closing it implements the claim
     * rather than tuning toward it.
     *
     * Note the pre-existing asymmetry it corrects: the DEMOTION path already has a
     * full apparatus for "this tier has nothing here" (activeDelivered,
     * fastDemoteOnZeroNeighbours, multiStepDemote, tierEvidenceMask). The system
     * could discover a tier was empty and leave, but could not decline to enter.
     *
     * BOTH steps are gated, by different evidence, because different evidence exists:
     *
     *   868 -> 2.4GHz   OBSERVED. tierEvidenceMask carries a recent non-empty
     *                   neighbour table for tier 2, refreshed every cycle from the
     *                   MAC cache for ALL tiers regardless of which is active, so it
     *                   stays fresh while the node sits on tier 1 and cannot latch.
     *
     *   2.4GHz -> UWB   PREDICTED. UWB anchors are not discoverable at range, so
     *                   there is nothing to observe until after the escalation that
     *                   the observation is meant to justify. The node predicts from
     *                   its own EKF estimate and the anchor coordinates it must hold
     *                   to multilaterate at all.
     *
     * Use observed evidence where it exists; predict only where it does not.
     */
    //! Response policy on rejection. Default NONE reproduces the pre-existing
    //! baseline byte-identically. See CaTblSuspectRecovery for the measurement.
    CaTblSuspectRecovery suspectRecovery;
    //! INFLATE: factor applied to P on each rejection. Releasing F3 needs
    //! P ~ 221^2/9.21 = 5300 against a measured 1621, i.e. ~3.3x.
    double suspectInflateFactor;
    //! BOUNDED/REINIT: consecutive rejections before the recovery action fires.
    //! Derived from the p90 of measured run lengths (5 on F1, 7 on F3) so it engages
    //! on the tail and not on the median, which is 1-2.
    uint32_t suspectRecoveryN;
    bool escalationRequiresEvidence;
    //! 868 -> 2.4 GHz gate. REBUILT 2026-09-08 as a PREDICTOR; still defaults off.
    //!
    //! The original asked the MAC neighbour cache whether tier 2 was audible, and the
    //! tier-2 radio sleeps while another tier is active, so the count was always 0 and
    //! enabling it latched the node at the floor -- 1440/1440 cycles on 868, 0
    //! escalations. That version is gone.
    //!
    //! The replacement is symmetric with uwbEvidenceFromNeighbours rather than with the
    //! map form: the ranges handed to Step() were taken by an AWAKE radio, so a
    //! neighbour ranged closer than t2PredictRadiusM is a neighbour 2.4 GHz would
    //! reach. Geometry rather than a map, so it follows finders.
    bool escalationRequiresEvidenceT2;
    //! Radius for the tier-2 prediction. 156.0 m, the probe's r80 AFTER the
    //! environment correction. The pre-correction figure was 909.5 m and is 5.8x too
    //! generous -- a predicate at that radius admits escalations into a tier that
    //! cannot reach, which is the failure it exists to prevent.
    double t2PredictRadiusM;
    //! Neighbours inside that radius before escalation is allowed. 3, because three
    //! is what a fix needs. The UWB predicate uses presence rather than count because
    //! tier 3 is a contact indicator; tier 2 is a localisation tier and one reachable
    //! anchor cannot solve.
    uint32_t t2PredictMinAnchors;
    //! Radius for the UWB prediction. NOT nominalRangeM[UWB] = 30.0, which is the
    //! trigger oracle's labelling constant, and not the measured r80 of 12.8 m, which
    //! declines two thirds of the cycles UWB would have served.
    double uwbPredictRadiusM;
    //! The prediction is only as good as the estimate feeding it. Escalation is
    //! blocked unless sqrt(traceSigma) < uwbPredictMaxSigmaFrac * uwbPredictRadiusM,
    //! so a tier-1 fallback fix at ~129 m error cannot drive a 20 m predicate.
    double uwbPredictMaxSigmaFrac;
    //! Tier-3 anchor coordinates, supplied once by the scenario. Empty disables the
    //! UWB predicate regardless of the flag -- a policy with no map cannot predict.
    std::vector<double> uwbAnchorX;
    std::vector<double> uwbAnchorY;
    /**
     * Take the UWB half of the evidence gate from the LIVE neighbour table instead
     * of the static anchor map above. FALSE by default.
     *
     * That map is a snapshot taken once at scenario start. It was adequate while
     * tier 3 sat in fixed rings around facility sites. Under the corrected tri-band
     * topology it is not: UWB now comes from FINDERS, which move and are never in
     * the map, so the predicate answers a question about a deployment the system no
     * longer has.
     *
     * The stated reason for predicting rather than observing -- "UWB anchors are
     * not discoverable at range" -- is also false under that topology. Finders
     * beacon on every tier, so a UWB neighbour IS discoverable, and
     * tierEvidenceMask already carries exactly that bit, refreshed every cycle for
     * all tiers regardless of which one is active. The 868 -> 2.4GHz half of this
     * same gate has always used it. This gives the UWB half the same treatment.
     *
     * PRESENCE, NOT COUNT, and deliberately. A fix needs three anchors; this asks
     * only whether the tier has anything here at all. That is the question a
     * suppress-only gate should ask -- it exists to stop the node paying UWB rates
     * to sit in an empty tier, not to predict whether a fix will solve. Counting
     * would need a per-tier neighbour vector threaded through Step(), and the
     * binary form is symmetric with the sibling half that already works.
     *
     * When on, the static map is NOT consulted as a fallback. A second way to pass
     * would defeat the gate, and observation subsumes the map here: a gateway close
     * enough to be inside the predicate radius is close enough to have been heard.
     */
    bool uwbEvidenceFromNeighbours;
    /**
     * Target radial position accuracy, as a variance: (3.162 m)^2 = 10.0 m^2.
     *
     * A SPECIFICATION, not a tuned constant, and stating it as the target rather
     * than the square is the point -- a reader can disagree with 3.162 m and cannot
     * disagree with 10.0.
     *
     * Its consequence is measured, not asserted. PCS clears its own escalation
     * threshold on 95.8% of UWB fixes (mean radial sigma 0.23 m) and on 0.3% of
     * tier-1 fixes (53.87 m). So the confidence model reduces to "escalate unless
     * already on UWB" BY DESIGN: the behaviour follows from this number, and it was
     * never written down. If the target should be tier-dependent, say so here.
     */
    double traceRef;
    double hopWeight;            //!< PCS hop-provenance weight -- see note below
    double fixedVarThresholdM2;  //!< CA_TBL_MODE_FIXED_THRESHOLD trigger

    //! Per-tier LOS/NLOS range std-dev. FALLBACK ONLY -- used when a measurement
    //! carries no sigmaM (RANGING_SYNTHETIC_GPS_DENIED). Do not treat these as the
    //! model; they are order-of-magnitude wrong against the measured error.
    double losSigmaM[CA_TBL_TIER_COUNT];
    double nlosSigmaM[CA_TBL_TIER_COUNT];
    //! Variance inflation applied to an NLOS-tagged measurement's physics sigma, to
    //! stand in for the unmodelled NLOS excess-delay bias. UNCALIBRATED.
    double nlosSigmaInflation;

    double processNoiseStdPos;
    double processNoiseStdVel;

    bool instantEscalate; //!< if true, a new tier takes effect the same cycle

    //! DOWNWEIGHT (DC-7): what a suspect cycle's PCS is multiplied by, in (0, 1]. 0.5 is
    //! OURS, not calibrated: "downweight" with no magnitude given, halved. Appended
    //! last so every existing member keeps its offset.
    double suspectPcsFactor = 0.5;

    /**
     * \brief Apply the pre-registered v2 thresholds.
     *
     * Kept beside the form itself rather than in the scenario so that the
     * derivation cannot drift away from the score it belongs to.
     *
     * UP = 0.0, every tier. The score is zero exactly when sigma_eff equals the
     * 3.1623 m target that traceRef already documents as a SPECIFICATION, so the
     * threshold is that specification restated on a log scale, not a free
     * parameter. ONE target for all three tiers: an accuracy requirement does not
     * change according to which radio is being used to meet it, and per-tier
     * thresholds would reintroduce the two free parameters the redesign exists to
     * remove.
     *
     * DOWN = log10(2) = 0.301, every tier. Demote only once uncertainty is better
     * than HALF the target -- one octave of hysteresis, a round choice fixed in
     * advance rather than the best of several tried. One score unit is one decade
     * of sigma_eff, so this constant means a factor of 2 as stated. Down above up
     * preserves the deliberate SAR bias that a missed escalation costs more than a
     * wasted one.
     *
     * The legacy 1.55/1.70/2.30 and 1.75/1.90/2.55 are NOT carried across: they
     * were hand-set for a differently-scaled score and were never calibrated even
     * for that.
     */
    void ApplyV2Thresholds();


    CaTblPolicyConfig();
};

/** Everything the policy decided and observed in one cycle. */
struct CaTblDecision
{
    CaTblTier activeTier; //!< tier that actually transmitted this cycle
    CaTblTier nextTier;   //!< tier that will transmit next cycle
    bool hadFix;          //!< enough anchors to solve this cycle
    bool escalated;
    bool deescalated;
    bool suspect;         //!< integrity check fired
    bool measurementRejected;

    double pcs;
    double gdop;
    double traceSigma;
    double mahalanobis;
    double estX;
    double estY;
    uint32_t nAnchorsUsed;
    //! Links tagged NLOS by the OPERATIVE flag -- i.e. under --nlosSource. This is
    //! what actually drove R, the LOS fraction and the all-LOS gate this cycle.
    uint32_t nNlos;
    //! The two candidate populations, counted over the same measurement set, and
    //! always both emitted whichever source is selected. Under --nlosSource=legacy
    //! nNlos is their union; under =propagation it equals nNlosPropagation. Emitting
    //! all three makes the size of the defect a measurement rather than an argument.
    uint32_t nNlosPropagation;
    uint32_t nHiddenChannel;
    //! sqrt(mean(varM2)) -- exactly what a scaled ComputePcs would receive.
    //! CONTAMINATED by nlosSigmaInflation, which is applied to ~75% of links
    //! because meas[i].nlos reads a 4-state Markov regime and not an NLOS flag.
    double meanSigmaM;
    //! The same before nlosSigmaInflation. Emitting both separates the question
    //! "does gdop * sigma order the tiers correctly" from the NLOS defect, so the
    //! premise behind a scaled PCS term can be tested now rather than after the
    //! NLOS chain lands. Their ratio measures the inflation's effect on the
    //! confidence model as a by-product.
    double meanSigmaRawM;
    //! This cycle's measurement-only radial position sigma, the exact
    //! sqrt(trace((J^T R^-1 J)^-1)). Negative when the weighted solve failed
    //! (degenerate geometry) or when the legacy form is active and it was never
    //! computed. Emitted so the v2 per-term variance decomposition required by
    //! pre-registration §0.4a can be done from the trace alone.
    double sigmaGeoM;
    //! UWB anchors the node's own estimate predicts are in range this cycle. -1 when
    //! the predicate did not run (flag off, wrong tier, or blocked by the
    //! estimate-quality gate before the count was computed).
    int32_t predictedVisUwb;
    //! Neighbours ranged inside t2PredictRadiusM this cycle, or -1 when the
    //! tier-2 predicate did not run. The counterpart of predictedVisUwb.
    int32_t predictedVis2p4;
    //! Links in this cycle's set that carried a classifier NLOS_score (nlosScore >= 0).
    uint32_t nNlosScored;
    //! mean(1 - NLOS_score) over those links: the spec's PCS term 3. Computed on every
    //! fix cycle that has a scored link, whether or not it is fed to PCS, so the term can
    //! be observed before it is allowed to decide anything; -1 when no link was scored.
    double nlosScoreTerm;
    //! Whether nlosScoreTerm replaced the tag's LOS fraction inside ComputePcs this cycle.
    bool nlosScoreFed;

    CaTblDecision();
};

/**
 * \brief Range-only EKF over the constant-velocity state [x, y, vx, vy].
 *
 * h(x) = ||pos - anchor|| is nonlinear, so the Jacobian is recomputed at every
 * update rather than held fixed -- a wrong or stale Jacobian is the single most
 * common EKF defect and is worth stating explicitly.
 */
class CaTblEkf
{
  public:
    CaTblEkf();

    void Init(double x, double y, double vx, double vy, double posVar, double velVar);
    void SetProcessNoise(double stdPos, double stdVel);

    /** Constant-velocity prediction. Stores the prior for the integrity test. */
    void Predict(double dt);

    /** Sequential scalar update with per-measurement variance. */
    void Update(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2);

    /** Fuse an absolute POSITION observation with isotropic variance \p varM2.
     *
     *  The filter's only other measurement path is a range against an anchor,
     *  which is why dead reckoning could not be fused before: it produces a
     *  position and there was no door for one.
     *
     *  Implemented as two sequential scalar updates with H = [1 0 0 0] and
     *  [0 1 0 0]. With a diagonal R that is algebraically the joint 2x2 update,
     *  and it reuses the scalar path whose numerical behaviour -- symmetry
     *  enforcement included -- is already exercised by Update().
     *
     *  Velocity moves too, through P[2][0] and P[3][1]. The prediction step
     *  builds that correlation, so a position observation carries velocity
     *  information; withholding it would discard evidence.
     *
     *  DOES NOT TOUCH THE STORED PRIOR. m_xPrior is what the integrity gate
     *  tests this cycle's radio fix against. If a dead-reckoned position became
     *  that reference, the gate would be validating the radio against the
     *  inertial engine instead of against the filter's own prediction. */
    void FusePosition(double zx, double zy, double varM2);

    /** Fuse an absolute POSITION observation with a FULL 2x2 covariance R.
     *
     *  FusePosition() takes an isotropic variance, which is right for a source whose
     *  error has no direction (dead reckoning). A multilateration fix is not such a
     *  source: its covariance is (J' Q^-1 J)^-1 and is elongated along whatever
     *  direction the anchor geometry resolves worst. Rounding it to its trace would
     *  over-trust that direction and under-trust the other, so this is the joint 2x2
     *  update with R as given: S = P_pos + R, K = P H' S^-1, P <- P - K H P.
     *
     *  Refuses -- returns false and changes nothing -- when R is not positive definite
     *  or S is singular: a covariance that claims certainty in some direction is a
     *  defect upstream, not information. Like FusePosition(), it leaves the stored
     *  prior alone, so the integrity gate's reference is still the prediction. */
    bool FusePositionCov(double zx, double zy, double rxx, double rxy, double ryy);

    /** Discard this cycle's measurement: keep the prior as the posterior. */
    void RevertToPrior();

    /** Scale the position block of P. Used by CA_TBL_RECOVERY_INFLATE so that a
     *  rejection widens the gate on the next cycle instead of leaving it unchanged. */
    void InflatePos(double factor)
    {
        m_P[0][0] *= factor;
        m_P[1][1] *= factor;
    }

    double X() const { return m_x[0]; }
    double Y() const { return m_x[1]; }
    double Vx() const { return m_x[2]; }
    double Vy() const { return m_x[3]; }

    double TracePos() const { return m_P[0][0] + m_P[1][1]; }

    /** The 2x2 POSITION covariance, not just its trace.
     *
     *  C5 needs the matrix, not the scalar: a relayed measurement inflates the range
     *  variance by u^T P u, the relay's uncertainty projected onto the RANGE
     *  DIRECTION. The trace throws the direction away, and a relay whose error is
     *  strongly directional -- which is the normal case, since anchor geometry is
     *  never isotropic -- would be inflated by the wrong amount in both directions.
     *  Pxy is the term that carries that, and it is the one a diagonal
     *  approximation silently drops. */
    double Pxx() const { return m_P[0][0]; }
    double Pxy() const { return m_P[0][1]; }
    double Pyy() const { return m_P[1][1]; }
    double PriorX() const { return m_xPrior[0]; }
    double PriorY() const { return m_xPrior[1]; }

    /** Mahalanobis distance of \p (zx,zy) from the stored prior, 2 DOF.
     *
     *  MIS-SPECIFIED: normalises by the prior covariance alone. A validation gate
     *  must normalise by the innovation covariance S = H P H^T + R; this omits R
     *  entirely, so the statistic scales with the measurement noise it is supposed
     *  to be discounting. Tier 1 has by far the largest R in the system (range
     *  error ~50% of true distance), which is why every firing observed at
     *  boxHalf 3000 was on a tier-1 fallback fix. Kept as the default only so the
     *  pre-existing baseline remains reproducible -- see MahalanobisFromPriorWithR. */
    double MahalanobisFromPrior(double zx, double zy) const;

    /** As above but forming S = P_prior_pos + Rfix, with \p rxx/rxy/ryy the
     *  covariance of the raw multilateration fix. This is the correctly specified
     *  gate; select it with CaTblPolicyConfig::mahalUseR. */
    double MahalanobisFromPriorWithR(double zx,
                                     double zy,
                                     double rxx,
                                     double rxy,
                                     double ryy) const;

  private:
    double m_x[4];
    double m_P[4][4];
    double m_xPrior[4];
    double m_PPrior[4][4];
    double m_qPos;
    double m_qVel;
};

/**
 * \brief Tier-selection policy: filter, confidence score, integrity gate, FSM.
 */
class CaTblPolicy
{
  public:
    CaTblPolicy();

    void Configure(const CaTblPolicyConfig& config);
    const CaTblPolicyConfig& GetConfig() const { return m_config; }

    /** Seed the filter, typically from the first known position. */
    void Initialize(double x0, double y0, double posStdM = 100.0, double velStdMps = 10.0);

    /** Tier that should transmit on the next cycle. */
    CaTblTier GetActiveTier() const { return m_tier; }

    /** Independent evidence for this cycle, supplied by the caller because the policy
     *  has no radio and no inertial sensor of its own.
     *
     *  \param imuDisplacementM  distance the body is believed to have moved since the
     *                           previous cycle, from dead reckoning
     *  \param corroborating     neighbours whose reported position is consistent with
     *                           this node's estimate given the measured range to them
     *  \param corroborable      neighbours against which that test could be run at all
     */
    /** What fraction of this cycle's measurement information arrived through a
     *  relay, by variance share. Supplied by the caller because the policy does not
     *  see where a measurement came from. */
    void SetRelayedVarFraction(double f) { m_relayedVarFrac = f; }

    void SetCrossCheck(double imuDisagreementM, double imuToleranceM,
                       int32_t corroborating, int32_t corroborable)
    {
        m_imuDispM = imuDisagreementM;
        m_imuTolM = imuToleranceM;
        m_corrOk = corroborating;
        m_corrTotal = corroborable;
        m_crossCheckFresh = true;
    }

    //! Escalations refused because the body and the filter disagreed about movement.
    uint64_t GetImuRejections() const { return m_imuRejections; }
    //! Escalations refused because too few neighbours agreed with the estimate.
    uint64_t GetCorroborationRejections() const { return m_corrRejections; }

    //! Position estimate and its covariance -- what a RELAY must report for its
    //! measurement to be admissible at all. See ca-tbl-relay-admission.h.
    double EstX() const { return m_ekf.X(); }
    double EstY() const { return m_ekf.Y(); }
    /** Fuse an absolute position observation into the filter -- the door the
     *  inertial engine needs, since every other measurement path takes a range.
     *  The caller owns the error model: this takes the variance it is given and
     *  does not invent one, because only the caller knows how long the engine has
     *  been running unaided. See CaTblEkf::FusePosition. */
    void FusePosition(double zx, double zy, double varM2)
    {
        m_ekf.FusePosition(zx, zy, varM2);
    }

    double Pxx() const { return m_ekf.Pxx(); }
    double Pxy() const { return m_ekf.Pxy(); }
    double Pyy() const { return m_ekf.Pyy(); }

    /** Queue one absolute position observation with its full covariance for the NEXT
     *  Step() to fuse -- tier-1 TDoA, which the caller solves from the same broadcast
     *  whose RSSI ranges Step() receives.
     *
     *  WHY INSIDE Step() AND NOT BEFORE OR AFTER IT. Before, it would be fused into the
     *  previous cycle's posterior ahead of Predict(). After, this cycle's trace(P) and
     *  PCS -- what the escalation decision reads -- would not contain it. Step() fuses it
     *  straight after the range update, so the filter, the confidence score and the
     *  decision all see the COMBINED tier-1 fix.
     *
     *  IT FOLLOWS THE CYCLE'S VERDICT. The two observables come from one broadcast, so
     *  a cycle the Switching-Point Integrity Check rejects (reverted to the prior, or
     *  reinitialised from the raw RSSI fix) fuses neither, and a cycle it force-accepts
     *  (the BOUNDED recovery) fuses both. A no-fix cycle fuses nothing. Our choice, not
     *  cited: the check is not run on the TDoA fix itself.
     *
     *  Consumed by exactly one Step() whatever it decides, so a queued fix never outlives
     *  the cycle it was measured in. Never called, it changes nothing. */
    void SetAuxPositionFix(double zx, double zy, double rxx, double rxy, double ryy)
    {
        m_auxPending = true;
        m_aux[0] = zx;
        m_aux[1] = zy;
        m_aux[2] = rxx;
        m_aux[3] = rxy;
        m_aux[4] = ryy;
    }

    //! Whether the last Step() fused a queued position observation.
    bool AuxFusedLast() const { return m_auxFusedLast; }
    //! Queued position observations fused, over the policy's life.
    uint64_t GetAuxFusedCount() const { return m_auxFused; }

    /**
     * \brief Advance one cycle.
     *
     * \param dt        seconds since the previous call
     * \param meas      range observations the EKF should fuse this cycle. On a
     *                  concurrent-tier arm these may come from the TIER-1 FLOOR
     *                  rather than from the active tier, when the active tier
     *                  returned too few ranges to solve.
     * \param hopCount  relay hops the fix traversed. Currently only recorded;
     *                  it feeds PCS only when hopWeight > 0. Left at zero
     *                  weight by default because there is no mesh yet -- see
     *                  the S5 step in CA_TBL_SIM_AUDIT.md. Do not claim
     *                  hop-provenance confidence until this is exercised.
     * \param activeTierRanges
     *                  How many ranges the ACTIVE tier returned, which is not the
     *                  same as meas.size() once a tier-1 fallback exists. Pass -1
     *                  for the legacy behaviour of inferring it from meas.size().
     *
     *                  This parameter exists because omitting it silently broke
     *                  de-escalation. The dropout path below counts a cycle in which
     *                  the active tier failed to deliver toward demotion, so a node
     *                  cannot strand itself on a tier it has walked out of. Once the
     *                  caller began substituting tier-1 ranges on failure,
     *                  meas.size() was always >= 3, the dropout path never ran, and
     *                  the policy never learned that its active tier was dead. The
     *                  measured consequence: ca_tbl_full latched onto UWB after 4
     *                  escalations and stayed there for the remaining 1436 cycles
     *                  while UWB was in coverage only 10.6% of the time -- paying UWB
     *                  energy every cycle, falling back to tier 1 for the fix, and
     *                  landing at tier-1 accuracy (121 m RMSE) for 3.3x tier-1 energy.
     * \param activeTierNeighbours
     *                  How many neighbours the ACTIVE tier currently has in its
     *                  table, or -1 for unknown (the legacy behaviour). Distinct
     *                  from activeTierRanges: zero neighbours means the tier
     *                  addressed nobody, whereas zero ranges from a non-empty table
     *                  means it asked and got no answer. Only read when
     *                  CaTblPolicyConfig::fastDemoteOnZeroNeighbours is set.
     * \param tierEvidenceMask
     *                  Bit k set if tier k is believed to be populated. Must be
     *                  built from a last-non-empty timestamp that survives sleep,
     *                  NOT from the live neighbour table -- see the note on
     *                  CaTblPolicyConfig::multiStepDemote. Zero means no evidence,
     *                  which reduces multi-step demotion to one-step.
     */
    CaTblDecision Step(double dt,
                       const std::vector<CaTblRangeMeasurement>& meas,
                       uint32_t hopCount = 0,
                       int32_t activeTierRanges = -1,
                       int32_t activeTierNeighbours = -1,
                       uint32_t tierEvidenceMask = 0);

    /**
     * \brief Undo the tier change the last Step() made, because the escalation
     *        handshake could not be transmitted inside the duty-cycle budget.
     *
     * The escalation request/grant is an ACK'd exchange on the tier being LEFT. If
     * the regulator will not permit that transmission, the handoff cannot be
     * signalled, and a handoff that cannot be signalled has not happened -- the
     * node must stay where it is and retry. Without this the simulation would let
     * the tier change take effect while quietly not sending the message that
     * effects it.
     *
     * Restores the tier and un-counts the escalation, so `escalations` keeps
     * meaning "handoffs that occurred" rather than "handoffs attempted"; the
     * attempts are counted separately as deferrals by the caller.
     *
     * DEMOTION IS DELIBERATELY NOT ROLLED BACK by the caller. A node must be able
     * to abandon a tier it cannot reach; requiring a successful handshake to leave
     * would strand it on a dead tier, which is precisely the failure the
     * fastDemote/multiStepDemote work exists to prevent.
     */
    void AbortTierChange(CaTblTier restoreTo);

    uint64_t GetEscalationCount() const { return m_escalations; }
    //! Escalations suppressed because the estimate was too poor to predict with.
    //! Times the recovery action fired (INFLATE counts every rejection).
    uint64_t GetRecoveryFirings() const { return m_recoveryFirings; }
    //! REINIT cycles where the fix covariance was unavailable and P was seeded from
    //! the 100 m^2 fallback -- a SECOND treatment, which must never be averaged in
    //! silently. With mahalUseR=true this should be 0; the caller announces it if not.
    uint64_t GetReinitFallbackCount() const { return m_reinitFallbackCount; }
    uint64_t GetPredictBlockedQuality() const { return m_predictBlockedQuality; }
    //! Escalations suppressed because fewer than 3 UWB anchors were predicted in range.
    uint64_t GetPredictBlockedCoverage() const { return m_predictBlockedCoverage; }
    //! 868->2.4 escalations suppressed because tier 2 had no recent neighbour evidence.
    uint64_t GetEvidenceBlockedT2() const { return m_evidenceBlockedT2; }
    uint64_t GetDeescalationCount() const { return m_deescalations; }
    uint64_t GetSuspectCount() const { return m_suspects; }
    //! DOWNWEIGHT (DC-7): suspect fixes accepted at inflated R with PCS downweighted.
    //! A subset of GetSuspectCount(); zero under every other recovery policy.
    uint64_t GetDownweightedFixes() const { return m_downweightedFixes; }
    uint64_t GetNoFixCount() const { return m_noFixCycles; }

    /** Per-tier count of cycles spent with that tier active. */
    uint64_t GetTierCycles(CaTblTier tier) const { return m_tierCycles[tier]; }

    // -- exposed for unit testing and for reuse by other scenarios ------------

    /** GDOP from anchor geometry about \p (x,y). Returns 100 on singular geometry. */
    static double ComputeGdop(double x, double y, const std::vector<CaTblRangeMeasurement>& meas);

    /** Gauss-Newton range-only least squares. Filter-free reference fix.
     *
     *  UNWEIGHTED: every range gets equal say regardless of its sigma. On tier 1,
     *  where sigma is ~50% of true distance, an anchor at 400 m and one at 2500 m
     *  differ by ~6x in sigma and ~40x in variance, and this solver treats them
     *  identically -- so the point being tested by the integrity gate is itself
     *  biased, not merely its covariance. Kept as the default only for baseline
     *  reproducibility; prefer RawMultilaterationWeighted. */
    static bool RawMultilateration(const std::vector<CaTblRangeMeasurement>& meas,
                                   double x0,
                                   double y0,
                                   double& outX,
                                   double& outY);

    /** Weighted Gauss-Newton: normal equations scaled by R^-1. Also returns the
     *  fix covariance (J^T R^-1 J)^-1, which is exactly the R the integrity gate
     *  needs to form S. Preferred over going via ComputeGdop(), because GDOP is
     *  defined for equal per-range sigma -- the assumption tier 1 violates hardest. */
    static bool RawMultilaterationWeighted(const std::vector<CaTblRangeMeasurement>& meas,
                                           const std::vector<double>& varM2,
                                           double x0,
                                           double y0,
                                           double& outX,
                                           double& outY,
                                           double& outRxx,
                                           double& outRxy,
                                           double& outRyy);

  private:
  private:
    //! Bounded multiplicative provenance penalty; identity unless form 1 is set.
    double ApplyProvenance(double pcs) const;

    //! Fuse the queued TDoA position (SetAuxPositionFix) and count it if it went in.
    void FuseAux();
    //! FuseAux with the queued covariance scaled by \p rScale (DOWNWEIGHT, DC-7). A
    //! separate function so FuseAux itself is untouched.
    void FuseAuxInflated(double rScale);

    //! \p losTermOverride >= 0 replaces term 3 (nLos / nTotal) and nothing else; the
    //! default -1 leaves every form exactly as it was.
    double ComputePcs(double traceSigma,
                      double gdop,
                      uint32_t nLos,
                      uint32_t nTotal,
                      uint32_t hopCount,
                      double sigmaGeoM,
                      double losTermOverride = -1.0) const;

    CaTblPolicyConfig m_config;
    CaTblEkf m_ekf;
    double m_imuDispM = 0.0;
    double m_imuTolM = 0.0;
    double m_relayedVarFrac = 0.0;
    int32_t m_corrOk = 0;
    int32_t m_corrTotal = 0;
    bool m_crossCheckFresh = false;
    double m_prevEstX = 0.0;
    double m_prevEstY = 0.0;
    bool m_havePrevEst = false;
    uint64_t m_imuRejections = 0;
    uint64_t m_corrRejections = 0;
    CaTblTier m_tier;
    uint32_t m_deescalateCounter;

    uint64_t m_escalations;
    uint32_t m_consecutiveRejections;
    uint64_t m_reinitFallbackCount;
    uint64_t m_recoveryFirings;
    uint64_t m_predictBlockedQuality;
    uint64_t m_predictBlockedCoverage;
    uint64_t m_evidenceBlockedT2;
    uint64_t m_deescalations;
    uint64_t m_suspects;
    uint64_t m_noFixCycles;
    uint64_t m_tierCycles[CA_TBL_TIER_COUNT];

    // Tier-1 TDoA hand-off (SetAuxPositionFix). Appended last so every existing member
    // keeps its offset.
    bool m_auxPending = false;
    bool m_auxFusedLast = false;
    double m_aux[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    uint64_t m_auxFused = 0;
    // DC-7. Appended last for the same reason.
    uint64_t m_downweightedFixes = 0;
};

} // namespace ns3

#endif // CA_TBL_POLICY_H
