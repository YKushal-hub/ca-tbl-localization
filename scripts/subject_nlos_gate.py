#!/usr/bin/env python3
"""G78: every measured subject scores its own links (--subjectNlosClassifier; register row 55).

usage: subject_nlos_gate.py <dir written by scripts/subject_nlos_gate.sh>

Runs read: on/ and off/ (the preset, seed 7, 900 s, the subjects' flag on and explicitly off;
the preset feeds the score to PCS term 3 and to integrity eligibility), r_nonlos (refusal:
without --nlosClassifier).

What a firing looks like on disk. Each subject 2..N gets subjnlos_links_ (one row per decoded
link it ranged, with its score) and subjnlos_summary_ files, and two blocks of stdout after
tag 0's: "NLOS classifier (subject <id>) ..." and "NLOS PCS term 3 (subject <id>) = ...: fix
cycles F, with a scored link S, fed to PCS P". F counts the subject's fix cycles, and P the
cycles its own Step() fed the score to PCS.

Checks (each prints "G78 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  FILES    on/: links and summary files for subjects 2..5, each digested.
  SCORED   on/: rows with a score in [0, 1] > 0 over the subjects.
  FED      on/: cycles fed to PCS summed over the subjects > 0 (the subjects' own policies
           read their own scores).
  FIXES    on/: each subject's "fix cycles" equals the hadFix rows of its subjtrace.
  TAG0     on/: the first classifier line is tag 0's, unlabelled.
  CONTROL  off/: no subject classifier line or file, and the start-up WARNING names
           --nlosClassifier; on/: the WARNING no longer does.
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
        print(f"  G78 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G78 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G78 MISSING {label:<8}{msg}")


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
    links = {int(re.search(r"_subj(\d+)\.csv$", f).group(1)): f for f in glob.glob(os.path.join(d, "on", "subjnlos_links_*_subj*.csv"))}
    summ = {int(re.search(r"_subj(\d+)\.csv$", f).group(1)): f for f in glob.glob(os.path.join(d, "on", "subjnlos_summary_*_subj*.csv"))}
    undig = [os.path.basename(f) for f in list(links.values()) + list(summ.values()) if os.path.basename(f) not in rid]
    report(sorted(links) == [2, 3, 4, 5] and sorted(summ) == [2, 3, 4, 5] and not undig, "FILES", f"on/: links for {sorted(links)}, summaries for {sorted(summ)} (need [2, 3, 4, 5]); not digested {undig}")
    if not links:
        missing("SCORED", "no subjnlos_links file")
        return 1
    scored = 0
    for f in links.values():
        for r in csv.DictReader(open(f)):
            try:
                s = float(r["score"])
            except ValueError:
                continue
            if 0.0 <= s <= 1.0:
                scored += 1
    report(scored > 0, "SCORED", f"on/: scored links over the subjects {scored} (need > 0)")
    fed, badfix = 0, []
    for sid in sorted(links):
        m = re.search(rf"NLOS PCS term 3 \(subject {sid}\) = [^\n]*fix cycles (\d+), with a scored link (\d+), fed to PCS (\d+)", on)
        if m is None:
            missing("FED", f"no PCS term line for subject {sid}")
            continue
        fed += int(m.group(3))
        tr = glob.glob(os.path.join(d, "on", f"subjtrace_*_subj{sid}.csv"))
        nfix = sum(1 for r in csv.DictReader(open(tr[0])) if r["hadFix"] == "1") if tr else None
        if nfix != int(m.group(1)):
            badfix.append((sid, int(m.group(1)), nfix))
    report(fed > 0, "FED", f"on/: cycles fed to PCS over the subjects {fed} (need > 0)")
    report(not badfix, "FIXES", f"on/: subjects whose fix cycles differ from their trace: {badfix}")
    first = re.search(r"NLOS classifier[^(\n]*(\(subject \d+\) )?\(threshold", on)
    report(first is not None and first.group(1) is None, "TAG0", f"on/: the first classifier line is labelled {first.group(1) if first else None!r} (need None: tag 0's)")
    off = read(os.path.join(d, "off.log"))
    offl = re.search(r"NLOS classifier \(subject", off) is not None
    offf = glob.glob(os.path.join(d, "off", "subjnlos_*"))
    wOff = re.search(r"WARNING: --measuredSubjects > 1[^\n]*--nlosClassifier", off) is not None
    wOn = re.search(r"WARNING: --measuredSubjects > 1[^\n]*--nlosClassifier", on) is not None
    report(not offl and not offf and wOff and not wOn, "CONTROL", f"off/: subject lines {offl}, files {len(offf)}, WARNING names --nlosClassifier {wOff}; on/: WARNING names it {wOn} (need False, 0, True, False)")
    t = read(os.path.join(d, "r_nonlos.log"))
    report(rc(d, "r_nonlos") == 1 and "--subjectNlosClassifier needs" in t, "R", f"without --nlosClassifier: exit {rc(d, 'r_nonlos')} (need 1)")
    print(f"  G78: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
