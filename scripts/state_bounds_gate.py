#!/usr/bin/env python3
"""G102: --ekf5StateBounds is wired, reached, keeps the stride scale and bias inside +/- k prior sigmas, and is off by
default.

Reads the run set scripts/state_bounds_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                                  (a crash under the flag)
  REACHED  "ekf5 state bounds: projections N", N > 0                        (a projection that never fired: rule 4)
  BOUNDED  on's final |scale est| <= 4 x 0.0266 and |bias est| <= 4 x DC-1's prior sigma; base's |scale est| above
           that bound                                                       (a bound that did not hold, or a cell
                                                                             where it had nothing to do)
  OFF      def/ and zero/ byte-identical, run_id.txt aside                  (the default not being 0)
  CONTROL  base, def and zero logs carry no bounds line                     (the projection leaking into the default)
  REFUSE   r_cv and r_neg exit 1                                            (without ekf5, or negative, accepted)
"""
import math
import os
import re
import sys

nok = nfail = nmiss = 0
SCALE_SIGMA = 0.0266
BIAS_SIGMA = 0.15 * math.pi / 180.0  # DC-1 under --imuGyroBiasSigmaDps=0.15


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G102 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G102 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def est(text):
    m = re.search(r"N\.28 ekf5: .*bias est (\S+) \(true \S+\) rad/s, scale est (\S+) \(true", text)
    return (float(m.group(1)), float(m.group(2))) if m else None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    on = log(d, "on")
    m = re.search(r"ekf5 state bounds: projections (\d+)", on)
    if not m:
        missing("REACHED", "no bounds line in on.log")
    else:
        report(int(m.group(1)) > 0, "REACHED", f"projections {m.group(1)} (need > 0)")
    eo, eb = est(on), est(log(d, "base"))
    if eo is None or eb is None:
        missing("BOUNDED", "no N.28 ekf5 line in on.log or base.log")
    else:
        lim_s, lim_b = 4.0 * SCALE_SIGMA, 4.0 * BIAS_SIGMA
        ok = abs(eo[1]) <= lim_s + 1e-9 and abs(eo[0]) <= lim_b + 1e-12 and abs(eb[1]) > lim_s
        report(ok, "BOUNDED", f"scale on {eo[1]:+.4f}, base {eb[1]:+.4f} (bound {lim_s:.4f}); bias on {eo[0]:+.3e} (bound {lim_b:.3e})")
    a, b = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(x for x in os.listdir(a) if x != "run_id.txt")
        fb = sorted(x for x in os.listdir(b) if x != "run_id.txt")
        diff = [x for x in fa if x not in fb or open(os.path.join(a, x), "rb").read() != open(os.path.join(b, x), "rb").read()]
        diff += [x for x in fb if x not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    leak = [n for n in ("base", "def", "zero") if "state bounds" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_cv", "r_neg")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G102: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
