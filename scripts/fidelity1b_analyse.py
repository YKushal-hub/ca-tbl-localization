#!/usr/bin/env python3
"""FIDELITY-1b (report/PREREG_FIDELITY1B.md): seeds 161-180, configurations A (5 km), B (endgame), C (8-gateway ring).
Base: REAL-BEST's S stack in the realistic world plus the registered base (N or W, RUN.md). F0 = the base; F1 = the
base + --uwbSensitivityDbm=-99.3 (DW3000 data sheet) + --iqOverloadOnly=true (no double count of receiver
impairments against data-sheet sensitivities). Adoption does not depend on these numbers (the registration says
why); they measure the size of the change. Committed with the registration, before any cell; not edited after the
first cell.

  R1  F1 lowers ca_tbl_full's five-node A(25 m) in no configuration (every paired lower bound above -0.03)
  R2  F1 changes ca_tbl_full's tag-0 A(25 m) in B by less than 0.02 either way (paired interval inside [-0.02, 0.02])
  R3  F1 lowers ca_tbl_full's subject J in A (paired interval below 0): more margin, fewer escalations and retries

usage: fidelity1b_analyse.py <outdir>
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
VS = ["F0", "F1"]
CF = ["A", "B", "C"]
NCELLS = len(VS) * len(CF) * 20


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources, other_refusals = [], set(), []
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_F[01]", "s*")))
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
        if not 161 <= int(d.rsplit("s", 1)[1]) <= 180:
            problems.append(f"{d}: seed outside 161-180")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d)).split("_")[1]
        cl = open(os.path.join(d, "run_id.txt")).read()
        if (v == "F1") != ("--iqOverloadOnly=true" in cl and "--uwbSensitivityDbm=-99.3" in cl) or \
                (v == "F0") != ("--iqOverloadOnly=false" in cl and "--uwbSensitivityDbm=-104" in cl):
            problems.append(f"{d}: variant flags do not match {v}")
        if "ekf5 window smoother:" not in lg or "ekf5 relay gate:" not in lg:
            problems.append(f"{d}: base lines missing")
        if "magnetometer heading reference" not in lg or "stride noise 5 %" not in lg:
            problems.append(f"{d}: realistic-world lines missing")
        # FIDELITY-1b's abort condition (registered): a window refusal in the CA-TBL arm. Refusals in other arms are
        # reported: a refusal leaves the EKF state untouched (CaTblImuEkf window solve, failure path returns).
        wins = re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)
        if wins and int(wins[0]):
            problems.append(f"{d}: window refusal in ca_tbl_full")
        other_refusals.extend(int(x) for x in wins[1:])
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
    md = ["# FIDELITY-1b — seeds 161-180 (scripts/fidelity1b_analyse.py)", "",
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

    md += ["", "## Paired contrasts F1 − F0, every arm", "",
           "| config | arm | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in CF:
        for a in ARMS:
            T, O = (c, "F1", a), (c, "F0", a)
            md.append(f"| {c} | {a} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 2), p=1)} | "
                      f"{cs.fmt(*pr(T, O, 3), p=2)} |")
    md += ["", "## CA-TBL − comparator, per variant", "",
           "| config | variant | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in CF:
        for v in VS:
            for comp in ARMS[1:]:
                T, O = (c, v, "ca_tbl_full"), (c, v, comp)
                md.append(f"| {c} | {v} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 3), p=2)} |")

    r1 = {c: pr((c, "F1", "ca_tbl_full"), (c, "F0", "ca_tbl_full"), 1) for c in CF}
    r2 = pr(("B", "F1", "ca_tbl_full"), ("B", "F0", "ca_tbl_full"), 0)
    r3 = pr(("A", "F1", "ca_tbl_full"), ("A", "F0", "ca_tbl_full"), 3)
    R1 = all(r1[c][1] > -0.03 for c in CF)
    R2 = r2[1] >= -0.02 and r2[2] <= 0.02
    R3 = r3[2] < 0.0
    md += ["", "## Registered predictions", "",
           f"- R1 F1 lowers CA-TBL five-node A(25 m) nowhere (lower bounds > -0.03): {'HOLDS' if R1 else 'FAILS'} "
           f"({'; '.join(c + ' ' + cs.fmt(*r1[c]) for c in CF)})",
           f"- R2 F1 moves CA-TBL tag-0 A(25 m) in B by < 0.02: {'HOLDS' if R2 else 'FAILS'} ({cs.fmt(*r2)})",
           f"- R3 F1 lowers CA-TBL subject J in A: {'HOLDS' if R3 else 'FAILS'} ({cs.fmt(*r3, p=2)})", ""]
    md += [f"Window refusals in the other four arms (reported, not an abort): {sum(other_refusals)} in "
           f"{len(other_refusals)} arm-runs", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "FIDELITY1B.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
