#!/usr/bin/env python3
"""UPG-SCOPE analysis (report/PREREG_UPG_SCOPE.md). usage: upg_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from v3_analyse import node_rows  # noqa: E402
from post_acq_metrics import post_acq  # noqa: E402
ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")
VARS = ("BASE", "C1", "C2", "C3", "B1", "B7", "WBASE", "WC5")


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
    out = sys.argv[1]; L = ["# UPG-SCOPE results (exploratory)", ""]; problems = []; M = {}
    for cfg in ("A", "B"):
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
                for arm in ARMS:
                    r = row(d, arm); n, c = node_rows(d, arm)
                    M[(cfg, v, arm, s)] = dict(c95=float(r["cep95M"]), J=float(r["totalJ"]), fix=float(r["fixCycles"]) / float(r["steps"]),
                                               c95p=post_acq(d, arm)[2], N=n, C=c)
    L += ["| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | fix share | tag-0 J |", "|---|---|---|---|---|---|---|---|"]
    A = {}
    for cfg in ("A", "B"):
        for v in VARS:
            for arm in ARMS:
                rs = [M[k] for k in M if k[:3] == (cfg, v, arm)]
                C = np.concatenate([np.array(r["C"], dtype=bool) for r in rs])
                a = (st.mean(r["c95"] for r in rs), float(np.nanmean([r["c95p"] for r in rs])), float(C.mean()), st.mean(r["fix"] for r in rs), st.mean(r["J"] for r in rs))
                A[(cfg, v, arm)] = a
                L.append(f"| {cfg} | {v} | {arm} | {a[0]:.1f} | {a[1]:.1f} | {a[2]:.3f} | {a[3]:.3f} | {a[4]:.2f} |")
    pd = lambda cfg, v1, v0, arm, k: [M[(cfg, v1, arm, s)][k] - M[(cfg, v0, arm, s)][k] for s in range(1, 21)]
    u1 = boot(pd("A", "C1", "BASE", "ca_tbl_full", "c95")); u2 = boot(pd("A", "C2", "BASE", "static_868", "c95"))
    jr = st.mean(M[("B", "C3", "ca_tbl_full", s)]["J"] / M[("B", "BASE", "ca_tbl_full", s)]["J"] for s in range(1, 21))
    u3c = boot(pd("B", "C3", "BASE", "ca_tbl_full", "c95")); bB = A[("B", "BASE", "ca_tbl_full")][0]
    u5 = {c: boot(pd(c, "B7", "BASE", "ca_tbl_full", "c95")) for c in ("A", "B")}
    u6 = boot(pd("B", "WC5", "WBASE", "static_2p4", "c95"))
    v = {1: u1[0] <= 0 and A[("A", "C1", "ca_tbl_full")][2] >= A[("A", "BASE", "ca_tbl_full")][2],
         2: u2[0] < 0, 3: jr <= 0.95 and u3c[0] <= 0.2 * bB,
         4: A[("A", "B1", "ca_tbl_full")][2] > A[("A", "BASE", "ca_tbl_full")][2],
         5: all(abs(u5[c][0]) < 0.1 * A[(c, "BASE", "ca_tbl_full")][0] for c in u5), 6: u6[2] < 0}
    L += ["", f"U1 A C1-BASE ca_tbl_full CEP95 {u1[0]:+.1f} [{u1[1]:+.1f}, {u1[2]:+.1f}] m",
          f"U2 A C2-BASE static_868 CEP95 {u2[0]:+.1f} [{u2[1]:+.1f}, {u2[2]:+.1f}] m",
          f"U3 B C3/BASE ca_tbl_full energy {jr:.3f}; CEP95 {u3c[0]:+.1f} [{u3c[1]:+.1f}, {u3c[2]:+.1f}] m on {bB:.1f}",
          f"U5 B7-BASE ca_tbl_full CEP95 A {u5['A'][0]:+.1f} [{u5['A'][1]:+.1f}, {u5['A'][2]:+.1f}], B {u5['B'][0]:+.1f} [{u5['B'][1]:+.1f}, {u5['B'][2]:+.1f}] m",
          f"U6 B WC5-WBASE static_2p4 CEP95 {u6[0]:+.1f} [{u6[1]:+.1f}, {u6[2]:+.1f}] m", "", "## Predictions", ""]
    L += [f"- **U{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems[:20])]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "UPG.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
