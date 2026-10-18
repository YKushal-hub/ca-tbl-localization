#!/usr/bin/env python3
"""B4-SWITCHING analysis (report/PREREG_B4_SWITCHING.md). usage: b4_analyse.py <outdir>"""
import math, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import s2c_analyse as s2  # noqa: E402
import numpy as np  # noqa: E402

PRESETS = ("engine", "gottschalk", "muller", "wolf")
MAIN = ("static_868", "static_2p4", "lora_fusion", "ca_tbl_full", "fixed_threshold")


def tagJ(d, arm):
    r = cs.arm_row(d, arm)
    return float(r["totalJ"]) if r else float("nan")


def collect(out, cell, arm):
    res = {}
    for d in cs.seeds(out, cell):
        if cs.ok(d):
            m = s2.seed_metrics(d, arm)
            if m is not None:
                m["tagJ"] = tagJ(d, arm)
                res[int(d.rsplit("s", 1)[1])] = m
    return res


def fmt(iv):
    return f"{iv[0]:+.2f} [{iv[1]:+.2f}, {iv[2]:+.2f}]"


def sign_test(a, b, k):
    """two-sided exact sign test that a[k] < b[k] more often than not; returns (n_less, n, p)."""
    s = sorted(set(a) & set(b)); dif = [a[i][k] - b[i][k] for i in s if a[i][k] != b[i][k]]
    n = len(dif); nl = sum(1 for x in dif if x < 0)
    p = sum(math.comb(n, j) for j in range(n + 1) if abs(j - n / 2) >= abs(nl - n / 2)) / 2 ** n if n else 1.0
    return nl, n, p


def main():
    out = sys.argv[1]; L = ["# B4-SWITCHING results (exploratory)", ""]; problems = []
    D = {}
    for p in PRESETS:
        for cfg in ("A", "B"):
            for cell in (f"P_{p}_{cfg}", f"P_{p}_{cfg}_OD"):
                ds = cs.seeds(out, cell)
                if len(ds) < 20:
                    problems.append(f"{cell}: {len(ds)} seeds")
                problems += [f"{d}: not ok" for d in ds if not cs.ok(d)]
            for arm in MAIN:
                D[(p, cfg, arm)] = collect(out, f"P_{p}_{cfg}", arm)
            D[(p, cfg, "on_demand")] = collect(out, f"P_{p}_{cfg}_OD", "ca_tbl_full")
            L += [f"## {p}, config {cfg}", "", "| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |", "|---|---|---|---|---|---|"]
            for arm in MAIN + ("on_demand",):
                x = D[(p, cfg, arm)]
                if not x:
                    continue
                v = lambda k: float(np.nanmean([x[s][k] for s in x]))
                L.append(f"| {arm} | {v('c95'):.1f} | {v('w25'):.3f} | {v('av'):.3f} | {v('tagJ'):.2f} | {len(x)} |")
            L.append("")
    for cfg in ("A", "B"):
        D[("S1off", cfg, "static_868")] = collect(out, f"S1off_muller_{cfg}", "static_868")
    pred = {}
    n1 = 0
    for p in PRESETS:
        iv, n = s2.paired(D[(p, "B", "ca_tbl_full")], D[(p, "B", "static_2p4")], "c95")
        n1 += (not math.isnan(iv[0])) and iv[0] <= 0
        L.append(f"B4-1 {p}: CEP95 ca_tbl_full − static_2p4 = {fmt(iv)} (n {n})")
    pred["B4-1"] = n1 >= 3
    ok2 = True
    for p in PRESETS:
        iv, n = s2.paired(D[(p, "B", "ca_tbl_full")], D[(p, "B", "static_868")], "w25")
        ok2 &= (not math.isnan(iv[1])) and iv[1] > 0
        L.append(f"B4-2 {p}: w25 ca_tbl_full − static_868 = {fmt(iv)}")
    pred["B4-2"] = ok2
    n3 = 0
    for p in PRESETS:
        a, b = D[(p, "B", "on_demand")], D[(p, "B", "ca_tbl_full")]
        s = sorted(set(a) & set(b)); fewer = sum(1 for i in s if a[i]["tagJ"] < b[i]["tagJ"])
        ca, cb = np.nanmean([a[i]["c95"] for i in s]), np.nanmean([b[i]["c95"] for i in s])
        ok = fewer >= 15 and ca <= 1.25 * cb; n3 += ok
        L.append(f"B4-3 {p}: on-demand uses less tag-0 J on {fewer}/{len(s)} seeds; CEP95 {ca:.1f} vs {cb:.1f} m")
    pred["B4-3"] = n3 >= 3
    n4 = 0
    for p in PRESETS:
        nl, n, pv = sign_test(D[(p, "B", "ca_tbl_full")], D[(p, "B", "fixed_threshold")], "c95")
        ok = pv < 0.05 and nl > n / 2; n4 += ok
        L.append(f"B4-4 {p}: ca_tbl_full CEP95 below fixed_threshold@12 on {nl}/{n} seeds, sign-test p = {pv:.4f}")
    pred["B4-4"] = n4 >= 3
    iv, n = s2.paired(D[("muller", "A", "static_868")], D[("S1off", "A", "static_868")], "c95")
    pred["B4-5"] = (not math.isnan(iv[1])) and iv[1] > 0
    L.append(f"B4-5: static_868 CEP95 S1 on − off (muller, A) = {fmt(iv)}")
    def gap(p):
        a, b = D[(p, "B", "static_2p4")], D[(p, "B", "static_868")]
        s = sorted(set(a) & set(b)); return float(np.nanmean([b[i]["c95"] - a[i]["c95"] for i in s]))
    pred["B4-6"] = gap("wolf") < gap("engine")
    L.append(f"B4-6: static_868 − static_2p4 CEP95 gap in B: engine {gap('engine'):.1f} m, wolf {gap('wolf'):.1f} m")
    L += ["", "## Predictions", ""] + [f"- **{k}**: {'held' if v else 'not held'}" for k, v in pred.items()]
    if problems:
        L += ["", "## Abort conditions", ""] + [f"- {x}" for x in problems]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "B4.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
