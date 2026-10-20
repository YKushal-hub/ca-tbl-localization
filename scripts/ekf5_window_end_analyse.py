#!/usr/bin/env python3
"""EKF5-WINDOW-END (report/PREREG_EKF5_WINDOW_END.md): the endgame only, out of sample, seeds 81-100. REAL-BEST's S stack
as given (S) and with the sliding-window smoother (W, --ekf5Window=60), paired by seed. Committed with the
registration, before any cell; not edited after the first cell.

  P1  W lowers ca_tbl_full's tag 0 CEP95 against S (paired interval below 0)
  P2  under W, ca_tbl_full's 95 % coverage is at least 0.93
  P3  W costs ca_tbl_full no availability: paired lower bound above -0.01
  P4  under W, ca_tbl_full - poucet_fusion: CEP95 interval contains 0, availability interval above 0
  P5  W lowers fixed_threshold's tag 0 CEP95 against S (paired interval below 0): the effect is the filter's, not
      the policy's

Estimators: lora_cmp_analyse.per_seed, camp_spec_analyse.ci, v3_analyse.node_rows (as REAL-*).
usage: ekf5_window_end_analyse.py <outdir>
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
VS = ["S", "W"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "B_[SW]", "s*")))
    if len(cells) != 40:
        problems.append(f"{len(cells)} cells, not 40")
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
        if not 81 <= seed <= 100:
            problems.append(f"{d}: seed outside 81-100")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d))[-1]
        if (v == "W") != ("ekf5 window smoother:" in lg) or "ekf5 relay gate:" not in lg:
            problems.append(f"{d}: variant lines do not match the cell")
        if "magnetometer heading reference" not in lg or "stride noise 5 %" not in lg:
            problems.append(f"{d}: realistic-world lines missing")
        fails = [int(x) for x in re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)]
        if any(fails):
            problems.append(f"{d}: window solver failures {sum(fails)}")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D = {(c, v, a): per_seed(out, f"{c}_{v}", a) for c in "B" for v in VS for a in ARMS}
    for k, x in D.items():
        if len(x) != 20:
            problems.append(f"{k}: {len(x)} seeds, not 20")
    COV = {}
    md = ["# EKF5-WINDOW-END — the endgame, out of sample, seeds 81-100 (scripts/ekf5_window_end_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", "",
          "| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | median NEES | subject J |",
          "|---|---|---|---|---|---|---|---|"]
    for c in "B":
        for v in VS:
            for a in ARMS:
                x = D[(c, v, a)]
                if not x:
                    continue
                N, C = [], []
                for d in cs.seeds(out, f"{c}_{v}"):
                    n_, c_ = node_rows(d, a)
                    N += n_
                    C += c_
                COV[(c, v, a)] = np.mean(C) if C else float("nan")
                md.append(f"| {c} | {v} | {a} | {np.mean([y['av'] for y in x.values()]):.3f} | "
                          f"{np.mean([y['c95'] for y in x.values()]):.1f} ({np.median([y['c95'] for y in x.values()]):.1f}) | "
                          f"{COV[(c, v, a)]:.3f} | {(np.median(N) if N else float('nan')):.2f} | "
                          f"{np.mean([y['J'] for y in x.values()]):.2f} |")

    def pr(A, B, k):
        a, b = D[A], D[B]
        s = sorted(set(a) & set(b))
        return cs.ci([a[i][k] - b[i][k] for i in s]) if s else (float("nan"),) * 3

    md += ["", "## W − S, paired by seed", "", "| config | arm | Δ CEP95 tag 0 (m) | Δ availability | Δ subject J |",
           "|---|---|---|---|---|"]
    for c in "B":
        for a in ARMS:
            md.append(f"| {c} | {a} | {cs.fmt(*pr((c, 'W', a), (c, 'S', a), 'c95'), p=1)} | "
                      f"{cs.fmt(*pr((c, 'W', a), (c, 'S', a), 'av'))} | {cs.fmt(*pr((c, 'W', a), (c, 'S', a), 'J'), p=2)} |")
    md += ["", "## CA-TBL − comparator under W, paired by seed", "",
           "| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
    for c in "B":
        for comp in ARMS[1:]:
            T, O = (c, "W", "ca_tbl_full"), (c, "W", comp)
            md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 'av'))} | {cs.fmt(*pr(T, O, 'c95'), p=1)} | "
                      f"{cs.fmt(*pr(T, O, 'J'), p=2)} |")
    p1 = pr(("B", "W", "ca_tbl_full"), ("B", "S", "ca_tbl_full"), "c95")
    p2 = COV.get(("B", "W", "ca_tbl_full"), float("nan"))
    p3 = pr(("B", "W", "ca_tbl_full"), ("B", "S", "ca_tbl_full"), "av")
    p4c = pr(("B", "W", "ca_tbl_full"), ("B", "W", "poucet_fusion"), "c95")
    p4a = pr(("B", "W", "ca_tbl_full"), ("B", "W", "poucet_fusion"), "av")
    p5 = pr(("B", "W", "fixed_threshold"), ("B", "S", "fixed_threshold"), "c95")
    P1 = p1[2] < 0.0
    P2 = p2 >= 0.93
    P3 = p3[1] > -0.01
    P4 = p4c[1] <= 0.0 <= p4c[2] and p4a[1] > 0.0
    P5 = p5[2] < 0.0
    md += ["", "## Registered predictions", "",
           f"- P1 W lowers CA-TBL CEP95 against S: {'HOLDS' if P1 else 'FAILS'} ({cs.fmt(*p1, p=1)})",
           f"- P2 under W, CA-TBL coverage >= 0.93: {'HOLDS' if P2 else 'FAILS'} ({p2:.3f})",
           f"- P3 W costs CA-TBL no availability (lower bound > -0.01): {'HOLDS' if P3 else 'FAILS'} ({cs.fmt(*p3)})",
           f"- P4 under W, CA-TBL ties Poucet's CEP95 and beats its availability: {'HOLDS' if P4 else 'FAILS'} "
           f"(CEP95 {cs.fmt(*p4c, p=1)}; availability {cs.fmt(*p4a)})",
           f"- P5 W lowers fixed_threshold's CEP95 against S: {'HOLDS' if P5 else 'FAILS'} ({cs.fmt(*p5, p=1)})", "",
           f"Decision rule: P1 and P3 hold -> --ekf5Window=60 is put to the owner for the endgame with the realistic "
           f"preset: {'MET' if (P1 and P3) else 'NOT MET'}.", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "EKF5_WINDOW_END.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
