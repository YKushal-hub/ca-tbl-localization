#!/usr/bin/env python3
"""Crediting-invariant marginal accuracy-per-joule, above the shared tier-1 floor.

    python3 scripts/sweeps/marginal_mpj.py results/sweep20_v6_b1000/seed_*

WHY THIS EXISTS. accuracy-per-joule = 1/(rmse x J) answers "which arm is best
overall", and its verdict moves depending on whether the baseline is credited
with a demotion rule it does not have (report SS1.3 finding 2: 1.65x one way,
0.70x the other). That crediting choice is a judgement call, and a headline
number that flips on a judgement call is not a result.

The marginal measure removes the choice. Every hybrid arm sits on the same
tier-1 floor -- static_868, same seed, same channel realisation. So ask what
each arm bought with the energy it spent ABOVE that floor:

    marginal m/J = (rmse_868 - rmse_arm) / (J_arm - J_868)

metres of error removed per extra joule. Both arms are debited the identical
floor, so no crediting convention can favour either. Report SS1.3 finding 3
measured fixed_threshold ahead 1.51x [1.37, 1.66] on this statistic.

ESTIMATOR. Seeds are the blocking factor: the same channel drives every arm
within a seed. The ratio is therefore formed per seed and aggregated as the
geometric mean (mean of per-seed log ratios, exponentiated), with a t-based
95% CI on the log scale -- the same estimator paired_stats.py uses for acc/J,
and for the same reason: a ratio's sampling distribution is skewed and an
arithmetic mean of raw ratios is not the estimator to quote.

Seeds where the denominator is <= 0 (an arm that spent nothing above the floor)
are excluded and COUNTED IN THE OUTPUT rather than dropped silently -- a
marginal efficiency is undefined, not infinite, when nothing was spent.
"""

import csv
import glob
import math
import os
import sys

T95 = {1: 12.706, 2: 4.303, 3: 3.182, 4: 2.776, 5: 2.571, 6: 2.447, 7: 2.365,
       8: 2.306, 9: 2.262, 10: 2.228, 11: 2.201, 12: 2.179, 13: 2.160,
       14: 2.145, 15: 2.131, 16: 2.120, 17: 2.110, 18: 2.101, 19: 2.093,
       20: 2.086, 24: 2.064, 29: 2.045, 30: 2.042}


def tcrit(df):
    return T95.get(df, min((abs(k - df), v) for k, v in T95.items())[1])


def load(dirs):
    by_seed = {}
    for d in dirs:
        for p in glob.glob(os.path.join(d, "arm_summary.csv")):
            if os.path.getsize(p) == 0:
                continue
            with open(p, newline="") as f:
                for r in csv.DictReader(f):
                    by_seed.setdefault(int(r["seed"]), {})[r["arm"]] = r
    return by_seed


def marginal(row, floor):
    """metres of RMSE removed per joule spent above the tier-1 floor."""
    dj = float(row["totalJ"]) - float(floor["totalJ"])
    dm = float(floor["rmseM"]) - float(row["rmseM"])
    if dj <= 0.0:
        return None
    return dm / dj


def main():
    by_seed = load(sys.argv[1:])
    seeds = sorted(by_seed)
    if not seeds:
        print("no data")
        return

    arms = ["fixed_threshold", "ca_tbl_full", "ca_tbl_no_integrity"]
    print(f"{len(seeds)} seeds: {seeds[0]}..{seeds[-1]}")
    print("floor = static_868, same seed\n")

    print(f"{'arm':<22} {'marginal m/J: mean':>22} {'SD':>10} {'n':>4} {'undefined':>10}")
    print("-" * 72)
    vals = {}
    for a in arms:
        xs = []
        undef = 0
        for s in seeds:
            if a not in by_seed[s] or "static_868" not in by_seed[s]:
                continue
            v = marginal(by_seed[s][a], by_seed[s]["static_868"])
            if v is None:
                undef += 1
            else:
                xs.append(v)
        vals[a] = xs
        if xs:
            m = sum(xs) / len(xs)
            sd = math.sqrt(sum((x - m) ** 2 for x in xs) / (len(xs) - 1)) if len(xs) > 1 else float("nan")
            print(f"{a:<22} {m:>22.4f} {sd:>10.4f} {len(xs):>4} {undef:>10}")
        else:
            print(f"{a:<22} {'--':>22} {'--':>10} {0:>4} {undef:>10}")

    # Paired ratio, per seed, geometric mean.
    print("\n=== PAIRED ratio, fixed_threshold / ca_tbl_full (>1 means the flat "
          "threshold converts joules into accuracy more efficiently) ===")
    logs = []
    skipped = 0
    for s in seeds:
        d = by_seed[s]
        if not all(k in d for k in ("fixed_threshold", "ca_tbl_full", "static_868")):
            continue
        a = marginal(d["fixed_threshold"], d["static_868"])
        b = marginal(d["ca_tbl_full"], d["static_868"])
        if a is None or b is None or a <= 0 or b <= 0:
            skipped += 1
            continue
        logs.append(math.log(a / b))
    if not logs:
        print("  no usable seeds")
        return
    n = len(logs)
    m = sum(logs) / n
    sd = math.sqrt(sum((x - m) ** 2 for x in logs) / (n - 1)) if n > 1 else float("nan")
    h = tcrit(n - 1) * sd / math.sqrt(n) if n > 1 else float("nan")
    won = sum(1 for x in logs if x < 0)
    print(f"  geometric mean ratio: {math.exp(m):.4f} "
          f"[{math.exp(m - h):.4f}, {math.exp(m + h):.4f}]   n={n}"
          + (f"  ({skipped} seeds undefined)" if skipped else ""))
    print(f"  seeds where ca_tbl_full converts joules better: {won}/{n}")


if __name__ == "__main__":
    main()
