#!/usr/bin/env python3
"""EKF5-CONSIDER (report/PREREG_EKF5_CONSIDER.md): --ekf5Robust=8 against EKF5-PRESET's ekf5 cells
(mode 0), paired by seed. Committed with the registration, before any cell; not edited after the
first cell.

  R1  9 of 9 mode-0 cells (seed 1) reproduce EKF5-PRESET's arm_summary.csv
  P1  over A, B, C x L2, L3 (120 seed-cells), ca_tbl_full cells ending with a heading offset error
      > 0.3 rad are fewer under mode 8, and the discordant pairs favour mode 8 at least 2 : 1
  P2  at L2, mode 8 lowers ca_tbl_full's tag 0 CEP95 in A and in C (paired interval below 0)
  P3  at L0, ca_tbl_full's mean per-seed CEP95 ratio mode 8 / mode 0 - 1 is at most +0.10 in A, B, C

Estimators are main's own: lora_cmp_analyse.per_seed, camp_spec_analyse.ci, camp_final_analyse.uwb_fixes.
Heading error: each arm's "N.28 ekf5:" line, which run.log prints before that arm's summary row.

usage: ekf5_consider_analyse.py <out> <ekf5_preset raw dir (with subject_summary.csv)>
"""

import csv
import glob
import math
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
HEAD = re.compile(r"N\.28 ekf5: .*heading offset est (\S+) \(true (\S+)\) rad")
ROW = re.compile(r"^  (" + "|".join(ARMS) + r")\s+\d")
CONS = re.compile(r"N\.33 ekf5 mode 8: .*considered (\d+)")


def rows(d):
    p = os.path.join(d, "arm_summary.csv")
    return list(csv.DictReader(open(p))) if os.path.exists(p) else None


def heading_errors(d):
    """{arm: |final heading offset error|, rad} from run.log; a line belongs to the next arm row."""
    out, pend = {}, None
    p = os.path.join(d, "run.log")
    if not os.path.exists(p):
        return out
    for ln in open(p, errors="replace"):
        m = HEAD.search(ln)
        if m:
            e, t = float(m.group(1)), float(m.group(2))
            pend = abs(math.atan2(math.sin(e - t), math.cos(e - t)))
            continue
        r = ROW.match(ln)
        if r and pend is not None:
            out[r.group(1)] = pend
            pend = None
    return out


def checks(out, ref, problems):
    m8 = sorted(glob.glob(os.path.join(out, "[ABC]_m8_L[023]", "s*")))
    m0 = sorted(glob.glob(os.path.join(out, "[ABC]_m0_L[023]", "s*")))
    if len(m8) != 180 or len(m0) != 9:
        problems.append(f"{len(m8)} mode-8 cells and {len(m0)} mode-0 cells, not 180 and 9")
    sources = set()
    for d in m8 + m0:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        is8 = "_m8_" in d
        if is8 != ("N.33 ekf5 mode 8:" in lg):
            problems.append(f"{d}: {'mode-8 cell without' if is8 else 'mode-0 cell with'} its N.33 line")
        if lg.count("N.28 ekf5:") != 6 or len(heading_errors(d)) != 6:
            problems.append(f"{d}: not six N.28 ekf5 lines paired with six arm rows")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    return sources


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    out, ref = sys.argv[1], sys.argv[2]
    problems = []
    sources = checks(out, ref, problems)
    md = ["# EKF5-CONSIDER — Schmidt updates for sparse fixes (scripts/ekf5_consider_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}; mode 0 from EKF5-PRESET at {ref}.", ""]
    same = differ = missing = 0
    for c, _ in CONFIGS:
        for lv in LEVELS:
            a, b = rows(os.path.join(out, f"{c}_m0_{lv}", "s1")), rows(os.path.join(ref, f"{c}_ekf5_{lv}", "s1"))
            if a is None or b is None:
                missing += 1
            elif a == b:
                same += 1
            else:
                differ += 1
    r1 = same == 9
    md += ["## R1 — mode 0 on this binary reproduces EKF5-PRESET", "",
           f"identical {same}, differing {differ}, missing {missing} -> R1 {'HOLDS' if r1 else 'FAILS'}", ""]
    c95, heads, cons = {}, {}, []
    for c, title in CONFIGS:
        md += [f"## {c} — {title}", ""]
        data = {}
        for lv in LEVELS:
            for a in ARMS:
                data[("m8", lv, a)] = per_seed(out, f"{c}_m8_{lv}", a)
                data[("m0", lv, a)] = per_seed(ref, f"{c}_ekf5_{lv}", a)
                for f in ("m8", "m0"):
                    if len(data[(f, lv, a)]) != 20:
                        problems.append(f"{c} {f} {lv} {a}: {len(data[(f, lv, a)])} seeds, not 20")
            for s in range(1, 21):
                h8 = heading_errors(os.path.join(out, f"{c}_m8_{lv}", f"s{s}"))
                h0 = heading_errors(os.path.join(ref, f"{c}_ekf5_{lv}", f"s{s}"))
                for a in ARMS:
                    if a in h8 and a in h0:
                        heads[(c, lv, s, a)] = (h0[a], h8[a])
                p = os.path.join(out, f"{c}_m8_{lv}", f"s{s}", "run.log")
                if os.path.exists(p):
                    cons += [int(x) for x in CONS.findall(open(p, errors="replace").read())[:1]]
        for a in ARMS:
            md += [f"**{a}**, mode 8 − mode 0 paired by seed", "",
                   "| level | CEP95 tag 0 m0 → m8 (m) | Δ CEP95 tag 0 | Δ CEP95 subjects | Δ availability | Δ subject J | heading err > 0.3 rad m0 → m8 |",
                   "|---|---|---|---|---|---|---|"]
            for lv in LEVELS:
                e, v = data[("m8", lv, a)], data[("m0", lv, a)]
                s = sorted(set(e) & set(v))
                if not s:
                    continue
                d = {k: cs.ci([e[i][k] - v[i][k] for i in s]) for k in ("c95", "s95", "av", "J")}
                me, mv = np.mean([e[i]["c95"] for i in s]), np.mean([v[i]["c95"] for i in s])
                hb0 = sum(1 for i in s if (c, lv, i, a) in heads and heads[(c, lv, i, a)][0] > 0.3)
                hb8 = sum(1 for i in s if (c, lv, i, a) in heads and heads[(c, lv, i, a)][1] > 0.3)
                md.append(f"| {lv} | {mv:.1f} → {me:.1f} | {cs.fmt(*d['c95'], p=1)} | {cs.fmt(*d['s95'], p=1)} | "
                          f"{cs.fmt(*d['av'])} | {cs.fmt(*d['J'], p=2)} | {hb0} → {hb8} |")
                if a == "ca_tbl_full":
                    c95[(c, lv)] = (d["c95"], me, float(np.mean([e[i]["c95"] / v[i]["c95"] - 1.0 for i in s])))
            md.append("")
        u = {lv: sum(uwb_fixes(dd, int(dd.rsplit("s", 1)[1])) for dd in cs.seeds(out, f"{c}_m8_{lv}")) for lv in LEVELS}
        md += ["UWB-delivered fixes, ca_tbl_full under mode 8: " + ", ".join(f"{lv} {u[lv]}" for lv in LEVELS) + ".", "",
               "Comparison-table contrasts, CA-TBL − comparator, paired by seed, mode 0 and mode 8:", "",
               "| contrast | level | Δ availability m0 / m8 | Δ CEP95 tag 0 m0 / m8 (m) | Δ subject J m8 |", "|---|---|---|---|---|"]
        for comp in ("lora_fusion", "poucet_fusion", "static_2p4", "fixed_threshold"):
            for lv in LEVELS:
                g = {}
                for f in ("m0", "m8"):
                    t, o = data[(f, lv, "ca_tbl_full")], data[(f, lv, comp)]
                    s = sorted(set(t) & set(o))
                    g[f] = (lambda k, p, t=t, o=o, s=s: cs.fmt(*cs.ci([t[i][k] - o[i][k] for i in s]), p=p)) if s else None
                if g["m0"] and g["m8"]:
                    md.append(f"| CA-TBL − {comp} | {lv} | {g['m0']('av', 3)} / {g['m8']('av', 3)} | "
                              f"{g['m0']('c95', 1)} / {g['m8']('c95', 1)} | {g['m8']('J', 2)} |")
        md.append("")
    pool = [heads[k] for k in heads if k[3] == "ca_tbl_full" and k[1] in ("L2", "L3")]
    b0 = sum(1 for h0, h8 in pool if h0 > 0.3)
    b8 = sum(1 for h0, h8 in pool if h8 > 0.3)
    only0 = sum(1 for h0, h8 in pool if h0 > 0.3 and not h8 > 0.3)
    only8 = sum(1 for h0, h8 in pool if h8 > 0.3 and not h0 > 0.3)
    if len(pool) != 120:
        problems.append(f"P1 pool has {len(pool)} seed-cells, not 120")
    p1 = len(pool) == 120 and b8 < b0 and only0 >= 2 * only8
    p2 = all((c, "L2") in c95 and c95[(c, "L2")][0][2] < 0.0 for c in ("A", "C"))
    p3 = all((c, "L0") in c95 and c95[(c, "L0")][2] <= 0.10 for c, _ in CONFIGS)
    md += ["## Registered predictions", "",
           f"- R1 mode 0 reproduces EKF5-PRESET, 9 of 9: {'HOLDS' if r1 else 'FAILS'}",
           f"- P1 heading divergence (> 0.3 rad), ca_tbl_full, A/B/C x L2/L3: {b0} → {b8} of {len(pool)}; "
           f"discordant mode-0-only {only0}, mode-8-only {only8}: {'HOLDS' if p1 else 'FAILS'}",
           f"- P2 mode 8 lowers CA-TBL CEP95 at L2 in A and C: {'HOLDS' if p2 else 'FAILS'}"
           + " (" + ", ".join(f"{c} {cs.fmt(*c95[(c, 'L2')][0], p=1)}" for c in ("A", "C") if (c, "L2") in c95) + ")",
           f"- P3 L0 mean per-seed CEP95 ratio - 1 <= +0.10 in A, B, C: {'HOLDS' if p3 else 'FAILS'}"
           + " (" + ", ".join(f"{c} {c95[(c, 'L0')][2]:+.3f}" for c, _ in CONFIGS if (c, "L0") in c95) + ")", "",
           f"Considered updates per ca_tbl_full cell under mode 8: median {np.median(cons) if cons else float('nan'):.0f}, "
           f"min {min(cons) if cons else 0}, max {max(cons) if cons else 0}, over {len(cons)} cells.", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "EKF5_CONSIDER.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
