#!/usr/bin/env python3
"""REALISTIC-1 (report/PREREG_REALISTIC1.md). Committed with the registration, before any cell.

Cells <SITE>_<F>_<V>/s<seed>: SITE in ROAD1, ROAD3, CLUS3, RING12; F in F2 (2 searchers), F0 (none); V in ARMS
(ca_tbl_full, fixed_threshold, static_868, static_2p4, poucet_fusion), TII2 (fixed_threshold under the TII-2 rule).
Per run and design (scripts/realistic_analyse.py): t25, t5 (s from the alarm to the lost worker's first estimate within
25 m / 5 m), treach (s to a searcher within 10 m), A250 (share of all five workers' cycles within 250 m), J/h.
"Never" is censored at the 4200 s from the alarm to the end of the run (restricted mean); the share of runs that
never get there is reported beside it. Intervals: paired by seed, 95 % bootstrap (camp_spec_analyse.ci).

  H1  In every realistic cell (ROAD1, ROAD3, CLUS3 x F2, F0), ca_tbl_full's censored t25 is no worse than the better
      of static_868 and static_2p4 by more than 120 s (paired upper bound < 120), and each of those two single-radio
      designs is worse than ca_tbl_full by more than 300 s (paired lower bound > 300) in at least one realistic cell.
  H2  In every realistic F2 cell, ca_tbl_full's censored treach is no worse than static_2p4's by more than 120 s, and
      better by more than 300 s in at least one.
  H3  In every realistic cell, ca_tbl_full's A250 is no worse than the better single-radio design's by more than 0.02.
  H4  (reported, no threshold) J/h of every design, and TII2 minus ca_tbl_full J/h with its interval, per cell.
  H5  (reported, no threshold) the same table for <SITE>_F2_MUL (the field-measured 2.4 GHz error, --tofErrorModel=muller),
      realistic sites, 2 searchers, the five arms, with ours - 2.4 only on t25 and treach.
  RING12 is the reference: reported, not part of H1-H3.

usage: realistic1_analyse.py <outdir>
"""
import glob
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import realistic_analyse as ra  # noqa: E402

SEEDS = list(range(501, 521))
SITES = ["ROAD1", "ROAD3", "CLUS3", "RING12"]
REAL = ["ROAD1", "ROAD3", "CLUS3"]
FS = ["F2", "F0"]
DESIGNS = [("ARMS", "ca_tbl_full"), ("ARMS", "fixed_threshold"), ("ARMS", "static_868"), ("ARMS", "static_2p4"),
           ("ARMS", "poucet_fusion"), ("TII2", "fixed_threshold")]
LAB = {("ARMS", "ca_tbl_full"): "ours", ("ARMS", "fixed_threshold"): "threshold", ("ARMS", "static_868"): "868 only",
       ("ARMS", "static_2p4"): "2.4 only", ("ARMS", "poucet_fusion"): "all on", ("TII2", "fixed_threshold"): "TII-2"}
CENS = 4200.0
KEYS = ["t25_s", "t5_s", "treach_s", "A250_five", "subjJ_per_h"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources, D = [], set(), {}
    cells = [(site, f, v, arm, f"{site}_{f}_{v}", (site, f, LAB[(v, arm)])) for site in SITES for f in FS for v, arm in DESIGNS]
    cells += [(site, "F2", "MUL", arm, f"{site}_F2_MUL", (site, "F2M", LAB[("ARMS", arm)]))
              for site in REAL for v, arm in DESIGNS if v == "ARMS"]
    for site, f, v, arm, cdir, key in cells:
        D[key] = {}
        for s in SEEDS:
            d = os.path.join(out, cdir, f"s{s}")
            if not cs.ok(d):
                problems.append(f"{d}: missing or rc not 0")
                continue
            m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
            if not m or m.group(2) != "0":
                problems.append(f"{d}: not dirty 0")
            else:
                sources.add(m.group(1))
            try:
                r = {**ra.emergency(d, arm), **ra.quiet(d, arm)}
            except Exception as e:  # noqa: BLE001
                problems.append(f"{d} {arm}: {e}")
                continue
            if not (abs(r["alarm_s"] - 3000.0) < 3.0):
                problems.append(f"{d} {arm}: alarm at {r['alarm_s']}, not 3000 s")
            for k in ("t25_s", "t5_s", "treach_s"):
                r[k + "_never"] = math.isnan(r[k])
                r[k] = CENS if math.isnan(r[k]) else r[k]
            D[key][s] = r
        if len(D[key]) != len(SEEDS):
            problems.append(f"{key}: {len(D[key])} seeds, not {len(SEEDS)}")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")

    def mean(key, k):
        x = D.get(key, {})
        return float(np.mean([r[k] for r in x.values()])) if x else float("nan")

    def never(key, k):
        x = D.get(key, {})
        return float(np.mean([r[k + "_never"] for r in x.values()])) if x else float("nan")

    def pr(a, b, k):
        x, y = D.get(a, {}), D.get(b, {})
        s = sorted(set(x) & set(y))
        return cs.ci([x[i][k] - y[i][k] for i in s]) if s else (float("nan"),) * 3

    md = ["# REALISTIC-1 (scripts/realistic1_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    for site in SITES:
        for f in FS:
            md += [f"## {site}, {'2 searchers' if f == 'F2' else 'no searchers'}", "",
                   "| design | t to 25 m (s) [never] | t to 5 m (s) [never] | searcher within 10 m (s) [never] | A250 | J/h |",
                   "|---|---|---|---|---|---|"]
            for v, arm in DESIGNS:
                key = (site, f, LAB[(v, arm)])
                md.append(f"| {LAB[(v, arm)]} | {mean(key, 't25_s'):.0f} [{never(key, 't25_s'):.2f}] | "
                          f"{mean(key, 't5_s'):.0f} [{never(key, 't5_s'):.2f}] | {mean(key, 'treach_s'):.0f} [{never(key, 'treach_s'):.2f}] | "
                          f"{mean(key, 'A250_five'):.3f} | {mean(key, 'subjJ_per_h'):.1f} |")
            md.append("")
    cellsR = [(s, f) for s in REAL for f in FS]
    h1a, h1b = True, {"868 only": False, "2.4 only": False}
    md += ["## H1 (time to 25 m after the alarm)", "", "| cell | ours - better single (s) | ours - 868 only | ours - 2.4 only |", "|---|---|---|---|"]
    for s, f in cellsR:
        best = min(("868 only", "2.4 only"), key=lambda lab: mean((s, f, lab), "t25_s"))
        dbest = pr((s, f, "ours"), (s, f, best), "t25_s")
        d8, d2 = pr((s, f, "ours"), (s, f, "868 only"), "t25_s"), pr((s, f, "ours"), (s, f, "2.4 only"), "t25_s")
        h1a = h1a and dbest[2] < 120.0
        h1b["868 only"] = h1b["868 only"] or (-d8[2] > 300.0)
        h1b["2.4 only"] = h1b["2.4 only"] or (-d2[2] > 300.0)
        md.append(f"| {s} {f} | {cs.fmt(*dbest, p=0)} ({best}) | {cs.fmt(*d8, p=0)} | {cs.fmt(*d2, p=0)} |")
    md += ["", f"**H1: {'HOLDS' if h1a and all(h1b.values()) else 'FAILS'}** (non-inferior everywhere: {h1a}; "
               f"each single radio clearly worse somewhere: {h1b})", ""]
    h2a, h2b = True, False
    md += ["## H2 (searcher reaches the worker, 2 searchers)", "", "| cell | ours - 2.4 only (s) |", "|---|---|"]
    for s in REAL:
        d = pr((s, "F2", "ours"), (s, "F2", "2.4 only"), "treach_s")
        h2a = h2a and d[2] < 120.0
        h2b = h2b or (-d[2] > 300.0)
        md.append(f"| {s} F2 | {cs.fmt(*d, p=0)} |")
    md += ["", f"**H2: {'HOLDS' if h2a and h2b else 'FAILS'}** (non-inferior: {h2a}; clearly better somewhere: {h2b})", ""]
    h3 = True
    md += ["## H3 (rough position, A250)", "", "| cell | ours - better single |", "|---|---|"]
    for s, f in cellsR:
        best = max(("868 only", "2.4 only"), key=lambda lab: mean((s, f, lab), "A250_five"))
        d = pr((s, f, "ours"), (s, f, best), "A250_five")
        h3 = h3 and d[1] >= -0.02
        md.append(f"| {s} {f} | {cs.fmt(*d)} ({best}) |")
    md += ["", f"**H3: {'HOLDS' if h3 else 'FAILS'}**", "", "## H4 (energy, reported)", "", "| cell | TII-2 - ours (J/h) |", "|---|---|"]
    for s in SITES:
        for f in FS:
            md.append(f"| {s} {f} | {cs.fmt(*pr((s, f, 'TII-2'), (s, f, 'ours'), 'subjJ_per_h'), p=1)} |")
    md += ["", "## H5 (reported) field-measured 2.4 GHz error (Mueller et al. 2021), 2 searchers", "",
           "| site | design | t to 25 m (s) [never] | t to 5 m (s) [never] | searcher within 10 m (s) [never] | A250 | J/h |",
           "|---|---|---|---|---|---|---|"]
    for site in REAL:
        for v, arm in DESIGNS:
            if v != "ARMS":
                continue
            key = (site, "F2M", LAB[(v, arm)])
            md.append(f"| {site} | {LAB[(v, arm)]} | {mean(key, 't25_s'):.0f} [{never(key, 't25_s'):.2f}] | "
                      f"{mean(key, 't5_s'):.0f} [{never(key, 't5_s'):.2f}] | {mean(key, 'treach_s'):.0f} [{never(key, 'treach_s'):.2f}] | "
                      f"{mean(key, 'A250_five'):.3f} | {mean(key, 'subjJ_per_h'):.1f} |")
        md.append(f"| {site} | ours - 2.4 only (t25) | {cs.fmt(*pr((site, 'F2M', 'ours'), (site, 'F2M', '2.4 only'), 't25_s'), p=0)} | | "
                  f"{cs.fmt(*pr((site, 'F2M', 'ours'), (site, 'F2M', '2.4 only'), 'treach_s'), p=0)} | | |")
    md += ["", f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC1.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
