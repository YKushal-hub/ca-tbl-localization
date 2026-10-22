#!/usr/bin/env python3
"""PRESET-5 (report/PREREG_PRESET5.md): seeds 341-360, configurations A (5 km) and B (endgame). NEW = the preset after
DESIGN_DECISIONS 26 (12 gateways, ring, 12 finders); OLD = the brief's deployment (3 gateways, star, 8 finders).
Primary: A(25 m) (scripts/goal_report.py). Committed with the registration, before any cell.

  P1  NEW's ca_tbl_full mean five-node A(25 m) in A is at least 0.95
  P2  NEW raises ca_tbl_full's five-node A(25 m) against OLD in A (paired interval above 0)
  P3  NEW's ca_tbl_full mean five-node A(25 m) in B is at least 0.95
  P4  NEW lowers ca_tbl_full's five-node A(25 m) in no configuration (every paired lower bound above -0.02)
  P5  NEW's ca_tbl_full mean tag-0 A(25 m) is at least 0.95 in A and in B

usage: preset5_analyse.py <outdir>
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
CF = ["A", "B"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_[ON][LE][DW]", "s*")))
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
        if not 341 <= int(d.rsplit("s", 1)[1]) <= 360:
            problems.append(f"{d}: seed outside 341-360")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d)).split("_")[1]
        new = (v == "NEW")
        nf = len(re.findall(r"finder \d+ -> subject", lg.split("MS-1 tasking (pairs):")[1].split("\n")[0])) \
            if "MS-1 tasking (pairs):" in lg else -1
        if nf != (12 if new else 8) or "UWB mode 850k:" not in lg or "ekf5 window smoother:" not in lg:
            problems.append(f"{d}: finder count {nf} or preset lines do not match the cell")
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
    md = ["# PRESET-5 — seeds 341-360 (scripts/preset5_analyse.py)", "",
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

    def mean(c, v, k):
        x = D[(c, v, "ca_tbl_full")]
        return float(np.mean([r[k] for r in x.values()])) if x else float("nan")

    F = "ca_tbl_full"
    p1 = mean("A", "NEW", 1)
    p2 = pr(("A", "NEW", F), ("A", "OLD", F), 1)
    p3 = mean("B", "NEW", 1)
    p4 = {c: pr((c, "NEW", F), (c, "OLD", F), 1) for c in CF}
    p5 = {c: mean(c, "NEW", 0) for c in CF}
    md += ["", "## Registered predictions", "",
           f"- P1 NEW's CA-TBL five-node A(25 m) in A >= 0.95: {'HOLDS' if p1 >= 0.95 else 'FAILS'} ({p1:.3f})",
           f"- P2 NEW raises CA-TBL five-node A(25 m) in A: {'HOLDS' if p2[1] > 0 else 'FAILS'} ({cs.fmt(*p2)})",
           f"- P3 NEW's CA-TBL five-node A(25 m) in B >= 0.95: {'HOLDS' if p3 >= 0.95 else 'FAILS'} ({p3:.3f})",
           f"- P4 NEW lowers CA-TBL five-node A(25 m) nowhere (lower bounds > -0.02): "
           f"{'HOLDS' if all(p4[c][1] > -0.02 for c in CF) else 'FAILS'} ({'; '.join(c + ' ' + cs.fmt(*p4[c]) for c in CF)})",
           f"- P5 NEW's CA-TBL tag-0 A(25 m) >= 0.95 in A and B: {'HOLDS' if all(p5[c] >= 0.95 for c in CF) else 'FAILS'} "
           f"({'; '.join(f'{c} {p5[c]:.3f}' for c in CF)})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "PRESET5.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
