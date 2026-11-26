#!/usr/bin/env python3
"""G175: --reacquireSigmaM (DD 34). usage: reacquire_gate.py <dir of reacquire_gate.sh>
  RUNS     off and on exit 0 and write the five tracked nodes' traces
  LINE     on reports "DD34 reacquireSigmaM (ca_tbl_full): re-entries over all tracked nodes N" with N > 0; off has no DD34
  CONSIST  "confidently wrong" cycles (error > 500 m while sqrt(trace P) < 50 m) over the five nodes: off > 1000 (the
           failure the flag is for is present) and on < 5 % of off
"""
import csv
import glob
import os
import re
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G175 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def bad(d, n):
    fs = glob.glob(os.path.join(d, n, "subjtrace_ca_tbl_full_seed541_subj*.csv")) + glob.glob(os.path.join(d, n, "trace_ca_tbl_full_seed541.csv"))
    if len(fs) < 5:
        return None
    k = 0
    for f in fs:
        for r in csv.DictReader(open(f)):
            k += float(r["errM"]) > 500.0 and float(r["traceSigma"]) ** 0.5 < 50.0
    return k


def main():
    d = sys.argv[1]
    rc = {n: (open(os.path.join(d, f"{n}.rc")).read().strip() if os.path.exists(os.path.join(d, f"{n}.rc")) else None) for n in ("off", "on")}
    b = {n: bad(d, n) for n in ("off", "on")}
    report(rc == {"off": "0", "on": "0"} and None not in b.values(), "RUNS", f"exit codes {rc}")
    lg = {n: open(os.path.join(d, f"{n}.log"), errors="replace").read() if os.path.exists(os.path.join(d, f"{n}.log")) else "" for n in ("off", "on")}
    m = re.search(r"DD34 reacquireSigmaM \(ca_tbl_full\): re-entries over all tracked nodes (\d+)", lg["on"])
    n = int(m.group(1)) if m else 0
    report(n > 0 and "DD34" not in lg["off"], "LINE", f"on re-entries {n}, DD34 in off {'DD34' in lg['off']}")
    ok = b["off"] is not None and b["on"] is not None and b["off"] > 1000 and b["on"] < 0.05 * b["off"]
    report(ok, "CONSIST", f"confidently wrong cycles off {b['off']}, on {b['on']}")
    print(f"  G175 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
