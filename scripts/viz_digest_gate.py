#!/usr/bin/env python3
"""G73: the sidecar digests the visualiser's outputs (--digestVizOutputs; TECH_DEBT M.4).

usage: viz_digest_gate.py <dir written by scripts/viz_digest_gate.sh>

Runs read: on/ and off/ (seed 7, 300 s, --vizEvents and --vizTablePeriodS=60, so all five
visualiser files exist), r_noviz (refusal: without --vizEvents).

What a firing looks like on disk: run_id.txt ends with a "digest sha256" block, one
"<hash>  <file>" line per output, in sha256sum's own format. Without the flag events_*,
nodes_*, nbrtable_*, loctable_* and deployment.json are written and have no line.

Checks (each prints "G73 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  DIGEST   on/: each of the five visualiser files written has a digest line, and every
           digest line verifies against the file (sha256sum -c).
  SAME     on/ and off/ hold the same files, byte-identical except run_id.txt: the flag
           changes nothing but the sidecar.
  CONTROL  off/: the five files exist and none has a digest line (the M.4 gap, kept).
  R        the refusal exits 1 with the reason.
"""
import glob
import hashlib
import os
import re
import sys

VIZ = ("events_", "nodes_", "nbrtable_", "loctable_", "deployment.json")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G73 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G73 FAIL    {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def digests(d):
    try:
        t = open(os.path.join(d, "run_id.txt")).read()
    except OSError:
        return None
    if "digest sha256\n" not in t:
        return {}
    block = t.split("digest sha256\n", 1)[1]
    return {m.group(2): m.group(1) for m in re.finditer(r"^([0-9a-f]{64})  (\S+)$", block, re.M)}


def vizfiles(d):
    return sorted(os.path.basename(p) for p in glob.glob(os.path.join(d, "*")) if os.path.basename(p).startswith(VIZ))


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    on, off = os.path.join(d, "on"), os.path.join(d, "off")
    dg = digests(on) or {}
    vf = vizfiles(on)
    kinds = {k for k in VIZ for f in vf if f.startswith(k)}
    undig = [f for f in vf if f not in dg]
    wrong = [f for f, h in dg.items() if not os.path.exists(os.path.join(on, f)) or hashlib.sha256(open(os.path.join(on, f), "rb").read()).hexdigest() != h]
    report(len(kinds) == 5 and not undig and not wrong and dg, "DIGEST", f"on/: visualiser files {len(vf)} of 5 kinds {len(kinds)}, without a digest line {undig}; digest lines {len(dg)}, not verifying {wrong}")
    fa = sorted(os.listdir(on))
    fb = sorted(os.listdir(off))
    diff = [f for f in fa if f != "run_id.txt" and (f not in fb or open(os.path.join(on, f), "rb").read() != open(os.path.join(off, f), "rb").read())]
    report(fa == fb and not diff, "SAME", f"on/ and off/: same file set {fa == fb}; differing besides run_id.txt {diff}")
    do = digests(off) or {}
    vo = vizfiles(off)
    dug = [f for f in vo if f in do]
    report(len(vo) == len(vf) and vo and not dug, "CONTROL", f"off/: visualiser files {len(vo)}, with a digest line {dug} (need none)")
    t = open(os.path.join(d, "r_noviz.log")).read() if os.path.exists(os.path.join(d, "r_noviz.log")) else ""
    report(rc(d, "r_noviz") == 1 and "--digestVizOutputs needs --vizEvents" in t, "R", f"without --vizEvents: exit {rc(d, 'r_noviz')} (need 1)")
    print(f"  G73: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
