#!/usr/bin/env python3
"""REALISTIC-4 (report/PREREG_REALISTIC4.md). Committed with the registration, before any cell.

Cells <SITE>_<V>/s<seed>, all with 2 searchers (ROAD3F4: 4), every run with the search party fixed (DD 30-32 and
--finderStaleSearch): ROAD1, ROAD3, CLUS3 seeds 501-520 (ROAD with --acquireSideOfRoad); RAND3, ROAD3F4 seeds 521-540.
V: ARMS (ca_tbl_full, fixed_threshold, static_868, static_2p4, poucet_fusion), TII2. Measures and statistics as
REALISTIC-1 ("never" = 4200 s, paired 95 % bootstrap over seeds).

  F1  (H2 with the party fixed) in every cell, ours' treach is no worse than 2.4 only's by more than 120 s (paired
      upper bound < 120 s), and better by more than 300 s (paired lower bound) in at least one cell.
  F2  (H1) in every cell, ours' t25 is no worse than the better single-radio design's by more than 120 s.
  F3  (H3) in every cell, ours' A250 is no worse than the better single-radio design's by more than 0.02.
  F4  (reported) each design's treach with the party fixed minus without it, against the same seeds and flags of
      REALISTIC-3 (ROAD1, ROAD3), REALISTIC-1 (CLUS3) and REALISTIC-2 (RAND3, ROAD3F4 = E_RAND3, E_ROAD3F4; those had
      no side-of-road prior, so ROAD3F4's difference includes it).

usage: realistic4_analyse.py <R4 outdir> <R1 outdir> <R2 outdir> <R3 outdir>
"""
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import realistic_analyse as ra  # noqa: E402

SITES = {"ROAD1": range(501, 521), "ROAD3": range(501, 521), "CLUS3": range(501, 521), "RAND3": range(521, 541),
         "ROAD3F4": range(521, 541)}
DESIGNS = [("ARMS", "ca_tbl_full"), ("ARMS", "fixed_threshold"), ("ARMS", "static_868"), ("ARMS", "static_2p4"),
           ("ARMS", "poucet_fusion"), ("TII2", "fixed_threshold")]
LAB = {("ARMS", "ca_tbl_full"): "ours", ("ARMS", "fixed_threshold"): "threshold", ("ARMS", "static_868"): "868 only",
       ("ARMS", "static_2p4"): "2.4 only", ("ARMS", "poucet_fusion"): "all on", ("TII2", "fixed_threshold"): "TII-2"}
CENS = 4200.0
SINGLE = ("868 only", "2.4 only")


def load_cell(d, arm, problems, check):
    if not cs.ok(d):
        problems.append(f"{d}: missing or rc not 0")
        return None
    log = open(os.path.join(d, "run.log"), errors="replace").read()
    if check:
        for k in ("DD30 finderDigestOnArmTier", "DD31 finderSweepFromDispatch", "DD32 finderLkpFirst"):
            if k not in log:
                problems.append(f"{d}: {k} not announced")
    try:
        r = {**ra.emergency(d, arm), **ra.quiet(d, arm)}
    except Exception as e:  # noqa: BLE001
        problems.append(f"{d} {arm}: {e}")
        return None
    if check and not abs(r["alarm_s"] - 3000.0) < 3.0:
        problems.append(f"{d} {arm}: alarm at {r['alarm_s']}")
    for k in ("t25_s", "t5_s", "treach_s"):
        r[k + "_never"] = math.isnan(r[k])
        r[k] = CENS if math.isnan(r[k]) else r[k]
    return r


def main():
    if len(sys.argv) < 5:
        print(__doc__)
        return 2
    out, r1, r2, r3 = sys.argv[1:5]
    problems, sources, D, B = [], set(), {}, {}
    base = {"ROAD1": (r3, "ROAD1_F2_{v}"), "ROAD3": (r3, "ROAD3_F2_{v}"), "CLUS3": (r1, "CLUS3_F2_{v}"),
            "RAND3": (r2, "E_RAND3_{v}"), "ROAD3F4": (r2, "E_ROAD3F4_{v}")}
    for site, seeds in SITES.items():
        for v, arm in DESIGNS:
            key = (site, LAB[(v, arm)])
            D[key], B[key] = {}, {}
            for s in seeds:
                d = os.path.join(out, f"{site}_{v}", f"s{s}")
                if cs.ok(d):
                    m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
                    if not m or m.group(2) != "0":
                        problems.append(f"{d}: not dirty 0")
                    else:
                        sources.add(m.group(1))
                r = load_cell(d, arm, problems, True)
                if r:
                    D[key][s] = r
                bd, pat = base[site]
                rb = load_cell(os.path.join(bd, pat.format(v=v), f"s{s}"), arm, [], False)
                if rb:
                    B[key][s] = rb
            if len(D[key]) != len(seeds):
                problems.append(f"{key}: {len(D[key])} seeds")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")

    def mean(X, key, k):
        x = X.get(key, {})
        return float(np.mean([r[k] for r in x.values()])) if x else float("nan")

    def never(X, key, k):
        x = X.get(key, {})
        return float(np.mean([r[k + "_never"] for r in x.values()])) if x else float("nan")

    def pr(X, a, Y, b, k):
        x, y = X.get(a, {}), Y.get(b, {})
        s = sorted(set(x) & set(y))
        return cs.ci([x[i][k] - y[i][k] for i in s]) if s else (float("nan"),) * 3

    md = ["# REALISTIC-4 (scripts/realistic4_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    for site in SITES:
        md += [f"## {site} (search party fixed)", "",
               "| design | t25 (s) [never] | t5 (s) [never] | treach (s) [never] | A250 | J/h | treach minus unfixed (s) |",
               "|---|---|---|---|---|---|---|"]
        for v, arm in DESIGNS:
            k = (site, LAB[(v, arm)])
            md.append(f"| {k[1]} | {mean(D, k, 't25_s'):.0f} [{never(D, k, 't25_s'):.2f}] | {mean(D, k, 't5_s'):.0f} [{never(D, k, 't5_s'):.2f}] | "
                      f"{mean(D, k, 'treach_s'):.0f} [{never(D, k, 'treach_s'):.2f}] | {mean(D, k, 'A250_five'):.3f} | "
                      f"{mean(D, k, 'subjJ_per_h'):.1f} | {cs.fmt(*pr(D, k, B, k, 'treach_s'), p=0)} |")
        md.append("")
    f1a, f1b, f2, f3 = True, False, True, True
    md += ["## F1-F3", "", "| cell | ours - 2.4 only treach (s) | ours - better single t25 (s) | ours - better single A250 |", "|---|---|---|---|"]
    for site in SITES:
        d1 = pr(D, (site, "ours"), D, (site, "2.4 only"), "treach_s")
        b2 = min(SINGLE, key=lambda lab: mean(D, (site, lab), "t25_s"))
        d2 = pr(D, (site, "ours"), D, (site, b2), "t25_s")
        b3 = max(SINGLE, key=lambda lab: mean(D, (site, lab), "A250_five"))
        d3 = pr(D, (site, "ours"), D, (site, b3), "A250_five")
        f1a = f1a and d1[2] < 120.0
        f1b = f1b or -d1[2] > 300.0
        f2 = f2 and d2[2] < 120.0
        f3 = f3 and d3[1] >= -0.02
        md.append(f"| {site} | {cs.fmt(*d1, p=0)} | {cs.fmt(*d2, p=0)} ({b2}) | {cs.fmt(*d3)} ({b3}) |")
    md += ["", f"**F1: {'HOLDS' if f1a and f1b else 'FAILS'}** (non-inferior: {f1a}; clearly better somewhere: {f1b})",
           f"**F2: {'HOLDS' if f2 else 'FAILS'}**  **F3: {'HOLDS' if f3 else 'FAILS'}**", "",
           f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC4.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
