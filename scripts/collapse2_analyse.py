#!/usr/bin/env python3
"""COLLAPSE-2 analysis (report/PREREG_COLLAPSE2.md).
usage: collapse2_analyse.py dig <collapse1 dir> | stations <collapse1 dir> <collapse2 stations dir> | field <field dir>
Committed with the registration, before any of its data.
"""
import csv
import glob
import math
import os
import statistics as st
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "collapse"))
import score_dig as SD  # noqa: E402


def ok_runs(cell):
    return [d for d in sorted(glob.glob(os.path.join(cell, "seed*")))
            if os.path.exists(os.path.join(d, "rc")) and open(os.path.join(d, "rc")).read().strip() == "0"]


def dig(c1):
    combos = holds = 0
    for cell in sorted(glob.glob(os.path.join(c1, "s*_*_n20"))):
        for teams in (1, 2):
            for tdig in (0.5, 2.0, 8.0):
                for r in (1.0, 2.0):
                    diffs = []
                    for d in ok_runs(cell):
                        W = SD.load(d, "ca_tbl_full", "END")
                        if not W:
                            continue
                        s0 = max(w["t"] for w in W.values()) / 3600.0
                        et = {k: tdig * (1 + (max(w["sigma"], 1e-6) ** 2 / r ** 2) * math.exp(-r ** 2 / max(w["sigma"], 1e-6) ** 2))
                              for k, w in W.items()}
                        a = SD.schedule(sorted(W, key=lambda k: et[k]), W, teams, tdig, r, s0)[0]
                        b = SD.schedule(sorted(W), W, teams, tdig, r, s0)[0]
                        diffs.append(a - b)
                    if diffs:
                        combos += 1
                        m = st.mean(diffs)
                        holds += m >= 0
                        print(f"{os.path.basename(cell):18} teams {teams} t_dig {tdig:3} r {r}: spt - id {m:+.2f} survivors (n={len(diffs)})")
    if combos:
        print(f"H8 {'HOLDS' if holds >= 0.8 * combos else 'FAILS'}: spt >= id in {holds}/{combos} combinations")


def l2(d, arm="ca_tbl_full"):
    rows = [r for r in csv.DictReader(open(os.path.join(d, "subject_summary.csv"))) if r["arm"] == arm]
    return sum(float(r["cep50M"]) <= 2.0 for r in rows) / len(rows)


def stations(c1, c2):
    base = {os.path.basename(d): l2(d) for d in ok_runs(os.path.join(c1, "s8_central_n20"))}
    for gw in (3, 6):
        v = {os.path.basename(d): l2(d) for d in ok_runs(os.path.join(c2, f"s8_central_n20_gw{gw}"))}
        common = [s for s in v if s in base]
        if not common:
            continue
        d = st.mean(v[s] - base[s] for s in common)
        ok = abs(d) <= 0.15 if gw == 3 else d >= -0.05
        print(f"H9 gw={gw}: L2 {st.mean(v[s] for s in common):.2f} vs 4 stations {st.mean(base[s] for s in common):.2f} "
              f"(diff {d:+.2f}, n={len(common)}) {'HOLDS' if ok else 'FAILS'}")


def field(fd):
    res = {}
    for br in ("low", "central", "high"):
        for sp in ("1.0", "0.5"):
            per = {}
            for d in ok_runs(os.path.join(fd, f"field_{br}")):
                f = os.path.join(d, f"estimates_sp{sp}.csv")
                if not os.path.exists(f):
                    continue
                truth = {int(r["id"]): (float(r["x"]), float(r["y"]), float(r["z"]))
                         for r in csv.DictReader(open(os.path.join(d, "truth_victims.csv")))}
                rows = list(csv.DictReader(open(f)))
                last = max(int(r["slot"]) for r in rows)
                for m in ("tof", "rss", "joint", "coop"):
                    e = [(math.hypot(float(r["x"]) - truth[int(r["worker"])][0], float(r["y"]) - truth[int(r["worker"])][1]),
                          abs(float(r["z"]) - truth[int(r["worker"])][2]))
                         for r in rows if r["method"] == m and int(r["slot"]) == last]
                    if not e:
                        per.setdefault(m, []).append((0.0, float("nan"), float("nan")))
                        continue
                    w2 = sum(h <= 2.0 for h, _ in e) / len(truth)
                    per.setdefault(m, []).append((w2, st.median(h for h, _ in e), st.median(v for _, v in e)))
            for m, vals in per.items():
                res[(br, sp, m)] = (st.mean(v[0] for v in vals), st.mean(v[1] for v in vals if not math.isnan(v[1])) if any(not math.isnan(v[1]) for v in vals) else float("nan"),
                                    st.mean(v[2] for v in vals if not math.isnan(v[2])) if any(not math.isnan(v[2]) for v in vals) else float("nan"), len(vals))
    print("bracket  cover method  within2m  h_med  v_med  seeds")
    for k in sorted(res):
        w2, hm, vm, n = res[k]
        print(f"{k[0]:8} {k[1]:5} {k[2]:6} {w2:8.2f} {hm:6.2f} {vm:6.2f} {n:6}")
    for br in ("low", "central"):
        for sp in ("1.0", "0.5"):
            j = res.get((br, sp, "joint"))
            for other in ("rss", "tof"):
                o = res.get((br, sp, other))
                if j and o:
                    print(f"H10 {br} cover {sp} joint vs {other}: {j[0]:.2f} vs {o[0]:.2f} {'HOLDS' if j[0] >= o[0] else 'FAILS'}")
        c, j = res.get((br, "0.5", "coop")), res.get((br, "0.5", "joint"))
        if c and j:
            print(f"H11 {br} cover 0.5 coop vs joint median h: {c[1]:.2f} vs {j[1]:.2f} {'HOLDS' if c[1] <= j[1] else 'FAILS'}")
    j = res.get(("central", "1.0", "joint"))
    if j:
        print(f"H12 joint depth error {j[2]:.2f} m vs 3 x CRB 2.63 m: {'HOLDS' if j[2] <= 3 * 0.878 else 'FAILS'}")


if __name__ == "__main__":
    mode = sys.argv[1]
    if mode == "dig":
        dig(sys.argv[2])
    elif mode == "stations":
        stations(sys.argv[2], sys.argv[3])
    else:
        field(sys.argv[2])
