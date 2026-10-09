#!/usr/bin/env python3
"""G111: --subjectPreciseOnly (S2 Part B, F2): a dual-base subject with >= 2 precise ranges fuses
them without its 868 MHz ranges.

usage: acc_b2_gate.py <dir written by scripts/acc_b2_gate.sh>

Checks (each prints "G111 ok|FAIL <LABEL> ..."):
  RUNS      the three runs exit 0.
  DEFAULT   the flag given as false and the flag absent: every output file except run_id.txt
            byte-identical, and the same stdout lines apart from the output paths.
  MOVED     with the flag on, the B-F2 line reports subject-cycles fused without 868 MHz > 0 and
            868 MHz ranges left out > 0; without it there is no B-F2 line.
  SUBJ      at least one of ca_tbl_full's subject traces differs on vs off (the change reaches the
            subjects' estimates).
  ISOLATION every file of static_2p4 and lora_fusion, and their arm_summary rows, byte-identical on
            and off (the flag reaches only hybrid arms' subjects).
"""
import csv
import filecmp
import glob
import os
import re
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G111 {'ok  ' if ok else 'FAIL'}    {label:<10}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def files(d):
    out = set()
    for root, _, fs in os.walk(d):
        for f in fs:
            out.add(os.path.relpath(os.path.join(root, f), d))
    out.discard("run_id.txt")
    return out


def summary_rows(d, arm):
    try:
        return [r for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))) if r["arm"] == arm]
    except OSError:
        return []


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("off", "zero", "on") if rc(d, n) != 0]
    report(not bad, "RUNS", "3 runs exited 0" if not bad else f"non-zero exit: {bad}")
    off, zero, on = (os.path.join(d, n) for n in ("off", "zero", "on"))
    fo, fz = files(off), files(zero)
    diff = sorted(f for f in fo & fz if not filecmp.cmp(os.path.join(off, f), os.path.join(zero, f), shallow=False))
    strip = lambda t: [ln.replace("/zero", "/X").replace("/off", "/X") for ln in t.splitlines()]  # noqa: E731
    try:
        lo, lz = strip(open(off + ".log").read()), strip(open(zero + ".log").read())
    except OSError:
        lo, lz = ["missing"], []
    report(fo == fz and not diff and len(fo) > 2 and lo == lz, "DEFAULT",
           f"{len(fo)} files; differing {diff[:3]}; only one side {sorted(fo ^ fz)[:3]}; stdout equal {lo == lz}")
    try:
        lon, loff = open(on + ".log").read(), open(off + ".log").read()
    except OSError:
        lon = loff = ""
    m = re.search(r"B-F2 subject precise-only \(ca_tbl_full\): subject-cycles fused without 868 MHz (\d+), "
                  r"868 MHz ranges left out (\d+)", lon)
    vals = tuple(int(x) for x in m.groups()) if m else None
    report(bool(vals) and all(v > 0 for v in vals) and "B-F2" not in loff, "MOVED",
           f"on: subject-cycles / ranges left out {vals} (need both > 0); B-F2 line absent off: {'B-F2' not in loff}")
    sub = sorted(glob.glob(os.path.join(off, "subjtrace_ca_tbl_full_seed*_subj*.csv")))
    ndiff = sum(1 for p in sub if not filecmp.cmp(p, os.path.join(on, os.path.basename(p)), shallow=False)
                if os.path.exists(os.path.join(on, os.path.basename(p))))
    report(len(sub) > 0 and ndiff > 0, "SUBJ", f"ca_tbl_full subject traces differing on vs off: {ndiff} of {len(sub)} (need > 0)")
    iso_bad, nfiles = [], 0
    for arm in ("static_2p4", "lora_fusion"):
        if summary_rows(off, arm) != summary_rows(on, arm) or not summary_rows(off, arm):
            iso_bad.append(f"{arm} arm_summary row")
        for p in glob.glob(os.path.join(off, f"*_{arm}_seed*")):
            q = os.path.join(on, os.path.basename(p))
            nfiles += 1
            if not os.path.exists(q) or not filecmp.cmp(p, q, shallow=False):
                iso_bad.append(os.path.basename(p))
    report(not iso_bad and nfiles > 0, "ISOLATION", f"{nfiles} non-hybrid files compared; differing {iso_bad[:4]}")
    print(f"  G111: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
