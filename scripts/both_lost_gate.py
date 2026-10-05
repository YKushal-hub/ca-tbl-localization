#!/usr/bin/env python3
"""G83: both colliding frames are judged (--collisionBothLost; TECH_DEBT N.22).

usage: both_lost_gate.py <dir written by scripts/both_lost_gate.sh>

Runs read: on/ (the flag) and off/ (the control), seed 7, 900 s, defaults, the event log.

What a firing looks like on disk. By default the channel erases a frame when it completes,
so of two overlapping frames only the one ending first is lost. With the flag, a frame lost
to one that had already completed is an events_ row with outcome collision_late and
";by=<sender>:<packet id>", naming that frame. The defect's mode is ABSENCE: with it, no
such row exists, so the checks below count what is present and what each row names.

Checks (each prints "G83 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  LATE     on/: collision_late rows > 0; the N.22 line's two counts equal the collision and
           collision_late rows of events_.
  BY       on/: every collision_late row names a sender other than the receiver (the
           frame's own sender may be named: its earlier frame still on air, N.6); where the
           named frame appears in events_ (at any receiver,
           same tier, same sender and packet id), it ended no later than the lost frame and
           within 5 s of it, and at least one does appear.
  CONTROL  off/: no collision_late row and no N.22 line.
No magnitude is compared between on/ and off/: the flag changes every channel's RNG use
(rule 8).
"""
import csv
import os
import re
import sys

LINE = re.compile(r"N\.22 both colliding frames judged \(\w+\): collisions (\d+), of them against a completed frame (\d+)")
BY = re.compile(r";by=(\d+):(\d+)")
PKT = re.compile(r"(?:^|;)pkt=(\d+)")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G83 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G83 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G83 MISSING {label:<8}{msg}")


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


def events(d, n):
    try:
        with open(os.path.join(d, n, "events_ca_tbl_full_seed7.csv")) as fh:
            rd = csv.reader(fh)
            next(rd, None)
            return [(float(r[0]), r[1], r[2], r[3], r[5], ",".join(r[7:])) for r in rd if len(r) >= 8]
    except OSError:
        return None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    ev = events(d, "on")
    m = LINE.search(read(os.path.join(d, "on.log")) or "")
    if ev is None or m is None:
        missing("LATE", f"on/: events_ {ev is not None}, N.22 line {m is not None}")
        return 1
    late = [e for e in ev if e[4] == "collision_late"]
    coll = sum(e[4] == "collision" for e in ev)
    got = (int(m.group(1)), int(m.group(2)))
    report(late and got == (coll + len(late), len(late)), "LATE",
           f"on/: N.22 line {got}; events_ collision {coll}, collision_late {len(late)} (need > 0, and the line equal)")

    ends = {}
    for t, tier, tx, rx, out, pl in ev:
        p = PKT.search(pl)
        if p:
            ends.setdefault((tier, tx, p.group(1)), []).append(t)
    bb, seen = [], 0
    for t, tier, tx, rx, out, pl in late:
        b = BY.search(pl)
        if not b or b.group(1) == rx:
            bb.append((t, tier, tx, rx, "by", b.group(0) if b else None))
            continue
        ts = ends.get((tier, b.group(1), b.group(2)))
        if ts is None:
            continue
        seen += 1
        if not any(t - 5.0 <= x <= t + 1e-9 for x in ts):
            bb.append((t, tier, tx, rx, "named frame ended", ts[:3]))
    report(not bb and seen > 0, "BY", f"on/: {len(late)} late losses, {seen} naming a frame events_ shows elsewhere; "
           f"{sum(1 for e in late if (BY.search(e[5]) or [None, None])[1] == e[2])} by the frame's own sender (N.6); "
           f"bad sender, or a named frame that did not end before the loss within 5 s: {len(bb)} (need 0)"
           + (f": {bb[:2]}" if bb else ""))

    eo = events(d, "off")
    ol = LINE.search(read(os.path.join(d, "off.log")) or "") is not None
    nlo = None if eo is None else sum(e[4] == "collision_late" for e in eo)
    report(eo is not None and nlo == 0 and not ol, "CONTROL", f"off/: collision_late rows {nlo}, N.22 line {ol} (need 0, False)")
    print(f"  G83: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
