#!/usr/bin/env python3
"""G80: the MAC sect 2 time-on-air table (--toaTable).

usage: toa_table_gate.py <dir written by scripts/toa_table_gate.sh>

Runs read: on/ and off/ (seed 7, 60 s, the table on and off).

What a firing looks like on disk: toa_table.csv, one row per message type x SF 7..12 x
bandwidth 125/250/500 kHz, with the frame's bytes and its time on air from the MAC's own ToA
function; one "MAC sect 2 ToA table" line; the file digested in run_id.txt.

Checks (each prints "G80 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  ROWS     180 rows, each (type, SF, BW) once, ten message types.
  FORMULA  every toaS equals the Semtech formula recomputed here, independently (CR 4/5,
           8-symbol preamble, explicit header, CRC on, low-data-rate optimisation when the
           symbol exceeds 16 ms), to 1e-9 relative.
  DE       the low-data-rate flag is set on exactly SF11/125, SF12/125 and SF12/250 kHz.
  FRAME    frameBytes = macHeaderBytes (9) + payloadBytes; ToA never falls as SF rises.
  DIGEST   toa_table.csv has a digest line and it verifies.
  CONTROL  off/: no table, no line, and every other output identical besides run_id.txt.
"""
import csv
import hashlib
import math
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G80 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G80 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G80 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def semtech(sf, bw, pl, cr=1, npre=8, explicit=True, crc=True):
    ts = (2 ** sf) / bw
    de = 1 if ts > 0.016 else 0
    ih = 0 if explicit else 1
    n = 8 + max(math.ceil((8 * pl - 4 * sf + 28 + 16 * (1 if crc else 0) - 20 * ih) / (4 * (sf - 2 * de))) * (cr + 4), 0)
    return (npre + 4.25) * ts + n * ts


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    p = os.path.join(d, "on", "toa_table.csv")
    try:
        rows = list(csv.DictReader(open(p)))
    except OSError:
        missing("ROWS", "no toa_table.csv in on/")
        return 1
    keys = {(r["msgType"], r["sf"], r["bwHz"]) for r in rows}
    types = {r["msgType"] for r in rows}
    report(len(rows) == 180 and len(keys) == 180 and len(types) == 10, "ROWS", f"rows {len(rows)}, distinct (type, SF, BW) {len(keys)}, types {len(types)} (need 180, 180, 10)")
    badf = []
    for r in rows:
        want = semtech(int(r["sf"]), float(r["bwHz"]), int(r["frameBytes"]))
        got = float(r["toaS"])
        if abs(got - want) > 1e-9 * max(1.0, want):
            badf.append((r["msgType"], r["sf"], r["bwHz"], got, want))
    report(not badf and rows, "FORMULA", f"rows off the independent Semtech formula {len(badf)} (need 0)" + (f": {badf[:2]}" if badf else ""))
    de = {(int(r["sf"]), int(float(r["bwHz"]))) for r in rows if r["lowDataRateOpt"] == "1"}
    report(de == {(11, 125000), (12, 125000), (12, 250000)}, "DE", f"low-data-rate rows at {sorted(de)} (need SF11/125k, SF12/125k, SF12/250k)")
    badb = [r["msgType"] for r in rows if int(r["frameBytes"]) != int(r["macHeaderBytes"]) + int(r["payloadBytes"]) or r["macHeaderBytes"] != "9"]
    mono = []
    for t in types:
        for bw in {r["bwHz"] for r in rows}:
            seq = sorted(((int(r["sf"]), float(r["toaS"])) for r in rows if r["msgType"] == t and r["bwHz"] == bw))
            if any(b[1] < a[1] for a, b in zip(seq, seq[1:])):
                mono.append((t, bw))
    report(not badb and not mono, "FRAME", f"rows with frame != 9 + payload {len(badb)}; (type, BW) where ToA falls as SF rises {mono} (need none)")
    rid = open(os.path.join(d, "on", "run_id.txt")).read() if os.path.exists(os.path.join(d, "on", "run_id.txt")) else ""
    m = re.search(r"^([0-9a-f]{64})  toa_table\.csv$", rid, re.M)
    okd = m is not None and m.group(1) == hashlib.sha256(open(p, "rb").read()).hexdigest()
    report(okd, "DIGEST", f"toa_table.csv digested and verifying {okd}")
    on, off = os.path.join(d, "on"), os.path.join(d, "off")
    fa = sorted(f for f in os.listdir(on) if f != "toa_table.csv")
    fb = sorted(os.listdir(off))
    diff = [f for f in fa if f != "run_id.txt" and (f not in fb or open(os.path.join(on, f), "rb").read() != open(os.path.join(off, f), "rb").read())]
    offl = "MAC sect 2 ToA table" in (open(os.path.join(d, "off.log")).read() if os.path.exists(os.path.join(d, "off.log")) else "")
    report(fa == fb and not diff and "toa_table.csv" not in fb and not offl, "CONTROL", f"off/: table {'toa_table.csv' in fb}, line {offl}; other outputs differing {diff} (need False, False, [])")
    sf12 = [float(r["toaS"]) for r in rows if r["sf"] == "12" and r["bwHz"] == "125000"]
    print(f"  G80 note    SF12/125 kHz frames take {min(sf12):.3f}-{max(sf12):.3f} s (the spec: a ~50-byte packet, 1.5-2+ s)")
    print(f"  G80: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
