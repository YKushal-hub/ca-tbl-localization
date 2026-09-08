#!/usr/bin/env python3
"""K1 domination analysis.

Section 6 of report/PREREG_K1_DOMINATION.md fixed this procedure before the data
existed. This implements that section and the two diagnostics Amendment 1 added, and
nothing else.

Usage:  k1_analyse.py <cell-dir> [<cell-dir> ...]
"""
import csv
import math
import os
import random
import sys

SUBJECT = "ca_tbl_full"
# The arms the headline claim is about. ca_tbl_no_integrity is an internal ablation,
# not a rival system: reported, but it does not enter the verdict.
CLAIM_ARMS = ["static_868", "static_2p4", "static_uwb", "fixed_threshold"]
ALL_COMPARATORS = CLAIM_ARMS + ["ca_tbl_no_integrity"]
EXPECT_ROWS = 120  # 20 seeds x 6 arms
ALPHA = 0.05


# --------------------------------------------------------------- statistics ---
def binom_two_sided(k, n):
    if n == 0:
        return 1.0
    pmf = lambda i: math.comb(n, i) * (0.5 ** n)
    obs = pmf(k)
    return min(1.0, sum(pmf(i) for i in range(n + 1) if pmf(i) <= obs + 1e-12))


def sign_test(diffs):
    pos = sum(1 for d in diffs if d > 0)
    neg = sum(1 for d in diffs if d < 0)
    return pos, neg, binom_two_sided(pos, pos + neg)


def median(xs):
    s = sorted(xs)
    n = len(s)
    if n == 0:
        return float("nan")
    return s[n // 2] if n % 2 else 0.5 * (s[n // 2 - 1] + s[n // 2])


def bootstrap_ci(diffs, resamples=10000, seed=20260908):
    if not diffs:
        return (float("nan"), float("nan"))
    rng = random.Random(seed)
    n = len(diffs)
    meds = sorted(median([diffs[rng.randrange(n)] for _ in range(n)])
                  for _ in range(resamples))
    return meds[int(0.025 * resamples)], meds[int(0.975 * resamples) - 1]


# ---------------------------------------------------------------- cell load ---
def load_cell(path):
    """(rows_by_arm, exclusion_note). Abort conditions A2/A3 are checked HERE, before
    any statistic exists, so an inadmissible cell cannot reach the analysis."""
    csv_path = os.path.join(path, "arm_summary.csv")
    if not os.path.exists(csv_path):
        return None, "no arm_summary.csv"
    rows = list(csv.DictReader(open(csv_path)))

    # A2 -- count rows. A --seedCount summary is appended per seed, so it is non-empty
    # from seed 1 and non-emptiness proves nothing (rule 12).
    if len(rows) != EXPECT_ROWS:
        return None, "EXCLUDED: %d rows, expected %d" % (len(rows), EXPECT_ROWS)

    # A3 -- dirty 0 or it is not citable.
    dirty = None
    sidecar = os.path.join(path, "run_id.txt")
    if os.path.exists(sidecar):
        for line in open(sidecar):
            if " dirty " in line:
                try:
                    dirty = int(line.split(" dirty ")[1].split()[0])
                except (IndexError, ValueError):
                    pass
                break
    if dirty is None:
        return None, "EXCLUDED: sidecar carries no dirty count"
    if dirty != 0:
        return None, "EXCLUDED: sidecar reports dirty %d" % dirty

    by_arm = {}
    for r in rows:
        by_arm.setdefault(r["arm"], {})[int(r["seed"])] = r
    return by_arm, None


def paired(by_arm, comparator, field, subject=SUBJECT):
    """Comparator minus subject, per seed. Positive = the SUBJECT is better, on both
    axes, since rmseM and totalJ are both quantities where less is better."""
    a, b = by_arm.get(subject, {}), by_arm.get(comparator, {})
    seeds = sorted(set(a) & set(b))
    return [float(b[s][field]) - float(a[s][field]) for s in seeds]


def verdict(dr, de):
    def state(diffs):
        pos, neg, p = sign_test(diffs)
        if p >= ALPHA:
            return "tie"
        return "subject" if pos > neg else "comparator"
    r, e = state(dr), state(de)
    if r == "comparator" and e == "comparator":
        return "DOMINATED"
    if r == "comparator" or e == "comparator":
        return "incomparable" if (r == "subject" or e == "subject") else "DOMINATED"
    if r == "subject" or e == "subject":
        return "DOMINATES"
    return "tie"


def scale_of(name):
    for tag in ("box", "split"):
        if tag in name:
            digits = "".join(c for c in name.split(tag)[-1] if c.isdigit())
            if digits:
                return int(digits)
    return 0


# ------------------------------------------------------------------- report ---
def main(cells):
    print("K1 DOMINATION ANALYSIS")
    print("subject %s | axes rmseM, totalJ | paired by seed, two-sided sign test"
          % SUBJECT)
    print("a positive paired difference means the SUBJECT is better on that seed")
    print("=" * 96)

    loaded, excluded = {}, []
    for path in cells:
        name = os.path.basename(path.rstrip("/\\"))
        by_arm, note = load_cell(path)
        if note:
            excluded.append((name, note))
        else:
            loaded[name] = by_arm

    verdicts = {}
    for name in sorted(loaded, key=lambda n: (n[:3], scale_of(n))):
        by_arm = loaded[name]
        print("\n### %s" % name)
        print("  %-22s %9s %9s %9s %8s %7s" %
              ("arm", "rmse(m)", "total(J)", "acc/J", "fix", "esc"))
        for arm in [SUBJECT] + ALL_COMPARATORS:
            rows = by_arm.get(arm)
            if not rows:
                continue
            print("  %-22s %9.3f %9.3f %9.6f %8.0f %7.0f" % (
                arm,
                median([float(r["rmseM"]) for r in rows.values()]),
                median([float(r["totalJ"]) for r in rows.values()]),
                median([float(r["accuracyPerJoule"]) for r in rows.values()]),
                median([float(r["fixCycles"]) for r in rows.values()]),
                median([float(r["escalations"]) for r in rows.values()])))

        print("  %-22s %-26s %-26s %s" % ("vs", "rmse (m)", "energy (J)", "verdict"))
        cv = {}
        for comp in ALL_COMPARATORS:
            if comp not in by_arm:
                continue
            dr, de = paired(by_arm, comp, "rmseM"), paired(by_arm, comp, "totalJ")
            pr, nr, ppr = sign_test(dr)
            pe, ne, ppe = sign_test(de)
            cv[comp] = verdict(dr, de)
            print("  %-22s %+9.3f %2d+/%2d- p%.4f %+9.3f %2d+/%2d- p%.4f  %s" % (
                comp, median(dr), pr, nr, ppr, median(de), pe, ne, ppe, cv[comp]))
        verdicts[name] = cv
        losers = [c for c in CLAIM_ARMS if cv.get(c) == "DOMINATED"]
        print("  --> %s" % ("NON-DOMINATED by every claim arm" if not losers
                            else "DOMINATED by " + ", ".join(losers)))

    print("\n" + "=" * 96)
    print("DOMINATION MATRIX -- how ca_tbl_full stands against each rival, per cell")
    print("%-18s %s" % ("cell", " ".join("%-15s" % c for c in CLAIM_ARMS)))
    for name in sorted(verdicts, key=lambda n: (n[:3], scale_of(n))):
        print("%-18s %s" % (name, " ".join("%-15s" % verdicts[name].get(c, "-")
                                           for c in CLAIM_ARMS)))

    if excluded:
        print("\nEXCLUDED CELLS (named, per abort conditions A2/A3):")
        for name, note in excluded:
            print("  %s -- %s" % (name, note))

    # ---- Amendment diagnostic 1: the environment-split defect, over 20 seeds ----
    if "k1e_split1000" in loaded and "k1a_box1000" in loaded:
        print("\n" + "=" * 96)
        print("THE ENVIRONMENT SPLIT, MEASURED OVER 20 PAIRED SEEDS "
              "(k1e_split1000 defective default vs k1a_box1000 unified)")
        split, uni = loaded["k1e_split1000"], loaded["k1a_box1000"]
        print("  %-22s %-12s %11s %11s %11s" %
              ("arm", "metric", "split", "unified", "median diff"))
        for arm in [SUBJECT] + ALL_COMPARATORS:
            if arm not in split or arm not in uni:
                continue
            for field in ("rmseM", "totalJ", "meanNbr_2p4", "meanNbr_868", "fixCycles"):
                seeds = sorted(set(split[arm]) & set(uni[arm]))
                if not seeds:
                    continue
                sv = [float(split[arm][s][field]) for s in seeds]
                uv = [float(uni[arm][s][field]) for s in seeds]
                d = [a - b for a, b in zip(sv, uv)]
                pos, neg, p = sign_test(d)
                print("  %-22s %-12s %11.3f %11.3f %+11.3f  %2d+/%2d- p%.4f" %
                      (arm, field, median(sv), median(uv), median(d), pos, neg, p))
            print()

    # ---- Amendment diagnostic 2: the RNG-displacement noise floor ----
    # The static arms carry no policy, so k1a and k1b should differ on them only
    # through shared-stream displacement. Whatever that difference is, it is the floor
    # under every k1a-vs-k1b claim about the policy arms.
    print("=" * 96)
    print("NOISE FLOOR -- static arms across the repaired/unrepaired cells.")
    print("These arms have no policy, so any difference is RNG displacement (rule 8),")
    print("and it bounds what a k1a-vs-k1b difference on a POLICY arm can mean.")
    for a_name in sorted([n for n in loaded if n.startswith("k1a_box")],
                         key=scale_of):
        b_name = a_name.replace("k1a_", "k1b_")
        if b_name not in loaded:
            continue
        for arm in ("static_868", "static_2p4"):
            if arm not in loaded[a_name] or arm not in loaded[b_name]:
                continue
            seeds = sorted(set(loaded[a_name][arm]) & set(loaded[b_name][arm]))
            d = [float(loaded[b_name][arm][s]["rmseM"]) -
                 float(loaded[a_name][arm][s]["rmseM"]) for s in seeds]
            pos, neg, p = sign_test(d)
            print("  %-16s %-12s median |drift| %8.4f m   %2d+/%2d- p%.4f" %
                  (a_name, arm, abs(median(d)), pos, neg, p))

    # ---- Bootstrap CIs on the two comparisons the claim rests on ----
    print("\n" + "=" * 96)
    print("BOOTSTRAP 95% CI on the median paired difference, 10 000 seed-level "
          "resamples")
    print("(positive = ca_tbl_full better)")
    for name in sorted(loaded, key=lambda n: (n[:3], scale_of(n))):
        for comp in ("static_2p4", "static_868", "fixed_threshold"):
            if comp not in loaded[name]:
                continue
            lo_r, hi_r = bootstrap_ci(paired(loaded[name], comp, "rmseM"))
            lo_e, hi_e = bootstrap_ci(paired(loaded[name], comp, "totalJ"))
            print("  %-18s vs %-16s rmse [%+8.3f, %+8.3f]  energy [%+8.3f, %+8.3f]"
                  % (name, comp, lo_r, hi_r, lo_e, hi_e))


if __name__ == "__main__":
    main(sys.argv[1:])
