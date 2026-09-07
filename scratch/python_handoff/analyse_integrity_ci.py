#!/usr/bin/env python3
"""Bootstrap CI on the paired integrity-check result (report/INTEGRITY_CHECK_RESULTS.md).

That report states sign consistency across 8 paired seeds and says, in its own words,
that "a bootstrap CI on the paired difference is the next step". This is that step and
nothing more: it adds an interval to differences already measured, and it does not
widen the configuration the differences came from.

PAIRED BY SEED. Each seed is one trajectory; off and on are the same trajectory under
two flag settings, so the seed is the unit and the difference is the observation.
Resampling seeds -- not runs -- is what the pairing requires.

COMPLETENESS IS CHECKED BEFORE ANYTHING IS POOLED (rule 12): every cell must exist and
carry exactly one data row, and the seed sets must match. A missing cell silently
dropped would shrink n and narrow the interval, which is the failure this guards.
"""
import csv, os, sys, random  # ROOT: pass the campaign directory as argv[1]

ROOT = (sys.argv[1] if len(sys.argv) > 1 else
        os.path.expanduser("~/catbl_work/cc"))
SEEDS = list(range(1, 9))
B = 20000
random.seed(20260908)  # fixed, so the interval is reproducible


def load(tag, seed):
    p = os.path.join(ROOT, "%s_s%d" % (tag, seed), "arm_summary.csv")
    if not os.path.exists(p):
        return None, "missing %s" % p
    rows = list(csv.DictReader(open(p)))
    if len(rows) != 1:
        return None, "%s has %d rows, expected exactly 1" % (p, len(rows))
    return rows[0], None


def pctile(xs, q):
    s = sorted(xs)
    if not s:
        return float("nan")
    k = (len(s) - 1) * q
    lo, hi = int(k), min(int(k) + 1, len(s) - 1)
    return s[lo] + (s[hi] - s[lo]) * (k - lo)


def boot(diffs):
    n = len(diffs)
    means = []
    for _ in range(B):
        means.append(sum(random.choice(diffs) for _ in range(n)) / float(n))
    return pctile(means, 0.025), pctile(means, 0.975)


def sign_test_p(diffs):
    """Exact one-sided binomial p for 'improvements are not chance', ties excluded."""
    k = sum(1 for d in diffs if d < 0)
    m = sum(1 for d in diffs if d != 0)
    if m == 0:
        return float("nan"), 0, 0
    from math import comb
    p = sum(comb(m, i) for i in range(k, m + 1)) / float(2 ** m)
    return p, k, m


def main():
    errs, pairs = [], []
    for s in SEEDS:
        off, e1 = load("off", s)
        on, e2 = load("on", s)
        for e in (e1, e2):
            if e:
                errs.append(e)
        if off and on:
            pairs.append((s, off, on))
    if errs:
        print("REFUSING TO POOL -- completeness failed:")
        for e in errs:
            print("  " + e)
        return 2
    if len(pairs) != len(SEEDS):
        print("REFUSING TO POOL -- %d of %d seed pairs" % (len(pairs), len(SEEDS)))
        return 2

    print("Paired integrity-check result -- %d seeds, complete\n" % len(pairs))
    for metric, label, unit in (("rmseM", "accuracy (rmse)", "m"),
                                ("totalJ", "energy", "J"),
                                ("escalations", "escalations", "")):
        diffs = []
        print("  %s" % label)
        for s, off, on in pairs:
            a, b = float(off[metric]), float(on[metric])
            diffs.append(b - a)
            print("    seed %d  %8.3f -> %8.3f   delta %+8.3f %s" % (s, a, b, b - a, unit))
        n = len(diffs)
        mean = sum(diffs) / n
        var = sum((d - mean) ** 2 for d in diffs) / (n - 1) if n > 1 else 0.0
        lo, hi = boot(diffs)
        p, k, m = sign_test_p(diffs)
        print("    mean delta %+.3f %s (sd %.3f)" % (mean, unit, var ** 0.5))
        print("    95%% percentile bootstrap CI [%+.3f, %+.3f] %s  (B=%d, paired by seed)"
              % (lo, hi, unit, B))
        print("    improved in %d of %d seeds, exact one-sided sign test p = %.4f" % (k, m, p))
        print("    CI EXCLUDES ZERO" if hi < 0 or lo > 0 else "    CI INCLUDES ZERO")
        print()
    print("n = 8. A percentile bootstrap at this n is an interval, not a guarantee: it")
    print("resamples the eight differences observed and cannot speak to configurations")
    print("not run. One duration, one box size, one finder count, one mobility model.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
