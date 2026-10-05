#!/usr/bin/env python3
"""G65: subjects bill their tier-2/3 range uplink (SY-1 for subjects, --subjectEdgeUplink;
TECH_DEBT N.3, register row 55).

usage: subject_uplink_gate.py <dir written by scripts/subject_uplink_gate.sh>

Runs read: on/, off/ (five subjects, static_2p4 and ca_tbl_full, tag 0's uplink billed in
both), r_noup (refusal: without --edgeUplinkRanges).

What a firing looks like on disk. At defaults energy feeds no decision, so the bill moves
only energy, only the subjects', and only on tiers 2 and 3: subject_summary's tx_2p4 / tx_uwb
(and j_2p4 / j_uwb / totalJ) rise, by exactly the airtime the SY-1 subjects line reports times
the radio's TX power, and nothing else moves -- tag 0's arm_summary and trace included.

Checks (each prints "G65 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  ONLY     arm_summary.csv and every trace_* byte-identical; subject_summary.csv identical
           outside {totalJ, j_2p4, j_uwb, tx_2p4, tx_uwb}; subjtrace_* identical outside cumJ.
  EXACT    per arm and subject, the rise in tx_2p4 is the line's 2.4 GHz airtime x 24 mA x
           3.3 V, and in tx_uwb the UWB airtime x 82 mA x 3.3 V (the SX/DW3000 TX currents of
           ca-tbl-energy-model.cc), to the CSV's 9 decimals.
  REACH    frames > 0 on 2.4 GHz for some subject (static_2p4 ranges there).
  SIX      a subject whose tier never transmitted (tx_ = 0 off) is not billed on it.
  CONTROL  off/: no subjects line, and the start-up WARNING names --edgeUplinkRanges as tag
           0's only; on/: it does not.
  R        the refusal exits 1 with the reason.
"""
import csv
import os
import re
import sys

MOVE = {"totalJ", "j_2p4", "j_uwb", "tx_2p4", "tx_uwb"}
PTX = {"2p4": 0.024 * 3.3, "uwb": 0.082 * 3.3}
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G65 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G65 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G65 MISSING {label:<8}{msg}")


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


def rows(p):
    try:
        return list(csv.DictReader(open(p)))
    except OSError:
        return None


SUBJ = re.compile(r"subject (\d+) 2\.4 GHz frames (\d+) records (\d+) airtime (\S+) s, UWB frames (\d+) records (\d+) airtime (\S+) s;")
HEAD = re.compile(r"SY-1 edge uplink \(subjects, (\w+)\):(.*)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    on, off = os.path.join(d, "on"), os.path.join(d, "off")
    same = []
    for f in sorted(os.listdir(off)):
        if f == "arm_summary.csv" or f.startswith("trace_"):
            if read(os.path.join(on, f)) != read(os.path.join(off, f)):
                same.append(f)
    s1, s0 = rows(os.path.join(on, "subject_summary.csv")), rows(os.path.join(off, "subject_summary.csv"))
    if s1 is None or s0 is None:
        missing("ONLY", "no subject_summary.csv")
        return 1
    for a, b in zip(s1, s0):
        for c in a:
            if c not in MOVE and a[c] != b[c]:
                same.append((a["arm"], a["subjectId"], c))
    for f in sorted(os.listdir(off)):
        if f.startswith("subjtrace_"):
            ta, tb = rows(os.path.join(on, f)), rows(os.path.join(off, f))
            if ta is None or len(ta) != len(tb):
                same.append((f, "rows"))
                continue
            for x, y in zip(ta, tb):
                if any(x[c] != y[c] for c in x if c != "cumJ"):
                    same.append((f, x["t"]))
                    break
    report(not same, "ONLY", f"tag 0's files and every non-energy subject column identical; moved {len(same)} (need 0)"
           + (f": {same[:3]}" if same else ""))

    lg = read(os.path.join(on, "on.log")) or read(os.path.join(d, "on.log")) or ""
    air = {}
    for m in HEAD.finditer(lg):
        for g in SUBJ.finditer(m.group(2)):
            air[(m.group(1), g.group(1))] = dict(f2=int(g.group(2)), a2=float(g.group(4)), fu=int(g.group(5)), au=float(g.group(7)))
    eb, six, reach = [], [], 0
    for a, b in zip(s1, s0):
        if a["subjectId"] == "1":
            if any(a[c] != b[c] for c in MOVE):
                eb.append((a["arm"], "tag 0 moved"))
            continue
        k = (a["arm"], a["subjectId"])
        if k not in air:
            eb.append((k, "no line entry"))
            continue
        x = air[k]
        reach += x["f2"]
        for t, key in (("2p4", "a2"), ("uwb", "au")):
            d_tx = float(a[f"tx_{t}"]) - float(b[f"tx_{t}"])
            want = x[key] * PTX[t]
            if abs(d_tx - want) > 3e-9 + 1e-7 * want:
                eb.append((k, t, d_tx, want))
            if float(b[f"tx_{t}"]) == 0.0 and float(a[f"tx_{t}"]) != 0.0:
                six.append((k, t))
    report(not eb and air, "EXACT", f"{len(air)} subject rows: the tx rise is the line's airtime x the TX power; off {len(eb)} (need 0)"
           + (f": {eb[:3]}" if eb else ""))
    report(reach > 0, "REACH", f"2.4 GHz uplink frames over the subjects {reach} (need > 0)")
    report(not six, "SIX", f"subjects billed on a tier they never transmitted on: {len(six)} (need 0)" + (f": {six[:3]}" if six else ""))
    offlog = read(os.path.join(d, "off.log")) or ""
    w_off = "--edgeUplinkRanges" in "".join(l for l in offlog.split("\n") if "WARNING" in l)
    w_on = "--edgeUplinkRanges" in "".join(l for l in lg.split("\n") if "WARNING" in l)
    report("SY-1 edge uplink (subjects" not in offlog and w_off and not w_on, "CONTROL",
           f"off/: subjects line {'SY-1 edge uplink (subjects' in offlog}, warning names it {w_off}; on/: warning names it {w_on} "
           f"(need False, True, False)")
    t = read(os.path.join(d, "r_noup.log")) or ""
    report(rc(d, "r_noup") == 1 and "--subjectEdgeUplink needs" in t, "R", f"without --edgeUplinkRanges: exit {rc(d, 'r_noup')} (need 1)")
    print(f"  G65: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
