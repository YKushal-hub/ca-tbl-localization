#!/usr/bin/env python3
"""G75: L.1.5's RECEDING eager push is counted (--reportRecedingPushes; TECH_DEBT M.9).

usage: receding_push_gate.py <dir written by scripts/receding_push_gate.sh>

Runs read: on/ and off/ (seed 7, 900 s, --mobilityFromRange, triggered updates off, the event
log), r_nomob (refusal: without --mobilityFromRange).

What a firing looks like on disk. With MC-2's triggers off, the only TRIGGERED_UPDATE tag 0
(node 1) originates is the RECEDING push. In events_*, one row per frame per receiver, an
originated frame carries the full hop limit (the highest hops= seen for its type) and a
relay a lower one, so the pushes heard are the distinct packet ids of TRIGGERED_UPDATE rows
from node 1 at the full hop limit. A push transmitted and heard by nobody leaves no row.

Checks (each prints "G75 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  COUNT    on/: the M.9 line reports handshakes on the RECEDING branch > 0 and push frames
           transmitted > 0, at most 3 a push (TRIGGERED_UPDATE is the redundant class: a
           first frame and up to two jittered repeats, one packet id).
  HEARD    on/: distinct pushes heard in events_ > 0, and at most the handshakes started.
  SAME     on/ and off/ hold the same files, byte-identical except run_id.txt: a report only.
  CONTROL  off/: no M.9 line.
  R        the refusal exits 1 with the reason.
"""
import csv
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G75 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G75 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G75 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def read(p):
    try:
        return open(p).read()
    except OSError:
        return None


LINE = re.compile(r"M\.9 RECEDING pushes \(\w+\): handshakes on the RECEDING branch (\d+), push frames transmitted (\d+)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    m = LINE.search(read(os.path.join(d, "on.log")) or "")
    if m is None:
        missing("COUNT", "no M.9 line in on.log")
        return 1
    started, sent = int(m.group(1)), int(m.group(2))
    report(started > 0 and 0 < sent <= 3 * started, "COUNT", f"on/: handshakes on the RECEDING branch {started}, push frames transmitted {sent} (need > 0, 0 < frames <= 3 x started)")
    try:
        rows = list(csv.DictReader(open(os.path.join(d, "on", "events_ca_tbl_full_seed7.csv"))))
    except OSError:
        missing("HEARD", "no events file in on/")
        return 1
    hops = lambda r: int(re.search(r"hops=(\d+)", r["payload"]).group(1))
    tu = [r for r in rows if r["msgType"] == "TRIGGERED_UPDATE"]
    top = max((hops(r) for r in tu), default=0)
    heard = {re.search(r"pkt=(\d+)", r["payload"]).group(1) for r in tu if r["txId"] == "1" and hops(r) == top}
    report(0 < len(heard) <= started, "HEARD", f"on/: distinct pushes from node 1 heard {len(heard)} (need > 0 and <= started {started})")
    on, off = os.path.join(d, "on"), os.path.join(d, "off")
    fa, fb = sorted(os.listdir(on)), sorted(os.listdir(off))
    diff = [f for f in fa if f != "run_id.txt" and (f not in fb or open(os.path.join(on, f), "rb").read() != open(os.path.join(off, f), "rb").read())]
    report(fa == fb and not diff, "SAME", f"on/ and off/: same file set {fa == fb}; differing besides run_id.txt {diff}")
    ol = LINE.search(read(os.path.join(d, "off.log")) or "") is not None
    report(not ol, "CONTROL", f"off/: M.9 line {ol} (need False)")
    t = read(os.path.join(d, "r_nomob.log")) or ""
    report(rc(d, "r_nomob") == 1 and "--reportRecedingPushes needs" in t, "R", f"without --mobilityFromRange: exit {rc(d, 'r_nomob')} (need 1)")
    print(f"  G75: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
