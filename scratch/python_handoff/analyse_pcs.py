#!/usr/bin/env python3
"""
analyse_pcs.py -- confidence-model quality, per the pre-registration in
report/PCS_REDESIGN_RESULTS.md §0.4 / §0.4a / §0.6.

WHY THIS FILE EXISTS
--------------------
Report §6.8 quotes rho(PCS, error) = -0.091 and rho(traceSigma, error) = +0.779
as the measurements motivating the whole PCS redesign. Neither is reproducible:
scripts/make_report_figures.py:501 and :508 set those values as LITERAL STRINGS
in matplotlib axis titles, and no script in the repository computes a rank
correlation over trace data. The population is gone too -- §6.8 cites 28,798 fix
cycles but no 20-seed trace set survives on disk.

So this is not a convenience wrapper. It is the missing measurement.

ESTIMATOR, FIXED IN ADVANCE
---------------------------
  * Spearman rank correlation (the report says "rank correlation"; Pearson on
    raw errM would be dominated by the heavy tail that §4.1 already flags).
  * Fix cycles only (hadFix == 1). A no-fix cycle has no realised fix to
    correlate against; including them would score dead reckoning.
  * Seed-level bootstrap for confidence intervals. SEEDS are the independent
    unit -- cycles within a seed share one channel realisation and one
    trajectory, so a cycle-level bootstrap would report an interval several times
    too narrow. This is the single most common way a CI in this kind of study is
    wrong.

  python3 scratch/python_handoff/analyse_pcs.py results/DIR [results/DIR2 ...]
"""

import sys
import os
import csv
import glob
import math
import random


# --------------------------------------------------------------- statistics --

def _rank(v):
    """Fractional ranks, ties averaged."""
    idx = sorted(range(len(v)), key=lambda i: v[i])
    r = [0.0] * len(v)
    i = 0
    while i < len(idx):
        j = i
        while j + 1 < len(idx) and v[idx[j + 1]] == v[idx[i]]:
            j += 1
        avg = (i + j) / 2.0 + 1.0
        for k in range(i, j + 1):
            r[idx[k]] = avg
        i = j + 1
    return r


def pearson(a, b):
    n = len(a)
    if n < 3:
        return float("nan")
    ma, mb = sum(a) / n, sum(b) / n
    va = sum((x - ma) ** 2 for x in a)
    vb = sum((y - mb) ** 2 for y in b)
    if va <= 0 or vb <= 0:
        return float("nan")
    return sum((x - ma) * (y - mb) for x, y in zip(a, b)) / math.sqrt(va * vb)


def spearman(a, b):
    return pearson(_rank(a), _rank(b))


def spearman_ci(by_seed, xkey, ykey, n_boot=2000, seed=12345):
    """Seed-level bootstrap CI for Spearman rho.

    by_seed: {seed: [(x, y), ...]}. Resamples SEEDS with replacement, pools the
    cycles of the drawn seeds, recomputes rho. With one seed the interval is
    undefined and is reported as such rather than as a spuriously tight number.
    """
    seeds = sorted(by_seed)
    pooled = [p for s in seeds for p in by_seed[s]]
    if len(pooled) < 3:
        return float("nan"), float("nan"), float("nan"), len(pooled)
    point = spearman([p[0] for p in pooled], [p[1] for p in pooled])
    if len(seeds) < 2:
        return point, float("nan"), float("nan"), len(pooled)

    rng = random.Random(seed)
    boots = []
    for _ in range(n_boot):
        drawn = [rng.choice(seeds) for _ in seeds]
        samp = [p for s in drawn for p in by_seed[s]]
        if len(samp) < 3:
            continue
        r = spearman([p[0] for p in samp], [p[1] for p in samp])
        if r == r:
            boots.append(r)
    if len(boots) < 20:
        return point, float("nan"), float("nan"), len(pooled)
    boots.sort()
    lo = boots[int(0.025 * len(boots))]
    hi = boots[min(int(0.975 * len(boots)), len(boots) - 1)]
    return point, lo, hi, len(pooled)


def variance(v):
    n = len(v)
    if n < 2:
        return 0.0
    m = sum(v) / n
    return sum((x - m) ** 2 for x in v) / (n - 1)


# ------------------------------------------------------------------- loading --

def load_dir(d):
    """{arm: {seed: [row, ...]}} over fix cycles only."""
    out = {}
    for p in sorted(glob.glob(os.path.join(d, "trace_*.csv"))):
        base = os.path.basename(p)[len("trace_"):-len(".csv")]
        arm, _, seed = base.rpartition("_seed")
        with open(p, newline="") as f:
            rows = [r for r in csv.DictReader(f) if int(r["hadFix"]) == 1]
        if rows:
            out.setdefault(arm, {})[seed] = rows
    return out


def derived(rows):
    """Signals the score is built from, plus the pre-registered controls.

    sigma_post is sqrt(traceSigma) -- the same quantity traceSigma ranks by, so
    its rho is identical up to sign; it is listed separately because it is the
    MONOTONE-OF-TRACESIGMA CONTROL the pre-registration requires v2 to be judged
    against. Criterion 2 (rho near -0.78) is trivially satisfiable by any
    monotone decreasing function of traceSigma alone, which is precisely what
    fixed_threshold already thresholds -- so v2 beating zero proves nothing and
    v2 beating THIS is the real test.
    """
    sig = {}
    ts = [float(r["traceSigma"]) for r in rows]
    sig["traceSigma"] = ts
    sig["pcs"] = [float(r["pcs"]) for r in rows]
    sig["gdop"] = [float(r["gdop"]) for r in rows]
    sig["ctrl_negLogSigmaPost"] = [-math.log10(math.sqrt(max(x, 1e-12))) for x in ts]
    if "meanSigmaM" in rows[0]:
        sig["meanSigmaM"] = [float(r["meanSigmaM"]) for r in rows]
    if "nNlosPropagation" in rows[0]:
        sig["losFrac"] = [
            (int(r["nAnchors"]) - int(r["nNlos"])) / max(int(r["nAnchors"]), 1)
            for r in rows]
    return sig


SIGMA_REF = math.sqrt(10.0)  # sqrt(traceRef) = 3.1623 m, the SPECIFICATION


def v2_terms(rows):
    """The two v2 terms separately, for the §0.4a per-term decomposition.

    Returns None where sigmaGeoM was never computed (legacy form) so the caller
    can skip rather than silently decompose a column of -1.
    """
    if "sigmaGeoM" not in rows[0]:
        return None
    usable = [r for r in rows if float(r["sigmaGeoM"]) > 0.0]
    if len(usable) < 3:
        return None
    t_post, t_geo, pcs, err = [], [], [], []
    for r in usable:
        sp = math.sqrt(max(float(r["traceSigma"]), 1e-12))
        sg = float(r["sigmaGeoM"])
        t_post.append(math.log10(SIGMA_REF / sp))
        t_geo.append(math.log10(SIGMA_REF / sg))
        pcs.append(float(r["pcs"]))
        err.append(float(r["errM"]))
    return {"term_post": t_post, "term_geo": t_geo,
            "pcs": pcs, "errM": err, "n": len(usable), "n_total": len(rows)}


def report_v2_terms(arm, rows):
    """Per-term variance shares, with the covariance so the decomposition closes.

    A sum can correlate well because ONE component does all the work. Reporting
    only rho of the sum would hide a one-term score wearing decoration -- which
    is precisely the failure the fixed_threshold comparison exists to expose. A
    v2 whose variance is overwhelmingly term_post is the fixed_threshold
    statistic on a log scale, and is reported as such.
    """
    t = v2_terms(rows)
    if t is None:
        return
    vp, vg = variance(t["term_post"]), variance(t["term_geo"])
    n = t["n"]
    mp = sum(t["term_post"]) / n
    mg = sum(t["term_geo"]) / n
    cov = sum((a - mp) * (b - mg)
              for a, b in zip(t["term_post"], t["term_geo"])) / (n - 1)
    vsum = variance([a + b for a, b in zip(t["term_post"], t["term_geo"])])

    print(f"      -- v2 term decomposition ({n}/{t['n_total']} cycles with a "
          f"valid weighted solve)")
    if vsum > 0:
        print(f"         Var(term_post) {vp:9.4g}  share {vp / vsum:6.1%}   "
              f"rho(term_post,err) {spearman(t['term_post'], t['errM']):+.3f}")
        print(f"         Var(term_geo)  {vg:9.4g}  share {vg / vsum:6.1%}   "
              f"rho(term_geo ,err) {spearman(t['term_geo'], t['errM']):+.3f}")
        print(f"         2*Cov          {2 * cov:9.4g}  share {2 * cov / vsum:6.1%}"
              f"   (shares sum to 100% by construction)")
        dom = max(vp, vg) / vsum
        if dom > 0.90:
            which = "term_post" if vp > vg else "term_geo"
            print(f"         WARNING: {dom:.1%} of the score's variance is {which} "
                  f"alone -- this is a one-term score with decoration")
    print(f"         rho(pcs_sum,err) {spearman(t['pcs'], t['errM']):+.3f}   "
          f"distinct term_post {len(set(round(x, 9) for x in t['term_post']))}  "
          f"term_geo {len(set(round(x, 9) for x in t['term_geo']))}")


# ------------------------------------------------------------------ reporting --

def report_dir(d):
    print("=" * 78)
    print(f"DIRECTORY {d}")
    ridp = os.path.join(d, "run_id.txt")
    if os.path.exists(ridp):
        with open(ridp) as f:
            lines = f.read().splitlines()
        print(f"  run_id     {lines[0] if lines else '?'}")
        print(f"  invocation {lines[1] if len(lines) > 1 else '?'}")
    data = load_dir(d)
    if not data:
        print("  no traces")
        return
    nseeds = len({s for a in data.values() for s in a})
    print(f"  arms {len(data)}  seeds {nseeds}")
    if nseeds < 2:
        print("  NOTE: one seed -- bootstrap CIs undefined, reported as [-- , --].")
    print()

    for arm in sorted(data):
        per_seed = data[arm]
        allrows = [r for s in sorted(per_seed) for r in per_seed[s]]
        sig_all = derived(allrows)
        ncyc = len(allrows)

        # Escalation rate per fix cycle -- reported BESIDE every rho, because a
        # score can improve its correlation while still escalating on ~96% of
        # cycles, and if it does the energy conclusion does not move.
        esc = sum(int(r["escalated"]) for r in allrows)
        desc = sum(int(r["deescalated"]) for r in allrows)
        susp = sum(int(r["suspect"]) for r in allrows)

        print(f"--- {arm}   fix cycles {ncyc}   seeds {len(per_seed)}")
        print(f"    escalations/fix cycle {esc / ncyc:.4f} ({esc})   "
              f"deescalations {desc / ncyc:.4f}   suspects {susp / ncyc:.4f} ({susp})")

        for name in ("pcs", "traceSigma", "gdop", "ctrl_negLogSigmaPost",
                     "meanSigmaM", "losFrac"):
            if name not in sig_all:
                continue
            by_seed = {}
            for s in sorted(per_seed):
                sg = derived(per_seed[s])
                by_seed[s] = list(zip(sg[name],
                                      [float(r["errM"]) for r in per_seed[s]]))
            rho, lo, hi, n = spearman_ci(by_seed, name, "errM")
            v = sig_all[name]
            nuniq = len(set(round(x, 6) for x in v))
            ci = f"[{lo:+.3f}, {hi:+.3f}]" if lo == lo else "[--    , --    ]"
            flag = "  <-- CONSTANT" if nuniq == 1 else ""
            print(f"      {name:22s} rho {rho:+.3f} {ci}  "
                  f"var {variance(v):9.4g}  distinct {nuniq:5d}{flag}")
        report_v2_terms(arm, allrows)
        print()


def main():
    dirs = sys.argv[1:] or ["results/causal"]
    print(__doc__.split("ESTIMATOR")[0].strip())
    print()
    for d in dirs:
        report_dir(d)


if __name__ == "__main__":
    main()
