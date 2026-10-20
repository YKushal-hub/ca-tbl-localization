#!/usr/bin/env python3
"""INTEG3 (report/PREREG_INTEG3.md): seeds 181-200, configurations A (5 km), B (endgame), C (8-gateway ring), the preset
as it stands (receiver corrections in, DESIGN_DECISIONS 18) plus --ekf5Window=60 and REAL-BEST's stack. Arms ca_tbl_full,
ca_tbl_no_integrity, fixed_threshold, poucet_fusion. Committed with the registration, before any cell; not edited after.

  I1  ca_tbl_no_integrity - ca_tbl_full, five-node A(25 m), in A: paired interval above 0
  I2  ca_tbl_no_integrity - ca_tbl_full, five-node A(25 m), in B and C: lower bounds above -0.02
  I3  ca_tbl_no_integrity - fixed_threshold, five-node A(25 m), in A: the interval contains 0
  I4  ca_tbl_no_integrity - ca_tbl_full, subject J, in A: upper bound below +1.0 J

usage: integ3_analyse.py <outdir>
"""
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell  # noqa: E402

ARMS = ["ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold", "poucet_fusion"]
VS = ["P"]
CF = ["A", "B", "C"]
NCELLS = len(VS) * len(CF) * 20


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources, other_refusals = [], set(), []
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_P", "s*")))
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
        if not 181 <= int(d.rsplit("s", 1)[1]) <= 200:
            problems.append(f"{d}: seed outside 181-200")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        sp = open(os.path.join(d, "spec_profile.txt")).read() if os.path.exists(os.path.join(d, "spec_profile.txt")) else ""
        if "uwbSensitivityDbm=-99.3" not in sp or "iqOverloadOnly=true" not in sp:
            problems.append(f"{d}: the preset's receiver corrections not applied")
        # INTEG3's abort condition (registered): a window refusal in either CA-TBL arm (the first two printed).
        wins = re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)
        if any(int(x) for x in wins[:2]):
            problems.append(f"{d}: window refusal in a CA-TBL arm")
        other_refusals.extend(int(x) for x in wins[2:])
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
    md = ["# INTEG3 — seeds 181-200 (scripts/integ3_analyse.py)", "",
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

    md += ["", "## Paired contrasts against ca_tbl_full", "",
           "| config | arm − ca_tbl_full | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in CF:
        for a in ARMS[1:]:
            T, O = (c, "P", a), (c, "P", "ca_tbl_full")
            md.append(f"| {c} | {a} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 2), p=1)} | {cs.fmt(*pr(T, O, 3), p=2)} |")
    md += ["", "## ca_tbl_no_integrity against the comparators", "",
           "| config | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |", "|---|---|---|---|---|"]
    for c in CF:
        for comp in ("fixed_threshold", "poucet_fusion"):
            T, O = (c, "P", "ca_tbl_no_integrity"), (c, "P", comp)
            md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 3), p=2)} |")
    N, F = "ca_tbl_no_integrity", "ca_tbl_full"
    i1 = pr(("A", "P", N), ("A", "P", F), 1)
    i2 = {c: pr((c, "P", N), (c, "P", F), 1) for c in ("B", "C")}
    i3 = pr(("A", "P", N), ("A", "P", "fixed_threshold"), 1)
    i4 = pr(("A", "P", N), ("A", "P", F), 3)
    I1, I2 = i1[1] > 0.0, all(i2[c][1] > -0.02 for c in i2)
    I3, I4 = i3[1] <= 0.0 <= i3[2], i4[2] < 1.0
    md += ["", "## Registered predictions", "",
           f"- I1 no-integrity raises five-node A(25 m) in A: {'HOLDS' if I1 else 'FAILS'} ({cs.fmt(*i1)})",
           f"- I2 no-integrity lowers five-node A(25 m) in neither B nor C (lower bounds > -0.02): {'HOLDS' if I2 else 'FAILS'} "
           f"({'; '.join(c + ' ' + cs.fmt(*i2[c]) for c in i2)})",
           f"- I3 no-integrity level with fixed_threshold in A (interval contains 0): {'HOLDS' if I3 else 'FAILS'} ({cs.fmt(*i3)})",
           f"- I4 no-integrity costs no material energy in A (upper bound < +1.0 J): {'HOLDS' if I4 else 'FAILS'} ({cs.fmt(*i4, p=2)})", ""]
    md += [f"Window refusals in the two comparator arms (reported, not an abort): {sum(other_refusals)} in "
           f"{len(other_refusals)} arm-runs", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "INTEG3.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
