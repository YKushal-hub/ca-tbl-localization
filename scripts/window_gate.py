#!/usr/bin/env python3
"""G152: --ekf5Window is wired, solves without failing, recovers the N.62 cell, and is off by default.

Reads the run set scripts/window_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                                    (a crash under the flag)
  REACHED  on.log "ekf5 window smoother: W 60, solves N, failures F", N > 0  (the flag silently not taking effect)
  SOLVES   F == 0                                                            (the solver failing: a singular window,
                                                                              a non-finite step)
  EFFECT   tag 0's median error over t >= 150 s in on/ below base/'s       (the window not recovering the cell it
                                                                              was built for: N.62's seed 5, where
                                                                              the EKF holds ~400 m)
  OFF      def/ and zero/ byte-identical, run_id.txt aside                    (the default not being 0)
  CONTROL  base, def and zero logs carry no window line                      (the flag leaking into the default)
  REFUSE   r_cv, r_rob and r_neg exit 1                                      (without ekf5, with a robust mode, or
                                                                              a window shorter than 2)
That the window is consistent (NEES within [0.5, 4] with three anchors) and recovers a corrupted heading with one
anchor is ca_tbl_imu_test's window cases.
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
    print(f"  G152 {'ok     ' if ok else 'FAIL   '} {label:<9}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G152 MISSING {label:<9}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def med_err(d):
    f = sorted(glob.glob(os.path.join(d, "trace_ca_tbl_full_seed*.csv")))
    if not f:
        return None
    e = [float(r["errM"]) for r in csv.DictReader(open(f[0])) if float(r["t"]) >= 150.0]
    return statistics.median(e) if e else None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    m = re.search(r"ekf5 window smoother: W (\d+), solves (\d+), failures (\d+)", log(d, "on"))
    if not m:
        missing("REACHED", "no window line in on.log")
        missing("SOLVES", "no window line in on.log")
    else:
        w, n, f = (int(x) for x in m.groups())
        report(n > 0, "REACHED", f"window {w}, solves {n} (need > 0)")
        report(f == 0, "SOLVES", f"failures {f} (need 0)")
    a, b = med_err(os.path.join(d, "on")), med_err(os.path.join(d, "base"))
    if a is None or b is None:
        missing("EFFECT", "no tag-0 trace in on/ or base/")
    else:
        report(a < b, "EFFECT", f"tag 0 median error t >= 150 s: window {a:.1f} m, EKF {b:.1f} m (need window below)")
    x, y = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(x) and os.path.isdir(y):
        fa = sorted(q for q in os.listdir(x) if q != "run_id.txt")
        fb = sorted(q for q in os.listdir(y) if q != "run_id.txt")
        diff = [q for q in fa if q not in fb or open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
        diff += [q for q in fb if q not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    leak = [n for n in ("base", "def", "zero") if "window smoother" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_cv", "r_rob", "r_neg")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1, 1)")
    print(f"  G152: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
