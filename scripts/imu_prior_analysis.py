#!/usr/bin/env python3
"""Score the C3 IMU-prior campaign exactly as report/PREREG_IMU_PRIOR.md registers it,
and apply A0 v2 (report/PREREG_A0_V2.md) to its four cells.

Written and committed BEFORE the campaign's first cell. Nothing here may be tuned after
the data exists; a change after the first cell is a deviation and is reported as one.

Layout read:   <root>/<cfg>_<cond>_s<seed>/
               cfg  in {ref, sparse}     cond in {cv, imu}     seed in 301..330
Each directory holds one simulator invocation: arm_summary.csv (6 arms), trace_*.csv
(6), run_id.txt with a digest block, and for cond=imu also imu_summary.csv and
imu_trace_*.csv.

Estimators follow the project's conventions: seeds are the independent unit; a ratio is
summarised by the geometric mean of per-seed ratios with a seed-level percentile
bootstrap; a difference by the mean of per-seed differences, same bootstrap. 10 000
resamples, generator seeded so the CI is reproducible.

Usage:  imu_prior_analysis.py <root> <outdir>
Writes  <outdir>/imu_prior_analysis.csv  (long format, one row per estimate)
        <outdir>/imu_prior_report.txt    (what stdout shows)
"""

import csv
import hashlib
import math
import os
import random
import statistics
import sys

CFGS = ["ref", "sparse"]
CONDS = ["cv", "imu"]
SEEDS = list(range(301, 331))
# Test-only override, mirroring scripts/sweeps/imu_prior_campaign.sh: lets the scorer be
# exercised on seeds outside the registered range before the registration is committed.
if os.environ.get("IMU_SEEDS"):
    SEEDS = [int(x) for x in os.environ["IMU_SEEDS"].split()]
ARMS = ["static_868", "static_2p4", "static_uwb", "fixed_threshold", "ca_tbl_no_integrity",
        "ca_tbl_full"]
STATIC = ["static_868", "static_2p4", "static_uwb"]
DYNAMIC = ["fixed_threshold", "ca_tbl_no_integrity", "ca_tbl_full"]
# The columns an IMU is ALLOWED to change on a static arm. Everything else in
# arm_summary.csv is radio-side and must be byte-identical (control R0).
ERR_COLS = {"rmseM", "cep50M", "cep95M", "maxErrM", "rmseFixedM", "cep50FixedM",
            "accuracyPerJoule"}
A_REQ, R_REQ, CYC = 0.95, 25.0, 1440
DEEP_K = 10            # a no-fix cycle is "deep" from the 10th consecutive one on
MIN_CYCLES = 5         # per-seed minimum for a median to enter a ratio
IMU_J_EXPECTED = 0.970e-3 * 3.3 * 3600.0
KAPPAS = [0.25, 0.5, 1.0, 2.0]
NBOOT = 10000

OUT = []
REPORT = []


def f3(v, fmt="%.3f"):
    return "nan" if v is None else fmt % v


def say(s=""):
    print(s)
    REPORT.append(s)


def emit(metric, cfg, arm, n, est, lo, hi, note=""):
    OUT.append({"metric": metric, "cfg": cfg, "arm": arm, "n": n,
                "estimate": "" if est is None else "%.6g" % est,
                "lo": "" if lo is None else "%.6g" % lo,
                "hi": "" if hi is None else "%.6g" % hi, "note": note})


def boot_mean(vals, rng):
    if not vals:
        return None, None, None
    m = sum(vals) / len(vals)
    if len(vals) == 1:
        return m, None, None
    bs = []
    for _ in range(NBOOT):
        s = [vals[rng.randrange(len(vals))] for _ in vals]
        bs.append(sum(s) / len(s))
    bs.sort()
    return m, bs[int(0.025 * NBOOT)], bs[int(0.975 * NBOOT) - 1]


def boot_gmean(ratios, rng):
    logs = [math.log(r) for r in ratios if r > 0]
    m, lo, hi = boot_mean(logs, rng)
    f = (lambda v: None if v is None else math.exp(v))
    return f(m), f(lo), f(hi)


def read_csv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def check_dir(d, cond):
    """Completeness and digest. Returns a list of problems; empty means usable."""
    probs = []
    if not os.path.isdir(d):
        return ["missing directory"]
    rid = os.path.join(d, "run_id.txt")
    if not os.path.exists(rid):
        return ["no run_id.txt"]
    text = open(rid).read()
    if " dirty 0 " not in text:
        probs.append("sidecar not dirty 0")
    if "profile optimized" not in text:
        probs.append("not built optimized")
    if "digest sha256" not in text:
        return probs + ["no digest block -- run did not finish"]
    digest = {}
    for line in text.split("digest sha256", 1)[1].strip().splitlines():
        h, name = line.split(None, 1)
        digest[name.strip()] = h
    for name, h in digest.items():
        p = os.path.join(d, name)
        if not os.path.exists(p):
            probs.append("digest names absent file %s" % name)
        elif hashlib.sha256(open(p, "rb").read()).hexdigest() != h:
            probs.append("digest mismatch %s" % name)
    # Rule 12: count rows, never test for non-empty.
    rows = read_csv(os.path.join(d, "arm_summary.csv")) if "arm_summary.csv" in digest else []
    if sorted(r["arm"] for r in rows) != sorted(ARMS):
        probs.append("arm_summary rows %d, not the six arms" % len(rows))
    traces = [n for n in digest if n.startswith("trace_")]
    if len(traces) != 6:
        probs.append("%d arm traces, not 6" % len(traces))
    if cond == "imu":
        if "imu_summary.csv" not in digest:
            probs.append("no imu_summary.csv")
        elif len(read_csv(os.path.join(d, "imu_summary.csv"))) != 6:
            probs.append("imu_summary rows not 6")
        if len([n for n in digest if n.startswith("imu_trace_")]) != 6:
            probs.append("imu traces not 6")
    else:
        if any(n.startswith("imu_") for n in digest):
            probs.append("IMU output in a cv cell")
    return probs


def trace_stats(path):
    """Per-cycle scoring. A cycle's deep/not-deep label depends only on cycles BEFORE it,
    so truncation by the run's end cannot relabel any cycle (rule 2)."""
    rows = read_csv(path)
    nofix, deep = [], []
    a_fix = a_est = 0
    run = 0
    for r in rows:
        err = float(r["errM"])
        had = r["hadFix"] == "1"
        if had:
            run = 0
            if err <= R_REQ:
                a_fix += 1
        else:
            run += 1
            nofix.append(err)
            if run >= DEEP_K:
                deep.append(err)
        if err <= R_REQ:
            a_est += 1
    n = len(rows)
    return {"n": n, "nofix": nofix, "deep": deep, "A_fix": a_fix / CYC, "A_est": a_est / CYC}


def verdict(dT, dB, N, rho, lo, hi, kappa):
    if dT == 0 and dB == 0:
        return "INCONCLUSIVE (requirement unmet by any arm)"
    dD = dT - dB
    allow = min(2.0, 1.0 + kappa * dD / N)
    if lo is None or hi is None:
        return "NO CI"
    if dD > 0 and hi < allow:
        return "PASS"
    if dD <= 0 or lo > allow:
        return "FAIL"
    return "INCONCLUSIVE"


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    root, outdir = argv[1], argv[2]
    os.makedirs(outdir, exist_ok=True)
    rng = random.Random(20261014)

    # ---- completeness -----------------------------------------------------------
    say("=== completeness (each directory: digest-verified, dirty 0, rows counted)")
    data = {}
    excluded = []
    for cfg in CFGS:
        for cond in CONDS:
            for seed in SEEDS:
                d = os.path.join(root, "%s_%s_s%d" % (cfg, cond, seed))
                probs = check_dir(d, cond)
                if probs:
                    excluded.append((cfg, cond, seed, probs))
                    continue
                summ = {r["arm"]: r for r in read_csv(os.path.join(d, "arm_summary.csv"))}
                imu = {}
                if cond == "imu":
                    imu = {r["arm"]: r for r in read_csv(os.path.join(d, "imu_summary.csv"))}
                tr = {a: trace_stats(os.path.join(d, "trace_%s_seed%d.csv" % (a, seed)))
                      for a in ARMS}
                data[(cfg, cond, seed)] = {"summ": summ, "imu": imu, "tr": tr}
    total = len(CFGS) * len(CONDS) * len(SEEDS)
    say("  usable %d of %d directories" % (len(data), total))
    for cfg, cond, seed, probs in excluded:
        say("  EXCLUDED %s_%s_s%d: %s" % (cfg, cond, seed, "; ".join(probs)))
    if len(excluded) > 0.10 * total:
        say("ABORT: more than 10%% of directories unusable (registered abort condition)")
        return 1

    def paired(cfg):
        return [s for s in SEEDS if (cfg, "cv", s) in data and (cfg, "imu", s) in data]

    # ---- R0: static arms' radio side identical ---------------------------------
    say("\n=== R0 control: static arms, every non-error arm_summary column identical cv vs imu")
    r0_bad = 0
    r0_n = 0
    for cfg in CFGS:
        for s in paired(cfg):
            for a in STATIC:
                cv = data[(cfg, "cv", s)]["summ"][a]
                im = data[(cfg, "imu", s)]["summ"][a]
                diff = [k for k in cv if k not in ERR_COLS and cv[k] != im[k]]
                r0_n += 1
                if diff:
                    r0_bad += 1
                    if r0_bad <= 5:
                        say("  LEAK %s seed %d %s: %s" % (cfg, s, a, ",".join(diff[:6])))
    say("  %d of %d static-arm row pairs differ outside the error columns" % (r0_bad, r0_n))
    emit("R0_static_rows_differing", "all", "static", r0_n, r0_bad, None, None)
    if r0_bad:
        say("ABORT INTERPRETATION: the IMU reached the radio path (registered abort)")

    # ---- P2: imuJ exact ---------------------------------------------------------
    say("\n=== P2 imuJ = %.6f J on every IMU row" % IMU_J_EXPECTED)
    bad = 0
    n = 0
    for (cfg, cond, s), v in data.items():
        for a, r in v["imu"].items():
            n += 1
            if abs(float(r["imuJ"]) - IMU_J_EXPECTED) > 5e-9:
                bad += 1
            if int(r["imuPredicts"]) != int(r["steps"]):
                bad += 1
    say("  %d of %d IMU rows off (imuJ or imuPredicts != steps)" % (bad, n))
    emit("P2_imu_rows_off", "all", "all", n, bad, None, None)

    # ---- M1/M2: error on no-fix and deep no-fix cycles --------------------------
    for key, label, metric in (("nofix", "no-fix", "M1"), ("deep", "deep (>=10th consecutive no-fix)", "M2")):
        say("\n=== %s ratio of per-seed MEDIAN error on %s cycles, imu / cv" % (metric, label))
        say("  %-7s %-20s %4s %9s %21s  %s" % ("cfg", "arm", "n", "gmean", "95% CI", "median cv / imu (m)"))
        for cfg in CFGS:
            for a in ARMS:
                ratios, mcv, mim = [], [], []
                for s in paired(cfg):
                    c = data[(cfg, "cv", s)]["tr"][a][key]
                    i = data[(cfg, "imu", s)]["tr"][a][key]
                    if len(c) >= MIN_CYCLES and len(i) >= MIN_CYCLES:
                        mc, mi = statistics.median(c), statistics.median(i)
                        if mc > 0 and mi > 0:
                            ratios.append(mi / mc)
                            mcv.append(mc)
                            mim.append(mi)
                g, lo, hi = boot_gmean(ratios, rng)
                emit(metric + "_ratio", cfg, a, len(ratios), g, lo, hi)
                if ratios:
                    say("  %-7s %-20s %4d %9s  [%7s, %7s]  %8.2f / %8.2f" % (
                        cfg, a, len(ratios), f3(g), f3(lo), f3(hi),
                        statistics.median(mcv), statistics.median(mim)))
                else:
                    say("  %-7s %-20s %4d   not testable (< %d cycles in a seed)" % (cfg, a, 0, MIN_CYCLES))

    # ---- M3: availability --------------------------------------------------------
    for akey in ("A_fix", "A_est"):
        say("\n=== M3 %s, paired difference imu - cv" % akey)
        for cfg in CFGS:
            for a in ARMS:
                ds = [data[(cfg, "imu", s)]["tr"][a][akey] - data[(cfg, "cv", s)]["tr"][a][akey]
                      for s in paired(cfg)]
                m, lo, hi = boot_mean(ds, rng)
                cvm = statistics.mean(data[(cfg, "cv", s)]["tr"][a][akey] for s in paired(cfg))
                emit("M3_" + akey + "_diff", cfg, a, len(ds), m, lo, hi, "cv mean %.4f" % cvm)
                say("  %-7s %-20s cv %.4f  diff %s [%s, %s]" % (
                    cfg, a, cvm, f3(m, "%+.5f"), f3(lo, "%+.5f"), f3(hi, "%+.5f")))

    # ---- M4: energy ---------------------------------------------------------------
    say("\n=== M4 radio energy totalJ, ratio imu / cv (geometric mean of per-seed ratios)")
    for cfg in CFGS:
        for a in ARMS:
            rs = [float(data[(cfg, "imu", s)]["summ"][a]["totalJ"]) /
                  float(data[(cfg, "cv", s)]["summ"][a]["totalJ"]) for s in paired(cfg)]
            g, lo, hi = boot_gmean(rs, rng)
            emit("M4_radioJ_ratio", cfg, a, len(rs), g, lo, hi)
            say("  %-7s %-20s %s [%s, %s]" % (cfg, a, f3(g, "%.4f"), f3(lo, "%.4f"), f3(hi, "%.4f")))

    # ---- M5: escalations ----------------------------------------------------------
    say("\n=== M5 escalations, paired difference imu - cv (dynamic arms)")
    for cfg in CFGS:
        for a in DYNAMIC:
            ds = [int(data[(cfg, "imu", s)]["summ"][a]["escalations"]) -
                  int(data[(cfg, "cv", s)]["summ"][a]["escalations"]) for s in paired(cfg)]
            cvm = statistics.mean(int(data[(cfg, "cv", s)]["summ"][a]["escalations"])
                                  for s in paired(cfg))
            m, lo, hi = boot_mean(ds, rng)
            emit("M5_escalations_diff", cfg, a, len(ds), m, lo, hi, "cv mean %.1f" % cvm)
            say("  %-7s %-20s cv mean %7.1f  diff %s [%s, %s]" % (
                cfg, a, cvm, f3(m, "%+8.2f"), f3(lo, "%+8.2f"), f3(hi, "%+8.2f")))

    # ---- M6: NEES (descriptive) ---------------------------------------------------
    say("\n=== M6 position NEES of the IMU filter in the simulation (descriptive; 2.0 = consistent)")
    for cfg in CFGS:
        for a in ARMS:
            vals = [float(data[(cfg, "imu", s)]["imu"][a]["neesMean"]) for s in paired(cfg)]
            over = [float(data[(cfg, "imu", s)]["imu"][a]["neesFracOver95"]) for s in paired(cfg)]
            m, lo, hi = boot_mean(vals, rng)
            emit("M6_nees_mean", cfg, a, len(vals), m, lo, hi,
                 "frac>chi2_95 %.4f" % statistics.mean(over))
            say("  %-7s %-20s NEES %s [%s, %s]  frac>5.991 %.4f (consistent: 0.05)" % (
                cfg, a, f3(m), f3(lo), f3(hi), statistics.mean(over)))

    # ---- A0 v2 --------------------------------------------------------------------
    say("\n=== A0 v2: T = ca_tbl_full, B = static_2p4 (report/PREREG_A0_V2.md)")
    for cfg in CFGS:
        for cond in CONDS:
            seeds = [s for s in SEEDS if (cfg, cond, s) in data]
            N = len(seeds)
            for akey, aprim in (("A_fix", "primary"), ("A_est", "secondary")):
                dT = sum(1 for s in seeds if data[(cfg, cond, s)]["tr"]["ca_tbl_full"][akey] >= A_REQ)
                dB = sum(1 for s in seeds if data[(cfg, cond, s)]["tr"]["static_2p4"][akey] >= A_REQ)
                energies = [("radio", "primary")]
                if cond == "imu":
                    energies.append(("system", "secondary"))
                for ename, eprim in energies:
                    rs = []
                    for s in seeds:
                        v = data[(cfg, cond, s)]
                        eT = float(v["summ"]["ca_tbl_full"]["totalJ"])
                        eB = float(v["summ"]["static_2p4"]["totalJ"])
                        if ename == "system":
                            eT += float(v["imu"]["ca_tbl_full"]["imuJ"])
                            eB += float(v["imu"]["static_2p4"]["imuJ"])
                        rs.append(eT / eB)
                    g, lo, hi = boot_gmean(rs, rng)
                    verdicts = {k: verdict(dT, dB, N, g, lo, hi, k) for k in KAPPAS}
                    tag = "%s/%s availability, %s/%s energy" % (akey, aprim, ename, eprim)
                    emit("A0v2_%s_%s" % (akey, ename), "%s_%s" % (cfg, cond), "T_vs_B", N, g, lo, hi,
                         "D_T=%d D_B=%d verdict(k=1)=%s band=%s" % (
                             dT, dB, verdicts[1.0],
                             "|".join("%g:%s" % (k, verdicts[k]) for k in KAPPAS)))
                    say("  %-6s %-3s %-46s D_T %2d/%d D_B %2d/%d rho %s [%s, %s] -> %s" % (
                        cfg, cond, tag, dT, N, dB, N, f3(g), f3(lo), f3(hi), verdicts[1.0]))
                    flips = sorted(set(verdicts.values()))
                    if len(flips) > 1:
                        say("         kappa band: %s" % ", ".join(
                            "%g:%s" % (k, verdicts[k]) for k in KAPPAS))

    with open(os.path.join(outdir, "imu_prior_analysis.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["metric", "cfg", "arm", "n", "estimate", "lo", "hi", "note"])
        w.writeheader()
        w.writerows(OUT)
    with open(os.path.join(outdir, "imu_prior_report.txt"), "w") as f:
        f.write("\n".join(REPORT) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
