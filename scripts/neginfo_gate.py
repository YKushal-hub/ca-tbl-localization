#!/usr/bin/env python3
"""G162: --traceNegInfo writes the negative-information trace, and only that.

Reads the run set scripts/neginfo_gate.sh writes. What each failure looks like on disk:
  RUNS    base/ and on/ exit 0                                              (a crash under the flag)
  FILES   on/ has one neginfo_ca_tbl_full_seed205_subj<k>.csv per subject   (the flag not writing, or the wrong
          (4), each with the registered header                               columns)
  ROWS    on/'s rows exist; none on 868MHz (the precise radio is off there, (the trace logging a cycle where silence
          so silence means nothing); every dEstM <= 150                      cannot mean anything, or past its radius)
  INERT   every file in on/ other than neginfo_* and run_id.txt is byte-     (the trace changing what it observes)
          identical to base/'s
  OFF     base/ has no neginfo_* file                                       (the default writing it)
"""
import csv
import glob
import os
import sys

HEADER = "t,subjectId,activeTier,finderId,dEstM,entryAgeS,silenceS,contactNow,sigmaClaimM,trueErrM,trueDistM"
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G162 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def main():
    global nmiss
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("base", "on")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need 0, 0)")
    fs = sorted(glob.glob(os.path.join(d, "on", "neginfo_ca_tbl_full_seed205_subj*.csv")))
    heads = [open(f).readline().strip() for f in fs]
    report(len(fs) == 4 and all(h == HEADER for h in heads), "FILES", f"on/: {len(fs)} files (need 4), headers ok "
           f"{sum(h == HEADER for h in heads)}")
    rows = [r for f in fs for r in csv.DictReader(open(f))]
    bad868 = sum(1 for r in rows if r["activeTier"] == "868MHz")
    far = sum(1 for r in rows if float(r["dEstM"]) > 150.0)
    report(len(rows) > 0 and bad868 == 0 and far == 0, "ROWS", f"{len(rows)} rows (need > 0), on 868MHz {bad868}, "
           f"dEstM > 150 {far} (need 0, 0)")
    x, y = os.path.join(d, "base"), os.path.join(d, "on")
    if os.path.isdir(x) and os.path.isdir(y):
        fb = sorted(q for q in os.listdir(x) if q != "run_id.txt")
        fo = sorted(q for q in os.listdir(y) if q != "run_id.txt" and not q.startswith("neginfo_"))
        diff = [q for q in fb if q not in fo or open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
        diff += [q for q in fo if q not in fb]
        report(not diff and bool(fb), "INERT", f"{len(fb)} files, differing {diff[:3]} (need none)")
        nb = [q for q in os.listdir(x) if q.startswith("neginfo_")]
        report(not nb, "OFF", f"base/ neginfo files {nb[:2]} (need none)")
    else:
        nmiss += 1
        print("  G162 MISSING INERT     base/ or on/ absent")
    print(f"  G162: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
