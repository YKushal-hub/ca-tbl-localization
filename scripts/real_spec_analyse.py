#!/usr/bin/env python3
"""REAL-SPEC (report/PREREG_REAL_SPEC.md): out of sample, seeds 41-60. The realistic world under the proposed stack (S:
ekf5, magnetometer, finder consider, relay gate; the preset's integrityChi2 1992.9) against the same with the
Switching-Point Integrity Check at its specified chi^2_0.99 = 9.21 (N). Committed with the registration, before any
cell; not edited after the first cell.

  P1  N raises CA-TBL's availability against S in B (paired interval above 0), and costs none in A (lower bound > -0.01)
  P2  N costs CA-TBL at most 10 % in CEP95: mean per-seed ratio N / S - 1 <= +0.10 in A and in B
  P3  under N, no comparator has significantly higher availability than CA-TBL in A or B
  P4  under N, no comparator is significantly more accurate than CA-TBL in A or B
  P5  under N, CA-TBL's availability exceeds poucet_fusion's in B (paired interval above 0)

usage: real_spec_analyse.py <outdir>
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
VS = ["S", "N"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_[SN]", "s*")))
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
        seed = int(d.rsplit("s", 1)[1])
        if not 41 <= seed <= 60:
            problems.append(f"{d}: seed outside 41-60")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d))[-1]
        if "ekf5 relay gate:" not in lg or "consider parameters" not in lg:
            problems.append(f"{d}: stack lines missing")
        argv = open(os.path.join(d, "run_id.txt")).read()
        if (v == "N") != ("--integrityChi2=9.21" in argv):
            problems.append(f"{d}: integrityChi2 does not match the cell")
        if "magnetometer heading reference" not in lg or "stride noise 5 %" not in lg:
            problems.append(f"{d}: realistic-world lines missing")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D = {(c, v, a): per_seed(out, f"{c}_{v}", a) for c in "AB" for v in VS for a in ARMS}
    for k, x in D.items():
        if len(x) != 20:
            problems.append(f"{k}: {len(x)} seeds, not 20")
    md = ["# REAL-SPEC — out of sample, seeds 41-60 (scripts/real_spec_analyse.py)", "",
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

    md += ["", "## N − S, ca_tbl_full", "", "| config | Δ CEP95 tag 0 | mean per-seed ratio − 1 | Δ availability |", "|---|---|---|---|"]
    rat = {}
    for c in "AB":
        a, b = D[(c, "N", "ca_tbl_full")], D[(c, "S", "ca_tbl_full")]
        s = sorted(set(a) & set(b))
        rat[c] = float(np.mean([a[i]["c95"] / b[i]["c95"] - 1.0 for i in s])) if s else float("nan")
        md.append(f"| {c} | {cs.fmt(*pr((c, 'N', 'ca_tbl_full'), (c, 'S', 'ca_tbl_full'), 'c95'), p=1)} | {rat[c]:+.3f} | "
                  f"{cs.fmt(*pr((c, 'N', 'ca_tbl_full'), (c, 'S', 'ca_tbl_full'), 'av'))} |")
    md += ["", "## CA-TBL − comparator under N, paired by seed", "",
           "| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
    acc_above, av_below, full = [], [], 0
    for c in "AB":
        for comp in ARMS[1:]:
            T, O = (c, "N", "ca_tbl_full"), (c, "N", comp)
            full += len(set(D[T]) & set(D[O])) == 20
            dc, da = pr(T, O, "c95"), pr(T, O, "av")
            if dc[1] > 0.0:
                acc_above.append(f"{c} {comp}")
            if da[2] < 0.0:
                av_below.append(f"{c} {comp}")
            md.append(f"| {c} | {comp} | {cs.fmt(*da)} | {cs.fmt(*dc, p=1)} | {cs.fmt(*pr(T, O, 'J'), p=2)} |")
    p1b = pr(("B", "N", "ca_tbl_full"), ("B", "S", "ca_tbl_full"), "av")
    p1a = pr(("A", "N", "ca_tbl_full"), ("A", "S", "ca_tbl_full"), "av")
    p5 = pr(("B", "N", "ca_tbl_full"), ("B", "N", "poucet_fusion"), "av")
    P1 = p1b[1] > 0.0 and p1a[1] > -0.01
    P2 = all(rat[c] <= 0.10 for c in "AB")
    P3 = full == 8 and not av_below
    P4 = full == 8 and not acc_above
    P5 = p5[1] > 0.0
    md += ["", "## Registered predictions", "",
           f"- P1 N raises CA-TBL availability in B, costs none in A: {'HOLDS' if P1 else 'FAILS'} (B {cs.fmt(*p1b)}; A {cs.fmt(*p1a)})",
           f"- P2 N costs CA-TBL at most +10 % CEP95 (mean per-seed ratio), A and B: {'HOLDS' if P2 else 'FAILS'} (A {rat['A']:+.3f}; B {rat['B']:+.3f})",
           f"- P3 under N no comparator significantly higher availability, A and B: {'HOLDS' if P3 else 'FAILS'}"
           + (f" (below 0: {', '.join(av_below)})" if av_below else "") + f" ({full} of 8 complete)",
           f"- P4 under N no comparator significantly more accurate, A and B: {'HOLDS' if P4 else 'FAILS'}"
           + (f" (above 0: {', '.join(acc_above)})" if acc_above else ""),
           f"- P5 under N CA-TBL availability above Poucet in B: {'HOLDS' if P5 else 'FAILS'} ({cs.fmt(*p5)})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REAL_BEST.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
