#!/usr/bin/env python3
"""B3 regeneration -- the CLAUDE.md section 6 invariant, and per-cycle tier cost.
SUPPLEMENTARY, not registered.

1. INVARIANT. Exactly one tier transmits per cycle and only that tier is charged, so an
   arm accrues no ACTIVE energy on a tier it never used. A radio that is carried but
   unused sits in deep sleep and is billed sleep current (sleep_<tier>), which is the
   price of carrying it, not of using it. So the check on every regenerated row is: for
   each tier, cycles_<tier> == 0 must imply tx_, rx_, cad_ and idle_<tier> all exactly
   0.0 and j_<tier> == sleep_<tier> exactly. An exact zero is a control-flow claim
   (rule 4), so the check also reports how many rows had the tier in use -- the path that
   would have to be wrong is the one exercised.
   (A first draft of this check demanded j_<tier> == 0 and flagged every unused tier's
   sleep current as a violation. The check was wrong, not the rows; recorded so the
   corrected predicate is not mistaken for a loosened one.)

2. PER-CYCLE COST BY TIER, inside one binary (admissible under PREREG_B3_REGEN section 1:
   same binary, same seed). j_<tier> / cycles_<tier> on the single-tier arms static_868
   and static_2p4, per K1 cell, mean over 20 seeds. It says which tier is cheaper per cycle
   under the billing this binary applies (--tier1ChargeReplies defaults to true since
   4779bf7), which is what a tiering-vs-single-band energy verdict turns on.

    python3 scripts/b3/b3_energy_invariant.py              # every regenerated row
    python3 scripts/b3/b3_energy_invariant.py FILE [...]   # named files only (fixture)

Negative fixture (rule 6), outside the tree: copy one arm_summary.csv, set rx_uwb to a
non-zero value on a row whose cycles_uwb is 0, and the check must exit 1 naming that row.
"""
import csv
import glob
import os
import sys

TIERS = ["868", "2p4", "uwb"]


def all_summaries():
    if len(sys.argv) > 1:
        # explicit files: used for the negative fixture, which must be seen to fire
        for f in sys.argv[1:]:
            with open(f, newline="") as fh:
                for r in csv.DictReader(fh):
                    yield f, r
        return
    pats = ["results/regen27_integrity/*/arm_summary.csv",
            "results/regen27_pcs_forms/*/*/arm_summary.csv",
            "results/regen27_k10/*/arm_summary.csv",
            "results/regen27_k1/*/arm_summary.csv",
            "results/regen27_edge/*/arm_summary.csv",
            "results/regen27_imu/*/arm_summary.csv"]
    for p in pats:
        for f in sorted(glob.glob(p)):
            with open(f, newline="") as fh:
                for r in csv.DictReader(fh):
                    yield f, r


def main():
    n = viol = 0
    used = {t: 0 for t in TIERS}
    unused = {t: 0 for t in TIERS}
    for f, r in all_summaries():
        n += 1
        for t in TIERS:
            c = float(r["cycles_" + t])
            if c == 0:
                unused[t] += 1
                active = [k for k in ("tx_", "rx_", "cad_", "idle_") if float(r[k + t]) != 0.0]
                if active or r["j_" + t] != r["sleep_" + t]:
                    viol += 1
                    print("VIOLATION %s arm %s seed %s: cycles_%s 0 but active %s, j %s, sleep %s"
                          % (f, r["arm"], r["seed"], t, active, r["j_" + t], r["sleep_" + t]))
            else:
                used[t] += 1
    print("1. INVARIANT over %d regenerated rows: %d violation(s)" % (n, viol))
    for t in TIERS:
        print("   tier %-4s rows with the tier unused %5d (each checked: no active energy, j == sleep); rows using it %5d"
              % (t, unused[t], used[t]))

    if len(sys.argv) > 1:
        return 0 if viol == 0 else 1
    print("\n2. PER-CYCLE COST (mJ per cycle on the tier), single-tier arms, K1 cells, mean of 20 seeds")
    for cell in sorted(glob.glob("results/regen27_k1/*")):
        rows = list(csv.DictReader(open(os.path.join(cell, "arm_summary.csv"))))
        out = []
        for arm, t in (("static_868", "868"), ("static_2p4", "2p4")):
            v = [1000.0 * float(r["j_" + t]) / float(r["cycles_" + t])
                 for r in rows if r["arm"] == arm and float(r["cycles_" + t]) > 0]
            out.append("%s %.2f mJ/cycle (n=%d)" % (arm, sum(v) / len(v), len(v)))
        full = [r for r in rows if r["arm"] == "ca_tbl_full"]
        share = sum(float(r["cycles_2p4"]) for r in full) / sum(float(r["steps"]) for r in full)
        out.append("ca_tbl_full share of cycles on 2.4 GHz %.3f" % share)
        print("   %-14s %s" % (os.path.basename(cell), " | ".join(out)))
    return 0 if viol == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
