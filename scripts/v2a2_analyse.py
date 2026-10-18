#!/usr/bin/env python3
"""V2A2-NLOS3GPP analysis (report/PREREG_V2A2_NLOS3GPP.md). usage: v2a2_analyse.py <outdir>"""
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
    out = sys.argv[1]; L = ["# V2A2-NLOS3GPP results (exploratory)", ""]; problems = []; M = {}; NL = {}
    for cfg in ("A", "B", "R8"):
        for v in ("OFF", "ON"):
            ds = sorted(glob.glob(os.path.join(out, f"{cfg}_{v}", "s*")))
            if len(ds) < 20:
                problems.append(f"{cfg}_{v}: {len(ds)} seeds")
            nn = [0, 0]
            for d in ds:
                if open(os.path.join(d, "rc")).read().strip() != "0":
                    problems.append(f"{d}: rc"); continue
                if "dirty 0" not in open(os.path.join(d, "run_id.txt")).read():
                    problems.append(f"{d}: dirty")
                s = int(d.rsplit("s", 1)[1])
                for arm in ARMS:
                    r = row(d, arm); n, c = node_rows(d, arm)
                    M[(cfg, v, arm, s)] = dict(c95=float(r["cep95M"]), fix=float(r["fixCycles"]) / float(r["steps"]), J=float(r["totalJ"]), N=n, C=c)
                for x in csv.DictReader(open(os.path.join(d, f"ranges_ca_tbl_full_seed{s}.csv"))):
                    if x["sourceKind"] == "1":
                        nn[0] += 1; nn[1] += x["nlosTruth"] == "1"
            NL[(cfg, v)] = (nn[1] / nn[0] if nn[0] else float("nan"), nn[0])
    L += ["| config | arm | variant | CEP95 (m) | fix share | coverage | tag-0 J |", "|---|---|---|---|---|---|---|"]
    for cfg in ("A", "B", "R8"):
        for arm in ARMS:
            for v in ("OFF", "ON"):
                rs = [M[k] for k in M if k[:3] == (cfg, v, arm)]
                C = np.concatenate([np.array(r["C"], dtype=bool) for r in rs])
                L.append(f"| {cfg} | {arm} | {v} | {st.mean(r['c95'] for r in rs):.1f} | {st.mean(r['fix'] for r in rs):.3f} | {C.mean():.3f} | {st.mean(r['J'] for r in rs):.2f} |")
    pd = lambda cfg, arm, k: [M[(cfg, "ON", arm, s)][k] - M[(cfg, "OFF", arm, s)][k] for s in range(1, 21)]
    t2 = boot(pd("A", "static_868", "c95")); t3 = boot(pd("R8", "ca_tbl_full", "c95")); t4 = boot(pd("A", "ca_tbl_full", "fix")); t5 = boot(pd("B", "static_2p4", "c95"))
    offB = st.mean(M[("B", "OFF", "static_2p4", s)]["c95"] for s in range(1, 21))
    L += ["", "NLOS share of tag 0's gateway ranges (ca_tbl_full arm): " + "; ".join(f"{c} {v} {NL[(c, v)][0]:.3f} (n {NL[(c, v)][1]})" for c in ("A", "B", "R8") for v in ("OFF", "ON")),
          f"A static_868 CEP95 ON-OFF {t2[0]:+.1f} [{t2[1]:+.1f}, {t2[2]:+.1f}] m", f"R8 ca_tbl_full CEP95 ON-OFF {t3[0]:+.1f} [{t3[1]:+.1f}, {t3[2]:+.1f}] m",
          f"A ca_tbl_full fix share ON-OFF {t4[0]:+.3f} [{t4[1]:+.3f}, {t4[2]:+.3f}]", f"B static_2p4 CEP95 ON-OFF {t5[0]:+.1f} [{t5[1]:+.1f}, {t5[2]:+.1f}] m (OFF {offB:.1f})"]
    v = {1: NL[("A", "ON")][0] > NL[("A", "OFF")][0] + 0.2, 2: t2[1] > 0, 3: t3[0] > 0, 4: t4[0] < 0, 5: abs(t5[0]) <= 0.25 * offB}
    L += ["", "## Predictions", ""] + [f"- **N{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems)]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V2A2.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
