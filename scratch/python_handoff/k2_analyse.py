#!/usr/bin/env python3
"""K2 -- which part of the integrity check costs accuracy and energy.

Procedure fixed by report/PREREG_K2_INTEGRITY_COST.md sec 6 before the cells ran.
Control is k1a_box1500: identical flags but for the one under test, same seeds.
"""
import sys

sys.path.insert(0, "/tmp")
from k1_analyse import sign_test, median, bootstrap_ci, load_cell  # noqa: E402

CONTROL = "results/k1a_box1500"
CELLS = [
    ("results/k2_nocorr_box1500", "--corroborationCheck=false"),
    ("results/k2_nosusesc_box1500", "--suspectForcesEscalation=false"),
    ("results/k2_both_box1500", "both off"),
]
FIELDS = [("rmseM", "m"), ("totalJ", "J"), ("escalations", "n"),
          ("fixCycles", "n"), ("suspects", "n")]

ctrl, note = load_cell(CONTROL)
if note:
    sys.exit("control %s -- %s" % (CONTROL, note))

print("K2 -- WHICH PART OF THE INTEGRITY CHECK COSTS")
print("control %s   subject ca_tbl_full   paired by seed" % CONTROL)
print("positive difference = the K2 cell is BETTER (less rmse, less energy,")
print("fewer escalations, MORE fix cycles, fewer suspects)")
print("=" * 96)

# The reference points the ablation is trying to reach.
for arm in ("ca_tbl_full", "ca_tbl_no_integrity"):
    rows = ctrl[arm]
    print("  %-22s rmse %8.3f  energy %7.3f  esc %5.0f  fix %6.0f  susp %6.0f" % (
        arm + " (control)",
        median([float(r["rmseM"]) for r in rows.values()]),
        median([float(r["totalJ"]) for r in rows.values()]),
        median([float(r["escalations"]) for r in rows.values()]),
        median([float(r["fixCycles"]) for r in rows.values()]),
        median([float(r["suspects"]) for r in rows.values()])))

for path, label in CELLS:
    cell, note = load_cell(path)
    if note:
        print("\n### %s -- %s" % (path, note))
        continue
    print("\n### %s   (%s)" % (path, label))
    for arm in ("ca_tbl_full",):
        seeds = sorted(set(ctrl[arm]) & set(cell[arm]))
        for field, unit in FIELDS:
            cv = [float(cell[arm][s][field]) for s in seeds]
            kv = [float(ctrl[arm][s][field]) for s in seeds]
            # fixCycles: more is better, so flip the sense.
            if field == "fixCycles":
                d = [c - k for c, k in zip(cv, kv)]
            else:
                d = [k - c for c, k in zip(cv, kv)]
            pos, neg, p = sign_test(d)
            lo, hi = bootstrap_ci(d)
            print("  %-13s %-2s control %9.3f  cell %9.3f  d %+9.3f  "
                  "%2d+/%2d- p%.4f  CI [%+.2f, %+.2f]" %
                  (field, unit, median(kv), median(cv), median(d),
                   pos, neg, p, lo, hi))
    # Noise floor: these arms carry no policy, so any movement is RNG displacement.
    print("  -- noise floor (arms the flags cannot reach) --")
    for arm in ("static_868", "static_2p4"):
        seeds = sorted(set(ctrl[arm]) & set(cell[arm]))
        d = [float(cell[arm][s]["rmseM"]) - float(ctrl[arm][s]["rmseM"])
             for s in seeds]
        pos, neg, p = sign_test(d)
        print("     %-12s median |drift| %8.4f m   %2d+/%2d- p%.4f" %
              (arm, abs(median(d)), pos, neg, p))
