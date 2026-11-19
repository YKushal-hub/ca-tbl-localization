#!/usr/bin/env python3
"""G169: --benefitLambda / --benefitSkip868 (TII-1, report/PREREG_PARETO1.md). usage: benefit_rule_gate.py <dir>

  RUNS  abs, off and on exit 0 and write fixed_threshold's trace
  OFF   off (--benefitLambda=0 --benefitSkip868=false) is byte-identical to abs (flags absent), every file but run_id.txt
  LINE  the "TII-1 benefitSkip868 (fixed_threshold" line is in on's log and in neither abs's nor off's
  ARMS  on's ca_tbl_full files are byte-identical to abs's (the flags reach no other arm)
  SKIP  on: 868 MHz slots considered > 0 and skipped > 0, and fixed_threshold's tag-0 j_868 is below abs's
  RULE  on's fixed_threshold trace differs from abs's (the trigger changed what the node did)
"""
import csv
import filecmp
import os
import re
import sys

LINE = "TII-1 benefitSkip868 (fixed_threshold"
TR = "trace_fixed_threshold_seed7.csv"
nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G169 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, f"{n}.rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, f"{n}.log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def same(a, b, pick):
    names = sorted(f for f in os.listdir(a) if pick(f)) if os.path.isdir(a) else []
    bad = [f for f in names if not (os.path.exists(os.path.join(b, f)) and
                                    filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False))]
    return names, bad


def j868(d, n):
    p = os.path.join(d, n, "arm_summary.csv")
    if not os.path.exists(p):
        return None
    for r in csv.DictReader(open(p)):
        if r["arm"] == "fixed_threshold":
            return float(r["j_868"])
    return None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("abs", "off", "on")}
    have = all(os.path.exists(os.path.join(d, n, TR)) for n in rcs)
    report(all(v == 0 for v in rcs.values()) and have, "RUNS", f"exit codes {rcs}, traces {have} (need all 0, True)")
    names, bad = same(os.path.join(d, "abs"), os.path.join(d, "off"), lambda f: f != "run_id.txt")
    report(bool(names) and not bad, "OFF", f"{len(names) - len(bad)}/{len(names)} files identical, abs vs off {bad[:3]}")
    has = {n: LINE in log(d, n) for n in ("abs", "off", "on")}
    report(has == {"abs": False, "off": False, "on": True}, "LINE", f"TII-1 line {has}")
    names, bad = same(os.path.join(d, "abs"), os.path.join(d, "on"), lambda f: "ca_tbl_full" in f)
    report(bool(names) and not bad, "ARMS", f"{len(names) - len(bad)}/{len(names)} ca_tbl_full files identical {bad[:3]}")
    m = re.search(r"TII-1 benefitSkip868 \(fixed_threshold, lambda [0-9.e+]+ m\^2/J\): 868 MHz slots considered (\d+), "
                  r"skipped (\d+)", log(d, "on"))
    ja, jo = j868(d, "abs"), j868(d, "on")
    ok = m is not None and int(m.group(1)) > 0 and int(m.group(2)) > 0 and ja is not None and jo is not None and jo < ja
    report(ok, "SKIP", f"considered/skipped {m.groups() if m else None}; tag-0 j_868 abs {ja} on {jo} (need > 0, > 0, on < abs)")
    pa, po = os.path.join(d, "abs", TR), os.path.join(d, "on", TR)
    differ = os.path.exists(pa) and os.path.exists(po) and not filecmp.cmp(pa, po, shallow=False)
    report(differ, "RULE", f"fixed_threshold trace on vs abs differs: {differ} (need True)")
    print(f"  G169 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
