#!/usr/bin/env python3
"""G99: --imuMagSigmaDeg is wired, fused every moving cycle, bounds the heading where the gyro alone did not, and is
off by default.

Reads the run set scripts/imu_mag_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                                   (a crash under the flag)
  LINE     on.log carries the magnetometer configuration line                (the flag silently not taking effect)
  FUSED    "headings fused N" equals ekf5's IMU cycles, moving + dwell       (readings dropped or never fused)
  BOUNDS   base's final heading offset error > 0.5 rad, on's < 0.2 rad       (a reference that does not bound the
                                                                              heading on the cell it exists for)
  OFF      def/ and zero/ byte-identical, run_id.txt aside                   (the default not being 0)
  CONTROL  base, def and zero logs carry no magnetometer line                (the reference leaking into the default)
  REFUSE   r_cv and r_neg exit 1                                             (without ekf5, or negative, accepted)
The reading's stated variance and ekf5's heading update are ca_tbl_imu_test's magnetometer cases.
"""
import math
import os
import re
import sys

nok = nfail = nmiss = 0
HEAD = re.compile(r"heading offset est (\S+) \(true (\S+)\) rad")


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G99 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G99 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def herr(text):
    m = HEAD.search(text)
    if not m:
        return None
    e, t = float(m.group(1)), float(m.group(2))
    return abs(math.atan2(math.sin(e - t), math.cos(e - t)))


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    on = log(d, "on")
    report("magnetometer heading reference, white 3 deg" in on, "LINE", "magnetometer line in on.log")
    f = re.search(r"magnetometer: headings fused (\d+)", on)
    m = re.search(r"N\.28 ekf5: moving (\d+), dwell (\d+)", on)
    if not f or not m:
        missing("FUSED", "no fused count or no N.28 line in on.log")
    else:
        cyc = int(m.group(1)) + int(m.group(2))
        report(int(f.group(1)) == cyc and cyc > 0, "FUSED",
               f"fused {f.group(1)} against IMU cycles {cyc} (moving {m.group(1)} + dwell {m.group(2)}; need equal, > 0)")
    eo, eb = herr(on), herr(log(d, "base"))
    if eo is None or eb is None:
        missing("BOUNDS", "no heading offset line in on.log or base.log")
    else:
        report(eb > 0.5 and eo < 0.2, "BOUNDS", f"final heading offset error: base {eb:.3f} rad (need > 0.5), on {eo:.3f} (need < 0.2)")
    a, b = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(x for x in os.listdir(a) if x != "run_id.txt")
        fb = sorted(x for x in os.listdir(b) if x != "run_id.txt")
        diff = [x for x in fa if x not in fb or open(os.path.join(a, x), "rb").read() != open(os.path.join(b, x), "rb").read()]
        diff += [x for x in fb if x not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    leak = [n for n in ("base", "def", "zero") if "magnetometer" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_cv", "r_neg")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G99: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
