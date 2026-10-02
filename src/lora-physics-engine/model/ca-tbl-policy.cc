#include "ca-tbl-policy.h"

#include "ca-tbl-sparse-anchor.h"

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

CaTblRangeMeasurement::CaTblRangeMeasurement()
    : anchorX(0.0),
      anchorY(0.0),
      rangeM(0.0),
      nlos(false),
      nlosPropagation(false),
      hiddenChannelActive(false),
      sigmaM(0.0),
      nlosScore(-1.0)
{
}

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

CaTblPolicyConfig::CaTblPolicyConfig()
{
    mode = CA_TBL_MODE_FULL;
    pcsForm = CA_TBL_PCS_LEGACY; // default preserves the pre-existing baseline

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

    // CA_TBL_PCS_WEIGHTED. Hand-set, NOT from LOOCV -- CAL-01 remains unmet. The
    // one property they are chosen for: pcsW2 + pcsW3 = 1.0 is below the lowest
    // pcsThresholdUp (1.55), so term1 = 0 puts the score under threshold whatever
    // geometry and LOS do. That is the veto the unweighted sum cannot express.
    pcsW1 = 2.0;
    pcsW2 = 0.5;
    pcsW3 = 0.5;
    // CA_TBL_PCS_GATED. Ours. term1 = traceRef/(traceRef+traceSigma), so 0.10
    // corresponds to traceSigma = 9*traceRef, i.e. posterior radial sigma 3x the
    // 3.1623 m target. Below that the filter is not tracking to spec and the
    // score should say so rather than being carried by the other two terms.
    pcsGateFloor = 0.10;
    pcsNlosFromScore = false; // default preserves the pre-existing baseline
    fastDemoteOnZeroNeighbours = false; // default preserves the pre-existing baseline
    multiStepDemote = false;            // default preserves the pre-existing baseline

    // true preserves the pre-existing baseline. See the header note: this is what
    // makes the integrity gate measurable as a gate rather than as a trigger.
    suspectForcesEscalation = true;

    mahalanobisThreshold = 9.21; // chi-square 99%, 2 DOF
    mahalUseR = false;           // default preserves the pre-existing baseline
    suspectRecovery = CA_TBL_RECOVERY_NONE; // default preserves the pre-existing baseline
    suspectInflateFactor = 4.0;
    suspectRecoveryN = 5;
    escalationRequiresEvidence = false;   // default preserves the pre-existing baseline
    escalationRequiresEvidenceT2 = false; // rebuilt as a predictor; see the header
    t2PredictRadiusM = 156.0;             // probe r80, post environment correction
    t2PredictMinAnchors = 3;              // three is what a fix needs
    uwbPredictRadiusM = 20.0;
    uwbPredictMaxSigmaFrac = 0.25;
    uwbEvidenceFromNeighbours = false; // default preserves the pre-existing baseline
    traceRef = 10.0;
    hopWeight = 0.0; // no mesh yet -- see header note
    hopProvenanceForm = 0;
    hopPenalty = 0.0;

    // Switching-point integrity, both off by default so the effect can be measured.
    imuCrossCheck = false;
    imuConsistencyK = 3.0;
    imuSigmaFloorM = 5.0;
    corroborationCheck = false;
    corroborationFloor = 0.5;
    corroborationMinWitnesses = 2;
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
      nNlos(0),
      nNlosPropagation(0),
      nHiddenChannel(0),
      meanSigmaM(0.0),
      meanSigmaRawM(0.0),
      sigmaGeoM(-1.0),
      predictedVisUwb(-1),
      predictedVis2p4(-1),
      nNlosScored(0),
      nlosScoreTerm(-1.0),
      nlosScoreFed(false)
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
CaTblEkf::PredictWithControl(double dt, double ux, double uy, double quxx, double quxy,
                             double quyy)
{
    m_x[0] += ux;
    m_x[1] += uy;
    const double inv = (dt > 0.0) ? 1.0 / dt : 0.0;
    m_x[2] = ux * inv;
    m_x[3] = uy * inv;
    const double q[2][2] = {{quxx, quxy}, {quxy, quyy}};
    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            m_P[i][j] += q[i][j];
            m_P[i][j + 2] = q[i][j] * inv;
            m_P[i + 2][j] = q[i][j] * inv;
            m_P[i + 2][j + 2] = q[i][j] * inv * inv;
        }
    }
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
CaTblEkf::FusePosition(double zx, double zy, double varM2)
{
    if (!(varM2 > 0.0) || !std::isfinite(varM2) || !std::isfinite(zx) ||
        !std::isfinite(zy))
    {
        return;
    }

    // Two scalar updates rather than one 2x2 solve. R is diagonal here, so the
    // two are algebraically the joint update, and this reuses Update()'s scalar
    // arithmetic instead of introducing a second inversion path to keep correct.
    const double z[2] = {zx, zy};
    for (int axis = 0; axis < 2; ++axis)
    {
        // H selects one position component. H[2] = H[3] = 0: a position
        // observation says nothing DIRECTLY about velocity -- what reaches
        // velocity arrives through P's off-diagonal block below.
        double H[4] = {0.0, 0.0, 0.0, 0.0};
        H[axis] = 1.0;

        // PH = P * H^T reduces to one column of P.
        double PH[4];
        for (int i = 0; i < 4; ++i)
        {
            PH[i] = m_P[i][axis];
        }

        const double S = PH[axis] + varM2;
        if (!(S > 0.0) || !std::isfinite(S))
        {
            continue;
        }

        double K[4];
        for (int i = 0; i < 4; ++i)
        {
            K[i] = PH[i] / S;
        }

        const double innovation = z[axis] - m_x[axis];
        for (int i = 0; i < 4; ++i)
        {
            m_x[i] += K[i] * innovation;
        }

        // P <- (I - K H) P, with H selecting a single row.
        double newP[4][4];
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                newP[i][j] = m_P[i][j] - K[i] * m_P[axis][j];
            }
        }
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_P[i][j] = newP[i][j];
            }
        }

        // Same reason as Update(): sequential scalar updates accumulate
        // asymmetry, and an asymmetric P silently corrupts the Mahalanobis gate.
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

// Out of line on purpose: Step() gains a call and a branch, not an inlined 2x2 update
// sitting among the expressions the byte-identical null control holds fixed.
__attribute__((noinline)) void
CaTblPolicy::FuseAux()
{
    if (m_ekf.FusePositionCov(m_aux[0], m_aux[1], m_aux[2], m_aux[3], m_aux[4]))
    {
        m_auxFusedLast = true;
        ++m_auxFused;
    }
}

void
CaTblPolicy::FuseAuxInflated(double rScale)
{
    if (m_ekf.FusePositionCov(m_aux[0],
                              m_aux[1],
                              rScale * m_aux[2],
                              rScale * m_aux[3],
                              rScale * m_aux[4]))
    {
        m_auxFusedLast = true;
        ++m_auxFused;
    }
}

bool
CaTblEkf::FusePositionCov(double zx, double zy, double rxx, double rxy, double ryy)
{
    if (!std::isfinite(zx) || !std::isfinite(zy) || !std::isfinite(rxx) ||
        !std::isfinite(rxy) || !std::isfinite(ryy) || !(rxx > 0.0) || !(ryy > 0.0) ||
        !(rxx * ryy - rxy * rxy > 0.0))
    {
        return false;
    }

    // S = H P H' + R, H selecting the two position states.
    const double pxy = 0.5 * (m_P[0][1] + m_P[1][0]);
    const double s00 = m_P[0][0] + rxx;
    const double s01 = pxy + rxy;
    const double s11 = m_P[1][1] + ryy;
    const double det = s00 * s11 - s01 * s01;
    if (!(det > 0.0) || !std::isfinite(det))
    {
        return false;
    }
    const double i00 = s11 / det;
    const double i01 = -s01 / det;
    const double i11 = s00 / det;

    // K = P H' S^-1: P H' is the first two columns of P.
    double K[4][2];
    for (int i = 0; i < 4; ++i)
    {
        K[i][0] = m_P[i][0] * i00 + m_P[i][1] * i01;
        K[i][1] = m_P[i][0] * i01 + m_P[i][1] * i11;
    }
    const double v0 = zx - m_x[0];
    const double v1 = zy - m_x[1];
    for (int i = 0; i < 4; ++i)
    {
        m_x[i] += K[i][0] * v0 + K[i][1] * v1;
    }

    // P <- P - K (H P): H P is the first two rows of P.
    double newP[4][4];
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            newP[i][j] = m_P[i][j] - K[i][0] * m_P[0][j] - K[i][1] * m_P[1][j];
        }
    }
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m_P[i][j] = newP[i][j];
        }
    }
    // Same reason as Update(): an asymmetric P silently corrupts the Mahalanobis gate.
    for (int i = 0; i < 4; ++i)
    {
        for (int j = i + 1; j < 4; ++j)
        {
            const double avg = 0.5 * (m_P[i][j] + m_P[j][i]);
            m_P[i][j] = avg;
            m_P[j][i] = avg;
        }
    }
    return true;
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
      // Initialised here since 2026-09-30. Until then these three were zero only because
      // the scenario value-initialises its std::vector<CaTblTagState>, which zero-fills
      // before this constructor runs; a CaTblPolicy constructed any other way started with
      // indeterminate counters (docs/SPEC_CONFORMANCE_PLAN.md sect 1.H item 2).
      m_consecutiveRejections(0),
      m_reinitFallbackCount(0),
      m_recoveryFirings(0),
      m_predictBlockedQuality(0),
      m_predictBlockedCoverage(0),
      m_evidenceBlockedT2(0),
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

void
CaTblPolicyConfig::ApplyV2Thresholds()
{
    // Pre-registered in report/PCS_REDESIGN_RESULTS.md §0.2/§0.3, committed before
    // any v2 run. Derived, not chosen -- see the header note.
    for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
    {
        pcsThresholdUp[k] = 0.0;   // sigma_eff == sigma_ref == 3.1623 m, the target
        pcsThresholdDown[k] = 0.301029995663981; // log10(2): one octave of hysteresis
    }
}

double
CaTblPolicy::ComputePcs(double traceSigma,
                        double gdop,
                        uint32_t nLos,
                        uint32_t nTotal,
                        uint32_t hopCount,
                        double sigmaGeoM,
                        double losTermOverride) const
{
    if (m_config.pcsForm == CA_TBL_PCS_V2)
    {
        // pcs = log10(sigma_ref / sqrt(sigma_post * sigma_geo))
        //
        // "How many decades of position uncertainty separate this fix from the
        // stated 3.1623 m target." Zero at the target, negative when worse.
        //
        // The geometric mean rather than the sum of two log terms: it makes one
        // score unit exactly one decade of uncertainty, so the pre-registered
        // hysteresis constant log10(2) means a factor of 2 as stated. Summing
        // would have made it a factor of 1.41. This changes no decision at
        // threshold 0 (0.5x > 0 iff x > 0) -- it is a fidelity fix to the
        // pre-registered rule, not a tuning knob.
        const double sigmaRef = std::sqrt(std::max(m_config.traceRef, 1e-12));
        const double sigmaPost = std::sqrt(std::max(traceSigma, 1e-12));

        // A failed weighted solve (collinear or degenerate anchor geometry) means
        // the fix quality is unknown. Treat that as maximally unconfident rather
        // than silently substituting an optimistic value: for SAR a missed
        // escalation costs more than a wasted one, and the caller signals the
        // condition by passing a non-positive sigmaGeoM.
        const double sigmaGeo = (sigmaGeoM > 0.0 && std::isfinite(sigmaGeoM))
                                    ? sigmaGeoM
                                    : 1e6;

        const double sigmaEff = std::sqrt(sigmaPost * sigmaGeo);
        double pcs = std::log10(sigmaRef / std::max(sigmaEff, 1e-12));

        // Hop-provenance penalty, still inert (hopWeight = 0, no mesh). Applied
        // on the same scale as the rest of the score.
        if (m_config.hopWeight > 0.0 && hopCount > 0)
        {
            pcs -= m_config.hopWeight * static_cast<double>(hopCount);
        }
        pcs = ApplyProvenance(pcs);
        return pcs;
    }

    // Term 1: filter uncertainty, mapped to (0,1] so it cannot dominate.
    const double term1 = m_config.traceRef / (m_config.traceRef + std::max(traceSigma, 0.0));
    // Term 2: geometric quality.
    const double term2 = 1.0 / std::max(gdop, 1.0);
    // Term 3: fraction of contributing links believed line-of-sight -- or, under
    // pcsNlosFromScore, the spec's mean (1 - NLOS_score), which Step() passes in. The
    // override replaces this one term; the sums below are the same code either way.
    const double term3 = (losTermOverride >= 0.0)
                             ? losTermOverride
                             : ((nTotal == 0) ? 0.0
                                              : static_cast<double>(nLos) / static_cast<double>(nTotal));

    // LEGACY sums these unweighted, which is the saturation defect: term2 + term3
    // alone reach 2.0 against a 1.55 threshold, so term1 cannot veto. The two
    // repairs below are opt-in; the LEGACY path is byte-unchanged.
    double pcs;
    if (m_config.pcsForm == CA_TBL_PCS_WEIGHTED)
    {
        pcs = m_config.pcsW1 * term1 + m_config.pcsW2 * term2 + m_config.pcsW3 * term3;
    }
    else if (m_config.pcsForm == CA_TBL_PCS_GATED && term1 < m_config.pcsGateFloor)
    {
        pcs = 0.0;
    }
    else
    {
        pcs = term1 + term2 + term3;
    }

    // Term 4: hop-provenance penalty. Inactive by default (hopWeight = 0)
    // because the current scenario is single-hop -- there is no mesh to
    // traverse, so switching this on now would penalise a hop count that is
    // structurally always zero and produce a meaningless constant. It exists so
    // that S5 (mesh + multi-hop) can activate it without reworking the score.
    if (m_config.hopWeight > 0.0 && hopCount > 0)
    {
        pcs -= m_config.hopWeight * static_cast<double>(hopCount);
    }

    pcs = ApplyProvenance(pcs);
    return pcs;
}

double
CaTblPolicy::ApplyProvenance(double pcs) const
{
    if (m_config.hopProvenanceForm != 1 || m_config.hopPenalty <= 0.0)
    {
        return pcs;
    }
    // Bounded and multiplicative. The divisor is at least 1, so the factor lies in
    // (0, 1] and the score can be reduced towards zero but never driven through it --
    // which the legacy subtraction could do, and did so without limit.
    //
    // Driven by the relayed VARIANCE share rather than a hop count: one relayed range
    // among nine direct ones is a different thing from a fix built entirely out of
    // relayed ones, and a count cannot tell those apart.
    const double f = std::min(1.0, std::max(0.0, m_relayedVarFrac));
    return pcs / (1.0 + m_config.hopPenalty * f);
}

bool
CaTblPolicy::UwbEvidenceWouldBlock(double traceSigma,
                                   const std::vector<CaTblRangeMeasurement>& meas) const
{
    // DC-9. Step()'s evidence-gate UWB branch, side-effect free. The tier-2 branch runs
    // BEFORE the witness, so a witness that was evaluated has already passed it.
    const bool haveMap = !m_config.uwbAnchorX.empty();
    if (!m_config.escalationRequiresEvidence || m_tier != CA_TBL_TIER_2P4 ||
        !(m_config.uwbEvidenceFromNeighbours || haveMap))
    {
        return false;
    }
    const double sigPos = std::sqrt(std::max(traceSigma, 0.0));
    if (sigPos > m_config.uwbPredictMaxSigmaFrac * m_config.uwbPredictRadiusM)
    {
        return true;
    }
    if (m_config.uwbEvidenceFromNeighbours)
    {
        for (size_t i = 0; i < meas.size(); ++i)
        {
            if (meas[i].rangeM > 0.0 && meas[i].rangeM <= m_config.uwbPredictRadiusM)
            {
                return false;
            }
        }
        return true;
    }
    const double r2 = m_config.uwbPredictRadiusM * m_config.uwbPredictRadiusM;
    uint32_t vis = 0;
    for (size_t i = 0; i < m_config.uwbAnchorX.size(); ++i)
    {
        const double dx = m_config.uwbAnchorX[i] - m_ekf.X();
        const double dy = m_config.uwbAnchorY[i] - m_ekf.Y();
        if (dx * dx + dy * dy <= r2)
        {
            ++vis;
        }
    }
    return vis < 3;
}

bool
CaTblPolicy::ConfirmUwbGrant()
{
    if (!m_uwbPending || m_tier != CA_TBL_TIER_2P4)
    {
        ++m_uwbLate;
        return false;
    }
    m_uwbPending = false;
    m_tier = CA_TBL_TIER_UWB;
    m_escalations++;
    m_deescalateCounter = 0;
    ++m_uwbConfirmed;
    return true;
}

bool
CaTblPolicy::ClearUwbPending(bool timeout)
{
    if (!m_uwbPending)
    {
        return false;
    }
    m_uwbPending = false;
    if (timeout)
    {
        ++m_uwbTimeouts;
    }
    else
    {
        ++m_uwbDenied;
    }
    return true;
}

void
CaTblPolicy::AbortTierChange(CaTblTier restoreTo)
{
    // NW-7: an escalation into UWB that is only pending changed no tier and counted
    // nothing, so aborting it drops the request and restores nothing.
    if (m_uwbPending && m_tier == restoreTo)
    {
        m_uwbPending = false;
        ++m_uwbAborted;
        m_deescalateCounter = 0;
        return;
    }
    m_tier = restoreTo;
    if (m_escalations > 0)
    {
        m_escalations--;
    }
    // The de-escalation counter was zeroed on escalation; leaving it zeroed is
    // correct -- the node has not accumulated evidence for demotion either.
    m_deescalateCounter = 0;
}

CaTblDecision
CaTblPolicy::Step(double dt,
                  const std::vector<CaTblRangeMeasurement>& meas,
                  uint32_t hopCount,
                  int32_t activeTierRanges,
                  int32_t activeTierNeighbours,
                  uint32_t tierEvidenceMask)
{
    CaTblDecision d;
    // NW-7. A request is pending only on 2.4 GHz; a demotion since (coverage loss, which
    // the freeze allows) has cancelled it. Otherwise this is one more cycle waiting.
    if (m_uwbPending)
    {
        if (m_tier != CA_TBL_TIER_2P4)
        {
            m_uwbPending = false;
            ++m_uwbCoverageClears;
        }
        else
        {
            ++m_uwbWaitCycles;
        }
    }
    d.activeTier = m_tier;
    d.nextTier = m_tier;

    // A queued tier-1 TDoA fix belongs to THIS cycle and to no other, whatever the cycle
    // decides below (SetAuxPositionFix). Never queued, these three lines are inert.
    const bool auxQueued = m_auxPending;
    m_auxPending = false;
    m_auxFusedLast = false;

    m_tierCycles[m_tier]++;

    if (m_imuPending)
    {
        // DC-1: the queued synthetic-IMU displacement drives the prediction.
        m_imuPending = false;
        m_ekf.PredictWithControl(dt, m_imu[0], m_imu[1], m_imu[2], m_imu[3], m_imu[4]);
        d.imuPredicted = true;
    }
    else
    {
        m_ekf.Predict(dt);
    }

    // Did the ACTIVE tier deliver, as distinct from "did we get a fix from
    // somewhere". A concurrent-tier caller substitutes tier-1 ranges when the active
    // tier returns too few, so meas.size() reports the fix and this reports the tier.
    // De-escalation must be driven by the latter or the policy cannot detect that it
    // has escalated into no coverage. See the note on Step() in the header.
    const bool activeDelivered =
        (activeTierRanges < 0) ? (meas.size() >= 3)
                               : (static_cast<uint32_t>(activeTierRanges) >= 3);

    // DC-11. Two ranges, solved HERE, after the prediction: the current state IS the
    // propagated prior Physical 2.4 resolves Cases A/B against (the scenario's pre-Step
    // solve used the previous posterior). Case A when both ranges are to surveyed
    // gateways, Case B otherwise -- provenance, not geometry, decides, as before. The
    // point is fused ONCE, as a position, with the same conservative variance the
    // scenario used, 2 (s0^2 + s1^2); the two ranges are never also applied as ranges,
    // which closes the double count of Case A plus relays (plan sect 1.H item 9) -- with
    // relays appended the set has three or more and takes the full path instead. A
    // solved cycle is a fix and does not count toward demotion. A refused one falls
    // through to the coast below, exactly as before.
    if (m_config.sparseInStep && meas.size() == 2)
    {
        const bool bothSurveyed = meas[0].sourceKind == CA_TBL_SRC_SURVEYED_GATEWAY &&
                                  meas[1].sourceKind == CA_TBL_SRC_SURVEYED_GATEWAY;
        d.sparsePriorX = m_ekf.X();
        d.sparsePriorY = m_ekf.Y();
        d.sparsePriorSource = d.imuPredicted ? 1 : 0; // DC-2
        const CaTblSparseResult sr =
            bothSurveyed
                ? CaTblSolveCaseA(meas[0].anchorX, meas[0].anchorY, meas[0].rangeM,
                                  meas[1].anchorX, meas[1].anchorY, meas[1].rangeM,
                                  m_ekf.X(), m_ekf.Y(), m_ekf.Pxx(), m_ekf.Pxy(), m_ekf.Pyy(),
                                  m_config.sparseMarginM)
                : CaTblSolveCaseB(meas[0].anchorX, meas[0].anchorY, meas[0].rangeM,
                                  meas[1].anchorX, meas[1].anchorY, meas[1].rangeM,
                                  m_ekf.X(), m_ekf.Y(), m_ekf.Pxx(), m_ekf.Pxy(), m_ekf.Pyy(),
                                  m_config.sparseMarginM);
        d.sparseVerdict = static_cast<uint8_t>(sr.verdict);
        const double var = 2.0 * (meas[0].sigmaM * meas[0].sigmaM +
                                  meas[1].sigmaM * meas[1].sigmaM);
        if (sr.verdict == CA_TBL_SPARSE_OK && var > 0.0 && std::isfinite(var))
        {
            m_ekf.FusePosition(sr.x, sr.y, var);
            d.positionFuses = 1;
            d.sparseX = sr.x;
            d.sparseY = sr.y;
            d.sparseVarM2 = var;
            d.fixKind = (sr.solvedBy == CA_TBL_CASE_B) ? CA_TBL_FIX_CASE_B : CA_TBL_FIX_CASE_A;
            d.hadFix = true;
            d.estX = m_ekf.X();
            d.estY = m_ekf.Y();
            d.traceSigma = m_ekf.TracePos();
            d.nAnchorsUsed = 2;
            d.nextTier = m_tier;
            return d;
        }
    }

    // DC-4. Case D: exactly one range, under the flag, goes down the ordinary path below
    // as a scalar update on the propagated state (NIS-guarded), and reaches the decision.
    const bool caseD = m_config.caseD && meas.size() == 1;

    // Fewer than three ranges cannot constrain a 2-D fix. Coast on the
    // prediction. Cases A and B now exist (ca-tbl-sparse-anchor, since ac5766f). By
    // default they run in the SCENARIO, before Step(), against the previous posterior --
    // so a two-range cycle still arrives here, is counted a no-fix and counts toward
    // demotion. Under sparseInStep (DC-11) they run above, and only a refused one
    // arrives here. Case C only classifies; Case D does not exist yet (DC-4).
    if (meas.size() < 3 && !caseD)
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
    if (m_config.sparseInStep)
    {
        d.fixKind = CA_TBL_FIX_FULL; // DC-11: the census reads it
    }
    if (caseD)
    {
        // DC-4: not a position fix -- one range constrains one direction -- but the
        // covariance it leaves is live.
        d.hadFix = false;
        d.fixKind = CA_TBL_FIX_CASE_D;
        m_noFixCycles++;
        m_caseDCycles++;
    }

    // Per-measurement variance, inflated for NLOS-tagged links. This is the
    // NLOS-adaptive R the design calls for.
    std::vector<double> varM2;
    varM2.reserve(meas.size());
    double sumVarRaw = 0.0;
    uint32_t nLos = 0;
    uint32_t nNlosProp = 0;
    uint32_t nHidden = 0;
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
        // Raw = pre-inflation. Recomputed rather than captured above so the two
        // branches of the sigma selection stay legible.
        {
            const double raw = (meas[i].sigmaM > 0.0)
                                   ? meas[i].sigmaM
                                   : (meas[i].nlos ? m_config.nlosSigmaM[m_tier]
                                                   : m_config.losSigmaM[m_tier]);
            sumVarRaw += raw * raw;
        }
        if (!meas[i].nlos)
        {
            nLos++;
        }
        // Counted regardless of which source is operative, so the two populations
        // are always directly comparable in the trace.
        if (meas[i].nlosPropagation)
        {
            nNlosProp++;
        }
        if (meas[i].hiddenChannelActive)
        {
            nHidden++;
        }
    }
    d.nNlos = static_cast<uint32_t>(meas.size()) - nLos;
    d.nNlosPropagation = nNlosProp;
    d.nHiddenChannel = nHidden;

    // The classifier's PCS term 3 (TECH_DEBT L.2.2): mean (1 - NLOS_score) over the links
    // that carry a score. Pure arithmetic over this cycle's set -- no RNG draw, no state --
    // and computed whether or not it is fed, so the term is observable before it decides
    // anything. Without --nlosClassifier every nlosScore is -1, nScored is 0 and nothing
    // below differs from before.
    {
        double sum = 0.0;
        uint32_t nScored = 0;
        for (size_t i = 0; i < meas.size(); ++i)
        {
            if (meas[i].nlosScore >= 0.0)
            {
                sum += 1.0 - std::min(1.0, meas[i].nlosScore);
                ++nScored;
            }
        }
        d.nNlosScored = nScored;
        d.nlosScoreTerm = (nScored > 0) ? sum / static_cast<double>(nScored) : -1.0;
        d.nlosScoreFed = m_config.pcsNlosFromScore && nScored > 0 &&
                         m_config.pcsForm != CA_TBL_PCS_V2;
    }

    // sigma_geo: this cycle's measurement-only radial position sigma, the exact
    // sqrt(trace((J^T R^-1 J)^-1)). Needed only by CA_TBL_PCS_V2, so it is computed
    // only for that form -- the legacy path must not gain a solve it never had, or
    // baseline reproduction becomes an argument rather than a diff.
    //
    // This is pure arithmetic over varM2 and the anchor geometry: NO RNG DRAW, so
    // computing it cannot perturb the channel realisation.
    //
    // Non-positive means the weighted solve failed (collinear or degenerate
    // geometry). ComputePcs treats that as maximally unconfident rather than
    // substituting an optimistic default.
    double sigmaGeoM = -1.0;
    if (m_config.pcsForm == CA_TBL_PCS_V2)
    {
        double gzx = 0.0;
        double gzy = 0.0;
        double grxx = 0.0;
        double grxy = 0.0;
        double gryy = 0.0;
        if (RawMultilaterationWeighted(meas, varM2, m_ekf.PriorX(), m_ekf.PriorY(),
                                       gzx, gzy, grxx, grxy, gryy))
        {
            const double tr = grxx + gryy;
            if (tr > 0.0 && std::isfinite(tr))
            {
                sigmaGeoM = std::sqrt(tr);
            }
        }
    }

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
    // Hoisted out of the MODE_FULL block: the recovery paths below need the raw fix
    // and its covariance, which were previously scoped to the test that produced
    // them. Declaration move only -- no behavioural change, asserted by the
    // byte-identical null control.
    double zx = 0.0;
    double zy = 0.0;
    double fixRxx = 0.0;
    double fixRxy = 0.0;
    double fixRyy = 0.0;
    bool haveRawFix = false;
    if (m_config.mode == CA_TBL_MODE_FULL)
    {
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
                fixRxx = rxx;
                fixRxy = rxy;
                fixRyy = ryy;
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

        haveRawFix = haveFix;
        if (haveFix)
        {
            d.mahalanobis = mahal;
            // "Every link tagged LOS": the Python reference's condition, and the default.
            bool allLos = (d.nNlos == 0);
            if (m_config.integrityNlosFromScore)
            {
                // DC-8, the spec's condition: "NLOS_score is LOW" (Physical 2.6), read as
                // the mean score over the links that carry one. With none scored there is
                // nothing to read, and the tag decides as before.
                double sum = 0.0;
                uint32_t nScored = 0;
                for (size_t i = 0; i < meas.size(); ++i)
                {
                    if (meas[i].nlosScore >= 0.0)
                    {
                        sum += std::min(1.0, meas[i].nlosScore);
                        ++nScored;
                    }
                }
                if (nScored > 0)
                {
                    d.integrityBasis = 1;
                    d.integrityMeanScore = sum / static_cast<double>(nScored);
                    allLos = d.integrityMeanScore < m_config.integrityNlosScoreThreshold;
                    if (allLos)
                    {
                        ++m_integrityScoreEligible;
                    }
                    else
                    {
                        ++m_integrityScoreIneligible;
                    }
                }
                else
                {
                    d.integrityBasis = 2;
                    ++m_integrityTagFallback;
                }
            }
            d.integrityEligible = allLos;
            if (mahal > m_config.mahalanobisThreshold && allLos)
            {
                suspect = true;
                m_suspects++;
            }
        }
    }
    d.suspect = suspect;

    const bool downweight =
        suspect && m_config.suspectRecovery == CA_TBL_RECOVERY_DOWNWEIGHT;
    if (downweight)
    {
        // DC-7, the spec's response (Physical 2.6): accepted, flagged, fused at inflated
        // R, and its PCS downweighted below. Not a rejection, so the rejection run ends.
        std::vector<double> inflated(varM2);
        for (double& v : inflated)
        {
            v *= m_config.suspectInflateFactor;
        }
        m_ekf.Update(meas, inflated);
        ++d.rangeUpdates;
        if (auxQueued)
        {
            FuseAuxInflated(m_config.suspectInflateFactor);
        }
        m_consecutiveRejections = 0;
        m_downweightedFixes++;
    }
    else if (suspect)
    {
        m_consecutiveRejections++;
        // RECOVERY POLICY. NONE is the pre-existing behaviour and the default.
        //
        // Rejecting is only half a response. RevertToPrior() runs before any Update()
        // of this cycle, so it is a no-op and a rejected cycle is a plain coast (this
        // comment said until 2026-09-30 that it froze P; see the correction on
        // CaTblSuspectRecovery). Because the gate's reference IS the prior, a
        // wrong prior makes every honest measurement look anomalous, and the gate
        // holds itself shut -- measured at 22 consecutive cycles on a run with NO
        // fault injected, and 108 / 251 on F1 / F3.
        bool recovered = false;
        if (m_config.suspectRecovery == CA_TBL_RECOVERY_BOUNDED &&
            m_consecutiveRejections >= m_config.suspectRecoveryN)
        {
            // Force-accept. Caps the lock-out at N by construction. The cost, stated:
            // during a genuine persistent fault this fuses the very measurement the
            // gate identified as faulty.
            m_ekf.Update(meas, varM2);
            ++d.rangeUpdates;
            recovered = true;
            if (auxQueued)
            {
                FuseAux(); // force-accepted cycle: the TDoA half of the fix goes in too
            }
        }
        else if (m_config.suspectRecovery == CA_TBL_RECOVERY_REINIT &&
                 m_consecutiveRejections >= m_config.suspectRecoveryN && haveRawFix)
        {
            // Discard a prior the evidence says is wrong and restart from the raw
            // filter-free fix, which is already computed for the test itself. Unlike
            // BOUNDED this replaces the prior instead of forcing data through it, and
            // it recovers the cycle after a transient fault clears.
            m_ekf.RevertToPrior();
            double pv = 100.0;
            if ((fixRxx + fixRyy) > 0.0)
            {
                pv = m_config.suspectInflateFactor * (fixRxx + fixRyy);
            }
            else
            {
                m_reinitFallbackCount++;
            }
            m_ekf.Init(zx, zy, m_ekf.Vx(), m_ekf.Vy(), pv, 100.0);
            recovered = true;
        }
        else
        {
            m_ekf.RevertToPrior();
            if (m_config.suspectRecovery == CA_TBL_RECOVERY_INFLATE)
            {
                // Rejecting is the filter asserting model and data disagree; the
                // honest consequence is more uncertainty, not the same uncertainty.
                m_ekf.InflatePos(m_config.suspectInflateFactor);
                recovered = true;
            }
        }
        if (recovered)
        {
            m_recoveryFirings++;
            m_consecutiveRejections = 0;
        }
        d.measurementRejected = true;
    }
    else
    {
        m_consecutiveRejections = 0;
        if (caseD)
        {
            // DC-4. One range against a wide P can leave an inconsistent covariance, so it
            // goes in only if its innovation is plausible: NIS = nu^2 / (H P H^T + R) with
            // H the unit vector from the anchor, against chi^2(1 dof, 0.99) = 6.63.
            const double dx = m_ekf.X() - meas[0].anchorX;
            const double dy = m_ekf.Y() - meas[0].anchorY;
            const double r = std::max(std::sqrt(dx * dx + dy * dy), 1e-6);
            const double hx = dx / r, hy = dy / r;
            const double s = hx * hx * m_ekf.Pxx() + 2.0 * hx * hy * m_ekf.Pxy() +
                             hy * hy * m_ekf.Pyy() + varM2[0];
            const double nu = meas[0].rangeM - r;
            d.caseDNis = (s > 0.0) ? nu * nu / s : -1.0;
            if (s > 0.0 && d.caseDNis <= 6.63)
            {
                m_ekf.Update(meas, varM2);
                ++d.rangeUpdates;
                d.caseDApplied = true;
            }
            else
            {
                m_caseDRefused++;
            }
        }
        else
        {
            m_ekf.Update(meas, varM2);
            ++d.rangeUpdates;
        }
        if (auxQueued)
        {
            FuseAux(); // the COMBINED tier-1 fix: RSSI ranges above, TDoA position here
        }
    }

    const double traceSigma = m_ekf.TracePos();
    double sumVar = 0.0;
    for (size_t i = 0; i < varM2.size(); ++i)
    {
        sumVar += varM2[i];
    }
    d.meanSigmaM = varM2.empty() ? 0.0 : std::sqrt(sumVar / static_cast<double>(varM2.size()));
    d.meanSigmaRawM =
        varM2.empty() ? 0.0 : std::sqrt(sumVarRaw / static_cast<double>(varM2.size()));

    const double gdop = ComputeGdop(m_ekf.X(), m_ekf.Y(), meas);
    double pcs = ComputePcs(traceSigma,
                            gdop,
                            nLos,
                            static_cast<uint32_t>(meas.size()),
                            hopCount,
                            sigmaGeoM,
                            d.nlosScoreFed ? d.nlosScoreTerm : -1.0);
    if (downweight)
    {
        // "Downweight in PCS regardless of raw trace(Sigma)" (Physical 2.6). Form-aware,
        // as ApplyProvenance should be (sect 1.H item 3): v2 scores can be negative, and
        // scaling a negative score by a factor < 1 would RAISE it.
        pcs = (m_config.pcsForm == CA_TBL_PCS_V2)
                  ? pcs - std::log10(1.0 / m_config.suspectPcsFactor)
                  : pcs * m_config.suspectPcsFactor;
    }

    d.traceSigma = traceSigma;
    d.gdop = gdop;
    d.pcs = pcs;
    d.sigmaGeoM = sigmaGeoM;
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
        if (suspect && m_config.suspectForcesEscalation)
        {
            // A distrusted fix is itself grounds to escalate: the coarse tier
            // has stopped being believable, which is precisely when the
            // precision tier earns its energy.
            //
            // Gated because this side effect otherwise dominates the gate's
            // accuracy effect and makes the integrity check unmeasurable as an
            // integrity check -- see CaTblPolicyConfig::suspectForcesEscalation.
            // With the flag off, a suspect fix is rejected and nothing else
            // happens, which is the mechanism report §2.3 and §10.5 claim.
            wantEscalate = true;
        }
    }

    d.wantedEscalation = wantEscalate; // DC-4: before any precondition suppresses it
    // ------------------------------------------ availability precondition (B.2+) --
    // Escalate only into a tier there are grounds to believe can deliver. Applied
    // AFTER wantEscalate is formed, so it can only ever SUPPRESS an escalation the
    // confidence model already wanted -- it never creates one. That ordering is what
    // keeps this a precondition on the existing trigger rather than a second trigger.
    //
    // TWO SEPARATE FLAGS, because the two steps have genuinely different evidence
    // situations and one of them does not work.
    //
    // escalationRequiresEvidenceT2 (868 -> 2.4), REBUILT 2026-09-08.
    //
    // The original asked the MAC neighbour cache whether tier 2 was audible. It is
    // not observable that way: the tier-2 radio is asleep while another tier is
    // active, so GetNeighbourCount returns 0, the evidence bit never sets, and the
    // node latched at the floor for the whole run -- measured, activeTier occupancy
    // {868MHz: 1440} at both boxHalf 1000 and 2000, escalations 0.
    //
    // This asks a question that CAN be answered from tier 1, and it is the same
    // question uwbEvidenceFromNeighbours asks one tier up: the ranges in `meas` were
    // taken by an AWAKE radio this cycle, so a neighbour ranged closer than the
    // tier-2 radius is a neighbour 2.4 GHz would reach. Geometry, not a map, so it
    // follows finders -- which a static anchor map cannot do and which is the whole
    // point under the tri-band topology.
    //
    // Three anchors, not presence. Tier 3 is a contact indicator and one is enough to
    // be worth confirming; tier 2 is a localisation tier and one reachable anchor
    // cannot solve, so escalating on it spends the handshake for nothing.
    //
    // Suppress-only, like every other evidence predicate here: it can refuse an
    // escalation the score already wanted and can never create one.
    if (m_config.escalationRequiresEvidenceT2 && wantEscalate && dynamic &&
        m_tier == CA_TBL_TIER_868)
    {
        int32_t nearT2 = 0;
        for (size_t i = 0; i < meas.size(); ++i)
        {
            if (meas[i].rangeM > 0.0 && meas[i].rangeM <= m_config.t2PredictRadiusM)
            {
                ++nearT2;
            }
        }
        d.predictedVis2p4 = nearT2;
        if (nearT2 < static_cast<int32_t>(m_config.t2PredictMinAnchors))
        {
            wantEscalate = false;
            m_evidenceBlockedT2++;
        }
    }

    // ---- SWITCHING-POINT INTEGRITY: is the confidence signal itself trustworthy? --
    //
    // Ordered BEFORE the coverage prediction deliberately. Coverage asks "is there
    // anything up there to talk to"; this asks "is the estimate that wants to escalate
    // worth believing at all". Predicting coverage from an estimate that has already
    // failed its own consistency check is work done on a number nobody should trust.
    if (wantEscalate && dynamic && m_crossCheckFresh)
    {
        if (m_config.imuCrossCheck && m_imuTolM > 0.0)
        {
            if (m_config.witnessCounts)
            {
                ++m_witnessEvals; // DC-9
                d.witnessEvaluated = true;
            }
            // The inertial estimate is an independent ABSOLUTE position that drifts.
            // The caller supplies both the disagreement and the tolerance, because the
            // drift model belongs with the engine that produces it -- an SHS is bounded
            // by time since its last correction, not by distance walked, and a policy
            // that assumed otherwise would be applying a strapdown error model to a
            // step-and-heading one.
            if (m_imuDispM > m_imuTolM)
            {
                wantEscalate = false;
                m_imuRejections++;
                d.witnessRefused = m_config.witnessCounts; // DC-9
            }
        }

        // Independent of the filter, which is the entire reason it is here: the
        // evidence comes from what OTHER nodes say about where they are.
        if (wantEscalate && m_config.corroborationCheck &&
            m_corrTotal >= static_cast<int32_t>(m_config.corroborationMinWitnesses))
        {
            const double frac =
                static_cast<double>(m_corrOk) / static_cast<double>(m_corrTotal);
            if (frac < m_config.corroborationFloor)
            {
                wantEscalate = false;
                m_corrRejections++;
            }
        }
    }

    if (m_config.escalationRequiresEvidence && wantEscalate && dynamic)
    {
        const bool haveMap = !m_config.uwbAnchorX.empty();
        if (m_tier == CA_TBL_TIER_2P4 &&
            (m_config.uwbEvidenceFromNeighbours || haveMap))
        {
            // Two stages, and the order matters: a coverage judgement computed from
            // a bad estimate is worse than none, so the estimate-quality gate runs
            // first. It runs in BOTH forms -- the observed form still escalates on
            // the strength of an estimate, and an estimate too poor to trust is a
            // reason to stay regardless of what is audible.
            const double sigPos = std::sqrt(std::max(traceSigma, 0.0));
            if (sigPos > m_config.uwbPredictMaxSigmaFrac * m_config.uwbPredictRadiusM)
            {
                wantEscalate = false;
                m_predictBlockedQuality++;
            }
            else if (m_config.uwbEvidenceFromNeighbours)
            {
                // PREDICTED FROM OBSERVATION, on a radio that is actually awake.
                //
                // The first version of this read tierEvidenceMask's UWB bit, and it
                // could never fire. A tier the node is not using is put in deep
                // sleep and its neighbour table is CLEARED every cycle, deliberately
                // -- keeping it would hand the policy free knowledge of a radio it
                // had switched off and make re-escalation instantaneous. So the UWB
                // table is only ever non-empty WHILE ALREADY ON UWB, and a gate that
                // demands UWB evidence before entering UWB is a permanent veto on
                // tier 3. Measured: uwbCycles 0 and meanNbr_uwb 0.0000 even in a
                // 300 m box with four converging finders over 1200 s.
                //
                // The comment this replaced was therefore RIGHT that there is
                // nothing to observe on tier 3 before escalating to it, and the
                // reason it gave -- discoverability -- was not the operative one.
                // The operative one is that the radio is asleep.
                //
                // What IS observable is range on the tier currently in use. The
                // measurements handed to this Step() were taken by an awake radio,
                // and a neighbour ranged at less than the UWB predicate radius is a
                // neighbour UWB would reach. That is an observation about geometry
                // rather than about a map, so it follows finders, which is what the
                // static map cannot do.
                //
                // Presence, not count, and counted under the SAME counter as the
                // predicted form: it is the same refusal for the same reason.
                int32_t near = 0;
                for (size_t i = 0; i < meas.size(); ++i)
                {
                    if (meas[i].rangeM > 0.0 &&
                        meas[i].rangeM <= m_config.uwbPredictRadiusM)
                    {
                        ++near;
                    }
                }
                d.predictedVisUwb = near;
                if (near < 1)
                {
                    wantEscalate = false;
                    m_predictBlockedCoverage++;
                }
            }
            else
            {
                // PREDICTED, the pre-existing form. Retained unchanged: it is what
                // every published result was measured under, and the legacy
                // topology it was written for is still reachable via --topology=1.
                const double r2 = m_config.uwbPredictRadiusM * m_config.uwbPredictRadiusM;
                uint32_t vis = 0;
                for (size_t i = 0; i < m_config.uwbAnchorX.size(); ++i)
                {
                    const double dx = m_config.uwbAnchorX[i] - m_ekf.X();
                    const double dy = m_config.uwbAnchorY[i] - m_ekf.Y();
                    // 2-D. Anchors sit at z = 2.5 and the node at z = 1.5, so the
                    // slant range exceeds this by under 0.03 m at a 20 m radius.
                    if (dx * dx + dy * dy <= r2)
                    {
                        ++vis;
                    }
                }
                d.predictedVisUwb = static_cast<int32_t>(vis);
                if (vis < 3)
                {
                    wantEscalate = false;
                    m_predictBlockedCoverage++;
                }
            }
        }
    }

    // DC-9: a witness refusal the evidence gate would have made anyway is ABSORBED.
    if (d.witnessRefused && UwbEvidenceWouldBlock(traceSigma, meas))
    {
        ++m_witnessAbsorbed;
        d.witnessAbsorbed = true;
    }
    // DC-4: a Case D cycle has a live covariance and is not a blind cycle, so it takes the
    // normal escalation decision below and does not count toward demotion.
    if (dynamic && !activeDelivered && !caseD)
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
                // Applied on the BLIND path only. The healthy-demotion branch below
                // steps down because things are going well, and stepping one tier at
                // a time is the correct energy taper there; skipping tiers would give
                // up precision the node is currently earning. Here the active tier
                // has nothing, so any intermediate tier that also has nothing is pure
                // blind cost on the way to the floor.
                int target = static_cast<int>(m_tier) - 1;
                if (m_config.multiStepDemote)
                {
                    target = static_cast<int>(CA_TBL_TIER_868);
                    for (int k = static_cast<int>(m_tier) - 1;
                         k > static_cast<int>(CA_TBL_TIER_868);
                         --k)
                    {
                        if (tierEvidenceMask & (1u << k))
                        {
                            target = k;
                            break;
                        }
                    }
                }
                m_tier = static_cast<CaTblTier>(target);
                m_deescalations++;
                d.deescalated = true;
            }
            m_deescalateCounter = 0;
        }
    }
    else if (dynamic && wantEscalate)
    {
        if (m_config.grantGatedUwb && m_tier == CA_TBL_TIER_2P4)
        {
            // NW-7: into UWB only on a confirmed grant. The first cycle that wants it asks
            // (escalated, so the caller sends the request); while pending, nothing more.
            if (!m_uwbPending)
            {
                m_uwbPending = true;
                ++m_uwbRequests;
                d.escalated = true;
                d.uwbPending = true;
            }
            m_deescalateCounter = 0;
        }
        else if (m_tier < CA_TBL_TIER_UWB)
        {
            m_tier = static_cast<CaTblTier>(static_cast<int>(m_tier) + 1);
            m_escalations++;
            d.escalated = true;
            m_deescalateCounter = 0;
        }
    }
    else if (dynamic && m_uwbPending)
    {
        // NW-7: frozen while a UWB request is pending -- no healthy demotion, which could
        // otherwise let the grant land on a node that has stepped down to 868.
        m_deescalateCounter = 0;
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
    // Remember where the filter was, so the next cycle can ask whether it moved the
    // way the body did. Recorded on EVERY cycle including refused ones: the comparison
    // is against the previous cycle, not the last accepted escalation.
    m_prevEstX = m_ekf.X();
    m_prevEstY = m_ekf.Y();
    m_havePrevEst = true;
    // Consumed. A stale cross-check is worse than none -- it would silently police one
    // cycle with another cycle evidence -- so the caller must supply it every cycle it
    // wants the check to run.
    m_crossCheckFresh = false;

    return d;
}

} // namespace ns3
