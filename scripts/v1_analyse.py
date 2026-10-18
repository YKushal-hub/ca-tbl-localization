#!/usr/bin/env python3
"""V1-RNGKEYED analysis (report/PREREG_V1_RNGKEYED.md). usage: v1_analyse.py <outdir>"""
import csv, glob, os, re, statistics as st, sys

TIERS = ("868MHz", "2.4GHz", "UWB")
ARMS = ("static_868", "static_2p4", "ca_tbl_full", "fixed_threshold")


def r80(rows, tier):
    pts = sorted((float(r["distance_m"]), float(r["pdr"])) for r in rows if r["tier"] == tier)
    if not pts or pts[0][1] < 0.8:
        return 0.0
    for (d0, p0), (d1, p1) in zip(pts, pts[1:]):
        if p1 < 0.8 <= p0:
            return d0 + (p0 - 0.8) * (d1 - d0) / (p0 - p1)
    return pts[-1][0]


def ok(d):
    return os.path.exists(os.path.join(d, "rc")) and open(os.path.join(d, "rc")).read().strip() == "0"


def main():
    out = sys.argv[1]; L = ["# V1-RNGKEYED results (exploratory)", ""]; problems = []
    for d in sorted(glob.glob(os.path.join(out, "*_k*", "s*"))):
        if not ok(d):
            problems.append(f"{d}: rc")
        rid = os.path.join(d, "run_id.txt")
        if os.path.exists(rid) and "dirty 0" not in open(rid).read():
            problems.append(f"{d}: not dirty 0")
        if "_ktrue" in d:
            m = re.findall(r"RNGKEYED unkeyed_draws=(\d+)", open(os.path.join(d, "run.log"), errors="replace").read())
            if not m or m[-1] != "0":
                problems.append(f"{d}: unkeyed draws {m[-1] if m else None}")
    # PROBE
    R = {}; C = {}
    for k in ("false", "true"):
        for d in sorted(glob.glob(os.path.join(out, f"PROBE_k{k}", "s*"))):
            if not ok(d):
                continue
            rows = list(csv.DictReader(open(os.path.join(d, "probe_range_sweep.csv"))))
            for t in TIERS:
                R.setdefault((k, t), []).append(r80(rows, t))
                for r in rows:
                    if r["tier"] == t:
                        C.setdefault((k, t, float(r["distance_m"])), []).append(float(r["pdr"]))
    L += ["## PROBE", "", "| band | r80 unkeyed (m) | r80 keyed (m) | change | max abs PDR difference | V1-1 | V1-2 |", "|---|---|---|---|---|---|---|"]
    v11 = v12 = True
    for t in TIERS:
        a, b = st.mean(R[("false", t)]), st.mean(R[("true", t)])
        okr = abs(b - a) <= 1.0 if t == "UWB" else abs(b - a) <= 0.05 * a
        ds = sorted(x for (k, tt, x) in C if k == "false" and tt == t)
        mx = max(abs(st.mean(C[("true", t, x)]) - st.mean(C[("false", t, x)])) for x in ds if ("true", t, x) in C)
        okc = mx <= 0.05
        v11 &= okr; v12 &= okc
        L.append(f"| {t} | {a:.1f} | {b:.1f} | {((b - a) / a * 100 if a else float('nan')):+.1f} % | {mx:.3f} | {'held' if okr else 'not held'} | {'held' if okc else 'not held'} |")
    # PAIR
    M = {}
    for k in ("false", "true"):
        for d in sorted(glob.glob(os.path.join(out, f"PAIR_k{k}", "s*"))):
            if not ok(d):
                continue
            s = int(d.rsplit("s", 1)[1])
            for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
                M[(k, r["arm"], s)] = float(r["cep95M"])
    L += ["", "## PAIR (config B, 900 s, 10 seeds)", "", "| arm | CEP95 unkeyed mean (SD) | CEP95 keyed mean (SD) | shift / max SD | V1-4 |", "|---|---|---|---|---|"]
    v14 = True
    for a in ARMS:
        xu = [M[("false", a, s)] for s in range(1, 11) if ("false", a, s) in M]
        xk = [M[("true", a, s)] for s in range(1, 11) if ("true", a, s) in M]
        sd = max(st.stdev(xu), st.stdev(xk)); sh = abs(st.mean(xk) - st.mean(xu))
        okk = sh < sd; v14 &= okk
        L.append(f"| {a} | {st.mean(xu):.1f} ({st.stdev(xu):.1f}) | {st.mean(xk):.1f} ({st.stdev(xk):.1f}) | {sh / sd:.2f} | {'held' if okk else 'not held'} |")
    du = [M[("false", "ca_tbl_full", s)] - M[("false", "fixed_threshold", s)] for s in range(1, 11)]
    dk = [M[("true", "ca_tbl_full", s)] - M[("true", "fixed_threshold", s)] for s in range(1, 11)]
    ratio = st.stdev(dk) / st.stdev(du) if st.stdev(du) > 0 else float("nan")
    L += ["", f"Paired difference CEP95(ca_tbl_full) - CEP95(fixed_threshold): SD unkeyed {st.stdev(du):.2f} m, keyed {st.stdev(dk):.2f} m, ratio {ratio:.2f}"]
    L += ["", "## Predictions", "", f"- **V1-1**: {'held' if v11 else 'not held'}", f"- **V1-2**: {'held' if v12 else 'not held'}",
          f"- **V1-3**: {'held' if ratio < 1 else 'not held'} (ratio {ratio:.2f})", f"- **V1-4**: {'held' if v14 else 'not held'}"]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems)]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V1.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
