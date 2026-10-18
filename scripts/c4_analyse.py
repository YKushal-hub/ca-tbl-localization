#!/usr/bin/env python3
"""C4-P833 analysis (report/PREREG_C4_P833.md). usage: c4_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from post_acq_metrics import post_acq  # noqa: E402
ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")


def row(d, arm):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            return r


def boot(x, n=4000):
    x = [v for v in x if np.isfinite(v)]
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; problems = []; M = {}
    for cfg in "AB":
        for v in ("BASE", "P833"):
            ds = sorted(glob.glob(os.path.join(out, f"{cfg}_{v}", "s*")))
            if len(ds) < 20:
                problems.append(f"{cfg}_{v}: {len(ds)} seeds")
            for d in ds:
                if open(os.path.join(d, "rc")).read().strip() != "0":
                    problems.append(f"{d}: rc"); continue
                if "dirty 0" not in open(os.path.join(d, "run_id.txt")).read():
                    problems.append(f"{d}: dirty")
                s = int(d.rsplit("s", 1)[1])
                for arm in ARMS:
                    r = row(d, arm)
                    M[(cfg, v, arm, s)] = dict(c95=float(r["cep95M"]), fix=float(r["fixCycles"]) / float(r["steps"]),
                                               J=float(r["totalJ"]), pa=post_acq(d, arm)[2])
    L = ["# C4-P833 results (exploratory)", "", "| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | fix share | tag-0 J |", "|---|---|---|---|---|---|---|"]
    for cfg in "AB":
        for v in ("BASE", "P833"):
            for arm in ARMS:
                rs = [M[k] for k in M if k[:3] == (cfg, v, arm)]
                L.append(f"| {cfg} | {v} | {arm} | {st.mean(r['c95'] for r in rs):.1f} | {np.nanmean([r['pa'] for r in rs]):.1f} | "
                         f"{st.mean(r['fix'] for r in rs):.3f} | {st.mean(r['J'] for r in rs):.2f} |")
    pd = lambda cfg, arm, k: [M[(cfg, "P833", arm, s)][k] - M[(cfg, "BASE", arm, s)][k] for s in range(1, 21)]
    f1 = boot(pd("B", "static_2p4", "c95")); f2 = boot(pd("B", "static_2p4", "fix")); f3 = boot(pd("A", "static_868", "c95"))
    f4 = {c: boot(pd(c, "ca_tbl_full", "c95")) for c in "AB"}
    base = {c: st.mean(M[(c, "BASE", "ca_tbl_full", s)]["c95"] for s in range(1, 21)) for c in "AB"}
    v = {1: f1[0] > 0, 2: f2[0] < 0, 3: f3[0] <= 0, 4: all(abs(f4[c][0]) < 0.15 * base[c] for c in "AB")}
    fm = lambda t, p=1: f"{t[0]:+.{p}f} [{t[1]:+.{p}f}, {t[2]:+.{p}f}]"
    L += ["", f"F1 B static_2p4 dCEP95 {fm(f1)} m", f"F2 B static_2p4 dfix {fm(f2, 3)}", f"F3 A static_868 dCEP95 {fm(f3)} m",
          f"F4 ca_tbl_full dCEP95 A {fm(f4['A'])} on {base['A']:.1f}, B {fm(f4['B'])} on {base['B']:.1f} m", "", "## Predictions", ""]
    L += [f"- **F{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems[:20])]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "C4.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
