#!/usr/bin/env python3
"""P3' and P4: what the repaired configuration bought, paired by seed.

k1a and k1b differ by FIVE flags, not the two P3 named -- gated PCS, floor on demand,
the evidence gate, the corroboration witness and the bidirectional digest. So this
measures a bundle, and says so.

The static arms carry no policy, so their k1a-vs-k1b difference is pure shared-RNG
displacement (rule 8). That is printed alongside as the floor: a policy-arm difference
smaller than it means nothing.
"""
import csv
import os
import sys

sys.path.insert(0, "/tmp")
from k1_analyse import sign_test, median, bootstrap_ci, load_cell  # noqa: E402

SCALES = [750, 1000, 1500, 2000, 3000]
ARMS = ["ca_tbl_full", "ca_tbl_no_integrity", "static_868", "static_2p4"]

print("REPAIRED (k1a) vs UNREPAIRED (k1b), paired by seed")
print("positive difference = the REPAIRED cell is better (less rmse, less energy)")
print("five flags move together: gated PCS, floor on demand, evidence gate,")
print("corroboration witness, bidirectional digest")
print("=" * 92)

for B in SCALES:
    a, na = load_cell("results/k1a_box%d" % B)
    b, nb = load_cell("results/k1b_box%d" % B)
    if na or nb:
        print("box%-6d EXCLUDED  %s %s" % (B, na or "", nb or ""))
        continue
    print("\n### boxHalf = %d" % B)
    print("  %-22s %9s %9s %10s   %s" %
          ("arm", "repaired", "unrepaired", "median d", "sign test"))
    for arm in ARMS:
        seeds = sorted(set(a[arm]) & set(b[arm]))
        for field, unit in (("rmseM", "m"), ("totalJ", "J")):
            av = [float(a[arm][s][field]) for s in seeds]
            bv = [float(b[arm][s][field]) for s in seeds]
            d = [x - y for x, y in zip(bv, av)]   # positive => repaired better
            pos, neg, p = sign_test(d)
            lo, hi = bootstrap_ci(d)
            tag = "  <-- NOISE FLOOR" if arm.startswith("static") else ""
            print("  %-16s %-5s %9.3f %9.3f %+10.3f   %2d+/%2d- p%.4f  "
                  "CI [%+.2f, %+.2f]%s" %
                  (arm, unit, median(av), median(bv), median(d),
                   pos, neg, p, lo, hi, tag))
