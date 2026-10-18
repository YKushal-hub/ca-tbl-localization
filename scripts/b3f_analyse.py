#!/usr/bin/env python3
"""B3F-FOLIAGE analysis (report/PREREG_B3F_FOLIAGE.md). usage: b3f_analyse.py <outdir>"""
import csv, glob, os, sys


def r80(rows, tier):
    pts = sorted((float(r["distance_m"]), float(r["pdr"])) for r in rows if r["tier"] == tier)
    if not pts or pts[0][1] < 0.8:
        return 0.0
    for (d0, p0), (d1, p1) in zip(pts, pts[1:]):
        if p1 < 0.8 <= p0:
            return d0 + (p0 - 0.8) * (d1 - d0) / (p0 - p1)
    return pts[-1][0]


def main():
    out = sys.argv[1]; tab = {}; problems = []
    for c in ("false", "true"):
        for d in sorted(glob.glob(os.path.join(out, f"clamp_{c}", "s*"))):
            if open(os.path.join(d, "rc")).read().strip() != "0":
                problems.append(d)
                continue
            rows = list(csv.DictReader(open(os.path.join(d, "probe_range_sweep.csv"))))
            for t in ("868MHz", "2.4GHz", "UWB"):
                tab.setdefault((c, t), []).append(r80(rows, t))
    lines = ["# B3F-FOLIAGE results", "", "| tier | r80 unclamped (m) | r80 clamped (m) | change |", "|---|---|---|---|"]
    m = {}
    for t in ("868MHz", "2.4GHz", "UWB"):
        a = sum(tab[("false", t)]) / len(tab[("false", t)]); b = sum(tab[("true", t)]) / len(tab[("true", t)])
        m[t] = (a, b)
        lines.append(f"| {t} | {a:.1f} | {b:.1f} | {((b - a) / a * 100 if a > 0 else float('inf')):+.1f} % |")
    a, b = m["868MHz"]; f1 = abs(b - a) <= 0.01 * a
    a2, b2 = m["UWB"]; f2 = a2 < 10.0 <= b2
    a3, b3 = m["2.4GHz"]; f3 = 0.0 <= (b3 - a3) / a3 <= 0.25
    lines += ["", f"- **F1**: {'held' if f1 else 'not held'}", f"- **F2**: {'held' if f2 else 'not held'}", f"- **F3**: {'held' if f3 else 'not held'}"]
    if problems:
        lines += ["", "Abort conditions: " + ", ".join(problems)]
    txt = "\n".join(lines) + "\n"; open(os.path.join(out, "B3F.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
