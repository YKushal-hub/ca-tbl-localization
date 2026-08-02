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
    bool nlos;     //!< NLOS-tagged (NlosDb > 0 or HiddenChannelState > 0)
    //! Ranging 1-sigma in metres for this measurement, from the physics engine's
    //! rangingSigmaM. 0.0 means unavailable, in which case the policy falls back to
    //! the per-tier losSigmaM/nlosSigmaM constants.
    //!
    //! Per-measurement rather than per-tier because (a) the CRLB-derived sigma now
    //! varies with distance and SNR within a tier, and (b) a concurrent-tier arm
    //! submits ranges whose sigmas span four orders of magnitude, so a single
    //! tier-keyed constant cannot weight them.
    double sigmaM;
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

/** Tunables. Defaults reproduce the Python constants so results are comparable. */
struct CaTblPolicyConfig
{
    CaTblPolicyMode mode;

    double pcsThresholdUp[CA_TBL_TIER_COUNT];   //!< escalate when PCS falls below this
    double pcsThresholdDown[CA_TBL_TIER_COUNT]; //!< candidate for de-escalation above this
    uint32_t nDeescalate;                       //!< consecutive good cycles before demoting
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

    double mahalanobisThreshold; //!< chi-square gate, 2 DOF
    //! Use the correctly specified integrity gate: weighted multilateration for the
    //! test point, and S = P_prior_pos + (J^T R^-1 J)^-1 for the distance. Both
    //! defects are one mechanism -- a biased numerator against a denominator missing
    //! R -- so they are fixed under a single flag. Default false so the pre-existing
    //! baseline reproduces byte-identically.
    bool mahalUseR;
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
    uint32_t nNlos;

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

    /** Discard this cycle's measurement: keep the prior as the posterior. */
    void RevertToPrior();

    double X() const { return m_x[0]; }
    double Y() const { return m_x[1]; }
    double Vx() const { return m_x[2]; }
    double Vy() const { return m_x[3]; }

    double TracePos() const { return m_P[0][0] + m_P[1][1]; }
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

    uint64_t GetEscalationCount() const { return m_escalations; }
    uint64_t GetDeescalationCount() const { return m_deescalations; }
    uint64_t GetSuspectCount() const { return m_suspects; }
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
    double ComputePcs(double traceSigma, double gdop, uint32_t nLos, uint32_t nTotal, uint32_t hopCount) const;

    CaTblPolicyConfig m_config;
    CaTblEkf m_ekf;
    CaTblTier m_tier;
    uint32_t m_deescalateCounter;

    uint64_t m_escalations;
    uint64_t m_deescalations;
    uint64_t m_suspects;
    uint64_t m_noFixCycles;
    uint64_t m_tierCycles[CA_TBL_TIER_COUNT];
};

} // namespace ns3

#endif // CA_TBL_POLICY_H
