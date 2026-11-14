#!/usr/bin/env python3
"""TDOA-TIMING (report/PREREG_TDOA_TIMING.md): the preset (L) against perfect gateway sync (S0), perfect arrival
timing (T0) and tier-1 TDoA off (N); ca_tbl_full and static_868; 5 km (A) and endgame (B); seeds 341-360.
Metrics from scripts/goal_report.py cell(); paired seed bootstrap, 10 000 resamples, 95 %.
Committed with the registration, before any cell.

usage: tdoa_timing_analyse.py <outdir of tdoa_timing_run.sh>
Writes <outdir>/TDOA_TIMING.md, prints it, and exits 1 if any registered check (PREREG sect 5) fails.
"""
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell  # noqa: E402

ARMS = ("ca_tbl_full", "static_868")
GEOS = ("A", "B")
VS = ("L", "S0", "T0", "N")
SEEDS = tuple(range(341, 361))
NBOOT = 10000
LINE = "TDOA TOA SCALE (--tdoaToaScale="
CONTRASTS = (("SYNC", "S0", "L"), ("TIMING", "T0", "L"), ("TDOA VALUE", "L", "N"))


def checks(out, problems):
    sources = set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_*", "s*")))
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
        v = os.path.basename(os.path.dirname(d)).split("_", 1)[1]
        if (LINE in open(os.path.join(d, "run.log"), errors="replace").read()) != (v == "T0"):
            problems.append(f"{d}: TDOA TOA SCALE line does not match the configuration")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    return sorted(sources)


def interval(v, idx):
    m = v[idx].mean(axis=1)
    return float(v.mean()), float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975))


def f(x, p=3):
    return f"{x[0]:+.{p}f} [{x[1]:+.{p}f}, {x[2]:+.{p}f}]"


def verdict(x):
    if x[1] > 0:
        return "MATERIAL" if x[0] >= 0.02 else "SMALL"
    return "NONE"


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    sources = checks(out, problems)
    D = {}
    for g in GEOS:
        for v in VS:
            for a in ARMS:
                rows = {}
                for s in SEEDS:
                    d = os.path.join(out, f"{g}_{v}", f"s{s}")
                    r = cell(d, a) if cs.ok(d) else None
                    if r:
                        rows[s] = r
                if len(rows) != len(SEEDS):
                    problems.append(f"{g}_{v} {a}: {len(rows)} seeds, not {len(SEEDS)}")
                D[(g, v, a)] = rows
    md = ["# TDOA-TIMING — seeds 341-360 (scripts/tdoa_timing_analyse.py)", "",
          f"Source(s): {', '.join(sources) or 'none'}. Intervals: paired seed bootstrap, {NBOOT} resamples, 95 %.", ""]
    if problems:
        md += ["## CHECKS FAILED — no verdict is read", ""] + [f"- {p}" for p in problems]
        print("\n".join(md))
        open(os.path.join(out, "TDOA_TIMING.md"), "w").write("\n".join(md) + "\n")
        return 1
    rng = np.random.default_rng(20261021)
    idx = {g: rng.integers(0, len(SEEDS), size=(NBOOT, len(SEEDS))) for g in GEOS}

    def vec(g, v, a, i):
        return np.array([D[(g, v, a)][s][i] for s in SEEDS], dtype=float)

    ver = {}
    for g in GEOS:
        md += [f"## Geometry {g} ({'5 km' if g == 'A' else 'endgame'})", "",
               "| arm | config | five-node A(25 m) | tag-0 A(25 m) | tag-0 CEP95 (m) | five-node J |",
               "|---|---|---|---|---|---|"]
        for a in ARMS:
            for v in VS:
                md.append(f"| {a} | {v} | {vec(g, v, a, 1).mean():.3f} | {vec(g, v, a, 0).mean():.3f} | "
                          f"{vec(g, v, a, 2).mean():.1f} | {vec(g, v, a, 3).mean():.2f} |")
        md += ["", "| arm | contrast | five-node A(25 m) | tag-0 A(25 m) | tag-0 CEP95 (m) | five-node J | verdict |",
               "|---|---|---|---|---|---|---|"]
        for a in ARMS:
            for name, hi, lo in CONTRASTS:
                x = [interval(vec(g, hi, a, i) - vec(g, lo, a, i), idx[g]) for i in (1, 0, 2, 3)]
                vd = verdict(x[0])
                if a == "ca_tbl_full":
                    ver[(g, name)] = vd
                md.append(f"| {a} | {name} ({hi} - {lo}) | {f(x[0])} | {f(x[1])} | {f(x[2], 1)} | {f(x[3], 2)} | "
                          f"{vd} |")
        md.append("")
    md += ["## Registered verdicts (ca_tbl_full, five-node A(25 m))", ""]
    for (g, name), vd in ver.items():
        md.append(f"- {name}, geometry {g}: **{vd}**")
    print("\n".join(md))
    open(os.path.join(out, "TDOA_TIMING.md"), "w").write("\n".join(md) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
