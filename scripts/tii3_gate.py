#!/usr/bin/env python3
"""G170: TII-3 flags. usage: tii3_gate.py <dir of tii3_gate.sh>

  RUNS  base, off, f1, f2, f4 exit 0 and write fixed_threshold's trace
  OFF   off is byte-identical to base in every file but run_id.txt
  LINE  the TII-3 line is in f1, f2 and f4's logs and in neither base's nor off's
  ARMS  every run's ca_tbl_full files are byte-identical to base's (the flags reach no other arm)
  F1    at lambda 0.0005, f1 reports a mean NIS > 1 and skips fewer 868 MHz slots than b01 (the scaled P kept slots)
  F2    f2 reports forced > 0 and skips fewer 868 MHz slots than base
  F4    f4 reports vetoes > 0 and skips fewer 868 MHz slots than base
"""
import filecmp
import os
import re
import sys

TR = "trace_fixed_threshold_seed7.csv"
RUNS = ("base", "off", "b01", "f1", "f2", "f4")
nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G170 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


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


def num(text, pat):
    m = re.search(pat, text)
    return int(m.group(1)) if m else None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in RUNS}
    have = all(os.path.exists(os.path.join(d, n, TR)) for n in RUNS)
    report(all(v == 0 for v in rcs.values()) and have, "RUNS", f"exit codes {rcs}, traces {have}")
    names, bad = same(os.path.join(d, "base"), os.path.join(d, "off"), lambda f: f != "run_id.txt")
    report(bool(names) and not bad, "OFF", f"{len(names) - len(bad)}/{len(names)} files identical, base vs off {bad[:3]}")
    has = {n: "TII-3 (fixed_threshold)" in log(d, n) for n in RUNS}
    report(has == {"base": False, "off": False, "b01": False, "f1": True, "f2": True, "f4": True}, "LINE", f"TII-3 line {has}")
    badall = []
    for n in RUNS[1:]:
        names, bad = same(os.path.join(d, "base"), os.path.join(d, n), lambda f: "ca_tbl_full" in f)
        badall += [f"{n}:{b}" for b in bad] + ([] if names else [f"{n}: none"])
    report(not badall, "ARMS", f"ca_tbl_full identical in every run {badall[:3]}")
    sk = lambda n: num(log(d, n), r"skipped (\d+) \(all measured nodes\)")
    m = re.search(r"tag-0 mean NIS ([0-9.]+)", log(d, "f1"))
    nis = float(m.group(1)) if m else None
    s0, s1 = sk("b01"), sk("f1")
    report(nis is not None and nis > 1.0 and None not in (s0, s1) and s1 < s0, "F1", f"mean NIS {nis}, skipped b01 {s0} f1 {s1} (need > 1, f1 < b01)")
    forced = num(log(d, "f2"), r"heartbeat N \d+ forced (\d+)")
    vetoes = num(log(d, "f4"), r"vetoes (\d+)")
    sb, s2, s4 = sk("base"), sk("f2"), sk("f4")
    report(forced is not None and forced > 0 and None not in (sb, s2) and s2 < sb, "F2", f"forced {forced}, skipped base {sb} f2 {s2}")
    report(vetoes is not None and vetoes > 0 and None not in (sb, s4) and s4 < sb, "F4", f"vetoes {vetoes}, skipped base {sb} f4 {s4}")
    print(f"  G170 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
