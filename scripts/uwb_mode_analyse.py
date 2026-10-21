#!/usr/bin/env python3
"""UWB-MODE (report/PREREG_UWB_MODE.md): seeds 261-280, configurations A (5 km), B (endgame), C (8-gateway ring).
L = the preset as it stands (DW3000 modes mixed); M6 = --uwbMode=6m8; M8 = --uwbMode=850k. Primary: A(25 m)
(scripts/goal_report.py). Committed with the registration, before any cell.

  U1  M8 raises ca_tbl_full's five-node A(25 m) against M6 in B (paired interval above 0)
  U2  M8 raises ca_tbl_full's subject J against M6 in B (paired interval above 0)
  U3  L - M8 ca_tbl_full five-node A(25 m) in B lies inside [-0.02, 0.02]
  U4  M6 lowers ca_tbl_full's five-node A(25 m) against L in B (paired interval below 0)
  U5  M8's ca_tbl_full mean five-node A(25 m) in B is at least 0.95

ADOPTION (registered, between M6 and M8 only; L is not a candidate): the first of these that separates them decides --
  1. ca_tbl_full five-node A(25 m) in B, M8 - M6, interval excluding 0: the higher;
  2. ca_tbl_full tag-0 A(25 m) in A, M8 - M6, interval excluding 0: the higher;
  3. ca_tbl_full five-node A(25 m) in C, M8 - M6, interval excluding 0: the higher;
  4. ca_tbl_full subject J in B, M8 - M6, interval excluding 0: the lower;
  5. none separates: 850k.

usage: uwb_mode_analyse.py <outdir>
"""
import glob
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from goal_report import cell  # noqa: E402

ARMS = ["ca_tbl_full", "fixed_threshold", "static_868", "static_2p4", "poucet_fusion"]
VS = ["L", "M6", "M8"]
CF = ["A", "B", "C"]
MODE_LINE = {"L": None, "M6": "UWB mode 6m8:", "M8": "UWB mode 850k:"}


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "[ABC]_*", "s*")))
    if len(cells) != 180:
        problems.append(f"{len(cells)} cells, not 180")
    for d in cells:
        if not cs.ok(d):
            problems.append(f"{d}: rc not 0")
            continue
        m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
        if not m or m.group(2) != "0":
            problems.append(f"{d}: run_id.txt not dirty 0")
        else:
            sources.add(m.group(1))
        if not 261 <= int(d.rsplit("s", 1)[1]) <= 280:
            problems.append(f"{d}: seed outside 261-280")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        v = os.path.basename(os.path.dirname(d)).split("_", 1)[1]
        if v not in VS:
            problems.append(f"{d}: unknown variant {v}")
            continue
        has = {k: (MODE_LINE[k] in lg) for k in ("M6", "M8")}
        if has != {k: (k == v) for k in ("M6", "M8")}:
            problems.append(f"{d}: UWB mode line does not match the variant")
        if "ekf5 window smoother:" not in lg or "MS-1 tasking (pairs)" not in lg:
            problems.append(f"{d}: preset lines missing")
        if "magnetometer heading reference" not in lg or "stride noise 5 %" not in lg:
            problems.append(f"{d}: realistic-world lines missing")
        if any(int(x) for x in re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)):
            problems.append(f"{d}: window solver failures")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D, AIR = {}, {}
    for c in CF:
        for v in VS:
            for a in ARMS:
                D[(c, v, a)] = {}
                for d in sorted(glob.glob(os.path.join(out, f"{c}_{v}", "s*"))):
                    if cs.ok(d):
                        r = cell(d, a)
                        if r:
                            D[(c, v, a)][int(d.rsplit("s", 1)[1])] = r
                        if a == "ca_tbl_full":
                            row = cs.arm_row(d, a)
                            if row and "airtime_uwb" in row:
                                AIR.setdefault((c, v), []).append(float(row["airtime_uwb"]))
                if len(D[(c, v, a)]) != 20:
                    problems.append(f"{(c, v, a)}: {len(D[(c, v, a)])} seeds, not 20")
    md = ["# UWB-MODE — seeds 261-280 (scripts/uwb_mode_analyse.py)", "",
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
    md += ["", "CA-TBL UWB airtime per run, s (mean): " +
           "; ".join(f"{c} {v} {np.mean(AIR[(c, v)]):.4f}" for c in CF for v in VS if AIR.get((c, v)))]

    def pr(A, B, k):
        a, b = D[A], D[B]
        s = sorted(set(a) & set(b))
        return cs.ci([a[i][k] - b[i][k] for i in s]) if s else (float("nan"),) * 3

    for (X, Y) in (("M8", "M6"), ("M8", "L"), ("M6", "L")):
        md += ["", f"## Paired contrasts {X} - {Y}, every arm", "",
               "| config | arm | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |", "|---|---|---|---|---|---|"]
        for c in CF:
            for arm in ARMS:
                T, O = (c, X, arm), (c, Y, arm)
                md.append(f"| {c} | {arm} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 2), p=1)} | "
                          f"{cs.fmt(*pr(T, O, 3), p=2)} |")
    for v in ("M6", "M8"):
        md += ["", f"## CA-TBL - comparator under {v}", "",
               "| config | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |", "|---|---|---|---|---|"]
        for c in CF:
            for comp in ARMS[1:]:
                T, O = (c, v, "ca_tbl_full"), (c, v, comp)
                md.append(f"| {c} | {comp} | {cs.fmt(*pr(T, O, 1))} | {cs.fmt(*pr(T, O, 0))} | {cs.fmt(*pr(T, O, 3), p=2)} |")

    F = "ca_tbl_full"
    u1 = pr(("B", "M8", F), ("B", "M6", F), 1)
    u2 = pr(("B", "M8", F), ("B", "M6", F), 3)
    u3 = pr(("B", "L", F), ("B", "M8", F), 1)
    u4 = pr(("B", "M6", F), ("B", "L", F), 1)
    x = D[("B", "M8", F)]
    u5 = float(np.mean([r[1] for r in x.values()])) if x else float("nan")
    U1, U2 = u1[1] > 0.0, u2[1] > 0.0
    U3 = u3[1] >= -0.02 and u3[2] <= 0.02
    U4, U5 = u4[2] < 0.0, u5 >= 0.95
    md += ["", "## Registered predictions", "",
           f"- U1 M8 raises CA-TBL five-node A(25 m) against M6 in B: {'HOLDS' if U1 else 'FAILS'} ({cs.fmt(*u1)})",
           f"- U2 M8 raises CA-TBL subject J against M6 in B: {'HOLDS' if U2 else 'FAILS'} ({cs.fmt(*u2, p=2)})",
           f"- U3 L - M8 CA-TBL five-node A(25 m) in B inside [-0.02, 0.02]: {'HOLDS' if U3 else 'FAILS'} ({cs.fmt(*u3)})",
           f"- U4 M6 lowers CA-TBL five-node A(25 m) against L in B: {'HOLDS' if U4 else 'FAILS'} ({cs.fmt(*u4)})",
           f"- U5 M8's CA-TBL mean five-node A(25 m) in B >= 0.95: {'HOLDS' if U5 else 'FAILS'} ({u5:.3f})"]
    steps = [("five-node A(25 m) in B", pr(("B", "M8", F), ("B", "M6", F), 1), +1),
             ("tag-0 A(25 m) in A", pr(("A", "M8", F), ("A", "M6", F), 0), +1),
             ("five-node A(25 m) in C", pr(("C", "M8", F), ("C", "M6", F), 1), +1),
             ("subject J in B", pr(("B", "M8", F), ("B", "M6", F), 3), -1)]
    verdict = None
    for i, (nm, (m, lo, hi), sgn) in enumerate(steps, 1):
        sep = (lo > 0 or hi < 0) and not np.isnan(lo)
        md.append(f"- adoption step {i}, {nm}, M8 - M6: {cs.fmt(m, lo, hi, p=3 if sgn > 0 else 2)} -> "
                  f"{'separates' if sep else 'does not separate'}")
        if sep and verdict is None:
            verdict = ("850k" if (m > 0) == (sgn > 0) else "6m8", i)
    if verdict is None:
        verdict = ("850k", 5)
    md += ["", f"**ADOPTED: --uwbMode={verdict[0]}** (step {verdict[1]})", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "UWB_MODE.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
