#!/usr/bin/env python3
"""G63: the active tier fused with the floor (TECH_DEBT N.14, --fuseFloorActive).

usage: fuse_floor_gate.py <dir written by scripts/fuse_floor_gate.sh>

Runs read (the preset, full arm, seed 7, 900 s):
  end/   the endgame (750 m box, finders converging) with --fuseFloorActive.
  off/   the same without it (the control).
  main/  the preset at 5 km with it.
  r_nostep   refusal: the flag without --sparseInStep.

What a firing looks like on disk. fusefloor_<arm>_seed7.csv gets one row per node-cycle
whose fix set was the union: nActive (0-2) active ranges, then nFloor floor ranges. Tag 0's
trace row at that t then has nRangesEkf = nActive + nFloor and fixTier = the active tier
(868 MHz when nActive is 0). The defects it guards against: a union formed where the old
path applies (active three or more, or the floor alone at three or more), a set that is
not the sum of its parts, a label that hides which ranges were fused, or a fix kind that
Step() could not have made from that many ranges.

Checks (each prints "G63 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     end, off, main exited 0.
  ROWS     end/: rows > 0, and some with the active tier UWB and active ranges (the case
           N.14 is about); main/: rows with no active range and a short floor (the case
           GEO-01 found).
  UNION    every row: nActive <= 2, nFloor >= 1, never nActive 0 with nFloor >= 3,
           nFused = nActive + nFloor, fixTier = activeTier when nActive > 0 else 868 MHz.
  KIND     Step()'s fix kind fits the size: 3+ full or none; 2 Case A, B or none; 1 Case
           D or none; hadFix exactly on full, A and B.
  TRACE    every tag-0 row (nodeId 1) matches its trace row: activeTier, fixTier, hadFix,
           and nRangesEkf = nFused.
  UWBFIX   end/: UWB-delivered fixes (fixTier UWB, hadFix) in tag 0's trace and the
           subjects' traces > 0 -- G24's zero population, reached.
  COUNTS   the N.14 line, recounted from the file, in end/ and main/.
  CONTROL  off/: no fusefloor file and no N.14 line.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

nok = nfail = nmiss = 0
NAMES = ("868MHz", "2.4GHz", "UWB")


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G63 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G63 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G63 MISSING {label:<8}{msg}")


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


def ff(d, n):
    fs = glob.glob(os.path.join(d, n, "fusefloor_*_seed7.csv"))
    return list(csv.DictReader(open(fs[0]))) if len(fs) == 1 else None


LINE = re.compile(r"N\.14 floor\+active fusion \(\S+\): sets (\d+) \(active ranges 0/1/2 (\d+)/(\d+)/(\d+); "
                  r"active tier 868/2\.4/UWB (\d+)/(\d+)/(\d+)\); Step made none/full/A/B/D (\d+)/(\d+)/(\d+)/(\d+)/(\d+); "
                  r"fixes using active ranges 868/2\.4/UWB (\d+)/(\d+)/(\d+)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("end", "off", "main") if rc(d, n) != 0]
    report(not bad, "RUNS", "end, off, main exited 0" if not bad else f"non-zero exit: {bad}")
    rows = {n: ff(d, n) for n in ("end", "main")}
    if rows["end"] is None or rows["main"] is None:
        missing("ROWS", "no fusefloor_*.csv in end/ or main/")
        return 1
    uwb = [r for r in rows["end"] if r["activeTier"] == "UWB" and int(r["nActive"]) > 0]
    short = [r for r in rows["main"] if int(r["nActive"]) == 0 and int(r["nFloor"]) < 3]
    report(rows["end"] and uwb and short, "ROWS",
           f"end/ {len(rows['end'])} rows, {len(uwb)} on UWB with active ranges; main/ {len(rows['main'])} rows, "
           f"{len(short)} with no active range and a short floor (need > 0 each)")

    ub = []
    kb = []
    for n in ("end", "main"):
        for r in rows[n]:
            a, f, s = int(r["nActive"]), int(r["nFloor"]), int(r["nFused"])
            want = r["activeTier"] if a > 0 else "868MHz"
            if a > 2 or f < 1 or (a == 0 and f >= 3) or s != a + f or r["fixTier"] != want:
                ub.append((n, r["t"], r["nodeId"], a, f, s, r["fixTier"]))
            k, h = int(r["fixKind"]), int(r["hadFix"])
            ok = (k in (0, 1)) if s >= 3 else ((k in (0, 2, 3)) if s == 2 else (k in (0, 4)))
            if not ok or (h == 1) != (k in (1, 2, 3)):
                kb.append((n, r["t"], r["nodeId"], s, k, h))
    report(not ub, "UNION", f"rows checked {len(rows['end']) + len(rows['main'])}; bad {len(ub)} (need 0)" + (f": {ub[:3]}" if ub else ""))
    report(not kb, "KIND", f"fix kinds that do not fit the set: {len(kb)} (need 0)" + (f": {kb[:3]}" if kb else ""))

    tb = []
    nt = 0
    for n in ("end", "main"):
        tr = glob.glob(os.path.join(d, n, "trace_*_seed7.csv"))
        if len(tr) != 1:
            tb.append((n, "no trace"))
            continue
        tm = {round(float(r["t"]), 3): r for r in csv.DictReader(open(tr[0]))}
        for r in rows[n]:
            if r["nodeId"] != "1":
                continue
            nt += 1
            x = tm.get(round(float(r["t"]), 3))
            if x is None or x["activeTier"] != r["activeTier"] or x["fixTier"] != r["fixTier"] \
                    or int(x["hadFix"]) != int(r["hadFix"]) or int(x["nRangesEkf"]) != int(r["nFused"]):
                tb.append((n, r["t"], x and (x["activeTier"], x["fixTier"], x["hadFix"], x["nRangesEkf"])))
    report(not tb and nt > 0, "TRACE", f"tag-0 rows matched to their trace row {nt}; bad {len(tb)} (need 0)" + (f": {tb[:3]}" if tb else ""))

    uf = 0
    for f in glob.glob(os.path.join(d, "end", "trace_*.csv")) + glob.glob(os.path.join(d, "end", "subjtrace_*.csv")):
        for r in csv.DictReader(open(f)):
            if r["hadFix"] == "1" and r["fixTier"] == "UWB":
                uf += 1
    report(uf > 0, "UWBFIX", f"end/: UWB-delivered fixes {uf} (need > 0)")

    cb = []
    for n in ("end", "main"):
        m = LINE.search(read(os.path.join(d, n + ".log")) or "")
        if not m:
            cb.append((n, "no line"))
            continue
        g = [int(x) for x in m.groups()]
        rs = rows[n]
        want = [len(rs)] + [sum(1 for r in rs if min(int(r["nActive"]), 2) == k) for k in (0, 1, 2)] \
            + [sum(1 for r in rs if r["activeTier"] == t) for t in NAMES] \
            + [sum(1 for r in rs if min(int(r["fixKind"]), 4) == k) for k in range(5)] \
            + [sum(1 for r in rs if r["hadFix"] == "1" and int(r["nActive"]) > 0 and r["activeTier"] == t) for t in NAMES]
        if g != want:
            cb.append((n, g, want))
    report(not cb, "COUNTS", "the N.14 line recounted from the file in end/ and main/" if not cb else f"disagree: {cb}")

    of = glob.glob(os.path.join(d, "off", "fusefloor_*.csv"))
    ol = "N.14 floor+active fusion" in (read(os.path.join(d, "off.log")) or "")
    report(not of and not ol, "CONTROL", f"off/: fusefloor files {len(of)}, N.14 line {ol} (need 0, False)")

    t = read(os.path.join(d, "r_nostep.log")) or ""
    report(rc(d, "r_nostep") == 1 and "--fuseFloorActive needs --sparseInStep" in t, "R",
           f"--fuseFloorActive without --sparseInStep: exit {rc(d, 'r_nostep')} (need 1)")
    print(f"  G63: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
