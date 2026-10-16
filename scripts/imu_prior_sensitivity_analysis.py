#!/usr/bin/env python3
"""Score the registered bias sensitivity (report/PREREG_IMU_PRIOR.md §8).

For each residual turn-on bias b in {0.05, 0.15 (primary), 0.5} deg/s, arms static_2p4
and ca_tbl_full, C_sparse, seeds 301-310: M1 and M2 exactly as the primary scorer defines
them, each IMU cell paired with the primary campaign's sparse_cv cell of the same seed.
No verdict is registered; this prints the curve.

Usage: imu_prior_sensitivity_analysis.py <primary_root> <sens_root> <outcsv>
"""

import csv
import math
import os
import random
import statistics
import sys

ARMS = ["static_2p4", "ca_tbl_full"]
SEEDS = list(range(301, 311))
BIASES = ["0.05", "0.15", "0.5"]
R_REQ, DEEP_K, MIN_CYCLES, NBOOT = 25.0, 10, 5, 10000


def trace(path):
    nofix, deep, run = [], [], 0
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            if r["hadFix"] == "1":
                run = 0
            else:
                run += 1
                e = float(r["errM"])
                nofix.append(e)
                if run >= DEEP_K:
                    deep.append(e)
    return {"nofix": nofix, "deep": deep}


def gmean_ci(ratios, rng):
    logs = [math.log(r) for r in ratios]
    if not logs:
        return None, None, None
    m = sum(logs) / len(logs)
    bs = sorted(sum(logs[rng.randrange(len(logs))] for _ in logs) / len(logs)
                for _ in range(NBOOT))
    return math.exp(m), math.exp(bs[int(0.025 * NBOOT)]), math.exp(bs[int(0.975 * NBOOT) - 1])


def main(argv):
    if len(argv) != 4:
        print(__doc__)
        return 2
    prim, sens, outcsv = argv[1:]
    rng = random.Random(20261015)
    rows = []
    print("bias(dps)  arm           metric  n   gmean   95% CI")
    for b in BIASES:
        for a in ARMS:
            for key in ("nofix", "deep"):
                ratios = []
                for s in SEEDS:
                    cv = os.path.join(prim, "sparse_cv_s%d" % s, "trace_%s_seed%d.csv" % (a, s))
                    if b == "0.15":
                        im = os.path.join(prim, "sparse_imu_s%d" % s, "trace_%s_seed%d.csv" % (a, s))
                    else:
                        im = os.path.join(sens, "sparse_imu_b%s_s%d" % (b, s),
                                          "trace_%s_seed%d.csv" % (a, s))
                    if not (os.path.exists(cv) and os.path.exists(im)):
                        continue
                    c, i = trace(cv)[key], trace(im)[key]
                    if len(c) >= MIN_CYCLES and len(i) >= MIN_CYCLES:
                        mc, mi = statistics.median(c), statistics.median(i)
                        if mc > 0 and mi > 0:
                            ratios.append(mi / mc)
                g, lo, hi = gmean_ci(ratios, rng)
                rows.append({"biasDps": b, "arm": a, "metric": key, "n": len(ratios),
                             "gmean": "" if g is None else "%.6g" % g,
                             "lo": "" if lo is None else "%.6g" % lo,
                             "hi": "" if hi is None else "%.6g" % hi})
                if g is None:
                    print("%-9s  %-13s %-6s %2d  not testable" % (b, a, key, 0))
                else:
                    print("%-9s  %-13s %-6s %2d  %6.3f  [%6.3f, %6.3f]" % (b, a, key, len(ratios), g, lo, hi))
    with open(outcsv, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["biasDps", "arm", "metric", "n", "gmean", "lo", "hi"])
        w.writeheader()
        w.writerows(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
