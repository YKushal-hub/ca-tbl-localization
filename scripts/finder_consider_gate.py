#!/usr/bin/env python3
"""G100: --ekf5FinderConsider is wired, takes the finder GNSS variance out of each range's R and into the filter, and is
off by default.

Reads the run set scripts/finder_consider_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                                    (a crash under the flag)
  LINE     on.log carries the consider line, with variance 4 m^2              (the flag silently not taking effect)
  REACHED  "ranges to finders N", N > 0                                       (consider never applied: rule 4)
  RSPLIT   tag 0's fused 2.4 GHz finder ranges: median sigma on/ is below base/'s by the GNSS part --
           base sqrt(s^2 + 4) against on s, so base^2 - on^2 within [3, 5] m^2 (the R still carrying the
                                                                              offset: counted twice)
  OFF      def/ and zero/ byte-identical, run_id.txt aside                    (the default not being false)
  CONTROL  base, def and zero logs carry no consider line                     (the flag leaking into the default)
  REFUSE   r_cv and r_rob exit 1                                              (without ekf5, or with a robust mode)
The consistency property (NEES ~2 against 53 naive) is ca_tbl_imu_test's finder consider cases.
"""
import csv
import glob
import os
import re
import statistics
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G100 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G100 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def finder_sigma(d):
    f = glob.glob(os.path.join(d, "ranges_ca_tbl_full_seed*.csv"))
    if not f:
        return None
    v = [float(r["sigmaM"]) for r in csv.DictReader(open(f[0]))
         if r["inSet"] == "1" and r["tier"] == "2.4GHz" and r["sourceKind"] == "2"]
    return statistics.median(v) if v else None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    on = log(d, "on")
    report("finder GNSS offsets carried as consider parameters, variance 4 m^2" in on, "LINE", "consider line in on.log")
    m = re.search(r"ekf5 finder consider: ranges to finders (\d+)", on)
    if not m:
        missing("REACHED", "no count line in on.log")
    else:
        report(int(m.group(1)) > 0, "REACHED", f"ranges to finders {m.group(1)} (need > 0)")
    so, sb = finder_sigma(os.path.join(d, "on")), finder_sigma(os.path.join(d, "base"))
    if so is None or sb is None:
        missing("RSPLIT", "no fused 2.4 GHz finder ranges in on/ or base/")
    else:
        dv = sb * sb - so * so
        report(3.0 <= dv <= 5.0, "RSPLIT", f"median finder sigma base {sb:.3f} m, on {so:.3f} m: base^2 - on^2 = {dv:.2f} m^2 (need [3, 5])")
    a, b = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(x for x in os.listdir(a) if x != "run_id.txt")
        fb = sorted(x for x in os.listdir(b) if x != "run_id.txt")
        diff = [x for x in fa if x not in fb or open(os.path.join(a, x), "rb").read() != open(os.path.join(b, x), "rb").read()]
        diff += [x for x in fb if x not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    leak = [n for n in ("base", "def", "zero") if "finder consider" in log(d, n) or "consider parameters" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its lines: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_cv", "r_rob")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G100: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
