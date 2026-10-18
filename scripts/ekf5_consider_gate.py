#!/usr/bin/env python3
"""G96: --ekf5Robust=8 (CONSIDER) is wired, reached on the population it claims, and is off by
default (TECH_DEBT N.33). EKF5-CONSIDER rejected the mode; nothing here says it helps.

Reads the run set scripts/ekf5_consider_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                    (a crash under the flag)
  LINE     on.log carries "N.33 ekf5 mode 8:" with counts    (the flag silently not taking effect)
  REACHED  considered > 0, and gated = protected =
           recoveries = 0                                    (a mode that never fired, or the
                                                              wrong bits set: rule 4)
  COUNTS   considered == Case D's applied updates            (the mode firing on some other
                                                              population than the one-range fixes)
  DIFFERS  on/ and base/ differ in arm_summary.csv           (a mode that fired but changed nothing:
                                                              not wired into the arithmetic)
  OFF      def/ and zero/ byte-identical, run_id.txt aside   (the default not being mode 0)
  CONTROL  base, def and zero logs carry no "N.33 ekf5" line (the mode leaking into the default)
  REFUSE   r_cv and r_range exit 1                           (a meaningless combination accepted)
The filter-level property -- one or two ranges leave theta, b, s and their variances unchanged,
three do not -- is ca_tbl_imu_test's N.33 cases.
"""
import os
import re
import sys

nok = nfail = nmiss = 0
CASED = re.compile(r"DC-4 Case D: cycles \d+, updates applied (\d+)")
LINE = re.compile(r"N\.33 ekf5 mode (\d+): gated (\d+), protected (\d+), recoveries (\d+), considered (\d+)")


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G96 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G96 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G96 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else None


def summary(d, n):
    p = os.path.join(d, n, "arm_summary.csv")
    return open(p, "rb").read() if os.path.exists(p) else None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")

    on = log(d, "on") or ""
    m = LINE.search(on)
    if not m:
        missing("LINE", "no complete N.33 ekf5 line in on.log")
    else:
        report(m.group(1) == "8", "LINE", f"N.33 line present, mode {m.group(1)} (need 8)")
        g, p, r, c = (int(m.group(i)) for i in range(2, 6))
        report(c > 0 and g == p == r == 0, "REACHED",
               f"considered {c}, gated {g}, protected {p}, recoveries {r} (need > 0, 0, 0, 0)")

    cd = CASED.search(on)
    if not m or not cd:
        missing("COUNTS", "no N.33 or DC-4 Case D line in on.log")
    else:
        report(int(m.group(5)) == int(cd.group(1)), "COUNTS",
               f"considered {m.group(5)} against Case D updates applied {cd.group(1)} (need equal)")
    so, sb = summary(d, "on"), summary(d, "base")
    if so is None or sb is None:
        missing("DIFFERS", "on/ or base/ arm_summary.csv absent")
    else:
        report(so != sb, "DIFFERS", f"on/ against base/ arm_summary.csv: {'differ' if so != sb else 'identical'} (need differ)")

    a, b = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(f for f in os.listdir(a) if f != "run_id.txt")
        fb = sorted(f for f in os.listdir(b) if f != "run_id.txt")
        diff = [f for f in fa if f not in fb or open(os.path.join(a, f), "rb").read() != open(os.path.join(b, f), "rb").read()]
        diff += [f for f in fb if f not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")

    leak = [n for n in ("base", "def", "zero") if "N.33 ekf5" in (log(d, n) or "")]
    report(not leak, "CONTROL", f"runs without the mode printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_cv", "r_range")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G96: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
