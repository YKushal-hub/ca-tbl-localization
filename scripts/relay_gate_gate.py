#!/usr/bin/env python3
"""G101: --ekf5RelayGate gates only relayed ranges, is reached, changes the run, and is off by default.

Reads the run set scripts/relay_gate_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, zero/ exit 0                                     (a crash under the flag)
  REACHED  "relayed ranges S, gated G": S > 0, 0 < G < S                 (a gate that never fired, or that dropped
                                                                         every relay: rule 4)
  DIFFERS  on/ and base/ arm_summary.csv differ                         (a gate that fired but changed nothing)
  OFF      base/ and zero/ byte-identical, run_id.txt aside             (the default not being false)
  CONTROL  base.log and zero.log carry no relay-gate line                (the gate leaking into the default)
  REFUSE   r_cv and r_rob exit 1                                         (without ekf5, or with a robust mode)
That it gates relays only, never the filter's own ranges, is ca_tbl_imu_test's relay gate cases.
"""
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G101 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G101 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    m = re.search(r"ekf5 relay gate: relayed ranges (\d+), gated (\d+)", log(d, "on"))
    if not m:
        missing("REACHED", "no relay-gate line in on.log")
    else:
        s, g = int(m.group(1)), int(m.group(2))
        report(s > 0 and 0 < g < s, "REACHED", f"relayed {s}, gated {g} (need relayed > 0, 0 < gated < relayed)")
    pa, pb = os.path.join(d, "on", "arm_summary.csv"), os.path.join(d, "base", "arm_summary.csv")
    if os.path.exists(pa) and os.path.exists(pb):
        same = open(pa, "rb").read() == open(pb, "rb").read()
        report(not same, "DIFFERS", f"on/ against base/: {'identical' if same else 'differ'} (need differ)")
    else:
        missing("DIFFERS", "on/ or base/ arm_summary.csv absent")
    a, b = os.path.join(d, "base"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(x for x in os.listdir(a) if x != "run_id.txt")
        fb = sorted(x for x in os.listdir(b) if x != "run_id.txt")
        diff = [x for x in fa if x not in fb or open(os.path.join(a, x), "rb").read() != open(os.path.join(b, x), "rb").read()]
        diff += [x for x in fb if x not in fa]
        report(not diff and bool(fa), "OFF", f"base/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "base/ or zero/ absent")
    leak = [n for n in ("base", "zero") if "relay gate" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_cv", "r_rob")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G101: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
