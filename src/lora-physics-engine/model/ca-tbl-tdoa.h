/*
 * CA-TBL Physical sect 1.1 -- tier-1 time-difference-of-arrival.
 *
 * WHAT THE SPEC SAYS AND WHAT THE CODE DID. The spec localises tier 1 by "RSSI-based
 * path-loss estimation, combined with TDOA where three or more anchors receive the same
 * transmission", solving c*(t_i - t_ref) = |p - p_i| - |p - p_ref| by linearised least
 * squares (Chan, or a Taylor series). Until this file the code did RSSI inversion only,
 * while the energy model billed a TDoA-shaped exchange (TECH_DEBT J.2, L.2.1).
 *
 * WHY TIER 1 CAN DO THIS AT ALL. The gateways are the only immobile class and they
 * hold a GNSS lock: a self-surveyed position and a PPS-disciplined clock. So the anchors
 * share a time base and the field node does not need one -- its unknown emission
 * instant is common to every arrival and cancels in the differences. That is the whole
 * point of DIFFERENCES, and it is why nothing here reads the transmitter's clock.
 *
 * THIS FILE IS PURE GEOMETRY AND ARITHMETIC. It draws no random number and knows nothing
 * about radios. The timestamps, and the per-anchor sigma the network believes each one
 * carries, are the caller's: the scenario builds them from the GPS-PPS residual, the
 * SNR-limited arrival-time estimate and the NLOS excess delay of each link. Keeping the
 * solver free of the error model is what lets the unit suite test it with timestamps
 * whose truth is known exactly.
 *
 * THE SOLVER, in the order it runs:
 *   1. Refuse fewer than three anchors (two unknowns need two independent differences),
 *      and refuse an anchor set that is collinear or coincident: the data then cannot
 *      say which side of the line the node is on, and a side chosen by the seed alone is
 *      the prior talking, not a measurement.
 *   2. EXACTLY THREE ANCHORS is a square system with up to TWO exact solutions: two
 *      hyperbolae can cross twice, and both crossings fit the stamps perfectly. Seeding
 *      an iteration and taking whatever root it lands on -- this file's first draft --
 *      lets the seed choose silently, and measured, it chose the mirror root 450-743 m
 *      from the truth while reporting a sigma near 85 m. So three anchors are solved in
 *      closed form instead: with the reference at the origin, each difference is linear
 *      in (x, y) given the reference range R, so p = a + b R, and R^2 = |p|^2 is a
 *      quadratic in R. Every root is found; a root is kept only if every |p - a_k| = d_k +
 *      R is non-negative (squaring admits spurious roots). None: NO_ROOT, the hyperbolae
 *      do not meet. One: it. Two: the cycle's RSSI RANGES choose -- the root whose
 *      weighted range residual is lower by at least minTieChi2Gap -- and if they cannot,
 *      AMBIGUOUS, no fix. This is the combination the spec names ("RSSI path loss,
 *      combined with TDOA"), used where it carries information, and the same refusal
 *      Case A makes (ca-tbl-sparse-anchor.h) when its prior cannot pick an intersection.
 *   3. Four or more: Chan & Ho's two-stage closed form (IEEE Trans. Signal Processing
 *      42(8), 1994) seeds the iteration -- no prior needed. When Chan's normal equations
 *      are singular, or the iteration from Chan fails, the caller's seed: in the scenario
 *      that is the cycle's own RSSI multilateration fix, as the spec says.
 *   4. Refine by Foy's Taylor-series least squares (IEEE Trans. AES 12(2), 1976), i.e.
 *      Gauss-Newton on the weighted range-difference residual, with step halving. This is
 *      the maximum-likelihood estimate for Gaussian timestamps; Chan's closed form reaches
 *      it only in the small-noise limit, and tier-1 noise is not small. An iterate that
 *      leaves the anchors' neighbourhood (maxExtentFactor) has run out along an asymptote
 *      and is DIVERGED, not converged: measured, two such "solutions" sat 1e17 m away.
 *   5. Hand back the covariance (J' Q^-1 J)^-1 at the solution. Q is NOT diagonal: every
 *      difference shares the reference anchor's error, so Q = diag(sigma_i^2) +
 *      sigma_ref^2 * 1 1'. Dropping the off-diagonal would understate the covariance the
 *      filter is then told to trust.
 */

#ifndef CA_TBL_TDOA_H
#define CA_TBL_TDOA_H

#include <cstdint>
#include <vector>

namespace ns3
{

//! One anchor's record of ONE broadcast.
struct CaTblTdoaObs
{
    uint16_t anchorId = 0;
    double x = 0.0;      //!< surveyed position, m
    double y = 0.0;
    double tArrS = 0.0;  //!< arrival stamp on the anchor's PPS-disciplined clock, s
    double sigmaS = 0.0; //!< the 1-sigma the NETWORK assigns this stamp, s (> 0)
};

enum CaTblTdoaVerdict : uint8_t
{
    CA_TBL_TDOA_OK = 0,
    CA_TBL_TDOA_TOO_FEW = 1,     //!< fewer than 3 anchors heard the broadcast
    CA_TBL_TDOA_COLLINEAR = 2,   //!< anchors on (or within tolerance of) one line, or coincident
    CA_TBL_TDOA_SINGULAR = 3,    //!< information matrix singular or ill-conditioned at the solution
    CA_TBL_TDOA_NO_CONVERGE = 4, //!< Taylor iteration did not settle within the budget
    CA_TBL_TDOA_BAD_INPUT = 5,   //!< non-finite stamp or position, or sigma <= 0
    CA_TBL_TDOA_NO_ROOT = 6,     //!< three anchors whose hyperbolae do not meet
    CA_TBL_TDOA_AMBIGUOUS = 7,   //!< three anchors, two roots, the range evidence cannot choose
    CA_TBL_TDOA_DIVERGED = 8     //!< the iterate ran out along an asymptote
};

//! Number of CaTblTdoaVerdict values, for callers that count them.
constexpr int CA_TBL_TDOA_VERDICT_COUNT = 9;

enum CaTblTdoaSeed : uint8_t
{
    CA_TBL_TDOA_SEED_NONE = 0,
    CA_TBL_TDOA_SEED_CHAN = 1,   //!< Chan's closed form (>= 4 anchors)
    CA_TBL_TDOA_SEED_CALLER = 2, //!< the caller's seed -- the RSSI fix in the scenario
    CA_TBL_TDOA_SEED_ROOT = 3    //!< the three-anchor closed form, root chosen by the ranges
};

//! Number of CaTblTdoaSeed values, for callers that count them.
constexpr int CA_TBL_TDOA_SEED_COUNT = 4;

/** One range from the SAME broadcast (RSSI inversion in the scenario), used only to
 *  choose between the two exact roots of a three-anchor solve. */
struct CaTblTdoaRangeHint
{
    double x = 0.0;      //!< the ranged anchor's position, m
    double y = 0.0;
    double rangeM = 0.0; //!< the measured range, m
    double sigmaM = 0.0; //!< its 1-sigma, m (> 0; a hint without one is ignored)
};

const char* CaTblTdoaVerdictName(CaTblTdoaVerdict v);
const char* CaTblTdoaSeedName(CaTblTdoaSeed s);

/** Solver limits. Every value is OURS, not cited; each says what it guards. */
struct CaTblTdoaConfig
{
    //! Gauss-Newton iterations before NO_CONVERGE. From a Chan or RSSI seed the
    //! unit suite converges in < 10; 25 leaves room without letting a wandering
    //! iteration run on.
    uint32_t maxIterations = 25;
    //! Converged when the accepted step is shorter than this, m.
    double convergedStepM = 0.01;
    //! Anchor set refused as collinear when sqrt(lambda_min / lambda_max) of the
    //! anchors' 2x2 scatter is below this: their spread off their principal line is
    //! under 1% of their spread along it.
    double minSpreadRatio = 0.01;
    //! Information matrix refused as singular above this condition number.
    double maxCondition = 1e10;
    //! An iterate farther from the anchors' centroid than this many anchor extents
    //! (the largest anchor distance from that centroid) is DIVERGED. At 10, a 2 km
    //! gateway spread allows 20 km, several times the tier-1 radio reach, so it refuses
    //! only what no radio could have produced.
    double maxExtentFactor = 10.0;
    //! Three anchors, two roots: the lower weighted RSSI-range residual wins only by at
    //! least this much. 4 is a likelihood ratio of e^2 ~ 7.4 : 1 between the roots.
    double minTieChi2Gap = 4.0;
    //! Three anchors, two roots, no range hints: the seed must be at least this many
    //! times closer to one root than to the other.
    double minTieSeedRatio = 2.0;
};

struct CaTblTdoaResult
{
    CaTblTdoaVerdict verdict = CA_TBL_TDOA_BAD_INPUT;
    CaTblTdoaSeed seededBy = CA_TBL_TDOA_SEED_NONE;
    double x = 0.0;
    double y = 0.0;
    //! Covariance of (x, y), m^2: (J' Q^-1 J)^-1 at the solution. Valid on OK only.
    double rxx = 0.0;
    double rxy = 0.0;
    double ryy = 0.0;
    //! Chan's closed form, when it ran (>= 4 anchors) and its normal equations held.
    bool chanOk = false;
    double chanX = 0.0;
    double chanY = 0.0;
    uint32_t nAnchors = 0;
    uint16_t refAnchorId = 0;
    uint32_t iterations = 0;
    //! Weighted residual r' Q^-1 r at the solution: chi-square with M - 3 degrees of
    //! freedom under the model. Identically ~0 at M = 3, where the system is square.
    double chi2 = 0.0;
    //! Three anchors: the number of exact roots found (0, 1 or 2), and, with two, the
    //! weighted RSSI-range residual of the chosen and of the rejected root (-1 when the
    //! tie was settled by the seed, or there was no second root).
    uint32_t nRoots = 0;
    double tieChi2Chosen = -1.0;
    double tieChi2Other = -1.0;
};

/**
 * \brief One-way arrival-time Cramer-Rao bound for a LoRa CSS frame, seconds.
 *
 *   sigma_t = 1 / (2 pi beta_rms sqrt(2 E/N0)),  beta_rms = BW / sqrt(12)
 *   E/N0    = in-band SNR + 10 log10(2^SF)       (the dechirp gain)
 *
 * The same bound and the same gain the tier-2 CSS time-of-flight model uses
 * (lora-ranging-submodule.cc), ONE-WAY because an anchor timestamps a single arrival.
 * Unlike RSSI inversion this IS a time-of-arrival estimator, so the bound applies to it.
 * It is a BOUND: a real estimator sits above it at low SNR (the threshold effect), which
 * is not modelled. Returns +infinity on a non-positive bandwidth or a non-finite SNR.
 */
double CaTblTdoaArrivalSigmaS(double bandwidthHz, uint8_t spreadingFactor, double snrDbInBand);

/**
 * \brief Chan & Ho (1994) two-stage closed form, 2-D, relative to anchor \p ref.
 *
 * Needs at least four anchors (stage 1 has three unknowns: x, y and the range to the
 * reference). Stage 1 is solved twice, first with the noise weighting alone and then
 * with it scaled by the stage-1 distances (Psi = B Q B); stage 2 imposes the constraint
 * between the stage-1 unknowns. When stage 2 is singular or yields a negative square it
 * falls back to stage 1. Returns false when stage 1's normal equations are singular --
 * which is what collinear anchors do to it.
 */
bool CaTblTdoaChan(const std::vector<CaTblTdoaObs>& obs, size_t ref, double& outX, double& outY);

/**
 * \brief Every exact solution of a three-anchor broadcast, relative to anchor \p ref.
 *
 * Returns the number of roots written to \p outX / \p outY (0, 1 or 2), each satisfying
 * all three stamps; -1 when the two baselines are parallel (collinear anchors).
 */
int CaTblTdoaThreeAnchorRoots(const std::vector<CaTblTdoaObs>& obs,
                              size_t ref,
                              double outX[2],
                              double outY[2]);

/**
 * \brief Solve one broadcast. \p seedX, \p seedY are used when Chan cannot run or its
 * iteration fails, and to break a three-anchor tie when \p hints is empty; the scenario
 * passes the cycle's RSSI multilateration fix, and its RSSI ranges as \p hints.
 */
CaTblTdoaResult CaTblSolveTdoa(const std::vector<CaTblTdoaObs>& obs,
                               double seedX,
                               double seedY,
                               const CaTblTdoaConfig& cfg = CaTblTdoaConfig(),
                               const std::vector<CaTblTdoaRangeHint>& hints =
                                   std::vector<CaTblTdoaRangeHint>());

} // namespace ns3

#endif // CA_TBL_TDOA_H
