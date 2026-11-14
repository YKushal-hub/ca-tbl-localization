#!/usr/bin/env python3
"""G167: --oracleTrigger (report/PREREG_ORACLE_ESC.md). usage: oracle_esc_gate.py <dir of oracle_esc_gate.sh>

  RUNS   abs, off and on exit 0 and write fixed_threshold's trace
  OFF    off (--oracleTrigger=false) is byte-identical to abs (flag absent) in every file but run_id.txt
  LINE   the ORACLE TRIGGER line is in on's log and in neither abs's nor off's
  ARMS   on's ca_tbl_full files are byte-identical to abs's (the flag reaches no other arm)
  COUNT  on counts decisions on the true error for fixed_threshold's tag 0 and subjects, none for ca_tbl_full
  RULE   on: every fixed_threshold de-escalation row has errM^2 < T/2 (at least one); abs: every one has
         traceSigma < T/2 and at least one has errM^2 >= T/2, so the check tells the two triggers apart
"""
import csv
import filecmp
import os
import re
import sys

T = 12.0
LINE = "ORACLE TRIGGER (--oracleTrigger)"
nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G167 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, f"{n}.rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def log(d, n):
    p = os.path.join(d, f"{n}.log")
    return open(p, errors="replace").read() if os.path.exists(p) else ""


def deesc(d, n):
    p = os.path.join(d, n, "trace_fixed_threshold_seed7.csv")
    if not os.path.exists(p):
        return None
    return [r for r in csv.DictReader(open(p)) if r["deescalated"] == "1" and r["hadFix"] == "1"]


def same(a, b, pick):
    names = sorted(f for f in os.listdir(a) if pick(f)) if os.path.isdir(a) else []
    bad = [f for f in names if not (os.path.exists(os.path.join(b, f)) and
                                    filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False))]
    return names, bad


def decisions(text, arm):
    m = re.search(rf"ORACLE-ESC oracleTrigger \({arm}\): decisions on the true error, tag 0 (\d+), subjects (\d+)", text)
    return (int(m.group(1)), int(m.group(2))) if m else None


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("abs", "off", "on")}
    have = all(os.path.exists(os.path.join(d, n, "trace_fixed_threshold_seed7.csv")) for n in rcs)
    report(all(v == 0 for v in rcs.values()) and have, "RUNS", f"exit codes {rcs}, traces {have} (need all 0, True)")
    names, bad = same(os.path.join(d, "abs"), os.path.join(d, "off"), lambda f: f != "run_id.txt")
    report(bool(names) and not bad, "OFF", f"{len(names) - len(bad)}/{len(names)} files identical, abs vs off {bad[:3]}")
    has = {n: LINE in log(d, n) for n in ("abs", "off", "on")}
    report(has == {"abs": False, "off": False, "on": True}, "LINE", f"ORACLE TRIGGER line {has}")
    names, bad = same(os.path.join(d, "abs"), os.path.join(d, "on"), lambda f: "ca_tbl_full" in f)
    report(bool(names) and not bad, "ARMS", f"{len(names) - len(bad)}/{len(names)} ca_tbl_full files identical {bad[:3]}")
    ft, cf = decisions(log(d, "on"), "fixed_threshold"), decisions(log(d, "on"), "ca_tbl_full")
    report(ft is not None and min(ft) > 0 and cf == (0, 0), "COUNT",
           f"decisions on the true error: fixed_threshold {ft}, ca_tbl_full {cf} (need both > 0; (0, 0))")
    on, ab = deesc(d, "on"), deesc(d, "abs")
    ok = on is not None and ab is not None and len(on) > 0 and len(ab) > 0
    on_e = sum(float(r["errM"]) ** 2 < T / 2 for r in on) if ok else 0
    ab_t = sum(float(r["traceSigma"]) < T / 2 for r in ab) if ok else 0
    ab_e = sum(float(r["errM"]) ** 2 >= T / 2 for r in ab) if ok else 0
    ok = ok and on_e == len(on) and ab_t == len(ab) and ab_e > 0
    report(ok, "RULE", f"on: {on_e}/{len(on or [])} de-escalations with errM^2 < {T / 2:g}; abs: {ab_t}/{len(ab or [])} "
                       f"with traceSigma < {T / 2:g}, {ab_e} with errM^2 >= {T / 2:g} (need all, all, > 0)")
    print(f"  G167: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
