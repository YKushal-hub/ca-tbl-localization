#!/usr/bin/env python3
"""G159: --pcsByteScale carries PCS above 1 to the requester, and is off by default.

Reads the run set scripts/pcs_byte_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                                  (a crash under the flag)
  DEFECT   base/'s session results never decode a PCS above 1.0            (the default byte cannot carry it: N.7;
                                                                            if this fails the defect is gone or
                                                                            the reading is wrong)
  CARRIES  on/'s session results decode a PCS above 1.0                    (the flag silently not taking effect)
  REFUSABLE on/ has results at or above the preset's pcsThrUp2p4 (1.377),  (the requester rule still has nothing
           which the requester rule now does not ask on                     it could refuse)
  OFF      def/ and zero/ byte-identical, run_id.txt aside                 (the default not being 1)
  REFUSE   r_low and r_high exit 1                                         (a scale outside [1, 64] accepted)
"""
import csv
import glob
import os
import sys

nok = nfail = nmiss = 0
THR = 1.376873  # kSpecProfile's pcsThrUp2p4


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G159 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G159 MISSING {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def pcs(d):
    f = glob.glob(os.path.join(d, "session_ca_tbl_full_seed*.csv"))
    if not f:
        return None
    return [float(r["pcs"]) for r in csv.DictReader(open(f[0])) if r["event"] == "result"]


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    b, o = pcs(os.path.join(d, "base")), pcs(os.path.join(d, "on"))
    if not b:
        missing("DEFECT", "no session results in base/")
    else:
        report(max(b) <= 1.0, "DEFECT", f"base: {len(b)} results, max decoded PCS {max(b):.4f} (need <= 1.0)")
    if not o:
        missing("CARRIES", "no session results in on/")
        missing("REFUSABLE", "no session results in on/")
    else:
        report(max(o) > 1.0, "CARRIES", f"on: {len(o)} results, max decoded PCS {max(o):.4f} (need > 1.0)")
        n = sum(1 for v in o if v >= THR)
        report(n > 0, "REFUSABLE", f"on: {n} results at or above {THR} (need > 0)")
    x, y = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(x) and os.path.isdir(y):
        fa = sorted(q for q in os.listdir(x) if q != "run_id.txt")
        fb = sorted(q for q in os.listdir(y) if q != "run_id.txt")
        diff = [q for q in fa if q not in fb or open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
        diff += [q for q in fb if q not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    rr = {n: rc(d, n) for n in ("r_low", "r_high")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G159: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
