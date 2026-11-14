#!/usr/bin/env python3
"""ENV-SWEEP (report/PREREG_ENV_SWEEP.md): seeds 361-380, configurations A (5 km) and B (endgame), environments
E0 open, E1 forested, E2 suburban, E3 urban moderate; the PRESET-5 preset, only --env changes. Primary: A(25 m)
(scripts/goal_report.py). Committed with the registration, before any cell.

  P1  CA-TBL five-node mean A(25 m) in A, E0, is at least 0.95
  P2  CA-TBL five-node mean A(25 m) in A, E3, is below 0.95
  P3  CA-TBL five-node mean A(25 m) in A falls monotonically E0 >= E1 >= E2 >= E3
  P4  CA-TBL beats static_868 on five-node A(25 m) in every env x config (paired interval above 0)
  P5  CA-TBL beats static_2p4 on five-node A(25 m) in at least one env x config (paired interval above 0)
  P6  CA-TBL subject J is below 10 % of poucet_fusion's in every env x config

usage: envsweep_analyse.py <outdir>
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
ENVS = ["E0", "E1", "E2", "E3"]
ENVNAME = {"E0": "open", "E1": "forest", "E2": "suburban", "E3": "urban moderate"}
CF = ["A", "B"]
F = "ca_tbl_full"


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_E[0-3]", "s*")))
    if len(cells) != 160:
        problems.append(f"{len(cells)} cells, not 160")
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        if not 361 <= int(d.rsplit("s", 1)[1]) <= 380:
            problems.append(f"{d}: seed outside 361-380")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        if "UWB mode 850k:" not in lg or "ekf5 window smoother:" not in lg or "magnetometer heading reference" not in lg:
            problems.append(f"{d}: preset lines missing")
        if any(int(x) for x in re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)):
            problems.append(f"{d}: window solver failures")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D = {}
    for c in CF:
        for e in ENVS:
            for a in ARMS:
                D[(c, e, a)] = {}
                for d in sorted(glob.glob(os.path.join(out, f"{c}_{e}", "s*"))):
                    if cs.ok(d):
                        r = cell(d, a)
                        if r:
                            D[(c, e, a)][int(d.rsplit("s", 1)[1])] = r
                if len(D[(c, e, a)]) != 20:
                    problems.append(f"{(c, e, a)}: {len(D[(c, e, a)])} seeds, not 20")
    md = ["# ENV-SWEEP — seeds 361-380 (scripts/envsweep_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}. Binary: the PRESET-5 frozen binary (8e92fe3a).", "",
          "| config | env | arm | A(25 m) tag 0 | A(25 m) five-node | runs >= 0.95 (five-node) | CEP95 tag 0 (m) | subject J |",
          "|---|---|---|---|---|---|---|---|"]
    for c in CF:
        for e in ENVS:
            for a in ARMS:
                x = D[(c, e, a)]
                if not x:
                    continue
                arr = np.array(list(x.values()))
                md.append(f"| {c} | {e} {ENVNAME[e]} | {a} | {arr[:, 0].mean():.3f} | {arr[:, 1].mean():.3f} | "
                          f"{(arr[:, 1] >= 0.95).mean():.2f} | {np.nanmean(arr[:, 2]):.1f} | {np.nanmean(arr[:, 3]):.2f} |")

    def pr(A, B, k):
        a, b = D[A], D[B]
        s = sorted(set(a) & set(b))
        return cs.ci([a[i][k] - b[i][k] for i in s]) if s else (float("nan"),) * 3

    md += ["", "## CA-TBL - comparator, per environment", "",
           "| config | env | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ CEP95 tag 0 (m) | Δ subject J |",
           "|---|---|---|---|---|---|---|"]
    for c in CF:
        for e in ENVS:
            for comp in ARMS[1:]:
                T, O = (c, e, F), (c, e, comp)
                md.append(f"| {c} | {e} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | "
                          f"{cs.fmt(*pr(T, O, 2), p=1)} | {cs.fmt(*pr(T, O, 3), p=2)} |")

    def mean(c, e, a, k):
        x = D[(c, e, a)]
        return float(np.mean([r[k] for r in x.values()])) if x else float("nan")

    p1 = mean("A", "E0", F, 1)
    p2 = mean("A", "E3", F, 1)
    chain = [mean("A", e, F, 1) for e in ENVS]
    p3 = all(chain[i] >= chain[i + 1] for i in range(3))
    p4 = {(c, e): pr((c, e, F), (c, e, "static_868"), 1) for c in CF for e in ENVS}
    p5 = {(c, e): pr((c, e, F), (c, e, "static_2p4"), 1) for c in CF for e in ENVS}
    p6 = {(c, e): mean(c, e, F, 3) / mean(c, e, "poucet_fusion", 3) for c in CF for e in ENVS}
    md += ["", "## Registered predictions", "",
           f"- P1 CA-TBL five-node A(25 m), A, E0 >= 0.95: {'HOLDS' if p1 >= 0.95 else 'FAILS'} ({p1:.3f})",
           f"- P2 CA-TBL five-node A(25 m), A, E3 < 0.95: {'HOLDS' if p2 < 0.95 else 'FAILS'} ({p2:.3f})",
           f"- P3 five-node A(25 m) in A monotone E0>=E1>=E2>=E3: {'HOLDS' if p3 else 'FAILS'} "
           f"({' >= '.join(f'{v:.3f}' for v in chain)})",
           f"- P4 CA-TBL beats static_868 everywhere: {'HOLDS' if all(v[1] > 0 for v in p4.values()) else 'FAILS'} "
           f"(lowest lower bound {min(v[1] for v in p4.values()):.3f})",
           f"- P5 CA-TBL beats static_2p4 in at least one cell: {'HOLDS' if any(v[1] > 0 for v in p5.values()) else 'FAILS'} "
           f"(cells with interval above 0: {', '.join(f'{c}/{e}' for (c, e), v in p5.items() if v[1] > 0) or 'none'})",
           f"- P6 CA-TBL J below 10 % of poucet_fusion's everywhere: {'HOLDS' if all(v < 0.10 for v in p6.values()) else 'FAILS'} "
           f"(largest ratio {max(p6.values()):.3f})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "ENV_SWEEP.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
