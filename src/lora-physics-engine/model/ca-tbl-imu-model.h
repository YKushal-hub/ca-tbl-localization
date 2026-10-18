/*
 * CA-TBL DC-1 -- a synthetic IMU for the filter's prediction (docs/SPEC_CONFORMANCE_PLAN.md
 * DC-1, owner decision 2).
 *
 * WHAT THE SPEC ASKS. Physical 2.2 predicts x_k|k-1 = F x + B u_k, "u_k is the IMU-derived
 * acceleration/heading correction". The filter so far has had no control input at all:
 * constant velocity, driven by Q alone. The SHS gait engine is an IMU, but it diverges
 * where fixes are sparse (TECH_DEBT K.9, owner decision 2026-09-08 to leave it), so it
 * cannot be the filter's input.
 *
 * WHAT THIS IS. A displacement-level dead-reckoning ERROR MODEL, not a sensor simulation.
 * Each cycle it takes the walker's TRUE displacement and returns what an IMU-based
 * dead-reckoner would have reported: u_k = (1 + s) R(psi_k) Delta_k, with
 *   - psi_k the heading error, psi_k = psi_{k-1} + b dt + N sqrt(dt) n_k: a gyro bias b drawn
 *     once per run from N(0, sigma_b^2), plus angle random walk at noise density N;
 *   - s a step-length (scale) error drawn once per run from N(0, sigma_s^2).
 * Every draw comes from its own stream (SetStream), so no shared stream moves.
 *
 * THE ERRORS ARE CORRELATED, AND THE COVARIANCE SAYS SO. b and s are constant for the run,
 * so u_k - Delta_k is strongly correlated from cycle to cycle; a per-step independent Q
 * would understate dead-reckoning drift by about 10x after 100 cycles (the plan's review
 * finding). So the reckoner tracks the ACCUMULATED displacement-error covariance since the
 * last absolute fix, with D the displacement accumulated since then:
 *   Sigma_acc = var_psi(t) |D|^2 p p^T + sigma_s^2 D D^T,  var_psi(t) = sigma_b^2 t^2 + N^2 t,
 * p the unit normal to D, t the time since the run began (the heading error does not reset
 * at a fix; the position error does). It hands the filter the increment
 * Q_u = Sigma_acc(k) - Sigma_acc(k-1), projected to be positive semi-definite, so the
 * marginal growth over a dead-reckoned stretch is right. Correlation with earlier states
 * is still ignored -- stated. On a step with no displacement (the walker dwells) p is
 * undefined and Q_u = sigma_w^2 I only.
 *
 * PARAMETERS. The gyro figures are the MPU-6050's, already cited in the repository
 * (src/lorawan/model/lora-shs-parameters.h, Rudyk et al., Sensors 20(17):4841, 2020): noise
 * density 1.6682e-4 rad/s/sqrt(Hz), bias instability 8.816 deg/h. The plan's first choice,
 * the BMI270 datasheet, needs a PDF tool this machine lacks (plan sect 0 rule 4); decision 2
 * names the MPU-6050 as the fallback. sigma_s = 0.0266 is OURS: the repository's own SHS
 * straight-traverse relative error (docs/IMU_SHS_DESIGN.md, 2.66%), a measured property of
 * this project's step-length model, not a datasheet number. sigma_w = 0.05 m is OURS.
 */

#ifndef CA_TBL_IMU_MODEL_H
#define CA_TBL_IMU_MODEL_H

#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

#include <cstdint>

namespace ns3
{

struct CaTblImuParams
{
    double gyroNoiseDensityRadSqrtS = 1.6682e-4; //!< N, rad/s/sqrt(Hz): Rudyk 2020 (MPU-6050)
    double gyroBiasSigmaRadS = 8.816 * 3.14159265358979323846 / 180.0 / 3600.0; //!< sigma_b
    double scaleSigma = 0.0266; //!< sigma_s, OURS (IMU_SHS_DESIGN.md straight traverse)
    double dwellSigmaM = 0.05;  //!< sigma_w, OURS
    //! --imuStrideSigmaPct: per-STEP random stride-length error, relative, independent between steps.
    //! 0 (default): none -- DC-1's stride is exact up to the per-run scale. Acceleration-based step-length
    //! estimators err by a few percent of distance walked (Jahn et al., IPIN 2010; Harle, IEEE COMST 2013).
    double strideSigmaRel = 0.0;
    double strideLenM = 0.7; //!< a step's length, m, to count steps in a cycle. OURS (adult walking stride)
};

class CaTblImuDeadReckoner
{
  public:
    /** Draw the run's bias and scale error from \p stream. Zero sigmas give u = Delta. */
    void Configure(const CaTblImuParams& p, int64_t stream);

    /** One cycle: true displacement (dx, dy) over dt. Writes u and Q_u (xx, xy, yy). */
    void Step(double dx, double dy, double dt, double& ux, double& uy, double qu[3]);

    /** The filter fused an absolute fix: the accumulated position error restarts. */
    void ResetAccumulation();

    double Heading() const { return m_psi; }
    double Bias() const { return m_bias; }
    double Scale() const { return m_scale; }
    double ElapsedS() const { return m_t; }
    //! Sigma_acc (xx, xy, yy) after the last Step().
    const double* SigmaAcc() const { return m_sigAcc; }
    //! The accumulated true displacement D since the last reset.
    double AccX() const { return m_dx; }
    double AccY() const { return m_dy; }

  private:
    CaTblImuParams m_p;
    Ptr<NormalRandomVariable> m_rv;
    double m_bias = 0.0;
    double m_scale = 0.0;
    double m_psi = 0.0;
    double m_t = 0.0;
    double m_dx = 0.0;
    double m_dy = 0.0;
    double m_sigAcc[3] = {0.0, 0.0, 0.0};
    double m_strideAcc[3] = {0.0, 0.0, 0.0}; //!< accumulated stride-noise covariance
};

/** Project a symmetric 2x2 matrix (xx, xy, yy) onto the positive semi-definite cone. */
void CaTblProjectPsd2(double m[3]);

} // namespace ns3

#endif // CA_TBL_IMU_MODEL_H
