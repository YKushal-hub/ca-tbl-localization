#!/usr/bin/env python3
"""G95: --imuFilter=ekf5 is wired, learns, and is off by default (TECH_DEBT N.28).

Reads the run set scripts/ekf5_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, def/ and cv/ exit 0                          (a crash under the flag)
  LINE     on.log carries "N.28 ekf5:" with every field      (the flag silently not taking effect)
  WIRED    moving + dwell == DC-1's cycles, moving > 0       (a prediction that bypassed ekf5)
  LEARNS   |bias est - true| < 0.5 |true|, and the heading
           offset est within 0.2 rad of DC-1's true error    (a filter that runs but learns nothing)
  OFF      def/ and cv/ byte-identical, run_id.txt aside     (the default not being cv)
  CONTROL  def.log and cv.log carry no "N.28 ekf5:" line     (ekf5 leaking into the default)
  REFUSE   r_noimu and r_bogus exit 1                        (a meaningless combination accepted)
"""
import math
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G95 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G95 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G95 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "def", "cv")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")

    on = log(d, "on") or ""
    m = re.search(r"N\.28 ekf5: moving (\d+), dwell (\d+), bias est (\S+) \(true (\S+)\) rad/s, "
                  r"scale est (\S+) \(true (\S+)\), heading offset est (\S+) \(true (\S+)\) rad", on)
    c = re.search(r"DC-1 IMU: bias \S+ rad/s, scale \S+, cycles (\d+)", on)
    if not m or not c:
        missing("LINE", "no complete N.28 ekf5 / DC-1 line in on.log")
    else:
        report(True, "LINE", "N.28 ekf5 and DC-1 lines present")
        mv, dw = int(m.group(1)), int(m.group(2))
        be, bt, he, ht = float(m.group(3)), float(m.group(4)), float(m.group(7)), float(m.group(8))
        cyc = int(c.group(1))
        report(mv > 0 and mv + dw == cyc, "WIRED", f"moving {mv} + dwell {dw} = {mv + dw} against DC-1 cycles {cyc} (need equal, moving > 0)")
        dh = math.atan2(math.sin(he - ht), math.cos(he - ht))
        ok = abs(bt) > 0 and abs(be - bt) < 0.5 * abs(bt) and abs(dh) < 0.2
        report(ok, "LEARNS", f"bias est {be:.3e} vs true {bt:.3e} (need within 50 %); heading offset error {dh:+.3f} rad (need |.| < 0.2)")

    a, b = os.path.join(d, "def"), os.path.join(d, "cv")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(f for f in os.listdir(a) if f != "run_id.txt")
        fb = sorted(f for f in os.listdir(b) if f != "run_id.txt")
        diff = [f for f in fa if f not in fb or open(os.path.join(a, f), "rb").read() != open(os.path.join(b, f), "rb").read()]
        diff += [f for f in fb if f not in fa]
        report(not diff and fa, "OFF", f"def/ against cv/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or cv/ absent")

    leak = [n for n in ("def", "cv") if "N.28 ekf5:" in (log(d, n) or "")]
    report(not leak, "CONTROL", f"runs without ekf5 printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_noimu", "r_bogus")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G95: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
