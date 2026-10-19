#!/usr/bin/env python3
"""G151: --ekf5Iterate is wired, re-linearises (more than one pass per update), changes the run, and is off by default.

Reads the run set scripts/iterate_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                                    (a crash under the flag)
  REACHED  on.log "ekf5 iterated update: updates U, ...", U > 0              (the flag silently not taking effect)
  ITERATES passes P > U                                                      (every update converged at once: the
                                                                              linearisation point never moved)
  EFFECT   on/ and base/ traces differ                                       (iterating changed nothing at all)
  OFF      def/ and zero/ byte-identical, run_id.txt aside                    (the default not being 0)
  CONTROL  base, def and zero logs carry no iterated-update line             (the flag leaking into the default)
  REFUSE   r_cv, r_rob and r_neg exit 1                                      (without ekf5, with a robust mode,
                                                                              or a negative pass count)
That the iterated update is consistent where the single one is not (NEES within [1, 3.5] against more than twice
that from a 200 m prior) is ca_tbl_imu_test's iterated-update cases.
"""
import glob
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G151 {'ok     ' if ok else 'FAIL   '} {label:<9}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G151 MISSING {label:<9}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, n + ".log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def same_dir(a, b):
    fa = sorted(x for x in os.listdir(a) if x != "run_id.txt")
    fb = sorted(x for x in os.listdir(b) if x != "run_id.txt")
    diff = [x for x in fa if x not in fb or open(os.path.join(a, x), "rb").read() != open(os.path.join(b, x), "rb").read()]
    diff += [x for x in fb if x not in fa]
    return fa, diff


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    on = log(d, "on")
    m = re.search(r"ekf5 iterated update: updates (\d+), passes (\d+), unconverged (\d+)", on)
    if not m:
        missing("REACHED", "no iterated-update line in on.log")
        missing("ITERATES", "no iterated-update line in on.log")
    else:
        u, p, c = (int(x) for x in m.groups())
        report(u > 0, "REACHED", f"iterated updates {u} (need > 0)")
        report(p > u, "ITERATES", f"passes {p} over {u} updates, unconverged {c} (need passes > updates)")
    tr_on = sorted(glob.glob(os.path.join(d, "on", "trace_ca_tbl_full_seed*.csv")))
    tr_base = sorted(glob.glob(os.path.join(d, "base", "trace_ca_tbl_full_seed*.csv")))
    if not tr_on or not tr_base:
        missing("EFFECT", "no tag-0 trace in on/ or base/")
    else:
        report(open(tr_on[0], "rb").read() != open(tr_base[0], "rb").read(), "EFFECT", "on/ and base/ tag-0 traces differ")
    a, b = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(a) and os.path.isdir(b):
        fa, diff = same_dir(a, b)
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    leak = [n for n in ("base", "def", "zero") if "iterated update" in log(d, n)]
    report(not leak, "CONTROL", f"runs without the flag printing its line: {leak} (need none)")
    rr = {n: rc(d, n) for n in ("r_cv", "r_rob", "r_neg")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1, 1)")
    print(f"  G151: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
