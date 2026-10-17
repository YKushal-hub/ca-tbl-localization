#!/usr/bin/env python3
"""D8-MAIN (report/PREREG_D8_MAIN.md): tier 2's committed sensitivity table (off) against the
SX1280 812.5 kHz row (on), on the preset at A (5 km), B (endgame) and C (N8 ring).
Committed with the registration, before any cell; not edited after the first cell.

  R1  off cells' ca_tbl_full arm_summary rows == FIXES-N26 ALL_{main,end,n8} rows, 60 of 60
  P1  static_2p4 availability rises in A, B and C (paired interval above 0)
  P2  lora_fusion availability rises in A and B
  P3  ca_tbl_full availability rises in A
  Reported: every arm's paired change; the comparison table's contrasts CA-TBL - lora_fusion
  and CA-TBL - poucet_fusion under off and under on; UWB-delivered fixes.

Estimators are main's own: lora_cmp_analyse.per_seed, camp_spec_analyse.ci (fixed-seed
bootstrap), camp_final_analyse.uwb_fixes.

usage: d8_main_analyse.py <out> [<fixes_n26 eval dir>]
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

CONFIGS = [("A", "5 km", "ALL_main"), ("B", "endgame", "ALL_end"), ("C", "N8 ring", "ALL_n8")]
ARMS = ["ca_tbl_full", "static_868", "static_2p4", "fixed_threshold", "poucet_fusion", "lora_fusion"]
SEEDS = list(range(1, 21))
Q = [("av", "availability", 3), ("c50", "CEP50 tag 0 (m)", 1), ("c95", "CEP95 tag 0 (m)", 1),
     ("s95", "CEP95 subjects (m)", 1), ("tff", "TTFF (s)", 0), ("J", "subject J", 2)]


def checks(out, problems):
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_o*", "s*")))
    if len(cells) != 120:
        problems.append(f"{len(cells)} cells, not 120")
    sources = set()
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        rid = open(os.path.join(d, "run_id.txt")).read()
        m = re.search(r"^source (\S+) dirty (\d+)", rid, re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        on = os.path.basename(os.path.dirname(d)).endswith("_on")
        has = "D-8:" in open(os.path.join(d, "run.log"), errors="replace").read()
        if on != has:
            problems.append(f"{d}: {'on cell without' if on else 'off cell with'} the D-8 line")
    if len(sources) > 1:
        problems.append(f"more than one source across the campaign: {sorted(sources)}")
    return sources


def row_or_none(d, arm):
    return cs.arm_row(d, arm) if os.path.exists(os.path.join(d, "arm_summary.csv")) else None


def r1(out, n26, md):
    md += ["## R1 — off cells reproduce FIXES-N26's ALL cells (ca_tbl_full row)", ""]
    if not n26 or not os.path.isdir(n26):
        md += ["FIXES-N26 eval directory not given or absent: R1 NOT CHECKED.", ""]
        return None
    same = differ = missing = 0
    for key, _, ref in CONFIGS:
        for s in SEEDS:
            a = row_or_none(os.path.join(out, f"{key}_off", f"s{s}"), "ca_tbl_full")
            b = row_or_none(os.path.join(n26, ref, f"s{s}"), "ca_tbl_full")
            if a is None or b is None:
                missing += 1
            elif a == b:
                same += 1
            else:
                differ += 1
                if differ <= 3:
                    k = [c for c in a if a[c] != b.get(c)]
                    md.append(f"- {key} s{s} differs in {len(k)} columns, e.g. {k[:4]}")
    held = differ == 0 and missing == 0
    md += [f"identical {same}, differing {differ}, missing {missing} -> R1 {'HOLDS' if held else 'FAILS'}", ""]
    return held


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    n26 = sys.argv[2] if len(sys.argv) > 2 else "results/fixes_n26_2026-10-05/eval"
    problems = []
    sources = checks(out, problems)
    md = ["# D8-MAIN — tier 2's committed table against the SX1280 812.5 kHz row (scripts/d8_main_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    held_r1 = r1(out, n26, md)
    rows, verdict = [], {}
    for key, title, _ in CONFIGS:
        data = {(c, a): per_seed(out, f"{key}_{c}", a) for c in ("off", "on") for a in ARMS}
        for (c, a), v in data.items():
            if len(v) != 20:
                problems.append(f"{key} {c} {a}: {len(v)} seeds, not 20")
        md += [f"## {key} — {title}", "",
               "| arm | availability off → on | Δ availability | Δ CEP95 tag 0 (m) | Δ CEP95 subjects (m) | Δ TTFF (s) | Δ subject J |",
               "|---|---|---|---|---|---|---|"]
        for a in ARMS:
            off, on = data[("off", a)], data[("on", a)]
            s = sorted(set(off) & set(on))
            if not s:
                continue
            d = {k: cs.ci([on[i][k] - off[i][k] for i in s]) for k, _, _ in Q}
            mo, mn = np.mean([off[i]["av"] for i in s]), np.mean([on[i]["av"] for i in s])
            md.append(f"| {a} | {mo:.3f} → {mn:.3f} | {cs.fmt(*d['av'])} | {cs.fmt(*d['c95'], p=1)} | "
                      f"{cs.fmt(*d['s95'], p=1)} | {cs.fmt(*d['tff'], p=0)} | {cs.fmt(*d['J'], p=2)} |")
            verdict[(key, a)] = d["av"]
            for k, name, _ in Q:
                rows.append(dict(config=key, arm=a, quantity=k, off=f"{np.nanmean([off[i][k] for i in s]):.6g}",
                                 on=f"{np.nanmean([on[i][k] for i in s]):.6g}", diff=f"{d[k][0]:.6g}",
                                 lo=f"{d[k][1]:.6g}", hi=f"{d[k][2]:.6g}"))
        u = {c: [uwb_fixes(dd, int(dd.rsplit("s", 1)[1])) for dd in cs.seeds(out, f"{key}_{c}")] for c in ("off", "on")}
        md += ["", f"UWB-delivered fixes, ca_tbl_full: off {sum(u['off'])} ({sum(x > 0 for x in u['off'])} seeds), "
                   f"on {sum(u['on'])} ({sum(x > 0 for x in u['on'])} seeds).", "",
               "Comparison-table contrasts, paired by seed, CA-TBL − comparator:", "",
               "| contrast | condition | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
        for comp in ("lora_fusion", "poucet_fusion"):
            for c in ("off", "on"):
                t, o = data[(c, "ca_tbl_full")], data[(c, comp)]
                s = sorted(set(t) & set(o))
                if not s:
                    continue
                f = lambda k, p: cs.fmt(*cs.ci([t[i][k] - o[i][k] for i in s]), p=p)  # noqa: E731
                md.append(f"| CA-TBL − {comp} | {c} | {f('av', 3)} | {f('c95', 1)} | {f('J', 2)} |")
        md.append("")
    above = lambda key, a: (key, a) in verdict and verdict[(key, a)][1] > 0.0  # noqa: E731
    p1 = all(above(k, "static_2p4") for k in "ABC")
    p2 = all(above(k, "lora_fusion") for k in "AB")
    p3 = above("A", "ca_tbl_full")
    md += ["## Registered predictions", "",
           f"- R1 off cells reproduce FIXES-N26 ALL: {'HOLDS' if held_r1 else ('NOT CHECKED' if held_r1 is None else 'FAILS')}",
           f"- P1 static_2p4 availability rises in A, B, C: {'HOLDS' if p1 else 'FAILS'}",
           f"- P2 lora_fusion availability rises in A, B: {'HOLDS' if p2 else 'FAILS'}",
           f"- P3 ca_tbl_full availability rises in A: {'HOLDS' if p3 else 'FAILS'}", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    if rows:
        with open(os.path.join(out, "d8_main_table.csv"), "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
            w.writeheader()
            w.writerows(rows)
    open(os.path.join(out, "D8_MAIN.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
