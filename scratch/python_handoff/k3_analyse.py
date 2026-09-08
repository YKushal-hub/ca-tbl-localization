#!/usr/bin/env python3
"""K3 -- is the evidence gate the 300 J? Procedure fixed by PREREG_K3 sec 5."""
import sys

sys.path.insert(0, "/tmp")
from k1_analyse import sign_test, median, bootstrap_ci, load_cell  # noqa: E402

ctrl, n1 = load_cell("results/k1a_box1500")
cell, n2 = load_cell("results/k3_noevid_box1500")
unrep, n3 = load_cell("results/k1b_box1500")
if n1 or n2:
    sys.exit("%s %s" % (n1 or "", n2 or ""))

print("K3 -- IS THE EVIDENCE GATE THE 300 J?")
print("control k1a_box1500 (gate ON)  cell k3_noevid_box1500 (gate OFF)")
print("positive difference = turning the gate OFF is WORSE (more energy, more rmse)")
print("=" * 92)

arm = "ca_tbl_full"
seeds = sorted(set(ctrl[arm]) & set(cell[arm]))
for field, unit in (("totalJ", "J"), ("j_uwb", "J"), ("j_2p4", "J"),
                    ("escalations", "n"), ("rmseM", "m"), ("fixCycles", "n")):
    cv = [float(cell[arm][s][field]) for s in seeds]
    kv = [float(ctrl[arm][s][field]) for s in seeds]
    d = [c - k for c, k in zip(cv, kv)]
    pos, neg, p = sign_test(d)
    lo, hi = bootstrap_ci(d)
    print("  %-12s %-2s gate ON %10.3f   gate OFF %10.3f   d %+10.3f   "
          "%2d+/%2d- p%.4f  CI [%+.2f, %+.2f]"
          % (field, unit, median(kv), median(cv), median(d), pos, neg, p, lo, hi))

print("\n  reference: k1b_box1500 (all five flags unrepaired) totalJ = %.3f J"
      % median([float(r["totalJ"]) for r in unrep[arm].values()]))

print("\n  -- noise floor: arms the flag cannot reach --")
for a in ("static_868", "static_2p4"):
    seeds = sorted(set(ctrl[a]) & set(cell[a]))
    d = [float(cell[a][s]["rmseM"]) - float(ctrl[a][s]["rmseM"]) for s in seeds]
    pos, neg, p = sign_test(d)
    print("     %-12s median |drift| %8.4f m   %2d+/%2d- p%.4f"
          % (a, abs(median(d)), pos, neg, p))
