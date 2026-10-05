#!/usr/bin/env python3
"""FUSE-01b analysis (report/PREREG_FUSE01B.md): scripts/fuse01_analyse.py's computation,
unchanged, on FUSE-01b's cells; its labels point at FUSE-01b. Writes <outdir>/FUSE01B.md and
<outdir>/fuse01b_cells.csv.

usage: fuse01b_analyse.py <outdir>
"""
import csv
import glob
import math
import os
import re
import sys

for _v in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_v, "1")
import numpy as np  # noqa: E402

DUR = 1800.0
NBOOT = 10000
RNG = np.random.default_rng(20261005)
SEEDS = list(range(1, 21))  # report/PREREG_FUSE01B.md sect 1
CONFIGS = ("main", "end")


def ok(d):
    p = os.path.join(d, "rc")
    return os.path.exists(p) and open(p).read().strip() == "0"


def arm_row(d, arm):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            return r
    return None


def subj_rows(d, arm):
    p = os.path.join(d, "subject_summary.csv")
    return [r for r in csv.DictReader(open(p)) if r["arm"] == arm] if os.path.exists(p) else []


def uwb_fixes(d, seed):
    """UWB-delivered fixes of ca_tbl_full over tag 0 and the subjects (trace fixTier UWB)."""
    n = 0
    fs = [os.path.join(d, f"trace_ca_tbl_full_seed{seed}.csv")] + \
        glob.glob(os.path.join(d, f"subjtrace_ca_tbl_full_seed{seed}_subj*.csv"))
    for f in fs:
        if not os.path.exists(f):
            continue
        for r in csv.DictReader(open(f)):
            if r["hadFix"] == "1" and r["fixTier"] == "UWB":
                n += 1
    return n


def fused(d, seed):
    p = os.path.join(d, f"fusefloor_ca_tbl_full_seed{seed}.csv")
    if not os.path.exists(p):
        return None
    rows = list(csv.DictReader(open(p)))
    return dict(sets=len(rows),
                uwb_fix=sum(1 for r in rows if r["hadFix"] == "1" and r["activeTier"] == "UWB" and int(r["nActive"]) > 0),
                floor_short=sum(1 for r in rows if int(r["nActive"]) == 0),
                fix=sum(1 for r in rows if r["hadFix"] == "1"))


def spread_defined(d):
    log = open(os.path.join(d, "run.log")).read()
    m = re.search(r"TA-1 base-station session \(ca_tbl_full\)[^\n]*batch precision spread (\S+)", log)
    return None if m is None else (m.group(1) != "UNDEFINED")


def metrics(d, seed, arm):
    a = arm_row(d, arm)
    ss = subj_rows(d, arm)
    if a is None:
        return None
    st = float(a["steps"])
    return dict(
        av=float(np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss])) if ss else float(a["fixCycles"]) / st,
        cep50=float(a["cep50M"]), cep95=float(a["cep95M"]),
        J=float(np.mean([float(r["totalJ"]) for r in ss])) if ss else float(a["totalJ"]))


def ci(v):
    v = np.asarray([x for x in v if not math.isnan(x)])
    if len(v) < 2:
        return (float(v[0]) if len(v) else float("nan")), float("nan"), float("nan")
    m = v[RNG.integers(0, len(v), size=(NBOOT, len(v)))].mean(axis=1)
    return float(v.mean()), float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975))


def f3(t, p=3):
    m, lo, hi = t
    return "n/a" if math.isnan(m) else (f"{m:.{p}f}" if math.isnan(lo) else f"{m:.{p}f} [{lo:.{p}f}, {hi:.{p}f}]")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    rows = []
    res = {}
    for c in CONFIGS:
        for f in ("off", "on"):
            for s in SEEDS:
                d = os.path.join(out, f"{c}_{f}", f"s{s}")
                if not ok(d):
                    problems.append(f"{c}_{f}/s{s} missing or non-zero exit")
                    continue
                for arm in ("ca_tbl_full", "static_868"):
                    m = metrics(d, s, arm)
                    if m is None:
                        problems.append(f"{c}_{f}/s{s} no {arm} row")
                        continue
                    if arm == "ca_tbl_full":
                        m["uwb"] = uwb_fixes(d, s)
                        m["spread"] = spread_defined(d)
                        fu = fused(d, s)
                        if f == "on" and fu is None:
                            problems.append(f"{c}_on/s{s}: no fusefloor file (abort condition)")
                        m.update({k: (fu or {}).get(k, 0) for k in ("sets", "uwb_fix", "floor_short", "fix")})
                    res[(c, f, s, arm)] = m
                    rows.append(dict(config=c, flag=f, seed=s, arm=arm, **{k: v for k, v in m.items()}))
    with open(os.path.join(out, "fuse01b_cells.csv"), "w", newline="") as fh:
        keys = sorted({k for r in rows for k in r})
        w = csv.DictWriter(fh, fieldnames=keys, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)

    md = ["# FUSE-01b -- the registered analysis, applied unchanged", "",
          "Paired by seed (on - off), 95 % seed-bootstrap intervals (10 000 resamples, seed 20261005).", ""]
    if problems:
        md += ["**Problems:**", ""] + [f"- {p}" for p in problems] + [""]
    verdict = {}
    for c in CONFIGS:
        md += [f"## {c}", "",
               "| arm, flag | availability | CEP50 / CEP95 tag 0 (m) | UWB-delivered fixes (sum; seeds > 0) | sessions with a defined spread | subject J |",
               "|---|---|---|---|---|---|"]
        for arm in ("ca_tbl_full", "static_868"):
            for f in ("off", "on"):
                ms = [res[(c, f, s, arm)] for s in SEEDS if (c, f, s, arm) in res]
                if not ms:
                    continue
                av = ci([m["av"] for m in ms])
                u = (f"{sum(m['uwb'] for m in ms)}; {sum(1 for m in ms if m['uwb'] > 0)}" if arm == "ca_tbl_full" else "-")
                sp = (f"{sum(1 for m in ms if m.get('spread'))} of {len(ms)}" if arm == "ca_tbl_full" else "-")
                md.append(f"| {arm}, {f} | {f3(av)} | {np.mean([m['cep50'] for m in ms]):.1f} / {np.mean([m['cep95'] for m in ms]):.1f} | "
                          f"{u} | {sp} | {np.mean([m['J'] for m in ms]):.2f} |")
        pair = [s for s in SEEDS if (c, "off", s, "ca_tbl_full") in res and (c, "on", s, "ca_tbl_full") in res]
        dav = ci([res[(c, "on", s, "ca_tbl_full")]["av"] - res[(c, "off", s, "ca_tbl_full")]["av"] for s in pair])
        d95 = ci([res[(c, "on", s, "ca_tbl_full")]["cep95"] / res[(c, "off", s, "ca_tbl_full")]["cep95"] - 1.0 for s in pair
                  if res[(c, "off", s, "ca_tbl_full")]["cep95"] > 0])
        vs868 = ci([res[(c, "on", s, "ca_tbl_full")]["av"] - res[(c, "on", s, "static_868")]["av"] for s in pair
                    if (c, "on", s, "static_868") in res])
        fz = [res[(c, "on", s, "ca_tbl_full")] for s in pair]
        md += ["", f"- paired availability difference (on - off), ca_tbl_full: {f3(dav)} over {len(pair)} seeds",
               f"- paired relative CEP95 change (on / off - 1), tag 0: {f3(d95)}",
               f"- availability, ca_tbl_full on - static_868: {f3(vs868)}",
               f"- fused sets (on): {sum(m['sets'] for m in fz)}; with no active range and a short floor: "
               f"{sum(m['floor_short'] for m in fz)}; became fixes: {sum(m['fix'] for m in fz)}; UWB fixes from a fused set: "
               f"{sum(m['uwb_fix'] for m in fz)}", ""]
        verdict[c] = dict(dav=dav, d95=d95, vs868=vs868, uwb_seeds=sum(1 for m in fz if m["uwb"] > 0))

    md += ["## Predictions (report/PREREG_FUSE01B.md sect 4)", ""]
    p1 = verdict["main"]["dav"][1] > 0
    p2 = verdict["main"]["vs868"][2] >= 0
    p3 = verdict["end"]["uwb_seeds"] >= 15
    p4 = verdict["end"]["d95"][2] <= 0.10
    md += [f"- P1 (main: availability rises, interval above 0): {'HELD' if p1 else 'FAILED'}",
           f"- P2 (main: ca_tbl_full on no longer below static_868, interval reaching 0): {'HELD' if p2 else 'FAILED'}",
           f"- P3 (end: UWB-delivered fixes on >= 15 of 20 seeds): {'HELD' if p3 else 'FAILED'} ({verdict['end']['uwb_seeds']})",
           f"- P4 (end: CEP95 not worse by more than 10 %, upper bound <= 0.10): {'HELD' if p4 else 'FAILED'}", ""]
    adopt = all(verdict[c]["dav"][0] >= -0.01 and verdict[c]["d95"][0] <= 0.10 for c in CONFIGS)
    md += ["## Adoption (sect 6)", "",
           f"Mean availability difference >= -0.01 and mean relative CEP95 change <= +0.10 in both configurations: "
           f"{'ADOPT' if adopt and not problems else 'DO NOT ADOPT'}"
           + (" (problems above)" if problems else ""), ""]
    open(os.path.join(out, "FUSE01B.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
