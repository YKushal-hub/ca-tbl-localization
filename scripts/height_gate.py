#!/usr/bin/env python3
"""G172: --rangeHeightCorrect (DD 28). usage: height_gate.py <dir of height_gate.sh>
  RUNS   off and on exit 0 and write trace_static_868_seed7.csv
  LINE   on's log has "DD28 rangeHeightCorrect (static_868)" with a count > 0; off's log has no DD28 line
  TRUTH  the true path (trueX, trueY) is identical in off and on: the flag moves the estimate, not the world
  MOVES  the estimate (estX, estY) differs between off and on on at least one cycle
"""
import csv
import os
import re
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G172 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rows(d, n):
    p = os.path.join(d, n, "trace_static_868_seed7.csv")
    if not os.path.exists(p):
        return None
    return [(r["t"], r["trueX"], r["trueY"], r["estX"], r["estY"]) for r in csv.DictReader(open(p))]


def main():
    d = sys.argv[1]
    rc = {n: (open(os.path.join(d, f"{n}.rc")).read().strip() if os.path.exists(os.path.join(d, f"{n}.rc")) else None) for n in ("off", "on")}
    tr = {n: rows(d, n) for n in ("off", "on")}
    report(rc == {"off": "0", "on": "0"} and None not in tr.values(), "RUNS", f"exit codes {rc}")
    lg = {n: open(os.path.join(d, f"{n}.log"), errors="replace").read() if os.path.exists(os.path.join(d, f"{n}.log")) else "" for n in ("off", "on")}
    m = re.search(r"DD28 rangeHeightCorrect \(static_868\).* (\d+)\s*$", lg["on"], re.M)
    cnt = int(m.group(1)) if m else 0
    report(cnt > 0 and "DD28" not in lg["off"], "LINE", f"on count {cnt}, DD28 in off {'DD28' in lg['off']}")
    a, b = tr["off"] or [], tr["on"] or []
    same_truth = len(a) == len(b) and len(a) > 0 and all(x[:3] == y[:3] for x, y in zip(a, b))
    report(same_truth, "TRUTH", f"{len(a)} / {len(b)} cycles, truth identical {same_truth}")
    ndiff = sum(1 for x, y in zip(a, b) if x[3:] != y[3:])
    report(ndiff > 0, "MOVES", f"{ndiff} cycles with a different estimate")
    print(f"  G172 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
