#!/usr/bin/env python3
"""G181 checker: DD 46 (UWB futility gate), DD 47 (depth B2), DD 48 (finder frame B3)."""
import csv
import math
import os
import sys


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read())
    except (OSError, ValueError):
        return None


def main():
    d = sys.argv[1]
    fails = []
    for n in ("g_off", "g_on", "d", "f_off", "f_on"):
        if rc(d, n) != 0:
            fails.append(f"{n}: exit {rc(d, n)}")
    if not fails:
        a = next(csv.DictReader(open(os.path.join(d, "g_off", "arm_summary.csv"))))
        b = next(csv.DictReader(open(os.path.join(d, "g_on", "arm_summary.csv"))))
        if not float(b["j_uwb"]) < 0.5 * float(a["j_uwb"]):
            fails.append(f"DD46: UWB energy {b['j_uwb']} with the gate vs {a['j_uwb']} without (not halved)")
        E = list(csv.DictReader(open(os.path.join(d, "d", "collapse_depth_ca_tbl_full_seed902.csv"))))
        T = {r["worker"]: r for r in csv.DictReader(open(os.path.join(d, "d", "collapse_truth_ca_tbl_full_seed902.csv")))}
        if len(E) != 20 or len(T) != 20:
            fails.append(f"DD47: {len(E)} estimates / {len(T)} truths, expected 20")
        for r in E:
            v = r["depth_est_m"]
            if v != "nan" and not (0.0 <= float(v) <= 10.0 and float(r["depth_sd_m"]) <= 1.0):
                fails.append(f"DD47: worker {r['worker']} depth {v} sd {r['depth_sd_m']} outside the reporting rule")
        for r in T.values():
            if not (0.1 - 1e-6 <= float(r["depth_true_m"]) <= 2.286 + 1e-6):
                fails.append(f"DD47: truth depth {r['depth_true_m']} outside [0.1, 2.286]")
        rows = [list(csv.DictReader(open(os.path.join(d, n, "subject_summary.csv")))) for n in ("f_off", "f_on")]
        if not (len(rows[0]) == len(rows[1]) == 20):
            fails.append("DD48: subject rows missing")
        elif rows[0] == rows[1]:
            fails.append("DD48: --finderFrameSolve changed nothing (published finder positions should differ)")
    if fails:
        print("G181 FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print(f"G181 PASS (DD46 UWB J {b['j_uwb']} vs {a['j_uwb']}; DD47 files and reporting rule; DD48 changes positions)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
