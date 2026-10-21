#!/usr/bin/env python3
"""ORACLE-ESC (report/PREREG_ORACLE_ESC.md): fixed_threshold with its trigger reading trace(P) (K) or the true
squared error (O, --oracleTrigger), T in 3/12/50/200 m^2, geometries A (5 km) and B (endgame), seeds 321-340.
Metrics from scripts/goal_report.py cell(); paired seed bootstrap, 10 000 resamples, 99 % intervals.
Committed with the registration, before any cell.

usage: oracle_esc_analyse.py <outdir of oracle_esc_run.sh>
Writes <outdir>/ORACLE_ESC.md, prints it, and exits 1 if any registered check (PREREG sect 5) fails.
"""
import csv
import filecmp
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell  # noqa: E402

ARM = "fixed_threshold"
GEOS = ("A", "B")
TRIGS = ("K", "O")
TS = (3, 12, 50, 200)
SEEDS = tuple(range(321, 341))
NBOOT = 10000
ORACLE_LINE = "ORACLE TRIGGER (--oracleTrigger)"
DEC_RE = re.compile(r"ORACLE-ESC oracleTrigger \(fixed_threshold\): decisions on the true error, tag 0 (\d+), "
                    r"subjects (\d+)")


def esc5(d):
    """Five-node escalations: subject_summary.csv rows (tag 0 is subjectId 1)."""
    return float(sum(int(r["escalations"]) for r in cs.subj_rows(d, ARM)))


def checks(out, problems):
    sources = set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_[KO]_T*", "s*")))
    if len(cells) != 320:
        problems.append(f"{len(cells)} campaign cells, not 320")
    for d in cells + [os.path.join(out, f"P_{g}", "s321") for g in GEOS]:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        grp = os.path.basename(os.path.dirname(d))
        oracle = grp.split("_")[1] == "O" if not grp.startswith("P_") else False
        if (ORACLE_LINE in lg) != oracle:
            problems.append(f"{d}: ORACLE TRIGGER line {'missing' if oracle else 'present'}")
        if oracle:
            dm = DEC_RE.search(lg)
            if not dm or int(dm.group(1)) == 0 or int(dm.group(2)) == 0:
                problems.append(f"{d}: oracle path not exercised ({dm.group(0) if dm else 'no decisions line'})")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    for g in GEOS:
        a, b = os.path.join(out, f"P_{g}", "s321"), os.path.join(out, f"{g}_K_T12", "s321")
        names = sorted(f for f in os.listdir(a) if ARM in f or f in ("arm_summary.csv", "subject_summary.csv")) \
            if os.path.isdir(a) else []
        same = bool(names) and all(os.path.exists(os.path.join(b, f)) and
                                   filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False) for f in names)
        if not same:
            problems.append(f"preset check: P_{g} s321 not byte-identical to {g}_K_T12 s321 ({len(names)} files)")
    return sorted(sources)


def load(out, problems):
    D = {}
    for g in GEOS:
        for v in TRIGS:
            for t in TS:
                rows = {}
                for s in SEEDS:
                    d = os.path.join(out, f"{g}_{v}_T{t}", f"s{s}")
                    r = cell(d, ARM) if cs.ok(d) else None
                    if r:
                        rows[s] = r + (esc5(d),)
                if len(rows) != len(SEEDS):
                    problems.append(f"{g}_{v}_T{t}: {len(rows)} seeds, not {len(SEEDS)}")
                D[(g, v, t)] = rows
    return D


def vec(D, key, i):
    return np.array([D[key][s][i] for s in SEEDS], dtype=float)


def interval(v, idx):
    m = v[idx].mean(axis=1)
    return float(v.mean()), float(np.quantile(m, 0.005)), float(np.quantile(m, 0.995))


def f(x, p=3):
    return f"{x[0]:+.{p}f} [{x[1]:+.{p}f}, {x[2]:+.{p}f}]"


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    sources = checks(out, problems)
    D = load(out, problems)
    md = ["# ORACLE-ESC — seeds 321-340 (scripts/oracle_esc_analyse.py)", "",
          f"Source(s): {', '.join(sources) or 'none'}. Intervals: paired seed bootstrap, {NBOOT} resamples, 99 %.", ""]
    if problems:
        md += ["## CHECKS FAILED — no verdict is read", ""] + [f"- {p}" for p in problems]
        print("\n".join(md))
        open(os.path.join(out, "ORACLE_ESC.md"), "w").write("\n".join(md) + "\n")
        return 1
    rng = np.random.default_rng(20261021)
    idx = {g: rng.integers(0, len(SEEDS), size=(NBOOT, len(SEEDS))) for g in GEOS}
    verdict = {}
    for g in GEOS:
        md += [f"## Geometry {g} ({'5 km' if g == 'A' else 'endgame'})", "",
               "| config | five-node A(25 m) | tag-0 A(25 m) | five-node J | tag-0 CEP95 (m) | five-node escalations |",
               "|---|---|---|---|---|---|"]
        for v in TRIGS:
            for t in TS:
                k = (g, v, t)
                md.append(f"| {v} T{t} | {vec(D, k, 1).mean():.3f} | {vec(D, k, 0).mean():.3f} | "
                          f"{vec(D, k, 3).mean():.2f} | {vec(D, k, 2).mean():.1f} | {vec(D, k, 4).mean():.1f} |")
        md += ["", "P1 (O escalates more than K at the same T): paired difference in five-node escalations", "",
               "| T | O - K escalations | P1 holds |", "|---|---|---|"]
        p1 = []
        for t in TS:
            e = interval(vec(D, (g, "O", t), 4) - vec(D, (g, "K", t), 4), idx[g])
            p1.append(bool(e[1] > 0))
            md.append(f"| {t} | {f(e, 1)} | {p1[-1]} |")
        md += ["", "| O T | K T | dA five-node A(25 m) | dJ five-node J | accuracy-qualifying | energy-qualifying "
               "| saving lower bound |", "|---|---|---|---|---|---|---|"]
        H, E = None, None
        for to in TS:
            for tk in TS:
                a = interval(vec(D, (g, "O", to), 1) - vec(D, (g, "K", tk), 1), idx[g])
                j = interval(vec(D, (g, "O", to), 3) - vec(D, (g, "K", tk), 3), idx[g])
                accq = bool(j[0] <= 0 and a[1] > 0)
                engq = bool(a[1] >= -0.02 and j[2] < 0)
                sav = -j[2] / vec(D, (g, "K", tk), 3).mean()
                if accq:
                    H = a[1] if H is None else max(H, a[1])
                if engq:
                    E = sav if E is None else max(E, sav)
                md.append(f"| {to} | {tk} | {f(a)} | {f(j, 2)} | {accq} | {engq} | "
                          f"{f'{sav:.3f}' if engq else '-'} |")
        hv = "NONE" if H is None else ("MATERIAL" if H >= 0.02 else "SMALL")
        ev = "NONE" if E is None else ("MATERIAL" if E >= 0.10 else "SMALL")
        verdict[g] = (hv, ev)
        md += ["", f"**Accuracy headroom {g}: {hv}**" + ("" if H is None else f" (H = {H:+.3f})") +
               f". **Energy headroom {g}: {ev}**" + ("" if E is None else f" (E = {E:.3f})") +
               f". P1 holds at every T: {all(p1)}.", ""]
    allnone = all(v == ("NONE", "NONE") for v in verdict.values())
    anymat = any("MATERIAL" in v for v in verdict.values())
    md += ["## Registered reading", "",
           "- A learned confidence input cannot improve this trigger in this simulator: " + str(allnone),
           "- A learned confidence input has room (some headroom MATERIAL): " + str(anymat), ""]
    print("\n".join(md))
    open(os.path.join(out, "ORACLE_ESC.md"), "w").write("\n".join(md) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
