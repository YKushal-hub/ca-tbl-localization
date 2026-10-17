#!/usr/bin/env python3
"""SIG868 analysis (report/PREREG_SIG868.md). usage: sig868_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from post_acq_metrics import post_acq  # noqa: E402
from v3_analyse import node_rows  # noqa: E402
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
    for cfg in "ABW":
        for v in ("BASE", "SIG"):
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
                                               J=float(r["totalJ"]), pa=post_acq(d, arm)[2], C=node_rows(d, arm)[1])
    L = ["# SIG868 results (exploratory)", "", "| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | fix share | tag-0 J |", "|---|---|---|---|---|---|---|---|"]
    for cfg in "ABW":
        for v in ("BASE", "SIG"):
            for arm in ARMS:
                rs = [M[k] for k in M if k[:3] == (cfg, v, arm)]
                L.append(f"| {cfg} | {v} | {arm} | {st.mean(r['c95'] for r in rs):.1f} | {np.nanmean([r['pa'] for r in rs]):.1f} | "
                         f"{np.concatenate([np.array(r['C'], dtype=bool) for r in rs]).mean():.3f} | {st.mean(r['fix'] for r in rs):.3f} | {st.mean(r['J'] for r in rs):.2f} |")
    pd = lambda cfg, arm, k: [M[(cfg, "SIG", arm, s)][k] - M[(cfg, "BASE", arm, s)][k] for s in range(1, 21)]
    base = lambda cfg, arm: st.mean(M[(cfg, "BASE", arm, s)]["c95"] for s in range(1, 21))
    cov = lambda cfg, v, arm: np.concatenate([np.array(M[(cfg, v, arm, s)]["C"], dtype=bool) for s in range(1, 21)]).mean()
    s1 = boot(pd("W", "ca_tbl_full", "c95")); s3 = boot(pd("B", "ca_tbl_full", "c95")); s4 = boot(pd("A", "ca_tbl_full", "c95"))
    c0, c1 = cov("A", "BASE", "static_868"), cov("A", "SIG", "static_868")
    v = {1: s1[2] < 0, 2: c1 > c0, 3: s3[0] <= 0.15 * base("B", "ca_tbl_full"), 4: s4[0] <= 0.15 * base("A", "ca_tbl_full")}
    fm = lambda t, p=1: f"{t[0]:+.{p}f} [{t[1]:+.{p}f}, {t[2]:+.{p}f}]"
    L += ["", f"S1 W ca_tbl_full dCEP95 {fm(s1)} m on {base('W', 'ca_tbl_full'):.1f}", f"S2 A static_868 coverage {c0:.3f} -> {c1:.3f}",
          f"S3 B ca_tbl_full dCEP95 {fm(s3)} m on {base('B', 'ca_tbl_full'):.1f}", f"S4 A ca_tbl_full dCEP95 {fm(s4)} m on {base('A', 'ca_tbl_full'):.1f}",
          "", "## Predictions", ""]
    L += [f"- **S{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    L += ["", "Adoption rule (S1, S3, S4 all hold): " + ("ADOPT" if v[1] and v[3] and v[4] else "DO NOT ADOPT")]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems[:20])]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "SIG868.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
