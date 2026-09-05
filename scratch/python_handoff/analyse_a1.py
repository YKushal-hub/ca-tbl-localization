#!/usr/bin/env python3
"""A1 -- the --dwell sweep at b3000.

THE REGISTERED PREDICTION, from docs/WORK_PACKAGES.md A1: precision-tier coverage
fraction, not gateway density, is what tracks the conjunction result. b1000/gw3 and
b3000/gw9 carry identical density and differ 8x in median error, while coverage
fraction differs 11.49% against 3.39%. Coverage fraction should follow

    dwell / (dwell + transit),  transit ~ 1880 s at b3000

so dwell = 245 s should restore b1000's 11.49%. The falsifier is measured coverage
fraction NOT tracking that model.

DENOMINATOR, stated (rule 1): coverage fraction is cyclesInUwbCoverage / steps over
the whole 3600 s run, all decision cycles, and a cycle out of coverage counts against.
"""
import csv
import os
import sys

ROOT = "results/dwell_b3000"
DWELLS = [90, 150, 245, 400, 800]
SEEDS = range(1, 21)
TRANSIT = 1880.0
A_MIN, R_MAX = 0.95, 25.0


def load(d, s):
    f = os.path.join(ROOT, "d%d_seed%d" % (d, s), "arm_summary.csv")
    if not os.path.isfile(f):
        return {}
    return {r["arm"]: r for r in csv.DictReader(open(f))}


def med(v):
    v = sorted(v)
    n = len(v)
    if not n:
        return float("nan")
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


if not os.path.isdir(ROOT):
    sys.exit("missing %s" % ROOT)

print("A1 -- dwell sweep at boxHalf=3000, gwGrid=3, 20 seeds per point")
print("coverage = cyclesInUwbCoverage / steps, all cycles, out-of-coverage counts against")
print("=" * 100)
print("%6s %10s %10s %9s  %11s %9s %8s"
      % ("dwell", "predicted", "measured", "delta pp", "med cep50M", "med A", "meets"))

for d in DWELLS:
    cov, cep, avail, meets, n = [], [], [], 0, 0
    for s in SEEDS:
        r = load(d, s).get("ca_tbl_full")
        if not r:
            continue
        steps = float(r["steps"])
        if steps <= 0:
            continue
        n += 1
        cov.append(float(r["cyclesInUwbCoverage"]) / steps)
        cep.append(float(r["cep50M"]))
        a = float(r["fixCycles"]) / steps
        avail.append(a)
        if a >= A_MIN and float(r["cep50M"]) <= R_MAX:
            meets += 1
    if not cov:
        print("%6d   NO DATA" % d)
        continue
    pred = d / (d + TRANSIT)
    m = sum(cov) / len(cov)
    print("%6d %9.2f%% %9.2f%% %+8.2f  %10.2f m %9.4f %5d/%-2d"
          % (d, 100 * pred, 100 * m, 100 * (m - pred), med(cep), med(avail), meets, n))

print("\n" + "=" * 100)
print("MODEL CHECK: measured coverage should track dwell/(dwell+1880) to 1-2 points,")
print("as it did at both existing geometries. A delta that GROWS with dwell means the")
print("transit estimate is wrong; a flat small delta means the model holds.")
print()
print("CONJUNCTION: b1000 reported 11.49% coverage with sub-10 m median. If dwell 245")
print("restores the coverage fraction AND the accuracy, the conjunction claim becomes a")
print("DEPLOYMENT REQUIREMENT -- a coverage floor -- rather than a failure at scale. If")
print("coverage returns and accuracy does not, coverage fraction is NOT sufficient and")
print("the claim must be stated as dwell-dependent. That caveat was registered up front.")
