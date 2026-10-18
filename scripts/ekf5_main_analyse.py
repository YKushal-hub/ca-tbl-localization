#!/usr/bin/env python3
"""EKF5-MAIN (report/PREREG_EKF5_MAIN.md): --imuFilter=ekf5 against cv on the final preset, paired by seed.
Committed with the registration, before any cell; not edited after the first cell.

  P1  at L2, ekf5 lowers ca_tbl_full's tag 0 CEP95 against cv in A and in B (paired interval below 0)
  P2  at L0, ca_tbl_full's mean per-seed CEP95 ratio ekf5 / cv - 1 is at most +0.10 in A and in B
  P3  at L0 in A, ekf5 raises ca_tbl_full's 95 % coverage (v3_analyse.node_rows, tag 0 and subjects)

Estimators are main's own: lora_cmp_analyse.per_seed (availability, CEP95), camp_spec_analyse.ci,
post_acq_metrics.post_acq (CEP95 over fix cycles after 300 s), v3_analyse.node_rows (coverage, NEES).
usage: ekf5_main_analyse.py <outdir>
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
CFGS = [("A", "5 km"), ("B", "endgame")]
LVS = ["L0", "L2"]
FS = ["cv", "ekf5"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_*_L[02]", "s*")))
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
        if ("_L2" in d) != ("DC-1: gyro bias sigma" in lg):
            problems.append(f"{d}: bias line does not match the cell")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D, cov, nees, pa = {}, {}, {}, {}
    for c, _ in CFGS:
        for lv in LVS:
            for f in FS:
                cell = f"{c}_{f}_{lv}"
                for a in ARMS:
                    D[(c, lv, f, a)] = per_seed(out, cell, a)
                    if len(D[(c, lv, f, a)]) != 20:
                        problems.append(f"{cell} {a}: {len(D[(c, lv, f, a)])} seeds, not 20")
                    Cs, Ns, P = [], [], {}
                    for d in cs.seeds(out, cell):
                        if not cs.ok(d):
                            continue
                        n, cc = node_rows(d, a)
                        Ns += n
                        Cs += cc
                        P[int(d.rsplit("s", 1)[1])] = post_acq(d, a)[2]
                    cov[(c, lv, f, a)] = float(np.mean(Cs)) if Cs else float("nan")
                    nees[(c, lv, f, a)] = float(np.median(Ns)) if Ns else float("nan")
                    pa[(c, lv, f, a)] = P
    md = ["# EKF5-MAIN — the 5-state filter on the final preset (scripts/ekf5_main_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}.", "",
          "| config | level | filter | arm | availability | CEP95 tag 0 (m) | CEP95 after 300 s, fix cycles (m) | coverage | median NEES | subject J |",
          "|---|---|---|---|---|---|---|---|---|---|"]
    mean = lambda x, k: float(np.mean([v[k] for v in x.values()])) if x else float("nan")  # noqa: E731
    for c, _ in CFGS:
        for lv in LVS:
            for f in FS:
                for a in ARMS:
                    x = D[(c, lv, f, a)]
                    p = [v for v in pa[(c, lv, f, a)].values() if np.isfinite(v)]
                    md.append(f"| {c} | {lv} | {f} | {a} | {mean(x, 'av'):.3f} | {mean(x, 'c95'):.1f} | "
                              f"{(np.mean(p) if p else float('nan')):.1f} | {cov[(c, lv, f, a)]:.3f} | "
                              f"{nees[(c, lv, f, a)]:.2f} | {mean(x, 'J'):.2f} |")
    md += ["", "## ekf5 − cv, paired by seed", "",
           "| config | level | arm | Δ CEP95 tag 0 | Δ CEP95 after 300 s | Δ availability | Δ CEP95 subjects |", "|---|---|---|---|---|---|---|"]
    res = {}
    for c, _ in CFGS:
        for lv in LVS:
            for a in ARMS:
                e, v = D[(c, lv, "ekf5", a)], D[(c, lv, "cv", a)]
                s = sorted(set(e) & set(v))
                if not s:
                    continue
                d = {k: cs.ci([e[i][k] - v[i][k] for i in s]) for k in ("c95", "av", "s95")}
                pe, pv = pa[(c, lv, "ekf5", a)], pa[(c, lv, "cv", a)]
                sp = [i for i in s if i in pe and i in pv and np.isfinite(pe[i]) and np.isfinite(pv[i])]
                dp = cs.ci([pe[i] - pv[i] for i in sp]) if sp else (float("nan"),) * 3
                md.append(f"| {c} | {lv} | {a} | {cs.fmt(*d['c95'], p=1)} | {cs.fmt(*dp, p=1)} | {cs.fmt(*d['av'])} | {cs.fmt(*d['s95'], p=1)} |")
                if a == "ca_tbl_full":
                    res[(c, lv)] = (d["c95"], float(np.mean([e[i]["c95"] / v[i]["c95"] - 1.0 for i in s])))
    md += ["", "## CA-TBL − comparator, tag 0, paired by seed (cv / ekf5)", "",
           "| config | level | comparator | Δ availability | Δ CEP95 | Δ CEP95 after 300 s |", "|---|---|---|---|---|---|"]
    for c, _ in CFGS:
        for lv in LVS:
            for comp in ARMS[1:]:
                row = []
                for k in ("av", "c95", "pa"):
                    parts = []
                    for f in FS:
                        t, o = D[(c, lv, f, "ca_tbl_full")], D[(c, lv, f, comp)]
                        if k == "pa":
                            t, o = pa[(c, lv, f, "ca_tbl_full")], pa[(c, lv, f, comp)]
                            s = [i for i in t if i in o and np.isfinite(t[i]) and np.isfinite(o[i])]
                            parts.append(cs.fmt(*cs.ci([t[i] - o[i] for i in s]), p=1) if s else "n/a")
                        else:
                            s = sorted(set(t) & set(o))
                            parts.append(cs.fmt(*cs.ci([t[i][k] - o[i][k] for i in s]), p=3 if k == "av" else 1) if s else "n/a")
                    row.append(" / ".join(parts))
                md.append(f"| {c} | {lv} | {comp} | " + " | ".join(row) + " |")
    p1 = all((c, "L2") in res and res[(c, "L2")][0][2] < 0.0 for c in "AB")
    p2 = all((c, "L0") in res and res[(c, "L0")][1] <= 0.10 for c in "AB")
    p3 = cov[("A", "L0", "ekf5", "ca_tbl_full")] > cov[("A", "L0", "cv", "ca_tbl_full")]
    md += ["", "## Registered predictions", "",
           f"- P1 at L2 ekf5 lowers CA-TBL CEP95 in A and B: {'HOLDS' if p1 else 'FAILS'} ("
           + ", ".join(f"{c} {cs.fmt(*res[(c, 'L2')][0], p=1)}" for c in "AB" if (c, "L2") in res) + ")",
           f"- P2 at L0 mean per-seed ratio - 1 <= +0.10 in A and B: {'HOLDS' if p2 else 'FAILS'} ("
           + ", ".join(f"{c} {res[(c, 'L0')][1]:+.3f}" for c in "AB" if (c, "L0") in res) + ")",
           f"- P3 at A L0 ekf5 raises coverage: {'HOLDS' if p3 else 'FAILS'} "
           f"({cov[('A', 'L0', 'cv', 'ca_tbl_full')]:.3f} -> {cov[('A', 'L0', 'ekf5', 'ca_tbl_full')]:.3f})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "EKF5_MAIN.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
