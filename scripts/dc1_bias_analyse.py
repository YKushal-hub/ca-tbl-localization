#!/usr/bin/env python3
"""DC1-BIAS (report/PREREG_DC1_BIAS.md): the preset with DC-1's gyro bias sigma at L0 (8.816 deg/h,
the preset), L2 0.15 and L3 0.5 deg/s (L1 dropped before registration), at A (5 km), B (endgame) and C (N8 ring).
Committed with the registration, before any cell; not edited after the first cell.

  R1  L0 cells reproduce D8-MAIN's off cells (arm_summary.csv, all rows), 60 of 60
  P1  ca_tbl_full tag 0 CEP95 rises at L3 against L0 in A and in B (paired interval above 0)
  P2  in B, ca_tbl_full's mean tag 0 CEP95 does not decrease L0 -> L2 -> L3
  Reported: every arm's paired change against L0; CA-TBL - lora_fusion and CA-TBL - poucet_fusion
  at every level; UWB-delivered fixes.

Estimators are main's own: lora_cmp_analyse.per_seed, camp_spec_analyse.ci, camp_final_analyse.uwb_fixes.

usage: dc1_bias_analyse.py <out> [<d8_main out>]
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
LEVELS = [("L0", "8.816 deg/h"), ("L2", "0.15 deg/s"), ("L3", "0.5 deg/s")]
ARMS = ["ca_tbl_full", "static_868", "static_2p4", "fixed_threshold", "poucet_fusion", "lora_fusion"]
SEEDS = list(range(1, 21))
Q = [("av", 3), ("c50", 1), ("c95", 1), ("s95", 1), ("tff", 0), ("J", 2)]


def checks(out, problems):
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_L[023]", "s*")))
    if len(cells) != 180:
        problems.append(f"{len(cells)} cells, not 180")
    sources = set()
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        biased = not os.path.basename(os.path.dirname(d)).endswith("_L0")
        has = "DC-1: gyro bias sigma" in open(os.path.join(d, "run.log"), errors="replace").read()
        if biased != has:
            problems.append(f"{d}: {'biased cell without' if biased else 'L0 cell with'} the DC-1 bias line")
    if len(sources) > 1:
        problems.append(f"more than one source across the campaign: {sorted(sources)}")
    return sources


def rows(d):
    p = os.path.join(d, "arm_summary.csv")
    return list(csv.DictReader(open(p))) if os.path.exists(p) else None


def r1(out, d8, md):
    md += ["## R1 — L0 reproduces D8-MAIN's off cells (arm_summary.csv, all rows)", ""]
    if not d8 or not os.path.isdir(d8):
        md += ["D8-MAIN directory not given or absent: R1 NOT CHECKED.", ""]
        return None
    same = differ = missing = 0
    for key, _ in CONFIGS:
        for s in SEEDS:
            a, b = rows(os.path.join(out, f"{key}_L0", f"s{s}")), rows(os.path.join(d8, f"{key}_off", f"s{s}"))
            if a is None or b is None:
                missing += 1
            elif a == b:
                same += 1
            else:
                differ += 1
    held = differ == 0 and missing == 0
    md += [f"identical {same}, differing {differ}, missing {missing} -> R1 {'HOLDS' if held else 'FAILS'}", ""]
    return held


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    d8 = sys.argv[2] if len(sys.argv) > 2 else "results/d8_main_2026-10-17"
    problems = []
    sources = checks(out, problems)
    md = ["# DC1-BIAS — the preset with a turn-on gyro bias (scripts/dc1_bias_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    held_r1 = r1(out, d8, md)
    table, c95 = [], {}
    for key, title in CONFIGS:
        data = {(lv, a): per_seed(out, f"{key}_{lv}", a) for lv, _ in LEVELS for a in ARMS}
        for (lv, a), v in data.items():
            if len(v) != 20:
                problems.append(f"{key} {lv} {a}: {len(v)} seeds, not 20")
        md += [f"## {key} — {title}", ""]
        for a in ARMS:
            ref = data[("L0", a)]
            md += [f"**{a}** (L0: availability {np.mean([x['av'] for x in ref.values()]) if ref else float('nan'):.3f}, "
                   f"tag 0 CEP95 {np.mean([x['c95'] for x in ref.values()]) if ref else float('nan'):.1f} m)", "",
                   "| level | Δ availability | Δ CEP50 tag 0 (m) | Δ CEP95 tag 0 (m) | Δ CEP95 subjects (m) | Δ subject J |",
                   "|---|---|---|---|---|---|"]
            for lv, lname in LEVELS[1:]:
                v = data[(lv, a)]
                s = sorted(set(v) & set(ref))
                if not s:
                    continue
                d = {k: cs.ci([v[i][k] - ref[i][k] for i in s]) for k, _ in Q}
                md.append(f"| {lv} {lname} | {cs.fmt(*d['av'])} | {cs.fmt(*d['c50'], p=1)} | {cs.fmt(*d['c95'], p=1)} | "
                          f"{cs.fmt(*d['s95'], p=1)} | {cs.fmt(*d['J'], p=2)} |")
                for k, _ in Q:
                    table.append(dict(config=key, level=lv, arm=a, quantity=k, diff=f"{d[k][0]:.6g}",
                                      lo=f"{d[k][1]:.6g}", hi=f"{d[k][2]:.6g}"))
                if a == "ca_tbl_full":
                    c95[(key, lv)] = d["c95"]
            md.append("")
        u = {lv: sum(uwb_fixes(dd, int(dd.rsplit("s", 1)[1])) for dd in cs.seeds(out, f"{key}_{lv}")) for lv, _ in LEVELS}
        md += [f"UWB-delivered fixes, ca_tbl_full: " + ", ".join(f"{lv} {u[lv]}" for lv, _ in LEVELS) + ".", "",
               "Comparison-table contrasts, CA-TBL − comparator, paired by seed:", "",
               "| contrast | level | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
        for comp in ("lora_fusion", "poucet_fusion"):
            for lv, _ in LEVELS:
                t, o = data[(lv, "ca_tbl_full")], data[(lv, comp)]
                s = sorted(set(t) & set(o))
                if not s:
                    continue
                f = lambda k, p: cs.fmt(*cs.ci([t[i][k] - o[i][k] for i in s]), p=p)  # noqa: E731
                md.append(f"| CA-TBL − {comp} | {lv} | {f('av', 3)} | {f('c95', 1)} | {f('J', 2)} |")
        means = [np.mean([x["c95"] for x in data[(lv, "ca_tbl_full")].values()]) if data[(lv, "ca_tbl_full")] else float("nan")
                 for lv, _ in LEVELS]
        md += ["", f"ca_tbl_full mean tag 0 CEP95 by level: " + ", ".join(f"{m:.1f}" for m in means) + " m", ""]
        c95[(key, "means")] = means
    p1 = all((k, "L3") in c95 and c95[(k, "L3")][1] > 0.0 for k in ("A", "B"))
    mb = c95.get(("B", "means"), [float("nan")] * 3)
    p2 = all(not np.isnan(x) for x in mb) and all(mb[i + 1] >= mb[i] for i in range(2))
    md += ["## Registered predictions", "",
           f"- R1 L0 reproduces D8-MAIN off: {'HOLDS' if held_r1 else ('NOT CHECKED' if held_r1 is None else 'FAILS')}",
           f"- P1 ca_tbl_full CEP95 rises at L3 in A and B: {'HOLDS' if p1 else 'FAILS'}",
           f"- P2 ca_tbl_full mean CEP95 in B non-decreasing L0, L2, L3: {'HOLDS' if p2 else 'FAILS'}", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    if table:
        with open(os.path.join(out, "dc1_bias_table.csv"), "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=list(table[0].keys()), lineterminator="\n")
            w.writeheader()
            w.writerows(table)
    open(os.path.join(out, "DC1_BIAS.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
