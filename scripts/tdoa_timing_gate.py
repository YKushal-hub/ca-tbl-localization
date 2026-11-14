#!/usr/bin/env python3
"""G168: --tdoaToaScale (report/PREREG_TDOA_TIMING.md). usage: tdoa_timing_gate.py <dir of tdoa_timing_gate.sh>

  RUNS    abs, one and t0 exit 0 and write static_868's TDoA log
  OFF     one (--tdoaToaScale=1) is byte-identical to abs (flag absent) in every file but run_id.txt
  LINE    the TDOA TOA SCALE line is in t0's log only
  EFFECT  at least 5 TDoA solves in abs and t0, and t0's median tdoaErrM is below 0.1 x abs's
  REFUSE  --tdoaToaScale=0 exits non-zero with its FATAL line
"""
import csv
import filecmp
import math
import os
import statistics
import sys

LINE = "TDOA TOA SCALE (--tdoaToaScale="
nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G168 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, f"{n}.rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, f"{n}.log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def errs(d, n):
    p = os.path.join(d, n, "tdoa_static_868_seed7.csv")
    if not os.path.exists(p):
        return None
    out = []
    for r in csv.DictReader(open(p)):
        try:
            v = float(r["tdoaErrM"])
        except (KeyError, ValueError):
            continue
        if math.isfinite(v) and v >= 0:
            out.append(v)
    return out


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("abs", "one", "t0")}
    ea, et = errs(d, "abs"), errs(d, "t0")
    report(all(v == 0 for v in rcs.values()) and ea is not None and et is not None, "RUNS",
           f"exit codes {rcs}, TDoA logs {ea is not None and et is not None}")
    a, b = os.path.join(d, "abs"), os.path.join(d, "one")
    names = sorted(f for f in os.listdir(a) if f != "run_id.txt") if os.path.isdir(a) else []
    bad = [f for f in names if not (os.path.exists(os.path.join(b, f)) and
                                    filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False))]
    report(bool(names) and not bad, "OFF", f"{len(names) - len(bad)}/{len(names)} files identical, abs vs one {bad[:3]}")
    has = {n: LINE in log(d, n) for n in ("abs", "one", "t0")}
    report(has == {"abs": False, "one": False, "t0": True}, "LINE", f"TDOA TOA SCALE line {has}")
    ok = bool(ea) and bool(et) and len(ea) >= 5 and len(et) >= 5
    ma, mt = (statistics.median(ea), statistics.median(et)) if ok else (float("nan"), float("nan"))
    report(ok and mt < 0.1 * ma, "EFFECT", f"solves {len(ea or [])} / {len(et or [])}, median tdoaErrM "
                                           f"{ma:.1f} m -> {mt:.1f} m (need >= 5 each, below 0.1 x)")
    r0 = rc(d, "r0")
    report(r0 not in (None, 0) and "FATAL: --tdoaToaScale=0 must be > 0" in log(d, "r0"), "REFUSE",
           f"--tdoaToaScale=0 exit {r0}")
    print(f"  G168: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
