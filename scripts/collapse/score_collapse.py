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
    ap.add_argument("--relative", action="store_true",
                    help="remove the rescuer frame's mean horizontal offset (frame_<frame>.csv vs truth_anchors): "
                         "rescuers dig where their own frame says, so a common offset cancels on the rubble")
    a = ap.parse_args()
    truth = {int(r["id"]): np.array([float(r["x"]), float(r["y"]), float(r["z"])])
             for r in csv.DictReader(open(os.path.join(a.run, "truth_victims.csv")))}
    est = defaultdict(dict)
    for r in csv.DictReader(open(os.path.join(a.run, f"estimates_{a.frame}.csv"))):
        est[(r["method"], int(r["slot"]))][int(r["worker"])] = np.array([float(r["x"]), float(r["y"]), float(r["z"])])
    if a.relative:
        fp = os.path.join(a.run, f"frame_{a.frame}.csv")
        ta = {int(r["id"]): np.array([float(r["x"]), float(r["y"]), float(r["z"])])
              for r in csv.DictReader(open(os.path.join(a.run, "truth_anchors.csv")))}
        if os.path.exists(fp):
            fr = {int(r["id"]): np.array([float(r["x"]), float(r["y"]), float(r["z"])]) for r in csv.DictReader(open(fp))}
        else:  # local / gps: the anchors file itself is the frame
            col = ("loc_x", "loc_y", "loc_z") if a.frame == "local" else ("gps_x", "gps_y", "gps_z")
            fr = {int(r["id"]): np.array([float(r[c]) for c in col])
                  for r in csv.DictReader(open(os.path.join(a.run, "obs_anchors.csv"))) if r["kind"] == "point"}
        off = np.mean([fr[i] - ta[i] for i in fr if i in ta], axis=0)
        off[2] = 0.0
        print(f"frame offset removed: ({off[0]:+.2f}, {off[1]:+.2f}) m")
        for k in est:
            for w in est[k]:
                est[k][w] = est[k][w] - off
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
