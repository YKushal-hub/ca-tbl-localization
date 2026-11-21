#!/usr/bin/env python3
"""Scorer (analysis side, may read World truth): estimates_<frame>.csv against truth_victims.csv.
Prints, per method, at the last slot and over time: located fraction, horizontal / vertical / 3-D
error (median, 90th percentile), fraction within 1 m and 2 m horizontally."""
import argparse
import csv
import os
from collections import defaultdict

import numpy as np


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run")
    ap.add_argument("--frame", default="local")
    ap.add_argument("--curve", action="store_true")
    a = ap.parse_args()
    truth = {int(r["id"]): np.array([float(r["x"]), float(r["y"]), float(r["z"])])
             for r in csv.DictReader(open(os.path.join(a.run, "truth_victims.csv")))}
    est = defaultdict(dict)
    for r in csv.DictReader(open(os.path.join(a.run, f"estimates_{a.frame}.csv"))):
        est[(r["method"], int(r["slot"]))][int(r["worker"])] = np.array([float(r["x"]), float(r["y"]), float(r["z"])])
    methods = sorted({m for (m, _) in est})
    slots = sorted({s for (_, s) in est})
    N = len(truth)

    def stats(m, s):
        e = est.get((m, s), {})
        h = np.array([np.linalg.norm((e[w] - truth[w])[:2]) for w in e])
        v = np.array([abs((e[w] - truth[w])[2]) for w in e])
        if len(h) == 0:
            return dict(n=0)
        return dict(n=len(h), loc=len(h) / N, h50=np.median(h), h90=np.percentile(h, 90), v50=np.median(v),
                    v90=np.percentile(v, 90), w1=np.sum(h <= 1.0) / N, w2=np.sum(h <= 2.0) / N)

    last = slots[-1]
    print(f"{'method':6} located  h_med  h_p90  v_med  v_p90  <=1m  <=2m   (slot {last}, N={N})")
    for m in methods:
        s = stats(m, last)
        if s["n"] == 0:
            print(f"{m:6} none")
            continue
        print(f"{m:6} {s['loc']:7.2f} {s['h50']:6.2f} {s['h90']:6.2f} {s['v50']:6.2f} {s['v90']:6.2f} "
              f"{s['w1']:5.2f} {s['w2']:5.2f}")
    if a.curve:
        for m in methods:
            row = []
            for s in slots:
                st = stats(m, s)
                row.append(f"{s}:{st.get('w2', 0):.2f}")
            print(m, "within2m", " ".join(row))


if __name__ == "__main__":
    main()
