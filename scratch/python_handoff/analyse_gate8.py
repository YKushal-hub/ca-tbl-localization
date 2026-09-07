#!/usr/bin/env python3
"""Paired analysis of the evidence gate and the bidirectional exchange, 8 seeds.

Completeness first (rule 12): every cell must exist with exactly one data row, and
every config must cover the same seed set. A dropped cell would shrink n silently.
"""
import csv, os, sys, random  # ROOT: pass the campaign directory as argv[1]
from math import comb

ROOT = (sys.argv[1] if len(sys.argv) > 1 else
        os.path.expanduser("~/catbl_work/gate8"))
SEEDS = list(range(1, 9))
CFGS = ["base", "gate", "full"]
B = 20000
random.seed(20260908)


def load(cfg, s):
    p = os.path.join(ROOT, "%s_s%d" % (cfg, s), "arm_summary.csv")
    if not os.path.exists(p):
        return None, "missing %s" % p
    rows = list(csv.DictReader(open(p)))
    if len(rows) != 1:
        return None, "%s has %d rows, expected 1" % (p, len(rows))
    return rows[0], None


def pctile(xs, q):
    s = sorted(xs); k = (len(s) - 1) * q
    lo, hi = int(k), min(int(k) + 1, len(s) - 1)
    return s[lo] + (s[hi] - s[lo]) * (k - lo)


def boot(d):
    n = len(d)
    m = [sum(random.choice(d) for _ in range(n)) / float(n) for _ in range(B)]
    return pctile(m, 0.025), pctile(m, 0.975)


def sign_p(d):
    k = sum(1 for x in d if x < 0); m = sum(1 for x in d if x != 0)
    if m == 0:
        return float("nan"), 0, 0
    return sum(comb(m, i) for i in range(k, m + 1)) / float(2 ** m), k, m


data, errs = {}, []
for c in CFGS:
    for s in SEEDS:
        r, e = load(c, s)
        if e:
            errs.append(e)
        else:
            data[(c, s)] = r
if errs:
    print("REFUSING TO POOL -- completeness failed:")
    for e in errs:
        print("  " + e)
    sys.exit(2)

print("Paired sweep, %d seeds x %d configs, all cells complete\n" % (len(SEEDS), len(CFGS)))
print("  %-5s %8s %8s %7s %7s %7s" % ("cfg", "rmse", "totalJ", "esc", "uwbCyc", "noFix"))
for c in CFGS:
    f = lambda k: [float(data[(c, s)][k]) for s in SEEDS]
    mean = lambda v: sum(v) / len(v)
    print("  %-5s %8.3f %8.2f %7.1f %7.1f %7.1f" % (
        c, mean(f("rmseM")), mean(f("totalJ")), mean(f("escalations")),
        mean(f("cycles_uwb")), mean(f("noFixCycles"))))
print()

for a, b in (("base", "gate"), ("gate", "full"), ("base", "full")):
    print("=== %s -> %s ===" % (a, b))
    for metric, unit in (("rmseM", "m"), ("totalJ", "J")):
        d = [float(data[(b, s)][metric]) - float(data[(a, s)][metric]) for s in SEEDS]
        lo, hi = boot(d)
        p, k, m = sign_p(d)
        mean = sum(d) / len(d)
        excl = "EXCLUDES ZERO" if (hi < 0 or lo > 0) else "INCLUDES ZERO"
        print("  %-7s mean %+9.3f %s   95%% CI [%+9.3f, %+9.3f]  %s   improved %d/%d, sign p=%.4f"
              % (metric, mean, unit, lo, hi, excl, k, m, p))
    print()
print("n = 8, one duration, one box size, one finder count, one mobility model.")
print("Every config differs in transmission count, so these are cross-configuration")
print("differences on a shared RNG stream (TECH_DEBT 1.1). Pairing by seed controls the")
print("trajectory; it does not undo stream displacement.")
