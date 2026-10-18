#!/usr/bin/env python3
"""S2-C subject consistency diagnosis (post-hoc, exploratory; training seeds 21-30 only).

usage: s2c_consistency_diag.py <outdir of scripts/s2c_run.sh>

From stage T's subject traces (ca_tbl_full, 5 km, --traceSubjectConsistency), per --measVarScale:
  - the subjects' NEES (posterior, against chi-square 2: mean 2, median 1.39, 5 % above 5.991) on
    coasting rows, on the coasting row just before a fix, and on fix rows;
  - at s = 1, the raw fix's 2-DOF NIS stratified by the fix's tier label, the set it came from
    (active tier or the 868 MHz floor set, which under the dual base also holds 2.4 GHz ranges)
    and its anchor count.
Rows with t < 120 s are excluded (warm-up), as in PREREG_S2C.
"""
import csv
import os
import sys
from collections import defaultdict

import numpy as np


def nees(r):
    pxx, pxy, pyy = float(r["pxx"]), float(r["pxy"]), float(r["pyy"])
    det = pxx * pyy - pxy * pxy
    if det <= 0:
        return None
    ex = float(r["estX"]) - float(r["trueX"])
    ey = float(r["estY"]) - float(r["trueY"])
    return (pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det


def fmt(x):
    x = np.array(x)
    return f"n {len(x)}, mean {x.mean():.1f}, median {np.median(x):.2f}, share > 5.991 {np.mean(x > 5.991):.2f}"


def main():
    out = sys.argv[1]
    for k in (1, 3, 10):
        coast, pre, fix, nis = [], [], [], defaultdict(list)
        for s in range(21, 31):
            for j in range(2, 6):
                p = os.path.join(out, f"T_A_s{k}", f"s{s}", f"subjtrace_ca_tbl_full_seed{s}_subj{j}.csv")
                rows = list(csv.DictReader(open(p)))
                for i, r in enumerate(rows):
                    if float(r["t"]) < 120.0:
                        continue
                    v = nees(r)
                    if v is None:
                        continue
                    if r["hadFix"] != "1":
                        coast.append(v)
                        continue
                    fix.append(v)
                    if i > 0 and rows[i - 1]["hadFix"] == "0":
                        pv = nees(rows[i - 1])
                        if pv is not None:
                            pre.append(pv)
                    m = float(r["mahalanobis"])
                    if m > 0:
                        key = (r["fixTier"], "868 floor set" if r["usedFloor"] == "1" else "active tier",
                               min(int(r["nAnchors"]), 6))
                        nis[key].append(m)
        print(f"## measVarScale {k}")
        print(f"- NEES, coasting rows: {fmt(coast)}")
        print(f"- NEES, coasting row just before a fix: {fmt(pre)}")
        print(f"- NEES, fix rows: {fmt(fix)}")
        if k == 1:
            print("\n| fix tier label | set | anchors (6 = 6+) | n | NIS > 9.21 | median NIS |")
            print("|---|---|---|---|---|---|")
            for key in sorted(nis, key=lambda x: -len(nis[x])):
                x = np.array(nis[key])
                if len(x) >= 15:
                    print(f"| {key[0]} | {key[1]} | {key[2]} | {len(x)} | {np.mean(x > 9.21):.2f} | {np.median(x):.2f} |")
        print()


if __name__ == "__main__":
    main()
