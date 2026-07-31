#include "ca-tbl-policy.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

const char*
CaTblPolicyModeName(CaTblPolicyMode mode)
{
    switch (mode)
    {
    case CA_TBL_MODE_FULL:
        return "ca_tbl_full";
    case CA_TBL_MODE_NO_INTEGRITY:
        return "ca_tbl_no_integrity";
    case CA_TBL_MODE_FIXED_THRESHOLD:
        return "fixed_threshold";
    case CA_TBL_MODE_STATIC_868:
        return "static_868";
    case CA_TBL_MODE_STATIC_2P4:
        return "static_2p4";
    case CA_TBL_MODE_STATIC_UWB:
        return "static_uwb";
    default:
        return "unknown";
    }
}

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

CaTblPolicyConfig::CaTblPolicyConfig()
{
    mode = CA_TBL_MODE_FULL;

    // NOTE: these thresholds are carried over verbatim from ca_tbl_fusion.py so
    // that the C++ port can be validated against the Python oracle. They are
    // hand-set values, NOT calibrated. The plan calls for leave-one-out
    // cross-validation over the collected dataset; until that is done these must
    // be reported as hand-tuned, and any result that is sensitive to them is not
    // yet a result. This is tracked as step S7 in CA_TBL_SIM_AUDIT.md.
    pcsThresholdUp[CA_TBL_TIER_868] = 1.55;
    pcsThresholdUp[CA_TBL_TIER_2P4] = 1.70;
    pcsThresholdUp[CA_TBL_TIER_UWB] = 2.30;

    pcsThresholdDown[CA_TBL_TIER_868] = 1.75;
    pcsThresholdDown[CA_TBL_TIER_2P4] = 1.90;
    pcsThresholdDown[CA_TBL_TIER_UWB] = 2.55;

    nDeescalate = 5;
    fastDemoteOnZeroNeighbours = false; // default preserves the pre-existing baseline

    mahalanobisThreshold = 9.21; // chi-square 99%, 2 DOF
    mahalUseR = false;           // default preserves the pre-existing baseline
    traceRef = 10.0;
    hopWeight = 0.0; // no mesh yet -- see header note
    fixedVarThresholdM2 = 100.0;

    losSigmaM[CA_TBL_TIER_868] = 50.0;
    losSigmaM[CA_TBL_TIER_2P4] = 15.0;
    losSigmaM[CA_TBL_TIER_UWB] = 0.1;

    nlosSigmaM[CA_TBL_TIER_868] = 200.0;
    nlosSigmaM[CA_TBL_TIER_2P4] = 50.0;
    nlosSigmaM[CA_TBL_TIER_UWB] = 5.0;

    // 3.0 is the geometric middle of the LOS-to-NLOS ratios the fallback constants
    // above imply (4.0 on tier 1, 3.33 on tier 2, 50 on tier 3 -- themselves
    // inconsistent and unsourced). UNCALIBRATED, and it is now the only uncited
    // number in the R-matrix path, so it is the first thing to sweep in S7.
    nlosSigmaInflation = 3.0;

    processNoiseStdPos = 1.0;
    processNoiseStdVel = 0.5;

    instantEscalate = false;
}

CaTblDecision::CaTblDecision()
    : activeTier(CA_TBL_TIER_868),
      nextTier(CA_TBL_TIER_868),
      hadFix(false),
      escalated(false),
      deescalated(false),
      suspect(false),
      measurementRejected(false),
      pcs(0.0),
      gdop(0.0),
      traceSigma(0.0),
      mahalanobis(0.0),
      estX(0.0),
      estY(0.0),
      nAnchorsUsed(0),
      nNlos(0)
{
}

// ---------------------------------------------------------------------------
// EKF
// ---------------------------------------------------------------------------

CaTblEkf::CaTblEkf()
    : m_qPos(1.0),
      m_qVel(0.5)
{
    Init(0.0, 0.0, 0.0, 0.0, 1e4, 1e2);
}

void
CaTblEkf::Init(double x, double y, double vx, double vy, double posVar, double velVar)
{
    m_x[0] = x;
    m_x[1] = y;
    m_x[2] = vx;
    m_x[3] = vy;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m_P[i][j] = 0.0;
            m_PPrior[i][j] = 0.0;
        }
        m_xPrior[i] = m_x[i];
    }
    m_P[0][0] = posVar;
    m_P[1][1] = posVar;
    m_P[2][2] = velVar;
    m_P[3][3] = velVar;
    for (int i = 0; i < 4; ++i)
    {
        m_PPrior[i][i] = m_P[i][i];
    }
}

void
CaTblEkf::SetProcessNoise(double stdPos, double stdVel)
{
    m_qPos = stdPos;
    m_qVel = stdVel;
}

void
CaTblEkf::Predict(double dt)
{
    // x <- F x, with F the constant-velocity transition.
    m_x[0] += m_x[2] * dt;
    m_x[1] += m_x[3] * dt;

    // P <- F P F^T + Q, expanded rather than looped so the structure is visible.
    double FP[4][4];
    for (int j = 0; j < 4; ++j)
    {
        FP[0][j] = m_P[0][j] + dt * m_P[2][j];
        FP[1][j] = m_P[1][j] + dt * m_P[3][j];
        FP[2][j] = m_P[2][j];
        FP[3][j] = m_P[3][j];
    }
    double newP[4][4];
    for (int i = 0; i < 4; ++i)
    {
        newP[i][0] = FP[i][0] + dt * FP[i][2];
        newP[i][1] = FP[i][1] + dt * FP[i][3];
        newP[i][2] = FP[i][2];
        newP[i][3] = FP[i][3];
    }

    // Diagonal Q, position and velocity scaled separately. This is not the Van
    // Loan discretisation (which couples position and velocity off-diagonal);
    // it matches the Python reference and is flagged so it is not mistaken for
    // something more rigorous than it is.
    newP[0][0] += m_qPos * m_qPos * dt;
    newP[1][1] += m_qPos * m_qPos * dt;
    newP[2][2] += m_qVel * m_qVel * dt;
    newP[3][3] += m_qVel * m_qVel * dt;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m_P[i][j] = newP[i][j];
        }
    }

    // Snapshot the prior: the integrity check compares the incoming fix against
    // this, so it must be the pre-update state, not the posterior.
    for (int i = 0; i < 4; ++i)
    {
        m_xPrior[i] = m_x[i];
        for (int j = 0; j < 4; ++j)
        {
            m_PPrior[i][j] = m_P[i][j];
        }
    }
}

void
CaTblEkf::Update(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    const size_t n = std::min(meas.size(), varM2.size());
    for (size_t k = 0; k < n; ++k)
    {
        const double dx = m_x[0] - meas[k].anchorX;
        const double dy = m_x[1] - meas[k].anchorY;
        const double rPred = std::sqrt(dx * dx + dy * dy);
        const double rSafe = std::max(rPred, 1e-6);

        // Jacobian row: range does not depend on velocity, so H[2] = H[3] = 0.
        const double H[4] = {dx / rSafe, dy / rSafe, 0.0, 0.0};

        // PH = P * H^T  (4x1)
        double PH[4];
        for (int i = 0; i < 4; ++i)
        {
            PH[i] = m_P[i][0] * H[0] + m_P[i][1] * H[1];
        }

        // S = H * P * H^T + R  (scalar)
        const double S = H[0] * PH[0] + H[1] * PH[1] + varM2[k];
        if (!(S > 0.0) || !std::isfinite(S))
        {
            continue;
        }

        double K[4];
        for (int i = 0; i < 4; ++i)
        {
            K[i] = PH[i] / S;
        }

        const double innovation = meas[k].rangeM - rPred;
        for (int i = 0; i < 4; ++i)
        {
            m_x[i] += K[i] * innovation;
        }

        // P <- (I - K H) P
        double newP[4][4];
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                newP[i][j] = m_P[i][j] - K[i] * (H[0] * m_P[0][j] + H[1] * m_P[1][j]);
            }
        }
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_P[i][j] = newP[i][j];
            }
        }

        // Enforce symmetry. Sequential scalar updates accumulate asymmetry over
        // long runs and an asymmetric covariance silently corrupts the
        // Mahalanobis gate downstream.
        for (int i = 0; i < 4; ++i)
        {
            for (int j = i + 1; j < 4; ++j)
            {
                const double avg = 0.5 * (m_P[i][j] + m_P[j][i]);
                m_P[i][j] = avg;
                m_P[j][i] = avg;
            }
        }
    }
}

void
CaTblEkf::RevertToPrior()
{
    for (int i = 0; i < 4; ++i)
    {
        m_x[i] = m_xPrior[i];
        for (int j = 0; j < 4; ++j)
        {
            m_P[i][j] = m_PPrior[i][j];
        }
    }
}

double
CaTblEkf::MahalanobisFromPrior(double zx, double zy) const
{
    const double dx = zx - m_xPrior[0];
    const double dy = zy - m_xPrior[1];

    const double a = m_PPrior[0][0];
    const double b = m_PPrior[0][1];
    const double c = m_PPrior[1][0];
    const double d = m_PPrior[1][1];
    const double det = a * d - b * c;
    if (std::fabs(det) < 1e-12)
    {
        return 0.0; // singular prior: cannot judge, so do not flag
    }

    // Inverse of the 2x2 position block.
    const double i00 = d / det;
    const double i01 = -b / det;
    const double i10 = -c / det;
    const double i11 = a / det;

    return dx * (i00 * dx + i01 * dy) + dy * (i10 * dx + i11 * dy);
}

double
CaTblEkf::MahalanobisFromPriorWithR(double zx, double zy, double rxx, double rxy, double ryy) const
{
    const double dx = zx - m_xPrior[0];
    const double dy = zy - m_xPrior[1];

    // S = H P H^T + R. H is the 2x4 position selector, so H P H^T is just the
    // position block of the prior, and R is the covariance of the raw fix being
    // tested. MahalanobisFromPrior() omits the second term entirely, which is what
    // makes the statistic scale with the measurement noise it exists to discount.
    const double a = m_PPrior[0][0] + rxx;
    const double b = m_PPrior[0][1] + rxy;
    const double c = m_PPrior[1][0] + rxy;
    const double d = m_PPrior[1][1] + ryy;
    const double det = a * d - b * c;
    if (std::fabs(det) < 1e-12)
    {
        return 0.0; // singular: cannot judge, so do not flag
    }

    const double i00 = d / det;
    const double i01 = -b / det;
    const double i10 = -c / det;
    const double i11 = a / det;

    return dx * (i00 * dx + i01 * dy) + dy * (i10 * dx + i11 * dy);
}

// ---------------------------------------------------------------------------
// Policy
// ---------------------------------------------------------------------------

CaTblPolicy::CaTblPolicy()
    : m_tier(CA_TBL_TIER_868),
      m_deescalateCounter(0),
      m_escalations(0),
      m_deescalations(0),
      m_suspects(0),
      m_noFixCycles(0)
{
    for (int i = 0; i < CA_TBL_TIER_COUNT; ++i)
    {
        m_tierCycles[i] = 0;
    }
}

void
CaTblPolicy::Configure(const CaTblPolicyConfig& config)
{
    m_config = config;
    m_ekf.SetProcessNoise(config.processNoiseStdPos, config.processNoiseStdVel);

    // The static arms pin the tier for the whole run; they are the single-band
    // baselines the architectural claim is measured against.
    if (config.mode == CA_TBL_MODE_STATIC_2P4)
    {
        m_tier = CA_TBL_TIER_2P4;
    }
    else if (config.mode == CA_TBL_MODE_STATIC_UWB)
    {
        m_tier = CA_TBL_TIER_UWB;
    }
    else
    {
        m_tier = CA_TBL_TIER_868;
    }
}

void
CaTblPolicy::Initialize(double x0, double y0, double posStdM, double velStdMps)
{
    m_ekf.Init(x0, y0, 0.0, 0.0, posStdM * posStdM, velStdMps * velStdMps);
    m_deescalateCounter = 0;
}

double
CaTblPolicy::ComputeGdop(double x, double y, const std::vector<CaTblRangeMeasurement>& meas)
{
    // G = H^T H over the 2-D position Jacobian; GDOP = sqrt(trace(G^-1)).
    double g00 = 0.0;
    double g01 = 0.0;
    double g11 = 0.0;
    for (size_t i = 0; i < meas.size(); ++i)
    {
        const double dx = x - meas[i].anchorX;
        const double dy = y - meas[i].anchorY;
        const double r = std::max(std::sqrt(dx * dx + dy * dy), 1e-6);
        const double ux = dx / r;
        const double uy = dy / r;
        g00 += ux * ux;
        g01 += ux * uy;
        g11 += uy * uy;
    }
    const double det = g00 * g11 - g01 * g01;
    if (std::fabs(det) < 1e-12)
    {
        return 100.0; // collinear or degenerate anchor geometry
    }
    const double trInv = (g11 + g00) / det;
    if (trInv <= 0.0)
    {
        return 100.0;
    }
    return std::sqrt(trInv);
}

bool
CaTblPolicy::RawMultilateration(const std::vector<CaTblRangeMeasurement>& meas,
                                double x0,
                                double y0,
                                double& outX,
                                double& outY)
{
    if (meas.size() < 3)
    {
        return false;
    }

    double px = x0;
    double py = y0;

    // Gauss-Newton on the range residuals. Replaces scipy least_squares from the
    // Python reference; 8 iterations is generous for this well-conditioned 2-D
    // problem and the loop exits early once the step is sub-millimetre.
    for (int iter = 0; iter < 8; ++iter)
    {
        double jtj00 = 0.0;
        double jtj01 = 0.0;
        double jtj11 = 0.0;
        double jtr0 = 0.0;
        double jtr1 = 0.0;

        for (size_t i = 0; i < meas.size(); ++i)
        {
            const double dx = px - meas[i].anchorX;
            const double dy = py - meas[i].anchorY;
            const double r = std::max(std::sqrt(dx * dx + dy * dy), 1e-6);
            const double ux = dx / r;
            const double uy = dy / r;
            const double residual = r - meas[i].rangeM;

            jtj00 += ux * ux;
            jtj01 += ux * uy;
            jtj11 += uy * uy;
            jtr0 += ux * residual;
            jtr1 += uy * residual;
        }

        const double det = jtj00 * jtj11 - jtj01 * jtj01;
        if (std::fabs(det) < 1e-12)
        {
            return false;
        }

        // Solve (J^T J) delta = -(J^T r)
        const double dxStep = -(jtj11 * jtr0 - jtj01 * jtr1) / det;
        const double dyStep = -(-jtj01 * jtr0 + jtj00 * jtr1) / det;

        px += dxStep;
        py += dyStep;

        if (std::fabs(dxStep) < 1e-3 && std::fabs(dyStep) < 1e-3)
        {
            break;
        }
        if (!std::isfinite(px) || !std::isfinite(py))
        {
            return false;
        }
    }

    outX = px;
    outY = py;
    return true;
}

bool
CaTblPolicy::RawMultilaterationWeighted(const std::vector<CaTblRangeMeasurement>& meas,
                                        const std::vector<double>& varM2,
                                        double x0,
                                        double y0,
                                        double& outX,
                                        double& outY,
                                        double& outRxx,
                                        double& outRxy,
                                        double& outRyy)
{
    const size_t n = std::min(meas.size(), varM2.size());
    if (n < 3)
    {
        return false;
    }

    double px = x0;
    double py = y0;
    double jtj00 = 0.0;
    double jtj01 = 0.0;
    double jtj11 = 0.0;

    for (int iter = 0; iter < 8; ++iter)
    {
        jtj00 = 0.0;
        jtj01 = 0.0;
        jtj11 = 0.0;
        double jtr0 = 0.0;
        double jtr1 = 0.0;

        for (size_t i = 0; i < n; ++i)
        {
            const double dx = px - meas[i].anchorX;
            const double dy = py - meas[i].anchorY;
            const double r = std::max(std::sqrt(dx * dx + dy * dy), 1e-6);
            const double ux = dx / r;
            const double uy = dy / r;
            const double residual = r - meas[i].rangeM;

            // The only change from the unweighted solver: each range contributes in
            // proportion to its precision. Without this, a 2500 m tier-1 range whose
            // variance is ~40x that of a 400 m one pulls the solution equally hard.
            const double w = 1.0 / std::max(varM2[i], 1e-12);

            jtj00 += w * ux * ux;
            jtj01 += w * ux * uy;
            jtj11 += w * uy * uy;
            jtr0 += w * ux * residual;
            jtr1 += w * uy * residual;
        }

        const double det = jtj00 * jtj11 - jtj01 * jtj01;
        if (std::fabs(det) < 1e-12)
        {
            return false;
        }

        const double dxStep = -(jtj11 * jtr0 - jtj01 * jtr1) / det;
        const double dyStep = -(-jtj01 * jtr0 + jtj00 * jtr1) / det;

        px += dxStep;
        py += dyStep;

        if (!std::isfinite(px) || !std::isfinite(py))
        {
            return false;
        }
        if (std::fabs(dxStep) < 1e-3 && std::fabs(dyStep) < 1e-3)
        {
            break;
        }
    }

    // (J^T R^-1 J)^-1 is the covariance of the weighted least-squares fix, which is
    // exactly the R the integrity gate needs. Recovered from the final accumulators
    // at no extra cost -- this is why weighting and the S fix are one change.
    const double det = jtj00 * jtj11 - jtj01 * jtj01;
    if (std::fabs(det) < 1e-12)
    {
        return false;
    }
    outRxx = jtj11 / det;
    outRxy = -jtj01 / det;
    outRyy = jtj00 / det;

    outX = px;
    outY = py;
    return true;
}

double
CaTblPolicy::ComputePcs(double traceSigma,
                        double gdop,
                        uint32_t nLos,
                        uint32_t nTotal,
                        uint32_t hopCount) const
{
    // Term 1: filter uncertainty, mapped to (0,1] so it cannot dominate.
    const double term1 = m_config.traceRef / (m_config.traceRef + std::max(traceSigma, 0.0));
    // Term 2: geometric quality.
    const double term2 = 1.0 / std::max(gdop, 1.0);
    // Term 3: fraction of contributing links believed line-of-sight.
    const double term3 = (nTotal == 0) ? 0.0 : static_cast<double>(nLos) / static_cast<double>(nTotal);

    double pcs = term1 + term2 + term3;

    // Term 4: hop-provenance penalty. Inactive by default (hopWeight = 0)
    // because the current scenario is single-hop -- there is no mesh to
    // traverse, so switching this on now would penalise a hop count that is
    // structurally always zero and produce a meaningless constant. It exists so
    // that S5 (mesh + multi-hop) can activate it without reworking the score.
    if (m_config.hopWeight > 0.0 && hopCount > 0)
    {
        pcs -= m_config.hopWeight * static_cast<double>(hopCount);
    }

    return pcs;
}

CaTblDecision
CaTblPolicy::Step(double dt,
                  const std::vector<CaTblRangeMeasurement>& meas,
                  uint32_t hopCount,
                  int32_t activeTierRanges,
                  int32_t activeTierNeighbours)
{
    CaTblDecision d;
    d.activeTier = m_tier;
    d.nextTier = m_tier;

    m_tierCycles[m_tier]++;

    m_ekf.Predict(dt);

    // Did the ACTIVE tier deliver, as distinct from "did we get a fix from
    // somewhere". A concurrent-tier caller substitutes tier-1 ranges when the active
    // tier returns too few, so meas.size() reports the fix and this reports the tier.
    // De-escalation must be driven by the latter or the policy cannot detect that it
    // has escalated into no coverage. See the note on Step() in the header.
    const bool activeDelivered =
        (activeTierRanges < 0) ? (meas.size() >= 3)
                               : (static_cast<uint32_t>(activeTierRanges) >= 3);

    // Fewer than three ranges cannot constrain a 2-D fix. Coast on the
    // prediction. Note this is where Case B (1 anchor + 1 peer) and Case C
    // (relative-only) from the design would plug in; they are not implemented
    // yet, so sparse geometry currently degrades to dead reckoning.
    if (meas.size() < 3)
    {
        m_noFixCycles++;
        d.hadFix = false;
        d.estX = m_ekf.X();
        d.estY = m_ekf.Y();
        d.traceSigma = m_ekf.TracePos();
        d.nAnchorsUsed = static_cast<uint32_t>(meas.size());

        // A dropout is evidence the current tier is not delivering. Count it
        // toward de-escalation so a node cannot get stranded on a tier whose
        // range it has walked out of.
        if (m_config.mode == CA_TBL_MODE_FULL || m_config.mode == CA_TBL_MODE_NO_INTEGRITY ||
            m_config.mode == CA_TBL_MODE_FIXED_THRESHOLD)
        {
            m_deescalateCounter++;
            if (m_deescalateCounter >= m_config.nDeescalate)
            {
                if (m_tier > CA_TBL_TIER_868)
                {
                    m_tier = static_cast<CaTblTier>(static_cast<int>(m_tier) - 1);
                    m_deescalations++;
                    d.deescalated = true;
                }
                m_deescalateCounter = 0;
            }
        }
        d.nextTier = m_tier;
        return d;
    }

    d.hadFix = true;
    d.nAnchorsUsed = static_cast<uint32_t>(meas.size());

    // Per-measurement variance, inflated for NLOS-tagged links. This is the
    // NLOS-adaptive R the design calls for.
    std::vector<double> varM2;
    varM2.reserve(meas.size());
    uint32_t nLos = 0;
    for (size_t i = 0; i < meas.size(); ++i)
    {
        // Prefer the physics engine's per-measurement sigma. The per-tier constants
        // are the fallback for RANGING_SYNTHETIC_GPS_DENIED, where no sigma is
        // produced, and they are wrong by large factors against the measured error:
        // losSigmaM[868] = 50 m against a tier-1 error reaching 2907 m at 6 km, and
        // losSigmaM[2p4] = 15 m against a tier-2 error of ~3 m.
        double sigma;
        if (meas[i].sigmaM > 0.0)
        {
            sigma = meas[i].sigmaM;
            if (meas[i].nlos)
            {
                // The physics sigma already uses the NLOS hardware floor, but it does
                // NOT contain the NLOS excess-delay BIAS, which is an unmodelled
                // positive offset. Variance inflation is the standard EKF treatment
                // for an unknown bias. The factor is UNCALIBRATED -- sweep in S7.
                sigma *= m_config.nlosSigmaInflation;
            }
        }
        else
        {
            sigma = meas[i].nlos ? m_config.nlosSigmaM[m_tier] : m_config.losSigmaM[m_tier];
        }
        varM2.push_back(sigma * sigma);
        if (!meas[i].nlos)
        {
            nLos++;
        }
    }
    d.nNlos = static_cast<uint32_t>(meas.size()) - nLos;

    // Integrity check, evaluated on the PRIOR, before the measurement is fused.
    //
    // The mechanism: solve a filter-free fix from this cycle's ranges and ask
    // how far it sits from where physical continuity says the node should be.
    // A fix that is statistically clean (all links reporting LOS) but violates
    // continuity is exactly the failure the confidence score cannot see, since
    // every input it reads looks healthy. That is the case this flags.
    //
    // Requiring ALL links to be LOS is a strict condition inherited from the
    // Python reference. It makes the flag specific but almost certainly costs
    // recall; the LOS-fraction cut-off is a parameter that S7 should sweep
    // rather than a value with any justification behind it.
    bool suspect = false;
    if (m_config.mode == CA_TBL_MODE_FULL)
    {
        double zx = 0.0;
        double zy = 0.0;
        bool haveFix = false;
        double mahal = 0.0;

        if (m_config.mahalUseR)
        {
            double rxx = 0.0;
            double rxy = 0.0;
            double ryy = 0.0;
            haveFix = RawMultilaterationWeighted(meas,
                                                 varM2,
                                                 m_ekf.PriorX(),
                                                 m_ekf.PriorY(),
                                                 zx,
                                                 zy,
                                                 rxx,
                                                 rxy,
                                                 ryy);
            if (haveFix)
            {
                mahal = m_ekf.MahalanobisFromPriorWithR(zx, zy, rxx, rxy, ryy);
            }
        }
        else
        {
            haveFix = RawMultilateration(meas, m_ekf.PriorX(), m_ekf.PriorY(), zx, zy);
            if (haveFix)
            {
                mahal = m_ekf.MahalanobisFromPrior(zx, zy);
            }
        }

        if (haveFix)
        {
            d.mahalanobis = mahal;
            const bool allLos = (d.nNlos == 0);
            if (mahal > m_config.mahalanobisThreshold && allLos)
            {
                suspect = true;
                m_suspects++;
            }
        }
    }
    d.suspect = suspect;

    if (suspect)
    {
        // Reject: keep the prior rather than fusing a fix we do not trust.
        m_ekf.RevertToPrior();
        d.measurementRejected = true;
    }
    else
    {
        m_ekf.Update(meas, varM2);
    }

    const double traceSigma = m_ekf.TracePos();
    const double gdop = ComputeGdop(m_ekf.X(), m_ekf.Y(), meas);
    const double pcs = ComputePcs(traceSigma, gdop, nLos, static_cast<uint32_t>(meas.size()), hopCount);

    d.traceSigma = traceSigma;
    d.gdop = gdop;
    d.pcs = pcs;
    d.estX = m_ekf.X();
    d.estY = m_ekf.Y();

    // ------------------------------------------------------------- decision --

    bool wantEscalate = false;
    const bool dynamic = (m_config.mode == CA_TBL_MODE_FULL ||
                          m_config.mode == CA_TBL_MODE_NO_INTEGRITY ||
                          m_config.mode == CA_TBL_MODE_FIXED_THRESHOLD);

    if (m_config.mode == CA_TBL_MODE_FIXED_THRESHOLD)
    {
        // The literature baseline: a flat variance threshold, no confidence
        // model, no integrity check, no geometry term.
        wantEscalate = (traceSigma > m_config.fixedVarThresholdM2);
    }
    else if (m_config.mode == CA_TBL_MODE_FULL || m_config.mode == CA_TBL_MODE_NO_INTEGRITY)
    {
        wantEscalate = (pcs < m_config.pcsThresholdUp[m_tier]);
        if (suspect)
        {
            // A distrusted fix is itself grounds to escalate: the coarse tier
            // has stopped being believable, which is precisely when the
            // precision tier earns its energy.
            wantEscalate = true;
        }
    }

    if (dynamic && !activeDelivered)
    {
        // A fix was obtained, but NOT from the active tier -- the caller fell back to
        // the tier-1 floor. That is direct evidence the active tier has no coverage
        // here, so it counts toward demotion exactly as a total dropout does, and the
        // PCS decision is bypassed.
        //
        // Bypassing PCS is required, not merely tidy: pcs on this cycle was computed
        // from the FALLBACK fix, so it describes tier-1 confidence. Feeding it to
        // pcsThresholdUp[m_tier], which is the threshold for the tier the node is
        // nominally on, would compare a tier-1 confidence against a UWB threshold.
        // Low PCS would then read as "escalate further" at the exact moment the
        // correct action is to come down.
        // A cycle in which the active tier addressed ZERO anchors is categorically
        // different from one whose anchors were addressed and failed to answer: it
        // means the tier has nothing here at all, after a full discovery window.
        // Waiting nDeescalate cycles to learn that costs 0.6299 J each on UWB.
        const bool nothingThere =
            m_config.fastDemoteOnZeroNeighbours && activeTierNeighbours == 0;

        m_deescalateCounter++;
        if (nothingThere || m_deescalateCounter >= m_config.nDeescalate)
        {
            if (m_tier > CA_TBL_TIER_868)
            {
                m_tier = static_cast<CaTblTier>(static_cast<int>(m_tier) - 1);
                m_deescalations++;
                d.deescalated = true;
            }
            m_deescalateCounter = 0;
        }
    }
    else if (dynamic && wantEscalate)
    {
        if (m_tier < CA_TBL_TIER_UWB)
        {
            m_tier = static_cast<CaTblTier>(static_cast<int>(m_tier) + 1);
            m_escalations++;
            d.escalated = true;
            m_deescalateCounter = 0;
        }
    }
    else if (dynamic)
    {
        // De-escalation requires sustained good behaviour, and the down
        // threshold sits above the up threshold. The gap is deliberate: for SAR
        // a missed escalation costs more than a wasted one, so the system is
        // biased toward staying at higher precision.
        const bool healthy = (m_config.mode == CA_TBL_MODE_FIXED_THRESHOLD)
                                 ? (traceSigma < m_config.fixedVarThresholdM2 * 0.5)
                                 : (!suspect && pcs > m_config.pcsThresholdDown[m_tier]);
        if (healthy)
        {
            m_deescalateCounter++;
        }
        else
        {
            m_deescalateCounter = 0;
        }

        if (m_deescalateCounter >= m_config.nDeescalate)
        {
            if (m_tier > CA_TBL_TIER_868)
            {
                m_tier = static_cast<CaTblTier>(static_cast<int>(m_tier) - 1);
                m_deescalations++;
                d.deescalated = true;
            }
            m_deescalateCounter = 0;
        }
    }

    d.nextTier = m_tier;
    return d;
}

} // namespace ns3
