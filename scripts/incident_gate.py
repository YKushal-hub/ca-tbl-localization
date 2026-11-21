#!/usr/bin/env python3
"""G176: --incidentRandom (DD 35). usage: incident_gate.py <dir of incident_gate.sh>
  RUNS    all four runs exit 0 and write nodes_static_868_seed<N>.csv
  LINE    on runs print "DD35 incident"; off runs do not
  VARY    off: the casualty stands at the same point at the alarm on both seeds (the defect); on: > 300 m apart
  INSIDE  on: the casualty's track stays inside 0.98 of the box half-width and is never pinned to the edge
"""
import csv
import math
import os
import sys

nok = nfail = 0
BOX = 5000.0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G176 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def track(d, n, s):
    p = os.path.join(d, n, f"nodes_static_868_seed{s}.csv")
    if not os.path.exists(p):
        return None
    return [(float(r["t"]), float(r["x"]), float(r["y"])) for r in csv.DictReader(open(p)) if r["nodeId"] == "1"]


def at(tr, t):
    return next(((x, y) for tt, x, y in tr if abs(tt - t) < 1.0), None)


def main():
    d = sys.argv[1]
    names = [(f"{k}{s}", s) for k in ("off", "on") for s in (601, 602)]
    rc = {n: (open(os.path.join(d, f"{n}.rc")).read().strip() if os.path.exists(os.path.join(d, f"{n}.rc")) else None) for n, _ in names}
    T = {n: track(d, n, s) for n, s in names}
    report(all(v == "0" for v in rc.values()) and all(T.values()), "RUNS", f"exit codes {rc}")
    lg = {n: open(os.path.join(d, f"{n}.log"), errors="replace").read() if os.path.exists(os.path.join(d, f"{n}.log")) else "" for n, _ in names}
    report(all("DD35 incident" in lg[f"on{s}"] and "DD35" not in lg[f"off{s}"] for s in (601, 602)), "LINE", "on prints the incident, off does not")
    if all(T.values()):
        doff = math.dist(at(T["off601"], 3000.0), at(T["off602"], 3000.0))
        don = math.dist(at(T["on601"], 3000.0), at(T["on602"], 3000.0))
    else:
        doff = don = float("nan")
    report(doff < 1.0 and don > 300.0, "VARY", f"casualty 601 vs 602 at the alarm: off {doff:.1f} m apart, on {don:.0f} m apart")
    edge = max((max(abs(x), abs(y)) for n in ("on601", "on602") for _, x, y in (T[n] or [])), default=float("nan"))
    report(edge <= 0.98 * BOX + 1.0, "INSIDE", f"largest |x|,|y| of the on tracks {edge:.0f} m (limit {0.98 * BOX:.0f})")
    print(f"  G176 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
