#!/usr/bin/env python3
"""DEPLOY-REQ (report/PREREG_DEPLOY_REQ.md): what the brief's 5 km geometry needs to meet A >= 0.95 at R <= 25 m.
Seeds 301-320; the preset unchanged; gateways g3 (the brief's 3), g8 / g12 (an 8- / 12-gateway ring) x finders 8, 12,
16, 24. Primary: A(25 m) (scripts/goal_report.py). Committed with the registration, before any cell.

  E1  g3 with 24 finders: CA-TBL five-node mean A(25 m) < 0.95 (finders alone do not meet it)
  E2  g8 with 8 finders: CA-TBL five-node mean < 0.95 (UWB-MODE ring 0.909, DISSEM ring 0.914)
  E3  g12 with 16 finders: CA-TBL five-node mean >= 0.95 and tag-0 mean >= 0.95
  E4  CA-TBL five-node A(25 m) rises with finders at every gateway count (24 - 8, paired interval above 0)

ANSWER (registered): the deployment requirement is every (gateways, finders) cell whose CA-TBL means are >= 0.95 for
tag 0 AND the five-node pool; the smallest is the one with the fewest gateways, then the fewest finders.

usage: deploy_req_analyse.py <outdir>
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
GW = ["g3", "g8", "g12"]
FI = [8, 12, 16, 24]
NGW = {"g3": 3, "g8": 8, "g12": 12}


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources = [], set()
    cells = sorted(glob.glob(os.path.join(out, "g*_f*", "s*")))
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
        if not 301 <= int(d.rsplit("s", 1)[1]) <= 320:
            problems.append(f"{d}: seed outside 301-320")
        lg = open(os.path.join(d, "run.log"), errors="replace").read()
        gname, f = os.path.basename(os.path.dirname(d)).split("_f")
        nf = len(re.findall(r"finder \d+ -> subject", lg.split("MS-1 tasking (pairs):")[1].split("\n")[0])) \
            if "MS-1 tasking (pairs):" in lg else -1
        if nf != int(f) or "UWB mode 850k:" not in lg or "ekf5 window smoother:" not in lg:
            problems.append(f"{d}: finder count {nf} or preset lines do not match the cell")
        if any(int(x) for x in re.findall(r"ekf5 window smoother: W \d+, solves \d+, failures (\d+)", lg)):
            problems.append(f"{d}: window solver failures")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")
    D = {}
    for g in GW:
        for f in FI:
            for a in ARMS:
                D[(g, f, a)] = {}
                for d in sorted(glob.glob(os.path.join(out, f"{g}_f{f}", "s*"))):
                    if cs.ok(d):
                        r = cell(d, a)
                        if r:
                            D[(g, f, a)][int(d.rsplit("s", 1)[1])] = r
                if len(D[(g, f, a)]) != 20:
                    problems.append(f"{(g, f, a)}: {len(D[(g, f, a)])} seeds, not 20")

    def mean(g, f, a, k):
        x = D[(g, f, a)]
        return float(np.mean([v[k] for v in x.values()])) if x else float("nan")

    def pr(A, B, k):
        a, b = D[A], D[B]
        s = sorted(set(a) & set(b))
        return cs.ci([a[i][k] - b[i][k] for i in s]) if s else (float("nan"),) * 3

    md = ["# DEPLOY-REQ — seeds 301-320 (scripts/deploy_req_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.",
          "", "## CA-TBL, A(25 m) means (tag 0 / five-node), CEP95 tag 0, subject J", "",
          "| gateways | finders | tag 0 | five-node | runs >= 0.95 (five-node) | CEP95 tag 0 (m) | subject J |", "|---|---|---|---|---|---|---|"]
    met = []
    for g in GW:
        for f in FI:
            x = D[(g, f, "ca_tbl_full")]
            if not x:
                continue
            arr = np.array(list(x.values()))
            t0, p5 = arr[:, 0].mean(), arr[:, 1].mean()
            if t0 >= 0.95 and p5 >= 0.95:
                met.append((NGW[g], f, g))
            md.append(f"| {NGW[g]} | {f} | {t0:.3f} | {p5:.3f} | {(arr[:, 1] >= 0.95).mean():.2f} | {np.nanmean(arr[:, 2]):.1f} | "
                      f"{np.nanmean(arr[:, 3]):.2f} |")
    md += ["", "## CA-TBL - comparator, five-node A(25 m) (paired)", "", "| gateways | finders | fixed_threshold | poucet_fusion | static_2p4 |",
           "|---|---|---|---|---|"]
    for g in GW:
        for f in FI:
            T = (g, f, "ca_tbl_full")
            md.append(f"| {NGW[g]} | {f} | " + " | ".join(cs.fmt(*pr(T, (g, f, c), 1)) for c in ("fixed_threshold", "poucet_fusion", "static_2p4")) + " |")
    e1 = mean("g3", 24, "ca_tbl_full", 1)
    e2 = mean("g8", 8, "ca_tbl_full", 1)
    e3 = (mean("g12", 16, "ca_tbl_full", 1), mean("g12", 16, "ca_tbl_full", 0))
    e4 = {g: pr((g, 24, "ca_tbl_full"), (g, 8, "ca_tbl_full"), 1) for g in GW}
    md += ["", "## Registered predictions", "",
           f"- E1 g3 + 24 finders five-node < 0.95: {'HOLDS' if e1 < 0.95 else 'FAILS'} ({e1:.3f})",
           f"- E2 g8 + 8 finders five-node < 0.95: {'HOLDS' if e2 < 0.95 else 'FAILS'} ({e2:.3f})",
           f"- E3 g12 + 16 finders five-node and tag 0 >= 0.95: {'HOLDS' if min(e3) >= 0.95 else 'FAILS'} (five-node {e3[0]:.3f}, tag 0 {e3[1]:.3f})",
           f"- E4 24 - 8 finders raises five-node at every gateway count: {'HOLDS' if all(e4[g][1] > 0 for g in GW) else 'FAILS'} "
           f"({'; '.join(g + ' ' + cs.fmt(*e4[g]) for g in GW)})", ""]
    met.sort()
    md += ["## ANSWER: the deployment requirement", "",
           ("Cells meeting A >= 0.95 for tag 0 and the five-node pool (CA-TBL means): " + ", ".join(f"{a} gateways + {b} finders" for a, b, _ in met)
            + f". **Smallest: {met[0][0]} gateways and {met[0][1]} finders.**") if met else
           "**No cell in the grid meets it.**", ""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "DEPLOY_REQ.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
