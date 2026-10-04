#!/usr/bin/env python3
"""G85: the fuse narrowed to a short floor (--fuseFloorShort; TECH_DEBT N.14, FUSE-02).

usage: fuse_short_gate.py <dir written by scripts/fuse_short_gate.sh>

Runs read (the preset with --fuseFloorActive, full arm, seed 7, 900 s):
  end/   the endgame with --fuseFloorShort.
  main/  5 km with --fuseFloorShort.
  full/  the endgame without it: the full fuse (the control).
  r_nofuse   refusal: --fuseFloorShort without --fuseFloorActive.

What a firing looks like on disk. fusefloor_<arm>_seed7.csv has one row per fused set, with
nFloor floor ranges. Under the narrowed fuse no row may have nFloor of three or more: that
is a union formed where the floor could fix alone, the case FUSE-01b found costs accuracy.
The full fuse does form such rows, so the narrowing removes something real.

Checks (each prints "G85 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     end, main, full exited 0.
  SHORT    end/ and main/: rows > 0 in each; every row nFloor <= 2, nActive <= 2, and
           nFused = nActive + nFloor.
  FULL     full/: rows with nFloor >= 3 > 0.
  COUNTS   the N.14 line's set count equals the rows, in end/ and main/.
  R        the refusal exits 1 with the reason.
"""
import csv
import os
import re
import sys

LINE = re.compile(r"N\.14 floor\+active fusion \(\w+\): sets (\d+)")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G85 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G85 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G85 MISSING {label:<8}{msg}")


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
        return list(csv.DictReader(open(os.path.join(d, n, "fusefloor_ca_tbl_full_seed7.csv"))))
    except OSError:
        return None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("end", "main", "full") if rc(d, n) != 0]
    report(not bad, "RUNS", "end, main and full exited 0" if not bad else f"non-zero exit: {bad}")
    re_, rm, rf = rows(d, "end"), rows(d, "main"), rows(d, "full")
    if None in (re_, rm, rf):
        missing("SHORT", f"fusefloor_ files: end {re_ is not None}, main {rm is not None}, full {rf is not None}")
        return 1
    sb = [(n, r["t"], r["nodeId"], r["nActive"], r["nFloor"]) for n, rr in (("end", re_), ("main", rm)) for r in rr
          if int(r["nFloor"]) > 2 or int(r["nActive"]) > 2 or int(r["nFused"]) != int(r["nActive"]) + int(r["nFloor"])]
    report(re_ and rm and not sb, "SHORT", f"rows end {len(re_)}, main {len(rm)} (need > 0 each); with a floor of 3+, "
           f"an active set of 3+, or a wrong sum: {len(sb)} (need 0)" + (f": {sb[:2]}" if sb else ""))
    nf = sum(int(r["nFloor"]) >= 3 for r in rf)
    report(nf > 0, "FULL", f"full/: rows {len(rf)}, with a floor of 3+ {nf} (need > 0)")
    got = []
    for n, rr in (("end", re_), ("main", rm)):
        m = LINE.search(read(os.path.join(d, n + ".log")) or "")
        got.append((n, int(m.group(1)) if m else None, len(rr)))
    report(all(a == b for _, a, b in got), "COUNTS", f"N.14 sets vs rows: {got}")
    t = read(os.path.join(d, "r_nofuse.log")) or ""
    report(rc(d, "r_nofuse") == 1 and "--fuseFloorShort needs" in t, "R",
           f"without --fuseFloorActive: exit {rc(d, 'r_nofuse')} (need 1)")
    print(f"  G85: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
