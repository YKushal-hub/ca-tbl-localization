#!/usr/bin/env python3
"""MAG (report/PREREG_MAG.md): a magnetometer heading reference (--imuMagSigmaDeg=3) on REAL-IMU's realistic-IMU
ekf5 cells, paired by seed with REAL-IMU's ekf5 cells without it (guard off). Committed with the registration,
before any cell; not edited after the first cell.

  R1  2 of 2 cells without the magnetometer on this binary reproduce REAL-IMU's arm_summary.csv (A s1, B s1)
  P1  the magnetometer lowers ca_tbl_full's tag 0 CEP95 in B (paired interval below 0)
  P2  fewer ca_tbl_full seeds end with a heading offset error > 0.3 rad, A and B pooled (40 seed-cells)
  P3  with the magnetometer, CA-TBL - poucet_fusion tag 0 CEP95 in B has an interval upper bound below +1.0 m

Estimators: lora_cmp_analyse.per_seed, camp_spec_analyse.ci, post_acq_metrics.post_acq, v3_analyse.node_rows.
usage: mag_analyse.py <outdir> <REAL-IMU outdir>
"""
import glob
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
from v3_analyse import node_rows  # noqa: E402

ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "poucet_fusion"]
HEAD = re.compile(r"N\.28 ekf5: .*heading offset est (\S+) \(true (\S+)\) rad")
ROW = re.compile(r"^  (" + "|".join(ARMS) + r")\s+\d")


def heads(d):
    out, pend = {}, None
    p = os.path.join(d, "run.log")
    if not os.path.exists(p):
        return out
    for ln in open(p, errors="replace"):
        m = HEAD.search(ln)
        if m:
            e, t = float(m.group(1)), float(m.group(2))
            pend = abs(math.atan2(math.sin(e - t), math.cos(e - t)))
            continue
        r = ROW.match(ln)
        if r and pend is not None:
            out[r.group(1)] = pend
            pend = None
    return out


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    out, ref = sys.argv[1], sys.argv[2]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[AB]_mag", "s*")))
    r1c = sorted(glob.glob(os.path.join(out, "[AB]_nomag", "s*")))
    if len(cells) != 40 or len(r1c) != 2:
        problems.append(f"{len(cells)} magnetometer cells and {len(r1c)} R1 cells, not 40 and 2")
    for d in cells + r1c:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        if ("_mag/" in d + "/") != ("magnetometer heading reference" in lg):
            problems.append(f"{d}: magnetometer line does not match the cell")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    same = 0
    for c in "AB":
        a, b = os.path.join(out, f"{c}_nomag", "s1", "arm_summary.csv"), os.path.join(ref, f"{c}_ekf5_g0", "s1", "arm_summary.csv")
        same += os.path.exists(a) and os.path.exists(b) and open(a, "rb").read() == open(b, "rb").read()
    r1 = same == 2
    md = ["# MAG — a magnetometer heading reference on the realistic-IMU preset (scripts/mag_analyse.py)", "",
          f"Sources: {', '.join(sorted(sources)) or 'none'}; without the magnetometer: REAL-IMU at {ref}.", "",
          f"R1: {same} of 2 cells reproduce REAL-IMU -> {'HOLDS' if r1 else 'FAILS'}", "",
          "| config | arm | CEP95 tag 0 no mag → mag (m) | Δ CEP95 tag 0 | Δ availability | heading err > 0.3 rad no mag → mag | coverage no mag → mag |",
          "|---|---|---|---|---|---|---|"]
    D, H = {}, {}
    for c in "AB":
        for a in ARMS:
            D[(c, "m", a)] = per_seed(out, f"{c}_mag", a)
            D[(c, "n", a)] = per_seed(ref, f"{c}_ekf5_g0", a)
            if len(D[(c, "m", a)]) != 20:
                problems.append(f"{c}_mag {a}: {len(D[(c, 'm', a)])} seeds, not 20")
        for s in range(1, 21):
            H[(c, "m", s)] = heads(os.path.join(out, f"{c}_mag", f"s{s}"))
            H[(c, "n", s)] = heads(os.path.join(ref, f"{c}_ekf5_g0", f"s{s}"))
        for a in ARMS:
            e, v = D[(c, "m", a)], D[(c, "n", a)]
            s = sorted(set(e) & set(v))
            if not s:
                continue
            dc = cs.ci([e[i]["c95"] - v[i]["c95"] for i in s])
            da = cs.ci([e[i]["av"] - v[i]["av"] for i in s])
            hn = sum(1 for i in s if H[(c, "n", i)].get(a, 0) > 0.3)
            hm = sum(1 for i in s if H[(c, "m", i)].get(a, 0) > 0.3)
            cov = []
            for tag, base, cell in (("n", ref, f"{c}_ekf5_g0"), ("m", out, f"{c}_mag")):
                C = []
                for d in cs.seeds(base, cell):
                    C += node_rows(d, a)[1]
                cov.append(float(np.mean(C)) if C else float("nan"))
            md.append(f"| {c} | {a} | {np.mean([v[i]['c95'] for i in s]):.1f} → {np.mean([e[i]['c95'] for i in s]):.1f} | "
                      f"{cs.fmt(*dc, p=1)} | {cs.fmt(*da)} | {hn} → {hm} | {cov[0]:.3f} → {cov[1]:.3f} |")
    md += ["", "## CA-TBL − comparator with the magnetometer, paired by seed", "",
           "| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
    cp = {}
    for c in "AB":
        for comp in ARMS[1:]:
            t, o = D[(c, "m", "ca_tbl_full")], D[(c, "m", comp)]
            s = sorted(set(t) & set(o))
            if not s:
                continue
            g = {k: cs.ci([t[i][k] - o[i][k] for i in s]) for k in ("av", "c95", "J")}
            cp[(c, comp)] = g
            md.append(f"| {c} | {comp} | {cs.fmt(*g['av'])} | {cs.fmt(*g['c95'], p=1)} | {cs.fmt(*g['J'], p=2)} |")
    t, v = D[("B", "m", "ca_tbl_full")], D[("B", "n", "ca_tbl_full")]
    s = sorted(set(t) & set(v))
    p1d = cs.ci([t[i]["c95"] - v[i]["c95"] for i in s]) if s else (float("nan"),) * 3
    p1 = p1d[2] < 0.0
    both = [(c, i) for c in "AB" for i in range(1, 21)
            if "ca_tbl_full" in H[(c, "n", i)] and "ca_tbl_full" in H[(c, "m", i)]]
    hn = sum(1 for c, i in both if H[(c, "n", i)]["ca_tbl_full"] > 0.3)
    hm = sum(1 for c, i in both if H[(c, "m", i)]["ca_tbl_full"] > 0.3)
    p2 = len(both) == 40 and hm < hn
    p3d = cp.get(("B", "poucet_fusion"), {}).get("c95", (float("nan"),) * 3)
    p3 = p3d[2] < 1.0
    md += ["", "## Registered predictions", "",
           f"- R1 2 of 2 cells reproduce REAL-IMU: {'HOLDS' if r1 else 'FAILS'}",
           f"- P1 the magnetometer lowers CA-TBL CEP95 in B: {'HOLDS' if p1 else 'FAILS'} ({cs.fmt(*p1d, p=1)})",
           f"- P2 fewer CA-TBL heading divergences (> 0.3 rad), A and B pooled: {'HOLDS' if p2 else 'FAILS'} ({hn} -> {hm} of {len(both)})",
           f"- P3 B, CA-TBL - Poucet CEP95 upper bound below +1.0 m: {'HOLDS' if p3 else 'FAILS'} ({cs.fmt(*p3d, p=1)})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "MAG.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
