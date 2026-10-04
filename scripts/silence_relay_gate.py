#!/usr/bin/env python3
"""G70: a silenced radio relays no other node's frame (--silenceStopsRelay; TECH_DEBT N.1).

usage: silence_relay_gate.py <dir written by scripts/silence_relay_gate.sh>

Runs read: on/ and off/ (G34's regime: seed 7, 900 s, a 3 J battery, triggered updates,
the event log), sil/ (node 1 mute from 300 to 600 s under --injectSilentId, the flag on),
r_none (refusal: neither fault).

What a firing looks like on disk: events_ca_tbl_full_seed7.csv has one row per frame per
receiver, with the transmitter (txId) and the frame's remaining hop limit (hops= in the
payload). An originator sends a frame with the full hop limit and each relay decrements it,
so a frame whose hop limit is below the highest seen for its type is a relay. After node 1's
battery dies its own frames stop (SetSilent gates Send). Under the defect its radios go on
relaying: rows with txId 1 keep appearing, every one of them a relay. A frame node 1 queued before the silence may still leave in the next
few seconds (the last gasp is one), so every check allows MARGIN seconds after the silence.

Checks (each prints "G70 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on, off and sil exited 0.
  DEFECT   off/: the battery died, and node 1 transmitted after it (rows with txId 1 later
           than the death + MARGIN), some of them relays (a hop limit below its type's
           highest): the defect reproduces, so the fix is tested against something.
  FIX      on/: the battery died, no row with txId 1 later than the death + MARGIN, and the
           N.1 line counts relays refused > 0.
  GASP     on/: the last gasp still left: a TRIGGERED_UPDATE from node 1 heard within
           MARGIN of the death.
  SIL      sil/: no row with txId 1 in (300 + MARGIN, 600); node 1 transmits again after
           600 (the fault ends); relays refused > 0.
  CONTROL  off/: no N.1 line.
  R        the refusal exits 1 with the reason.
"""
import csv
import os
import re
import sys

MARGIN = 5.0
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G70 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G70 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G70 MISSING {label:<8}{msg}")


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
        return list(csv.DictReader(open(os.path.join(d, n, "events_ca_tbl_full_seed7.csv"))))
    except OSError:
        return None


def death(d, n):
    m = re.search(r"FAULT sect 49: battery exhausted at t=([0-9.]+) s", read(os.path.join(d, n + ".log")) or "")
    return float(m.group(1)) if m else None


def refused(d, n):
    m = re.search(r"N\.1 silenced radios \(\w+\): relays refused (\d+)", read(os.path.join(d, n + ".log")) or "")
    return int(m.group(1)) if m else None


def hops(r):
    m = re.search(r"hops=(\d+)", r["payload"])
    return int(m.group(1)) if m else None


def relayed(rows):
    """A predicate: the row's hop limit is below the highest seen for its message type."""
    top = {}
    for r in rows:
        h = hops(r)
        if h is not None:
            top[r["msgType"]] = max(top.get(r["msgType"], 0), h)
    return lambda r: hops(r) is not None and hops(r) < top.get(r["msgType"], 0)


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off", "sil") if rc(d, n) != 0]
    report(not bad, "RUNS", "on, off and sil exited 0" if not bad else f"non-zero exit: {bad}")
    ev = {n: events(d, n) for n in ("on", "off", "sil")}
    if any(v is None for v in ev.values()):
        missing("DEFECT", f"no events file in {[n for n, v in ev.items() if v is None]}")
        return 1
    td = death(d, "off")
    late = [r for r in ev["off"] if r["txId"] == "1" and td is not None and float(r["t"]) > td + MARGIN]
    isrelay = relayed(ev["off"])
    nf = sum(1 for r in late if isrelay(r))
    report(td is not None and late and nf > 0, "DEFECT", f"off/: battery died at {td}; rows with txId 1 after it {len(late)}, relays among them {nf} (need > 0, > 0)")
    tn = death(d, "on")
    lateOn = [r for r in ev["on"] if r["txId"] == "1" and tn is not None and float(r["t"]) > tn + MARGIN]
    ref = refused(d, "on")
    report(tn is not None and not lateOn and (ref or 0) > 0, "FIX", f"on/: battery died at {tn}; rows with txId 1 after it {len(lateOn)} (need 0); relays refused {ref} (need > 0)" + (f": {[(r['t'], r['msgType']) for r in lateOn[:3]]}" if lateOn else ""))
    gasp = [r for r in ev["on"] if r["txId"] == "1" and r["msgType"] == "TRIGGERED_UPDATE" and tn is not None and tn <= float(r["t"]) <= tn + MARGIN]
    report(len(gasp) > 0, "GASP", f"on/: TRIGGERED_UPDATE rows from node 1 within {MARGIN} s of the death {len(gasp)} (need > 0)")
    inWin = [r for r in ev["sil"] if r["txId"] == "1" and 300.0 + MARGIN < float(r["t"]) < 600.0]
    after = [r for r in ev["sil"] if r["txId"] == "1" and float(r["t"]) >= 600.0]
    rs = refused(d, "sil")
    report(not inWin and after and (rs or 0) > 0, "SIL", f"sil/: rows with txId 1 inside the window {len(inWin)} (need 0), after it {len(after)} (need > 0); relays refused {rs} (need > 0)")
    ol = refused(d, "off")
    report(ol is None, "CONTROL", f"off/: N.1 line {ol is not None} (need False)")
    t = read(os.path.join(d, "r_none.log")) or ""
    report(rc(d, "r_none") == 1 and "--silenceStopsRelay needs" in t, "R", f"neither fault: exit {rc(d, 'r_none')} (need 1)")
    print(f"  G70: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
