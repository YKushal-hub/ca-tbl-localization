#!/usr/bin/env python3
"""Reusable degeneracy pre-screen. USAGE: degeneracy_check.py <result-dir>

Emits USEFUL or DEGENERATE:<reason> per arm, on the D1 rules. Intended to be run
on ONE seed before a campaign spends twenty.
"""
import csv, os, sys

HYBRID = {"ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold"}
STATIC = {"static_868": "cycles_868", "static_2p4": "cycles_2p4", "static_uwb": "cycles_uwb"}

def check(d):
    f = os.path.join(d, "arm_summary.csv")
    if not os.path.exists(f) or os.path.getsize(f) == 0:
        return [("(all)", "DEGENERATE: no arm_summary")]
    out = []
    for r in csv.DictReader(open(f)):
        arm = r["arm"]; g = lambda k: float(r.get(k, 0) or 0)
        c8, c24, cu = g("cycles_868"), g("cycles_2p4"), g("cycles_uwb")
        tot = c8 + c24 + cu; why = []
        if arm in HYBRID and tot > 0:
            if max(c8, c24, cu) / tot > 0.99: why.append("%.0f%% on one tier" % (100 * max(c8, c24, cu) / tot))
            if g("escalations") == 0: why.append("0 escalations")
        if arm == "ca_tbl_full" and g("suspects") == 0: why.append("gate inert (0 suspects)")
        fix, nofix = g("fixCycles"), g("noFixCycles")
        if fix + nofix and fix / (fix + nofix) < 0.90: why.append("hadFix %.2f" % (fix / (fix + nofix)))
        if arm in STATIC:
            other = sum(g(k) for k in ("cycles_868", "cycles_2p4", "cycles_uwb") if k != STATIC[arm])
            if other > 0: why.append("static arm on other tiers")
        out.append((arm, "DEGENERATE: " + ", ".join(why) if why else "USEFUL"))
    return out

if __name__ == "__main__":
    for arm, v in check(sys.argv[1]):
        print("%-22s %s" % (arm, v))
