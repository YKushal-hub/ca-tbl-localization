/*
 * Unit suite for the Switching-Point Integrity Check's responses (CaTblPolicy::Step,
 * the suspect branch). Not the evidence gate: that is a different mechanism
 * (RULES.md sect 2).
 *
 * DC-7 (docs/SPEC_CONFORMANCE_PLAN.md). The spec (Physical 2.6) says a SUSPECT fix is
 * flagged, is not allowed alone to justify a de-escalation, and is "downweight[ed] in
 * PCS regardless of raw trace(Sigma)". The default rejects it instead.
 * CA_TBL_RECOVERY_DOWNWEIGHT accepts it with R x suspectInflateFactor and scales PCS by
 * suspectPcsFactor (or, on v2's log scale, subtracts log10(1/factor)).
 *
 * Every cycle below is the same one: three LOS anchors, the prior at (500, 500), and
 * ranges placing the node 3.5 km away -- a fix the check must call suspect. The exact
 * control is BOUNDED with N = 1, which force-accepts that cycle at full R: DOWNWEIGHT at
 * inflate 1 and PCS factor 1 must equal it bit for bit.
 *
 * DC-8. Which fixes are ELIGIBLE to be called suspect. The spec's condition is "NLOS_score
 * is LOW"; the default is "every link tagged LOS". Under integrityNlosFromScore the mean
 * score over the scored links decides, and a set with no scored link falls back to the tag.
 */

#include "ns3/ca-tbl-policy.h"
#include "ns3/core-module.h"

#include <cmath>
#include <iostream>
#include <vector>

using namespace ns3;

static int g_pass = 0;
static int g_fail = 0;

static void
Check(bool ok, const char* what)
{
    if (ok)
    {
        ++g_pass;
        std::cout << "  ok    " << what << "\n";
    }
    else
    {
        ++g_fail;
        std::cout << "  FAIL  " << what << "\n";
    }
}

struct Pt
{
    double x;
    double y;
};

static std::vector<CaTblRangeMeasurement>
Ranges(const std::vector<Pt>& a, Pt p, double sigmaM)
{
    std::vector<CaTblRangeMeasurement> m;
    for (size_t k = 0; k < a.size(); ++k)
    {
        CaTblRangeMeasurement r;
        r.anchorX = a[k].x;
        r.anchorY = a[k].y;
        r.rangeM = std::hypot(p.x - a[k].x, p.y - a[k].y);
        r.nlos = false;
        r.nlosPropagation = false;
        r.hiddenChannelActive = false;
        r.sigmaM = sigmaM;
        m.push_back(r);
    }
    return m;
}

static const std::vector<Pt> kA3 = {{0, 0}, {1000, 0}, {0, 1000}};
static const Pt kPrior = {500, 500};

struct Out
{
    CaTblDecision d;
    double x;
    double y;
    double pxx;
    double pyy;
    uint64_t suspects;
    uint64_t downweighted;
    uint64_t recoveries;
    bool auxFused;
};

//! One Step of \p cfg from the fixed prior, on ranges placing the node at \p at.
static Out
Run(const CaTblPolicyConfig& cfg, Pt at, bool aux = false)
{
    CaTblPolicy p;
    p.Configure(cfg);
    p.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
    if (aux)
    {
        p.SetAuxPositionFix(at.x, at.y, 25.0, 0.0, 25.0);
    }
    Out o;
    o.d = p.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
    o.x = p.EstX();
    o.y = p.EstY();
    o.pxx = p.Pxx();
    o.pyy = p.Pyy();
    o.suspects = p.GetSuspectCount();
    o.downweighted = p.GetDownweightedFixes();
    o.recoveries = p.GetRecoveryFirings();
    o.auxFused = p.AuxFusedLast();
    return o;
}

//! One Step on ranges placing the node at \p at, every link tagged \p nlosTag and carrying
//! the classifier score \p score (-1 = unscored).
static CaTblDecision
RunScored(const CaTblPolicyConfig& cfg, Pt at, bool nlosTag, double score, CaTblPolicy* keep = nullptr)
{
    CaTblPolicy local;
    CaTblPolicy& p = keep ? *keep : local;
    p.Configure(cfg);
    p.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
    std::vector<CaTblRangeMeasurement> m = Ranges(kA3, at, 20.0);
    for (CaTblRangeMeasurement& r : m)
    {
        r.nlos = nlosTag;
        r.nlosScore = score;
    }
    return p.Step(1.0, m, 0, 3, 3, 7u);
}

static bool
SameState(const Out& a, const Out& b)
{
    return a.x == b.x && a.y == b.y && a.pxx == b.pxx && a.pyy == b.pyy;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd;
    cmd.Parse(argc, argv);
    const Pt far = {3000, 3000};
    const Pt near = {510, 495};

    CaTblPolicyConfig none; // the default: CA_TBL_RECOVERY_NONE
    CaTblPolicyConfig dw = none;
    dw.suspectRecovery = CA_TBL_RECOVERY_DOWNWEIGHT;
    CaTblPolicyConfig bounded1 = none;
    bounded1.suspectRecovery = CA_TBL_RECOVERY_BOUNDED;
    bounded1.suspectRecoveryN = 1;

    std::cout << "--- the default rejects (negative control)\n";
    const Out rN = Run(none, far);
    Check(rN.d.suspect && rN.d.measurementRejected && rN.downweighted == 0,
          "NONE: the far fix is suspect and rejected; nothing is counted downweighted");

    std::cout << "--- DOWNWEIGHT accepts, flagged\n";
    const Out rD = Run(dw, far);
    Check(rD.d.suspect && !rD.d.measurementRejected,
          "the far fix is still flagged suspect, and is NOT rejected");
    Check(rD.suspects == 1 && rD.downweighted == 1 && rD.recoveries == 0,
          "counted: 1 suspect, 1 downweighted fix, 0 recovery firings (it is not a recovery)");
    Check(std::hypot(rD.x - rN.x, rD.y - rN.y) > 1.0,
          "the estimate moved toward the fix (the rejected run's did not)");

    std::cout << "--- R is inflated by suspectInflateFactor\n";
    const Out rB = Run(bounded1, far);
    Check(rB.d.suspect && rB.pxx + rB.pyy < rD.pxx + rD.pyy &&
              std::hypot(rB.x - rN.x, rB.y - rN.y) > std::hypot(rD.x - rN.x, rD.y - rN.y),
          "at inflate 4 it fuses less than a full-R force-accept: larger trace(P), moved less");
    CaTblPolicyConfig dw11 = dw;
    dw11.suspectInflateFactor = 1.0;
    dw11.suspectPcsFactor = 1.0;
    const Out rD11 = Run(dw11, far);
    Check(SameState(rD11, rB) && rD11.d.pcs == rB.d.pcs,
          "EXACT CONTROL: at inflate 1 and PCS factor 1 it IS the full-R force-accept, bit for bit");

    std::cout << "--- PCS is downweighted, and nothing else is\n";
    CaTblPolicyConfig dw1h = dw11;
    dw1h.suspectPcsFactor = 0.5;
    const Out rD1h = Run(dw1h, far);
    Check(SameState(rD1h, rD11) && rD1h.d.pcs == 0.5 * rD11.d.pcs && rD11.d.pcs > 0.0,
          "sum forms: PCS x 0.5 exactly; the estimate and P are untouched by the factor");
    CaTblPolicyConfig v2a = dw11;
    v2a.pcsForm = CA_TBL_PCS_V2;
    v2a.ApplyV2Thresholds();
    CaTblPolicyConfig v2b = v2a;
    v2b.suspectPcsFactor = 0.5;
    const Out rV1 = Run(v2a, far);
    const Out rV2 = Run(v2b, far);
    Check(SameState(rV1, rV2) && std::fabs((rV1.d.pcs - rV2.d.pcs) - std::log10(2.0)) < 1e-12,
          "v2 (log scale, may be negative): PCS - log10(1/0.5), not x 0.5");

    std::cout << "--- only suspect cycles are touched\n";
    const Out gN = Run(none, near);
    const Out gD = Run(dw, near);
    Check(!gN.d.suspect && !gD.d.suspect && SameState(gN, gD) && gN.d.pcs == gD.d.pcs &&
              gD.downweighted == 0,
          "a clean fix: DOWNWEIGHT and NONE give the same state and PCS, bit for bit");

    std::cout << "--- the TDoA half of the fix is inflated the same way\n";
    const Out aD = Run(dw, far, true);
    const Out aB = Run(bounded1, far, true);
    const Out aN = Run(none, far, true);
    Check(aD.auxFused && !aN.auxFused && aB.auxFused && aD.pxx + aD.pyy > aB.pxx + aB.pyy,
          "fused under DOWNWEIGHT (not under NONE), at inflated R: larger trace(P) than full R");
    CaTblPolicyConfig dw11a = dw11;
    const Out aD11 = Run(dw11a, far, true);
    Check(SameState(aD11, aB),
          "EXACT CONTROL with the aux fix: inflate 1 IS the full-R force-accept of both halves");

    std::cout << "--- the escalation decision reads the downweighted PCS\n";
    CaTblPolicyConfig e1 = dw11;
    e1.suspectForcesEscalation = false;
    const Out probe = Run(e1, far);
    // An up threshold between the penalised and the unpenalised score.
    const double thr = 0.75 * probe.d.pcs;
    for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
    {
        e1.pcsThresholdUp[k] = thr;
        e1.pcsThresholdDown[k] = thr + 10.0;
    }
    CaTblPolicyConfig e5 = e1;
    e5.suspectPcsFactor = 0.5;
    const Out o1 = Run(e1, far);
    const Out o5 = Run(e5, far);
    Check(probe.d.pcs > 0.0 && !o1.d.escalated && o5.d.escalated,
          "with the forced escalation off, the penalised score crosses the threshold and "
          "escalates; the same cycle unpenalised does not");

    std::cout << "--- DC-8: eligibility from the NLOS score\n";
    {
        CaTblPolicyConfig tag; // the default: every link tagged LOS
        CaTblPolicyConfig sc = tag;
        sc.integrityNlosFromScore = true;
        const CaTblDecision a = RunScored(tag, far, true, 0.1);
        Check(!a.suspect && a.integrityBasis == 0 && !a.integrityEligible,
              "NEGATIVE: default, links tagged NLOS but scored low: ineligible, not suspect");
        CaTblPolicy pb;
        const CaTblDecision b = RunScored(sc, far, true, 0.1, &pb);
        Check(b.suspect && b.integrityBasis == 1 && b.integrityEligible &&
                  std::fabs(b.integrityMeanScore - 0.1) < 1e-12,
              "flag: the same fix, scored 0.1 -- the score decides, eligible, suspect");
        Check(pb.GetIntegrityScoreEligible() == 1 && pb.GetIntegrityScoreIneligible() == 0 &&
                  pb.GetIntegrityTagFallback() == 0,
              "counted: 1 score-eligible, 0 ineligible, 0 fallbacks");
        CaTblPolicy pc;
        const CaTblDecision c = RunScored(sc, far, false, 0.8, &pc);
        Check(!c.suspect && c.integrityBasis == 1 && !c.integrityEligible &&
                  pc.GetIntegrityScoreIneligible() == 1,
              "flag: tagged LOS but scored 0.8 -- the score decides, ineligible, not suspect");
        CaTblPolicy pd;
        const CaTblDecision dd = RunScored(sc, far, false, -1.0, &pd);
        Check(dd.suspect && dd.integrityBasis == 2 && dd.integrityMeanScore == -1.0 &&
                  pd.GetIntegrityTagFallback() == 1,
              "flag: no link scored -- falls back to the tag (all LOS), suspect, counted");
        const CaTblDecision e = RunScored(sc, far, true, -1.0);
        Check(!e.suspect && e.integrityBasis == 2,
              "flag: no link scored and tagged NLOS -- the tag says ineligible");
        CaTblPolicyConfig lo = sc;
        lo.integrityNlosScoreThreshold = 0.2;
        const CaTblDecision f5 = RunScored(sc, far, true, 0.3);
        const CaTblDecision f2 = RunScored(lo, far, true, 0.3);
        Check(f5.suspect && !f2.suspect && !f2.integrityEligible,
              "the threshold decides: 0.3 is LOW against 0.5 and not against 0.2");
        CaTblPolicy pg;
        const CaTblDecision g = RunScored(sc, near, true, 0.1, &pg);
        Check(g.integrityEligible && !g.suspect && pg.GetSuspectCount() == 0,
              "a clean fix: eligible, and the Mahalanobis test clears it");
        Check(RunScored(tag, far, false, 0.9).suspect,
              "NEGATIVE: default ignores the score -- tagged LOS, scored 0.9, still suspect");
    }

    std::cout << "--- NW-7: an escalation into UWB commits only on a confirmed grant\n";
    {
        const Pt at = {400, 300};
        CaTblPolicyConfig g;
        g.mode = CA_TBL_MODE_FULL;
        g.escalationRequiresEvidence = false;
        g.escalationRequiresEvidenceT2 = false;
        for (int k = 0; k < CA_TBL_TIER_COUNT; ++k)
        {
            g.pcsThresholdUp[k] = 1e9; // always wants to escalate
            g.pcsThresholdDown[k] = 2e9;
        }
        g.grantGatedUwb = true;
        CaTblPolicy p;
        p.Configure(g);
        p.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
        const CaTblDecision s1 = p.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        const CaTblDecision s2 = p.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        const CaTblDecision s3 = p.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        Check(s1.escalated && !s1.uwbPending && s1.nextTier == CA_TBL_TIER_2P4,
              "868 -> 2.4 GHz is not gated");
        Check(s2.escalated && s2.uwbPending && s2.nextTier == CA_TBL_TIER_2P4 &&
                  p.IsUwbPending() && p.GetUwbRequests() == 1 && p.GetEscalationCount() == 1,
              "wanting UWB asks: escalated with the pending qualifier, the tier stays 2.4 GHz, "
              "and no escalation is counted yet");
        Check(!s3.escalated && s3.activeTier == CA_TBL_TIER_2P4 && p.GetUwbWaitCycles() == 1 &&
                  p.GetUwbRequests() == 1,
              "no UWB without a grant: the next cycle waits on 2.4 GHz and does not ask again");
        Check(p.ConfirmUwbGrant() && p.GetActiveTier() == CA_TBL_TIER_UWB &&
                  p.GetEscalationCount() == 2,
              "a confirmed grant switches the tier, counted as the escalation");
        const CaTblDecision s4 = p.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        Check(s4.activeTier == CA_TBL_TIER_UWB && !p.ConfirmUwbGrant() && p.GetUwbLate() == 1,
              "the next cycle runs on UWB; a second grant changes nothing and is counted late");

        // A denial drops the request; the next cycle that wants UWB asks afresh.
        CaTblPolicy q;
        q.Configure(g);
        q.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
        q.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        q.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        const bool cleared = q.ClearUwbPending(false);
        const CaTblDecision q3 = q.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        Check(cleared && q.GetUwbDenied() == 1 && q3.uwbPending && q.GetUwbRequests() == 2 &&
                  q3.activeTier == CA_TBL_TIER_2P4,
              "a denial drops the request; the node stays on 2.4 GHz and asks again");

        // Coverage loss still demotes while pending, and cancels the request: a grant can
        // never jump the node 868 -> UWB.
        CaTblPolicyConfig gb = g;
        gb.nDeescalate = 1;
        CaTblPolicy r;
        r.Configure(gb);
        r.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
        r.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        r.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        const CaTblDecision r3 = r.Step(1.0, std::vector<CaTblRangeMeasurement>(), 0, 0, 0, 7u);
        const bool lateOk = r.ConfirmUwbGrant();
        Check(r3.deescalated && r.GetActiveTier() == CA_TBL_TIER_868 && !lateOk &&
                  r.GetActiveTier() == CA_TBL_TIER_868,
              "coverage loss demotes while pending, and a grant arriving after it is refused");
        r.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
        Check(r.GetUwbCoverageClears() == 1 && !r.IsUwbPending(),
              "the request is counted cancelled by coverage loss");

        // Healthy demotion is frozen while pending. FIXED_THRESHOLD wants to escalate
        // while trace(P) > T and is healthy below T/2; the trace falls cycle by cycle on a
        // stationary node, so a T between the second cycle's trace and twice a later one
        // makes the request and then reaches the healthy branch with it pending.
        CaTblPolicyConfig f = g;
        f.mode = CA_TBL_MODE_FIXED_THRESHOLD;
        f.nDeescalate = 1;
        f.grantGatedUwb = false;
        CaTblPolicy probe;
        probe.Configure(f);
        probe.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
        std::vector<double> tr;
        for (int k = 0; k < 40; ++k)
        {
            tr.push_back(probe.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u).traceSigma);
        }
        f.fixedVarThresholdM2 = 0.9 * tr[1];
        f.grantGatedUwb = true;
        CaTblPolicy h;
        h.Configure(f);
        h.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
        int healthyPending = 0;
        bool demotedPending = false;
        for (int k = 0; k < 40; ++k)
        {
            const bool wasPending = h.IsUwbPending();
            const CaTblDecision dk = h.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u);
            if (wasPending && h.IsUwbPending() && dk.traceSigma < 0.5 * f.fixedVarThresholdM2)
            {
                ++healthyPending;
            }
            demotedPending = demotedPending || (wasPending && dk.deescalated);
        }
        Check(tr[0] > f.fixedVarThresholdM2 && healthyPending > 0 && !demotedPending &&
                  h.GetActiveTier() == CA_TBL_TIER_2P4 && h.IsUwbPending(),
              "healthy cycles while pending do not demote: the tier is frozen on 2.4 GHz");
        CaTblPolicyConfig fo = f;
        fo.grantGatedUwb = false;
        CaTblPolicy ho;
        ho.Configure(fo);
        ho.Initialize(kPrior.x, kPrior.y, 100.0, 5.0);
        bool demotedOff = false;
        for (int k = 0; k < 40; ++k)
        {
            demotedOff = ho.Step(1.0, Ranges(kA3, at, 20.0), 0, 3, 3, 7u).deescalated || demotedOff;
        }
        Check(demotedOff && ho.GetEscalationCount() == 2,
              "CONTROL: the same cycles without the flag reach UWB and then demote -- the "
              "healthy branch is reached");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
