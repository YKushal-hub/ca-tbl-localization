#!/usr/bin/env python3
"""COLLAPSE-1 analysis (report/PREREG_COLLAPSE1.md). usage: collapse1_analyse.py <campaign outdir>

Writes <outdir>/collapse1_cells.csv (one row per cell, arm and seed) and prints the hypothesis table.
Committed with the registration, before any cell.
"""
import csv
import glob
import math
import os
import random
import statistics as st
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "collapse"))
import score_dig as SD  # noqa: E402

ARMS = ("ca_tbl_full", "static_868", "static_2p4")


def run_measures(d, arm):
    rows = [r for r in csv.DictReader(open(os.path.join(d, "subject_summary.csv"))) if r["arm"] == arm] \
        if os.path.exists(os.path.join(d, "subject_summary.csv")) else []
    if not rows:  # N = 1: the arm summary is subject 1
        rows = [dict(r, subjectId="1") for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv")))
                if r["arm"] == arm]
    cep = [float(r["cep50M"]) for r in rows]
    n = len(cep)
    m = dict(n=n, L2=sum(c <= 2.0 for c in cep) / n, L5=sum(c <= 5.0 for c in cep) / n,
             cep_med=st.median(cep), cep_max=max(cep), J_med=st.median(float(r["totalJ"]) for r in rows),
             w1_cep=next(float(r["cep50M"]) for r in rows if r["subjectId"] == "1"))
    # first time each worker is within 2 m
    firsts = []
    files = glob.glob(os.path.join(d, f"trace_{arm}_seed*.csv")) + glob.glob(os.path.join(d, f"subjtrace_{arm}_seed*_subj*.csv"))
    for f in files:
        t = next((float(r["t"]) for r in csv.DictReader(open(f)) if float(r["errM"]) <= 2.0), float("nan"))
        firsts.append(t)
    ok = [t for t in firsts if not math.isnan(t)]
    m["first2_med"] = st.median(ok) if ok else float("nan")
    # digging: 2 teams, t_dig 2 h, radius 2 m
    W = SD.load(d, arm, "END")
    if W:
        start_h = max(w["t"] for w in W.values()) / 3600.0
        exp_t = {k: 2.0 * (1 + (max(w["sigma"], 1e-6) ** 2 / 4.0) * math.exp(-4.0 / max(w["sigma"], 1e-6) ** 2))
                 for k, w in W.items()}
        m["dig_spt"] = SD.schedule(sorted(W, key=lambda k: exp_t[k]), W, 2, 2.0, 2.0, start_h)[0]
        m["dig_id"] = SD.schedule(sorted(W), W, 2, 2.0, 2.0, start_h)[0]
    return m


def boot_lo(diffs, n=4000, seed=1):
    g = random.Random(seed)
    ms = sorted(st.mean(g.choices(diffs, k=len(diffs))) for _ in range(n))
    return ms[int(0.05 * n)]


def main():
    out = sys.argv[1]
    recs = []
    for cell in sorted(glob.glob(os.path.join(out, "s*_*_n*"))):
        name = os.path.basename(cell)
        stv, br, nn = name.split("_")
        for sd in sorted(glob.glob(os.path.join(cell, "seed*"))):
            if not (os.path.exists(os.path.join(sd, "rc")) and open(os.path.join(sd, "rc")).read().strip() == "0"):
                continue
            for arm in ARMS:
                try:
                    m = run_measures(sd, arm)
                except (StopIteration, FileNotFoundError, ValueError):
                    continue
                recs.append(dict(storeys=int(stv[1:]), bracket=br, N=int(nn[1:]), seed=int(sd.rsplit("seed", 1)[1]),
                                 arm=arm, **m))
    if not recs:
        raise SystemExit("no finished cells")
    keys = list(recs[0].keys())
    with open(os.path.join(out, "collapse1_cells.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=keys)
        w.writeheader()
        for r in recs:
            w.writerow({k: r.get(k, "") for k in keys})

    def sel(**kw):
        return [r for r in recs if all(r[k] == v for k, v in kw.items())]

    def by_seed(rows):
        return {r["seed"]: r for r in rows}

    print("cell                     arm          seeds   L2    L5   cepMed  cepMax  first2  J/worker  dig_spt dig_id")
    for stv in (2, 5, 8):
        for br in ("low", "central", "high"):
            for N in (1, 20, 30):
                for arm in ARMS:
                    rs = sel(storeys=stv, bracket=br, N=N, arm=arm)
                    if not rs:
                        continue
                    f = lambda k: st.mean(r[k] for r in rs if k in r and not (isinstance(r[k], float) and math.isnan(r[k]))) \
                        if any(k in r for r in rs) else float("nan")
                    print(f"s{stv} {br:7} N={N:2}  {arm:12} {len(rs):5} {f('L2'):5.2f} {f('L5'):5.2f} {f('cep_med'):7.1f} "
                          f"{f('cep_max'):7.1f} {f('first2_med'):7.0f} {f('J_med'):8.1f} {f('dig_spt'):7.2f} {f('dig_id'):6.2f}")
    print("\nHYPOTHESES")
    res = []
    # H1
    for stv in (2, 5, 8):
        for br in ("low", "central"):
            rs = sel(storeys=stv, bracket=br, N=20, arm="ca_tbl_full")
            if rs:
                v = st.mean(r["L2"] for r in rs)
                res.append(("H1", f"s{stv} {br}", v >= 0.80, f"mean L2 {v:.2f} (n={len(rs)})"))
    # H2
    for stv in (2, 5, 8):
        for br in ("low", "central", "high"):
            a = by_seed(sel(storeys=stv, bracket=br, N=20, arm="ca_tbl_full"))
            b = by_seed(sel(storeys=stv, bracket=br, N=1, arm="ca_tbl_full"))
            d = [a[s]["w1_cep"] - b[s]["w1_cep"] for s in a if s in b]
            if d:
                res.append(("H2", f"s{stv} {br}", st.median(d) <= 1.0, f"median paired diff {st.median(d):+.2f} m (n={len(d)})"))
    # H3
    for stv in (2, 5, 8):
        for br in ("low", "central", "high"):
            a = by_seed(sel(storeys=stv, bracket=br, N=20, arm="ca_tbl_full"))
            for other in ("static_868", "static_2p4"):
                b = by_seed(sel(storeys=stv, bracket=br, N=20, arm=other))
                d = [a[s]["L2"] - b[s]["L2"] for s in a if s in b]
                if len(d) >= 3:
                    lo = boot_lo(d)
                    res.append(("H3", f"s{stv} {br} vs {other}", lo > -0.05, f"mean diff {st.mean(d):+.2f}, 5% bound {lo:+.2f}"))
    # H4
    for stv in (5, 8):
        for arm in ARMS:
            rs = sel(storeys=stv, bracket="high", N=20, arm=arm)
            if rs:
                v = st.mean(r["L2"] for r in rs)
                res.append(("H4", f"s{stv} high {arm}", v < 0.5, f"mean L2 {v:.2f}"))
    # H5
    for stv in (2, 5, 8):
        for br in ("low", "central", "high"):
            rs = [r for r in sel(storeys=stv, bracket=br, N=20, arm="ca_tbl_full") if "dig_spt" in r]
            if rs:
                d = st.mean(r["dig_spt"] - r["dig_id"] for r in rs)
                res.append(("H5", f"s{stv} {br}", d >= 0.0, f"mean spt - id {d:+.2f} survivors"))
    # H6
    for stv in (2, 5, 8):
        a = sel(storeys=stv, bracket="central", N=30, arm="ca_tbl_full")
        b = sel(storeys=stv, bracket="central", N=20, arm="ca_tbl_full")
        if a and b:
            d = st.mean(r["L2"] for r in a) - st.mean(r["L2"] for r in b)
            res.append(("H6", f"s{stv} central", abs(d) <= 0.10, f"L2(30) - L2(20) {d:+.2f}"))
    for h, c, ok, info in res:
        print(f"{h} {'HOLDS' if ok else 'FAILS'}  {c:28} {info}")


if __name__ == "__main__":
    main()
