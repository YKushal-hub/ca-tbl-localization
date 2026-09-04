#!/usr/bin/env python3
"""p10_reliability: does the section 4.1 result reproduce on this tree?

THE CLAIM UNDER TEST (CA-TBL_Technical_Report_SelfContained.md section 4.1, via
MASTER_PLAN section 7.1): at RURAL_FOREST, dwell=800, boxHalf=2000, ca_tbl_full
meets the requirement in 19 of 20 deployments and static_2p4 in 0 of 20, at 1.56x
the energy.

A DEPLOYMENT is one seed. It MEETS THE REQUIREMENT when both hold:
    availability A = fixCycles / steps >= 0.95
    accuracy     R = cep50M            <= 25.0 m
Both halves are stated in Architecture.md section 1.3, whose denominator note is
explicit: all decision cycles of the run count, and a cycle with no fix counts
against. fixCycles/steps is that denominator, taken from the summary rather than
recounted from traces.

WHAT THIS CANNOT ESTABLISH. nlosSource is not recorded in section 4.1, so both
legacy and propagation are run and reported separately. Whichever reproduces the
published counts identifies what the original campaign used -- that is a finding
about the original campaign's provenance, not a free parameter to select on.
"""
import csv
import glob
import os
import sys

ROOT = "results/p10_reliability"
A_MIN = 0.95
R_MAX = 25.0

if not os.path.isdir(ROOT):
    sys.exit("missing %s" % ROOT)

for ns in ("legacy", "propagation"):
    cells = sorted(glob.glob(os.path.join(ROOT, "c_ef1_%s_dw800_b2000" % ns, "seed_*")))
    if not cells:
        print("\n=== nlosSource=%s: NO CELLS ===" % ns)
        continue

    per_arm = {}
    for c in cells:
        f = os.path.join(c, "arm_summary.csv")
        if not os.path.isfile(f):
            continue
        with open(f) as fh:
            for row in csv.DictReader(fh):
                arm = row["arm"]
                steps = float(row["steps"])
                if steps <= 0:
                    continue
                a = float(row["fixCycles"]) / steps
                r = float(row["cep50M"])
                meets = (a >= A_MIN) and (r <= R_MAX)
                d = per_arm.setdefault(arm, {"n": 0, "meet": 0, "J": [], "A": [], "R": []})
                d["n"] += 1
                d["meet"] += 1 if meets else 0
                d["J"].append(float(row["totalJ"]))
                d["A"].append(a)
                d["R"].append(r)

    print("\n=== nlosSource=%s   requirement: A >= %.2f and R <= %.0f m ===" % (ns, A_MIN, R_MAX))
    print("  %-22s %8s %10s %10s %10s" % ("arm", "meets", "medA", "medR_m", "meanJ"))

    def med(v):
        v = sorted(v)
        n = len(v)
        return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])

    base = per_arm.get("static_2p4")
    for arm in sorted(per_arm):
        d = per_arm[arm]
        print("  %-22s %4d/%-3d %10.4f %10.2f %10.2f"
              % (arm, d["meet"], d["n"], med(d["A"]), med(d["R"]),
                 sum(d["J"]) / len(d["J"])))

    full = per_arm.get("ca_tbl_full")
    if full and base and base["J"]:
        ratio = (sum(full["J"]) / len(full["J"])) / (sum(base["J"]) / len(base["J"]))
        print("  energy ca_tbl_full / static_2p4 = %.3fx   (report says 1.56x)" % ratio)
        print("  report says ca_tbl_full 19/20 and static_2p4 0/20; measured here "
              "%d/%d and %d/%d" % (full["meet"], full["n"], base["meet"], base["n"]))
