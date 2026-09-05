#!/usr/bin/env python3
"""A0 — apply the registered joint availability/energy criterion (M2) to the
regenerated 320-cell grid. Criterion taken verbatim from
report/PREREG_A0_JOINT_CRITERION.md section 2, committed 6d44895 BEFORE the grid ran.

    dD(c)    = D_T(c) - D_B(c)                    deployments met, over N paired seeds
    rho(c)   = geometric mean over seeds of totalJ_T / totalJ_B
    allow(c) = min(2.0, 1 + kappa * dD(c)/N)      kappa = 1.0 is the registered verdict

    PASS          dD > 0 AND the bootstrap CI for rho lies ENTIRELY BELOW allow
    FAIL          dD <= 0, OR the CI lies entirely ABOVE allow
    INCONCLUSIVE  the CI straddles allow

A deployment is one seed and it MEETS the requirement when A >= 0.95 and R <= 25 m,
with A = fixCycles/steps per Architecture.md section 1.3 -- every decision cycle
counts and a cycle with no fix counts against.

Geometric mean of PAIRED per-seed ratios with a seed-level bootstrap, as registered:
a ratio's sampling distribution is skewed and an arithmetic mean of raw ratios is not
the estimator to quote.

Section 4 requires the kappa sensitivity band {0.25, 0.5, 1.0, 2.0} to be reported
ALWAYS, in the same table, so any dependence on kappa is visible rather than
discoverable later. A cell that flips is reported as flipping.
"""
import csv
import glob
import math
import os
import random
import sys

ROOT = "results/p10_grid"
TIERED = "ca_tbl_full"
BASE = "static_2p4"
A_MIN, R_MAX = 0.95, 25.0
KAPPAS = [0.25, 0.5, 1.0, 2.0]
NBOOT = 10000
random.seed(20260905)

EFN = {0: "RURAL_OPEN", 1: "RURAL_FOREST", 3: "URBAN_MOD", 4: "URBAN_DENSE"}


def cell_rows(d):
    f = os.path.join(d, "arm_summary.csv")
    if not os.path.isfile(f):
        return {}
    return {r["arm"]: r for r in csv.DictReader(open(f))}


def meets(r):
    steps = float(r["steps"])
    if steps <= 0:
        return False
    return (float(r["fixCycles"]) / steps >= A_MIN) and (float(r["cep50M"]) <= R_MAX)


def gci(ratios):
    """Geometric mean with a seed-level bootstrap 95% CI."""
    logs = [math.log(x) for x in ratios if x > 0]
    if not logs:
        return float("nan"), float("nan"), float("nan")
    g = math.exp(sum(logs) / len(logs))
    boot = []
    n = len(logs)
    for _ in range(NBOOT):
        s = [logs[random.randrange(n)] for _ in range(n)]
        boot.append(math.exp(sum(s) / n))
    boot.sort()
    return g, boot[int(0.025 * NBOOT)], boot[int(0.975 * NBOOT)]


def verdict(dD, lo, hi, allow):
    if dD <= 0:
        return "FAIL"
    if hi < allow:
        return "PASS"
    if lo > allow:
        return "FAIL"
    return "INCONC"


if not os.path.isdir(ROOT):
    sys.exit("missing %s" % ROOT)

configs = sorted(set(os.path.dirname(p) for p in
                     glob.glob(os.path.join(ROOT, "c_*", "seed_*"))))

print("A0 — registered joint criterion M2 applied to the regenerated grid")
print("tiered = %s   baseline = %s   requirement A >= %.2f and R <= %.0f m"
      % (TIERED, BASE, A_MIN, R_MAX))
print("=" * 100)
print("%-38s %3s %3s %4s %7s %-18s %s"
      % ("configuration", "D_T", "D_B", "dD", "rho", "rho 95% CI", "verdict by kappa .25/.5/1.0/2.0"))

tally = {}
flips = []
passes = []

for c in configs:
    seeds = sorted(glob.glob(os.path.join(c, "seed_*")))
    dT = dB = 0
    ratios = []
    for s in seeds:
        rows = cell_rows(s)
        if TIERED not in rows or BASE not in rows:
            continue
        dT += 1 if meets(rows[TIERED]) else 0
        dB += 1 if meets(rows[BASE]) else 0
        jb = float(rows[BASE]["totalJ"])
        if jb > 0:
            ratios.append(float(rows[TIERED]["totalJ"]) / jb)
    n = len(ratios)
    if n == 0:
        continue
    dD = dT - dB
    g, lo, hi = gci(ratios)

    vs = []
    for k in KAPPAS:
        allow = min(2.0, 1.0 + k * dD / n)
        vs.append(verdict(dD, lo, hi, allow))
    name = os.path.basename(c)
    print("%-38s %3d %3d %4d %7.3f [%7.3f,%7.3f] %s"
          % (name, dT, dB, dD, g, lo, hi, " ".join("%-6s" % v for v in vs)))

    reg = vs[KAPPAS.index(1.0)]
    tally[reg] = tally.get(reg, 0) + 1
    if len(set(vs)) > 1:
        flips.append((name, vs))
    if reg == "PASS":
        passes.append((name, dD, g, lo, hi))

print("=" * 100)
print("REGISTERED VERDICT (kappa = 1.0) over %d configurations:" % len(configs))
for k in ("PASS", "FAIL", "INCONC"):
    print("   %-8s %d" % (k, tally.get(k, 0)))

print("\nPASS region (the thing M1 could not return a verdict on):")
if passes:
    for nm, dD, g, lo, hi in passes:
        print("   %-38s dD=%+d  rho=%.3f [%.3f,%.3f]" % (nm, dD, g, lo, hi))
else:
    print("   none")

print("\nCells whose verdict FLIPS across the kappa band (reported, per section 4):")
if flips:
    for nm, vs in flips:
        print("   %-38s %s" % (nm, " ".join(vs)))
else:
    print("   none -- every verdict is robust across kappa 0.25 to 2.0")
