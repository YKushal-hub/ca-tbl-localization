#!/usr/bin/env python3
"""Fit r50 and an effective sigma to the 2.4 GHz PDR curve of probe sweeps (PREREG_B4_SWITCHING sect 2).
Model: pdr(d) = Phi(10 n log10(r50 / d) / s), n = 2.3. Grid search, least squares over rows with n >= 200.
usage: b4_fit_ondemand.py <probe_range_sweep.csv> [...]   prints 'r50 s rmse'"""
import csv, math, sys
import numpy as np

pts = []
for f in sys.argv[1:]:
    for r in csv.DictReader(open(f)):
        if r["tier"] == "2.4GHz" and int(r["n"]) >= 200:
            pts.append((float(r["distance_m"]), float(r["pdr"])))
d = np.array([p[0] for p in pts]); y = np.array([p[1] for p in pts])
best = None
for r50 in np.arange(20, 2000, 1.0):
    for s in np.arange(2, 25, 0.1):
        z = 10 * 2.3 * np.log10(r50 / d) / s
        pred = 0.5 * np.array([math.erfc(-x / math.sqrt(2)) for x in z])
        e = float(np.sqrt(np.mean((pred - y) ** 2)))
        if best is None or e < best[2]:
            best = (r50, s, e)
print(f"{best[0]:.0f} {best[1]:.1f} {best[2]:.4f}")
