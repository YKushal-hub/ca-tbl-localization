#!/usr/bin/env python3
"""TDOA-TIMING-B (report/PREREG_TDOA_TIMING_B.md): the TDOA-TIMING contrasts on the paper's comparators.
Preset (L), perfect gateway sync (S0), perfect arrival timing (T0), tier-1 TDoA off (N); seeds 361-380.
Metrics: the paper's per-seed quantities (scripts/lora_cmp_analyse.py per_seed): availability, CEP95, energy.
Paired seed bootstrap, 10 000 resamples, 95 %. Committed with the registration, before any cell.

usage: tdoa_timing_b_analyse.py <outdir of tdoa_timing_b_run.sh>
Writes <outdir>/TDOA_TIMING_B.md, prints it, and exits 1 if any registered check (PREREG sect 5) fails.
"""
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402

ARMS = {"A": ("fixed_threshold", "poucet_fusion", "lora_fusion", "static_868", "static_2p4"),
        "B": ("fixed_threshold", "poucet_fusion", "lora_fusion", "static_868", "static_2p4"),
        "C": ("ca_tbl_full", "static_868", "lora_fusion")}
TITLE = {"A": "5 km, 3 gateways", "B": "endgame", "C": "8 gateways (ring)"}
VS = ("L", "S0", "T0", "N")
SEEDS = tuple(range(361, 381))
NBOOT = 10000
LINE = "TDOA TOA SCALE (--tdoaToaScale="
CONTRASTS = (("SYNC", "S0", "L"), ("TIMING", "T0", "L"), ("TDOA VALUE", "L", "N"))


def checks(out, problems):
    sources = set()
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_*", "s*")))
    if len(cells) != 240:
        problems.append(f"{len(cells)} cells, not 240")
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


def f(x, p=3, k=1.0):
    return f"{k * x[0]:+.{p}f} [{k * x[1]:+.{p}f}, {k * x[2]:+.{p}f}]"


def verdict(x):
    if x[1] > 0:
        return "MATERIAL" if x[0] >= 0.02 else "SMALL"
    return "ADVERSE" if x[2] < 0 else "NONE"


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    sources = checks(out, problems)
    D = {}
    for g, arms in ARMS.items():
        for v in VS:
            for a in arms:
                r = per_seed(out, f"{g}_{v}", a)
                if sorted(r) != list(SEEDS):
                    problems.append(f"{g}_{v} {a}: {len(r)} seeds, not {len(SEEDS)}")
                D[(g, v, a)] = r
    md = ["# TDOA-TIMING-B — seeds 361-380 (scripts/tdoa_timing_b_analyse.py)", "",
          f"Source(s): {', '.join(sources) or 'none'}. Intervals: paired seed bootstrap, {NBOOT} resamples, 95 %.",
          "Availability in percentage points (pp).", ""]
    if problems:
        md += ["## CHECKS FAILED — no verdict is read", ""] + [f"- {p}" for p in problems]
        print("\n".join(md))
        open(os.path.join(out, "TDOA_TIMING_B.md"), "w").write("\n".join(md) + "\n")
        return 1
    rng = np.random.default_rng(20261021)
    idx = {g: rng.integers(0, len(SEEDS), size=(NBOOT, len(SEEDS))) for g in ARMS}

    def vec(g, v, a, q):
        return np.array([D[(g, v, a)][s][q] for s in SEEDS], dtype=float)

    ver = []
    for g, arms in ARMS.items():
        md += [f"## Geometry {g} ({TITLE[g]})", "", "| arm | config | availability (%) | CEP95 (m) | energy (J) |",
               "|---|---|---|---|---|"]
        for a in arms:
            for v in VS:
                md.append(f"| {a} | {v} | {100 * vec(g, v, a, 'av').mean():.1f} | {vec(g, v, a, 'c95').mean():.1f} | "
                          f"{vec(g, v, a, 'J').mean():.2f} |")
        md += ["", "| arm | contrast | Δ availability (pp) | Δ CEP95 (m) | Δ energy (J) | verdict |",
               "|---|---|---|---|---|---|"]
        for a in arms:
            for name, hi, lo in CONTRASTS:
                x = [interval(vec(g, hi, a, q) - vec(g, lo, a, q), idx[g]) for q in ("av", "c95", "J")]
                vd = verdict(x[0])
                ver.append((g, a, name, vd))
                md.append(f"| {a} | {name} ({hi} - {lo}) | {f(x[0], 1, 100)} | {f(x[1], 1)} | {f(x[2], 2)} | {vd} |")
        md.append("")
    md += ["## Registered verdicts (availability)", "", "| geometry | arm | SYNC | TIMING | TDOA VALUE |",
           "|---|---|---|---|---|"]
    for g, arms in ARMS.items():
        for a in arms:
            row = {n: vd for gg, aa, n, vd in ver if gg == g and aa == a}
            md.append(f"| {g} | {a} | {row['SYNC']} | {row['TIMING']} | {row['TDOA VALUE']} |")
    print("\n".join(md))
    open(os.path.join(out, "TDOA_TIMING_B.md"), "w").write("\n".join(md) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
