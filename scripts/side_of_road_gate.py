#!/usr/bin/env python3
"""G173: --acquireSideOfRoad (DD 29). usage: side_of_road_gate.py <dir of side_of_road_gate.sh>
  RUNS    off and on exit 0 and write trace_static_2p4_seed501.csv
  LINE    on's log has "DD29 acquireSideOfRoad (static_2p4): cloud confined to y <= 2750" with a cut count > 0; off has none
  REFUSE  without --gwRidge the flag is refused: rc != 0 and a FATAL naming --acquireSideOfRoad
  SIDE    on: the lost worker's estimate never lies beyond the line (estY <= 2750.5 on every cycle); off on this seed
          does cross it (the mirror root the flag exists for)
"""
import csv
import os
import re
import sys

nok = nfail = 0
LINE_Y = 2750.0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G173 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rd(d, n, f):
    p = os.path.join(d, f"{n}{f}")
    return open(p, errors="replace").read().strip() if os.path.exists(p) else None


def ys(d, n):
    p = os.path.join(d, n, "trace_static_2p4_seed501.csv")
    if not os.path.exists(p):
        return None
    return [float(r["estY"]) for r in csv.DictReader(open(p))]


def main():
    d = sys.argv[1]
    rc = {n: rd(d, n, ".rc") for n in ("off", "on", "nor")}
    y = {n: ys(d, n) for n in ("off", "on")}
    report(rc["off"] == "0" and rc["on"] == "0" and None not in y.values(), "RUNS", f"exit codes {rc}")
    lg = {n: rd(d, n, ".log") or "" for n in ("off", "on", "nor")}
    m = re.search(r"DD29 acquireSideOfRoad \(static_2p4\): cloud confined to y <= 2750\S* m; tag-0 particle-updates zeroed "
                  r"across the line (\d+)", lg["on"])
    cuts = int(m.group(1)) if m else 0
    report(cuts > 0 and "DD29" not in lg["off"], "LINE", f"on cuts {cuts}, DD29 in off {'DD29' in lg['off']}")
    report(rc["nor"] not in (None, "0") and "FATAL: --acquireSideOfRoad" in lg["nor"], "REFUSE", f"rc {rc['nor']}")
    on_max = max(y["on"]) if y["on"] else float("nan")
    off_max = max(y["off"]) if y["off"] else float("nan")
    report(on_max <= LINE_Y + 0.5 and off_max > LINE_Y + 0.5, "SIDE", f"max estY on {on_max:.1f}, off {off_max:.1f} (line {LINE_Y})")
    print(f"  G173 summary: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
