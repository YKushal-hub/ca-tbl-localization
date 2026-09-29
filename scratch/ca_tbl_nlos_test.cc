/*
 * Unit suite for the NLOS scores (Physical sect 1.3, 2.5; TECH_DEBT L.2.2, K.8).
 *
 * WHAT IS UNDER TEST: the leading-edge test (NLOS_score = 1 - A_le / A_peak) on CIRs built
 * by hand; the 802.15.4a synthesis (energy normalisation, the NLOS first-cluster onset,
 * determinism of the dedicated stream); the synthesis-plus-test on engine-like LOS and
 * NLOS link states; the nominal path-loss mirror against the engine's own function; the
 * robust reference; the von Neumann variability; and the policy hand-off, i.e. that PCS
 * term 3 moves by exactly the classifier's term and by nothing else.
 *
 * HOW THE EXPECTED VALUES ARE MADE. Never by calling the code under test a second time:
 * the leading-edge fixtures have scores computable by hand; the path loss is compared with
 * ComputePathLossDb() itself, the function the mirror must track; the PCS difference is
 * derived from the form's own algebra (LEGACY sums the terms, so the score must move by
 * exactly the change in term 3, WEIGHTED by w3 times it).
 *
 * THE NEGATIVE FIXTURES: a CIR whose peak is under the noise threshold (not detected, no
 * score), a first path under the threshold (skipped, the edge moves to the next tap), a
 * short robust pool (no score), the flag off with scored links (PCS unchanged), the flag
 * on with no scored link (falls back), and the v2 form (nothing to feed).
 *
 * THE CHARACTERISATION at the end is NOT a scenario result. It feeds the classifier link
 * states drawn like the engine's (RURAL_FORESTED: LOS K ~ N(3,1) in [1,6]; NLOS K ~ N(0.5,
 * 1) >= 0 and an obstruction loss near the UWB value) and prints the confusion matrix, so
 * both truth classes are exercised somewhere -- the scenario cannot, because a received
 * NLOS UWB frame does not occur there (see the commit and the G20 gate block). By the
 * circularity stated in ca-tbl-nlos-classifier.h a near-perfect matrix here is expected
 * and means only that the synthesis encodes the label.
 */

#include "ns3/ca-tbl-nlos-classifier.h"
#include "ns3/ca-tbl-policy.h"
#include "ns3/core-module.h"
#include "ns3/lora-distribution-sampler.h"
#include "ns3/lora-propagation-submodule.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>
#include <string>
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

static bool
Near(double a, double b, double tol)
{
    return std::fabs(a - b) <= tol;
}

static std::vector<CaTblRangeMeasurement>
Meas(const std::vector<std::pair<double, double>>& anchors, double px, double py,
     const std::vector<double>& scores, const std::vector<bool>& nlosTag)
{
    std::vector<CaTblRangeMeasurement> m;
    for (size_t i = 0; i < anchors.size(); ++i)
    {
        CaTblRangeMeasurement r;
        r.anchorX = anchors[i].first;
        r.anchorY = anchors[i].second;
        r.rangeM = std::hypot(px - r.anchorX, py - r.anchorY);
        r.sigmaM = 20.0;
        r.nlos = nlosTag[i];
        r.nlosScore = scores[i];
        m.push_back(r);
    }
    return m;
}

int
main(int argc, char* argv[])
{
    std::cout << "CA-TBL NLOS classifier suite\n";

    // ---------------------------------------------------------------- leading edge
    {
        std::vector<double> c(40, 0.0);
        c[16] = 1.0;
        c[20] = 0.4;
        const CaTblCirResult r = CaTblLeadingEdge(c, 0.0, 3.0);
        Check(r.detected && r.leTap == 16 && r.peakTap == 16 && Near(r.score, 0.0, 1e-15),
              "a CIR whose first path is its peak scores 0 (LOS)");
    }
    {
        std::vector<double> c(40, 0.0);
        c[16] = 0.2;
        c[25] = 1.0;
        const CaTblCirResult r = CaTblLeadingEdge(c, 0.0, 3.0);
        Check(r.detected && r.leTap == 16 && r.peakTap == 25 && Near(r.score, 0.8, 1e-12),
              "first path at 0.2 of the peak scores 1 - 0.2 = 0.8 (sect 1.3)");
    }
    {
        // Noise sigma 0.1, k = 3: threshold 0.3. The 0.2 path is under it and is skipped;
        // the leading edge is the 0.5 tap, so the score is 1 - 0.5/1.0 = 0.5.
        std::vector<double> c(40, 0.05);
        c[16] = 0.2;
        c[18] = 0.5;
        c[25] = 1.0;
        const CaTblCirResult r = CaTblLeadingEdge(c, 0.1, 3.0);
        Check(r.detected && r.leTap == 18 && Near(r.score, 0.5, 1e-12),
              "NEGATIVE: a first path under k*sigma is not the leading edge; the next tap is");
    }
    {
        std::vector<double> c(40, 0.05);
        c[25] = 0.25;
        const CaTblCirResult r = CaTblLeadingEdge(c, 0.1, 3.0);
        Check(!r.detected && r.leTap == -1 && r.score == -1.0,
              "NEGATIVE: a peak under the noise threshold is not detected and gets no score");
    }
    {
        std::vector<double> c(40, 0.0);
        c[30] = 0.7;
        const CaTblCirResult r = CaTblLeadingEdge(c, 0.0, 3.0);
        Check(r.detected && r.leTap == 30,
              "with zero noise the zero-valued pre-window taps are never the leading edge");
    }

    // ------------------------------------------------------------------- synthesis
    {
        CaTblCirConfig cfg;
        cfg.windowNs = 2048.0; // long enough that no CM5 ray is dropped
        CaTblCirSynth s(4242, cfg);
        const CaTblCirModel& m = CaTblCirModelFor(ENV_RURAL_FORESTED, false);
        double eSum = 0.0;
        const int n = 400;
        for (int i = 0; i < n; ++i)
        {
            CaTblCirResult info;
            const std::vector<double> mag =
                s.Synthesize(m, 0.0, std::numeric_limits<double>::infinity(), info);
            for (double v : mag)
            {
                eSum += v * v;
            }
        }
        // Random ray phases: the cross terms of the coherent sum per tap have zero mean, so
        // the expected tap energy is the ray energy, which the synthesis normalises to 1.
        Check(Near(eSum / n, 1.0, 0.05),
              "diffuse part carries unit energy on average (coherent binning, random phases)");
    }
    {
        CaTblCirConfig cfg;
        cfg.windowNs = 2048.0;
        CaTblCirSynth s(777, cfg);
        const CaTblCirModel& m = CaTblCirModelFor(ENV_RURAL_FORESTED, true);
        double onset = 0.0;
        const int n = 2000;
        const double nPre = cfg.preNs / cfg.tapNs;
        for (int i = 0; i < n; ++i)
        {
            CaTblCirResult info;
            const std::vector<double> mag =
                s.Synthesize(m, 0.0, std::numeric_limits<double>::infinity(), info);
            for (size_t k = 0; k < mag.size(); ++k)
            {
                if (mag[k] > 0.0)
                {
                    onset += (static_cast<double>(k) - nPre) * cfg.tapNs;
                    break;
                }
            }
        }
        // CM6 (nlos == 1): first cluster at Exp(mean 1/Lambda) = 1/0.0243 = 41.15 ns after
        // the direct-path delay; taps floor the delay, so expect about 0.5 ns less.
        Check(Near(onset / n, 1.0 / 0.0243 - 0.5, 3.0),
              "CM6 first cluster arrives Exp(1/Lambda) = 41 ns after the direct path");
        Check(std::string(m.name) == "CM6 outdoor NLOS" &&
                  std::string(CaTblCirModelFor(ENV_INDOOR_OFFICE, true).name) == "CM4 office NLOS" &&
                  CaTblCirModelFor(ENV_INDOOR_OFFICE, true).nlosMode == 2,
              "outdoor classes map to CM5/CM6, --envFacility=6 to CM3/CM4 (eq. 22 shape)");
    }
    {
        CaTblCirSynth a(99, CaTblCirConfig());
        CaTblCirSynth b(99, CaTblCirConfig());
        CaTblCirSynth c(100, CaTblCirConfig());
        const CaTblCirResult ra = a.Classify(ENV_RURAL_FORESTED, true, 0.4, 25.0, 25.0);
        const CaTblCirResult rb = b.Classify(ENV_RURAL_FORESTED, true, 0.4, 25.0, 25.0);
        const CaTblCirResult rc = c.Classify(ENV_RURAL_FORESTED, true, 0.4, 25.0, 25.0);
        Check(ra.score == rb.score && ra.leTap == rb.leTap && ra.nRays == rb.nRays &&
                  (rc.nRays != ra.nRays || rc.score != ra.score),
              "the dedicated stream is deterministic, and a different stream differs");
    }
    {
        // Direct power, not the truth label, is what the detector reads: the same NLOS
        // multipath with an unobstructed strong direct ray reads LOS.
        CaTblCirSynth s(31, CaTblCirConfig());
        int low = 0;
        for (int i = 0; i < 200; ++i)
        {
            low += (s.Classify(ENV_RURAL_FORESTED, true, 5.0, 0.0, 30.0).score < 0.5) ? 1 : 0;
        }
        Check(low >= 190, "an NLOS draw with K = 5 and no obstruction loss reads LOS (>= 95%)");
    }

    // ------------------------------------------------ nominal path loss vs the engine
    {
        LoRaDistributionSampler sampler(1);
        PropagationParams sub = PropagationParams::DefaultsFor(ENV_SUBURBAN);
        sub.frequencyHz = 2.44e9;
        const double eng = ComputePathLossDb(Vector(0, 0, 0), Vector(250, 0, 0), 1.5, 1.5, sub,
                                             sampler);
        Check(Near(CaTblNominalPathLossDb(250.0, sub), eng, 1e-9),
              "nominal path loss equals ComputePathLossDb, single slope (suburban, 250 m)");
        PropagationParams urb = PropagationParams::DefaultsFor(ENV_URBAN_MODERATE);
        urb.frequencyHz = 2.44e9;
        const double engU = ComputePathLossDb(Vector(0, 0, 0), Vector(1500, 0, 0), 1.5, 1.5, urb,
                                              sampler);
        Check(Near(CaTblNominalPathLossDb(1500.0, urb), engU, 1e-9),
              "nominal path loss equals ComputePathLossDb, 3GPP UMa branch (1500 m)");
        PropagationParams ds = sub;
        ds.enableDualSlope = true;
        ds.dualSlopeBreakpointM = 400.0;
        ds.pathLossExponent2 = DistributionParam::Fixed(1.58);
        const double engD = ComputePathLossDb(Vector(0, 0, 0), Vector(900, 0, 0), 1.5, 1.5, ds,
                                              sampler);
        Check(Near(CaTblNominalPathLossDb(900.0, ds), engD, 1e-9),
              "nominal path loss equals ComputePathLossDb, dual slope beyond the break");
        const CaTblTofRssiResidual e = CaTblTofRssiExcess(250.0, -90.0, 10.0, sub);
        Check(e.valid && Near(e.excessDb, (10.0 - eng) - (-90.0), 1e-9) &&
                  !CaTblTofRssiExcess(0.0, -90.0, 10.0, sub).valid,
              "ToF/RSSI excess = (tx - PL(d_tof)) - RSSI; a zero range is not scored");
    }

    // ------------------------------------------------------------ robust reference
    {
        CaTblRobustRef ref(200, 20);
        double z = 0.0, med = 0.0, sig = 0.0;
        for (int v = 1; v <= 19; ++v)
        {
            ref.Add(v);
        }
        Check(!ref.Score(25.0, z, med, sig) && ref.Size() == 19,
              "NEGATIVE: a pool under minPool gives no score");
        for (int v = 20; v <= 21; ++v)
        {
            ref.Add(v);
        }
        // 1..21: median 11, |x - 11| = 0,1,1,..,10,10 -> MAD 5 -> sigma 7.413.
        const bool ok = ref.Score(25.0, z, med, sig);
        Check(ok && Near(med, 11.0, 1e-12) && Near(sig, 1.4826 * 5.0, 1e-12) &&
                  Near(z, 14.0 / (1.4826 * 5.0), 1e-12) && ref.Size() == 21,
              "median 11, sigma 1.4826 * MAD, z of 25; scoring does not add the value");
        CaTblRobustRef small(4, 3);
        for (int v = 0; v < 10; ++v)
        {
            small.Add(v);
        }
        Check(small.Size() == 4, "the pool keeps only its last `pool` values");
    }
    {
        Check(CaTblNlosRamp(0.5, 1.0, 3.0) == 0.0 && Near(CaTblNlosRamp(2.0, 1.0, 3.0), 0.5, 1e-15) &&
                  CaTblNlosRamp(3.0, 1.0, 3.0) == 1.0 && CaTblNlosRamp(-4.0, 1.0, 3.0) == 0.0 &&
                  CaTblNlosRamp(std::nan(""), 1.0, 3.0) == 0.0,
              "ramp: 0 at z <= 1, 0.5 at 2, 1 at >= 3; one-sided; NaN scores 0");
    }

    // --------------------------------------------------------------- von Neumann
    {
        CaTblLinkVariability v(8, 3);
        CaTblLinkVariability::Result r;
        for (int i = 0; i < 8; ++i)
        {
            r = v.Push(7, -100.0 + 2.0 * i, 10.0 + 2.0 * i); // a pure drift of 2 dB/reading
        }
        // Differences all 2: s^2 = 7*4 / (2*7) = 2. The sample SD of the same ramp is 4.9.
        Check(r.valid && r.n == 8 && Near(r.sRssiDb, std::sqrt(2.0), 1e-12),
              "von Neumann on a 2 dB/reading drift is sqrt(2), not the ramp's 4.9 dB SD");
        CaTblLinkVariability w(8, 3);
        CaTblLinkVariability::Result a = w.Push(1, -90.0, 5.0);
        a = w.Push(1, -80.0, 15.0);
        Check(!a.valid && a.n == 2, "NEGATIVE: two readings are under minReadings: no value");
        a = w.Push(1, -90.0, 5.0);
        // Alternating +-5 about -85: differences 10, s^2 = 2*100 / (2*2) = 50.
        Check(a.valid && Near(a.sRssiDb, std::sqrt(50.0), 1e-12) &&
                  Near(a.sSnrDb, std::sqrt(50.0), 1e-12) && !w.Push(2, -90.0, 5.0).valid,
              "alternating readings give sqrt(50); a new link starts its own window");
    }

    // ---------------------------------------------------- PCS term 3 hand-off
    {
        const std::vector<std::pair<double, double>> a = {{0, 0}, {800, 0}, {0, 800}, {800, 800}};
        const std::vector<double> scores = {0.1, 0.7, 0.0, 0.4};
        const std::vector<bool> tag = {true, false, true, true}; // tag LOS fraction 0.25
        const std::vector<double> none = {-1.0, -1.0, -1.0, -1.0};
        const double px = 300.0, py = 420.0;

        CaTblPolicyConfig off;
        off.mode = CA_TBL_MODE_NO_INTEGRITY;
        CaTblPolicyConfig on = off;
        on.pcsNlosFromScore = true;

        CaTblPolicy pOff, pOn, pNone, pOnNone;
        pOff.Configure(off);
        pOn.Configure(on);
        pNone.Configure(off);
        pOnNone.Configure(on);
        for (CaTblPolicy* p : {&pOff, &pOn, &pNone, &pOnNone})
        {
            p->Initialize(px + 30.0, py - 20.0, 100.0, 5.0);
        }
        const CaTblDecision dOff = pOff.Step(1.0, Meas(a, px, py, scores, tag), 0, 4, 4, 1u);
        const CaTblDecision dNone = pNone.Step(1.0, Meas(a, px, py, none, tag), 0, 4, 4, 1u);
        const CaTblDecision dOn = pOn.Step(1.0, Meas(a, px, py, scores, tag), 0, 4, 4, 1u);
        const CaTblDecision dOnNone = pOnNone.Step(1.0, Meas(a, px, py, none, tag), 0, 4, 4, 1u);
        const double term = (0.9 + 0.3 + 1.0 + 0.6) / 4.0; // mean(1 - score) = 0.7
        Check(dOff.pcs == dNone.pcs && !dOff.nlosScoreFed && dOff.nNlosScored == 4 &&
                  Near(dOff.nlosScoreTerm, term, 1e-15),
              "NEGATIVE: flag off, scored links change nothing; the term is still observed");
        Check(dOn.nlosScoreFed && Near(dOn.pcs - dOff.pcs, term - 0.25, 1e-12) &&
                  dOn.traceSigma == dOff.traceSigma && dOn.gdop == dOff.gdop,
              "LEGACY: PCS moves by exactly term - tag fraction (0.70 - 0.25); nothing else moves");
        Check(!dOnNone.nlosScoreFed && dOnNone.pcs == dNone.pcs && dOnNone.nNlosScored == 0 &&
                  dOnNone.nlosScoreTerm == -1.0,
              "NEGATIVE: flag on with no scored link falls back to the tag, byte for byte");

        CaTblPolicyConfig wOff = off;
        wOff.pcsForm = CA_TBL_PCS_WEIGHTED;
        CaTblPolicyConfig wOn = wOff;
        wOn.pcsNlosFromScore = true;
        CaTblPolicy qOff, qOn;
        qOff.Configure(wOff);
        qOn.Configure(wOn);
        qOff.Initialize(px + 30.0, py - 20.0, 100.0, 5.0);
        qOn.Initialize(px + 30.0, py - 20.0, 100.0, 5.0);
        const CaTblDecision eOff = qOff.Step(1.0, Meas(a, px, py, scores, tag), 0, 4, 4, 1u);
        const CaTblDecision eOn = qOn.Step(1.0, Meas(a, px, py, scores, tag), 0, 4, 4, 1u);
        Check(Near(eOn.pcs - eOff.pcs, wOn.pcsW3 * (term - 0.25), 1e-12),
              "WEIGHTED: PCS moves by w3 * (term - tag fraction); the weights are untouched");

        CaTblPolicyConfig vOn = off;
        vOn.pcsForm = CA_TBL_PCS_V2;
        vOn.ApplyV2Thresholds();
        vOn.pcsNlosFromScore = true;
        CaTblPolicyConfig vOff = vOn;
        vOff.pcsNlosFromScore = false;
        CaTblPolicy rOn, rOff;
        rOn.Configure(vOn);
        rOff.Configure(vOff);
        rOn.Initialize(px + 30.0, py - 20.0, 100.0, 5.0);
        rOff.Initialize(px + 30.0, py - 20.0, 100.0, 5.0);
        const CaTblDecision fOn = rOn.Step(1.0, Meas(a, px, py, scores, tag), 0, 4, 4, 1u);
        const CaTblDecision fOff = rOff.Step(1.0, Meas(a, px, py, scores, tag), 0, 4, 4, 1u);
        Check(!fOn.nlosScoreFed && fOn.pcs == fOff.pcs,
              "NEGATIVE: v2 has no NLOS term; the flag feeds nothing there");
    }

    // ------------------------------------------ characterisation (not a scenario run)
    //
    // Link states drawn the way the engine draws them (lora-physics-parameters.cc: LOS K
    // from the class's Rician distribution, NLOS K ~ N(kNlosMean, 1) >= 0; an obstruction
    // loss near the UWB NLOS value, both terms carrying the +9.64 dB scaling), SNR_acc
    // uniform over the 12-40 dB the scenario's decoded UWB links span.
    //
    // FIRST RUN, recorded because it is a finding and not a tuning target: outdoor (CM6)
    // NLOS came out tp 157 fn 277. With the direct ray 30 dB down it sits under the noise
    // threshold, so the first DETECTABLE path is the delayed first cluster's own first ray
    // -- the strongest expected ray of that cluster, since CM6 has no rise -- and it is
    // often within 6 dB of the peak. That is the undetected-direct-path case the ratio test
    // is known to miss: the fix it produces is biased and the CIR still looks clean. The
    // office set (CM4, eq. 22) attenuates the first component by (1 - chi) and rises, and
    // was expected to catch more; measured, it does not (tp 132 fn 294), because its rising
    // edge is itself under the noise threshold at most of these SNRs and the first tap that
    // clears it is already near the peak. Both sets call LOS almost perfectly (fp 0 and 5).
    // The check asserts what holds in both: both classes are reached, and an NLOS link is
    // called NLOS more often than a LOS link is.
    for (int envCase = 0; envCase < 2; ++envCase)
    {
        const bool office = (envCase == 1);
        std::mt19937_64 g(20260929 + envCase);
        std::normal_distribution<double> nk(office ? 2.0 : 3.0, 1.0);
        std::normal_distribution<double> nn(office ? 0.0 : 0.5, 1.0);
        std::normal_distribution<double> nl(office ? 28.0 : 30.0, office ? 6.0 : 7.0);
        std::uniform_real_distribution<double> us(12.0, 40.0);
        CaTblCirSynth s(555 + envCase, CaTblCirConfig());
        uint64_t tp = 0, fp = 0, fn = 0, tn = 0, un = 0;
        for (int i = 0; i < 1000; ++i)
        {
            const bool nlos = (i % 2) == 1;
            const double kLo = office ? 0.0 : 1.0;
            const double kHi = office ? 5.0 : 6.0;
            const double k = nlos ? std::max(0.0, nn(g)) : std::min(kHi, std::max(kLo, nk(g)));
            const double loss = nlos ? std::max(0.0, nl(g)) : 0.0;
            const CaTblCirResult r = s.Classify(office ? ENV_INDOOR_OFFICE : ENV_RURAL_FORESTED,
                                                nlos, k, loss, us(g));
            if (r.score < 0.0)
            {
                ++un;
                continue;
            }
            const bool v = r.score >= 0.5;
            tp += (v && nlos);
            fp += (v && !nlos);
            fn += (!v && nlos);
            tn += (!v && !nlos);
        }
        std::cout << "  characterisation " << (office ? "CM3/CM4 office" : "CM5/CM6 outdoor")
                  << ", 500 LOS + 500 NLOS engine-like states: tp " << tp << " fp " << fp
                  << " fn " << fn << " tn " << tn << " unscored " << un << "\n";
        const double tpr = (tp + fn) ? static_cast<double>(tp) / (tp + fn) : 0.0;
        const double fpr = (fp + tn) ? static_cast<double>(fp) / (fp + tn) : 1.0;
        Check(tp > 0 && fn + tn > 0 && tp + fn > 0 && fp + tn > 0 && tpr > fpr,
              office ? "office: both classes reached; P(NLOS verdict | NLOS) > P(... | LOS)"
                     : "outdoor: both classes reached; P(NLOS verdict | NLOS) > P(... | LOS)");
    }

    std::cout << "\n  " << g_pass << " passed, " << g_fail << " failed\n";
    return (g_fail == 0) ? 0 : 1;
}
