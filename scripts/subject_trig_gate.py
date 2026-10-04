#!/usr/bin/env python3
"""G66: subjects send MC-2's large correction (--subjectTriggeredUpdates; TECH_DEBT N.3,
register row 55).

usage: subject_trig_gate.py <dir written by scripts/subject_trig_gate.sh>

Runs read: on/ and off/ (G34's regime, five subjects, tag 0's triggered updates on in both),
r_notu (refusal: without --triggeredUpdates).

What a firing looks like on disk: subjtriggered_ca_tbl_full_seed7.csv gets one row per
subject large correction, and the subject's TRIGGERED_UPDATE frames (reason=1, packet ids
carrying the subject's node id) appear in the events file within 2 s of it.

Checks (each prints "G66 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  ROWS     large-correction rows > 0.
  LARGE    every row agrees with the subject's own trace, an independent record: estX/estY are
           the subjtrace's at that t; threshM is k sqrt(traceSigma) of the subject's previous
           trace row (k = 0.5, the runner's); moveM is |est - pred| from the row's columns and exceeds it; the
           tier is the subjtrace's activeTier at that t (sect 6).
  FRAMES   every reason=1 frame a subject originated, as heard in events_*, follows one of its
           large-correction rows by at most 2 s, and some are heard.
  COUNTS   the MC-2 subjects line equals the file (rows; 3 frames billed each).
  CONTROL  off/: no subjtriggered_ file, no MC-2 subjects line, no subject reason=1 frame.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import math
import os
import re
import sys

nok = nfail = nmiss = 0
K = 0.5  # the runner's --triggeredUpdateK


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G66 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G66 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G66 MISSING {label:<8}{msg}")


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


def field(p, k):
    m = re.search(r"(?:^|;)" + k + r"=(\d+)", p)
    return int(m.group(1)) if m else None


def subj_frames(ev, subjects):
    """{(tier, pkt): (first t, node, reason)} for frames a subject ORIGINATED."""
    out = {}
    for r in ev or []:
        pkt = field(r["payload"], "pkt")
        if pkt is None or (pkt >> 20) not in subjects:
            continue
        k = (r["tier"], pkt)
        t = float(r["t"])
        if k not in out or t < out[k][0]:
            out[k] = (t, pkt >> 20, field(r["payload"], "reason"))
    return out


TIERN = {"868MHz": 0, "2.4GHz": 1, "UWB": 2}
LINE = re.compile(r"MC-2 triggered updates \(subjects, \w+\): large correction (\d+), frames billed (\d+)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    tg = rows(os.path.join(d, "on", "subjtriggered_ca_tbl_full_seed7.csv"))
    if tg is None:
        missing("ROWS", "no subjtriggered_ca_tbl_full_seed7.csv in on/")
        return 1
    subjects = sorted({int(r["subjectId"]) for r in tg})
    report(len(tg) > 0, "ROWS", f"large-correction rows {len(tg)} from subjects {subjects} (need > 0)")

    trs = {}
    for f in glob.glob(os.path.join(d, "on", "subjtrace_ca_tbl_full_seed7_subj*.csv")):
        sid = int(re.search(r"_subj(\d+)\.csv$", f).group(1))
        trs[sid] = rows(f)
    lb = []
    for r in tg:
        sid, t = int(r["subjectId"]), float(r["t"])
        tr = trs.get(sid) or []
        idx = next((i for i, x in enumerate(tr) if abs(float(x["t"]) - t) < 1e-3), None)
        if idx is None or idx == 0:
            lb.append((r["t"], sid, "no trace row or no previous one"))
            continue
        x, prev = tr[idx], tr[idx - 1]
        est_ok = abs(float(x["estX"]) - float(r["estX"])) < 1e-3 and abs(float(x["estY"]) - float(r["estY"])) < 1e-3
        thr = K * math.sqrt(float(prev["traceSigma"]))
        mv = math.hypot(float(r["estX"]) - float(r["predX"]), float(r["estY"]) - float(r["predY"]))
        if not est_ok or abs(thr - float(r["threshM"])) > 1e-3 * max(1.0, thr) or abs(mv - float(r["moveM"])) > 1e-3 \
                or not mv > thr or TIERN.get(x["activeTier"], -1) != int(r["tier"]):
            lb.append((r["t"], sid, est_ok, thr, float(r["threshM"]), mv, x["activeTier"], r["tier"]))
    report(not lb and tg, "LARGE", f"{len(tg)} rows checked against the subjects' traces; bad {len(lb)} (need 0)" + (f": {lb[:2]}" if lb else ""))

    ev = rows(os.path.join(d, "on", "events_ca_tbl_full_seed7.csv"))
    fr = subj_frames(ev, {2, 3, 4, 5})
    r1 = [(v[0], v[1]) for v in fr.values() if v[2] == 1]
    by = {}
    for r in tg:
        by.setdefault(int(r["subjectId"]), []).append(float(r["t"]))
    stray = [(t, n) for t, n in r1 if not any(0.0 <= t - t0 <= 2.0 for t0 in by.get(n, []))]
    report(r1 and not stray, "FRAMES", f"subject reason=1 frames heard {len(r1)} (need > 0); more than 2 s after any of that "
           f"subject's triggers {len(stray)} (need 0)" + (f": {stray[:3]}" if stray else ""))

    m = LINE.search(read(os.path.join(d, "on.log")) or "")
    got = (int(m.group(1)), int(m.group(2))) if m else None
    report(got == (len(tg), 3 * len(tg)), "COUNTS", f"MC-2 subjects line {got}; recounted {(len(tg), 3 * len(tg))}")

    of = glob.glob(os.path.join(d, "off", "subjtriggered_*"))
    ol = "MC-2 triggered updates (subjects" in (read(os.path.join(d, "off.log")) or "")
    ofr = [v for v in subj_frames(rows(os.path.join(d, "off", "events_ca_tbl_full_seed7.csv")), {2, 3, 4, 5}).values() if v[2] == 1]
    report(not of and not ol and not ofr, "CONTROL", f"off/: subjtriggered files {len(of)}, subjects line {ol}, subject reason=1 frames {len(ofr)} (need 0, False, 0)")
    t = read(os.path.join(d, "r_notu.log")) or ""
    report(rc(d, "r_notu") == 1 and "--subjectTriggeredUpdates needs" in t, "R", f"without --triggeredUpdates: exit {rc(d, 'r_notu')} (need 1)")
    print(f"  G66: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
