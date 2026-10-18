#!/usr/bin/env python3
"""REAL-STACK (report/PREREG_REAL_STACK.md): the realistic world (0.15 deg/s gyro, 5 % stride noise, 2 m finder GNSS
error) under ekf5 + magnetometer (M) and with finder offsets as consider parameters and the relay gate added (S),
paired by seed. Committed with the registration, before any cell; not edited after the first cell.

  P1  S lowers ca_tbl_full's tag 0 CEP95 against M in B (paired interval below 0)
  P2  S costs CA-TBL no availability: S - M paired interval lower bound above -0.01, in A and in B
  P3  under S, CA-TBL's availability exceeds poucet_fusion's in B (paired interval above 0)
  P4  under S, CA-TBL - poucet_fusion tag 0 CEP95 in B: the interval contains 0 (no significant difference)

Estimators: lora_cmp_analyse.per_seed, camp_spec_analyse.ci, v3_analyse.node_rows.
usage: real_stack_analyse.py <outdir>
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


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_[MS]", "s*")))
    if len(cells) != 80:
        problems.append(f"{len(cells)} cells, not 80")
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        isS = "_S/" in d + "/"
        if isS != ("ekf5 relay gate:" in lg) or isS != ("consider parameters" in lg):
            problems.append(f"{d}: variant lines do not match the cell")
        if "magnetometer heading reference" not in lg or "stride noise 5 %" not in lg:
            problems.append(f"{d}: realistic-world lines missing")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D = {(c, v, a): per_seed(out, f"{c}_{v}", a) for c in "AB" for v in "MS" for a in ARMS}
    for k, x in D.items():
        if len(x) != 20:
            problems.append(f"{k}: {len(x)} seeds, not 20")
    md = ["# REAL-STACK — the realistic world, with and without the consistency stack (scripts/real_stack_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", "",
          "| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | subject J |", "|---|---|---|---|---|---|---|"]
    for c in "AB":
        for v in "MS":
            for a in ARMS:
                x = D[(c, v, a)]
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

    md += ["", "## S − M, ca_tbl_full", "", "| config | Δ CEP95 tag 0 | Δ availability |", "|---|---|---|"]
    for c in "AB":
        md.append(f"| {c} | {cs.fmt(*pr((c, 'S', 'ca_tbl_full'), (c, 'M', 'ca_tbl_full'), 'c95'), p=1)} | "
                  f"{cs.fmt(*pr((c, 'S', 'ca_tbl_full'), (c, 'M', 'ca_tbl_full'), 'av'))} |")
    md += ["", "## CA-TBL − comparator under S, paired by seed", "",
           "| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
    for c in "AB":
        for comp in ARMS[1:]:
            T, O = (c, "S", "ca_tbl_full"), (c, "S", comp)
            md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 'av'))} | {cs.fmt(*pr(T, O, 'c95'), p=1)} | {cs.fmt(*pr(T, O, 'J'), p=2)} |")
    p1d = pr(("B", "S", "ca_tbl_full"), ("B", "M", "ca_tbl_full"), "c95")
    p2 = {c: pr((c, "S", "ca_tbl_full"), (c, "M", "ca_tbl_full"), "av") for c in "AB"}
    p3d = pr(("B", "S", "ca_tbl_full"), ("B", "S", "poucet_fusion"), "av")
    p4d = pr(("B", "S", "ca_tbl_full"), ("B", "S", "poucet_fusion"), "c95")
    P1 = p1d[2] < 0.0
    P2 = all(p2[c][1] > -0.01 for c in "AB")
    P3 = p3d[1] > 0.0
    P4 = p4d[1] <= 0.0 <= p4d[2]
    md += ["", "## Registered predictions", "",
           f"- P1 S lowers CA-TBL CEP95 against M in B: {'HOLDS' if P1 else 'FAILS'} ({cs.fmt(*p1d, p=1)})",
           f"- P2 S costs CA-TBL no availability (lower bound > -0.01), A and B: {'HOLDS' if P2 else 'FAILS'} "
           f"(A {cs.fmt(*p2['A'])}; B {cs.fmt(*p2['B'])})",
           f"- P3 under S, CA-TBL availability above Poucet in B: {'HOLDS' if P3 else 'FAILS'} ({cs.fmt(*p3d)})",
           f"- P4 under S, CA-TBL - Poucet CEP95 in B contains 0: {'HOLDS' if P4 else 'FAILS'} ({cs.fmt(*p4d, p=1)})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REAL_STACK.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
