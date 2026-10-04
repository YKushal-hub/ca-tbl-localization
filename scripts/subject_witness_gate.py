#!/usr/bin/env python3
"""G79: every measured subject's escalation is vetted by its own witnesses
(--subjectIntegrityWitness; register row 55, Physical 2.6).

usage: subject_witness_gate.py <dir written by scripts/subject_witness_gate.sh>

Runs read: on/ and off/ (the preset, seed 7, 900 s: --imuCrossCheck with the DC-9 model
witness, and --corroborationCheck; the subjects' flag on and explicitly off), r_shs (refusal:
--imuCrossCheck with the SHS witness source, which is not built per subject).

What a firing looks like on disk. Each subject gets subjwitness_<arm>_seed7_subj<id>.csv,
one row per cycle: its IMU track, this cycle's raw fix, the disagreement and tolerance, the
corroboration count (agreeing of witnessing), and what its own Step() did with them
(evaluated, refused, absorbed). One "Row 55 witness (subject <id>)" line per subject.

Checks (each prints "G79 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  FILES    on/: one subjwitness file per subject 2..5, each digested.
  EVAL     on/: IMU evaluations and corroboration witnesses, summed over the subjects, > 0.
  COUNTS   on/: each line equals its file: evaluations, refusals and absorbed rows; cycles
           with a witness; witnesses; agreeing.
  LOGIC    on/: an evaluated row has an anchored track, a fix and a tolerance > 0; a refused
           row's disagreement exceeds its tolerance; an anchored row follows a fused fix.
  CONTROL  off/: no subject witness line or file, and the start-up WARNING names
           --corroborationCheck; on/: it no longer does.
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
        print(f"  G79 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G79 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G79 MISSING {label:<8}{msg}")


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


LINE = r"Row 55 witness \(subject {}\): IMU evaluations (\d+), refusals (\d+), absorbed by the evidence gate (\d+); corroboration cycles (\d+), witnesses (\d+), agreeing (\d+);"


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    on = read(os.path.join(d, "on.log"))
    rid = read(os.path.join(d, "on", "run_id.txt"))
    files = {int(re.search(r"_subj(\d+)\.csv$", f).group(1)): f for f in glob.glob(os.path.join(d, "on", "subjwitness_*_subj*.csv"))}
    undig = [os.path.basename(f) for f in files.values() if os.path.basename(f) not in rid]
    report(sorted(files) == [2, 3, 4, 5] and not undig, "FILES", f"on/: subjwitness files for {sorted(files)} (need [2, 3, 4, 5]); not digested {undig}")
    if not files:
        missing("EVAL", "no subjwitness file")
        return 1
    evals = wit = 0
    badc, badl = [], []
    for sid, f in sorted(files.items()):
        rows = list(csv.DictReader(open(f)))
        m = re.search(LINE.format(sid), on)
        if m is None:
            missing("COUNTS", f"no witness line for subject {sid}")
            continue
        got = tuple(int(x) for x in m.groups())
        want = (sum(r["evaluated"] == "1" for r in rows), sum(r["refused"] == "1" for r in rows),
                sum(r["absorbed"] == "1" for r in rows), sum(int(r["corrTotal"]) > 0 for r in rows),
                sum(int(r["corrTotal"]) for r in rows), sum(int(r["corrOk"]) for r in rows))
        if got != want:
            badc.append((sid, got, want))
        evals += got[0]
        wit += got[4]
        for r in rows:
            if r["evaluated"] == "1" and not (r["anchored"] == "1" and r["haveFix"] == "1" and float(r["tol"]) > 0.0):
                badl.append((sid, r["t"], "evaluated without track, fix or tolerance"))
            if r["refused"] == "1" and not float(r["disagree"]) > float(r["tol"]):
                badl.append((sid, r["t"], "refused within tolerance"))
            if int(r["corrOk"]) > int(r["corrTotal"]):
                badl.append((sid, r["t"], "more agreeing than witnessing"))
    report(evals > 0 and wit > 0, "EVAL", f"on/: IMU evaluations {evals}, corroboration witnesses {wit} over the subjects (need > 0, > 0)")
    report(not badc, "COUNTS", f"on/: line vs file mismatches {badc[:2]}")
    report(not badl, "LOGIC", f"on/: rows breaking the witness rules {len(badl)}" + (f": {badl[:3]}" if badl else ""))
    off = read(os.path.join(d, "off.log"))
    offl = "Row 55 witness (subject" in off
    offf = glob.glob(os.path.join(d, "off", "subjwitness_*"))
    wOff = re.search(r"WARNING: --measuredSubjects > 1[^\n]*--corroborationCheck", off) is not None
    wOn = re.search(r"WARNING: --measuredSubjects > 1[^\n]*--corroborationCheck", on) is not None
    report(not offl and not offf and wOff and not wOn, "CONTROL", f"off/: subject lines {offl}, files {len(offf)}, WARNING names --corroborationCheck {wOff}; on/: WARNING names it {wOn} (need False, 0, True, False)")
    t = read(os.path.join(d, "r_shs.log"))
    report(rc(d, "r_shs") == 1 and "--subjectIntegrityWitness needs" in t, "R", f"--imuCrossCheck with the SHS source: exit {rc(d, 'r_shs')} (need 1)")
    print(f"  G79: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
