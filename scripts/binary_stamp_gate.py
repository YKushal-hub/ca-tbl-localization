#!/usr/bin/env python3
"""G76: run_id.txt records the binary that ran (--stampBinaryHash; TECH_DEBT M.3).

usage: binary_stamp_gate.py <dir written by scripts/binary_stamp_gate.sh>

Runs read: on/ and off/ (the repo's binary, 120 s, launched from outside the repository),
copy/ (a byte copy of the binary at another path, with the flag). bin.sha256 and bin.path
hold the binary's hash and path, taken by the runner.

What a firing looks like on disk: run_id.txt gains one line after "source ...":
"binary sha256 <hash> <path>". Launched outside any git tree, the source line reads
"unknown" (the M.3 hazard's other face); the binary line still names what ran.

Checks (each prints "G76 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on, off and copy exited 0.
  STAMP    on/: the binary line carries the binary's hash and path.
  COPY     copy/: the same hash, with the copy's path (copy.bin), not the repo binary's: the
           hash follows the bytes, the path follows the launch.
  SAME     on/ and off/ hold the same files, byte-identical except run_id.txt.
  CONTROL  off/: no binary line.
"""
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G76 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G76 FAIL    {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def stamp(d, n):
    try:
        m = re.search(r"^binary sha256 ([0-9a-f]{64}) (\S+)$", open(os.path.join(d, n, "run_id.txt")).read(), re.M)
    except OSError:
        return None
    return m.groups() if m else None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off", "copy") if rc(d, n) != 0]
    report(not bad, "RUNS", "on, off and copy exited 0" if not bad else f"non-zero exit: {bad}")
    h = open(os.path.join(d, "bin.sha256")).read().strip()
    p = open(os.path.join(d, "bin.path")).read().strip()
    s = stamp(d, "on")
    report(s == (h, os.path.realpath(p)), "STAMP", f"on/: binary line {s}; the binary {h[:16]}... at {p}")
    c = stamp(d, "copy")
    okc = c is not None and c[0] == h and c[1].endswith("/copy.bin") and c[1] != os.path.realpath(p)
    report(okc, "COPY", f"copy/: binary line {c} (need the same hash, at the copy's path)")
    on, off = os.path.join(d, "on"), os.path.join(d, "off")
    fa, fb = sorted(os.listdir(on)), sorted(os.listdir(off))
    diff = [f for f in fa if f != "run_id.txt" and (f not in fb or open(os.path.join(on, f), "rb").read() != open(os.path.join(off, f), "rb").read())]
    report(fa == fb and not diff, "SAME", f"on/ and off/: same file set {fa == fb}; differing besides run_id.txt {diff}")
    report(stamp(d, "off") is None, "CONTROL", f"off/: binary line {stamp(d, 'off')} (need None)")
    print(f"  G76: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
