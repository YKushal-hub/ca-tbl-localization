#!/usr/bin/env python3
"""PRESET-3 (report/PREREG_PRESET3.md): seeds 241-260, configurations A (5 km), B (endgame), C (8-gateway ring). NEW =
the preset after DESIGN_DECISIONS 18-22; OLD = NEW with decisions 19, 21, 22 pinned back (DD 20's realistic world and
DD 18's receiver kept). Primary: A(25 m) (scripts/goal_report.py). Committed with the registration, before any cell.

  P1  NEW raises ca_tbl_full's five-node A(25 m) against OLD in B (paired interval above 0)
  P2  NEW raises ca_tbl_full's tag-0 A(25 m) against OLD in A (paired interval above 0)
  P3  NEW's ca_tbl_full mean five-node A(25 m) in B is at least 0.95
  P4  NEW lowers ca_tbl_full's five-node A(25 m) in no configuration (every paired lower bound above -0.02)
  P5  NEW's ca_tbl_full subject J is at most 1.10 x OLD's in every configuration

usage: preset3_analyse.py <outdir>
"""
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell  # noqa: E402

ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "poucet_fusion"]
VS = ["OLD", "NEW"]
CF = ["A", "B", "C"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_[ON][LE][DW]", "s*")))
    if len(cells) != 120:
        problems.append(f"{len(cells)} cells, not 120")
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        if not 241 <= int(d.rsplit("s", 1)[1]) <= 260:
            problems.append(f"{d}: seed outside 241-260")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d)).split("_")[1]
        new = (v == "NEW")
        if new != ("ekf5 window smoother:" in lg) or "ekf5 relay gate:" not in lg or \
                new != ("MS-1 tasking (pairs)" in lg):
            problems.append(f"{d}: variant lines do not match the cell")
        if "magnetometer heading reference" not in lg or "stride noise 5 %" not in lg:
            problems.append(f"{d}: realistic-world lines missing")
        if any(int(x) for x in re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)):
            problems.append(f"{d}: window solver failures")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D = {}
    for c in CF:
        for v in VS:
            for a in ARMS:
                D[(c, v, a)] = {}
                for d in sorted(glob.glob(os.path.join(out, f"{c}_{v}", "s*"))):
                    if cs.ok(d):
                        r = cell(d, a)
                        if r:
                            D[(c, v, a)][int(d.rsplit("s", 1)[1])] = r
                if len(D[(c, v, a)]) != 20:
                    problems.append(f"{(c, v, a)}: {len(D[(c, v, a)])} seeds, not 20")
    md = ["# PRESET-3 — seeds 241-260 (scripts/preset3_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", "",
          "| config | variant | arm | A(25 m) tag 0 | A(25 m) five-node | runs >= 0.95 (five-node) | CEP95 tag 0 (m) | subject J |",
          "|---|---|---|---|---|---|---|---|"]
    for c in CF:
        for v in VS:
            for a in ARMS:
                x = D[(c, v, a)]
                if not x:
                    continue
                arr = np.array(list(x.values()))
                md.append(f"| {c} | {v} | {a} | {arr[:, 0].mean():.3f} | {arr[:, 1].mean():.3f} | {(arr[:, 1] >= 0.95).mean():.2f} | "
                          f"{np.nanmean(arr[:, 2]):.1f} | {np.nanmean(arr[:, 3]):.2f} |")

    def pr(A, B, k):
        a, b = D[A], D[B]
        s = sorted(set(a) & set(b))
        return cs.ci([a[i][k] - b[i][k] for i in s]) if s else (float("nan"),) * 3

    md += ["", "## Paired contrasts NEW - OLD, every arm", "",
           "| config | arm | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in CF:
        for arm in ARMS:
            T, O = (c, "NEW", arm), (c, "OLD", arm)
            md.append(f"| {c} | {arm} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 2), p=1)} | "
                      f"{cs.fmt(*pr(T, O, 3), p=2)} |")
    md += ["", "## CA-TBL - comparator under NEW", "",
           "| config | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |", "|---|---|---|---|---|"]
    for c in CF:
        for comp in ARMS[1:]:
            T, O = (c, "NEW", "ca_tbl_full"), (c, "NEW", comp)
            md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 3), p=2)} |")

    def mean5(c, v):
        x = D[(c, v, "ca_tbl_full")]
        return float(np.mean([r[1] for r in x.values()])) if x else float("nan")

    def jratio(c):
        n = np.nanmean([v[3] for v in D[(c, "NEW", "ca_tbl_full")].values()])
        o = np.nanmean([v[3] for v in D[(c, "OLD", "ca_tbl_full")].values()])
        return n / o if o > 0 else float("nan")

    p1 = pr(("B", "NEW", "ca_tbl_full"), ("B", "OLD", "ca_tbl_full"), 1)
    p2 = pr(("A", "NEW", "ca_tbl_full"), ("A", "OLD", "ca_tbl_full"), 0)
    p3 = mean5("B", "NEW")
    p4 = {c: pr((c, "NEW", "ca_tbl_full"), (c, "OLD", "ca_tbl_full"), 1) for c in CF}
    p5 = {c: jratio(c) for c in CF}
    P1, P2, P3 = p1[1] > 0.0, p2[1] > 0.0, p3 >= 0.95
    P4 = all(p4[c][1] > -0.02 for c in CF)
    P5 = all(p5[c] <= 1.10 for c in CF)
    md += ["", "## Registered predictions", "",
           f"- P1 NEW raises CA-TBL five-node A(25 m) in B: {'HOLDS' if P1 else 'FAILS'} ({cs.fmt(*p1)})",
           f"- P2 NEW raises CA-TBL tag-0 A(25 m) in A: {'HOLDS' if P2 else 'FAILS'} ({cs.fmt(*p2)})",
           f"- P3 NEW's CA-TBL mean five-node A(25 m) in B >= 0.95: {'HOLDS' if P3 else 'FAILS'} ({p3:.3f})",
           f"- P4 NEW lowers CA-TBL five-node A(25 m) nowhere (lower bounds > -0.02): {'HOLDS' if P4 else 'FAILS'} "
           f"({'; '.join(c + ' ' + cs.fmt(*p4[c]) for c in CF)})",
           f"- P5 NEW's subject J within +10 % of OLD's: {'HOLDS' if P5 else 'FAILS'} ({'; '.join(f'{c} {p5[c]:.3f}' for c in CF)})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "PRESET3.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
