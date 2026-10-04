#!/usr/bin/env python3
"""G82: run_id.txt records the commit the binary was BUILT from (--stampBuildCommit;
TECH_DEBT M.3).

usage: build_stamp_gate.py <dir written by scripts/build_stamp_gate.sh>

Runs read: on/ and off/ (the repository's binary, 120 s, launched from outside the
repository). head.txt and dirty.txt hold the repository's short HEAD and tracked
modifications under src/ and scratch/ when the runner ran; stamp.cc is the source the build
generated (cmake-cache/scratch/catbl_build_stamp.cc).

What a firing looks like on disk: run_id.txt gains one line after "source ...": "build source
<commit> dirty <n>". Launched outside any git tree, the source line reads "unknown"; the
build line still names the tree that compiled the binary.

Checks (each prints "G82 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  STAMP    on/: the build line is present and well formed, and agrees with stamp.cc.
  FRESH    the build line names the repository's HEAD and dirty count now. A binary not
           rebuilt since the last commit or edit fails here: that is the M.3 hazard in the
           gate's own run, and the fix is ./ns3 build.
  UNKNOWN  on/: the source line reads "unknown" (the launch was outside the repository), so
           the build line is the only record of the commit.
  SAME     on/ and off/ hold the same files, byte-identical except run_id.txt.
  CONTROL  off/: no build line.
"""
import os
import re
import sys

nok = nfail = nmiss = 0
BUILD = re.compile(r"^build source (\S+) dirty (-?\d+)$", re.M)


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G82 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G82 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G82 MISSING {label:<8}{msg}")


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


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    rid = read(os.path.join(d, "on", "run_id.txt"))
    st = read(os.path.join(d, "stamp.cc"))
    if rid is None or st is None:
        missing("STAMP", "no on/run_id.txt or no stamp.cc")
        return 1
    m = BUILD.findall(rid)
    sc = re.search(r'kCatblBuildCommit = "([^"]*)";', st)
    sd = re.search(r"kCatblBuildDirty = (-?\d+);", st)
    want = (sc.group(1), sd.group(1)) if sc and sd else None
    report(len(m) == 1 and m[0] == want, "STAMP", f"on/: build lines {m}; stamp.cc {want}")
    now = (read(os.path.join(d, "head.txt")) or "").strip(), (read(os.path.join(d, "dirty.txt")) or "").strip()
    report(len(m) == 1 and m[0] == now, "FRESH", f"build line {m[0] if m else None}; repository now {now}")
    src = re.search(r"^source (\S+) ", rid, re.M)
    report(src is not None and src.group(1) == "unknown", "UNKNOWN",
           f"on/: source line names {src.group(1) if src else None} (need unknown: launched outside the tree)")
    on, off = os.path.join(d, "on"), os.path.join(d, "off")
    fa, fb = sorted(os.listdir(on)), sorted(os.listdir(off))
    diff = [f for f in fa if f != "run_id.txt" and (f not in fb or open(os.path.join(on, f), "rb").read() != open(os.path.join(off, f), "rb").read())]
    report(fa == fb and not diff, "SAME", f"on/ and off/: same file set {fa == fb}; differing besides run_id.txt {diff}")
    mo = BUILD.findall(read(os.path.join(off, "run_id.txt")) or "")
    report(not mo, "CONTROL", f"off/: build lines {mo} (need none)")
    print(f"  G82: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
