#!/usr/bin/env python3
"""G74: every radio half-duplex (--halfDuplex; TECH_DEBT N.6).

usage: half_duplex_gate.py <dir written by scripts/half_duplex_gate.sh>

Runs read: off/, count/ and enforce/ (seed 7, 900 s, defaults, the event log, --halfDuplex
0, 1 and 2), r_bad (refusal: --halfDuplex=3).

What a firing looks like on disk. The N.6 line counts, over every radio, the frames whose
transmission came due while the radio's own previous frame was on air, how many were sent
back to CAD, and the receptions of other nodes' frames that overlapped one of the
receiver's own. Count-only mode changes nothing else. Enforced, each deaf reception is an
events_ row with outcome halfduplex instead of a delivery.

Checks (each prints "G74 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     off, count and enforce exited 0.
  COUNT    count/: every output but run_id.txt byte-identical to off/, and receptions while
           transmitting > 0: the defect occurs at defaults, and measuring it moves nothing.
  ENFORCE  enforce/: every frame due over the radio's own was sent back to CAD; receptions
           while transmitting > 0, each an events_ row with outcome halfduplex (the line
           equals the rows).
  CONTROL  off/: no N.6 line and no halfduplex row.
  R        --halfDuplex=3 exits 1 with the reason.
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
        print(f"  G74 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G74 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G74 MISSING {label:<8}{msg}")


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


LINE = re.compile(r"N\.6 half-duplex \((count|enforce), \w+\): frames due over the radio's own (\d+), sent back to CAD (\d+), receptions while transmitting (\d+)")


def hdrows(d, n):
    try:
        return sum(1 for r in csv.DictReader(open(os.path.join(d, n, "events_ca_tbl_full_seed7.csv"))) if r["outcome"] == "halfduplex")
    except OSError:
        return None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("off", "count", "enforce") if rc(d, n) != 0]
    report(not bad, "RUNS", "off, count and enforce exited 0" if not bad else f"non-zero exit: {bad}")
    mc = LINE.search(read(os.path.join(d, "count.log")) or "")
    me = LINE.search(read(os.path.join(d, "enforce.log")) or "")
    if mc is None or me is None:
        missing("COUNT", f"no N.6 line: count {mc is not None}, enforce {me is not None}")
        return 1
    a, b = os.path.join(d, "off"), os.path.join(d, "count")
    fa, fb = sorted(os.listdir(a)), sorted(os.listdir(b))
    diff = [f for f in fa if f != "run_id.txt" and (f not in fb or open(os.path.join(a, f), "rb").read() != open(os.path.join(b, f), "rb").read())]
    ov, df, deaf = int(mc.group(2)), int(mc.group(3)), int(mc.group(4))
    report(fa == fb and not diff and deaf > 0 and df == 0, "COUNT", f"count/: identical to off/ besides run_id.txt {fa == fb and not diff}" + (f" (differing {diff[:3]})" if diff else "") + f"; frames due over own {ov}, sent back {df} (need 0), receptions while transmitting {deaf} (need > 0)")
    eov, edf, edeaf = int(me.group(2)), int(me.group(3)), int(me.group(4))
    rows = hdrows(d, "enforce")
    report(eov == edf and edeaf > 0 and rows == edeaf, "ENFORCE", f"enforce/: frames due over own {eov}, sent back to CAD {edf} (need equal); receptions lost {edeaf} (need > 0), halfduplex rows {rows} (need equal)")
    ol = LINE.search(read(os.path.join(d, "off.log")) or "") is not None
    orow = hdrows(d, "off")
    report(not ol and orow == 0, "CONTROL", f"off/: N.6 line {ol}, halfduplex rows {orow} (need False, 0)")
    t = read(os.path.join(d, "r_bad.log")) or ""
    report(rc(d, "r_bad") == 1 and "--halfDuplex is 0" in t, "R", f"--halfDuplex=3: exit {rc(d, 'r_bad')} (need 1)")
    print(f"  G74: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
