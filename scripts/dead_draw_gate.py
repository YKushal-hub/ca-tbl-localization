#!/usr/bin/env python3
"""G71: a node whose battery has run out draws nothing more (--deadBatteryNoDraw; TECH_DEBT
N.20).

usage: dead_draw_gate.py <dir written by scripts/dead_draw_gate.sh>

Runs read: on/ and off/ (G67's regime with --silenceStopsRelay: tag 0 and four subjects on a
3 J budget, triggered updates), r_norelay and r_nobat (refusals: without --silenceStopsRelay,
without --batteryDepleteJ).

What a firing looks like on disk. Each death gets a FAULT line with its time. A node's trace
(trace_* for tag 0, subjtrace_*_subj<N> for a subject) carries cumJ, its cumulative energy,
on every cycle. By default cumJ keeps rising after the death: listening, the search window
and sleep are still billed. With the flag, cumJ is flat on every row after the death: the
last gasp and the death cycle's own exchange are billed in the death cycle, and nothing
after it.

Checks (each prints "G71 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  DEAD     in both runs tag 0 died and at least one subject died.
  DEFECT   off/: some dead node's cumJ rises by more than 0.01 J after its death: the defect
           reproduces, so the fix is tested against something.
  FIX      on/: every dead node's cumJ is flat (to 1e-6 J) on every row after its death.
  COUNTS   on/: the N.20 line's radio-cycles equal 3 per trace row from the death on (three
           radios a cycle), for tag 0 and summed over the subjects.
  CONTROL  off/: no N.20 line.
  R        both refusals exit 1 with the reason.
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
        print(f"  G71 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G71 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G71 MISSING {label:<8}{msg}")


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


def deaths(d, n):
    """{node: (death time, trace rows)} for tag 0 (node 1) and every subject that died."""
    log = read(os.path.join(d, n + ".log")) or ""
    out = {}
    m = re.search(r"FAULT sect 49: battery exhausted at t=([0-9.]+) s", log)
    if m:
        f = glob.glob(os.path.join(d, n, "trace_*.csv"))
        out[1] = (float(m.group(1)), list(csv.DictReader(open(f[0]))) if f else None)
    for m in re.finditer(r"FAULT sect 49 \(subject (\d+)\): battery exhausted at t=([0-9.]+) s", log):
        f = glob.glob(os.path.join(d, n, f"subjtrace_*_subj{m.group(1)}.csv"))
        out[int(m.group(1))] = (float(m.group(2)), list(csv.DictReader(open(f[0]))) if f else None)
    return out


def rise(td, rows):
    """cumJ's largest rise over the rows after the death, against the last row at or before it."""
    before = [r for r in rows if float(r["t"]) <= td + 1e-9]
    after = [r for r in rows if float(r["t"]) > td + 1e-9]
    if not before or not after:
        return None
    base = float(before[-1]["cumJ"])
    return max(float(r["cumJ"]) - base for r in after)


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    dd = {n: deaths(d, n) for n in ("on", "off")}
    if any(any(v[1] is None for v in dd[n].values()) for n in dd):
        missing("DEAD", "a dead node's trace is missing")
        return 1
    ok = all(1 in dd[n] and len(dd[n]) >= 2 for n in dd)
    report(ok, "DEAD", f"deaths: on {sorted(dd['on'])}, off {sorted(dd['off'])} (need tag 0 and a subject in both)")
    ro = {k: rise(td, rows) for k, (td, rows) in dd["off"].items()}
    report(any((v or 0) > 0.01 for v in ro.values()), "DEFECT", "off/: cumJ rise after the death by node " + ", ".join(f"{k}: {v:.3f} J" for k, v in sorted(ro.items()) if v is not None) + " (need one > 0.01)")
    rn = {k: rise(td, rows) for k, (td, rows) in dd["on"].items()}
    badn = {k: v for k, v in rn.items() if v is None or v > 1e-6}
    report(dd["on"] and not badn, "FIX", "on/: cumJ rise after the death by node " + ", ".join(f"{k}: {v:.6f} J" for k, v in sorted(rn.items()) if v is not None) + " (need all <= 1e-6)" + (f"; bad {badn}" if badn else ""))
    m = re.search(r"N\.20 dead radios \(\w+\): radio-cycles unbilled, tag 0 (\d+), subjects (\d+)", read(os.path.join(d, "on.log")) or "")
    want0 = 3 * sum(1 for r in dd["on"][1][1] if float(r["t"]) >= dd["on"][1][0] - 1e-9) if 1 in dd["on"] else None
    wantS = 3 * sum(sum(1 for r in rows if float(r["t"]) >= td - 1e-9) for k, (td, rows) in dd["on"].items() if k != 1)
    got = (int(m.group(1)), int(m.group(2))) if m else None
    report(got == (want0, wantS), "COUNTS", f"N.20 line {got}; 3 x rows from the death {(want0, wantS)}")
    ol = "N.20 dead radios" in (read(os.path.join(d, "off.log")) or "")
    report(not ol, "CONTROL", f"off/: N.20 line {ol} (need False)")
    rr = [(n, rc(d, n)) for n in ("r_norelay", "r_nobat")]
    okr = all(c == 1 and "--deadBatteryNoDraw needs" in (read(os.path.join(d, n + ".log")) or "") for n, c in rr)
    report(okr, "R", f"refusals {rr} (need exit 1 with the reason)")
    print(f"  G71: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
