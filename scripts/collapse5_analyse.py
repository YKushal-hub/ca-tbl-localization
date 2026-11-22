#!/usr/bin/env python3
"""COLLAPSE-5 analyser (report/PREREG_COLLAPSE5.md). Committed with the registration, before any cell.
usage: collapse5_analyse.py pipe PIPE_OUT | field FIELD_OUT | dig COLLAPSE1_DIR"""
import csv
import glob
import math
import os
import statistics as st
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "collapse"))


def ok_runs(cell):
    return sorted(d for d in glob.glob(os.path.join(cell, "seed*"))
                  if os.path.exists(os.path.join(d, "rc")) and open(os.path.join(d, "rc")).read().strip() == "0")


def arm_stats(cell, arm):
    """per seed: (L2, median worker J)"""
    out = {}
    for d in ok_runs(cell):
        f = os.path.join(d, "subject_summary.csv")
        if not os.path.exists(f):
            continue
        R = [r for r in csv.DictReader(open(f)) if r["arm"] == arm]
        if R:
            out[os.path.basename(d)] = (sum(float(r["cep50M"]) <= 2.0 for r in R) / len(R), st.median(float(r["totalJ"]) for r in R))
    return out


def mean(v):
    return st.mean(v) if v else float("nan")


def pipe(o):
    L = {}
    for s in (5, 8):
        for cfg, arms in (("g", ("ca_tbl_full", "static_868", "static_2p4", "static_uwb", "poucet_fusion")),
                          ("rubble", ("ca_tbl_full",)), ("muller", ("ca_tbl_full", "static_2p4")),
                          ("st0", ("ca_tbl_full",)), ("st2", ("ca_tbl_full",)), ("n10", ("ca_tbl_full",))):
            for a in arms:
                v = arm_stats(os.path.join(o, f"s{s}_{cfg}"), a)
                L[(s, cfg, a)] = (mean([x[0] for x in v.values()]), mean([x[1] for x in v.values()]), len(v))
    print("storeys config  arm             L2     J_median  seeds")
    for k in sorted(L):
        print(f"{k[0]:7} {k[1]:7} {k[2]:14} {L[k][0]:5.2f} {L[k][1]:9.1f} {L[k][2]:6}")
    f = lambda s, c, a="ca_tbl_full": L[(s, c, a)]
    for s in (5, 8):
        g = f(s, "g")
        r = f(s, "rubble")
        print(f"H17 s{s}: rubble {r[0]:.2f} vs g {g[0]:.2f} {'HOLDS' if r[0] >= g[0] + 0.05 else 'FAILS'}")
        p = f(s, "g", "poucet_fusion")
        ok = g[0] >= p[0] - 0.05 and g[1] <= 0.5 * p[1]
        print(f"H18 s{s}: L2 {g[0]:.2f} vs always-on {p[0]:.2f}; J {g[1]:.1f} vs {p[1]:.1f} {'HOLDS' if ok else 'FAILS'}")
        best = max(("static_868", "static_2p4", "static_uwb"), key=lambda a: f(s, "g", a)[0])
        print(f"H19 s{s}: L2 {g[0]:.2f} vs best single {best} {f(s, 'g', best)[0]:.2f} "
              f"{'HOLDS' if g[0] >= f(s, 'g', best)[0] - 0.10 else 'FAILS'}")
        s0, s2 = f(s, "st0")[0], f(s, "st2")[0]
        print(f"H20 s{s}: stations 0/2/4 L2 {s0:.2f}/{s2:.2f}/{g[0]:.2f} {'HOLDS' if s0 <= s2 + 0.05 and s2 <= g[0] + 0.05 else 'FAILS'}")
        n = f(s, "n10")[0]
        print(f"H21 s{s}: crew 10 L2 {n:.2f} vs 20 {g[0]:.2f} {'HOLDS' if abs(n - g[0]) <= 0.10 else 'FAILS'}")
    m, m2 = f(8, "muller"), f(8, "muller", "static_2p4")
    print(f"H22 s8 muller: ca_tbl_full {m[0]:.2f} vs static_2p4 {m2[0]:.2f} {'HOLDS' if m[0] >= m2[0] - 0.05 else 'FAILS'}")


def score(d, name):
    f = os.path.join(d, f"estimates_{name}.csv")
    if not os.path.exists(f):
        return None
    T = {int(r["id"]): (float(r["x"]), float(r["y"]), float(r["z"])) for r in csv.DictReader(open(os.path.join(d, "truth_victims.csv")))}
    R = list(csv.DictReader(open(f)))
    if not R:
        return None
    last = max(int(r["slot"]) for r in R)
    res = {}
    for m in {r["method"] for r in R}:
        rows = [r for r in R if r["method"] == m and int(r["slot"]) == last and r["x"] not in ("nan", "")]
        e = [(math.hypot(float(r["x"]) - T[int(r["worker"])][0], float(r["y"]) - T[int(r["worker"])][1]),
              abs(float(r["z"]) - T[int(r["worker"])][2])) for r in rows]
        e = [x for x in e if not math.isnan(x[0])]
        res[m] = (sum(h <= 2 for h, _ in e) / len(T), st.median(h for h, _ in e) if e else float("nan"),
                  st.median(v for _, v in e) if e else float("nan"), len(e) / len(T))
    return res


def field(o):
    agg = {}
    for part in ("reuse", "h15"):
        for br in ("low", "central", "high"):
            for d in ok_runs(os.path.join(o, f"{part}_{br}")):
                for name in ("serp", "dopt"):
                    r = score(d, name)
                    for m, v in (r or {}).items():
                        agg.setdefault((part, br, name, m), []).append(v)
    M = {k: tuple(mean([x[i] for x in v if not math.isnan(x[i])]) for i in range(4)) + (len(v),) for k, v in agg.items()}
    print("part   bracket moves method    within2m h_med  v_med  placed seeds")
    for k in sorted(M):
        print(f"{k[0]:6} {k[1]:7} {k[2]:5} {k[3]:9} {M[k][0]:8.2f} {M[k][1]:6.2f} {M[k][2]:6.2f} {M[k][3]:6.2f} {M[k][4]:5}")
    g = lambda br, mv, m, part="reuse": M.get((part, br, mv, m))
    for br in ("low", "central"):
        j = g(br, "serp", "joint")
        for b in ("rsstri", "spawn"):
            o_ = g(br, "serp", b)
            if j and o_:
                print(f"H23 {br}: joint {j[0]:.2f} vs {b} {o_[0]:.2f} {'HOLDS' if j[0] >= o_[0] else 'FAILS'}")
        c, s = g(br, "serp", "coop"), g(br, "serp", "spawn")
        if c and s:
            print(f"H24 {br}: coop {c[0]:.2f} vs spawn {s[0]:.2f} {'HOLDS' if c[0] >= s[0] else 'FAILS'}")
        cd = g(br, "dopt", "coop")
        if c and cd:
            print(f"H25 {br}: coop dopt {cd[0]:.2f} vs serp {c[0]:.2f} {'HOLDS' if cd[0] >= c[0] - 0.05 else 'FAILS'}")
        jh = g(br, "serp", "joint", "h15")
        if j and jh:
            print(f"H27 {br}: joint |z| median with spread {j[2]:.2f} vs 1.5 m only {jh[2]:.2f} {'HOLDS' if j[2] <= jh[2] else 'FAILS'}")
    for br in ("low", "central", "high"):
        v = g(br, "serp", "coop_via")
        print(f"H26 {br}: placed only via other workers {v[3] if v else 0:.2f} of workers; median h error "
              f"{v[1] if v else float('nan'):.2f} m")


def dig(c1):
    import score_dig as S
    comb = ok = 0
    for cell in sorted(glob.glob(os.path.join(c1, "s*_n20"))):
        for teams in (1, 2):
            for td in (0.5, 2.0, 8.0):
                for rad in (1.0, 2.0):
                    a_rd, a_end, t0 = [], [], []
                    for d in ok_runs(cell):
                        W = S.load(d, "ca_tbl_full", "END")
                        if not W:
                            continue
                        start = max(w["t"] for w in W.values()) / 3600.0
                        et = lambda w: td * (1 + (max(w["sigma"], 1e-6) ** 2 / rad ** 2) * math.exp(-rad ** 2 / max(w["sigma"], 1e-6) ** 2))
                        a_end.append(S.schedule(sorted(W, key=lambda k: et(W[k])), W, teams, td, rad, start)[0])
                        ser = S.load_series(d, "ca_tbl_full")
                        t = S.range_or_dig(ser, teams, td, rad)
                        Wd = {}
                        for k, v in ser.items():
                            x = next((row for row in reversed(v) if row[0] <= t), v[0])
                            Wd[k] = dict(est=x[1], true=x[2], sigma=x[3], t=x[0])
                        a_rd.append(S.schedule(sorted(Wd, key=lambda k: et(Wd[k])), Wd, teams, td, rad, t / 3600.0)[0])
                        t0.append(t / 60)
                    if not a_rd:
                        continue
                    comb += 1
                    good = mean(a_rd) >= mean(a_end) - 0.1
                    ok += good
                    print(f"{os.path.basename(cell):16} teams {teams} tdig {td:3} r {rad}: start {mean(t0):5.1f} min, "
                          f"survivors {mean(a_rd):.2f} vs at end {mean(a_end):.2f}")
    print(f"H28: {ok}/{comb} combinations within 0.1 {'HOLDS' if comb and ok >= 0.9 * comb else 'FAILS'}")


if __name__ == "__main__":
    {"pipe": pipe, "field": field, "dig": dig}[sys.argv[1]](sys.argv[2])
