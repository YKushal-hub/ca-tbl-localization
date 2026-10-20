#!/usr/bin/env python3
"""PRESET-2 (report/PREREG_PRESET2.md): seeds 101-120, configurations A (5 km), B (endgame), C (8-gateway ring).
S = REAL-BEST's S stack in the realistic world (the current campaign reference). N = S + every adoption candidate.
R = N with arbitration V2 off (DEBT-A's ARBOFF set). Primary metric: A(25 m), the design requirement's quantity
(scripts/goal_report.py). Committed with the registration, before any cell; not edited after the first cell.

  P1  N raises ca_tbl_full's five-node A(25 m) against S in B (paired interval above 0)
  P2  N raises ca_tbl_full's tag-0 A(25 m) against S in A (paired interval above 0)
  P3  N lowers ca_tbl_full's five-node A(25 m) in no configuration (every paired lower bound above -0.02)
  P4  N raises ca_tbl_full's subject energy by at most 10 % in every configuration (mean J ratio N/S <= 1.10)
  P5  R against N in B: ca_tbl_full subject J lower (interval below 0) and five-node A(25 m) not lower (lower bound
      above -0.02)

Decision: P1-P4 hold -> N is the preset (kSpecProfile, references regenerated). P5 holds -> arbitration V2,
requester escalation and queue gossip leave the preset.
usage: preset2_analyse.py <outdir>
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
VS = ["S", "N", "R"]
CF = ["A", "B", "C"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_[SNR]", "s*")))
    if len(cells) != 180:
        problems.append(f"{len(cells)} cells, not 180")
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        if not 101 <= int(d.rsplit("s", 1)[1]) <= 120:
            problems.append(f"{d}: seed outside 101-120")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d))[-1]
        if (v in "NR") != ("ekf5 window smoother:" in lg) or "ekf5 relay gate:" not in lg:
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
    md = ["# PRESET-2 — seeds 101-120 (scripts/preset2_analyse.py)", "",
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

    md += ["", "## Paired contrasts, ca_tbl_full", "", "| config | contrast | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |",
           "|---|---|---|---|---|---|"]
    for c in CF:
        for x, y in (("N", "S"), ("R", "N")):
            T, O = (c, x, "ca_tbl_full"), (c, y, "ca_tbl_full")
            md.append(f"| {c} | {x} − {y} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 2), p=1)} | "
                      f"{cs.fmt(*pr(T, O, 3), p=2)} |")
    md += ["", "## CA-TBL − comparator under N, five-node A(25 m) and subject J", "",
           "| config | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |", "|---|---|---|---|---|"]
    for c in CF:
        for comp in ARMS[1:]:
            T, O = (c, "N", "ca_tbl_full"), (c, "N", comp)
            md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 3), p=2)} |")

    def jratio(c):
        n = np.nanmean([v[3] for v in D[(c, "N", "ca_tbl_full")].values()])
        s = np.nanmean([v[3] for v in D[(c, "S", "ca_tbl_full")].values()])
        return n / s if s > 0 else float("nan")

    p1 = pr(("B", "N", "ca_tbl_full"), ("B", "S", "ca_tbl_full"), 1)
    p2 = pr(("A", "N", "ca_tbl_full"), ("A", "S", "ca_tbl_full"), 0)
    p3 = {c: pr((c, "N", "ca_tbl_full"), (c, "S", "ca_tbl_full"), 1) for c in CF}
    p4 = {c: jratio(c) for c in CF}
    p5j = pr(("B", "R", "ca_tbl_full"), ("B", "N", "ca_tbl_full"), 3)
    p5a = pr(("B", "R", "ca_tbl_full"), ("B", "N", "ca_tbl_full"), 1)
    P1 = p1[1] > 0.0
    P2 = p2[1] > 0.0
    P3 = all(p3[c][1] > -0.02 for c in CF)
    P4 = all(p4[c] <= 1.10 for c in CF)
    P5 = p5j[2] < 0.0 and p5a[1] > -0.02
    md += ["", "## Registered predictions", "",
           f"- P1 N raises CA-TBL five-node A(25 m) in B: {'HOLDS' if P1 else 'FAILS'} ({cs.fmt(*p1)})",
           f"- P2 N raises CA-TBL tag-0 A(25 m) in A: {'HOLDS' if P2 else 'FAILS'} ({cs.fmt(*p2)})",
           f"- P3 N lowers CA-TBL five-node A(25 m) nowhere (lower bounds > -0.02): {'HOLDS' if P3 else 'FAILS'} "
           f"({'; '.join(c + ' ' + cs.fmt(*p3[c]) for c in CF)})",
           f"- P4 N's subject J within +10 %: {'HOLDS' if P4 else 'FAILS'} ({'; '.join(f'{c} {p4[c]:.3f}' for c in CF)})",
           f"- P5 R against N in B, J lower and five-node A(25 m) not lower: {'HOLDS' if P5 else 'FAILS'} "
           f"(J {cs.fmt(*p5j, p=2)}; A {cs.fmt(*p5a)})", "",
           f"Decision: N becomes the preset: {'MET' if (P1 and P2 and P3 and P4) else 'NOT MET'}. "
           f"Arbitration V2 leaves the preset: {'MET' if P5 else 'NOT MET'}.", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "PRESET2.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
