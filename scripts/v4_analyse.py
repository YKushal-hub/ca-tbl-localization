#!/usr/bin/env python3
"""V4-ACQUIRE analysis (report/PREREG_V4_ACQUIRE.md). usage: v4_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from v3_analyse import node_rows, cep95  # noqa: E402

ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")


def med_err(d, arm):
    s = d.rsplit("s", 1)[1]
    e = [float(r["errM"]) for r in csv.DictReader(open(os.path.join(d, f"trace_{arm}_seed{s}.csv")))
         if r["hadFix"] == "1" and float(r["t"]) >= 120.0]
    return float(np.median(e)) if e else float("nan")


def boot(x, n=4000):
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; L = ["# V4-ACQUIRE results (exploratory)", ""]; problems = []; S = {}
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
                N, C, P, E = [], [], {}, {}
                for d in ds:
                    n, c = node_rows(d, arm); N += n; C += c
                    s = int(d.rsplit("s", 1)[1]); P[s] = cep95(d, arm); E[s] = med_err(d, arm)
                S[(cfg, v, arm)] = (np.array(N), np.array(C, dtype=bool), P, E)
    L += ["| config | arm | variant | median NEES | R95 coverage | mean CEP95 (m) | mean of seed median errors (m) |", "|---|---|---|---|---|---|---|"]
    for cfg in ("A", "B"):
        for arm in ARMS:
            for v in ("OFF", "ON"):
                N, C, P, E = S[(cfg, v, arm)]
                L.append(f"| {cfg} | {arm} | {v} | {np.median(N):.2f} | {C.mean():.3f} | {st.mean(P.values()):.1f} | {st.mean(E.values()):.1f} |")
    def pd(cfg, arm, k):
        a, b = S[(cfg, "ON", arm)][k], S[(cfg, "OFF", arm)][k]
        return [a[s] - b[s] for s in sorted(set(a) & set(b))]
    cov = lambda c, v, a: S[(c, v, a)][1].mean(); med = lambda c, v, a: float(np.median(S[(c, v, a)][0]))
    d1 = boot(pd("A", "ca_tbl_full", 2)); d2 = boot(pd("A", "static_2p4", 3)); d4 = boot(pd("A", "static_868", 2)); d6 = boot(pd("B", "ca_tbl_full", 2))
    offB = st.mean(S[("B", "OFF", "ca_tbl_full")][2].values())
    v = {1: d1[2] < 0, 2: d2[0] < 0, 3: cov("A", "ON", "ca_tbl_full") > cov("A", "OFF", "ca_tbl_full") and med("A", "ON", "ca_tbl_full") < med("A", "OFF", "ca_tbl_full"),
         4: d4[0] < 0, 5: cov("B", "ON", "static_2p4") >= 0.90, 6: d6[0] <= 0.30 * offB}
    L += ["", f"A ca_tbl_full CEP95 ON-OFF {d1[0]:+.1f} [{d1[1]:+.1f}, {d1[2]:+.1f}] m",
          f"A static_2p4 median error ON-OFF {d2[0]:+.1f} [{d2[1]:+.1f}, {d2[2]:+.1f}] m",
          f"A static_868 CEP95 ON-OFF {d4[0]:+.1f} [{d4[1]:+.1f}, {d4[2]:+.1f}] m",
          f"B ca_tbl_full CEP95 ON-OFF {d6[0]:+.1f} [{d6[1]:+.1f}, {d6[2]:+.1f}] m (OFF {offB:.1f} m)", "", "## Predictions", ""]
    L += [f"- **V4-{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems)]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V4.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
