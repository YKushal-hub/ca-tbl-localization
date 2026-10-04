#!/usr/bin/env python3
"""G69: no finder range uses the finder's true position (--finderPseudoAnchorStrict;
TECH_DEBT N.21, tag 0's half).

usage: finder_strict_gate.py <dir written by scripts/finder_strict_gate.sh>

Runs read: on/ and off/ (G68's converging regime, the subjects' flag on in both), r_nopa
(refusal: without --finderPseudoAnchor).

What a firing looks like on disk: pseudo_ca_tbl_full_seed7.csv gets one row per tag 0 range
to a finder -- kind `report` (the finder's entry replaced the truth, its variance projected
into sigma), `report_noinflate` (the entry replaced the truth, the variance could not be
projected) or `dropped` (tag 0 held no entry: received and billed, not used; by default the
truth is used there). usedX/usedY are the position the range was solved against (for a
dropped row, the truth it would have used); trueX/trueY the finder's true position.

Checks (each prints "G69 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  ROWS     report rows > 0 and dropped rows > 0: the case the flag changes is reached.
  SUBS     report rows equal the TABLE(reverse) pseudo-anchor substitutions: every
           substitution is logged, and nothing else is logged as one.
  NOTTRUE  the used position is not the truth: on some report rows it is more than 1 m from
           the finder's true position, and on none is it the truth to the file's precision.
           Under --finderPosSigma=2.0 every report carries its finder's constant GNSS
           offset, so a used position at the truth can only be the truth itself.
  COUNTS   the N.21 strict line equals the file (report, dropped and uninflated rows).
  CONTROL  off/: no pseudo_ file and no N.21 strict line.
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
        print(f"  G69 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G69 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G69 MISSING {label:<8}{msg}")


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


LINE = re.compile(r"N\.21 strict pseudo-anchors \(\w+\): tag 0 finder ranges on reports (\d+), dropped for want of a position (\d+), reports used uninflated (\d+); subjects' reports used uninflated (\d+)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    rs = rows(os.path.join(d, "on", "pseudo_ca_tbl_full_seed7.csv"))
    if rs is None:
        missing("ROWS", "no pseudo_ca_tbl_full_seed7.csv in on/")
        return 1
    rep = [r for r in rs if r["kind"] == "report"]
    noinf = [r for r in rs if r["kind"] == "report_noinflate"]
    drop = [r for r in rs if r["kind"] == "dropped"]
    report(len(rep) > 0 and len(drop) > 0, "ROWS", f"report rows {len(rep)}, uninflated {len(noinf)}, dropped {len(drop)} (need report > 0, dropped > 0)")
    on = read(os.path.join(d, "on.log")) or ""
    ms = re.search(r"TABLE\(reverse\): (\d+) pseudo-anchor substitutions", on)
    if ms is None:
        missing("SUBS", "no TABLE(reverse) substitution line in on.log")
    else:
        report(int(ms.group(1)) == len(rep), "SUBS", f"TABLE(reverse) substitutions {ms.group(1)}, report rows {len(rep)} (need equal)")
    used = rep + noinf
    far = sum(1 for r in used if math.hypot(float(r["usedX"]) - float(r["trueX"]), float(r["usedY"]) - float(r["trueY"])) > 1.0)
    same = [(r["t"], r["finderId"]) for r in used if r["usedX"] == r["trueX"] and r["usedY"] == r["trueY"]]
    report(far > 0 and not same and used, "NOTTRUE", f"used rows more than 1 m from the truth {far} of {len(used)} (need > 0); at the truth {len(same)} (need 0)" + (f": {same[:2]}" if same else ""))
    m = LINE.search(on)
    got = (int(m.group(1)), int(m.group(2)), int(m.group(3))) if m else None
    report(got == (len(rep), len(drop), len(noinf)), "COUNTS", f"N.21 strict line {got}; recounted {(len(rep), len(drop), len(noinf))}")
    of = glob.glob(os.path.join(d, "off", "pseudo_*"))
    ol = "N.21 strict" in (read(os.path.join(d, "off.log")) or "")
    report(not of and not ol, "CONTROL", f"off/: pseudo_ files {len(of)}, N.21 strict line {ol} (need 0, False)")
    t = read(os.path.join(d, "r_nopa.log")) or ""
    report(rc(d, "r_nopa") == 1 and "--finderPseudoAnchorStrict needs" in t, "R", f"without --finderPseudoAnchor: exit {rc(d, 'r_nopa')} (need 1)")
    print(f"  G69: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
