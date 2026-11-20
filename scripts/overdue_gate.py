#!/usr/bin/env python3
"""G171: the overdue alarm alone dispatches the party. usage: overdue_gate.py <dir of overdue_gate.sh>
  RUNS  none and od exit 0 and write nodes_static_2p4_seed7.csv
  LINE  od's log reports "reported overdue -- party deployed at t=600.0 s"; none's has no ALERT line
  MOVE  od: no searcher (node >= 500) moves more than 5 m before t = 600 s and one does after; none (no alarm
        configured, so the party leaves at once by design): one moves before t = 600 s
"""
import csv
import math
import os
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G171 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def moved(d, n):
    p = os.path.join(d, n, "nodes_static_2p4_seed7.csv")
    if not os.path.exists(p):
        return None
    start, before, after = {}, False, False
    for r in csv.DictReader(open(p)):
        i = int(r["nodeId"])
        if i < 500:
            continue
        xy = (float(r["x"]), float(r["y"]))
        start.setdefault(i, xy)
        if math.dist(xy, start[i]) > 5.0:
            if float(r["t"]) < 600.0:
                before = True
            else:
                after = True
    return before, after


def main():
    d = sys.argv[1]
    rc = {n: (open(os.path.join(d, f"{n}.rc")).read().strip() if os.path.exists(os.path.join(d, f"{n}.rc")) else None) for n in ("none", "od")}
    mv = {n: moved(d, n) for n in ("none", "od")}
    report(rc == {"none": "0", "od": "0"} and None not in mv.values(), "RUNS", f"exit codes {rc}")
    lg = {n: open(os.path.join(d, f"{n}.log"), errors="replace").read() if os.path.exists(os.path.join(d, f"{n}.log")) else "" for n in ("none", "od")}
    report("reported overdue -- party deployed at t=600.0 s" in lg["od"] and "ALERT" not in lg["none"], "LINE",
           f"od line {'reported overdue -- party deployed at t=600.0 s' in lg['od']}, ALERT in none {'ALERT' in lg['none']}")
    ok = mv["od"] == (False, True) and mv["none"] is not None and mv["none"][0]
    report(ok, "MOVE", f"searchers moved (before 600 s, after): od {mv['od']}, none {mv['none']} (need (False, True); none moving before 600 s)")
    print(f"  G171 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
