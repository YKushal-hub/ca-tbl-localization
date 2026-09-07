#!/usr/bin/env python3
"""A6 -- the hysteresis sweep. Applies report/PREREG_A6_HYSTERESIS.md verbatim.

Paired per-seed differences against the BASELINE CELL (band 0.2, dwell 5), which is
the built-in configuration, with a two-sided sign test. Paired because the seed fixes
the channel realisation; a single-seed difference is never a result (rule 8).

Registered decision rule, section 6:
  FLAT          no cell moves cep50M beyond its paired CI -> CAL-03/CAL-04 answered
                as measured and inconsequential. Named in advance as EXPECTED.
  LIVE AXIS     at least one cell moves accuracy beyond CI with a consistent
                direction across BOTH arms -> they become calibration targets.
  INCONCLUSIVE  movement differing in sign between the arms, or not ordering with
                the band. Reported as such, not resolved by picking the arm that
                agrees.
"""
import csv
import glob
import math
import os
import random
import sys

ROOT = "results/a6_hysteresis"
BANDS = ["0.0", "0.1", "0.2", "0.4", "0.8"]
DWELLS = ["1", "5", "10"]
SEEDS = range(1, 21)
ARMS = ["ca_tbl_full", "fixed_threshold"]
BASE = ("0.2", "5")
NBOOT = 10000
random.seed(20260905)


def load(band, dwell, seed):
    f = os.path.join(ROOT, "b%s_n%s" % (band, dwell), "seed_%d" % seed, "arm_summary.csv")
    if not os.path.isfile(f):
        return {}
    return {r["arm"]: r for r in csv.DictReader(open(f))}


def sign_test(d):
    pos = sum(1 for x in d if x > 0)
    neg = sum(1 for x in d if x < 0)
    n = pos + neg
    if n == 0:
        return pos, neg, 1.0
    k = min(pos, neg)
    tail = sum(math.comb(n, i) for i in range(k + 1))
    return pos, neg, min(1.0, 2.0 * tail / (2.0 ** n))


def ci(vals):
    if not vals:
        return float("nan"), float("nan")
    b = []
    n = len(vals)
    for _ in range(NBOOT):
        s = [vals[random.randrange(n)] for _ in range(n)]
        b.append(sum(s) / n)
    b.sort()
    return b[int(0.025 * NBOOT)], b[int(0.975 * NBOOT)]


if not os.path.isdir(ROOT):
    sys.exit("missing %s" % ROOT)

base = {s: load(BASE[0], BASE[1], s) for s in SEEDS}

print("A6 -- hysteresis sweep, paired against the built-in cell (band 0.2, dwell 5)")
print("=" * 92)

moved = []
for arm in ARMS:
    print("\n### %s" % arm)
    print("  %-14s %12s %-22s %10s %12s %10s"
          % ("cell", "d cep50M", "95% CI", "sign", "d totalJ", "d escal"))
    for band in BANDS:
        for dw in DWELLS:
            if (band, dw) == BASE:
                continue
            dc, dj, de = [], [], []
            for s in SEEDS:
                b = base.get(s, {}).get(arm)
                c = load(band, dw, s).get(arm)
                if not b or not c:
                    continue
                dc.append(float(c["cep50M"]) - float(b["cep50M"]))
                dj.append(float(c["totalJ"]) - float(b["totalJ"]))
                de.append(float(c["escalations"]) - float(b["escalations"]))
            if not dc:
                continue
            m = sum(dc) / len(dc)
            lo, hi = ci(dc)
            pos, neg, p = sign_test(dc)
            flag = ""
            if lo > 0 or hi < 0:
                flag = "  <-- MOVED"
                moved.append((arm, band, dw, m, lo, hi))
            print("  b%-5s n%-6s %+12.4f [%+8.4f,%+8.4f] %4d+/%2d- %+12.3f %+10.1f%s"
                  % (band, dw, m, lo, hi, pos, neg, sum(dj) / len(dj), sum(de) / len(de), flag))

print("\n" + "=" * 92)
print("REGISTERED VERDICT")
if not moved:
    print("  FLAT -- no cell moves cep50M beyond its paired CI, on either arm.")
    print("  CAL-03 and CAL-04 are answered: measured, and inconsequential at this")
    print("  geometry. The registration named this as the expected outcome and as a")
    print("  reportable result -- measured and inconsequential beats never asked.")
else:
    per_arm = {}
    for arm, band, dw, m, lo, hi in moved:
        per_arm.setdefault((band, dw), []).append((arm, m))
    both = {k: v for k, v in per_arm.items() if len(v) == 2}
    consistent = {k: v for k, v in both.items()
                  if (v[0][1] > 0) == (v[1][1] > 0)}
    print("  %d cell/arm combinations moved beyond CI." % len(moved))
    print("  cells moving on BOTH arms: %d, of which consistent in sign: %d"
          % (len(both), len(consistent)))
    if consistent:
        print("  LIVE AXIS -- hysteresis is a real tuning axis and CAL-03/CAL-04 need a")
        print("  calibrated value rather than a verdict.")
    else:
        print("  INCONCLUSIVE -- movement does not agree across the two arms. Reported")
        print("  as such rather than resolved by choosing the arm that agrees.")
