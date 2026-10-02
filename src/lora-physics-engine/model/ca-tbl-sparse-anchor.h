/*
 * CA-TBL sect 7 -- sparse-anchor geometry resolution, Cases A / B / C.
 *
 * WHY THIS EXISTS. Multilateration needs three anchors. Below three the estimator had
 * exactly one behaviour: coast on the prediction. That is correct and it is also a
 * waste, because two ranges are not nothing -- they cut the plane to two points, and
 * the filter already holds a prior good enough to choose between them nearly always.
 *
 * IT MATTERS MOST WHERE THE SYSTEM IS WEAKEST. Tier 3 reaches ~13.5 m, so three UWB
 * neighbours at once is rare while two is markedly less rare. A tier that can fix on
 * two anchors is reachable in a deployment where a tier needing three is not.
 *
 * THE DISAMBIGUATION IS THE WHOLE DESIGN, and it is where this can go wrong. The two
 * circle intersections are geometrically equal; only the prior distinguishes them, and
 * a prior that has drifted will choose confidently and wrongly. So the choice is made
 * on Mahalanobis distance IN THE PRIOR'S OWN METRIC, and it is REFUSED when the two
 * candidates are not separated by a margin. A coin flip between two positions tens of
 * metres apart is worse than coasting, because coasting is honest about knowing
 * nothing while a wrong fix enters the filter as evidence.
 */

#ifndef CA_TBL_SPARSE_ANCHOR_H
#define CA_TBL_SPARSE_ANCHOR_H

#include <cstdint>

namespace ns3
{

//! Which sect 7 case produced (or refused) a solution.
enum CaTblSparseCase : uint8_t
{
    CA_TBL_CASE_NONE = 0,  //!< nothing usable; the caller must coast
    CA_TBL_CASE_A = 1,     //!< two anchors, two-circle intersection
    CA_TBL_CASE_B = 2,     //!< one anchor + one peer, orientation from heading
    CA_TBL_CASE_C = 3      //!< peers only -- RELATIVE shape, never an absolute fix
};

//! Why a case refused, so a refusal is reportable rather than silent.
enum CaTblSparseVerdict : uint8_t
{
    CA_TBL_SPARSE_OK = 0,
    CA_TBL_SPARSE_DEGENERATE = 1,  //!< anchors coincident, or circles do not meet
    CA_TBL_SPARSE_AMBIGUOUS = 2,   //!< the prior cannot separate the two candidates
    CA_TBL_SPARSE_NO_PRIOR = 3     //!< prior covariance unusable, so nothing can choose
};

struct CaTblSparseResult
{
    double x = 0.0;
    double y = 0.0;
    double dmChosen = 0.0;    //!< Mahalanobis distance of the chosen candidate
    double dmRejected = 0.0;  //!< and of the one not chosen -- the margin is their gap
    CaTblSparseCase solvedBy = CA_TBL_CASE_NONE;
    CaTblSparseVerdict verdict = CA_TBL_SPARSE_DEGENERATE;
};

/**
 * \brief Case A -- two anchors at known positions, two measured ranges.
 *
 * Two-circle intersection, then the prior chooses. \p marginThreshold is the minimum
 * separation in Mahalanobis units required to accept; below it the verdict is
 * AMBIGUOUS and no position is returned.
 *
 * Ranges that do not reach are pulled to the radical line rather than refused: real
 * ranges are noisy and two circles that fall barely short still carry the direction
 * information. Circles that miss by more than their own noise are DEGENERATE.
 */
CaTblSparseResult CaTblSolveCaseA(double x1, double y1, double d1,
                                  double x2, double y2, double d2,
                                  double priorX, double priorY,
                                  double pxx, double pxy, double pyy,
                                  double marginThreshold);

/**
 * \brief Case B -- one anchor and one peer whose own position is held.
 *
 * The spec resolves the remaining rotational freedom with the IMU heading. This
 * project's SHS inertial driver is recorded as debt (TECH_DEBT K.9) and its heading is not
 * trustworthy, so Case B uses the SAME prior-based disambiguation as Case A rather than
 * a heading it cannot rely on.
 *
 * WHAT THE PRIOR IS decides whether that is a deviation (docs/SPEC_CONFORMANCE_PLAN.md
 * DC-2). Under --imuPredict and --sparseInStep, Step() solves Case B AFTER the controlled
 * prediction, so the prior is the last posterior plus the synthetic IMU's displacement u_k
 * with covariance P + Q_u: the IMU heading estimate carried into position, which in this
 * geometry is what resolving the orientation by heading means. The deviation is CLOSED
 * there, and the decision says so (CaTblDecision::sparsePriorSource = 1). Without
 * --imuPredict the prior is the constant-velocity one and the deviation stays OPEN (0).
 * The SHS heading is still not used, by owner decision.
 *
 * Structurally this is Case A with the peer standing in as the second anchor, so it
 * shares the implementation and differs only in what it reports as solvedBy -- which
 * matters, because a fix that leaned on a peer's estimate is not the same evidence as
 * one that leaned on two surveyed anchors.
 */
CaTblSparseResult CaTblSolveCaseB(double anchorX, double anchorY, double dAnchor,
                                  double peerX, double peerY, double dPeer,
                                  double priorX, double priorY,
                                  double pxx, double pxy, double pyy,
                                  double marginThreshold);

/**
 * \brief Case C -- peers only.
 *
 * ALWAYS REFUSES, and that is the implementation rather than a gap. With no surveyed
 * anchor there is no absolute frame: the peer set fixes a SHAPE, and the shape can be
 * translated and rotated freely without contradicting any measurement. The spec says
 * so and restricts Case C to relative offset and cohesion tracking.
 *
 * It exists as a named function so the refusal is explicit at the call site. A caller
 * that reaches Case C has learned something -- that it is unlocalisable this cycle --
 * and that is different from not having asked.
 */
CaTblSparseResult CaTblSolveCaseC();

} // namespace ns3

#endif // CA_TBL_SPARSE_ANCHOR_H
