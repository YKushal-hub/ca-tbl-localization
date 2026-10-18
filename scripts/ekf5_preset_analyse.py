#!/usr/bin/env python3
"""EKF5-PRESET (report/PREREG_EKF5_PRESET.md): --imuFilter=ekf5 on the preset at DC-1 bias L0, L2,
L3, against DC1-BIAS's cv cells, paired by seed. Committed with the registration, before any cell;
not edited after the first cell.

  R1  9 of 9 cv cells (seed 1) reproduce DC1-BIAS's arm_summary.csv
  P1  at L3, ekf5 lowers ca_tbl_full tag 0 CEP95 against cv in A and B (paired interval below 0)
  P2  in B, ekf5's mean tag 0 CEP95 at L3 is at most 66.2 m (80 % of the cv loss recovered)
  P3  at L0, ca_tbl_full's mean per-seed CEP95 ratio ekf5 / cv - 1 is at most +0.10 in A, B, C

Estimators are main's own: lora_cmp_analyse.per_seed, camp_spec_analyse.ci, camp_final_analyse.uwb_fixes.

usage: ekf5_preset_analyse.py <out> <dc1_bias raw dir (with subject_summary.csv and traces)>
"""

import csv
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from camp_final_analyse import uwb_fixes  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
import numpy as np  # noqa: E402

CONFIGS = [("A", "5 km"), ("B", "endgame"), ("C", "N8 ring")]
LEVELS = ["L0", "L2", "L3"]
ARMS = ["ca_tbl_full", "static_868", "static_2p4", "fixed_threshold", "poucet_fusion", "lora_fusion"]
B_L0_CV, B_L3_CV = 12.0, 283.0  # DC1-BIAS's registered means, quoted in P2
BIAS_RE = re.compile(r"DC-1 IMU: bias (\S+) rad/s")


def rows(d):
    p = os.path.join(d, "arm_summary.csv")
    return list(csv.DictReader(open(p))) if os.path.exists(p) else None


def checks(out, dc1, problems):
    ek = sorted(glob.glob(os.path.join(out, "[ABC]_ekf5_L[023]", "s*")))
    cv = sorted(glob.glob(os.path.join(out, "[ABC]_cv_L[023]", "s*")))
    if len(ek) != 180 or len(cv) != 9:
        problems.append(f"{len(ek)} ekf5 cells and {len(cv)} cv cells, not 180 and 9")
    sources = set()
    for d in ek + cv:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        is_ek = "_ekf5_" in d
        if is_ek != ("N.28 ekf5:" in lg):
            problems.append(f"{d}: {'ekf5 cell without' if is_ek else 'cv cell with'} its N.28 ekf5 line")
        if is_ek:
            cell = os.path.basename(os.path.dirname(d)).replace("_ekf5_", "_")  # A_L3
            twin = os.path.join(dc1, cell, os.path.basename(d), "run.log")
            if os.path.exists(twin):
                a, b = BIAS_RE.findall(lg), BIAS_RE.findall(open(twin, errors="replace").read())
                if a != b:
                    problems.append(f"{d}: DC-1's drawn biases differ from DC1-BIAS's twin")
            else:
                problems.append(f"{d}: no DC1-BIAS twin at {twin}")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    return sources


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    out, dc1 = sys.argv[1], sys.argv[2]
    problems = []
    sources = checks(out, dc1, problems)
    md = ["# EKF5-PRESET — the 5-state filter on the preset (scripts/ekf5_preset_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}; cv from DC1-BIAS at {dc1}.", ""]
    same = differ = missing = 0
    for c, _ in CONFIGS:
        for lv in LEVELS:
            a, b = rows(os.path.join(out, f"{c}_cv_{lv}", "s1")), rows(os.path.join(dc1, f"{c}_{lv}", "s1"))
            if a is None or b is None:
                missing += 1
            elif a == b:
                same += 1
            else:
                differ += 1
    r1 = same == 9
    md += ["## R1 — cv on this binary reproduces DC1-BIAS", "",
           f"identical {same}, differing {differ}, missing {missing} -> R1 {'HOLDS' if r1 else 'FAILS'}", ""]
    c95 = {}
    for c, title in CONFIGS:
        md += [f"## {c} — {title}", ""]
        data = {}
        for lv in LEVELS:
            for a in ARMS:
                data[("ekf5", lv, a)] = per_seed(out, f"{c}_ekf5_{lv}", a)
                data[("cv", lv, a)] = per_seed(dc1, f"{c}_{lv}", a)
                for f in ("ekf5", "cv"):
                    if len(data[(f, lv, a)]) != 20:
                        problems.append(f"{c} {f} {lv} {a}: {len(data[(f, lv, a)])} seeds, not 20")
        for a in ARMS:
            md += [f"**{a}**, ekf5 − cv paired by seed", "",
                   "| level | CEP95 tag 0 cv → ekf5 (m) | Δ CEP95 tag 0 | Δ CEP95 subjects | Δ availability | Δ subject J |",
                   "|---|---|---|---|---|---|"]
            for lv in LEVELS:
                e, v = data[("ekf5", lv, a)], data[("cv", lv, a)]
                s = sorted(set(e) & set(v))
                if not s:
                    continue
                d = {k: cs.ci([e[i][k] - v[i][k] for i in s]) for k in ("c95", "s95", "av", "J")}
                me, mv = np.mean([e[i]["c95"] for i in s]), np.mean([v[i]["c95"] for i in s])
                md.append(f"| {lv} | {mv:.1f} → {me:.1f} | {cs.fmt(*d['c95'], p=1)} | {cs.fmt(*d['s95'], p=1)} | "
                          f"{cs.fmt(*d['av'])} | {cs.fmt(*d['J'], p=2)} |")
                if a == "ca_tbl_full":
                    c95[(c, lv)] = (d["c95"], me, float(np.mean([e[i]["c95"] / v[i]["c95"] - 1.0 for i in s])))
            md.append("")
        u = {lv: sum(uwb_fixes(dd, int(dd.rsplit("s", 1)[1])) for dd in cs.seeds(out, f"{c}_ekf5_{lv}")) for lv in LEVELS}
        md += ["UWB-delivered fixes, ca_tbl_full under ekf5: " + ", ".join(f"{lv} {u[lv]}" for lv in LEVELS) + ".", "",
               "Comparison-table contrasts under ekf5, CA-TBL − comparator, paired by seed:", "",
               "| contrast | level | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
        for comp in ("lora_fusion", "poucet_fusion"):
            for lv in LEVELS:
                t, o = data[("ekf5", lv, "ca_tbl_full")], data[("ekf5", lv, comp)]
                s = sorted(set(t) & set(o))
                if not s:
                    continue
                g = lambda k, p: cs.fmt(*cs.ci([t[i][k] - o[i][k] for i in s]), p=p)  # noqa: E731
                md.append(f"| CA-TBL − {comp} | {lv} | {g('av', 3)} | {g('c95', 1)} | {g('J', 2)} |")
        md.append("")
    p1 = all((c, "L3") in c95 and c95[(c, "L3")][0][2] < 0.0 for c in ("A", "B"))
    bound = B_L0_CV + 0.2 * (B_L3_CV - B_L0_CV)
    p2 = ("B", "L3") in c95 and c95[("B", "L3")][1] <= bound
    p3 = all((c, "L0") in c95 and c95[(c, "L0")][2] <= 0.10 for c, _ in CONFIGS)
    md += ["## Registered predictions", "",
           f"- R1 cv reproduces DC1-BIAS, 9 of 9: {'HOLDS' if r1 else 'FAILS'}",
           f"- P1 ekf5 lowers CA-TBL CEP95 at L3 in A and B: {'HOLDS' if p1 else 'FAILS'}",
           f"- P2 B, ekf5 mean CEP95 at L3 <= {bound:.1f} m: {'HOLDS' if p2 else 'FAILS'}"
           + (f" ({c95[('B', 'L3')][1]:.1f} m)" if ("B", "L3") in c95 else ""),
           f"- P3 L0 mean per-seed CEP95 ratio - 1 <= +0.10 in A, B, C: {'HOLDS' if p3 else 'FAILS'}"
           + " (" + ", ".join(f"{c} {c95[(c, 'L0')][2]:+.3f}" for c, _ in CONFIGS if (c, "L0") in c95) + ")", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "EKF5_PRESET.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
