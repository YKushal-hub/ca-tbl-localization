#!/usr/bin/env python3
"""The design requirement, measured: A >= 0.95 at R <= 25 m (Architecture.md 1.3).

A(25 m) is the share of ALL decision cycles on which a node's position estimate is within 25 m of the truth. A cycle
with no fix counts by the estimate the filter holds then, which is what a searcher would be sent to. So the
denominator is every cycle, as the requirement states. It is read from each run's own traces:
  - tag 0: trace_<arm>_seed<N>.csv, column errM;
  - subjects 2..N: subjtrace_<arm>_seed<N>_subj<k>.csv, column errM.
"Five-node" pools tag 0 and the subjects, cycle by cycle. Not to be confused with the fix share (fixCycles/steps)
that the REAL-* and EKF5-WINDOW scorers report as "availability".

usage: goal_report.py <campaign dir> [<campaign dir> ...]
   Groups are the campaign's cell directories (e.g. A_S, B_W); seeds are their s<N> subdirectories.
   Also reports CEP95 (tag 0) and subject energy J from arm_summary / subject_summary, the REAL-* way.
"""
import csv
import glob
import os
import re
import sys

import numpy as np

ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "poucet_fusion", "lora_fusion"]
R = 25.0


def errs(path):
    try:
        return np.array([float(r["errM"]) for r in csv.DictReader(open(path))], dtype=float)
    except (OSError, KeyError, ValueError):
        return None


def cell(d, arm):
    """(tag-0 A25, five-node A25, tag-0 CEP95, subject J) for one run, or None if its traces are absent."""
    seed = re.search(r"s(\d+)$", d).group(1)
    t0 = errs(os.path.join(d, f"trace_{arm}_seed{seed}.csv"))
    if t0 is None or not len(t0):
        return None
    pool = [t0]
    for f in sorted(glob.glob(os.path.join(d, f"subjtrace_{arm}_seed{seed}_subj*.csv"))):
        e = errs(f)
        if e is not None and len(e):
            pool.append(e)
    allc = np.concatenate(pool)
    c95 = j = float("nan")
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            c95 = float(r["cep95M"])
    sp = os.path.join(d, "subject_summary.csv")
    if os.path.exists(sp):
        js = [float(r["totalJ"]) for r in csv.DictReader(open(sp)) if r["arm"] == arm]
        j = float(np.mean(js)) if js else float("nan")
    return (float(np.mean(t0 <= R)), float(np.mean(allc <= R)), c95, j)


def report(top):
    rows = []
    for g in sorted(x for x in os.listdir(top) if os.path.isdir(os.path.join(top, x)) and re.match(r"^[A-Z]_", x)):
        seeds = sorted(glob.glob(os.path.join(top, g, "s*")))
        for arm in ARMS:
            v = [c for c in (cell(d, arm) for d in seeds if os.path.exists(os.path.join(d, "arm_summary.csv"))) if c]
            if not v:
                continue
            a = np.array(v)
            rows.append((g, arm, len(v), a[:, 0].mean(), a[:, 1].mean(), (a[:, 1] >= 0.95).mean(), np.nanmean(a[:, 2]),
                         np.nanmean(a[:, 3])))
    out = [f"## {os.path.basename(top.rstrip('/'))}", "",
           "| group | arm | runs | A(25 m) tag 0 | A(25 m) five-node | runs meeting 0.95 | CEP95 tag 0 (m) | subject J |",
           "|---|---|---|---|---|---|---|---|"]
    for g, arm, n, a0, a5, met, c, j in rows:
        out.append(f"| {g} | {arm} | {n} | {a0:.3f} | {a5:.3f} | {met:.2f} | {c:.1f} | {j:.2f} |")
    return "\n".join(out) + "\n"


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    print("\n".join(report(t) for t in sys.argv[1:]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
