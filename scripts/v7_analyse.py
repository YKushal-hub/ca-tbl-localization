#!/usr/bin/env python3
"""V7-REPLYPOS analysis (report/PREREG_V7_REPLYPOS.md). usage: v7_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from v3_analyse import node_rows  # noqa: E402

ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")


def row(d, arm):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            return r


def boot(x, n=4000):
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; L = ["# V7-REPLYPOS results (exploratory)", ""]; problems = []; S = {}
    for cfg in ("A", "B"):
        for v in ("OFF", "ON"):
            ds = sorted(glob.glob(os.path.join(out, f"{cfg}_{v}", "s*")))
            if len(ds) < 20:
                problems.append(f"{cfg}_{v}: {len(ds)} seeds")
            for d in ds:
                if open(os.path.join(d, "rc")).read().strip() != "0":
                    problems.append(f"{d}: rc")
                if "dirty 0" not in open(os.path.join(d, "run_id.txt")).read():
                    problems.append(f"{d}: dirty")
            for arm in ARMS:
                N, C, P, J, F = [], [], {}, {}, {}
                for d in ds:
                    n, c = node_rows(d, arm); N += n; C += c
                    s = int(d.rsplit("s", 1)[1]); r = row(d, arm)
                    P[s] = float(r["cep95M"]); J[s] = float(r["totalJ"]); F[s] = float(r["fixCycles"]) / float(r["steps"])
                S[(cfg, v, arm)] = (np.array(N), np.array(C, dtype=bool), P, J, F)
    L += ["| config | arm | variant | median NEES | coverage | CEP95 (m) | tag-0 J | fix share |", "|---|---|---|---|---|---|---|---|"]
    for cfg in ("A", "B"):
        for arm in ARMS:
            for v in ("OFF", "ON"):
                N, C, P, J, F = S[(cfg, v, arm)]
                L.append(f"| {cfg} | {arm} | {v} | {np.median(N):.2f} | {C.mean():.3f} | {st.mean(P.values()):.1f} | {st.mean(J.values()):.2f} | {st.mean(F.values()):.3f} |")
    def pd(cfg, arm, k):
        a, b = S[(cfg, "ON", arm)][k], S[(cfg, "OFF", arm)][k]
        return [a[s] - b[s] for s in sorted(set(a) & set(b))]
    d1 = boot(pd("A", "ca_tbl_full", 2)); d2 = boot(pd("A", "static_868", 2)); d3 = boot(pd("A", "static_2p4", 2))
    d4 = boot(pd("B", "static_2p4", 2)); d5 = boot(pd("B", "ca_tbl_full", 2))
    offB = st.mean(S[("B", "OFF", "ca_tbl_full")][2].values())
    jr = [S[("B", "ON", "ca_tbl_full")][3][s] / S[("B", "OFF", "ca_tbl_full")][3][s] for s in range(1, 21)]
    v = {1: d1[2] < 0, 2: d2[2] < 0, 3: d3[0] < 0, 4: d4[0] < 0, 5: abs(d5[0]) <= 0.25 * offB, 6: st.mean(jr) >= 1.10}
    for name, dd in (("A ca_tbl_full", d1), ("A static_868", d2), ("A static_2p4", d3), ("B static_2p4", d4), ("B ca_tbl_full", d5)):
        L.append(f"\n{name} CEP95 ON-OFF {dd[0]:+.1f} [{dd[1]:+.1f}, {dd[2]:+.1f}] m")
    L += [f"\nB ca_tbl_full tag-0 energy ON/OFF {st.mean(jr):.3f}", "", "## Predictions", ""]
    L += [f"- **V7-{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems)]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V7.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
