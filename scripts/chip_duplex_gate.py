#!/usr/bin/env python3
"""G84: one LR1121 serves 868 MHz and 2.4 GHz (--halfDuplexChip; TECH_DEBT N.6).

usage: chip_duplex_gate.py <dir written by scripts/chip_duplex_gate.sh>

Runs read: count/ (--halfDuplex=1), countchip/ (--halfDuplex=1 --halfDuplexChip),
enforcechip/ (--halfDuplex=2 --halfDuplexChip), all on --radioBasis=lr1121, seed 7, 900 s,
the event log; r_sx and r_nohd (refusals: on the SX basis, and without --halfDuplex).

What a firing looks like on disk. The N.6 chip line counts, over every node, the frames
that came due while the OTHER band of the same chip was transmitting, and the receptions
that overlapped the other band's transmission and none of the radio's own. Per radio, a
node's two LoRa bands never see each other, so without the flag both are 0 by construction.

Checks (each prints "G84 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     count, countchip and enforcechip exited 0.
  COUNT    countchip/: every output but run_id.txt byte-identical to count/; chips > 0 and
           receptions while the other band transmitted > 0; its N.6 line counts the same
           frames due over the radio's own as count/'s, and receptions while transmitting
           equal to count/'s plus the chip line's.
  ENFORCE  enforcechip/: every frame due over the radio's own or the other band's was sent
           back to CAD; receptions lost > 0, each an events_ row with outcome halfduplex.
  CONTROL  count/: no N.6 chip line.
  R        both refusals exit 1 with the reason.
"""
import csv
import os
import re
import sys

N6 = re.compile(r"N\.6 half-duplex \((count|enforce), \w+\): frames due over the radio's own (\d+), sent back to CAD (\d+), receptions while transmitting (\d+)")
CHIP = re.compile(r"N\.6 chip \(\w+\): chips (\d+), frames due over the other band's (\d+), receptions while the other band transmitted (\d+)")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G84 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G84 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G84 MISSING {label:<8}{msg}")


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
    bad = [n for n in ("count", "countchip", "enforcechip") if rc(d, n) != 0]
    report(not bad, "RUNS", "count, countchip and enforcechip exited 0" if not bad else f"non-zero exit: {bad}")
    lc, lcc, lec = (read(os.path.join(d, n + ".log")) or "" for n in ("count", "countchip", "enforcechip"))
    a, b, c = N6.search(lc), N6.search(lcc), N6.search(lec)
    cb, ce = CHIP.search(lcc), CHIP.search(lec)
    if None in (a, b, c, cb, ce):
        missing("COUNT", f"N.6 lines {[x is not None for x in (a, b, c)]}, chip lines {[x is not None for x in (cb, ce)]}")
        return 1
    p, q = os.path.join(d, "count"), os.path.join(d, "countchip")
    fa, fb = sorted(os.listdir(p)), sorted(os.listdir(q))
    diff = [f for f in fa if f != "run_id.txt" and (f not in fb or open(os.path.join(p, f), "rb").read() != open(os.path.join(q, f), "rb").read())]
    chips, cov, cdeaf = (int(x) for x in cb.groups())
    ok = (fa == fb and not diff and chips > 0 and cdeaf > 0 and int(b.group(2)) == int(a.group(2))
          and int(b.group(4)) == int(a.group(4)) + cdeaf and int(b.group(3)) == 0)
    report(ok, "COUNT", f"countchip/: identical to count/ besides run_id.txt {fa == fb and not diff}"
           + (f" (differing {diff[:3]})" if diff else "") + f"; chips {chips}, due over the other band {cov}, "
           f"receptions while it transmitted {cdeaf}; due over own {b.group(2)} vs {a.group(2)}, "
           f"receptions while transmitting {b.group(4)} vs {a.group(4)} + {cdeaf}")
    eov, edf, edeaf = int(c.group(2)), int(c.group(3)), int(c.group(4))
    ecov = int(ce.group(2))
    rows = hdrows(d, "enforcechip")
    report(eov + ecov == edf and edeaf > 0 and rows == edeaf, "ENFORCE",
           f"enforcechip/: due over own {eov} + over the other band {ecov}, sent back to CAD {edf} (need equal); "
           f"receptions lost {edeaf} (need > 0), halfduplex rows {rows} (need equal)")
    report(CHIP.search(lc) is None, "CONTROL", f"count/: N.6 chip line {CHIP.search(lc) is not None} (need False)")
    t1, t2 = read(os.path.join(d, "r_sx.log")) or "", read(os.path.join(d, "r_nohd.log")) or ""
    okr = (rc(d, "r_sx") == 1 and "--halfDuplexChip needs" in t1 and rc(d, "r_nohd") == 1 and "--halfDuplexChip needs" in t2)
    report(okr, "R", f"on the SX basis: exit {rc(d, 'r_sx')}; without --halfDuplex: exit {rc(d, 'r_nohd')} (need 1, 1)")
    print(f"  G84: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
