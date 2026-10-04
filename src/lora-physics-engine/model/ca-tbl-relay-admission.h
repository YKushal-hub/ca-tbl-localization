#ifndef CA_TBL_RELAY_ADMISSION_H
#define CA_TBL_RELAY_ADMISSION_H

#include "ca-tbl-mac.h"

#include <cstdint>
#include <set>

namespace ns3
{

/**
 * \file
 * \brief C5 — admission and covariance inflation for a RELAYED range.
 *
 * THE CATEGORY ERROR THIS EXISTS TO PREVENT. A relayed range is not a range.
 * `r_AB + r_BG` is an upper bound on `|x_A - x_G|` by the triangle inequality, tight
 * only when the three are collinear. Handing it to the filter as if it were a direct
 * range is the same class of mistake as applying a ToA Cramér–Rao bound to RSSI
 * inversion — which this project has already made once, and which
 * `lora-ranging-submodule.cc:151` calls a category error in its own comment.
 *
 * What a relay actually supplies is a **pseudo-anchor with a covariance**. The relay's
 * own position uncertainty projects onto the range direction and inflates the
 * measurement noise:
 *
 *     sigma_eff^2 = sigma_range^2 + u^T P_B u
 *
 * with `u` the unit vector from the relay toward the measured node. That projection —
 * not a scalar fudge factor — is what makes the measurement usable.
 *
 * WHY ADMISSION IS SEPARATE FROM WEIGHTING, and why five conditions rather than one
 * threshold: each condition marks a place where the *model above* stops holding, not a
 * place where the measurement is merely poor. A poor measurement can be down-weighted.
 * A measurement whose error model is wrong cannot, because the weight would be computed
 * from the same wrong model. Full derivation and the falsifiable test attached to each
 * condition: `docs/RELAYED_RANGING_ERROR_MODEL.md` §2–§4.
 *
 * NOT IMPLEMENTED HERE, and deliberately. Admitted measurements are **not yet fed to
 * the EKF**. Doing so requires the relay to transmit its covariance, which serialises
 * three doubles, which changes airtime and therefore energy — and the §6 invariant
 * (exactly one tier transmits per cycle and only that tier is charged) has to be
 * re-established at that point, not assumed. That is the next step in
 * `docs/RELAYED_RANGING_ERROR_MODEL.md` §6, and it is a separate change.
 */

//! Why a relayed range was refused. Ordered as A1–A5 are checked.
enum CaTblRelayVerdict
{
    CA_TBL_RELAY_ADMIT = 0,
    CA_TBL_RELAY_REJECT_TIER,      //!< A1: tier 1 error is a correlated shadowing field
    CA_TBL_RELAY_REJECT_DEPTH,     //!< A2: depth beyond the uncertainty budget
    CA_TBL_RELAY_REJECT_GEOMETRY,  //!< A3: relay too close relative to its own uncertainty
    CA_TBL_RELAY_REJECT_STALE,     //!< A4: P_B older than the relay's own fix interval
    CA_TBL_RELAY_REJECT_DUPLICATE  //!< A5: this relay already contributed this update
};

//! One candidate relayed range, with everything the admission test needs.
struct CaTblRelayedRange
{
    CaTblRelayedRange();

    uint16_t relayId;         //!< the relay B
    CaTblTier tier;           //!< tier the relayed leg was measured on
    uint8_t hopDepth;         //!< hops the measurement traversed
    //! Hard ceiling on relay depth. 1 reproduces the single-hop behaviour every
    //! result before 2026-09-08 was measured under. Above 1 the BUDGET below is
    //! normally what binds, not this.
    uint32_t maxRelayDepth;
    //! Accumulated positional variance a relayed measurement may carry, m^2.
    //! Zero disables the budget. A2 used to reject any depth but 1 outright;
    //! depth is now bounded by how much uncertainty the chain picked up, which is
    //! the quantity that actually decides whether the measurement is worth having.
    double relayVarBudgetM2;
    double sigmaRangeM;       //!< sigma of the direct ranging on this tier
    double relayP[2][2];      //!< P_B, the relay's 2-D position covariance
    double relayAgeS;         //!< age of P_B at the time of use
    double relayFixIntervalS; //!< the relay's own fix interval
    double ux;                //!< unit vector, relay -> measured node
    double uy;
    double distToRelayM; //!< ||x_A - xhat_B||
};

/**
 * \brief Apply A1–A5 and, if admitted, return the inflated sigma.
 *
 * \param m               the candidate
 * \param relaysUsed      relays that have ALREADY contributed to this update (A5)
 * \param sigmaEffMOut    written only on admission
 * \return the verdict; `CA_TBL_RELAY_ADMIT` means sigmaEffMOut is valid
 *
 * The order of the checks is the order of §4 and is not arbitrary: the cheap structural
 * conditions (tier, depth) are tested before the ones that need the covariance, so a
 * malformed P_B on an inadmissible tier is never dereferenced for a decision that was
 * already made.
 */
CaTblRelayVerdict CaTblAdmitRelayedRange(const CaTblRelayedRange& m,
                                         const std::set<uint16_t>& relaysUsed,
                                         double* sigmaEffMOut);

//! Which half of A2 decided a candidate.
enum CaTblRelayA2Reason
{
    CA_TBL_RELAY_A2_PASS = 0, //!< within the ceiling, and within the budget if one is set
    CA_TBL_RELAY_A2_CEILING,  //!< depth 0 (not relayed at all) or beyond maxRelayDepth
    CA_TBL_RELAY_A2_BUDGET    //!< within the ceiling, but tr(P_B) exceeds relayVarBudgetM2
};

/**
 * \brief A2 alone: the depth ceiling, then the variance budget.
 *
 * A2 has two halves and both refuse with CA_TBL_RELAY_REJECT_DEPTH, so the verdict cannot
 * say which bound refused. A run that has to show the BUDGET binding -- not merely the
 * ceiling -- asks here, and CaTblAdmitRelayedRange() asks the same function, so the two
 * cannot drift apart. The answer is meaningful only for a candidate that passed A1; A1 is
 * checked first and a tier-1 candidate never reaches A2.
 */
CaTblRelayA2Reason CaTblRelayA2(const CaTblRelayedRange& m);

//! "pass", "ceiling" or "budget".
const char* CaTblRelayA2ReasonName(CaTblRelayA2Reason r);

//! u^T P_B u — the relay's uncertainty projected onto the range direction.
double CaTblProjectRelayCovariance(const double P[2][2], double ux, double uy);

/**
 * \brief Compose a relay chain: correct P_B for uncertainty the relay itself inherited.
 *
 * THE DEFECT THIS FIXES IS NAMED IN THIS FILE'S OWN A2 NOTE. At depth 1 the relay B
 * measured its own position against surveyed anchors, so P_B is the whole story. At
 * depth 2 B's position was ITSELF obtained through a relay C, and B's covariance
 * understates its true error by exactly the inflation term this module adds -- unless
 * B's own filter already applied it, which it cannot have if B admitted the range as a
 * plain measurement.
 *
 * Using an uncorrected P_B at depth 2 is therefore not merely optimistic, it is
 * optimistic BY A KNOWN AMOUNT, and that is the amount added back here.
 *
 * \param relayP        P_B as the relay reports it
 * \param inheritedVarM2 variance the relay inherited along ITS incoming leg -- the
 *        projected term its own admission added, or 0 at depth 1
 * \param ux,uy        unit vector along which the inherited error acts, normally the
 *        direction of the relay's own upstream leg
 * \param outP         corrected covariance
 *
 * THE INFLATION IS DIRECTIONAL, and that is the reason this takes a vector rather than
 * a scalar. A relay whose own error runs along the line to ITS relay is well determined
 * across that line and poorly along it. Adding the variance isotropically would inflate
 * the well-known axis too, which is conservative in the wrong place: it would refuse
 * geometry that is actually usable while under-inflating the axis that is not.
 *
 * \return false if relayP is not positive-definite, in which case outP is untouched.
 *         A chain that cannot be composed must not silently pass the uncorrected
 *         covariance to the next hop -- that is the exact understatement above.
 */
bool CaTblComposeRelayChain(const double relayP[2][2],
                            double inheritedVarM2,
                            double ux,
                            double uy,
                            double outP[2][2]);

//! Human-readable verdict, for gates and traces.
const char* CaTblRelayVerdictName(CaTblRelayVerdict v);

/**
 * A3's constant. OURS, not cited: set so the neglected second-order term in the §2
 * linearisation stays below 10 % of sigma_eff^2. Named rather than inlined so that a
 * reader can find it and see that it is a choice.
 */
extern const double CA_TBL_RELAY_GEOMETRY_RATIO;

} // namespace ns3

#endif /* CA_TBL_RELAY_ADMISSION_H */
