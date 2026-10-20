#!/usr/bin/env python3
"""ANCHOR-SUPPLY (report/PREREG_ANCHOR_SUPPLY.md): seeds 121-140, configurations A (5 km) and B (endgame).
Base: REAL-BEST's S stack in the realistic world plus --ekf5Window=60. Variants: finder tasking roundrobin (R) or
pairs (P), with 4 or 8 finders. R4 is the brief's deployment and the reference. Primary metric: A(25 m), the design
requirement's quantity (scripts/goal_report.py). Committed with the registration, before any cell; not edited after
the first cell.

  Q1  P4 does not raise ca_tbl_full's tag-0 A(25 m) against R4 in A (paired upper bound below +0.02): with four
      finders, pairs leaves three of five subjects without one
  Q2  R8 raises ca_tbl_full's five-node A(25 m) against R4 in A (paired interval above 0)
  Q3  R8 raises ca_tbl_full's five-node A(25 m) against R4 in B (paired interval above 0)
  Q4  P8 raises ca_tbl_full's five-node A(25 m) against R8 in B (paired interval above 0)
  Q5  in no variant does ca_tbl_full's mean five-node A(25 m) reach 0.95 in A
  Q6  in every configuration and variant ca_tbl_full's subject J is below poucet_fusion's (paired interval below 0)

Reported, not predicted: every variant's A(25 m) for every arm, the share of runs meeting 0.95, and CA-TBL against
each comparator per variant.
usage: anchor_analyse.py <outdir>
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
VS = ["R4", "P4", "R8", "P8"]
CF = ["A", "B"]
NCELLS = len(VS) * len(CF) * 20


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_[RP][48]", "s*")))
    if len(cells) != NCELLS:
        problems.append(f"{len(cells)} cells, not {NCELLS}")
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        if not 121 <= int(d.rsplit("s", 1)[1]) <= 140:
            problems.append(f"{d}: seed outside 121-140")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d)).split("_")[1]
        kind = "pairs" if v[0] == "P" else "roundrobin"
        lines = re.findall(rf"MS-1 tasking \({kind}\):([^\n]*)", lg)
        if not lines or any(len(re.findall(r"finder \d+ -> subject", x)) != int(v[1]) for x in lines):
            problems.append(f"{d}: tasking lines do not match {v}")
        if "ekf5 window smoother:" not in lg or "ekf5 relay gate:" not in lg:
            problems.append(f"{d}: base lines missing")
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
    md = ["# ANCHOR-SUPPLY — seeds 121-140 (scripts/anchor_analyse.py)", "",
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

    md += ["", "## Paired contrasts against R4, ca_tbl_full", "",
           "| config | contrast | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in CF:
        for x in ("P4", "R8", "P8"):
            T, O = (c, x, "ca_tbl_full"), (c, "R4", "ca_tbl_full")
            md.append(f"| {c} | {x} − R4 | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 2), p=1)} | "
                      f"{cs.fmt(*pr(T, O, 3), p=2)} |")
    md += ["", "## CA-TBL − comparator, per variant", "",
           "| config | variant | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in CF:
        for v in VS:
            for comp in ARMS[1:]:
                T, O = (c, v, "ca_tbl_full"), (c, v, comp)
                md.append(f"| {c} | {v} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 3), p=2)} |")

    def mean5(c, v):
        x = D[(c, v, "ca_tbl_full")]
        return float(np.mean([r[1] for r in x.values()])) if x else float("nan")

    q1 = pr(("A", "P4", "ca_tbl_full"), ("A", "R4", "ca_tbl_full"), 0)
    q2 = pr(("A", "R8", "ca_tbl_full"), ("A", "R4", "ca_tbl_full"), 1)
    q3 = pr(("B", "R8", "ca_tbl_full"), ("B", "R4", "ca_tbl_full"), 1)
    q4 = pr(("B", "P8", "ca_tbl_full"), ("B", "R8", "ca_tbl_full"), 1)
    q5 = {v: mean5("A", v) for v in VS}
    q6 = {(c, v): pr((c, v, "ca_tbl_full"), (c, v, "poucet_fusion"), 3) for c in CF for v in VS}
    Q1, Q2, Q3, Q4 = q1[2] < 0.02, q2[1] > 0.0, q3[1] > 0.0, q4[1] > 0.0
    Q5 = all(q5[v] < 0.95 for v in VS)
    Q6 = all(q6[k][2] < 0.0 for k in q6)
    md += ["", "## Registered predictions", "",
           f"- Q1 P4 does not raise CA-TBL tag-0 A(25 m) in A (upper bound < +0.02): {'HOLDS' if Q1 else 'FAILS'} ({cs.fmt(*q1)})",
           f"- Q2 R8 raises CA-TBL five-node A(25 m) in A: {'HOLDS' if Q2 else 'FAILS'} ({cs.fmt(*q2)})",
           f"- Q3 R8 raises CA-TBL five-node A(25 m) in B: {'HOLDS' if Q3 else 'FAILS'} ({cs.fmt(*q3)})",
           f"- Q4 P8 raises CA-TBL five-node A(25 m) against R8 in B: {'HOLDS' if Q4 else 'FAILS'} ({cs.fmt(*q4)})",
           f"- Q5 no variant reaches mean five-node A(25 m) 0.95 in A: {'HOLDS' if Q5 else 'FAILS'} "
           f"({'; '.join(f'{v} {q5[v]:.3f}' for v in VS)})",
           f"- Q6 CA-TBL subject J below Poucet's everywhere: {'HOLDS' if Q6 else 'FAILS'} "
           f"(worst upper bound {max(q6[k][2] for k in q6):.2f} J)", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "ANCHOR_SUPPLY.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
