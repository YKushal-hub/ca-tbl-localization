#!/usr/bin/env python3
"""A4 — is tier-1 reported sigma over-stated relative to realised error?

STATISTIC. NEES on the position residual, 2 DOF:

    NEES = e^T P_pos^-1 e,  E[NEES] = 2 for a correctly specified filter

The trace carries trace(P_pos) as `traceSigma`, not the full matrix. Under isotropy
P_pos = diag(p, p), trace = 2p, so NEES = 2 * errM^2 / traceSigma. **That isotropy step
is an approximation and is stated rather than hidden**: the covariance is demonstrably
NOT isotropic (Pxy is non-zero throughout the C5 relay measurements), so this is a
summary of the radial fit, not the exact quadratic form. It is the right direction
indicator and the wrong thing to quote to three figures.

NEES < 2 means the filter's claimed uncertainty EXCEEDS the error it actually makes --
sigma over-stated, which is A4's hypothesis.
"""
import csv
import glob
import os
import sys
from collections import defaultdict

ROOT = os.path.expanduser("~/catbl_work/a4")

# fixTier: 0 = 868. hadFix must be 1 or there is no residual to score.
rows_by_gw = defaultdict(list)
rows_all = defaultdict(list)
cells = 0
missing = []
incomplete = []
# 6 arms x (1440 cycles + header) for a 3600 s run at dt = 2.5 s.
EXPECT_ROWS = 8646

for d in sorted(glob.glob(os.path.join(ROOT, "gw*_s*"))):
    name = os.path.basename(d)
    gw = int(name.split("_")[0][2:])
    traces = glob.glob(os.path.join(d, "trace_*.csv"))
    if not traces:
        missing.append(name)
        continue
    # RULE 12: count rows, do not test for non-empty. A trace still being written is
    # non-empty from its first cycle, and an earlier pass of this analysis silently
    # pooled five half-written cells -- gw20 showed n=12028 against ~23500 elsewhere,
    # which is what gave it away.
    nrows = sum(sum(1 for _ in open(f)) for f in traces)
    if nrows != EXPECT_ROWS:
        incomplete.append("%s(%d)" % (name, nrows))
        continue
    cells += 1
    for f in traces:
        with open(f) as fh:
            for r in csv.DictReader(fh):
                try:
                    had = int(r["hadFix"])
                    # fixTier is a TIER NAME in the trace, not an index. Reading it as
                    # an int silently skipped every row and produced an empty pool that
                    # looked like an abort condition.
                    ft = r["fixTier"].strip()
                    err = float(r["errM"])
                    ts = float(r["traceSigma"])
                except (KeyError, ValueError, TypeError):
                    continue
                if had != 1 or ts <= 0.0:
                    continue
                nees = 2.0 * err * err / ts
                rows_all[gw].append(nees)
                if ft == "868MHz":
                    rows_by_gw[gw].append(nees)


def summarise(v):
    if not v:
        return None
    s = sorted(v)
    n = len(s)
    mean = sum(s) / n
    med = s[n // 2] if n % 2 else 0.5 * (s[n // 2 - 1] + s[n // 2])
    # 2-DOF chi-square 95% bound is 5.991
    above = sum(1 for x in s if x > 5.991) / n
    return n, mean, med, above


print("A4 -- tier-1 measurement calibration")
print("complete cells: %d/20%s%s" % (
    cells,
    ("  MISSING: " + ",".join(missing)) if missing else "",
    ("  INCOMPLETE (excluded, not silently dropped): " + ",".join(incomplete)) if incomplete else ""))
print()
print("TIER-1-ONLY FIX CYCLES (fixTier == 0, hadFix == 1)")
print("  %-6s %8s %10s %10s %10s" % ("gwGrid", "n", "meanNEES", "medNEES", "frac>95%"))
tot = []
for gw in sorted(rows_by_gw):
    r = summarise(rows_by_gw[gw])
    if r:
        n, mean, med, above = r
        print("  %-6d %8d %10.3f %10.3f %9.1f%%" % (gw, n, mean, med, 100 * above))
        tot += rows_by_gw[gw]

r = summarise(tot)
if r:
    n, mean, med, above = r
    print("  %-6s %8d %10.3f %10.3f %9.1f%%" % ("POOLED", n, mean, med, 100 * above))
print()

print("ALL-TIER FIX CYCLES (P3 reconciliation against the baseline's 2.95)")
alltot = []
for gw in sorted(rows_all):
    alltot += rows_all[gw]
r = summarise(alltot)
if r:
    n, mean, med, above = r
    print("  pooled n=%d  mean %.3f  median %.3f  above 95%% bound %.1f%%" %
          (n, mean, med, 100 * above))
print()

if not tot:
    sys.exit("no tier-1-only fix cycles pooled -- abort condition")
n1 = len(tot)
print("VERDICT")
if n1 < 200:
    print("  ABORT: only %d tier-1-only fix cycles pooled, below the pre-registered" % n1)
    print("  minimum of 200. Too thin to act on; reporting the count and stopping.")
else:
    m = sum(tot) / n1
    if m < 2.0:
        print("  P1 HOLDS: pooled tier-1 NEES %.3f < 2.0." % m)
        print("  The filter's claimed uncertainty EXCEEDS its realised error, i.e. tier-1")
        print("  sigma is OVER-STATED by roughly sqrt(2/%.3f) = %.2fx in sigma terms."
              % (m, (2.0 / m) ** 0.5))
    else:
        print("  P1 FALSIFIED: pooled tier-1 NEES %.3f >= 2.0." % m)
        print("  Tier-1 sigma is NOT over-stated. A4's premise is wrong, no recalibration")
        print("  is applied, and A5 proceeds without a calibration fix.")
    signs = []
    for gw in sorted(rows_by_gw):
        rr = summarise(rows_by_gw[gw])
        if rr:
            signs.append(1 if rr[1] >= 2.0 else -1)
    print("  P2 %s: sign of (NEES - 2) across densities = %s" %
          ("HOLDS" if len(set(signs)) == 1 else "FALSIFIED", signs))
