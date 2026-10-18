/*
 * CaTblImuEkf -- the 5-state inertial filter [x, y, theta, b, s], ported from the local line on
 * 2026-10-16 (docs/LOCAL_LINE_INTEGRATION.md). It ESTIMATES the gyro bias b and stride scale s
 * from range fixes, where DC-1 (CaTblImuDeadReckoner, ca-tbl-imu-model.h) draws them and leaves
 * them unobserved.
 *
 * Standalone and NOT wired into CaTblPolicy here: it changes no scenario output. It is
 * exercised by scratch/ca_tbl_imu_validator.cc and gates G91 (closed forms, NEES) and G94
 * (a 1000 s outage). Wiring it into the policy, through Case B, Case D and the witness, is
 * listed as left in docs/LOCAL_LINE_INTEGRATION.md sect 4.
 */

#ifndef CA_TBL_IMU_EKF_H
#define CA_TBL_IMU_EKF_H

#include "ca-tbl-imu-sensor-model.h"

#include <vector>

namespace ns3
{

// Forward-declared, not included: ca-tbl-policy.h includes this header (--imuFilter=ekf5).
// Only references are taken here; ca-tbl-imu-ekf.cc includes the full definition.
struct CaTblRangeMeasurement;

/**
 * \brief C3: range-only EKF whose predict step is IMU dead reckoning.
 *
 * State [x, y, theta, b, s]:
 *   theta  offset between the IMU's integrated heading and the map heading, rad
 *   b      gyro bias, rad/s
 *   s      stride-model scale error, fraction
 *
 * None of theta, b or s is measured. They become observable through the position
 * corrections range updates make: a heading offset bends the dead-reckoned path
 * sideways, a stride error stretches it along track.
 *
 * WHY s IS A STATE AND NOT PROCESS NOISE. The stride scale error is one number for
 * the whole run. Modelled as white noise it would grow like sqrt(n) when it really
 * grows like n: at the nominal 3 % over a 120 s outage the filter would claim
 * 0.78 m along-track against a true 5.4 m -- seven times overconfident on exactly
 * the cycles where a fix returns after a gap. An overconfident prior there makes
 * the integrity check fire on a correct fix. The same argument puts b in the state.
 *
 * A separate class rather than a mode of CaTblEkf so the constant-velocity filter's
 * code -- and therefore every committed result -- is not touched at all.
 */
class CaTblImuEkf
{
  public:
    static const int N = 5;

    CaTblImuEkf();

    /** All cross-covariances zero; theta = b = s = 0 with the given variances. */
    void Init(double x,
              double y,
              double posVar,
              double thetaVar,
              double biasVar,
              double strideVar);

    /**
     * Dead-reckoning prediction from one cycle of IMU output. Stores the prior.
     *
     *   theta' = theta + b dt                 bias accumulates into heading
     *   phi    = rawHeading - theta'          map heading of this cycle's step
     *   D      = dist / (1 + s)               stride-corrected distance
     *   x' = x + D cos(phi),  y' = y + D sin(phi),  b' = b,  s' = s
     *
     * \param qStep2   per-step stride noise variance along the direction of travel,
     *                 m^2 (the per-RUN scale error is the state s, not noise)
     * \param qTheta2  variance added to theta this cycle, rad^2. It moves the
     *                 position through phi, so it is propagated with the heading
     *                 Jacobian rather than added to the position block alone
     * \param qBias2   random-walk variance added to b, (rad/s)^2
     */
    void Predict(double dt,
                 double distM,
                 double rawHeadingRad,
                 double qStep2,
                 double qTheta2,
                 double qBias2);

    /**
     * The same motion model as Predict(), propagated with sigma points instead of the
     * Jacobian. Under heading uncertainty the dead-reckoned position is curved -- a heading
     * error rotates the path, and the along-track shortening L(1 - cos d) is second-order,
     * which the Jacobian drops. Sigma points in theta carry it through every cycle.
     * Symmetric set, 2N points, weights 1/(2N). Process noise is added as in Predict().
     */
    void PredictUnscented(double dt,
                          double distM,
                          double rawHeadingRad,
                          double qStep2,
                          double qTheta2,
                          double qBias2);

    /**
     * A cycle with no displacement: theta' = theta + b dt with its variance grown by qTheta2 and the
     * bias's, the position unmoved but given posVar2 per axis (the dwell noise). The prior is
     * re-snapshotted, as every prediction does.
     */
    void PredictDwell(double dt, double qTheta2, double qBias2, double posVar2);

    /** REINIT recovery: new position and variance; theta, b, s and their block kept. */
    void ReinitPosition(double x, double y, double posVar);

    /** --imuMagSigmaDeg: a direct reading z of the heading offset theta with variance r (a magnetometer's
     *  raw-minus-magnetic heading). Scalar update, H = e_theta, innovation wrapped to (-pi, pi]. */
    void FuseHeading(double z, double r);

    /** Set the position and its full 2x2 covariance, as CaTblEkf::SetPositionCov does for
     *  --acquireParticles' handover. Position's cross-covariance with theta, b and s is zeroed:
     *  the cloud says nothing about them, and what the filter has learned of them is kept. */
    void SetPositionCov(double x, double y, double pxx, double pxy, double pyy);

    /** --relayNisInflate: running mean (EWMA, weight 0.02) of the range updates' NIS, nu^2 / S,
     *  exactly as CaTblEkf's. Observation only: the update's arithmetic is unchanged. */
    void EnableNisTracking() { m_trackNis = true; }
    double NisMean() const { return m_nisN > 0 ? m_nisMean : 1.0; }

    /**
     * Process noise for one cycle, from the ASSUMED sensor parameters only -- never
     * from a realised error. One definition, used by the policy and by the validator's
     * consistency test, so the thing tested is the thing that runs.
     *
     *   qTheta2 = N^2 dt + (scale_err * turn)^2   angle random walk, plus scale factor
     *                                            acting on this cycle's rotation
     *   qStep2  = step_err^2 * L * dist           per-step stride noise; the per-run
     *                                            scale error is the state s, not noise
     *   qBias2  = sigma_gm^2 (1 - exp(-2 dt/tau)) Gauss-Markov increment, applied as a
     *                                            random walk: never decays where the
     *                                            true process does, so it errs wide
     *
     * \param turnRad  rotation this cycle as the tag sees it (raw heading change)
     */
    static void ProcessNoise(const CaTblImuSensorParams& p,
                             double dt,
                             double distM,
                             double turnRad,
                             double& qStep2,
                             double& qTheta2,
                             double& qBias2);

    void Update(const std::vector<CaTblRangeMeasurement>& meas,
                const std::vector<double>& varM2);
    void RevertToPrior();
    void InflatePos(double factor);
    double X() const { return m_x[0]; }
    double Y() const { return m_x[1]; }
    double TracePos() const { return m_P[0][0] + m_P[1][1]; }
    double PriorX() const { return m_xPrior[0]; }
    double PriorY() const { return m_xPrior[1]; }
    double MahalanobisFromPrior(double zx, double zy) const;
    double MahalanobisFromPriorWithR(double zx,
                                     double zy,
                                     double rxx,
                                     double rxy,
                                     double ryy) const;
    void PosCov(double& pxx, double& pxy, double& pyy) const
    {
        pxx = m_P[0][0];
        pxy = m_P[0][1];
        pyy = m_P[1][1];
    }

    // --- The rest of the interface CaTblPolicy uses on its filter (--imuFilter=ekf5) ---
    // Each mirrors the constant-velocity CaTblEkf's method of the same name over five states;
    // a position observation corrects theta, b and s through their covariance with position.

    /** Fuse an absolute position with isotropic variance (CaTblEkf::FusePosition). Leaves the
     *  stored prior alone, as that one does. Ends a coast. */
    void FusePosition(double zx, double zy, double varM2);
    /** NIS of a position observation with covariance R against the current state, or -1. */
    double PositionNis(double zx, double zy, double rxx, double rxy, double ryy) const;
    /** Fuse a position with a full 2x2 covariance; false (nothing changed) if R or S is bad. */
    bool FusePositionCov(double zx, double zy, double rxx, double rxy, double ryy);
    double Pxx() const { return m_P[0][0]; }
    double Pxy() const { return 0.5 * (m_P[0][1] + m_P[1][0]); }
    double Pyy() const { return m_P[1][1]; }
    /** Any covariance element, read only (the unit suite's CONSIDER cases). */
    double Cov(int i, int j) const { return m_P[i][j]; }
    /** Velocity is not a state here: the last prediction's displacement over its dt. */
    double Vx() const { return m_vx; }
    double Vy() const { return m_vy; }

    // --- N.33: robustness of the sensor states to bad fixes (default off: mode 0) ---------
    // Bad fixes -- a range to a stale pseudo-anchor, an NLOS excess -- reach theta, b and s
    // through their covariance with position, and can rotate the heading estimate until the
    // filter dead-reckons the wrong way and never recovers (EKF5-PRESET: every catastrophic
    // seed had a final heading error > 0.3 rad). Bits, combinable:
    //   GATE     skip a range whose innovation NIS > gate1 (chi^2_1 0.999 = 10.83), a position
    //            fix whose NIS > gate2 (chi^2_2 0.999 = 13.82)
    //   PROTECT  when the fix as a whole is inconsistent with the prior (mean range NIS >
    //            protect1 = 6.63, or position NIS > protect2 = 9.21), update position only:
    //            zero gain on theta, b, s; Joseph-form covariance, valid for that gain
    //   RECOVER  if a majority of a fix's ranges fail the gate on recoverAfter consecutive
    //            fixes, the FILTER is presumed wrong, not the ranges: reopen position and
    //            heading variance and take that fix ungated -- so gating cannot lock it out
    // Mode 0 runs exactly the original arithmetic. All thresholds OURS except the chi^2 values.
    static const int ROBUST_GATE = 1;
    static const int ROBUST_PROTECT = 2;
    static const int ROBUST_RECOVER = 4;
    // CONSIDER  a range update with fewer than 3 ranges cannot observe heading, bias or scale
    //           (one range fixes a circle, two a pair of points): a Schmidt (consider) update --
    //           zero gain on theta, b, s, Joseph covariance, so their uncertainty is kept and
    //           carried in P but no single-range innovation is spent learning them.
    static const int ROBUST_CONSIDER = 8;
    void SetRobust(int mode) { m_robust = mode; }
    //! Diagnostics only (the validator's wrong-start case): set the heading-offset state.
    void ForceHeadingOffsetForTest(double th) { m_x[2] = th; SnapshotPrior(); }
    int Robust() const { return m_robust; }
    uint64_t RobustGated() const { return m_nGated; }
    uint64_t RobustProtected() const { return m_nProtected; }
    uint64_t RobustRecoveries() const { return m_nRecoveries; }
    uint64_t RobustConsidered() const { return m_nConsidered; }

    double HeadingOffset() const { return m_x[2]; }
    double GyroBias() const { return m_x[3]; }
    double StrideScale() const { return m_x[4]; }

    /**
     * Moment-matched coasting. A heading error d rotates the whole dead-reckoned path since
     * the last fix, and the along-track consequence is second order -- E[cos d] = exp(-s^2/2),
     * Var(cos d) = (1 + exp(-2 s^2))/2 - exp(-s^2) -- so the Jacobian drops it, and the filter
     * is overconfident once the heading sigma s is no longer small (report/IMU_PRIOR_RESULT.md
     * §3). With this on, after every Predict() the position mean is pulled back along the path
     * by |path|(1 - exp(-s^2/2)) and the along-track variance gains |path|^2 Var(cos d), both
     * applied incrementally and both reset by the next range update. The heading error is
     * treated as common to the whole path at its CURRENT variance, which over-states the early
     * part -- conservative by construction. Added variance is PSD, so P stays positive definite.
     */
    void SetMomentMatched(bool on) { m_mm = on; }
    //! Apply the variance correction only and leave the mean on the straight dead-reckoned
    //! path (diagnostic switch; the mean shrink uses the current heading sigma for the whole
    //! path, which over-corrects its early part).
    void SetMomentMatchedMeanShift(bool on) { m_mmShift = on; }
    double Var(int i) const { return m_P[i][i]; }

  private:
    void SnapshotPrior();
    void Symmetrise();

    double m_x[N];
    double m_P[N][N];
    double m_xPrior[N];
    double m_PPrior[N][N];

    void MomentMatch();
    void ResetCoast();
    bool m_mm = false;
    bool m_mmShift = true;
    double m_pathX = 0.0;    //!< dead-reckoned displacement since the last range update
    double m_pathY = 0.0;
    double m_shiftX = 0.0;   //!< mean correction already applied this outage
    double m_shiftY = 0.0;
    double m_varApplied = 0.0; //!< along-track variance already added this outage
    double m_vx = 0.0;         //!< last prediction's displacement / dt (Vx())
    double m_vy = 0.0;
    int m_robust = 0;
    uint32_t m_badStreak = 0;
    uint64_t m_nGated = 0;
    uint64_t m_nProtected = 0;
    uint64_t m_nRecoveries = 0;
    uint64_t m_nConsidered = 0;
    bool m_trackNis = false;
    double m_nisMean = 1.0;
    uint64_t m_nisN = 0;
    void NoteNis(double v)
    {
        if (m_trackNis && std::isfinite(v))
        {
            m_nisMean = (m_nisN == 0) ? v : 0.98 * m_nisMean + 0.02 * v;
            ++m_nisN;
        }
    }
    void UpdateRobust(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2);
    bool FusePositionCovRobust(double zx, double zy, double rxx, double rxy, double ryy);
    void JosephScalar(const double H[5], const double K[5], double r);
    bool NoteFixVerdict(bool majorityOut);
    void FuseAxis(int axis, double z, double varM2);
};

} // namespace ns3

#endif // CA_TBL_IMU_EKF_H
