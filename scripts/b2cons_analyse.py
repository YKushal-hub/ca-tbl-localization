#!/usr/bin/env python3
"""B2CONS analysis (report/PREREG_B2CONS.md). usage: b2cons_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from v3_analyse import node_rows  # noqa: E402
from post_acq_metrics import post_acq  # noqa: E402
ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")
VARS = ("BASE", "Q30", "CORR")


def boot(x, n=4000):
    x = [v for v in x if np.isfinite(v)]
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; problems = []; M = {}
    for cfg in "AB":
        for v in VARS:
            ds = sorted(glob.glob(os.path.join(out, f"{cfg}_{v}", "s*")))
            if len(ds) < 20:
                problems.append(f"{cfg}_{v}: {len(ds)} seeds")
            for d in ds:
                if open(os.path.join(d, "rc")).read().strip() != "0":
                    problems.append(f"{d}: rc"); continue
                if "dirty 0" not in open(os.path.join(d, "run_id.txt")).read():
                    problems.append(f"{d}: dirty")
                s = int(d.rsplit("s", 1)[1])
                rows = {r["arm"]: r for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv")))}
                for arm in ARMS:
                    n, c = node_rows(d, arm)
                    M[(cfg, v, arm, s)] = dict(c95=float(rows[arm]["cep95M"]), J=float(rows[arm]["totalJ"]),
                                               pa=post_acq(d, arm)[2], N=n, C=c)
    cov = lambda cfg, v, arm: float(np.mean(np.concatenate([np.array(M[(cfg, v, arm, s)]["C"], dtype=bool) for s in range(1, 21)])))
    nees = lambda cfg, v, arm: float(np.median(np.concatenate([np.array(M[(cfg, v, arm, s)]["N"]) for s in range(1, 21)])))
    mean = lambda cfg, v, arm, k: float(np.nanmean([M[(cfg, v, arm, s)][k] for s in range(1, 21)]))
    pd = lambda cfg, v, arm, k: [M[(cfg, v, arm, s)][k] - M[(cfg, "BASE", arm, s)][k] for s in range(1, 21)]
    L = ["# B2CONS results (exploratory)", "", "| config | variant | arm | coverage | median NEES | CEP95 (m) | CEP95 after 300 s (m) | tag-0 J |", "|---|---|---|---|---|---|---|---|"]
    for cfg in "AB":
        for v in VARS:
            for arm in ARMS:
                L.append(f"| {cfg} | {v} | {arm} | {cov(cfg, v, arm):.3f} | {nees(cfg, v, arm):.2f} | {mean(cfg, v, arm, 'c95'):.1f} | "
                         f"{mean(cfg, v, arm, 'pa'):.1f} | {mean(cfg, v, arm, 'J'):.2f} |")
    fm = lambda t: f"{t[0]:+.1f} [{t[1]:+.1f}, {t[2]:+.1f}]"
    k1 = boot(pd("A", "Q30", "ca_tbl_full", "c95")); k2 = boot(pd("A", "CORR", "static_868", "c95")); k3 = boot(pd("A", "CORR", "ca_tbl_full", "pa"))
    v = {1: cov("A", "Q30", "ca_tbl_full") > cov("A", "BASE", "ca_tbl_full") and k1[0] <= 0,
         2: cov("A", "CORR", "static_868") > cov("A", "BASE", "static_868") and k2[0] < 0,
         3: k3[0] > 0,
         4: cov("B", "Q30", "ca_tbl_full") >= cov("B", "BASE", "ca_tbl_full")}
    L += ["", f"K1 A Q30 ca_tbl_full coverage {cov('A', 'BASE', 'ca_tbl_full'):.3f} -> {cov('A', 'Q30', 'ca_tbl_full'):.3f}; dCEP95 {fm(k1)} m",
          f"K2 A CORR static_868 coverage {cov('A', 'BASE', 'static_868'):.3f} -> {cov('A', 'CORR', 'static_868'):.3f}; dCEP95 {fm(k2)} m",
          f"K3 A CORR ca_tbl_full d(CEP95 after 300 s) {fm(k3)} m",
          f"K4 B Q30 ca_tbl_full coverage {cov('B', 'BASE', 'ca_tbl_full'):.3f} -> {cov('B', 'Q30', 'ca_tbl_full'):.3f}", "", "## Predictions", ""]
    L += [f"- **K{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    L += ["", "## Adoption rule (per variant)", ""]
    for var in ("Q30", "CORR"):
        ok = True; why = []
        for cfg in "AB":
            c0, c1 = cov(cfg, "BASE", "ca_tbl_full"), cov(cfg, var, "ca_tbl_full")
            d = boot(pd(cfg, var, "ca_tbl_full", "c95")); b = mean(cfg, "BASE", "ca_tbl_full", "c95")
            ok = ok and c1 > c0 and d[0] <= 0.10 * b
            why.append(f"{cfg}: coverage {c0:.3f} -> {c1:.3f}, dCEP95 {fm(d)} on {b:.1f}")
        L.append(f"- **{var}**: {'ADOPT' if ok else 'DO NOT ADOPT'} ({'; '.join(why)})")
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems[:20])]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "B2CONS.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
