#!/usr/bin/env python3
"""G41: Case D, a single range as a constraint on the propagated state
(docs/SPEC_CONFORMANCE_PLAN.md DC-4, --caseD).

Physical 2.4D: with only one peer or anchor range, propagate with the IMU and incorporate
the single range as a measurement constraint; the estimate's covariance stays live, so PCS
falls naturally. Step() used to discard a single range and coast, and the cycle counted
toward demotion. Under the flag it is applied as a scalar update when NIS <= 6.63, the cycle
(fixKind CASE_D) reaches the normal escalation decision, and it does not count toward
demotion. cased_<arm>_seed<N>.csv records every Case D cycle.

Reads a directory written by scripts/case_d_gate.sh (base, off, on: NC3's regime with
--sparseInStep --imuPredict; r_nosparse; w_noimu). Every check runs and is reported; exit 1
if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL      --caseD=false IS the flag absent: every file byte-identical, no DC-4 line, no
            cased_ file.
  GUARD     every Case D row went in exactly when its NIS was <= 6.63, and some went in.
  TRACE     every Case D row's cycle is, in the trace, a one-range cycle that is NOT a
            position fix (hadFix 0, nAnchors 1), predicted with the IMU (imu_ row
            imuPredicted 1), and not demoted.
  DECISION  every Case D cycle reached the decision: its PCS was computed (> 0, where a
            coast leaves 0), and the confidence model wanted to escalate on some of them.
  LINE      the DC-4 stdout counts equal the CSV's.
  R         --caseD without --sparseInStep is refused (exit 1); without --imuPredict it runs
            with a WARNING (exit 0).
"""
import csv
import os
import re
import sys

LINE = re.compile(r"DC-4 Case D: cycles (\d+), updates applied (\d+), refused by NIS (\d+); wanted "
                  r"escalation (\d+), escalated (\d+), demoted (\d+)")


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
        print("usage: case_d_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in ("base", "off", "on", "w_noimu") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fb = sorted(f for f in os.listdir(os.path.join(d, "base")) if f != "run_id.txt")
        fo = sorted(f for f in os.listdir(os.path.join(d, "off")) if f != "run_id.txt")
        same = fb == fo and all(open(os.path.join(d, "base", f), "rb").read() ==
                                open(os.path.join(d, "off", f), "rb").read() for f in fb)
        lines = log(d, "base").count("DC-4")
        cd = [f for f in fb if f.startswith("cased_")]
        check("NULL", same and fb and lines == 0 and not cd,
              f"--caseD=false vs absent: {len(fb)} files, {'identical' if same else 'DIFFER'}; "
              f"DC-4 lines {lines}, cased_ files {len(cd)} (need 0, 0)")
    safe("NULL", c_null)

    def c_guard():
        cd = rows(d, "on", "cased_ca_tbl_full_seed7.csv")
        bad = [r["t"] for r in cd if (r["applied"] == "1") != (0.0 <= float(r["nis"]) <= 6.63)]
        app = sum(r["applied"] == "1" for r in cd)
        check("GUARD", cd and app > 0 and not bad,
              f"on: {len(cd)} Case D cycles, {app} applied (need > 0); applied but NIS > 6.63, "
              f"or refused with NIS <= 6.63: {len(bad)} (need 0)")
    safe("GUARD", c_guard)

    def c_trace():
        cd = rows(d, "on", "cased_ca_tbl_full_seed7.csv")
        tr = {r["t"]: r for r in rows(d, "on", "trace_ca_tbl_full_seed7.csv")}
        im = {}
        for r in rows(d, "on", "imu_ca_tbl_full_seed7.csv"):
            im["%.4f" % float(r["t"])] = r
        bad = []
        for r in cd:
            t = tr[r["t"]]
            i = im[r["t"]]
            if t["hadFix"] != "0" or t["nAnchors"] != "1" or i["imuPredicted"] != "1" or \
                    r["deescalated"] != "0" or t["deescalated"] != "0":
                bad.append((r["t"], t["hadFix"], t["nAnchors"], i["imuPredicted"], t["deescalated"]))
        check("TRACE", cd and not bad,
              f"on: Case D rows that are, in the trace, not a one-range non-fix IMU-predicted "
              f"cycle, or that demoted: {len(bad)} (need 0){': ' + str(bad[:3]) if bad else ''}")
    safe("TRACE", c_trace)

    def c_decision():
        cd = rows(d, "on", "cased_ca_tbl_full_seed7.csv")
        nopcs = [r["t"] for r in cd if not float(r["pcs"]) > 0.0]
        wanted = sum(r["wantedEscalation"] == "1" for r in cd)
        check("DECISION", cd and not nopcs and wanted > 0,
              f"on: Case D rows with no PCS computed {len(nopcs)} (need 0); rows where the "
              f"confidence model wanted to escalate {wanted} (need > 0)")
    safe("DECISION", c_decision)

    def c_line():
        m = LINE.search(log(d, "on"))
        cd = rows(d, "on", "cased_ca_tbl_full_seed7.csv")
        want = (len(cd), sum(r["applied"] == "1" for r in cd), sum(r["applied"] == "0" for r in cd),
                sum(r["wantedEscalation"] == "1" for r in cd), sum(r["escalated"] == "1" for r in cd),
                sum(r["deescalated"] == "1" for r in cd))
        got = tuple(int(x) for x in m.groups()) if m else None
        check("LINE", got == want, f"on: DC-4 line {got} vs CSV {want} (need equal)")
    safe("LINE", c_line)

    def c_ref():
        got = rc(d, "r_nosparse")
        has = "--caseD needs --sparseInStep=true" in log(d, "r_nosparse")
        check("R", got == 1 and has, f"r_nosparse: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
        got = rc(d, "w_noimu")
        has = "WARNING: --caseD without --imuPredict" in log(d, "w_noimu")
        check("R", got == 0 and has, f"w_noimu: exit {got} (need 0), warning {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G41 {tag:7s} {name:8s} {why}")
    print(f"  G41: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
