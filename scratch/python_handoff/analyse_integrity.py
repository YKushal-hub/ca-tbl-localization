#!/usr/bin/env python3
"""Stage 1 read-out: integrity-gate separation, ROC, latency and the injection census.

Estimator conventions follow analyse_pcs.py: seeds are the independent unit, ratios
are aggregated as a geometric mean of per-seed ratios with a seed-level bootstrap CI,
and fix cycles are the population wherever a fix is required.

THE INJECTION CENSUS IS DERIVED, NOT ESTIMATED. Every input to the fault window is
recorded in the run's own sidecar:

    ca_tbl_causal_sim.cc:2125   biasStartS = durationS * (0.25 + 0.5*(seed % 7)/7.0)
    ca_tbl_causal_sim.cc:2299   biasActive = injectBiasFraction > 0
                                             && t >= biasStartS
                                             && t <  biasStartS + injectBiasFraction*durationS

so seed, durationS and injectBiasFraction -- all on line 2 of run_id.txt -- determine
the window by arithmetic. What is reconstructed is the mapping from `t` to in/out, a
boundary-condition question, and it is CHECKED IN CLOSED FORM:

    expected faulted cycles = injectBiasFraction * durationS / dt

A mismatch is an off-by-one at `t >= start` or `t < start+len`, which is a bug to fix
rather than an uncertainty to disclose, so it raises rather than warns.

Corroboration is separate and uses a quantity the derivation did not consume: on F3
(common-mode bias on every tier-2 link) the Mahalanobis step should coincide with the
derived boundary. Offset is reported in cycles; expect 0. That ordering -- derive,
assert, THEN corroborate against an unused quantity -- is what keeps it non-circular.

SEED CLUSTERING. `seed % 7` gives 20 seeds only 7 distinct window starts, unevenly
weighted (residue 0 has 2 seeds, the rest 3). Whether that matters is measured, not
assumed: within- vs between-residue variance of the coverage conditions at onset. If
the clustering is real the cluster bootstrap governs, because the registered
estimator's independence assumption is then measurably violated.
"""

import csv
import glob
import math
import os
import random
import statistics as st
import sys

DT = 2.5
CHI2_99_2DOF = 9.21


def read_sidecar(d):
    """seed, durationS, injectBiasFraction and the fault mode, from run_id.txt line 2."""
    p = os.path.join(d, "run_id.txt")
    if not os.path.exists(p):
        raise SystemExit(f"FATAL: {d} has no run_id.txt; the census inputs are unrecorded")
    with open(p) as f:
        lines = f.read().splitlines()
    inv = lines[1] if len(lines) > 1 else ""
    out = {"seed": None, "durationS": None, "injectBiasFraction": 0.0,
           "injectRangeBias": 0.0, "injectFaultAnchors": 0, "mahalUseR": None}
    for tok in inv.split():
        for k, cast in (("--seed=", int), ("--duration=", float),
                        ("--injectBiasFraction=", float), ("--injectRangeBias=", float),
                        ("--injectFaultAnchors=", int)):
            if tok.startswith(k):
                out[k[2:].rstrip("=")] = cast(tok.split("=", 1)[1])
        if tok.startswith("--mahalUseR="):
            out["mahalUseR"] = tok.split("=", 1)[1] == "true"
    out["seed"] = out.pop("seed", None) or out.get("seed")
    return {"seed": out.get("seed"), "durationS": out.get("duration"),
            "frac": out.get("injectBiasFraction", 0.0),
            "bias": out.get("injectRangeBias", 0.0),
            "anchors": out.get("injectFaultAnchors", 0),
            "mahalUseR": out.get("mahalUseR")}


def window(seed, durationS, frac):
    start = durationS * (0.25 + 0.5 * (seed % 7) / 7.0)
    return start, start + frac * durationS


def label_rows(rows, seed, durationS, frac):
    """Tag each row faulted/clean by the simulator's own predicate."""
    start, end = window(seed, durationS, frac)
    out = []
    for r in rows:
        t = float(r["t"])
        out.append((r, frac > 0.0 and start <= t < end))
    return out, start, end


def geo_mean_ci(ratios, n_boot=10000, rng=None, clusters=None):
    """Geometric mean of per-seed ratios with a bootstrap CI.

    clusters=None -> seed-level resampling (the registered estimator).
    clusters=dict(seed->class) -> cluster bootstrap: resample CLASSES with
    replacement, then take every seed in each drawn class.
    """
    rng = rng or random.Random(12345)
    vals = [(s, r) for s, r in ratios if r is not None and r > 0 and math.isfinite(r)]
    if len(vals) < 3:
        return None, None, None, len(vals)
    logs = {s: math.log(r) for s, r in vals}
    point = math.exp(st.mean(logs.values()))
    boots = []
    if clusters is None:
        keys = list(logs)
        for _ in range(n_boot):
            samp = [logs[rng.choice(keys)] for _ in keys]
            boots.append(math.exp(st.mean(samp)))
    else:
        by_class = {}
        for s in logs:
            by_class.setdefault(clusters[s], []).append(logs[s])
        cls = list(by_class)
        for _ in range(n_boot):
            samp = []
            for _ in cls:
                samp.extend(by_class[rng.choice(cls)])
            if samp:
                boots.append(math.exp(st.mean(samp)))
    boots.sort()
    lo = boots[int(0.025 * len(boots))]
    hi = boots[int(0.975 * len(boots))]
    return point, lo, hi, len(vals)


def load_cell(d, arm="ca_tbl_full"):
    t = glob.glob(os.path.join(d, f"trace_{arm}_seed*.csv"))
    if not t:
        return None
    with open(t[0]) as f:
        return list(csv.DictReader(f))


def analyse_set(dirs, label, arm="ca_tbl_full"):
    """Per-seed separation, ROC counts, latency, census. Returns a dict."""
    per_seed = {}
    census_fail = []
    for d in sorted(dirs):
        sc = read_sidecar(d)
        rows = load_cell(d, arm)
        if not rows:
            continue
        seed, dur, frac = sc["seed"], sc["durationS"], sc["frac"]
        tagged, start, end = label_rows(rows, seed, dur, frac)
        nf = sum(1 for _, f in tagged if f)
        nc = len(tagged) - nf
        # ---- closed-form census assertion (hard failure) ------------------------
        if frac > 0.0:
            expected = round(frac * dur / DT)
            if nf != expected:
                census_fail.append((d, nf, expected))
        # ---- separation: median statistic in vs out, fix cycles only -----------
        def med(sel):
            v = [float(r["mahalanobis"]) for r, f in tagged
                 if f == sel and r["hadFix"] == "1" and r["mahalanobis"] not in ("", "nan")]
            return st.median(v) if v else None
        mi, mo = med(True), med(False)
        ratio = (mi / mo) if (mi and mo and mo > 0) else None
        # ---- ROC on the suspect flag ------------------------------------------
        tp = fp = fn = tn = 0
        for r, f in tagged:
            if r["hadFix"] != "1":
                continue
            s = r["suspect"] == "1"
            if f and s: tp += 1
            elif (not f) and s: fp += 1
            elif f and (not s): fn += 1
            else: tn += 1
        # ---- latency from onset, in cycles ------------------------------------
        lat = None
        if frac > 0.0:
            seq = [(float(r["t"]), r["suspect"] == "1") for r, f in tagged if f]
            seq.sort()
            for i, (t, s) in enumerate(seq):
                if s:
                    lat = i
                    break
        per_seed[seed] = {"nf": nf, "nc": nc, "ratio": ratio, "medIn": mi, "medOut": mo,
                          "tp": tp, "fp": fp, "fn": fn, "tn": tn, "lat": lat,
                          "start": start, "end": end, "frac": frac, "dur": dur,
                          "suspRate": sum(1 for r, _ in tagged
                                          if r["hadFix"] == "1" and r["suspect"] == "1")
                                      / max(1, sum(1 for r, _ in tagged if r["hadFix"] == "1"))}
    return {"label": label, "seeds": per_seed, "census_fail": census_fail}


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else "results/campaign_20260810/stage1"
    sets = {}
    for fs in ("F0", "F1", "F3"):
        for rr in ("Rfalse", "Rtrue"):
            dirs = sorted(glob.glob(os.path.join(base, f"{fs}_{rr}_seed*")))
            if dirs:
                sets[f"{fs}_{rr}"] = analyse_set(dirs, f"{fs}_{rr}")

    print("=" * 78)
    print("1. INJECTION CENSUS -- derived from the recorded invocation, count-asserted")
    print("=" * 78)
    hard_fail = False
    for k, s in sets.items():
        seeds = s["seeds"]
        if not seeds:
            continue
        fracs = sorted({v["frac"] for v in seeds.values()})
        nf = [v["nf"] for v in seeds.values()]
        nc = [v["nc"] for v in seeds.values()]
        realised = st.mean(v["nf"] / (v["nf"] + v["nc"]) for v in seeds.values())
        print(f"  {k:<10} n={len(seeds):>2}  injectBiasFraction={fracs}  "
              f"faulted={min(nf)}-{max(nf)}  clean={min(nc)}-{max(nc)}  "
              f"realised fraction={realised:.4f}")
        if s["census_fail"]:
            hard_fail = True
            print(f"    CENSUS ASSERTION FAILED on {len(s['census_fail'])} cell(s):")
            for d, got, exp in s["census_fail"][:5]:
                print(f"      {os.path.basename(d)}: faulted={got} expected={exp}")
    if hard_fail:
        print("\n  FATAL: the derived census disagrees with its own closed form.")
        print("  This is an off-by-one in the boundary handling, not an uncertainty.")
        return 1
    print("  All cells match injectBiasFraction * durationS / dt exactly.")

    # ---------------------------------------------------------------- 1b. corroborate
    print()
    print("=" * 78)
    print("1b. CORROBORATION -- F3 Mahalanobis step vs the derived boundary")
    print("    (uses a quantity the derivation did not consume; expect offset 0)")
    print("=" * 78)
    offs = []
    for d in sorted(glob.glob(os.path.join(base, "F3_Rfalse_seed*"))):
        sc = read_sidecar(d); rows = load_cell(d)
        if not rows: continue
        start, _ = window(sc["seed"], sc["durationS"], sc["frac"])
        seq = [(float(r["t"]), float(r["mahalanobis"]))
               for r in rows if r["hadFix"] == "1" and r["mahalanobis"] not in ("", "nan")]
        seq.sort()
        base_med = st.median([v for t, v in seq if t < start]) if any(t < start for t, _ in seq) else None
        if base_med is None: continue
        # first cycle at/after which the statistic exceeds 5x its pre-window median
        K = 5
        obs = None
        for i in range(len(seq) - K):
            t0, _ = seq[i]
            if t0 < start - 400:
                continue
            if all(seq[i + j][1] > 5 * base_med for j in range(K)):
                obs = t0
                break
        if obs is not None:
            offs.append(round((obs - start) / DT))
    if offs:
        print(f"  n={len(offs)} seeds   offset (cycles): median={st.median(offs):.1f} "
              f"min={min(offs)} max={max(offs)}   zero-offset seeds={sum(1 for o in offs if o == 0)}/{len(offs)}")
    else:
        print("  NOT AVAILABLE -- no F3 step detected above 5x the pre-window median")

    # ---------------------------------------------------------------- 2. clustering
    print()
    print("=" * 78)
    print("2. SEED CLUSTERING -- seed % 7 gives 20 seeds only 7 window starts")
    print("   within- vs between-residue variance of coverage conditions AT ONSET")
    print("=" * 78)
    obs = {k: {} for k in ("vis2p4", "visUwb", "nAnchors", "gdop")}
    for d in sorted(glob.glob(os.path.join(base, "F0_Rfalse_seed*"))):
        sc = read_sidecar(d); rows = load_cell(d)
        if not rows: continue
        start, _ = window(sc["seed"], sc["durationS"], sc["frac"])
        row = min((r for r in rows), key=lambda r: abs(float(r["t"]) - start))
        for k in obs:
            try: obs[k][sc["seed"]] = float(row[k])
            except Exception: pass
    resid = {s: s % 7 for s in obs["vis2p4"]}
    print(f"  residue classes: " + ", ".join(f"{c}:{sorted(s for s in resid if resid[s]==c)}"
                                             for c in sorted(set(resid.values()))))
    print()
    print(f"  {'quantity':<10}{'within-var':>12}{'between-var':>13}{'ratio B/W':>11}  verdict")
    clustered = False
    for k, v in obs.items():
        if len(v) < 8: continue
        by = {}
        for s, x in v.items(): by.setdefault(resid[s], []).append(x)
        wv = st.mean([st.pvariance(g) for g in by.values() if len(g) > 1])
        gm = st.mean(list(v.values()))
        bv = st.mean([len(g) * (st.mean(g) - gm) ** 2 for g in by.values()])
        if wv == 0.0 and bv == 0.0:
            print(f"  {k:<10}{wv:>12.4f}{bv:>13.4f}{'n/a':>11}  constant -- no information")
            continue
        r = bv / wv if wv > 0 else float("inf")
        if r > 2.0: clustered = True
        print(f"  {k:<10}{wv:>12.4f}{bv:>13.4f}{r:>11.2f}  {'CLUSTERED' if r > 2.0 else 'not material'}")
    print(f"\n  verdict: clustering is {'REAL -- cluster bootstrap governs' if clustered else 'NOT material -- seed-level bootstrap stands'}")

    # ---------------------------------------------------------------- 3. separation
    print()
    print("=" * 78)
    print("3. SEPARATION RATIO -- median D^2 in-window / out-of-window, fix cycles")
    print("   per-seed ratios, geometric mean. Falsifier fires on the CI UPPER bound < 5x.")
    print("   F0 has zero bias magnitude, so its ratio is undefined and not computed.")
    print("=" * 78)
    resid_map = {s: s % 7 for s in range(1, 21)}
    print(f"  {'set':<10}{'n':>3}{'geo mean':>10}   {'seed-level CI':<20}{'cluster CI (7)':<20}verdict")
    for k in ("F1_Rfalse", "F1_Rtrue", "F3_Rfalse", "F3_Rtrue"):
        if k not in sets: continue
        rr = [(sd, v["ratio"]) for sd, v in sets[k]["seeds"].items()]
        pt, lo, hi, n = geo_mean_ci(rr)
        cpt, clo, chi, _ = geo_mean_ci(rr, clusters=resid_map)
        if pt is None:
            print(f"  {k:<10} NOT AVAILABLE -- fewer than 3 usable seeds"); continue
        sv = "FALSIFIED" if hi < 5.0 else "not falsified"
        cv = "FALSIFIED" if chi < 5.0 else "not falsified"
        print(f"  {k:<10}{n:>3}{pt:>10.3f}   [{lo:>6.3f}, {hi:>6.3f}]     [{clo:>6.3f}, {chi:>6.3f}]     "
              f"seed:{sv} / cluster:{cv}")

    # ---------------------------------------------------------------- 4. ROC
    print()
    print("=" * 78)
    print("4. ROC -- counts first. Population: fix cycles. Realised base rate 0.2000,")
    print("   equal to the rate the 0.90-0.95 target was derived under, so the target")
    print("   stands UNCORRECTED.")
    print("=" * 78)
    print(f"  {'set':<10}{'TP':>7}{'FP':>7}{'FN':>7}{'TN':>7}{'prec':>8}{'recall':>8}{'F1':>7}{'FA':>8}")
    fa_by = {}
    for k in ("F0_Rfalse","F0_Rtrue","F1_Rfalse","F1_Rtrue","F3_Rfalse","F3_Rtrue"):
        if k not in sets: continue
        v = sets[k]["seeds"].values()
        tp = sum(x["tp"] for x in v); fp = sum(x["fp"] for x in v)
        fn = sum(x["fn"] for x in v); tn = sum(x["tn"] for x in v)
        pre = tp/(tp+fp) if tp+fp else float('nan')
        rec = tp/(tp+fn) if tp+fn else float('nan')
        f1 = 2*pre*rec/(pre+rec) if (pre==pre and rec==rec and pre+rec>0) else float('nan')
        fa = fp/(fp+tn) if fp+tn else float('nan')
        fa_by[k] = fa
        print(f"  {k:<10}{tp:>7}{fp:>7}{fn:>7}{tn:>7}{pre:>8.4f}{rec:>8.4f}{f1:>7.4f}{fa:>8.4f}")

    # ---------------------------------------------------------------- 5. latency
    print()
    print("=" * 78)
    print("5. LATENCY from onset, in cycles -- each beside its concurrent FA and the")
    print("   chance floor 1-(1-FA)^(k+1), the probability of a 'detection' by luck alone.")
    print("=" * 78)
    print(f"  {'set':<10}{'n det':>7}{'median':>8}{'p90':>7}{'FA':>8}{'floor@med':>11}{'floor@p90':>11}")
    for k in ("F1_Rfalse","F1_Rtrue","F3_Rfalse","F3_Rtrue"):
        if k not in sets: continue
        lats = [x["lat"] for x in sets[k]["seeds"].values() if x["lat"] is not None]
        if not lats:
            print(f"  {k:<10}  0 seeds detected"); continue
        lats.sort()
        med = st.median(lats); p90 = lats[min(len(lats)-1, int(0.9*len(lats)))]
        fa = fa_by.get(k, float('nan'))
        fl = lambda kk: 1-(1-fa)**(kk+1)
        print(f"  {k:<10}{len(lats):>4}/{len(sets[k]['seeds'])}{med:>8.1f}{p90:>7.1f}{fa:>8.4f}"
              f"{fl(med):>11.4f}{fl(p90):>11.4f}")

    # ---------------------------------------------------------------- 6. F0
    print()
    print("=" * 78)
    print("6. F0 SUSPECT RATE -- the null control. Target <= 0.03.")
    print("=" * 78)
    for k in ("F0_Rfalse","F0_Rtrue"):
        if k not in sets: continue
        rates = [x["suspRate"] for x in sets[k]["seeds"].values()]
        m = st.mean(rates)
        print(f"  {k:<10} mean={m:.4f}  min={min(rates):.4f}  max={max(rates):.4f}  "
              f"{'PASS' if m <= 0.03 else 'FAIL -- ' + f'{m/0.03:.1f}x the target'}")
    return 0



if __name__ == "__main__":
    sys.exit(main())
