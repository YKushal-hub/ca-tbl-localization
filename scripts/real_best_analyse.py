#!/usr/bin/env python3
"""REAL-BEST (report/PREREG_REAL_BEST.md): out of sample, seeds 21-40. The realistic world (0.15 deg/s gyro, 5 % stride
noise, 2 m finder GNSS error, ekf5, magnetometer) as given (M), with finder consider and the relay gate (S), and with
state bounds added (F). Committed with the registration, before any cell; not edited after the first cell.

  P1  F lowers ca_tbl_full's tag 0 CEP95 against M in B (paired interval below 0)
  P2  F costs CA-TBL no availability against M: lower bound above -0.01 in A and in B
  P3  under F in B: CA-TBL - poucet_fusion CEP95 interval contains 0, and its availability interval is above 0
  P4  under F, no comparator is significantly more accurate than CA-TBL in A or B (no CA-TBL - comparator CEP95
      interval entirely above 0)
  P5  F lowers CA-TBL's tag 0 CEP95 against S in A (paired interval below 0)

usage: real_best_analyse.py <outdir>
"""
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
from v3_analyse import node_rows  # noqa: E402

ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "poucet_fusion"]
VS = ["M", "S", "F"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_[MSF]", "s*")))
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
        seed = int(d.rsplit("s", 1)[1])
        if not 21 <= seed <= 40:
            problems.append(f"{d}: seed outside 21-40")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d))[-1]
        if (v in "SF") != ("ekf5 relay gate:" in lg) or (v == "F") != ("ekf5 state bounds:" in lg):
            problems.append(f"{d}: variant lines do not match the cell")
        if "magnetometer heading reference" not in lg or "stride noise 5 %" not in lg:
            problems.append(f"{d}: realistic-world lines missing")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D = {(c, v, a): per_seed(out, f"{c}_{v}", a) for c in "AB" for v in VS for a in ARMS}
    for k, x in D.items():
        if len(x) != 20:
            problems.append(f"{k}: {len(x)} seeds, not 20")
    md = ["# REAL-BEST — out of sample, seeds 21-40 (scripts/real_best_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", "",
          "| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | subject J |", "|---|---|---|---|---|---|---|"]
    for c in "AB":
        for v in VS:
            for a in ARMS:
                x = D[(c, v, a)]
                if not x:
                    continue
                C = []
                for d in cs.seeds(out, f"{c}_{v}"):
                    C += node_rows(d, a)[1]
                md.append(f"| {c} | {v} | {a} | {np.mean([y['av'] for y in x.values()]):.3f} | "
                          f"{np.mean([y['c95'] for y in x.values()]):.1f} ({np.median([y['c95'] for y in x.values()]):.1f}) | "
                          f"{(np.mean(C) if C else float('nan')):.3f} | {np.mean([y['J'] for y in x.values()]):.2f} |")

    def pr(A, B, k):
        a, b = D[A], D[B]
        s = sorted(set(a) & set(b))
        return cs.ci([a[i][k] - b[i][k] for i in s]) if s else (float("nan"),) * 3

    md += ["", "## Variants, ca_tbl_full", "", "| config | contrast | Δ CEP95 tag 0 | Δ availability |", "|---|---|---|---|"]
    for c in "AB":
        for x, y in (("S", "M"), ("F", "M"), ("F", "S")):
            md.append(f"| {c} | {x} − {y} | {cs.fmt(*pr((c, x, 'ca_tbl_full'), (c, y, 'ca_tbl_full'), 'c95'), p=1)} | "
                      f"{cs.fmt(*pr((c, x, 'ca_tbl_full'), (c, y, 'ca_tbl_full'), 'av'))} |")
    md += ["", "## CA-TBL − comparator under F, paired by seed", "",
           "| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
    above, full = [], 0
    for c in "AB":
        for comp in ARMS[1:]:
            T, O = (c, "F", "ca_tbl_full"), (c, "F", comp)
            full += len(set(D[T]) & set(D[O])) == 20
            dc = pr(T, O, "c95")
            if dc[1] > 0.0:
                above.append(f"{c} {comp}")
            md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 'av'))} | {cs.fmt(*dc, p=1)} | {cs.fmt(*pr(T, O, 'J'), p=2)} |")
    p1d = pr(("B", "F", "ca_tbl_full"), ("B", "M", "ca_tbl_full"), "c95")
    p2 = {c: pr((c, "F", "ca_tbl_full"), (c, "M", "ca_tbl_full"), "av") for c in "AB"}
    p3c = pr(("B", "F", "ca_tbl_full"), ("B", "F", "poucet_fusion"), "c95")
    p3a = pr(("B", "F", "ca_tbl_full"), ("B", "F", "poucet_fusion"), "av")
    p5d = pr(("A", "F", "ca_tbl_full"), ("A", "S", "ca_tbl_full"), "c95")
    P1 = p1d[2] < 0.0
    P2 = all(p2[c][1] > -0.01 for c in "AB")
    P3 = p3c[1] <= 0.0 <= p3c[2] and p3a[1] > 0.0
    P4 = full == 8 and not above
    P5 = p5d[2] < 0.0
    md += ["", "## Registered predictions", "",
           f"- P1 F lowers CA-TBL CEP95 against M in B: {'HOLDS' if P1 else 'FAILS'} ({cs.fmt(*p1d, p=1)})",
           f"- P2 F costs CA-TBL no availability against M (lower bound > -0.01), A and B: {'HOLDS' if P2 else 'FAILS'} "
           f"(A {cs.fmt(*p2['A'])}; B {cs.fmt(*p2['B'])})",
           f"- P3 under F in B: CA-TBL - Poucet CEP95 contains 0, availability above 0: {'HOLDS' if P3 else 'FAILS'} "
           f"(CEP95 {cs.fmt(*p3c, p=1)}; availability {cs.fmt(*p3a)})",
           f"- P4 under F no comparator significantly more accurate than CA-TBL, A and B: {'HOLDS' if P4 else 'FAILS'}"
           + f" ({full} of 8 contrasts with 20 paired seeds)"
           + (f" (above 0: {', '.join(above)})" if above else ""),
           f"- P5 F lowers CA-TBL CEP95 against S in A: {'HOLDS' if P5 else 'FAILS'} ({cs.fmt(*p5d, p=1)})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REAL_BEST.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
