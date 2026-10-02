#!/usr/bin/env python3
"""G39: Cases A/B solved inside Step(), after the prediction
(docs/SPEC_CONFORMANCE_PLAN.md DC-11, --sparseInStep).

Physical 2.4 resolves the sparse cases "against the node's own propagated prior". By
default --sparseCaseA solves in the scenario BEFORE Step(), against the previous posterior,
fuses the point a cycle early, and Step() then counts the cycle as a no-fix and toward
demotion; with relays appended it range-updates on the same pair again. Under the flag
Step() solves after Predict(), fuses once, and a solved cycle is a fix.

Reads a directory written by scripts/sparse_in_step_gate.sh (def, off, old, new: NC3's
regime; relay: NC4's; r_both). Every check runs and is reported; exit 1 if any failed, 2 if
an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    --sparseInStep=false IS the default: every file byte-identical; no DC-11 line, no
          sparse_ file.
  OLD     the default path's defect, observed: every Case A/B the old path admitted (its
          "sect 7 CASE A/B" line) sits on a trace row with hadFix = 0 -- a solved cycle
          recorded as a no-fix. Rows with nAnchors = 2 and hadFix = 0 >= admitted > 0.
  STEP    every sparse_ row has two ranges; an accepted one (verdict 0) is Case A or B,
          one position fuse, ZERO range updates and a positive variance -- the pair is
          never also used as ranges -- and a refused one fused nothing.
  TRACE   the trace agrees: an accepted row's cycle has hadFix = 1, nAnchors = 2, the
          CSV's estimate, and no demotion; a refused row's has hadFix = 0. noFixCycles in
          arm_summary equals the trace's hadFix = 0 rows.
  PRIOR   the solve read the PROPAGATED prior: on some rows it differs from the previous
          posterior (the old path's prior, recorded beside it) -- by v dt.
  CENSUS  the DC-11 line's counts equal the CSV's, and the sect 7 census still classifies
          every cycle (complete).
  RELAY   in the relayed regime, two-range cycles completed by relays take the full path
          (> 0 of them), and no cycle both fuses a sparse point and range-updates.
  R       --sparseCaseA with --sparseInStep is refused (exit 1).
"""
import csv
import os
import re
import sys

DC11 = re.compile(r"DC-11 sparse in Step: attempts (\d+), case A (\d+), case B (\d+), refused degenerate "
                  r"(\d+) ambiguous (\d+) no-prior (\d+); two-range cycles made full by relays (\d+)")
OLDAB = re.compile(r"sect 7 CASE A/B: attempts (\d+), admitted (\d+)")
CENSUS = re.compile(r"sect 7 CASES: classified (\d+) of (\d+) cycles \((complete|INCOMPLETE)\)")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(d, n, f):
    return list(csv.DictReader(open(os.path.join(d, n, f))))


def main():
    if len(sys.argv) != 2:
        print("usage: sparse_in_step_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError, StopIteration) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in ("def", "off", "old", "new", "relay") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fo = sorted(f for f in os.listdir(os.path.join(d, "off")) if f != "run_id.txt")
        same = fd == fo and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "off", f), "rb").read() for f in fd)
        lines = log(d, "def").count("DC-11")
        sp = [f for f in fd if f.startswith("sparse_")]
        check("NULL", same and fd and lines == 0 and not sp,
              f"--sparseInStep=false vs default: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"DC-11 lines {lines}, sparse_ files {len(sp)} (need 0, 0)")
    safe("NULL", c_null)

    def c_old():
        m = OLDAB.search(log(d, "old"))
        adm = int(m.group(2)) if m else 0
        tr = rows(d, "old", "trace_ca_tbl_full_seed7.csv")
        two_nofix = sum(1 for r in tr if r["nAnchors"] == "2" and r["hadFix"] == "0")
        check("OLD", m and adm > 0 and two_nofix >= adm,
              f"old path: admitted {adm} (need > 0); trace rows with two ranges recorded as a "
              f"no-fix {two_nofix} (need >= admitted)")
    safe("OLD", c_old)

    def c_step():
        sp = rows(d, "new", "sparse_ca_tbl_full_seed7.csv")
        bad = []
        ok = 0
        for r in sp:
            v, k = int(r["verdict"]), int(r["fixKind"])
            ru, pf, var = int(r["rangeUpdates"]), int(r["positionFuses"]), float(r["varM2"])
            if r["nMeas"] != "2" or v not in (0, 1, 2, 3):
                bad.append((r["t"], "shape"))
            elif v == 0:
                ok += 1
                if k not in (2, 3) or pf != 1 or ru != 0 or not var > 0:
                    bad.append((r["t"], v, k, ru, pf, var))
            elif k != 0 or pf != 0 or ru != 0:
                bad.append((r["t"], v, k, ru, pf))
        check("STEP", sp and ok > 0 and not bad,
              f"new: {len(sp)} sparse rows, {ok} accepted (need > 0); wrong: {len(bad)} (need 0)"
              f"{': ' + str(bad[:3]) if bad else ''}")
    safe("STEP", c_step)

    def c_trace():
        sp = rows(d, "new", "sparse_ca_tbl_full_seed7.csv")
        tr = {r["t"]: r for r in rows(d, "new", "trace_ca_tbl_full_seed7.csv")}
        bad = []
        for r in sp:
            t = tr[r["t"]]
            if r["verdict"] == "0":
                if t["hadFix"] != "1" or t["nAnchors"] != "2" or t["deescalated"] != "0" or \
                        abs(float(t["estX"]) - float(r["estX"])) > 1e-3 or \
                        abs(float(t["estY"]) - float(r["estY"])) > 1e-3:
                    bad.append((r["t"], "accepted", t["hadFix"], t["nAnchors"], t["deescalated"]))
            elif t["hadFix"] != "0":
                bad.append((r["t"], "refused", t["hadFix"]))
        nofix = sum(1 for t in tr.values() if t["hadFix"] == "0")
        summ = rows(d, "new", "arm_summary.csv")[0]
        check("TRACE", not bad and int(summ["noFixCycles"]) == nofix,
              f"new: sparse rows disagreeing with the trace {len(bad)} (need 0)"
              f"{': ' + str(bad[:3]) if bad else ''}; noFixCycles {summ['noFixCycles']} vs trace "
              f"hadFix=0 rows {nofix} (need equal)")
    safe("TRACE", c_trace)

    def c_prior():
        sp = rows(d, "new", "sparse_ca_tbl_full_seed7.csv")
        moved = sum(1 for r in sp if abs(float(r["priorX"]) - float(r["prevX"])) +
                    abs(float(r["priorY"]) - float(r["prevY"])) > 1e-4)
        check("PRIOR", moved > 0,
              f"new: rows whose propagated prior differs from the previous posterior {moved} of "
              f"{len(sp)} (need > 0)")
    safe("PRIOR", c_prior)

    def c_census():
        m = DC11.search(log(d, "new"))
        sp = rows(d, "new", "sparse_ca_tbl_full_seed7.csv")
        want = (len(sp), sum(1 for r in sp if r["fixKind"] == "2"), sum(1 for r in sp if r["fixKind"] == "3"),
                sum(1 for r in sp if r["verdict"] == "1"), sum(1 for r in sp if r["verdict"] == "2"),
                sum(1 for r in sp if r["verdict"] == "3"))
        got = tuple(int(x) for x in m.groups()[:6]) if m else None
        c = CENSUS.search(log(d, "new"))
        check("CENSUS", got == want and c and c.group(3) == "complete",
              f"new: DC-11 line {got} vs CSV {want} (need equal); census "
              f"{c.group(0) if c else 'ABSENT'}")
    safe("CENSUS", c_census)

    def c_relay():
        m = DC11.search(log(d, "relay"))
        full = int(m.group(7)) if m else 0
        sp = [r for r in rows(d, "relay", "sparse_static_2p4_seed7.csv")]
        both = sum(1 for r in sp if int(r["rangeUpdates"]) > 0 and int(r["positionFuses"]) > 0)
        check("RELAY", m and full > 0 and both == 0,
              f"relay: two-range cycles completed by relays {full} (need > 0); cycles that fused a "
              f"sparse point AND range-updated {both} (need 0)")
    safe("RELAY", c_relay)

    def c_ref():
        got = rc(d, "r_both")
        has = "--sparseInStep replaces --sparseCaseA" in log(d, "r_both")
        check("R", got == 1 and has, f"r_both: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G39 {tag:7s} {name:6s} {why}")
    print(f"  G39: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
