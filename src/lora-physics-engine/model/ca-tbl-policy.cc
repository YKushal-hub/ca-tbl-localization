#include "ca-tbl-policy.h"

#include <iostream>
#include <limits>

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
    case CA_TBL_MODE_FUSE_ALL:
        return "poucet_fusion";
    case CA_TBL_MODE_FUSE_LORA:
        return "lora_fusion";
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
    measVarScale = 1.0; // S2-C; 1.0 multiplies exactly, so the default changes nothing
    nlosRSource = 0;    // S2-C; 0 keys R and the LOS count off the operative tag, as before
    acquireMinAnchors = 0; // S2-C; 0: acquired from the start, as before
    acquireReinit = false; // R1 (N.54); off: the first fix is a range update from the prior, as before
    hold24AfterK = 0; // C3; off: UWB escalation as before
    tofRecal = false; // C2; off: 868 MHz ranges used as measured
    acquireNegInfo = false; // C1; off: non-detections ignored
    relayNisInflate = false; // B1; off: a relay's P_B is used as it stands
    rssiCorrInflate = false; // B2; off: successive 868 MHz RSSI ranges are fused as independent, as before
    rssiCorrDistM = 30.0;    // ShadowingParams::DefaultsFor(ENV_RURAL_FORESTED).correlationDistanceM
    rssiCorrMaxFactor = 100.0;
    negR50M[0] = 3600.0; // 868 MHz: probe r80 ~1700 m with sigma 9 dB, n 2.3 (V1/V2 probes), r50 = r80 10^(0.84 sigma / 10 n)
    negR50M[1] = 328.0;  // 2.4 GHz: fitted to the B3F clamped probe (b4_fit_ondemand.py)
    negR50M[2] = 15.0;   // UWB: probe r80 ~7 m (B3F/V1), unused for gateways
    negSigmaDb = 9.1;
    c2bEscalation = false; // V5; off: escalation triggered by PCS / the trace threshold, as before
    acquireParticles = false; // V4; off: the EKF acquires from its prior, as before
    acquireParticleCount = 2000;
    acquireSeed = 0;
    acquireSpeedMps = 1.5;
    rangeBiasStates = false; // V3; off: every range error treated as independent, as before
    rangeBiasCorrM = 30.0;
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
    m_cons.clear();
    m_consPrior.clear();
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
    if (m_consOn)
    {
        // V3: cross-covariances follow x <- F x; the biases decorrelate over the estimated motion.
        for (auto& kv : m_cons)
        {
            kv.second.pxc[0] += dt * kv.second.pxc[2];
            kv.second.pxc[1] += dt * kv.second.pxc[3];
        }
        ConsiderDecay(std::hypot(m_x[2], m_x[3]) * dt);
        ConsiderSnapshot();
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
    if (m_consOn)
    {
        // V3: position += u is independent of the state; velocity := u/dt carries no state either.
        for (auto& kv : m_cons)
        {
            kv.second.pxc[2] = 0.0;
            kv.second.pxc[3] = 0.0;
        }
        ConsiderDecay(std::hypot(ux, uy));
        ConsiderSnapshot();
    }
}

void
CaTblEkf::Update(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    const size_t n = std::min(meas.size(), varM2.size());
    for (size_t k = 0; k < n; ++k)
    {
        if (m_consOn && meas[k].tier == 0)
        {
            // V3: an 868 MHz range with its consider bias, out of line; every other range takes the
            // original arithmetic below, so arms without 868 MHz ranges stay byte-identical.
            ConsiderUpdate(std::vector<CaTblRangeMeasurement>(1, meas[k]), std::vector<double>(1, varM2[k]));
            continue;
        }
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
        if (m_trackNis)
        {
            NoteNis(innovation * innovation / S); // B1
        }
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
        if (m_consOn && !m_cons.empty())
        {
            ConsiderAfterRangeGain(K, H); // V3: the state moved, so every cross-covariance moves too
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
        if (m_consOn)
        {
            double K2[4][2];
            for (int i = 0; i < 4; ++i)
            {
                K2[i][0] = K[i];
                K2[i][1] = 0.0;
            }
            // One axis: shift the selected component into slot 0 for the shared helper.
            for (auto& kv : m_cons)
            {
                const double h = kv.second.pxc[axis];
                for (int i = 0; i < 4; ++i)
                {
                    kv.second.pxc[i] -= K2[i][0] * h;
                }
            }
        }
    }
}

// Out of line on purpose: Step() gains a call and a branch, not an inlined 2x2 update
// sitting among the expressions the byte-identical null control holds fixed.
bool
CaTblPolicy::AuxPassesGate(double rScale)
{
    if (!m_config.auxInnovationGate)
    {
        return true;
    }
    const double nis =
        FltPositionNis(m_aux[0], m_aux[1], rScale * m_aux[2], rScale * m_aux[3], rScale * m_aux[4]);
    if (nis >= 0.0 && nis <= m_config.auxInnovationChi2)
    {
        return true;
    }
    ++m_auxGated;
    return false;
}

__attribute__((noinline)) void
CaTblPolicy::FuseAux()
{
    if (!AuxPassesGate(1.0))
    {
        return;
    }
    if (FltFusePositionCov(m_aux[0], m_aux[1], m_aux[2], m_aux[3], m_aux[4]))
    {
        m_auxFusedLast = true;
        ++m_auxFused;
    }
}

void
CaTblPolicy::FuseAuxInflated(double rScale)
{
    if (!AuxPassesGate(rScale))
    {
        return;
    }
    if (FltFusePositionCov(m_aux[0],
                              m_aux[1],
                              rScale * m_aux[2],
                              rScale * m_aux[3],
                              rScale * m_aux[4]))
    {
        m_auxFusedLast = true;
        ++m_auxFused;
    }
}

double
CaTblEkf::PositionNis(double zx, double zy, double rxx, double rxy, double ryy) const
{
    if (!std::isfinite(zx) || !std::isfinite(zy) || !std::isfinite(rxx) ||
        !std::isfinite(rxy) || !std::isfinite(ryy) || !(rxx > 0.0) || !(ryy > 0.0) ||
        !(rxx * ryy - rxy * rxy > 0.0))
    {
        return -1.0;
    }
    const double s00 = m_P[0][0] + rxx;
    const double s01 = 0.5 * (m_P[0][1] + m_P[1][0]) + rxy;
    const double s11 = m_P[1][1] + ryy;
    const double det = s00 * s11 - s01 * s01;
    if (!(det > 0.0) || !std::isfinite(det))
    {
        return -1.0;
    }
    const double v0 = zx - m_x[0];
    const double v1 = zy - m_x[1];
    return (s11 * v0 * v0 - 2.0 * s01 * v0 * v1 + s00 * v1 * v1) / det;
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
    if (m_consOn)
    {
        ConsiderAfterPositionGain(K, 2);
    }
    return true;
}

void
CaTblEkf::SetPositionCov(double x, double y, double pxx, double pxy, double pyy, double velVar)
{
    m_x[0] = x;
    m_x[1] = y;
    m_x[2] = 0.0;
    m_x[3] = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            m_P[i][j] = 0.0;
        }
    }
    m_P[0][0] = std::max(pxx, 1e-6);
    m_P[1][1] = std::max(pyy, 1e-6);
    m_P[0][1] = m_P[1][0] = pxy;
    m_P[2][2] = m_P[3][3] = velVar;
    for (int i = 0; i < 4; ++i)
    {
        m_xPrior[i] = m_x[i];
        for (int j = 0; j < 4; ++j)
        {
            m_PPrior[i][j] = m_P[i][j];
        }
    }
    m_cons.clear();
    m_consPrior.clear();
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
    if (m_consOn)
    {
        m_cons = m_consPrior;
    }
}

// ---- V3 --rangeBiasStates: Schmidt-Kalman consider biases (TECH_DEBT N.51) ----------------------

__attribute__((noinline)) void
CaTblEkf::ConsiderDecay(double ds)
{
    const double rho = std::exp(-std::max(0.0, ds) / std::max(1e-6, m_consCorrM));
    for (auto& kv : m_cons)
    {
        ConsiderBias& c = kv.second;
        const double target = (1.0 - m_consWhite) * c.lastVar;
        c.pcc = rho * rho * c.pcc + (1.0 - rho * rho) * target;
        for (int i = 0; i < 4; ++i)
        {
            c.pxc[i] *= rho;
        }
    }
}

__attribute__((noinline)) void
CaTblEkf::ConsiderAfterPositionGain(const double K[4][2], int nAxes)
{
    // A position observation H = [I 0]: every cross-covariance loses K (H P_xc).
    for (auto& kv : m_cons)
    {
        double* p = kv.second.pxc;
        const double h0 = p[0];
        const double h1 = p[1];
        for (int i = 0; i < 4; ++i)
        {
            p[i] -= K[i][0] * h0 + (nAxes > 1 ? K[i][1] * h1 : 0.0);
        }
    }
}

__attribute__((noinline)) void
CaTblEkf::ConsiderAfterRangeGain(const double K[4], const double H[4])
{
    for (auto& kv : m_cons)
    {
        double* p = kv.second.pxc;
        const double h = H[0] * p[0] + H[1] * p[1];
        for (int i = 0; i < 4; ++i)
        {
            p[i] -= K[i] * h;
        }
    }
}

__attribute__((noinline)) void
CaTblEkf::ConsiderUpdate(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    const size_t n = std::min(meas.size(), varM2.size());
    for (size_t k = 0; k < n; ++k)
    {
        const double dx = m_x[0] - meas[k].anchorX;
        const double dy = m_x[1] - meas[k].anchorY;
        const double rPred = std::sqrt(dx * dx + dy * dy);
        const double rSafe = std::max(rPred, 1e-6);
        const double H[4] = {dx / rSafe, dy / rSafe, 0.0, 0.0};
        double PH[4];
        for (int i = 0; i < 4; ++i)
        {
            PH[i] = m_P[i][0] * H[0] + m_P[i][1] * H[1];
        }
        const double var = varM2[k];
        if (!(var > 0.0) || !std::isfinite(var))
        {
            continue;
        }

        ConsiderBias* cb = nullptr;
        double R = var;
        if (meas[k].tier == 0) // CA_TBL_TIER_868: correlated RSSI error
        {
            const uint32_t key = (static_cast<uint32_t>(meas[k].tier) << 16) | meas[k].anchorId;
            auto it = m_cons.find(key);
            if (it == m_cons.end())
            {
                ConsiderBias c;
                c.pcc = (1.0 - m_consWhite) * var;
                c.lastVar = var;
                c.ax = meas[k].anchorX;
                c.ay = meas[k].anchorY;
                it = m_cons.emplace(key, c).first;
            }
            cb = &it->second;
            // The anchor's own motion decorrelates the shadowing too.
            const double da = std::hypot(meas[k].anchorX - cb->ax, meas[k].anchorY - cb->ay);
            if (da > 0.0)
            {
                const double rho = std::exp(-da / std::max(1e-6, m_consCorrM));
                cb->pcc = rho * rho * cb->pcc + (1.0 - rho * rho) * (1.0 - m_consWhite) * cb->lastVar;
                for (int i = 0; i < 4; ++i)
                {
                    cb->pxc[i] *= rho;
                }
            }
            cb->ax = meas[k].anchorX;
            cb->ay = meas[k].anchorY;
            // RSSI error is proportional to range: rescale the bias to this reading's variance.
            if (cb->lastVar > 0.0 && var != cb->lastVar)
            {
                const double s = std::sqrt(var / cb->lastVar);
                cb->pcc *= s * s;
                for (int i = 0; i < 4; ++i)
                {
                    cb->pxc[i] *= s;
                }
            }
            cb->lastVar = var;
            R = m_consWhite * var;
        }

        // g = cov(x, z) = P H' + P_xc (the bias enters z with coefficient 1).
        double g[4];
        double hpc = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            g[i] = PH[i] + (cb ? cb->pxc[i] : 0.0);
        }
        if (cb)
        {
            hpc = H[0] * cb->pxc[0] + H[1] * cb->pxc[1];
        }
        const double S = H[0] * PH[0] + H[1] * PH[1] + 2.0 * hpc + (cb ? cb->pcc : 0.0) + R;
        if (!(S > 0.0) || !std::isfinite(S))
        {
            continue;
        }
        double K[4];
        for (int i = 0; i < 4; ++i)
        {
            K[i] = g[i] / S;
        }
        const double innovation = meas[k].rangeM - rPred;
        for (int i = 0; i < 4; ++i)
        {
            m_x[i] += K[i] * innovation;
        }
        // P <- P - K g'  (= P - g g' / S)
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                m_P[i][j] -= K[i] * g[j];
            }
        }
        for (int i = 0; i < 4; ++i)
        {
            for (int j = i + 1; j < 4; ++j)
            {
                const double avg = 0.5 * (m_P[i][j] + m_P[j][i]);
                m_P[i][j] = avg;
                m_P[j][i] = avg;
            }
        }
        // Every cross-covariance: P_xc,j <- P_xc,j - K (H P_xc,j + [j == this bias] pcc). The bias
        // itself and its variance are not updated (consider, not estimate).
        for (auto& kv : m_cons)
        {
            ConsiderBias& c = kv.second;
            const double hj = H[0] * c.pxc[0] + H[1] * c.pxc[1] + (&c == cb ? c.pcc : 0.0);
            for (int i = 0; i < 4; ++i)
            {
                c.pxc[i] -= K[i] * hj;
            }
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
CaTblPolicy::Ekf5Predict(double dt, bool haveImu, double ux, double uy)
{
    // --imuFilter=ekf5. DC-1's u is what a step-and-heading dead reckoner reports: the stride
    // scaled by (1 + s) and the heading rotated by the gyro's accumulated error. So |u| is the
    // stride the filter's s corrects, atan2(u) the raw heading its theta and b correct. Process
    // noise is DC-1's own model: angle random walk on theta; the bias and scale are constant per
    // run, so they get none; a small per-step stride floor (OURS) keeps the along-track block
    // from collapsing where DC-1 has no stride noise at all.
    const double qTheta2 = m_config.ekf5NoiseDensity * m_config.ekf5NoiseDensity * dt;
    const double dist = haveImu ? std::sqrt(ux * ux + uy * uy) : 0.0;
    if (dist > 0.0)
    {
        m_ekf5Heading = std::atan2(uy, ux);
        m_ekf5HaveHeading = true;
        const double qStep2 = m_config.ekf5StepSigma * m_config.ekf5StepSigma;
        m_ekf5.Predict(dt, dist, m_ekf5Heading, qStep2, qTheta2, 0.0);
        ++m_ekf5Moving;
    }
    else
    {
        // No displacement (a dwell, or no IMU input this cycle): the heading error still
        // drifts with the bias, and the position gets DC-1's dwell noise.
        m_ekf5.PredictDwell(dt, qTheta2, 0.0, m_config.ekf5DwellSigma * m_config.ekf5DwellSigma);
        ++m_ekf5Dwell;
    }
}

void
CaTblPolicy::Configure(const CaTblPolicyConfig& config)
{
    m_config = config;
    m_ekf.SetProcessNoise(config.processNoiseStdPos, config.processNoiseStdVel);
    if (config.rangeBiasStates)
    {
        m_ekf.EnableConsider(config.rangeBiasCorrM, 0.05);
    }
    if (config.relayNisInflate)
    {
        m_ekf.EnableNisTracking();
        m_ekf5.EnableNisTracking(); // read only when ekf5 is the policy filter (GetNisMean)
    }

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
    if (m_config.imuFilterEkf5)
    {
        // The IMU's errors start at the run's beginning: heading offset ~0, bias and stride
        // scale at their priors -- the spec sheet's sigmas, which an honest filter knows.
        m_ekf5.Init(x0, y0, posStdM * posStdM, m_config.ekf5ThetaVar0,
                    m_config.ekf5BiasSigma * m_config.ekf5BiasSigma,
                    m_config.ekf5ScaleSigma * m_config.ekf5ScaleSigma);
        m_ekf5.SetMomentMatched(m_config.ekf5MomentMatch);
        m_ekf5.SetMomentMatchedMeanShift(false);
        m_ekf5.SetRobust(m_config.ekf5Robust);
        m_ekf5HaveHeading = false;
    }
    m_deescalateCounter = 0;
    // --acquireMinAnchors (S2-C). 0 (default): acquired from the start, as before.
    m_acquired = (m_config.acquireMinAnchors == 0);
    // --acquireReinit (R1, TECH_DEBT N.54): armed until the first cycle with three or more ranges.
    m_reinitPending = m_config.acquireReinit;
    if (m_config.acquireParticles)
    {
        // V4: the prior is the box the filter is told about (uniform, half-width sqrt(3) sigma).
        PfInit(x0, y0, std::sqrt(3.0) * posStdM);
    }
}

// ---- V4 --acquireParticles ----------------------------------------------------------------------------
//
// Range-only acquisition is multimodal: three km-scale RSSI ranges intersect in two or more places, and a
// single Gaussian started anywhere commits to one of them (R1: re-initialising at a multilateration root did
// not help). A particle filter keeps every root alive until the ranges, the node's motion and the anchors'
// motion disambiguate them (Blanco et al., IROS 2008, sum-of-Gaussians range-only initialisation; particles
// are the sample-based form of the same idea). Likelihoods are robust: 868 MHz ranges are log-normal in the
// ratio r/d (the RSSI inversion's error law), the others Gaussian; each term is floored at -8 (4 sigma), so
// a relayed or NLOS outlier cannot annihilate the true mode. Handover to the EKF when the cloud is unimodal:
// split along its principal axis, and it is one mode when either half carries < 2 % of the weight or
// Ashman's D between the halves is below 4 (a single Gaussian split at its mean gives D ~ 2.7).

void
CaTblPolicy::PfInit(double cx, double cy, double half)
{
    const uint32_t n = std::max<uint32_t>(100, m_config.acquireParticleCount);
    m_pfRng.seed(m_config.acquireSeed * 0x9E3779B97F4A7C15ULL + m_pfSalt * 0xBF58476D1CE4E5B9ULL + 0x7700001ULL);
    std::uniform_real_distribution<double> u(-1.0, 1.0);
    m_pf.assign(n, PfParticle{0.0, 0.0, 1.0 / n});
    for (auto& p : m_pf)
    {
        p.x = cx + half * u(m_pfRng);
        p.y = cy + half * u(m_pfRng);
    }
    m_pfActive = true;
    m_pfCycles = 0;
}

void
CaTblPolicy::PfPredict(double dt, double ux, double uy, double qxx, double qyy)
{
    std::normal_distribution<double> nz(0.0, 1.0);
    // Without a control the node may have walked anywhere within speed * dt; with one, the control's own
    // variance plus a small floor.
    const double sWalk = m_config.acquireSpeedMps * std::max(0.0, dt);
    const double sx = (qxx >= 0.0) ? std::sqrt(qxx + 1.0) : sWalk;
    const double sy = (qyy >= 0.0) ? std::sqrt(qyy + 1.0) : sWalk;
    for (auto& p : m_pf)
    {
        p.x += ux + sx * nz(m_pfRng);
        p.y += uy + sy * nz(m_pfRng);
    }
}

bool
CaTblPolicy::PfUpdate(const std::vector<CaTblRangeMeasurement>& meas, const std::vector<double>& varM2)
{
    ++m_pfCycles;
    const size_t n = m_pf.size();
    // Progressive correction (Oudjane & Musso 2000): a range far sharper than the cloud would hand all the
    // weight to whichever particles happen to sit nearest its peak, and with two mirror roots that is a coin
    // toss that kills one (measured: 30 of 40 two-anchor trials). Each range's likelihood variance is its own
    // plus the cloud's spread projected on the range direction, var + H C H', so the likelihood is never
    // sharper than the cloud; the cloud then contracts geometrically and both roots keep their particles.
    // As the cloud shrinks the term vanishes and the plain likelihood is recovered.
    double px, py, pcxx, pcxy, pcyy;
    PfStats(px, py, pcxx, pcxy, pcyy);
    std::vector<double> hch(meas.size(), 0.0);
    for (size_t i = 0; i < meas.size(); ++i)
    {
        const double dx = px - meas[i].anchorX, dy = py - meas[i].anchorY;
        const double dd = std::max(1e-6, std::hypot(dx, dy));
        const double hx = dx / dd, hy = dy / dd;
        hch[i] = hx * hx * pcxx + 2.0 * hx * hy * pcxy + hy * hy * pcyy;
    }
    std::vector<double> lw(n, 0.0);
    double lmax = -1e300;
    for (size_t k = 0; k < n; ++k)
    {
        double l = std::log(std::max(m_pf[k].w, 1e-300));
        for (size_t i = 0; i < meas.size() && i < varM2.size(); ++i)
        {
            const double d = std::max(1.0, std::hypot(m_pf[k].x - meas[i].anchorX, m_pf[k].y - meas[i].anchorY));
            const double r = std::max(1.0, meas[i].rangeM);
            const double sd = std::sqrt(std::max(varM2[i], 1e-6) + hch[i]);
            double term;
            if (meas[i].tier == 0)
            {
                const double s = std::min(2.0, std::max(0.05, sd / r));
                const double z = (std::log(r) - std::log(d)) / s;
                term = -0.5 * z * z;
            }
            else
            {
                const double z = (r - d) / sd;
                term = -0.5 * z * z;
            }
            l += std::max(term, -8.0);
        }
        // C1 --acquireNegInfo: an addressed gateway that stayed silent says "probably not within its reach":
        // weight by 1 - P(link | d), P = Phi(10 n log10(r50 / d) / sigma) with the engine-fitted curve (n = 2.3).
        if (m_config.acquireNegInfo)
        {
            for (const auto& sa : m_silent)
            {
                const double d = std::max(1.0, std::hypot(m_pf[k].x - sa.x, m_pf[k].y - sa.y));
                const double r50 = m_config.negR50M[std::min<int>(sa.tier, 2)];
                const double z = 10.0 * 2.3 * std::log10(r50 / d) / std::max(0.1, m_config.negSigmaDb);
                const double p = 0.5 * std::erfc(-z / std::sqrt(2.0));
                l += std::log(std::max(1e-3, 1.0 - std::min(p, 0.999)));
            }
        }
        lw[k] = l;
        lmax = std::max(lmax, l);
    }
    double sw = 0.0;
    for (size_t k = 0; k < n; ++k)
    {
        m_pf[k].w = std::exp(lw[k] - lmax);
        sw += m_pf[k].w;
    }
    double s2 = 0.0;
    for (auto& p : m_pf)
    {
        p.w /= sw;
        s2 += p.w * p.w;
    }
    const double ess = 1.0 / s2;
    if (ess < 0.5 * static_cast<double>(n))
    {
        // Systematic resampling, then roughening (Gordon, Salmond & Smith 1993): jitter of K E N^(-1/2)
        // per axis with K = 0.2 and E the cloud's extent, so resampled duplicates spread again.
        std::vector<PfParticle> out;
        out.reserve(n);
        std::uniform_real_distribution<double> u01(0.0, 1.0 / static_cast<double>(n));
        double c = m_pf[0].w;
        size_t j = 0;
        const double u0 = u01(m_pfRng);
        for (size_t k = 0; k < n; ++k)
        {
            const double uk = u0 + static_cast<double>(k) / static_cast<double>(n);
            while (uk > c && j + 1 < n)
            {
                ++j;
                c += m_pf[j].w;
            }
            out.push_back(PfParticle{m_pf[j].x, m_pf[j].y, 1.0 / n});
        }
        double x0 = 1e300, x1 = -1e300, y0 = 1e300, y1 = -1e300;
        for (const auto& p : out)
        {
            x0 = std::min(x0, p.x);
            x1 = std::max(x1, p.x);
            y0 = std::min(y0, p.y);
            y1 = std::max(y1, p.y);
        }
        std::normal_distribution<double> nz(0.0, 1.0);
        const double kj = 0.2 / std::sqrt(static_cast<double>(n));
        for (auto& p : out)
        {
            p.x += kj * (x1 - x0) * nz(m_pfRng);
            p.y += kj * (y1 - y0) * nz(m_pfRng);
        }
        m_pf.swap(out);
    }
    // Unimodality test along the principal axis.
    double mx, my, cxx, cxy, cyy;
    PfStats(mx, my, cxx, cxy, cyy);
    const double tr = cxx + cyy;
    const double l1 = 0.5 * tr + std::sqrt(std::max(0.0, 0.25 * (cxx - cyy) * (cxx - cyy) + cxy * cxy));
    double ex = 1.0, ey = 0.0;
    if (std::fabs(cxy) > 1e-12 || std::fabs(l1 - cxx) > 1e-12)
    {
        ex = cxy;
        ey = l1 - cxx;
        const double nn = std::hypot(ex, ey);
        if (nn > 0.0)
        {
            ex /= nn;
            ey /= nn;
        }
        else
        {
            ex = 1.0;
            ey = 0.0;
        }
    }
    double wA = 0, mA = 0, qA = 0, wB = 0, mB = 0, qB = 0;
    for (const auto& p : m_pf)
    {
        const double s = (p.x - mx) * ex + (p.y - my) * ey;
        if (s < 0.0)
        {
            wA += p.w;
            mA += p.w * s;
            qA += p.w * s * s;
        }
        else
        {
            wB += p.w;
            mB += p.w * s;
            qB += p.w * s * s;
        }
    }
    // Linearisation guard: the EKF may take over only once the cloud is small against the ranges it will
    // linearise (largest SD under a quarter of this cycle's median range); a km-wide cloud handed over is the
    // R1 failure again. Then unimodality: a half under 2 % of the weight, or Ashman's D < 3 (a Gaussian split
    // at its mean gives ~2.7, a uniform ~3.5, two separated modes far more).
    if (m_pfTestForceOne)
    {
        return true; // V4 fixture: hand over at once, no guard, no unimodality test
    }
    std::vector<double> rr;
    for (const auto& m : meas)
    {
        rr.push_back(m.rangeM);
    }
    std::nth_element(rr.begin(), rr.begin() + rr.size() / 2, rr.end());
    if (std::sqrt(std::max(0.0, l1)) > 0.25 * rr[rr.size() / 2])
    {
        return false;
    }
    if (wA < 0.02 || wB < 0.02)
    {
        return true;
    }
    mA /= wA;
    mB /= wB;
    const double vA = std::max(1e-9, qA / wA - mA * mA);
    const double vB = std::max(1e-9, qB / wB - mB * mB);
    const double D = std::sqrt(2.0) * std::fabs(mA - mB) / std::sqrt(vA + vB);
    return D < 3.0;
}

void
CaTblPolicy::PfStats(double& mx, double& my, double& cxx, double& cxy, double& cyy) const
{
    mx = my = cxx = cxy = cyy = 0.0;
    double sw = 0.0;
    for (const auto& p : m_pf)
    {
        mx += p.w * p.x;
        my += p.w * p.y;
        sw += p.w;
    }
    if (sw <= 0.0)
    {
        return;
    }
    mx /= sw;
    my /= sw;
    for (const auto& p : m_pf)
    {
        const double dx = p.x - mx, dy = p.y - my;
        cxx += p.w * dx * dx;
        cxy += p.w * dx * dy;
        cyy += p.w * dy * dy;
    }
    cxx /= sw;
    cxy /= sw;
    cyy /= sw;
    // The cloud cannot resolve below its own spacing: never report less (V4 tempering).
    cxx += m_pfFloor2;
    cyy += m_pfFloor2;
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

    // Term 1: filter uncertainty, mapped to (0,1] so it cannot dominate -- or, under
    // pcsTerm1Inverse (row 17), the spec's literal 1/trace(Sigma), unbounded above.
    const double term1 = m_config.pcsTerm1Inverse
                             ? 1.0 / std::max(traceSigma, 1e-12)
                             : m_config.traceRef / (m_config.traceRef + std::max(traceSigma, 0.0));
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
        const double dx = m_config.uwbAnchorX[i] - FltX();
        const double dy = m_config.uwbAnchorY[i] - FltY();
        if (dx * dx + dy * dy <= r2)
        {
            ++vis;
        }
    }
    return vis < 3;
}

void
CaTblPolicy::RequestQueryEscalation()
{
    m_queryEscArmed = true;
    ++m_queryEscRequests;
}

void
CaTblPolicy::QueryEscalateOneTier(CaTblDecision& d)
{
    if (m_config.grantGatedUwb && m_tier == CA_TBL_TIER_2P4)
    {
        if (!m_uwbPending)
        {
            m_uwbPending = true;
            ++m_uwbRequests;
            d.escalated = true;
            d.uwbPending = true;
        }
    }
    else if (m_tier < CA_TBL_TIER_UWB)
    {
        m_tier = static_cast<CaTblTier>(static_cast<int>(m_tier) + 1);
        m_escalations++;
        d.escalated = true;
    }
    m_deescalateCounter = 0;
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
CaTblPolicy::AcceptGrantOnBehalf()
{
    const bool hybrid = (m_config.mode == CA_TBL_MODE_FULL ||
                         m_config.mode == CA_TBL_MODE_NO_INTEGRITY ||
                         m_config.mode == CA_TBL_MODE_FIXED_THRESHOLD);
    if (m_uwbPending && ConfirmUwbGrant())
    {
        ++m_oboAccepted;
        return true;
    }
    if (hybrid && m_tier == CA_TBL_TIER_2P4)
    {
        m_tier = CA_TBL_TIER_UWB;
        m_escalations++;
        m_deescalateCounter = 0;
        ++m_oboAccepted;
        return true;
    }
    ++m_oboRefused;
    return false;
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
    if (!m_config.rssiCorrInflate)
    {
        return StepRecal(dt, meas, hopCount, activeTierRanges, activeTierNeighbours, tierEvidenceMask);
    }
    // B2 --rssiCorrInflate (scope 2026-10-17). An 868 MHz RSSI range's error is the link's shadowing, which is
    // spatially correlated: Gudmundson, rho = exp(-moved / d_corr), the engine's own model. A static or slow node
    // re-reads nearly the same shadowing every cycle, and fusing those readings as independent collapses Sigma
    // (static_868 median NEES ~240 at 5 km). For an AR(1) error sequence the variance of the mean grows by the
    // long-run factor (1 + rho) / (1 - rho); each reading's variance is scaled by it, with rho from the distance the
    // node's ESTIMATE and the anchor moved since that anchor's previous range (no truth used). Capped.
    std::vector<CaTblRangeMeasurement> adj(meas);
    const double ex = EstX();
    const double ey = EstY();
    for (auto& m : adj)
    {
        if (m.tier != 0 || !(m.rangeM > 0.0))
        {
            continue;
        }
        double k = 1.0;
        auto it = m_corrLast.find(m.anchorId);
        if (it != m_corrLast.end())
        {
            const double moved = std::hypot(ex - it->second.nx, ey - it->second.ny) +
                                 std::hypot(m.anchorX - it->second.ax, m.anchorY - it->second.ay);
            const double rho = std::exp(-moved / std::max(1e-6, m_config.rssiCorrDistM));
            k = (rho >= 1.0) ? m_config.rssiCorrMaxFactor
                             : std::min(m_config.rssiCorrMaxFactor, (1.0 + rho) / (1.0 - rho));
        }
        m_corrLast[m.anchorId] = CorrRef{ex, ey, m.anchorX, m.anchorY};
        m.sigmaM *= std::sqrt(k);
        m_corrSum += k;
        ++m_corrN;
    }
    return StepRecal(dt, adj, hopCount, activeTierRanges, activeTierNeighbours, tierEvidenceMask);
}

CaTblDecision
CaTblPolicy::StepRecal(double dt,
                       const std::vector<CaTblRangeMeasurement>& meas,
                       uint32_t hopCount,
                       int32_t activeTierRanges,
                       int32_t activeTierNeighbours,
                       uint32_t tierEvidenceMask)
{
    if (!m_config.tofRecal)
    {
        return StepImpl(dt, meas, hopCount, activeTierRanges, activeTierNeighbours, tierEvidenceMask);
    }
    // C2 --tofRecal (scope 2026-10-17; U9). A tri-band anchor ranged on both 868 MHz (RSSI) and 2.4 GHz (ToF) in one
    // cycle gives e = ln(r868 / r2.4): the 868 MHz range's log-error, with the ToF range as reference. Its running
    // mean (EWMA, weight 0.05) is the systematic part of the RSSI error -- path-loss model mismatch and the bias
    // toward short ranges that reception selects for -- and is divided out of every 868 MHz range once >= 5 pairs
    // have taught it. The sigma scales with the range, as the RSSI sigma does.
    for (const auto& a : meas)
    {
        if (a.tier != 0 || !(a.rangeM > 0.0))
        {
            continue;
        }
        for (const auto& b : meas)
        {
            if (b.tier == 1 && b.anchorId == a.anchorId && b.rangeM > 1.0)
            {
                const double e = std::log(a.rangeM / b.rangeM);
                if (std::isfinite(e))
                {
                    m_recalBias = (m_recalPairs == 0) ? e : 0.95 * m_recalBias + 0.05 * e;
                    ++m_recalPairs;
                }
                break;
            }
        }
    }
    if (m_recalPairs < 5)
    {
        return StepImpl(dt, meas, hopCount, activeTierRanges, activeTierNeighbours, tierEvidenceMask);
    }
    std::vector<CaTblRangeMeasurement> adj(meas);
    const double f = std::exp(-m_recalBias);
    for (auto& m : adj)
    {
        if (m.tier == 0)
        {
            m.rangeM *= f;
            m.sigmaM *= f;
        }
    }
    return StepImpl(dt, adj, hopCount, activeTierRanges, activeTierNeighbours, tierEvidenceMask);
}

CaTblDecision
CaTblPolicy::StepImpl(double dt,
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
    // NW-4: a static arm is single-band by definition and ignores a query's hook.
    const bool hybridMode = (m_config.mode == CA_TBL_MODE_FULL ||
                             m_config.mode == CA_TBL_MODE_NO_INTEGRITY ||
                             m_config.mode == CA_TBL_MODE_FIXED_THRESHOLD);
    if (m_queryEscArmed && (!m_config.queryTriggeredEscalation || !hybridMode))
    {
        m_queryEscArmed = false;
        ++m_queryEscIgnored;
    }

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
        if (m_config.imuFilterEkf5)
        {
            Ekf5Predict(dt, true, m_imu[0], m_imu[1]);
        }
        else
        {
            m_ekf.PredictWithControl(dt, m_imu[0], m_imu[1], m_imu[2], m_imu[3], m_imu[4]);
        }
        // The particle cloud is predicted with the same IMU increment whichever filter runs.
        d.imuPredicted = true;
        if (m_pfActive)
        {
            PfPredict(dt, m_imu[0], m_imu[1], m_imu[2], m_imu[4]);
        }
    }
    else if (m_config.imuFilterEkf5)
    {
        Ekf5Predict(dt, false, 0.0, 0.0);
        if (m_pfActive)
        {
            PfPredict(dt, 0.0, 0.0, -1.0, -1.0);
        }
    }
    else
    {
        m_ekf.Predict(dt);
        if (m_pfActive)
        {
            PfPredict(dt, 0.0, 0.0, -1.0, -1.0);
        }
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
    // --acquireMinAnchors (S2-C): before the first cycle that fuses at least that many ranges,
    // the prior cannot pick a two-range root or anchor a one-range update, so neither runs and
    // any cycle with fewer ranges coasts. Off (0), acqBlock is false and nothing changes.
    const bool acqBlock = !m_acquired;
    if (m_config.sparseInStep && meas.size() == 2 && !acqBlock)
    {
        const bool bothSurveyed = meas[0].sourceKind == CA_TBL_SRC_SURVEYED_GATEWAY &&
                                  meas[1].sourceKind == CA_TBL_SRC_SURVEYED_GATEWAY;
        d.sparsePriorX = FltX();
        d.sparsePriorY = FltY();
        d.sparsePriorSource = d.imuPredicted ? 1 : 0; // DC-2
        const CaTblSparseResult sr =
            bothSurveyed
                ? CaTblSolveCaseA(meas[0].anchorX, meas[0].anchorY, meas[0].rangeM,
                                  meas[1].anchorX, meas[1].anchorY, meas[1].rangeM,
                                  FltX(), FltY(), FltPxx(), FltPxy(), FltPyy(),
                                  m_config.sparseMarginM)
                : CaTblSolveCaseB(meas[0].anchorX, meas[0].anchorY, meas[0].rangeM,
                                  meas[1].anchorX, meas[1].anchorY, meas[1].rangeM,
                                  FltX(), FltY(), FltPxx(), FltPxy(), FltPyy(),
                                  m_config.sparseMarginM);
        d.sparseVerdict = static_cast<uint8_t>(sr.verdict);
        const double var = 2.0 * (meas[0].sigmaM * meas[0].sigmaM +
                                  meas[1].sigmaM * meas[1].sigmaM);
        if (sr.verdict == CA_TBL_SPARSE_OK && var > 0.0 && std::isfinite(var))
        {
            FltFusePosition(sr.x, sr.y, var);
            d.positionFuses = 1;
            d.sparseX = sr.x;
            d.sparseY = sr.y;
            d.sparseVarM2 = var;
            d.fixKind = (sr.solvedBy == CA_TBL_CASE_B) ? CA_TBL_FIX_CASE_B : CA_TBL_FIX_CASE_A;
            d.hadFix = true;
            d.estX = FltX();
            d.estY = FltY();
            d.traceSigma = FltTracePos();
            d.nAnchorsUsed = 2;
            d.nextTier = m_tier;
            return d;
        }
    }

    // DC-4. Case D: exactly one range, under the flag, goes down the ordinary path below
    // as a scalar update on the propagated state (NIS-guarded), and reaches the decision.
    const bool caseD = m_config.caseD && meas.size() == 1 && !acqBlock;

    // Fewer than three ranges cannot constrain a 2-D fix. Coast on the
    // prediction. Cases A and B now exist (ca-tbl-sparse-anchor, since ac5766f). By
    // default they run in the SCENARIO, before Step(), against the previous posterior --
    // so a two-range cycle still arrives here, is counted a no-fix and counts toward
    // demotion. Under sparseInStep (DC-11) they run above, and only a refused one
    // arrives here. Case C only classifies; Case D does not exist yet (DC-4).
    if ((meas.size() < 3 && !caseD) ||
        (acqBlock && meas.size() < static_cast<size_t>(m_config.acquireMinAnchors)))
    {
        m_noFixCycles++;
        d.hadFix = false;
        d.estX = FltX();
        d.estY = FltY();
        d.traceSigma = FltTracePos();
        d.nAnchorsUsed = static_cast<uint32_t>(meas.size());

        // NW-4. The hook acts here too: an URGENT query is a reason to go up even on a
        // cycle with no fix. The two evidence preconditions still apply, from the ranges
        // this cycle did get; the IMU and corroboration witnesses need a fix and do not
        // run on a coast. The dropout is not counted toward demotion on such a cycle.
        if (m_queryEscArmed && m_tier < CA_TBL_TIER_UWB && !m_uwbPending)
        {
            ++m_queryEscFired;
            d.queryTriggered = true;
            d.wantedEscalation = true;
            bool blocked = false;
            if (m_tier == CA_TBL_TIER_2P4)
            {
                blocked = UwbEvidenceWouldBlock(d.traceSigma, meas);
            }
            else if (m_config.escalationRequiresEvidenceT2)
            {
                uint32_t nearT2 = 0;
                for (size_t i = 0; i < meas.size(); ++i)
                {
                    if (meas[i].rangeM > 0.0 && meas[i].rangeM <= m_config.t2PredictRadiusM)
                    {
                        ++nearT2;
                    }
                }
                blocked = nearT2 < m_config.t2PredictMinAnchors;
            }
            if (blocked)
            {
                ++m_queryEscSuppressed;
                m_queryEscArmed = false;
            }
            else
            {
                QueryEscalateOneTier(d);
                ++m_queryEscalations;
                if (m_tier == CA_TBL_TIER_UWB || m_uwbPending)
                {
                    m_queryEscArmed = false;
                }
            }
            d.nextTier = m_tier;
            return d;
        }

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
    m_acquired = true; // --acquireMinAnchors (S2-C): the first full fix acquires; already true when off
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
        // --nlosRSource (S2-C). 0 (default): the operative tag `nlos`, built by --nlosSource
        // (under the preset, the propagation truth). 1: the detector's verdict only (score at or
        // above integrityNlosScoreThreshold; an unscored link counts as LOS), so no truth reaches
        // R or the LOS count. 2: no link is treated as NLOS here. Off, `nl` is `nlos` exactly.
        const bool nl = (m_config.nlosRSource == 0)   ? meas[i].nlos
                        : (m_config.nlosRSource == 1) ? (meas[i].nlosScore >= 0.0 &&
                                                         meas[i].nlosScore >= m_config.integrityNlosScoreThreshold)
                                                      : false;
        double sigma;
        if (meas[i].sigmaM > 0.0)
        {
            sigma = meas[i].sigmaM;
            if (nl)
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
            sigma = nl ? m_config.nlosSigmaM[m_tier] : m_config.losSigmaM[m_tier];
        }
        // --measVarScale (S2-C): one multiplier on every range variance this Step() fuses.
        // 1.0 multiplies exactly; the TDoA position (FuseAux) and the scenario's sparse
        // solves form their own variances and are not scaled.
        varM2.push_back(sigma * sigma * m_config.measVarScale);
        // Raw = pre-inflation. Recomputed rather than captured above so the two
        // branches of the sigma selection stay legible.
        {
            const double raw = (meas[i].sigmaM > 0.0)
                                   ? meas[i].sigmaM
                                   : (nl ? m_config.nlosSigmaM[m_tier]
                                         : m_config.losSigmaM[m_tier]);
            sumVarRaw += raw * raw;
        }
        if (!nl)
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
        if (RawMultilaterationWeighted(meas, varM2, FltPriorX(), FltPriorY(),
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
    // --acquireReinit (R1, TECH_DEBT N.54). Under --unknownStart the filter is seeded at a prior
    // kilometres off, and linearising the first range update there locks it in (S2-C: every arm
    // about 0.2 within 25 m at 5 km; --acquireMinAnchors did not help). On the first cycle with
    // three or more ranges, solve the weighted multilateration from two starting points -- the
    // propagated prior and the anchors' centroid -- keep the one with the smaller weighted
    // residual, and re-initialise the filter there with the fix's own covariance (trace per axis,
    // velocity zero, the same velocity variance the REINIT recovery uses). The ranges are not then
    // applied again (no double count), and the integrity test does not run on this cycle: its
    // reference would be the solution itself. Off (default), acqReinitNow is false and nothing
    // below changes.
    bool acqReinitNow = false;
    if (m_pfActive && !meas.empty())
    {
        // V4 --acquireParticles: this cycle's ranges weight the particle cloud instead of entering the EKF.
        // The EKF carries the cloud's mean and covariance (so every reader -- PCS, escalation, relays,
        // finders -- sees the honest, possibly multimodal spread). When the cloud is one mode, the EKF takes
        // over from it; these ranges are not applied again, and the integrity test does not run.
        m_negApplied += m_config.acquireNegInfo ? m_silent.size() : 0;
        const bool oneMode = PfUpdate(meas, varM2);
        double mx = 0.0, my = 0.0, cxx = 0.0, cxy = 0.0, cyy = 0.0;
        PfStats(mx, my, cxx, cxy, cyy);
        if (m_config.imuFilterEkf5)
        {
            m_ekf5.SetPositionCov(mx, my, cxx, cxy, cyy);
        }
        else
        {
            m_ekf.SetPositionCov(mx, my, cxx, cxy, cyy, 100.0);
        }
        if (oneMode)
        {
            m_pfActive = false;
            ++m_pfHandovers;
            std::cout << "PFACQ_HANDOVER cycles=" << m_pfCycles << " x=" << mx << " y=" << my
                      << " sd=" << std::sqrt(std::max(0.0, cxx + cyy)) << "\n";
        }
        acqReinitNow = true;
    }
    if (!acqReinitNow && m_config.acquireReinit && m_reinitPending && !caseD && meas.size() >= 3)
    {
        double cx = 0.0;
        double cy = 0.0;
        for (size_t i = 0; i < meas.size(); ++i)
        {
            cx += meas[i].anchorX;
            cy += meas[i].anchorY;
        }
        cx /= static_cast<double>(meas.size());
        cy /= static_cast<double>(meas.size());
        const double sx[2] = {FltPriorX(), cx};
        const double sy[2] = {FltPriorY(), cy};
        double bestRes = std::numeric_limits<double>::infinity();
        double bx = 0.0, by = 0.0, brxx = 0.0, bryy = 0.0;
        for (int k = 0; k < 2; ++k)
        {
            double ox = 0.0, oy = 0.0, rxx = 0.0, rxy = 0.0, ryy = 0.0;
            if (!RawMultilaterationWeighted(meas, varM2, sx[k], sy[k], ox, oy, rxx, rxy, ryy))
            {
                continue;
            }
            double res = 0.0;
            for (size_t i = 0; i < meas.size() && i < varM2.size(); ++i)
            {
                const double dd = std::hypot(ox - meas[i].anchorX, oy - meas[i].anchorY);
                const double e = meas[i].rangeM - dd;
                res += (varM2[i] > 0.0) ? e * e / varM2[i] : 0.0;
            }
            if (std::isfinite(res) && res < bestRes)
            {
                bestRes = res;
                bx = ox;
                by = oy;
                brxx = rxx;
                bryy = ryy;
            }
        }
        // Residual guard: a solution its own ranges contradict is not an acquisition. Accept it only
        // if the weighted residual is within chi^2(n - 2, 0.999), Wilson-Hilferty approximation
        // (within 0.4 of the table value at k = 1 and closer above); otherwise stay armed and try
        // the next cycle with three or more ranges.
        const double kdof = std::max(1.0, static_cast<double>(meas.size()) - 2.0);
        const double wh = 1.0 - 2.0 / (9.0 * kdof) + 3.0902 * std::sqrt(2.0 / (9.0 * kdof));
        const double chi999 = kdof * wh * wh * wh;
        if (std::isfinite(bestRes) && bestRes > chi999)
        {
            ++m_acqReinitRefused;
            std::cout << "ACQREINIT_REFUSED n=" << meas.size() << " res=" << bestRes << " limit=" << chi999 << "\n";
        }
        if (std::isfinite(bestRes) && bestRes <= chi999)
        {
            const double pv = ((brxx + bryy) > 0.0 && std::isfinite(brxx + bryy)) ? (brxx + bryy) : 100.0;
            if (m_config.imuFilterEkf5)
            {
                m_ekf5.ReinitPosition(bx, by, pv); // position only, as REINIT recovery does
            }
            else
            {
                m_ekf.Init(bx, by, 0.0, 0.0, pv, 100.0);
            }
            m_reinitPending = false;
            ++m_acqReinits;
            acqReinitNow = true;
            std::cout << "ACQREINIT x=" << bx << " y=" << by << " pv=" << pv << " n=" << meas.size()
                      << " res=" << bestRes << "\n";
        }
    }

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
    if (m_config.mode == CA_TBL_MODE_FULL && !acqReinitNow)
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
                                                 FltPriorX(),
                                                 FltPriorY(),
                                                 zx,
                                                 zy,
                                                 rxx,
                                                 rxy,
                                                 ryy);
            if (haveFix)
            {
                mahal = FltMahalanobisFromPriorWithR(zx, zy, rxx, rxy, ryy);
                fixRxx = rxx;
                fixRxy = rxy;
                fixRyy = ryy;
            }
        }
        else
        {
            haveFix = RawMultilateration(meas, FltPriorX(), FltPriorY(), zx, zy);
            if (haveFix)
            {
                mahal = FltMahalanobisFromPrior(zx, zy);
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
        FltUpdate(meas, inflated);
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
            FltUpdate(meas, varM2);
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
            FltRevertToPrior();
            double pv = 100.0;
            if ((fixRxx + fixRyy) > 0.0)
            {
                pv = m_config.suspectInflateFactor * (fixRxx + fixRyy);
            }
            else
            {
                m_reinitFallbackCount++;
            }
            if (m_config.imuFilterEkf5)
            {
                // REINIT moves the position only: the heading offset, bias and stride scale
                // the filter has learned are not what the fault was in.
                m_ekf5.ReinitPosition(zx, zy, pv);
            }
            else
            {
                m_ekf.Init(zx, zy, m_ekf.Vx(), m_ekf.Vy(), pv, 100.0);
            }
            recovered = true;
        }
        else
        {
            FltRevertToPrior();
            if (m_config.suspectRecovery == CA_TBL_RECOVERY_INFLATE)
            {
                // Rejecting is the filter asserting model and data disagree; the
                // honest consequence is more uncertainty, not the same uncertainty.
                FltInflatePos(m_config.suspectInflateFactor);
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
    else if (acqReinitNow)
    {
        // R1: the filter was re-initialised at this cycle's solution above; nothing to fuse.
        m_consecutiveRejections = 0;
    }
    else
    {
        m_consecutiveRejections = 0;
        if (caseD)
        {
            // DC-4. One range against a wide P can leave an inconsistent covariance, so it
            // goes in only if its innovation is plausible: NIS = nu^2 / (H P H^T + R) with
            // H the unit vector from the anchor, against chi^2(1 dof, 0.99) = 6.63.
            const double dx = FltX() - meas[0].anchorX;
            const double dy = FltY() - meas[0].anchorY;
            const double r = std::max(std::sqrt(dx * dx + dy * dy), 1e-6);
            const double hx = dx / r, hy = dy / r;
            const double s = hx * hx * FltPxx() + 2.0 * hx * hy * FltPxy() +
                             hy * hy * FltPyy() + varM2[0];
            const double nu = meas[0].rangeM - r;
            d.caseDNis = (s > 0.0) ? nu * nu / s : -1.0;
            if (s > 0.0 && d.caseDNis <= 6.63)
            {
                FltUpdate(meas, varM2);
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
            FltUpdate(meas, varM2);
            ++d.rangeUpdates;
        }
        if (auxQueued)
        {
            FuseAux(); // the COMBINED tier-1 fix: RSSI ranges above, TDoA position here
        }
    }

    const double traceSigma = FltTracePos();
    double sumVar = 0.0;
    for (size_t i = 0; i < varM2.size(); ++i)
    {
        sumVar += varM2[i];
    }
    d.meanSigmaM = varM2.empty() ? 0.0 : std::sqrt(sumVar / static_cast<double>(varM2.size()));
    d.meanSigmaRawM =
        varM2.empty() ? 0.0 : std::sqrt(sumVarRaw / static_cast<double>(varM2.size()));

    const double gdop = ComputeGdop(FltX(), FltY(), meas);
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
    d.estX = FltX();
    d.estY = FltY();

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

    // C3 --hold24AfterK: while 2.4 GHz has delivered a fix on its own (>= 3 ranges) for K straight cycles, the node
    // holds on 2.4 GHz -- no UWB escalation -- because in the endgame 2.4 GHz alone matched the hybrid at ~40 % of the
    // energy (V8) and UWB is the largest share of it (V6). A cycle below 3 ranges resets the streak and releases.
    if (m_config.hold24AfterK > 0)
    {
        m_streak24 = (m_tier == CA_TBL_TIER_2P4 && activeTierRanges >= 3) ? m_streak24 + 1 : 0;
    }

    // V5 --c2bEscalation (master plan v3; C2b). The trigger is NEED, not low confidence: a hybrid arm always
    // wants the next tier, and only the observation that it is reachable decides (below, the evidence gate's
    // near-neighbour test with its estimate-quality veto skipped -- that veto is the confidence gate Stage A
    // found harmful). 868 -> 2.4 under the dual base is already attempted every cycle; this acts on 2.4 -> UWB.
    if (m_config.c2bEscalation && dynamic && m_tier < CA_TBL_TIER_UWB)
    {
        wantEscalate = true;
    }

    // NW-4: an URGENT query forces the decision, whatever the PCS; the suppress-only
    // preconditions below still apply to it.
    const bool queryHook = m_queryEscArmed && dynamic && m_tier < CA_TBL_TIER_UWB && !m_uwbPending;
    if (queryHook)
    {
        wantEscalate = true;
        d.queryTriggered = true;
        ++m_queryEscFired;
    }
    d.wantedEscalation = wantEscalate; // DC-4: before any precondition suppresses it
    if (m_config.hold24AfterK > 0 && m_streak24 >= m_config.hold24AfterK && m_tier == CA_TBL_TIER_2P4 && !queryHook)
    {
        wantEscalate = false; // C3 hold: 2.4 GHz is solving on its own
    }
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
            if (!m_config.c2bEscalation &&
                sigPos > m_config.uwbPredictMaxSigmaFrac * m_config.uwbPredictRadiusM)
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
                    const double dx = m_config.uwbAnchorX[i] - FltX();
                    const double dy = m_config.uwbAnchorY[i] - FltY();
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
    if (dynamic && !activeDelivered && !caseD && !(queryHook && wantEscalate))
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
        // (C3 dev, 2026-10-17: also vetoing this de-escalation during the hold kept the node on 2.4 GHz, where every
        // broken streak let PCS escalate to UWB: UWB cycles 37 -> 87, tag-0 energy 30.3 -> 43.1 J. Not kept.)

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

    if (queryHook)
    {
        // NW-4: an escalation made under the hook is counted; on UWB, or once UWB is asked
        // for, the hook has done its job. Suppressed by a precondition, it disarms.
        if (d.escalated)
        {
            ++m_queryEscalations;
            if (m_tier == CA_TBL_TIER_UWB || m_uwbPending)
            {
                m_queryEscArmed = false;
            }
        }
        else if (!wantEscalate)
        {
            ++m_queryEscSuppressed;
            m_queryEscArmed = false;
        }
    }
    d.nextTier = m_tier;
    // Remember where the filter was, so the next cycle can ask whether it moved the
    // way the body did. Recorded on EVERY cycle including refused ones: the comparison
    // is against the previous cycle, not the last accepted escalation.
    m_prevEstX = FltX();
    m_prevEstY = FltY();
    m_havePrevEst = true;
    // Consumed. A stale cross-check is worse than none -- it would silently police one
    // cycle with another cycle evidence -- so the caller must supply it every cycle it
    // wants the check to run.
    m_crossCheckFresh = false;

    return d;
}

} // namespace ns3
