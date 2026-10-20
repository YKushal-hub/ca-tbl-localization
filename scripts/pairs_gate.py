#!/usr/bin/env python3
"""G160: --finderTasking=pairs sends finders 2k and 2k+1 to one subject, and is off by default.

Reads the run set scripts/pairs_gate.sh writes. What each failure looks like on disk:
  RUNS     on/, base/, def/, zero/ exit 0                                   (a crash under the flag)
  PAIRS    on.log's "MS-1 tasking (pairs)" line sends finders 2k, 2k+1 to   (the pairing not happening, or the
           subject (k mod N)+1: every tasked subject has exactly two         line missing)
  ROBIN    base.log's "MS-1 tasking (roundrobin)" line sends finder k to     (roundrobin moved by the change)
           subject (k mod N)+1: no subject twice with N >= finders
  EFFECT   on/'s tag-0 trace differs from base/'s                            (the flag parsed but not acting)
  OFF      def/ and zero/ byte-identical, run_id.txt aside and spec_profile.txt's finderTasking line, which records
           the explicit flag as kept (the preset default not roundrobin)
  REFUSE   r_name and r_none exit 1                                          (an unknown tasking, or pairs with no
                                                                              finders, accepted)
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
    print(f"  G160 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G160 MISSING {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def tasking(d, n, kind):
    p = os.path.join(d, n + ".log")
    if not os.path.exists(p):
        return None
    for line in open(p, errors="replace"):
        if f"MS-1 tasking ({kind}):" in line:
            return [(int(f), int(s)) for f, s in re.findall(r"finder (\d+) -> subject (\d+)", line)]
    return None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("on", "base", "def", "zero")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    p = tasking(d, "on", "pairs")
    if not p:
        missing("PAIRS", "no 'MS-1 tasking (pairs)' line in on.log")
    else:
        subj = sorted({s for _, s in p})
        ok = len(p) >= 2 and all(p[2 * k][1] == p[2 * k + 1][1] for k in range(len(p) // 2)) and \
            all(sum(1 for _, s in p if s == x) == 2 for x in subj) and len(p) % 2 == 0
        report(ok, "PAIRS", f"on: {len(p)} finders -> subjects {[s for _, s in p]} (need each tasked subject twice, "
                            f"in consecutive finders)")
    r = tasking(d, "base", "roundrobin")
    if not r:
        missing("ROBIN", "no 'MS-1 tasking (roundrobin)' line in base.log")
    else:
        ss = [s for _, s in r]
        report(len(ss) == len(set(ss)), "ROBIN", f"base: {len(r)} finders -> subjects {ss} (need all distinct)")
    a = glob.glob(os.path.join(d, "on", "trace_ca_tbl_full_seed*.csv"))
    b = glob.glob(os.path.join(d, "base", "trace_ca_tbl_full_seed*.csv"))
    if not a or not b:
        missing("EFFECT", "a tag-0 trace is absent in on/ or base/")
    else:
        report(open(a[0], "rb").read() != open(b[0], "rb").read(), "EFFECT",
               "on/ against base/ tag-0 trace (need different)")
    x, y = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(x) and os.path.isdir(y):
        fa = sorted(q for q in os.listdir(x) if q != "run_id.txt")
        fb = sorted(q for q in os.listdir(y) if q != "run_id.txt")

        def body(p):
            # spec_profile.txt records an explicit --finderTasking as kept rather than set: that one line is
            # provenance, and only that line is excused.
            t = open(p, "rb").read()
            if os.path.basename(p) == "spec_profile.txt":
                t = b"\n".join(ln for ln in t.split(b"\n") if not ln.startswith(b"finderTasking"))
            return t
        diff = [q for q in fa if q not in fb or body(os.path.join(x, q)) != body(os.path.join(y, q))]
        diff += [q for q in fb if q not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none; "
                                             f"spec_profile.txt's finderTasking line excused)")
    else:
        missing("OFF", "def/ or zero/ absent")
    rr = {n: rc(d, n) for n in ("r_name", "r_none")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G160: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
