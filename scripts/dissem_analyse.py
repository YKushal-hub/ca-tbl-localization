#!/usr/bin/env python3
"""DISSEM (report/PREREG_DISSEM.md): seeds 281-300, configurations A (5 km), B (endgame), C (8-gateway ring). OFF =
the preset after DESIGN_DECISIONS 18-24; ON = OFF + --beaconCarriesEntry. Primary: A(25 m) (scripts/goal_report.py).
Committed with the registration, before any cell.

  D1  ON raises ca_tbl_full's five-node A(25 m) against OFF in A (paired interval above 0)
  D2  ON moves ca_tbl_full's tag-0 A(25 m) in A by under 0.02 either way (paired interval inside [-0.02, 0.02])
  D3  ON lowers ca_tbl_full's five-node A(25 m) in no configuration (every paired lower bound above -0.02)
  D4  ON's ca_tbl_full subject J is at most 1.10 x OFF's in every configuration
  D5  ON lowers the median age of the entry a ca_tbl_full finder steers on, in A, by more than half

ADOPTION (registered): --beaconCarriesEntry joins the preset if D1, D3 and D4 all hold.

usage: dissem_analyse.py <outdir>
"""
import csv
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell  # noqa: E402

ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "poucet_fusion"]
VS = ["OFF", "ON"]
CF = ["A", "B", "C"]


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_O*", "s*")))
    if len(cells) != 120:
        problems.append(f"{len(cells)} cells, not 120")
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        if not 281 <= int(d.rsplit("s", 1)[1]) <= 300:
            problems.append(f"{d}: seed outside 281-300")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d)).split("_")[1]
        on = (v == "ON")
        if on != ("beacon entries (" in lg) or "ekf5 window smoother:" not in lg or "MS-1 tasking (pairs)" not in lg \
                or "UWB mode 850k:" not in lg:
            problems.append(f"{d}: variant or preset lines do not match the cell")
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
    md = ["# DISSEM — seeds 281-300 (scripts/dissem_analyse.py)", "",
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

    md += ["", "## Paired contrasts ON - OFF, every arm", "",
           "| config | arm | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
    for c in CF:
        for arm in ARMS:
            T, O = (c, "ON", arm), (c, "OFF", arm)
            md.append(f"| {c} | {arm} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 2), p=1)} | "
                      f"{cs.fmt(*pr(T, O, 3), p=2)} |")
    md += ["", "## CA-TBL - comparator under ON", "",
           "| config | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |", "|---|---|---|---|---|"]
    for c in CF:
        for comp in ARMS[1:]:
            T, O = (c, "ON", "ca_tbl_full"), (c, "ON", comp)
            md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 3), p=2)} |")

    def jratio(c):
        n = np.nanmean([v[3] for v in D[(c, "ON", "ca_tbl_full")].values()])
        o = np.nanmean([v[3] for v in D[(c, "OFF", "ca_tbl_full")].values()])
        return n / o if o > 0 else float("nan")

    def age(c, v):
        a = []
        for d in sorted(glob.glob(os.path.join(out, f"{c}_{v}", "s*"))):
            for f in glob.glob(os.path.join(d, "tasking_ca_tbl_full_seed*.csv")):
                for r in csv.DictReader(open(f)):
                    if r["entryT"] not in ("", "nan"):
                        a.append(float(r["t"]) - float(r["entryT"]))
        return float(np.median(a)) if a else float("nan")

    F = "ca_tbl_full"
    d1 = pr(("A", "ON", F), ("A", "OFF", F), 1)
    d2 = pr(("A", "ON", F), ("A", "OFF", F), 0)
    d3 = {c: pr((c, "ON", F), (c, "OFF", F), 1) for c in CF}
    d4 = {c: jratio(c) for c in CF}
    aon, aoff = age("A", "ON"), age("A", "OFF")
    D1, D2 = d1[1] > 0.0, (d2[1] >= -0.02 and d2[2] <= 0.02)
    D3 = all(d3[c][1] > -0.02 for c in CF)
    D4 = all(d4[c] <= 1.10 for c in CF)
    D5 = aon < 0.5 * aoff
    md += ["", "## Registered predictions", "",
           f"- D1 ON raises CA-TBL five-node A(25 m) in A: {'HOLDS' if D1 else 'FAILS'} ({cs.fmt(*d1)})",
           f"- D2 ON moves CA-TBL tag-0 A(25 m) in A by under 0.02: {'HOLDS' if D2 else 'FAILS'} ({cs.fmt(*d2)})",
           f"- D3 ON lowers CA-TBL five-node A(25 m) nowhere (lower bounds > -0.02): {'HOLDS' if D3 else 'FAILS'} "
           f"({'; '.join(c + ' ' + cs.fmt(*d3[c]) for c in CF)})",
           f"- D4 ON's subject J within +10 % of OFF's: {'HOLDS' if D4 else 'FAILS'} ({'; '.join(f'{c} {d4[c]:.3f}' for c in CF)})",
           f"- D5 ON halves the steering entry age in A: {'HOLDS' if D5 else 'FAILS'} (median {aon:.1f} s against {aoff:.1f} s)",
           "", f"**ADOPTION: --beaconCarriesEntry {'JOINS the preset' if (D1 and D3 and D4) else 'does NOT join the preset'}** "
               "(D1, D3 and D4 all hold)", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "DISSEM.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
