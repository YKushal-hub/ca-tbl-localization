#!/usr/bin/env python3
"""G174: searcher fixes DD 30-32. usage: search_doctrine_gate.py <dir of search_doctrine_gate.sh>
  RUNS   off and on exit 0 and write nodes_static_2p4_seed501.csv
  LINES  on announces DD30 (with > 0 polls on 2.4 GHz), DD31 and DD32; off announces none
  JUMP   off: some searcher moves > 1000 m in one step at the dispatch (the bug); on: no searcher moves more than
         5 m in one step at any time (1.2 m/s x 2.5 s plus GNSS slack)
  STEER  on: a searcher is within 10 m of the casualty before the end; off: none is
"""
import csv
import math
import os
import re
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G174 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def tracks(d, n):
    p = os.path.join(d, n, "nodes_static_2p4_seed501.csv")
    if not os.path.exists(p):
        return None
    T = {}
    for r in csv.DictReader(open(p)):
        T.setdefault(int(r["nodeId"]), []).append((float(r["t"]), float(r["x"]), float(r["y"])))
    return T


def stats(T):
    maxstep, reach = 0.0, False
    w = {t: (x, y) for t, x, y in T.get(1, [])}
    for i, tr in T.items():
        if i < 500:
            continue
        for (t0, x0, y0), (t1, x1, y1) in zip(tr, tr[1:]):
            maxstep = max(maxstep, math.hypot(x1 - x0, y1 - y0))
        reach = reach or any(t in w and math.dist((x, y), w[t]) <= 10.0 for t, x, y in tr if t >= 3000.0)
    return maxstep, reach


def main():
    d = sys.argv[1]
    rc = {n: (open(os.path.join(d, f"{n}.rc")).read().strip() if os.path.exists(os.path.join(d, f"{n}.rc")) else None) for n in ("off", "on")}
    T = {n: tracks(d, n) for n in ("off", "on")}
    report(rc == {"off": "0", "on": "0"} and None not in T.values(), "RUNS", f"exit codes {rc}")
    lg = {n: open(os.path.join(d, f"{n}.log"), errors="replace").read() if os.path.exists(os.path.join(d, f"{n}.log")) else "" for n in ("off", "on")}
    m = re.search(r"DD30 finderDigestOnArmTier \(static_2p4\): searcher polls on 2.4 GHz (\d+)", lg["on"])
    polls = int(m.group(1)) if m else 0
    ok = polls > 0 and "DD31 finderSweepFromDispatch" in lg["on"] and "DD32 finderLkpFirst" in lg["on"] and not re.search(r"DD3[012]", lg["off"])
    report(ok, "LINES", f"on polls {polls}, DD31 {'DD31' in lg['on']}, DD32 {'DD32' in lg['on']}, any in off {bool(re.search(r'DD3[012]', lg['off']))}")
    so = stats(T["off"]) if T["off"] else (float("nan"), None)
    sn = stats(T["on"]) if T["on"] else (float("nan"), None)
    report(so[0] > 1000.0 and sn[0] <= 5.0, "JUMP", f"largest searcher step off {so[0]:.0f} m, on {sn[0]:.1f} m")
    report(sn[1] is True and so[1] is False, "STEER", f"a searcher within 10 m: on {sn[1]}, off {so[1]}")
    print(f"  G174 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
