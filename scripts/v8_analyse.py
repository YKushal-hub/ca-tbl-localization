#!/usr/bin/env python3
"""V8-REGEN analysis (report/PREREG_V8_REGEN.md). usage: v8_analyse.py <outdir>"""
import csv, glob, os, random, statistics as st, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from v3_analyse import node_rows  # noqa: E402

ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")
CELLS = [(c, v) for c in ("A", "B", "R8", "BS") for v in ("BASE", "FULL", "NO_REPLY", "NO_PF", "NO_NLOS")] + \
        [(c, v) for c in ("WA", "WB") for v in ("BASE", "FULL")]


def row(d, arm):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            return r


def cep95_post(d, arm):
    s = d.rsplit("s", 1)[1]
    e = [float(r["errM"]) for r in csv.DictReader(open(os.path.join(d, f"trace_{arm}_seed{s}.csv")))
         if r["hadFix"] == "1" and float(r["t"]) >= 300.0]
    return float(np.percentile(e, 95)) if e else float("nan")


def boot(x, n=4000):
    x = [v for v in x if np.isfinite(v)]
    if not x:
        return float("nan"), float("nan"), float("nan")
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; L = ["# V8-REGEN results (exploratory)", ""]; problems = []; M = {}
    for cfg, v in CELLS:
        ds = sorted(glob.glob(os.path.join(out, f"{cfg}_{v}", "s*")))
        if len(ds) < 20:
            problems.append(f"{cfg}_{v}: {len(ds)} seeds")
        for d in ds:
            if not os.path.exists(os.path.join(d, "rc")) or open(os.path.join(d, "rc")).read().strip() != "0":
                problems.append(f"{d}: rc"); continue
            if "dirty 0" not in open(os.path.join(d, "run_id.txt")).read():
                problems.append(f"{d}: dirty")
            s = int(d.rsplit("s", 1)[1])
            for arm in ARMS:
                r = row(d, arm); n, c = node_rows(d, arm)
                M[(cfg, v, arm, s)] = dict(c95=float(r["cep95M"]), c95p=cep95_post(d, arm), J=float(r["totalJ"]),
                                           fix=float(r["fixCycles"]) / float(r["steps"]), N=n, C=c)
    L += ["| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | median NEES | fix share | tag-0 J |",
          "|---|---|---|---|---|---|---|---|---|"]
    def agg(cfg, v, arm):
        rs = [M[k] for k in M if k[:3] == (cfg, v, arm)]
        N = np.concatenate([np.array(r["N"]) for r in rs]) if rs else np.array([np.nan])
        C = np.concatenate([np.array(r["C"], dtype=bool) for r in rs]) if rs else np.array([False])
        f = lambda k: float(np.nanmean([r[k] for r in rs])) if rs else float("nan")
        return f("c95"), f("c95p"), float(C.mean()), float(np.nanmedian(N)), f("fix"), f("J")
    A = {}
    for cfg, v in CELLS:
        for arm in ARMS:
            a = agg(cfg, v, arm); A[(cfg, v, arm)] = a
            L.append(f"| {cfg} | {v} | {arm} | {a[0]:.1f} | {a[1]:.1f} | {a[2]:.3f} | {a[3]:.2f} | {a[4]:.3f} | {a[5]:.2f} |")
    def pd(cfg, v1, v0, arm, k):
        return [M[(cfg, v1, arm, s)][k] - M[(cfg, v0, arm, s)][k] for s in range(1, 21) if (cfg, v1, arm, s) in M and (cfg, v0, arm, s) in M]
    T = {}
    T[1] = boot(pd("A", "FULL", "BASE", "ca_tbl_full", "c95")); T[2] = boot(pd("A", "NO_REPLY", "FULL", "ca_tbl_full", "c95"))
    T[4] = boot(pd("R8", "FULL", "BASE", "ca_tbl_full", "c95")); T[6] = boot(pd("B", "FULL", "BASE", "ca_tbl_full", "c95p"))
    T[7] = boot(pd("WA", "FULL", "BASE", "ca_tbl_full", "c95")); T[9] = boot(pd("A", "FULL", "BASE", "static_2p4", "c95"))
    nl = {cfg: boot(pd(cfg, "NO_NLOS", "FULL", "ca_tbl_full", "c95")) for cfg in ("A", "B", "R8", "BS")}
    ok8 = all(abs(nl[c][0]) < 0.10 * A[(c, "FULL", "ca_tbl_full")][0] for c in nl)
    v = {1: T[1][2] < 0, 2: T[2][1] > 0, 3: A[("A", "FULL", "ca_tbl_full")][2] > A[("A", "NO_PF", "ca_tbl_full")][2],
         4: T[4][0] < 0, 5: A[("BS", "FULL", "ca_tbl_full")][2] >= A[("BS", "BASE", "ca_tbl_full")][2],
         6: T[6][0] <= 0, 7: T[7][0] < 0, 8: ok8, 9: T[9][0] < 0}
    L += [""]
    for k, lab in ((1, "A CEP95 FULL-BASE"), (2, "A CEP95 NO_REPLY-FULL"), (4, "R8 CEP95 FULL-BASE"), (6, "B CEP95-after-300s FULL-BASE"),
                   (7, "wolf A CEP95 FULL-BASE"), (9, "A static_2p4 CEP95 FULL-BASE")):
        L.append(f"- {lab}: {T[k][0]:+.1f} [{T[k][1]:+.1f}, {T[k][2]:+.1f}] m")
    for c in nl:
        L.append(f"- {c} CEP95 NO_NLOS-FULL: {nl[c][0]:+.1f} [{nl[c][1]:+.1f}, {nl[c][2]:+.1f}] m")
    # every ablation, for the record
    L += ["", "### Every ablation against FULL (ca_tbl_full CEP95, paired mean [95 %])", ""]
    for c in ("A", "B", "R8", "BS"):
        for ab in ("BASE", "NO_REPLY", "NO_PF", "NO_NLOS"):
            b = boot(pd(c, ab, "FULL", "ca_tbl_full", "c95")); bp = boot(pd(c, ab, "FULL", "ca_tbl_full", "c95p"))
            L.append(f"- {c} {ab} - FULL: CEP95 {b[0]:+.1f} [{b[1]:+.1f}, {b[2]:+.1f}]; after 300 s {bp[0]:+.1f} [{bp[1]:+.1f}, {bp[2]:+.1f}]")
    L += ["", "## Predictions", ""] + [f"- **V8-{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems[:20]) + (f" (+{len(problems)-20} more)" if len(problems) > 20 else "")]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V8.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
