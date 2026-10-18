#!/usr/bin/env python3
"""G97: --imuStrideSigmaPct adds per-step stride noise to DC-1 as specified, and is off by default.

Reads the run set scripts/imu_stride_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, def/, zero/ exit 0                                     (a crash under the flag)
  LINE     on.log carries "DC-1: stride noise 5 % per step"            (the flag silently not taking effect)
  NOISE    tag 0's imu_ trace: along-track residual |u| - (1 + scale)|d| over moving cycles has
           E[e^2] / (sigma^2 l |d|) in [0.75, 1.25] with sigma 0.05, l 0.7 m   (noise absent, or the wrong size)
  OFF      def/ and zero/ byte-identical, run_id.txt aside             (the default not being 0)
  CONTROL  def.log and zero.log carry no stride line                   (the noise leaking into the default)
  REFUSE   r_noimu and r_neg exit 1                                    (a meaningless value accepted)
The per-cycle law and its stated Q_u are ca_tbl_imu_test's stride cases.
"""
import csv
import glob
import math
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G97 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G97 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    on = log(d, "on")
    report("DC-1: stride noise 5 % per step" in on, "LINE", "stride line in on.log")
    m = re.search(r"DC-1 IMU: bias \S+ rad/s, scale (\S+),", on)
    f = glob.glob(os.path.join(d, "on", "imu_ca_tbl_full_seed*.csv"))
    if not m or not f:
        missing("NOISE", "no DC-1 scale line or no imu_ trace")
    else:
        sc = float(m.group(1))
        num = den = 0.0
        n = 0
        for r in csv.DictReader(open(f[0])):
            dx, dy, ux, uy = (float(r[k]) for k in ("dxTrue", "dyTrue", "ux", "uy"))
            L = math.hypot(dx, dy)
            if L < 0.5:
                continue
            e = math.hypot(ux, uy) - (1.0 + sc) * L
            num += e * e
            den += 0.05 * 0.05 * 0.7 * L
            n += 1
        ratio = num / den if den > 0 else float("nan")
        report(n > 100 and 0.75 <= ratio <= 1.25, "NOISE", f"E[e^2] / model {ratio:.3f} over {n} moving cycles (need [0.75, 1.25], n > 100)")
    a, b = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(x for x in os.listdir(a) if x != "run_id.txt")
        fb = sorted(x for x in os.listdir(b) if x != "run_id.txt")
        diff = [x for x in fa if x not in fb or open(os.path.join(a, x), "rb").read() != open(os.path.join(b, x), "rb").read()]
        diff += [x for x in fb if x not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    leak = [n for n in ("def", "zero") if "stride noise" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_noimu", "r_neg")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G97: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
