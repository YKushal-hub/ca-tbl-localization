#!/usr/bin/env python3
"""REAL-IMU (report/PREREG_REAL_IMU.md): the preset with a realistic IMU (BMI270 gyro bias 0.15 deg/s, 5 % per-step
stride noise), cv vs ekf5, with and without --acquireHandoverCurvature, paired by seed.
Committed with the registration, before any cell; not edited after the first cell.

  P1  ekf5 lowers ca_tbl_full's tag 0 CEP95 against cv (guard off) in A and in B (paired interval below 0)
  P2  under ekf5, the guard lowers ca_tbl_full's tag 0 CEP95 in A (interval below 0), and in B its mean
      per-seed CEP95 ratio guard / no guard - 1 is at most +0.10
  P3  under ekf5 with the guard, ca_tbl_full's availability exceeds every comparator's in A and in B
      (paired interval above 0)
  P4  under ekf5 with the guard, CA-TBL - poucet_fusion tag 0 CEP95: interval below 0 in A, above 0 in B

Estimators: lora_cmp_analyse.per_seed, camp_spec_analyse.ci, post_acq_metrics.post_acq, v3_analyse.node_rows.
usage: real_imu_analyse.py <outdir>
"""
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
from post_acq_metrics import post_acq  # noqa: E402
from v3_analyse import node_rows  # noqa: E402

ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "poucet_fusion"]
CFGS = ["A", "B"]
FS = ["cv", "ekf5"]
GS = ["g0", "g1"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_*_g[01]", "s*")))
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
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        if ("_ekf5_" in d) != ("N.28 ekf5:" in lg):
            problems.append(f"{d}: filter line does not match the cell")
        if ("_g1" in d) != ("curvature guard kept the cloud" in lg):
            problems.append(f"{d}: guard line does not match the cell")
        if "DC-1: stride noise 5 % per step" not in lg or "DC-1: gyro bias sigma" not in lg:
            problems.append(f"{d}: realistic-IMU lines missing")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D, cov, nees, pa = {}, {}, {}, {}
    for c in CFGS:
        for f in FS:
            for g in GS:
                cell = f"{c}_{f}_{g}"
                for a in ARMS:
                    D[(c, f, g, a)] = per_seed(out, cell, a)
                    if len(D[(c, f, g, a)]) != 20:
                        problems.append(f"{cell} {a}: {len(D[(c, f, g, a)])} seeds, not 20")
                    Cs, Ns, P = [], [], {}
                    for d in cs.seeds(out, cell):
                        if not cs.ok(d):
                            continue
                        n, cc = node_rows(d, a)
                        Ns += n
                        Cs += cc
                        P[int(d.rsplit("s", 1)[1])] = post_acq(d, a)[2]
                    cov[(c, f, g, a)] = float(np.mean(Cs)) if Cs else float("nan")
                    nees[(c, f, g, a)] = float(np.median(Ns)) if Ns else float("nan")
                    pa[(c, f, g, a)] = P
    mean = lambda x, k: float(np.mean([v[k] for v in x.values()])) if x else float("nan")  # noqa: E731
    md = ["# REAL-IMU — the preset with a realistic IMU (scripts/real_imu_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", "",
          "| config | filter | guard | arm | availability | CEP95 tag 0 (m) | CEP95 after 300 s, fix cycles (m) | coverage | median NEES | subject J |",
          "|---|---|---|---|---|---|---|---|---|---|"]
    for c in CFGS:
        for f in FS:
            for g in GS:
                for a in ARMS:
                    x = D[(c, f, g, a)]
                    p = [v for v in pa[(c, f, g, a)].values() if np.isfinite(v)]
                    md.append(f"| {c} | {f} | {g} | {a} | {mean(x, 'av'):.3f} | {mean(x, 'c95'):.1f} | "
                              f"{(np.mean(p) if p else float('nan')):.1f} | {cov[(c, f, g, a)]:.3f} | {nees[(c, f, g, a)]:.2f} | {mean(x, 'J'):.2f} |")

    def pair(c, k, A, B):
        a, b = D[A], D[B]
        s = sorted(set(a) & set(b))
        return (cs.ci([a[i][k] - b[i][k] for i in s]), float(np.mean([a[i]["c95"] / b[i]["c95"] - 1.0 for i in s]))) if s else ((float("nan"),) * 3, float("nan"))

    md += ["", "## Paired contrasts, ca_tbl_full tag 0 CEP95", "", "| contrast | config | Δ (m) | mean per-seed ratio − 1 |", "|---|---|---|---|"]
    r = {}
    for c in CFGS:
        r[("P1", c)] = pair(c, "c95", (c, "ekf5", "g0", "ca_tbl_full"), (c, "cv", "g0", "ca_tbl_full"))
        r[("P2", c)] = pair(c, "c95", (c, "ekf5", "g1", "ca_tbl_full"), (c, "ekf5", "g0", "ca_tbl_full"))
        r[("cvg", c)] = pair(c, "c95", (c, "cv", "g1", "ca_tbl_full"), (c, "cv", "g0", "ca_tbl_full"))
        for k, lab in (("P1", "ekf5 − cv (guard off)"), ("P2", "guard on − off (ekf5)"), ("cvg", "guard on − off (cv)")):
            md.append(f"| {lab} | {c} | {cs.fmt(*r[(k, c)][0], p=1)} | {r[(k, c)][1]:+.3f} |")
    md += ["", "## CA-TBL − comparator under ekf5 with the guard, paired by seed", "",
           "| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ CEP95 after 300 s (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    av = {}
    for c in CFGS:
        for comp in ARMS[1:]:
            T, O = (c, "ekf5", "g1", "ca_tbl_full"), (c, "ekf5", "g1", comp)
            dav = pair(c, "av", T, O)[0]
            dc = pair(c, "c95", T, O)[0]
            dj = pair(c, "J", T, O)[0]
            t, o = pa[T], pa[O]
            s = [i for i in t if i in o and np.isfinite(t[i]) and np.isfinite(o[i])]
            dp = cs.ci([t[i] - o[i] for i in s]) if s else (float("nan"),) * 3
            av[(c, comp)] = (dav, dc)
            md.append(f"| {c} | {comp} | {cs.fmt(*dav)} | {cs.fmt(*dc, p=1)} | {cs.fmt(*dp, p=1)} | {cs.fmt(*dj, p=2)} |")
    p1 = all(r[("P1", c)][0][2] < 0.0 for c in CFGS)
    p2 = r[("P2", "A")][0][2] < 0.0 and r[("P2", "B")][1] <= 0.10
    p3 = all(av[(c, comp)][0][1] > 0.0 for c in CFGS for comp in ARMS[1:])
    p4 = av[("A", "poucet_fusion")][1][2] < 0.0 and av[("B", "poucet_fusion")][1][1] > 0.0
    md += ["", "## Registered predictions", "",
           f"- P1 ekf5 lowers CA-TBL CEP95 against cv in A and B: {'HOLDS' if p1 else 'FAILS'} ("
           + ", ".join(f"{c} {cs.fmt(*r[('P1', c)][0], p=1)}" for c in CFGS) + ")",
           f"- P2 the guard lowers CA-TBL CEP95 under ekf5 in A, costs <= +10 % in B: {'HOLDS' if p2 else 'FAILS'} "
           f"(A {cs.fmt(*r[('P2', 'A')][0], p=1)}; B ratio {r[('P2', 'B')][1]:+.3f})",
           f"- P3 CA-TBL availability above every comparator in A and B (ekf5, guard): {'HOLDS' if p3 else 'FAILS'}",
           f"- P4 CA-TBL - Poucet CEP95 below 0 in A, above 0 in B (ekf5, guard): {'HOLDS' if p4 else 'FAILS'} "
           f"(A {cs.fmt(*av[('A', 'poucet_fusion')][1], p=1)}; B {cs.fmt(*av[('B', 'poucet_fusion')][1], p=1)})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REAL_IMU.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
