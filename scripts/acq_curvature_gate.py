#!/usr/bin/env python3
"""G98: --acquireHandoverCurvature is wired, reached, changes the run when it fires, and is off by default.

Reads the run set scripts/acq_curvature_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                           (a crash under the flag)
  REACHED  on.log's "cycles the curvature guard kept the cloud N", N > 0  (a guard that never held:
                                                                     rule 4, a zero is a control-flow claim)
  DIFFERS  on/ and base/ arm_summary.csv differ                     (a guard that held but changed nothing)
  OFF      def/ and zero/ byte-identical, run_id.txt aside          (the default not being false)
  CONTROL  base, def and zero logs carry no guard line              (the guard leaking into the default)
  REFUSE   r_nopf exits 1                                           (the guard without the cloud it guards)
"""
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G98 {'ok     ' if ok else 'FAIL   '} {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G98 MISSING {label:<8}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    m = re.search(r"acquireHandoverCurvature \(ca_tbl_full\): cycles the curvature guard kept the cloud (\d+)", log(d, "on"))
    if not m:
        missing("REACHED", "no guard line for ca_tbl_full in on.log")
    else:
        report(int(m.group(1)) > 0, "REACHED", f"held {m.group(1)} cycles (need > 0)")
    pa, pb = os.path.join(d, "on", "arm_summary.csv"), os.path.join(d, "base", "arm_summary.csv")
    if os.path.exists(pa) and os.path.exists(pb):
        same = open(pa, "rb").read() == open(pb, "rb").read()
        report(not same, "DIFFERS", f"on/ against base/ arm_summary.csv: {'identical' if same else 'differ'} (need differ)")
    else:
        missing("DIFFERS", "on/ or base/ arm_summary.csv absent")
    a, b = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa = sorted(x for x in os.listdir(a) if x != "run_id.txt")
        fb = sorted(x for x in os.listdir(b) if x != "run_id.txt")
        diff = [x for x in fa if x not in fb or open(os.path.join(a, x), "rb").read() != open(os.path.join(b, x), "rb").read()]
        diff += [x for x in fb if x not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    leak = [n for n in ("base", "def", "zero") if "curvature guard" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its line: {leak} (need none)")
    report(rc(d, "r_nopf") == 1, "REFUSE", f"exit code {rc(d, 'r_nopf')} (need 1)")
    print(f"  G98: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
