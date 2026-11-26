#!/usr/bin/env python3
"""COLLAPSE-4 analysis (report/PREREG_COLLAPSE4.md). usage: collapse4_analyse.py <outdir>"""
import csv
import glob
import os
import statistics as st
import sys

CELL_J, COMPUTE_JH = 16740.0, 1.82


def ok(d):
    return os.path.exists(os.path.join(d, "rc")) and open(os.path.join(d, "rc")).read().strip() == "0"


def rows(d):
    f = os.path.join(d, "subject_summary.csv")
    if os.path.exists(f):
        return [r for r in csv.DictReader(open(f)) if r["arm"] == "ca_tbl_full"]
    return [r for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))) if r["arm"] == "ca_tbl_full"]


def hours(r):
    dur = float(r["durationS"]) if r.get("durationS") else 2.5 * float(r["steps"])
    return CELL_J / (float(r["totalJ"]) * 3600.0 / dur + COMPUTE_JH)


def main():
    out = sys.argv[1]
    M = {}
    depth = {}
    for cell in sorted(glob.glob(os.path.join(out, "s*_*_n*_*"))):
        stv, br, nn, cfg = os.path.basename(cell).split("_")
        for sd in sorted(glob.glob(os.path.join(cell, "seed*"))):
            if not ok(sd):
                continue
            R = rows(sd)
            L2 = sum(float(r["cep50M"]) <= 2.0 for r in R) / len(R)
            H = [hours(r) for r in R]
            M.setdefault((stv, br, nn, cfg), []).append((L2, H))
            E = os.path.join(sd, "collapse_depth_ca_tbl_full_seed%s.csv" % sd.rsplit("seed", 1)[1])
            T = E.replace("collapse_depth_", "collapse_truth_")
            if nn == "n20" and os.path.exists(E):
                tr = {r["worker"]: float(r["depth_true_m"]) for r in csv.DictReader(open(T))}
                for r in csv.DictReader(open(E)):
                    dl = depth.setdefault((stv, cfg), [0, 0, []])
                    dl[0] += 1
                    if r["depth_est_m"] != "nan":
                        dl[1] += 1
                        dl[2].append(abs(float(r["depth_est_m"]) - tr[r["worker"]]))
    print("cell                        seeds  L2    hours_med  hours_p5")
    for k in sorted(M):
        L = [v[0] for v in M[k]]
        H = sorted(h for v in M[k] for h in v[1])
        print(f"{'_'.join(k):26} {len(L):5} {st.mean(L):5.2f} {H[len(H) // 2]:10.0f} {H[int(0.05 * (len(H) - 1))]:9.0f}")
    print("\nHYPOTHESES")
    for stv in ("s5", "s8"):
        for br in ("low", "central"):
            g = M.get((stv, br, "n20", "gate"))
            if g:
                H = sorted(h for v in g for h in v[1])
                p5 = H[int(0.05 * (len(H) - 1))]
                print(f"H13 {stv} {br} n20: gate p5 {p5:.0f} h {'HOLDS' if p5 >= 72 else 'FAILS'}")
            g1 = M.get((stv, br, "n1", "gate"))
            if g1:
                H = sorted(h for v in g1 for h in v[1])
                med = H[len(H) // 2]
                print(f"H13 {stv} {br} n1: gate median {med:.0f} h {'HOLDS' if med >= 72 else 'FAILS'}")
            b, g = M.get((stv, br, "n20", "base")), M.get((stv, br, "n20", "gate"))
            if b and g:
                d = st.mean(v[0] for v in g) - st.mean(v[0] for v in b)
                print(f"H14 {stv} {br}: gate L2 - base L2 {d:+.2f} {'HOLDS' if d >= -0.05 else 'FAILS'}")
            f, g = M.get((stv, br, "n20", "frame")), M.get((stv, br, "n20", "gate"))
            if f and g:
                d = st.mean(v[0] for v in f) - st.mean(v[0] for v in g)
                print(f"H16 {stv} {br}: frame L2 - gate L2 {d:+.2f} {'HOLDS' if d >= -0.05 else 'FAILS'}")
    for stv in ("s5", "s8"):
        tot = res = 0
        errs = []
        for (s_, cfg), (t, r, e) in depth.items():
            if s_ == stv:
                tot += t
                res += r
                errs += e
        if tot:
            med = st.median(errs) if errs else float("nan")
            okh = errs and med <= 1.0 and res / tot >= 0.10
            print(f"H15 {stv}: resolved {res}/{tot} ({res / tot:.2f}), median |error| {med:.2f} m {'HOLDS' if okh else 'FAILS'}")


if __name__ == "__main__":
    main()
