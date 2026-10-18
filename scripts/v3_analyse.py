#!/usr/bin/env python3
"""V3-CONSIDER analysis (report/PREREG_V3_CONSIDER.md). usage: v3_analyse.py <outdir>"""
import csv, filecmp, glob, math, os, random, statistics as st, sys
import numpy as np

ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")


def node_rows(d, arm):
    s = d.rsplit("s", 1)[1]
    fs = [os.path.join(d, f"trace_{arm}_seed{s}.csv")] + sorted(glob.glob(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj*.csv")))
    N, C = [], []
    for f in fs:
        if not os.path.exists(f):
            continue
        seen = False
        for r in csv.DictReader(open(f)):
            if r.get("hadFix") == "1":
                seen = True
            if not seen or float(r["t"]) < 120.0:
                continue
            try:
                pxx, pxy, pyy = float(r["pxx"]), float(r["pxy"]), float(r["pyy"])
                ex, ey = float(r["estX"]) - float(r["trueX"]), float(r["estY"]) - float(r["trueY"])
            except (KeyError, ValueError):
                continue
            det = pxx * pyy - pxy * pxy
            if not (det > 0 and math.isfinite(det)):
                continue
            N.append((pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det)
            lm = 0.5 * (pxx + pyy) + math.sqrt(0.25 * (pxx - pyy) ** 2 + pxy * pxy)
            C.append(math.hypot(ex, ey) <= math.sqrt(5.991 * lm))
    return N, C


def cep95(d, arm):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            return float(r["cep95M"])
    return float("nan")


def boot(x, n=4000):
    random.seed(1)
    m = [st.mean(random.choices(x, k=len(x))) for _ in range(n)]
    m.sort()
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; L = ["# V3-CONSIDER results (exploratory)", ""]; problems = []
    S = {}
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
                N, C, P = [], [], {}
                for d in ds:
                    n, c = node_rows(d, arm); N += n; C += c
                    P[int(d.rsplit("s", 1)[1])] = cep95(d, arm)
                S[(cfg, v, arm)] = (np.array(N), np.array(C, dtype=bool), P)
    L += ["| config | arm | variant | rows | median NEES | R95 coverage | mean CEP95 (m) |", "|---|---|---|---|---|---|---|"]
    for cfg in ("A", "B"):
        for arm in ARMS:
            for v in ("OFF", "ON"):
                N, C, P = S[(cfg, v, arm)]
                L.append(f"| {cfg} | {arm} | {v} | {len(N)} | {np.median(N):.2f} | {C.mean():.3f} | {st.mean(P.values()):.1f} |")
    cov = lambda c, v, a: S[(c, v, a)][1].mean()
    med = lambda c, v, a: float(np.median(S[(c, v, a)][0]))
    on, off = S[("B", "ON", "ca_tbl_full")][2], S[("B", "OFF", "ca_tbl_full")][2]
    dif = [on[s] - off[s] for s in sorted(set(on) & set(off))]
    m, lo, hi = boot(dif)
    v1 = cov("B", "ON", "ca_tbl_full") > cov("B", "OFF", "ca_tbl_full") and 1.0 <= med("B", "ON", "ca_tbl_full") <= 3.0
    v2 = cov("B", "ON", "static_868") - cov("B", "OFF", "static_868") >= 0.08
    v3 = m <= 0.10 * st.mean(off.values())
    v4 = cov("A", "ON", "static_868") > cov("A", "OFF", "static_868")
    v5 = all(abs(cov("B", v, a) - 0.95) > 0.03 for v in ("OFF", "ON") for a in ARMS)
    same = True
    for cfg in ("A", "B"):
        for s in range(1, 21):
            a = os.path.join(out, f"{cfg}_OFF", f"s{s}"); b = os.path.join(out, f"{cfg}_ON", f"s{s}")
            for f in (f"trace_static_2p4_seed{s}.csv",):
                if not (os.path.exists(os.path.join(a, f)) and filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False)):
                    same = False
    L += ["", f"B ca_tbl_full CEP95 ON - OFF: {m:+.2f} [{lo:+.2f}, {hi:+.2f}] m (OFF mean {st.mean(off.values()):.1f} m)", "",
          "## Predictions", "",
          f"- **V3-1**: {'held' if v1 else 'not held'} (coverage {cov('B','OFF','ca_tbl_full'):.3f} -> {cov('B','ON','ca_tbl_full'):.3f}, median NEES {med('B','ON','ca_tbl_full'):.2f})",
          f"- **V3-2**: {'held' if v2 else 'not held'} ({cov('B','OFF','static_868'):.3f} -> {cov('B','ON','static_868'):.3f})",
          f"- **V3-3**: {'held' if v3 else 'not held'}",
          f"- **V3-4**: {'held' if v4 else 'not held'} ({cov('A','OFF','static_868'):.3f} -> {cov('A','ON','static_868'):.3f})",
          f"- **V3-5**: {'held' if v5 else 'not held'}",
          f"- **V3-6**: {'held' if same else 'not held'} (static_2p4 tag-0 trace identical ON/OFF, 40 pairs)"]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems)]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V3.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
