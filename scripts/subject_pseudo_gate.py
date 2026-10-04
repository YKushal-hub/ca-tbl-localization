#!/usr/bin/env python3
"""G68: a subject's range to a finder uses the finder's reported position
(--subjectFinderPseudoAnchor; TECH_DEBT N.21).

usage: subject_pseudo_gate.py <dir written by scripts/subject_pseudo_gate.sh>

Runs read: on/ and off/ (G57's converging regime with entry velocity and prediction),
r_nopa (refusal: without --finderPseudoAnchor).

What a firing looks like on disk: subjpseudo_ca_tbl_full_seed7.csv gets one row per subject
range to a finder -- kind `report` (the entry used, carried forward, and the sigma it priced)
or `dropped` (no entry held: the subject cannot know where the finder is, so the range is not
used; tag 0's code keeps the truth there, N.21). Subjects ask the finders' digest question to
learn entries; the N.21 line counts the questions.

Checks (each prints "G68 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  ROWS     report rows > 0, and digests asked > 0 (subjects learn finder entries by asking).
  PRED     every report row: entryT <= t; predX/predY = entryX/Y + v x (t - entryT); ageVar =
           (0.5 x age)^2 (DC-13's Q, --entryPrediction on).
  SIGMA    every report row: sigma_raw <= sigma_eff, and sigma_eff^2 - sigma_raw^2 at most the
           trace of the entry covariance plus 2 ageVar (a projection never exceeds the trace).
  NOTTRUE  the reported position is not the truth: on some report rows it is more than 1 m
           from the finder's true position (an oracle would put it at 0).
  COUNTS   the N.21 line equals the file (report and dropped rows).
  CONTROL  off/: no subjpseudo_ file and no N.21 line.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import math
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G68 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G68 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G68 MISSING {label:<8}{msg}")


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


LINE = re.compile(r"N\.21 finder pseudo-anchors \(subjects, \w+\): digests asked (\d+), reported positions used (\d+), dropped for want of a position (\d+)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    rs = rows(os.path.join(d, "on", "subjpseudo_ca_tbl_full_seed7.csv"))
    if rs is None:
        missing("ROWS", "no subjpseudo_ca_tbl_full_seed7.csv in on/")
        return 1
    rep = [r for r in rs if r["kind"] == "report"]
    kept = [r for r in rs if r["kind"] == "dropped"]
    m0 = LINE.search(read(os.path.join(d, "on.log")) or "")
    asked = int(m0.group(1)) if m0 else 0
    report(len(rep) > 0 and asked > 0, "ROWS", f"report rows {len(rep)}, dropped rows {len(kept)}, digests asked {asked} (need report > 0, asked > 0)")
    pb, sb, far = [], [], 0
    for r in rep:
        t, et = float(r["t"]), float(r["entryT"])
        age = t - et
        px = float(r["entryX"]) + float(r["vx"]) * age
        py = float(r["entryY"]) + float(r["vy"]) * age
        av = (0.5 * age) ** 2
        if age < -1e-6 or abs(px - float(r["predX"])) > 2e-3 or abs(py - float(r["predY"])) > 2e-3 \
                or abs(av - float(r["ageVar"])) > 1e-4 * max(1.0, av):
            pb.append((r["t"], r["subjectId"], r["finderId"], age))
        s0, s1 = float(r["sigmaRaw"]), float(r["sigmaEff"])
        cap = float(r["entryVarX"]) + float(r["entryVarY"]) + 2 * float(r["ageVar"])
        if s1 + 1e-9 < s0 or s1 * s1 - s0 * s0 > cap * (1 + 1e-6) + 1e-4:
            sb.append((r["t"], r["subjectId"], s0, s1, cap))
        if math.hypot(float(r["predX"]) - float(r["trueX"]), float(r["predY"]) - float(r["trueY"])) > 1.0:
            far += 1
    report(not pb and rep, "PRED", f"{len(rep)} report rows carried forward as DC-13 says; bad {len(pb)} (need 0)" + (f": {pb[:2]}" if pb else ""))
    report(not sb and rep, "SIGMA", f"sigma_eff within [sigma_raw, sqrt(sigma_raw^2 + trace)] on every report row; bad {len(sb)} (need 0)" + (f": {sb[:2]}" if sb else ""))
    report(far > 0, "NOTTRUE", f"report rows more than 1 m from the finder's true position {far} of {len(rep)} (need > 0)")
    m = LINE.search(read(os.path.join(d, "on.log")) or "")
    got = (int(m.group(2)), int(m.group(3))) if m else None
    report(got == (len(rep), len(kept)), "COUNTS", f"N.21 line {got}; recounted {(len(rep), len(kept))}")
    of = glob.glob(os.path.join(d, "off", "subjpseudo_*"))
    ol = "N.21 finder pseudo-anchors" in (read(os.path.join(d, "off.log")) or "")
    report(not of and not ol, "CONTROL", f"off/: subjpseudo files {len(of)}, N.21 line {ol} (need 0, False)")
    t = read(os.path.join(d, "r_nopa.log")) or ""
    report(rc(d, "r_nopa") == 1 and "--subjectFinderPseudoAnchor needs" in t, "R", f"without --finderPseudoAnchor: exit {rc(d, 'r_nopa')} (need 1)")
    print(f"  G68: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
