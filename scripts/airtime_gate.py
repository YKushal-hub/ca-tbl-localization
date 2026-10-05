#!/usr/bin/env python3
"""G86: airtime per tier and message type (--reportAirtime; MAC sect 2, Transport sect 7).

usage: airtime_gate.py <dir written by scripts/airtime_gate.sh>

Runs read (the preset, full arm, seed 7, 900 s): on/ (the report), off/ (without it, the
control), flood/ (the report with --reversePath=false: RESPONSEs flooded).

What a firing looks like on disk. airtime_<arm>_seed7.csv: per tier, one row per message
type (frames sent, of them relays, airtime) summed over every MAC on that tier, and a
FULL_PUSH_CF row (the airtime of pushing the whole table instead of each DIGEST sent). The
AIRTIME line prints, per tier, the MACs' frame total beside the channel's own count of the
frames it carried. The defect this guards: a report whose rows do not add up to what the
channel carried, or a report that changes the run.

Checks (each prints "G86 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on, off and flood exited 0.
  SAME     on/ and off/ hold the same files besides airtime_* and run_id.txt, byte-identical.
  TALLY    on/ and flood/: per tier, the rows' frames equal the channel's count in the AIRTIME
           line, and the line's digest, entry-push, full-push and response seconds equal the
           rows'.
  CF       on/: DIGEST frames > 0 on some tier, and every such tier has FULL_PUSH_CF > 0.
  FLOOD    flood/ flooded its responses and unicast none, on/ the reverse: the pair is the
           comparison it is read as.
  CONTROL  off/: no airtime_ file and no AIRTIME line.
"""
import csv
import glob
import os
import re
import sys

TIER = re.compile(r"tier (\d) frames (\d+) \(channel (\d+)\), digest ([\d.e+-]+) s \+ entry push ([\d.e+-]+) s "
                  r"against full push ([\d.e+-]+) s, response ([\d.e+-]+) s;")
NET = re.compile(r"NET4/TRANS1: proxy answered \d+, proxy refused stale \d+, responses unicast (\d+), flooded (\d+)")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G86 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G86 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G86 MISSING {label:<8}{msg}")


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


def rows(d, n):
    try:
        return list(csv.DictReader(open(os.path.join(d, n, "airtime_ca_tbl_full_seed7.csv"))))
    except OSError:
        return None


def close(a, b):
    return abs(a - b) <= 1e-5 * max(1.0, abs(a), abs(b))


def tally(d, n):
    rr = rows(d, n)
    line = read(os.path.join(d, n + ".log")) or ""
    tl = {int(m.group(1)): m.groups()[1:] for m in TIER.finditer(line)}
    if rr is None or len(tl) != 3:
        return None
    bad = []
    for t in range(3):
        tr = [r for r in rr if int(r["tier"]) == t]
        air = {r["msgType"]: float(r["airtimeS"]) for r in tr}
        frames = sum(int(r["framesSent"]) for r in tr if r["msgType"] != "FULL_PUSH_CF")
        f, ch, dg, ep, cf, rs = tl[t]
        if not (frames == int(f) == int(ch)):
            bad.append((n, t, "frames", frames, f, ch))
        for k, v in (("DIGEST", dg), ("ENTRY_PUSH", ep), ("FULL_PUSH_CF", cf), ("RESPONSE", rs)):
            if not close(air.get(k, -1.0), float(v)):
                bad.append((n, t, k, air.get(k), v))
    return bad


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off", "flood") if rc(d, n) != 0]
    report(not bad, "RUNS", "on, off and flood exited 0" if not bad else f"non-zero exit: {bad}")
    a, b = os.path.join(d, "on"), os.path.join(d, "off")
    fa = sorted(f for f in os.listdir(a) if not f.startswith("airtime_") and f != "run_id.txt")
    fb = sorted(f for f in os.listdir(b) if f != "run_id.txt")
    diff = [f for f in fa if f not in fb or open(os.path.join(a, f), "rb").read() != open(os.path.join(b, f), "rb").read()]
    report(fa == fb and not diff, "SAME", f"on/ and off/ besides airtime_ and run_id.txt: same set {fa == fb}, differing {diff[:3]}")
    tb = [tally(d, n) for n in ("on", "flood")]
    if None in tb:
        missing("TALLY", "on/ or flood/ lacks the airtime file or a full AIRTIME line")
        return 1
    tb = tb[0] + tb[1]
    report(not tb, "TALLY", f"rows against the channel and the AIRTIME line, on/ and flood/: {len(tb)} mismatches (need 0)"
           + (f": {tb[:2]}" if tb else ""))
    rr = rows(d, "on")
    dt = [t for t in range(3) if any(int(r["tier"]) == t and r["msgType"] == "DIGEST" and int(r["framesSent"]) > 0 for r in rr)]
    cfz = [t for t in dt if not any(int(r["tier"]) == t and r["msgType"] == "FULL_PUSH_CF" and float(r["airtimeS"]) > 0 for r in rr)]
    report(dt and not cfz, "CF", f"on/: tiers with DIGEST frames {dt} (need some); of them without a full-push counterfactual {cfz} (need none)")
    mo, mf = NET.search(read(os.path.join(d, "on.log")) or ""), NET.search(read(os.path.join(d, "flood.log")) or "")
    okf = (mo is not None and mf is not None and int(mo.group(1)) > 0 and int(mo.group(2)) == 0
           and int(mf.group(1)) == 0 and int(mf.group(2)) > 0)
    report(okf, "FLOOD", f"responses unicast/flooded: on {mo.groups() if mo else None} (need >0/0), "
           f"flood {mf.groups() if mf else None} (need 0/>0)")
    of = glob.glob(os.path.join(d, "off", "airtime_*"))
    ol = "AIRTIME (" in (read(os.path.join(d, "off.log")) or "")
    report(not of and not ol, "CONTROL", f"off/: airtime_ files {len(of)}, AIRTIME line {ol} (need 0, False)")
    print(f"  G86: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
