#!/usr/bin/env python3
"""Per-arm and PAIRED cross-arm statistics over a multi-seed sweep.

    python3 scripts/sweeps/paired_stats.py results/sweep20 [more dirs...]

Seeds are a blocking factor: the same channel realisation drives every arm within
a seed, so the two arms in a comparison are matched, not independent. Independent
two-sample means would therefore overstate the variance of the DIFFERENCE and
understate significance. Everything below the per-arm table is paired.

The claim ratios are reported as the mean of the per-seed log ratio, exponentiated
(the geometric mean). acc/J is a ratio of products, so its sampling distribution is
skewed and an arithmetic mean of raw ratios is not the estimator to quote.

No scipy: the t critical values are hard-coded for the df actually used.
"""

import csv
import glob
import math
import os
import sys

# two-sided 95% t critical values, df -> t
T95 = {1: 12.706, 2: 4.303, 3: 3.182, 4: 2.776, 5: 2.571, 6: 2.447, 7: 2.365,
       8: 2.306, 9: 2.262, 10: 2.228, 11: 2.201, 12: 2.179, 13: 2.160,
       14: 2.145, 15: 2.131, 16: 2.120, 17: 2.110, 18: 2.101, 19: 2.093,
       20: 2.086, 24: 2.064, 29: 2.045, 30: 2.042}


def tcrit(df):
    if df in T95:
        return T95[df]
    return min((abs(k - df), v) for k, v in T95.items())[1]


def mean(xs):
    return sum(xs) / len(xs)


def sd(xs):
    if len(xs) < 2:
        return float("nan")
    m = mean(xs)
    return math.sqrt(sum((x - m) ** 2 for x in xs) / (len(xs) - 1))


def ci95(xs):
    n = len(xs)
    if n < 2:
        return (float("nan"), float("nan"))
    h = tcrit(n - 1) * sd(xs) / math.sqrt(n)
    return (mean(xs) - h, mean(xs) + h)


def accpj(r):
    rm, j = float(r["rmseM"]), float(r["totalJ"])
    return 1.0 / (rm * j) if rm > 0 and j > 0 else float("nan")


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


def main():
    by_seed = load(sys.argv[1:])
    seeds = sorted(by_seed)
    if not seeds:
        print("no data")
        return

    arms = ["static_868", "static_2p4", "static_uwb", "fixed_threshold",
            "ca_tbl_no_integrity", "ca_tbl_full"]
    arms = [a for a in arms if all(a in by_seed[s] for s in seeds)]

    print(f"{len(seeds)} seeds: {seeds[0]}..{seeds[-1]}"
          f"{'' if len(seeds) == seeds[-1] - seeds[0] + 1 else ' (GAPS)'}\n")

    print("=== per-arm, across seeds (mean +/- SD [95% CI of the mean]) ===")
    hdr = f"{'arm':<22} {'rmseM':>26} {'totalJ':>26} {'acc/J':>28}"
    print(hdr)
    print("-" * len(hdr))
    for a in arms:
        rm = [float(by_seed[s][a]["rmseM"]) for s in seeds]
        tj = [float(by_seed[s][a]["totalJ"]) for s in seeds]
        ap = [accpj(by_seed[s][a]) for s in seeds]
        lo1, hi1 = ci95(rm)
        lo2, hi2 = ci95(tj)
        lo3, hi3 = ci95(ap)
        print(f"{a:<22} {mean(rm):8.3f}+-{sd(rm):6.3f}[{lo1:6.2f},{hi1:7.2f}]"
              f" {mean(tj):8.2f}+-{sd(tj):6.2f}[{lo2:6.1f},{hi2:7.1f}]"
              f" {mean(ap):9.3e}+-{sd(ap):8.2e}")

    print("\n=== small-n stability check: rmseFixedM ===")
    print(f"{'arm':<22} {'mean':>10} {'SD':>10} {'SD/mean':>10} {'min':>10} {'max':>10}")
    for a in arms:
        v = [float(by_seed[s][a]["rmseFixedM"]) for s in seeds]
        print(f"{a:<22} {mean(v):10.3f} {sd(v):10.3f} {sd(v)/mean(v):10.1%}"
              f" {min(v):10.3f} {max(v):10.3f}")

    print("\n=== PAIRED comparisons (same seed, two arms) ===")
    pairs = [("ca_tbl_full", "static_2p4", "CLAIM 1: hybrid > best single band"),
             ("ca_tbl_full", "fixed_threshold", "CLAIM 2: policy > naive threshold"),
             ("ca_tbl_full", "ca_tbl_no_integrity", "INTEGRITY ABLATION")]
    for a, b, label in pairs:
        if a not in arms or b not in arms:
            continue
        print(f"\n-- {label}\n   {a} vs {b}, n={len(seeds)}")
        for metric, fn in (("rmseM", lambda r: float(r["rmseM"])),
                           ("totalJ", lambda r: float(r["totalJ"])),
                           ("acc/J", accpj)):
            d = [fn(by_seed[s][a]) - fn(by_seed[s][b]) for s in seeds]
            lo, hi = ci95(d)
            # NaN compares false against everything, so an n=1 CI must not be
            # allowed to fall through to "significant".
            if lo != lo or hi != hi:
                sig = "  (n<2: no CI)"
            elif lo <= 0 <= hi:
                sig = ""
            else:
                sig = "  SIGNIFICANT at 95%"
            print(f"   paired diff {metric:>7}: {mean(d):+12.5g} "
                  f"[{lo:+.5g}, {hi:+.5g}]{sig}")

        # geometric mean of the per-seed acc/J ratio
        ratios = [accpj(by_seed[s][a]) / accpj(by_seed[s][b]) for s in seeds]
        if all(r > 0 for r in ratios):
            lg = [math.log(r) for r in ratios]
            lo, hi = ci95(lg)
            print(f"   acc/J ratio (geometric mean): {math.exp(mean(lg)):.4f} "
                  f"[{math.exp(lo):.4f}, {math.exp(hi):.4f}]"
                  f"   -> {'PASSES' if math.exp(lo) > 1 else 'FAILS'} "
                  f"(1/{1/math.exp(mean(lg)):.1f}x)")

        wins = sum(1 for s in seeds if accpj(by_seed[s][a]) > accpj(by_seed[s][b]))
        print(f"   seeds where {a} wins on acc/J: {wins}/{len(seeds)}")

    print("\n=== integrity check firing rate (the central claim) ===")
    for a in arms:
        tot = sum(int(by_seed[s][a]["suspects"]) for s in seeds)
        print(f"   {a:<22} suspects over all seeds: {tot}")


if __name__ == "__main__":
    main()
