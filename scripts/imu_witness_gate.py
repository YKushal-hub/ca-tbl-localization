#!/usr/bin/env python3
"""G44: the IMU witness uses the synthetic IMU against the raw radio fix
(docs/SPEC_CONFORMANCE_PLAN.md DC-9, --imuWitnessSource=model).

The final plan's item 2: before the confidence estimate may trigger escalation, check it
against "IMU dead-reckoning consistency". --imuCrossCheck compared the SHS absolute track,
which diverges (K.9), with the EKF estimate. Under --imuWitnessSource=model the track is the
synthetic IMU's (DC-1), anchored at the last fused fix and advanced by u, and it is compared
with the cycle's RAW radio fix, which never saw u: under --imuPredict the estimate already
contains u, so it would be the IMU checking itself. Tolerance K sqrt(trace R_fix +
trace Sigma_acc), K = 3 (--imuConsistencyK). The counters are split: evaluations, refusals,
and refusals the evidence gate would also have made (absorbed).

Reads a directory written by scripts/imu_witness_gate.sh (def, shs, model, model_ev: NC9's
regime with --imuPredict --imuCrossCheck; r_noimu, r_bad). Every check runs and is reported;
exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    --imuWitnessSource=shs IS the default witness: every file byte-identical, no DC-9
          line, no witness_ file.
  TRACK   the witness track is the IMU's: each row's (wx, wy) is the previous row's, or the
          previous fused fix's estimate after a reset, plus this cycle's (ux, uy) from imu_,
          to 1e-3 m.
  TEST    each row's disagreement is |track - raw fix| and its tolerance 3 sqrt(rxx + ryy +
          sigTrace), recomputed (1e-3 m); and the raw fix is NOT the estimate (they differ on
          most rows with a fix), so the test is independent of the IMU-fed filter.
  COUNTS  an evaluated row had a tolerance > 0; a refused row was evaluated with
          disagreement > tolerance; absorbed <= refused; the DC-9 lines equal the CSVs;
          evaluations > 0; and with the evidence gate off nothing is absorbed.
  R       model without --imuPredict, and an unknown value, are refused (exit 1).
"""
import csv
import math
import os
import re
import sys

K = 3.0
LINE = re.compile(r"DC-9 witness \(model\): evaluations (\d+), refusals (\d+), absorbed by the "
                  r"evidence gate (\d+)")


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
        print("usage: imu_witness_gate.py <dir>", file=sys.stderr)
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
        bad = [n for n in ("def", "shs", "model", "model_ev") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fs = sorted(f for f in os.listdir(os.path.join(d, "shs")) if f != "run_id.txt")
        same = fd == fs and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "shs", f), "rb").read() for f in fd)
        lines = log(d, "def").count("DC-9")
        wf = [f for f in fd if f.startswith("witness_")]
        check("NULL", same and fd and lines == 0 and not wf,
              f"--imuWitnessSource=shs vs default: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"DC-9 lines {lines}, witness_ files {len(wf)} (need 0, 0)")
    safe("NULL", c_null)

    def c_track():
        bad, n = [], 0
        for run in ("model", "model_ev"):
            w = rows(d, run, "witness_ca_tbl_full_seed1.csv")
            imu = {"%.4f" % float(r["t"]): r for r in rows(d, run, "imu_ca_tbl_full_seed1.csv")}
            for a, b in zip(w, w[1:]):
                if b["anchored"] != "1":
                    continue
                ia = imu[a["t"]]
                ib = imu[b["t"]]
                base = (float(a["estX"]), float(a["estY"])) if ia["reset"] == "1" else \
                    (float(a["wx"]), float(a["wy"]))
                wx, wy = base[0] + float(ib["ux"]), base[1] + float(ib["uy"])
                n += 1
                if abs(float(b["wx"]) - wx) > 1e-3 or abs(float(b["wy"]) - wy) > 1e-3:
                    bad.append((run, b["t"], b["wx"], wx))
        check("TRACK", n > 0 and not bad,
              f"{n} anchored rows; track not the anchor + u: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("TRACK", c_track)

    def c_test():
        bad, fixes, diff = [], 0, 0
        for run in ("model", "model_ev"):
            for r in rows(d, run, "witness_ca_tbl_full_seed1.csv"):
                if r["haveFix"] != "1":
                    continue
                fixes += 1
                dis = math.hypot(float(r["wx"]) - float(r["fixX"]), float(r["wy"]) - float(r["fixY"]))
                tol = K * math.sqrt(max(0.0, float(r["rxx"]) + float(r["ryy"]) + float(r["sigTrace"])))
                if abs(dis - float(r["disagree"])) > 1e-3 or abs(tol - float(r["tol"])) > 1e-3:
                    bad.append((run, r["t"]))
                if math.hypot(float(r["fixX"]) - float(r["estX"]), float(r["fixY"]) - float(r["estY"])) > 1e-3:
                    diff += 1
        check("TEST", fixes > 0 and not bad and diff > fixes // 2,
              f"{fixes} rows with a raw fix; disagreement/tolerance not as recomputed: {len(bad)} "
              f"(need 0); raw fix differing from the estimate on {diff} (need > half)")
    safe("TEST", c_test)

    def c_counts():
        bad = []
        out = {}
        for run in ("model", "model_ev"):
            w = rows(d, run, "witness_ca_tbl_full_seed1.csv")
            for r in w:
                if r["evaluated"] == "1" and not float(r["tol"]) > 0:
                    bad.append((run, r["t"], "evaluated without a tolerance"))
                if r["refused"] == "1" and (r["evaluated"] != "1" or not float(r["disagree"]) > float(r["tol"])):
                    bad.append((run, r["t"], "refused"))
                if r["absorbed"] == "1" and r["refused"] != "1":
                    bad.append((run, r["t"], "absorbed"))
            want = (sum(r["evaluated"] == "1" for r in w), sum(r["refused"] == "1" for r in w),
                    sum(r["absorbed"] == "1" for r in w))
            m = LINE.search(log(d, run))
            got = tuple(int(x) for x in m.groups()) if m else None
            if got != want:
                bad.append((run, "line", got, want))
            out[run] = want
        ok = not bad and out["model"][0] + out["model_ev"][0] > 0 and out["model"][2] == 0
        check("COUNTS", ok,
              f"(evaluations, refusals, absorbed): evidence off {out.get('model')}, on "
              f"{out.get('model_ev')}; need evaluations > 0, nothing absorbed with the gate off; "
              f"inconsistent rows/lines {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("COUNTS", c_counts)

    def c_ref():
        got = rc(d, "r_noimu")
        has = "--imuWitnessSource=model needs --imuPredict=true" in log(d, "r_noimu")
        check("R", got == 1 and has, f"r_noimu: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
        got = rc(d, "r_bad")
        has = "--imuWitnessSource must be shs or model" in log(d, "r_bad")
        check("R", got == 1 and has, f"r_bad: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G44 {tag:7s} {name:6s} {why}")
    print(f"  G44: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
