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
#include "ca-tbl-policy.h" // CaTblRangeMeasurement

#include <vector>

namespace ns3
{

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

    /** REINIT recovery: new position and variance; theta, b, s and their block kept. */
    void ReinitPosition(double x, double y, double posVar);

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
};

} // namespace ns3

#endif // CA_TBL_IMU_EKF_H
