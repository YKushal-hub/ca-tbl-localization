#!/usr/bin/env python3
"""K5 -- is the suspect rate an under-grown covariance? Procedure fixed by PREREG_K5 s7."""
import sys

sys.path.insert(0, "/tmp")
from k1_analyse import sign_test, median, bootstrap_ci, load_cell  # noqa: E402

ctrl, n = load_cell("results/k1a_box1500")
if n:
    sys.exit("control: %s" % n)
CELLS = [("/tmp/k5_q2", "Q x2 in std (x4 in variance)"),
         ("/tmp/k5_q4", "Q x4 in std (x16 in variance)")]
FIELDS = ("suspects", "rmseM", "totalJ", "escalations", "fixCycles")

print("K5 -- PROCESS NOISE AGAINST THE SUSPECT RATE")
print("control k1a_box1500 (Q = 1.0 / 0.5), subject ca_tbl_full, paired by seed")
print("=" * 92)
arm = "ca_tbl_full"
cr = ctrl[arm]
print("  control      suspects %6.0f of 1440 (%.1f%%)   rmse %8.3f  energy %7.3f  esc %5.0f"
      % (median([float(r["suspects"]) for r in cr.values()]),
         100.0 * median([float(r["suspects"]) for r in cr.values()]) / 1440.0,
         median([float(r["rmseM"]) for r in cr.values()]),
         median([float(r["totalJ"]) for r in cr.values()]),
         median([float(r["escalations"]) for r in cr.values()])))
print("  reference    ca_tbl_no_integrity rmse %8.3f  energy %7.3f  (suspects 0 by construction)"
      % (median([float(r["rmseM"]) for r in ctrl["ca_tbl_no_integrity"].values()]),
         median([float(r["totalJ"]) for r in ctrl["ca_tbl_no_integrity"].values()])))

for path, label in CELLS:
    cell, note = load_cell(path)
    if note:
        print("\n### %s -- %s" % (path, note)); continue
    print("\n### %s   (%s)" % (path, label))
    seeds = sorted(set(ctrl[arm]) & set(cell[arm]))
    for f in FIELDS:
        cv = [float(cell[arm][s][f]) for s in seeds]
        kv = [float(ctrl[arm][s][f]) for s in seeds]
        d = [c - k for c, k in zip(cv, kv)]
        pos, neg, p = sign_test(d)
        lo, hi = bootstrap_ci(d)
        extra = ""
        if f == "suspects":
            extra = "   -> %.1f%% of cycles" % (100.0 * median(cv) / 1440.0)
        print("  %-12s control %9.3f   cell %9.3f   d %+9.3f   %2d+/%2d- p%.4f  "
              "CI [%+.2f, %+.2f]%s"
              % (f, median(kv), median(cv), median(d), pos, neg, p, lo, hi, extra))
    print("  -- noise floor: arms the flag cannot reach --")
    for a in ("static_868", "static_2p4"):
        sd = sorted(set(ctrl[a]) & set(cell[a]))
        d = [float(cell[a][s]["rmseM"]) - float(ctrl[a][s]["rmseM"]) for s in sd]
        pos, neg, p = sign_test(d)
        print("     %-12s median |drift| %8.4f m   %2d+/%2d- p%.4f"
              % (a, abs(median(d)), pos, neg, p))
