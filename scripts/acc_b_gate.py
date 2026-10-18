#!/usr/bin/env python3
"""G110: --dualBaseRelayTier (S2 Part B, F1): relay legs on 2.4 GHz under the dual base.

usage: acc_b_gate.py <dir written by scripts/acc_b_gate.sh>

Checks (each prints "G110 ok|FAIL|MISSING <LABEL> ..."):
  RUNS      the three runs exit 0.
  DEFAULT   the flag given as false and the flag absent: every output file except run_id.txt
            byte-identical, and the same stdout lines apart from the output paths.
  MOVED     with the flag on, the B-F1 line reports tag 0 legs on 2.4 GHz with 868 MHz active > 0,
            admitted > 0, and subject exchanges moved > 0; without it there is no B-F1 line.
  BILLED    ca_tbl_full's 2.4 GHz energy (j_2p4) is higher with the flag on: the legs are billed
            on the radio that made them.
  ISOLATION every file of static_2p4 and lora_fusion, and their arm_summary rows, are byte-identical
            on and off (the flag reaches only hybrid arms; P-B5).
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
    print(f"  G110 {'ok  ' if ok else 'FAIL'}    {label:<10}{msg}")


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
    m = re.search(r"B-F1 dual-base relay tier \(ca_tbl_full\): tag 0 legs on 2\.4 GHz with 868 MHz active (\d+), "
                  r"admitted (\d+); subject exchanges moved from 868 MHz to 2\.4 GHz (\d+)", lon)
    vals = tuple(int(x) for x in m.groups()) if m else None
    report(bool(vals) and all(v > 0 for v in vals) and "B-F1" not in loff, "MOVED",
           f"on: legs/admitted/subject exchanges {vals} (need all > 0); B-F1 line absent off: {'B-F1' not in loff}")
    so, sn = summary_rows(off, "ca_tbl_full"), summary_rows(on, "ca_tbl_full")
    jo = float(so[0]["j_2p4"]) if so else float("nan")
    jn = float(sn[0]["j_2p4"]) if sn else float("nan")
    report(bool(so and sn) and jn > jo, "BILLED", f"ca_tbl_full j_2p4 off {jo:.4f} J, on {jn:.4f} J (need on > off)")
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
    print(f"  G110: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
