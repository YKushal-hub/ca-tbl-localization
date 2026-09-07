#!/usr/bin/env python3
"""A5 — complementary tier-3 siting, paired by seed. Per report/PREREG_A5.md."""
import csv
import glob
import os
import sys
from collections import defaultdict

ROOT = os.path.expanduser("~/catbl_work/a5")
SEEDS = [1, 2, 3, 4, 5]
METRICS = ["fixCycles", "cyclesInUwbCoverage", "rmseM", "cep95M", "totalJ"]

data = {}   # (mode, seed) -> {arm: row}
missing = []
for m in (0, 1):
    for s in SEEDS:
        f = os.path.join(ROOT, "m%d_s%d" % (m, s), "arm_summary.csv")
        if not os.path.isfile(f) or os.path.getsize(f) == 0:
            missing.append("m%d_s%d" % (m, s))
            continue
        rows = list(csv.DictReader(open(f)))
        if not rows:
            missing.append("m%d_s%d(empty)" % (m, s))
            continue
        data[(m, s)] = {r["arm"]: r for r in rows}

print("A5 -- complementary tier-3 siting (sitingMode 1) vs co-located (0)")
if missing:
    print("  EXCLUDED BY NAME (rule 12): %s" % ",".join(missing))
paired = [s for s in SEEDS if (0, s) in data and (1, s) in data]
print("  paired seeds: %d/%d" % (len(paired), len(SEEDS)))
if not paired:
    sys.exit("no paired seeds")

# Abort check: the flag must actually do something.
identical = all(data[(0, s)] == data[(1, s)] for s in paired)
if identical:
    sys.exit("ABORT: mode 0 and mode 1 identical -- the flag is not wired, nothing reported")

arms = sorted(data[(0, paired[0])].keys())
for metric in METRICS:
    print()
    print("  %s" % metric)
    print("    %-22s %10s %10s %10s   %s" % ("arm", "mode0", "mode1", "delta", "signs"))
    for arm in arms:
        v0, v1, signs = [], [], []
        for s in paired:
            try:
                a = float(data[(0, s)][arm][metric])
                b = float(data[(1, s)][arm][metric])
            except (KeyError, ValueError):
                continue
            v0.append(a)
            v1.append(b)
            signs.append("+" if b > a else ("-" if b < a else "0"))
        if not v0:
            continue
        m0 = sum(v0) / len(v0)
        m1 = sum(v1) / len(v1)
        print("    %-22s %10.4g %10.4g %+10.4g   %s"
              % (arm, m0, m1, m1 - m0, "".join(signs)))

# P3: sign consistency on fixCycles for the escalating arms.
print()
print("  P3 -- sign consistency on fixCycles (>=4 of 5 same sign required)")
for arm in arms:
    signs = []
    for s in paired:
        try:
            a = float(data[(0, s)][arm]["fixCycles"])
            b = float(data[(1, s)][arm]["fixCycles"])
        except (KeyError, ValueError):
            continue
        signs.append(1 if b > a else (-1 if b < a else 0))
    if not signs:
        continue
    pos = signs.count(1)
    neg = signs.count(-1)
    verdict = "consistent" if max(pos, neg) >= 4 else "SPLIT -- not a result (rule 8)"
    print("    %-22s +%d/-%d/0%d  %s" % (arm, pos, neg, signs.count(0), verdict))
