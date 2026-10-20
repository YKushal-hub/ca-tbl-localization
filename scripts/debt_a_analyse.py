#!/usr/bin/env python3
"""DEBT-A analysis (report/PREREG_DEBT_A.md). usage: debt_a_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from v3_analyse import node_rows  # noqa: E402
from post_acq_metrics import post_acq  # noqa: E402
ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")
VARS = ("BASE", "NLOSDET", "FOLB", "INTERF", "RXPOW", "SUBNOTDOA", "EDGEOFF", "ARBOFF")


def boot(x, n=4000):
    x = [v for v in x if np.isfinite(v)]
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def subj_w25(d, arm):
    s = d.rsplit("s", 1)[1]
    v = []
    for f in glob.glob(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj*.csv")):
        v += [float(r["errM"]) <= 25.0 for r in csv.DictReader(open(f)) if float(r["t"]) >= 120.0]
    return float(np.mean(v)) if v else float("nan")


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
                    r = rows[arm]; n, c = node_rows(d, arm)
                    M[(cfg, v, arm, s)] = dict(c95=float(r["cep95M"]), J=float(r["totalJ"]), pa=post_acq(d, arm)[2],
                                               fix=float(r["fixCycles"]) / float(r["steps"]),
                                               uwb=float(r["cycles_uwb"]) / float(r["steps"]), C=c,
                                               w25=subj_w25(d, arm) if arm == "ca_tbl_full" else float("nan"))
    S = lambda cfg, v, arm, k: [M[(cfg, v, arm, s)][k] for s in range(1, 21)]
    cov = lambda cfg, v, arm: float(np.mean(np.concatenate([np.array(M[(cfg, v, arm, s)]["C"], dtype=bool) for s in range(1, 21)])))
    pd = lambda cfg, v, arm, k: [a - b for a, b in zip(S(cfg, v, arm, k), S(cfg, "BASE", arm, k))]
    L = ["# DEBT-A results (exploratory)", "", "| config | variant | arm | CEP95 | CEP95 after 300 s | coverage | fix share | UWB share | tag-0 J | subj within 25 m |",
         "|---|---|---|---|---|---|---|---|---|---|"]
    for cfg in "AB":
        for v in VARS:
            for arm in ARMS:
                L.append(f"| {cfg} | {v} | {arm} | {np.mean(S(cfg, v, arm, 'c95')):.1f} | {np.nanmean(S(cfg, v, arm, 'pa')):.1f} | "
                         f"{cov(cfg, v, arm):.3f} | {np.mean(S(cfg, v, arm, 'fix')):.3f} | {np.mean(S(cfg, v, arm, 'uwb')):.3f} | "
                         f"{np.mean(S(cfg, v, arm, 'J')):.2f} | {np.nanmean(S(cfg, v, arm, 'w25')):.3f} |")
    fm = lambda t, p=1: f"{t[0]:+.{p}f} [{t[1]:+.{p}f}, {t[2]:+.{p}f}]"
    base = lambda cfg, arm: np.mean(S(cfg, "BASE", arm, "c95"))
    d1 = {c: boot(pd(c, "NLOSDET", "ca_tbl_full", "c95")) for c in "AB"}
    d2 = boot(pd("B", "FOLB", "ca_tbl_full", "uwb")); d3 = boot(pd("A", "INTERF", "ca_tbl_full", "fix"))
    d4 = boot(pd("A", "RXPOW", "static_868", "c95")); d5 = boot(pd("A", "SUBNOTDOA", "ca_tbl_full", "w25"))
    jr = np.mean([a / b for a, b in zip(S("A", "EDGEOFF", "static_868", "J"), S("A", "BASE", "static_868", "J"))])
    d7 = boot(pd("B", "ARBOFF", "ca_tbl_full", "c95"))
    v = {1: all(abs(d1[c][0]) <= 0.1 * base(c, "ca_tbl_full") for c in "AB"), 2: d2[0] < 0, 3: d3[0] > 0, 4: d4[0] > 0,
         5: d5[0] > 0, 6: jr >= 1.25, 7: abs(d7[0]) <= 0.1 * base("B", "ca_tbl_full")}
    L += ["", f"D1 NLOSDET ca_tbl_full dCEP95 A {fm(d1['A'])} on {base('A', 'ca_tbl_full'):.1f}, B {fm(d1['B'])} on {base('B', 'ca_tbl_full'):.1f}",
          f"D2 FOLB B ca_tbl_full dUWB share {fm(d2, 4)}", f"D3 INTERF A ca_tbl_full dfix share {fm(d3, 4)}",
          f"D4 RXPOW A static_868 dCEP95 {fm(d4)} on {base('A', 'static_868'):.1f}",
          f"D5 SUBNOTDOA A subjects within 25 m {fm(d5, 4)}", f"D6 EDGEOFF A static_868 energy ratio {jr:.3f}",
          f"D7 ARBOFF B ca_tbl_full dCEP95 {fm(d7)} on {base('B', 'ca_tbl_full'):.1f}", "", "## Predictions", ""]
    L += [f"- **D{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems[:20])]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "DEBT_A.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
