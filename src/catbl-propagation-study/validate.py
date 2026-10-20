#!/usr/bin/env python3
"""§7 validation: an INDEPENDENTLY COMPUTED link budget against the engines.

WHAT CHANGED, AND WHY IT MATTERS
--------------------------------
The previous version read `mean_margin_db` from the probe CSV and applied a shadowing
distribution to it. The link budget -- the quantity this file exists to validate --
was imported from the subject, so no engine constant could move the output: detuning
the noise constant by 14 dB and a tier's TX power by 16 dB both produced BYTE-IDENTICAL
results. A comparison run against such a model tests nothing.

The model now lives in linkbudget.py, which imports only `math`. It cannot see the
probe. This file reads the probe for COMPARISON ONLY: the probe's margin column is
loaded into `probe_margin`, which is never passed to the model.

REGISTERED ACCEPTANCE, stated before the comparison is computed:
    r80 within 15%, PDR within 3 dB across the swept range, all three tiers.
Unchanged, and not relaxed after any failure.
"""
import csv, hashlib, json, math, os, sys

import linkbudget as lb
import ranging

HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS = os.path.join(HERE, "results", "corpus.json")
# Repointed to the post-D1 probe. results/causal/probe_range_sweep.csv is pre-D1 and
# superseded; the module's own DEFECTS.md records results/probe_audit as invalidated.
# Repointed again 2026-10-20 (TECH_DEBT R.1, VAL-V8): probe_v7 (Aug 27) predates 50fc8620, which made tiers 2/3
# follow the wide-area environment and vegetation, so from 2026-09-08 this file reconciled the model with an engine
# configuration no run used. probe_v8 is G11's invocation at today's defaults, and TIER_OVERRIDES puts the model's
# tiers 2/3 where the engine's defaults put them. Every value is the engine's declared default; none is fitted.
PROBE = os.path.join(HERE, "..", "..", "results", "probe_v8", "probe_range_sweep.csv")
TIER_OVERRIDES = {"868MHz": {},
                  "2.4GHz": {"env": "RURAL_FORESTED", "veg": dict(lb.VEGETATION["VEG_DENSE_FOREST"])},
                  "UWB": {"env": "RURAL_FORESTED", "veg": dict(lb.VEGETATION["VEG_DENSE_FOREST"])}}

R80_TOL = 0.15        # registered, UNCHANGED
MARGIN_TOL_DB = 3.0   # registered "3 dB", applied to the quantity that is in dB

# THE PDR CRITERION WAS NEVER WELL-DEFINED, AND ITS FIRST IMPLEMENTATION WAS AN ARTEFACT
# ---------------------------------------------------------------------------------
# §7 registered "within 3 dB on PDR". PDR is a probability on [0,1]; "3 dB" on a
# probability requires a stated transformation and §7 gave none.
#
# The first implementation used a log-ratio, |10log10(p_model) - 10log10(p_engine)|,
# floored at 1e-6. That is unbounded as either side approaches zero, and it produced
# 52.07 dB for 2.4 GHz -- ENTIRELY from the single 6000 m point where the engine PDR is
# 0.0000 (clamped to 1e-6, -60 dB) against a model PDR of 0.1612 (-7.93 dB). The tier
# with the SMALLER margin error had a PDR error twelve times larger than the tier with
# the larger one; no physical mechanism does that, and a log-ratio near zero does.
#
# Candidates considered, all computed before choosing so the choice is not a selection
# on outcome:
#     (a) absolute difference in probability     868 0.125  2.4 0.320  UWB 0.192
#     (b) log-ratio inside 0.05<=PDR<=0.95       still 52.07 -- the band test admitted
#                                                a point when only ONE side was in band
#     (c) r80 and transition width               r80 already registered separately
#     (d) |computed margin - engine margin|, dB  868 4.10   2.4 19.91  UWB 2.05
#
# CHOSEN: (d), and NOT because it is smallest -- (a) is smaller in absolute terms and
# (d) fails two tiers. It is chosen because:
#   * "3 dB" is natively a dB quantity. A margin is in dB; a probability is not.
#   * the margin IS the link budget, which is the thing §7 exists to validate.
#   * it is bounded and defined at every distance, including where PDR saturates at
#     0 or 1 and a ratio becomes meaningless.
#   * it is only computable because the model now computes a margin. The previous
#     implementation could not have used it, which is why it reached for a ratio.
# The 15% on r80 half of the criterion is well-defined and stays exactly as registered.


def load_corpus():
    if not os.path.exists(CORPUS):
        raise SystemExit("FATAL: corpus absent at %s -- run corpus.py first" % CORPUS)
    d = json.load(open(CORPUS))
    h = hashlib.sha256()
    with open(CORPUS, "rb") as f:
        for c in iter(lambda: f.read(1 << 20), b""):
            h.update(c)
    draws = [(r["anchors"][0]["u_shadow"], r["anchors"][0]["u_nlos"],
              r["anchors"][0]["u_excess"], r["anchors"][0]["u_fade"])
             for r in d["realizations"]]
    return draws, h.hexdigest(), d["n"]


def load_probe_for_comparison():
    """Engine output, for comparison ONLY.

    Returns per tier: [(distance, engine_pdr, probe_margin)]. `probe_margin` is
    carried so it can be COMPARED against the computed margin; it is never handed to
    linkbudget.py, which cannot accept it -- the model's signature takes distance and
    corpus draws, not a margin.
    """
    rows = list(csv.DictReader(open(PROBE)))
    out = {}
    for t in lb.TIER_PHY:
        out[t] = sorted([(float(r["distance_m"]), float(r["pdr"]),
                          float(r["mean_margin_db"]))
                         for r in rows if r["tier"] == t])
    return out


def db(x):
    return 10.0 * math.log10(max(x, 1e-9))


def main():
    draws, digest, n = load_corpus()
    probe = load_probe_for_comparison()

    print("=" * 78)
    print("REGISTERED ACCEPTANCE, stated before the comparison:")
    print("  r80 within %.0f%% (unchanged)   margin within %.1f dB across the swept range" %
          (100 * R80_TOL, MARGIN_TOL_DB))
    print("  the dB half is applied to the MARGIN, not to PDR -- see the note in this file")
    print("  population: %d corpus realizations, digest %s" % (n, digest[:16]))
    print("  probe: %s" % os.path.relpath(os.path.realpath(PROBE), os.path.dirname(HERE)))
    print("  model: linkbudget.py, which imports only `math` and cannot read the probe")
    print("=" * 78)
    print()

    allpass = True
    for t in ("868MHz", "2.4GHz", "UWB"):
        pts = probe[t]
        dists = [p[0] for p in pts]
        o = TIER_OVERRIDES[t]
        env = o.get("env", lb.TIER_ENV[t])
        print("  --- %s   (%s, n=%.2f)" % (t, env, lb.ENV[env]["n"]))
        print("    %-8s %9s %9s | %11s %11s %9s" %
              ("dist", "pdr_eng", "pdr_mdl", "margin_eng", "margin_mdl", "gap_dB"))
        worst_db = 0.0
        for (d, pe, m_probe) in pts:
            pm = lb.pdr_at(t, d, draws, o)
            # mean computed margin over the corpus, for comparison against the probe's
            mm = sum(lb.margin_db(t, d, u_nl, u_at, u_df, u_df, o)
                     for (_, u_nl, u_at, u_df) in draws) / len(draws)
            worst_db = max(worst_db, abs(mm - m_probe))
            print("    %-8.0f %9.4f %9.4f | %11.2f %11.2f %+9.2f"
                  % (d, pe, pm, m_probe, mm, mm - m_probe))
        rm = lb.r80(t, draws, dists, o)
        re_ = None
        prev = None
        for (d, pe, _) in pts:
            if prev is not None and prev[1] >= 0.80 > pe:
                re_ = prev[0] + (prev[1] - 0.80) / (prev[1] - pe) * (d - prev[0])
                break
            prev = (d, pe)
        rel = abs(rm - re_) / re_ if (re_ and rm == rm) else float("nan")
        ok_r = rel == rel and rel <= R80_TOL
        ok_p = worst_db <= MARGIN_TOL_DB
        allpass = allpass and ok_r and ok_p
        print("    r80  model %8.1f   engine %8.1f   rel %6.1f%%  %s"
              % (rm, re_ if re_ else float("nan"), 100 * rel, "PASS" if ok_r else "FAIL"))
        print("    margin worst deviation %.2f dB  %s" % (worst_db, "PASS" if ok_p else "FAIL"))
        print()

    print("  ACCEPTANCE: %s" % ("PASS" if allpass else "FAIL"))
    if not allpass:
        print("  The model is NOT tuned to match. Report which term differs and which")
        print("  side is more likely wrong.")
    return 0 if allpass else 1


if __name__ == "__main__":
    sys.exit(main())
