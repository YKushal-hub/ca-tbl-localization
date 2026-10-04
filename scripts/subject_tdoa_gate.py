#!/usr/bin/env python3
"""G77: every measured subject runs tier-1 TDoA (--subjectTier1Tdoa; register row 55).

usage: subject_tdoa_gate.py <dir written by scripts/subject_tdoa_gate.sh>

Runs read: on/ and off/ (the preset, seed 7, 900 s, the subjects' flag on and explicitly off),
r_notdoa (refusal: without --tier1Tdoa).

What a firing looks like on disk. Each subject 2..N gets subjtdoa_<arm>_seed7_subj<id>.csv,
one row per tier-1 fix cycle (verdict, solution, whether Step() fused it), and two stdout
lines, "TDoA tier 1 (subject <id>): ..." and "TDoA billing (subject <id>): ...", printed
after tag 0's. Tag 0's lines keep their exact old form.

Checks (each prints "G77 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  FILES    on/: one subjtdoa file per subject 2..5, each digested in run_id.txt.
  SOLVES   on/: OK solves summed over the subjects > 0; each subject's "solves OK" equals the
           OK rows in its file.
  FUSED    on/: each subject's fused count equals its own policy's ("fused X (policy X)"),
           and a fused row only where the verdict is OK.
  BILL     on/: each subject's "stamps beyond a billed reply" is 0.
  TAG0     on/: tag 0's "TDoA tier 1:" line is still printed first, unlabelled.
  CONTROL  off/: no subject TDoA line or file, and the start-up WARNING names --tier1Tdoa;
           on/: the WARNING no longer does.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G77 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G77 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G77 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def read(p):
    try:
        return open(p).read()
    except OSError:
        return ""


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    on = read(os.path.join(d, "on.log"))
    rid = read(os.path.join(d, "on", "run_id.txt"))
    files = {int(re.search(r"_subj(\d+)\.csv$", f).group(1)): f for f in glob.glob(os.path.join(d, "on", "subjtdoa_*_subj*.csv"))}
    undig = [os.path.basename(f) for f in files.values() if os.path.basename(f) not in rid]
    report(sorted(files) == [2, 3, 4, 5] and not undig, "FILES", f"on/: subjtdoa files for subjects {sorted(files)} (need [2, 3, 4, 5]); not digested {undig}")
    if not files:
        missing("SOLVES", "no subjtdoa file")
        return 1
    tot = 0
    badc, badf, badb = [], [], []
    for sid, f in sorted(files.items()):
        rows = list(csv.DictReader(open(f)))
        okr = sum(1 for r in rows if r["verdict"] == "OK")
        m = re.search(rf"TDoA tier 1 \(subject {sid}\): [^\n]*solves OK (\d+)[^\n]*; fused (\d+) \(policy (\d+)\)", on)
        b = re.search(rf"TDoA billing \(subject {sid}\): [^\n]*stamps beyond a billed reply (\d+)", on)
        if m is None or b is None:
            missing("SOLVES", f"no TDoA lines for subject {sid}")
            continue
        tot += int(m.group(1))
        if int(m.group(1)) != okr:
            badc.append((sid, int(m.group(1)), okr))
        fusedNotOk = sum(1 for r in rows if r["fused"] == "1" and r["verdict"] != "OK")
        if m.group(2) != m.group(3) or fusedNotOk:
            badf.append((sid, m.group(2), m.group(3), fusedNotOk))
        if b.group(1) != "0":
            badb.append((sid, b.group(1)))
    report(tot > 0 and not badc, "SOLVES", f"on/: OK solves over the subjects {tot} (need > 0); line vs file mismatches {badc}")
    report(not badf, "FUSED", f"on/: subjects whose fused count differs from their policy's, or fused a non-OK solve: {badf}")
    report(not badb, "BILL", f"on/: subjects with stamps beyond a billed reply: {badb}")
    first = re.search(r"TDoA tier 1[^:]*:", on)
    report(first is not None and first.group(0) == "TDoA tier 1:", "TAG0", f"on/: the first TDoA line is {first.group(0) if first else None!r} (need 'TDoA tier 1:')")
    off = read(os.path.join(d, "off.log"))
    offl = re.search(r"TDoA tier 1 \(subject", off) is not None
    offf = glob.glob(os.path.join(d, "off", "subjtdoa_*"))
    wOff = re.search(r"WARNING: --measuredSubjects > 1[^\n]*--tier1Tdoa", off) is not None
    wOn = re.search(r"WARNING: --measuredSubjects > 1[^\n]*--tier1Tdoa", on) is not None
    report(not offl and not offf and wOff and not wOn, "CONTROL", f"off/: subject lines {offl}, files {len(offf)}, WARNING names --tier1Tdoa {wOff}; on/: WARNING names it {wOn} (need False, 0, True, False)")
    t = read(os.path.join(d, "r_notdoa.log"))
    report(rc(d, "r_notdoa") == 1 and "--subjectTier1Tdoa needs" in t, "R", f"without --tier1Tdoa: exit {rc(d, 'r_notdoa')} (need 1)")
    print(f"  G77: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
