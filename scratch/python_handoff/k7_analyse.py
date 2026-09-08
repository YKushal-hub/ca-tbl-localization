#!/usr/bin/env python3
"""K7 -- does restoring the R term to the innovation covariance fix the suspect rate?
Procedure fixed by PREREG_K7_MAHAL_R.md sec 7."""
import sys

sys.path.insert(0, "/tmp")
from k1_analyse import sign_test, median, bootstrap_ci, load_cell  # noqa: E402

ctrl, n = load_cell("results/k1a_box1500")
cell, m = load_cell("/tmp/k7_mahalR")
if n or m:
    sys.exit("%s %s" % (n or "", m or ""))

arm = "ca_tbl_full"
print("K7 -- MAHALANOBIS WITH THE R TERM RESTORED")
print("control k1a_box1500 (mahalUseR=false)   cell k7_mahalR (mahalUseR=true)")
print("=" * 96)
cr, cc = ctrl[arm], cell[arm]
ni = ctrl["ca_tbl_no_integrity"]
print("  reference points:")
print("    control ca_tbl_full         suspects %6.0f (%.1f%%)  rmse %8.3f  energy %7.3f"
      % (median([float(r["suspects"]) for r in cr.values()]),
         100.0 * median([float(r["suspects"]) for r in cr.values()]) / 1440.0,
         median([float(r["rmseM"]) for r in cr.values()]),
         median([float(r["totalJ"]) for r in cr.values()])))
print("    control ca_tbl_no_integrity suspects      0 ( 0.0%%)  rmse %8.3f  energy %7.3f"
      % (median([float(r["rmseM"]) for r in ni.values()]),
         median([float(r["totalJ"]) for r in ni.values()])))
print("    K5 best (Q x4)              suspects  310.5 (21.6%)")

seeds = sorted(set(cr) & set(cc))
print("\n  %-13s %10s %10s %11s   %s" % ("", "control", "cell", "median d", "sign test"))
for f in ("suspects", "rmseM", "totalJ", "escalations", "fixCycles"):
    cv = [float(cc[s][f]) for s in seeds]
    kv = [float(cr[s][f]) for s in seeds]
    d = [c - k for c, k in zip(cv, kv)]
    pos, neg, p = sign_test(d)
    lo, hi = bootstrap_ci(d)
    extra = ""
    if f == "suspects":
        extra = "   -> %.1f%% of cycles" % (100.0 * median(cv) / 1440.0)
    print("  %-13s %10.3f %10.3f %+11.3f   %2d+/%2d- p%.4f  CI [%+.2f, %+.2f]%s"
          % (f, median(kv), median(cv), median(d), pos, neg, p, lo, hi, extra))

print("\n  -- cross-binary displacement floor (arms the flag cannot reach) --")
for a in ("static_868", "static_2p4"):
    sd = sorted(set(ctrl[a]) & set(cell[a]))
    d = [float(cell[a][s]["rmseM"]) - float(ctrl[a][s]["rmseM"]) for s in sd]
    pos, neg, p = sign_test(d)
    print("     %-12s median |drift| %8.4f m   %2d+/%2d- p%.4f"
          % (a, abs(median(d)), pos, neg, p))
print("  Any ca_tbl_full rmse difference smaller than this floor is NOT RESOLVABLE.")
