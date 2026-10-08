#!/usr/bin/env python3
"""G90: register row 17 -- PCS term 1 as the spec's literal 1/trace(Sigma).

usage: pcs_term1_gate.py <dir written by scripts/pcs_term1_gate.sh>

Under the legacy form pcs = t1 + t2 + t3, with t2 = 1/max(GDOP, 1) and t3 in [0, 1]. So from
each fix row of the trace, t3 = pcs - t1 - t2 must land in [0, 1] for the t1 the run used,
within the rounding of the four-decimal columns.

Checks (each prints "G90 ok|FAIL|MISSING <LABEL> ..."):
  RUNS    both runs exit 0.
  ON      with the flag, t1 = 1/trace puts t3 in [0, 1] on every fix row.
  OFF     without it, t1 = 10/(10 + trace) does (the control: the bounded form still holds).
  REACH   rule 4: on the flag's run the bounded reading puts t3 out of range on some fix rows,
          so the two forms differ on this data (rows with 1/trace > 1 are reported).
  R       --pcsTerm1Inverse with --pcsForm=v2 exits 1 with the reason.
"""
import csv
import glob
import os
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G90 ok      {label:<6}{msg}")
    else:
        nfail += 1
        print(f"  G90 FAIL    {label:<6}{msg}")


def rows(d, run):
    f = glob.glob(os.path.join(d, run, "trace_ca_tbl_full_seed*.csv"))
    if not f:
        return None
    out = []
    for r in csv.DictReader(open(f[0])):
        if r["hadFix"] == "1" and float(r["gdop"]) != 0.0 and float(r["traceSigma"]) > 0.0:
            out.append((float(r["pcs"]), float(r["traceSigma"]), float(r["gdop"])))
    return out


def t3s(rs, inverse):
    res = []
    for pcs, tr, g in rs:
        t1 = 1.0 / tr if inverse else 10.0 / (10.0 + tr)
        # rounding of the 4-decimal columns: pcs, trace and gdop each +-5e-5
        tol = 5e-5 * (1.0 + (1.0 / tr ** 2 if inverse else 10.0 / (10.0 + tr) ** 2) + 1.0 / max(g, 1.0) ** 2) + 1e-6
        res.append((pcs - t1 - 1.0 / max(g, 1.0), tol, tr))
    return res


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "both runs exited 0" if not bad else f"non-zero exit: {bad}")
    on, off = rows(d, "on"), rows(d, "off")
    if on is None or off is None:
        global nmiss
        nmiss += 1
        print("  G90 MISSING ON    no trace")
        return 1
    b_on = [(t3, tol, tr) for t3, tol, tr in t3s(on, True) if not (-tol <= t3 <= 1 + tol)]
    report(on and not b_on, "ON", f"{len(on)} fix rows; t3 out of [0, 1] with t1 = 1/trace: {len(b_on)} (need 0)"
           + (f": {b_on[:3]}" if b_on else ""))
    b_off = [(t3, tol, tr) for t3, tol, tr in t3s(off, False) if not (-tol <= t3 <= 1 + tol)]
    report(off and not b_off, "OFF", f"{len(off)} fix rows; t3 out of [0, 1] with t1 = 10/(10+trace): {len(b_off)} (need 0)"
           + (f": {b_off[:3]}" if b_off else ""))
    big = [r for r in on if 1.0 / r[1] > 1.0]
    wrong = [x for x in t3s(on, False) if not (-x[1] <= x[0] <= 1 + x[1])]
    report(len(wrong) > 0, "REACH",
           f"rows the bounded reading cannot explain: {len(wrong)} (need > 0); rows with 1/trace > 1: {len(big)}")
    try:
        t = open(os.path.join(d, "r_v2.log")).read()
    except OSError:
        t = ""
    report(rc(d, "r_v2") == 1 and "--pcsTerm1Inverse with --pcsForm=v2" in t, "R",
           f"r_v2: exit {rc(d, 'r_v2')} (need 1)")
    print(f"  G90: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
