#!/usr/bin/env python3
"""REALISTIC-3 (report/PREREG_REALISTIC3.md). Committed with the registration, before any cell.

Cells <SITE>_<F>_<V>/s<seed> as REALISTIC-1 (SITE ROAD1, ROAD3; F F2, F0; V ARMS, TII2; ROAD3_F2_MUL), seeds 501-520,
every run with --acquireSideOfRoad. Measures and statistics as REALISTIC-1 ("never" = 4200 s, paired 95 % bootstrap).

  S1  the flag's effect: in ROAD3_F0, for ca_tbl_full and for static_2p4, REALISTIC-3 minus REALISTIC-1 t25 (same seed,
      same flags but the one) has a paired upper bound < 0; every design and cell's difference is reported.
  S2  (H1) in every cell, ca_tbl_full's t25 is no worse than the better single-radio design's by more than 120 s
      (paired upper bound < 120 s), and each single-radio design is worse by more than 300 s (lower bound) in at least one.
  S3  (H2) in every F2 cell, ca_tbl_full's treach is no worse than static_2p4's by more than 120 s, and better by more than
      300 s in at least one.
  S4  (H3) in every cell, ca_tbl_full's A250 is no worse than the better single-radio design's by more than 0.02.

usage: realistic3_analyse.py <REALISTIC-3 outdir> <REALISTIC-1 outdir>
"""
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import realistic_analyse as ra  # noqa: E402

SEEDS = list(range(501, 521))
CELLS = [(s, f) for s in ("ROAD1", "ROAD3") for f in ("F2", "F0")]
DESIGNS = [("ARMS", "ca_tbl_full"), ("ARMS", "fixed_threshold"), ("ARMS", "static_868"), ("ARMS", "static_2p4"),
           ("ARMS", "poucet_fusion"), ("TII2", "fixed_threshold")]
LAB = {("ARMS", "ca_tbl_full"): "ours", ("ARMS", "fixed_threshold"): "threshold", ("ARMS", "static_868"): "868 only",
       ("ARMS", "static_2p4"): "2.4 only", ("ARMS", "poucet_fusion"): "all on", ("TII2", "fixed_threshold"): "TII-2"}
CENS = 4200.0
SINGLE = ("868 only", "2.4 only")


def load(out, problems, sources, check):
    D = {}
    runs = [(s, f, v, arm, f"{s}_{f}_{v}") for s, f in CELLS for v, arm in DESIGNS]
    runs += [("ROAD3", "F2M", "ARMS", arm, "ROAD3_F2_MUL") for v, arm in DESIGNS if v == "ARMS"]
    for site, f, v, arm, cdir in runs:
        key = (site, f, LAB[(v, arm)])
        D[key] = {}
        for s in SEEDS:
            d = os.path.join(out, cdir, f"s{s}")
            if not cs.ok(d):
                problems.append(f"{d}: missing or rc not 0")
                continue
            if check:
                m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
                if not m or m.group(2) != "0":
                    problems.append(f"{d}: not dirty 0")
                else:
                    sources.add(m.group(1))
                if "DD29 acquireSideOfRoad" not in open(os.path.join(d, "run.log"), errors="replace").read():
                    problems.append(f"{d}: the flag not announced")
            try:
                r = {**ra.emergency(d, arm), **ra.quiet(d, arm)}
            except Exception as e:  # noqa: BLE001
                problems.append(f"{d} {arm}: {e}")
                continue
            if check and not abs(r["alarm_s"] - 3000.0) < 3.0:
                problems.append(f"{d} {arm}: alarm at {r['alarm_s']}")
            for k in ("t25_s", "t5_s", "treach_s"):
                r[k + "_never"] = math.isnan(r[k])
                r[k] = CENS if math.isnan(r[k]) else r[k]
            D[key][s] = r
        if check and len(D[key]) != len(SEEDS):
            problems.append(f"{key}: {len(D[key])} seeds")
    return D


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    out, r1 = sys.argv[1], sys.argv[2]
    problems, sources = [], set()
    D = load(out, problems, sources, True)
    B = load(r1, [], set(), False)
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

    md = ["# REALISTIC-3 (scripts/realistic3_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    for site, f in CELLS + [("ROAD3", "F2M")]:
        md += [f"## {site} {f} (with the side of the road known)", "",
               "| design | t25 (s) [never] | t5 (s) [never] | treach (s) [never] | A250 | J/h | t25 minus REALISTIC-1 (s) |",
               "|---|---|---|---|---|---|---|"]
        for v, arm in DESIGNS:
            k = (site, f, LAB[(v, arm)])
            if not D.get(k):
                continue
            md.append(f"| {k[2]} | {mean(D, k, 't25_s'):.0f} [{never(D, k, 't25_s'):.2f}] | {mean(D, k, 't5_s'):.0f} [{never(D, k, 't5_s'):.2f}] | "
                      f"{mean(D, k, 'treach_s'):.0f} [{never(D, k, 'treach_s'):.2f}] | {mean(D, k, 'A250_five'):.3f} | "
                      f"{mean(D, k, 'subjJ_per_h'):.1f} | {cs.fmt(*pr(D, k, B, k, 't25_s'), p=0)} |")
        md.append("")
    s1 = all(pr(D, ("ROAD3", "F0", lab), B, ("ROAD3", "F0", lab), "t25_s")[2] < 0 for lab in ("ours", "2.4 only"))
    md += [f"**S1: {'HOLDS' if s1 else 'FAILS'}** (ROAD3 F0, ours and 2.4 only, t25 with minus without the flag)", ""]
    s2a, s2b = True, {lab: False for lab in SINGLE}
    s4 = True
    md += ["## S2 / S4", "", "| cell | ours - better single t25 (s) | ours - 868 only | ours - 2.4 only | ours - better single A250 |",
           "|---|---|---|---|---|"]
    for site, f in CELLS:
        b = min(SINGLE, key=lambda lab: mean(D, (site, f, lab), "t25_s"))
        d = pr(D, (site, f, "ours"), D, (site, f, b), "t25_s")
        d8 = pr(D, (site, f, "ours"), D, (site, f, "868 only"), "t25_s")
        d2 = pr(D, (site, f, "ours"), D, (site, f, "2.4 only"), "t25_s")
        b4 = max(SINGLE, key=lambda lab: mean(D, (site, f, lab), "A250_five"))
        a4 = pr(D, (site, f, "ours"), D, (site, f, b4), "A250_five")
        s2a = s2a and d[2] < 120.0
        s2b["868 only"] = s2b["868 only"] or -d8[2] > 300.0
        s2b["2.4 only"] = s2b["2.4 only"] or -d2[2] > 300.0
        s4 = s4 and a4[1] >= -0.02
        md.append(f"| {site} {f} | {cs.fmt(*d, p=0)} ({b}) | {cs.fmt(*d8, p=0)} | {cs.fmt(*d2, p=0)} | {cs.fmt(*a4)} ({b4}) |")
    md += ["", f"**S2: {'HOLDS' if s2a and all(s2b.values()) else 'FAILS'}** (non-inferior: {s2a}; clearly better than each: {s2b})",
           f"**S4: {'HOLDS' if s4 else 'FAILS'}**", ""]
    s3a, s3b = True, False
    md += ["## S3 (searcher within 10 m)", "", "| cell | ours - 2.4 only (s) |", "|---|---|"]
    for site in ("ROAD1", "ROAD3"):
        d = pr(D, (site, "F2", "ours"), D, (site, "F2", "2.4 only"), "treach_s")
        s3a = s3a and d[2] < 120.0
        s3b = s3b or -d[2] > 300.0
        md.append(f"| {site} F2 | {cs.fmt(*d, p=0)} |")
    md += ["", f"**S3: {'HOLDS' if s3a and s3b else 'FAILS'}**", "", f"Problems: {len(problems)}" + ("" if problems else " (none)")]
    md += [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC3.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
