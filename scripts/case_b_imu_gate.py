#!/usr/bin/env python3
"""G42: Case B resolves against the IMU-propagated prior (docs/SPEC_CONFORMANCE_PLAN.md DC-2).

Physical 2.4 resolves Case B's orientation "by IMU heading estimate rather than a second
geometric constraint". The code chose with the prior (ca-tbl-sparse-anchor.h), a recorded
deviation while the SHS heading diverges (K.9). With DC-1 and DC-11 on, Step() solves after
the controlled prediction, so the prior IS the last posterior plus the IMU's u_k: the
heading carried into position. The decision labels the prior (sparsePriorSource) and the
sparse_ CSV records it.

Reads a directory written by scripts/case_b_imu_gate.sh (imu1-4, cv1-4: NC3's regime with
four finders and --sparseInStep, with and without --imuPredict). Every check runs and is reported; exit 1 if
any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  LABEL   every sparse solve under --imuPredict is labelled IMU (priorSource 1), and every
          one without it constant-velocity (0).
  CASEB   Case B occurs under --imuPredict (fixKind 3, > 0 over the four seeds).
  PRIOR   the IMU's displacement is what moved the prior: on every sparse row under
          --imuPredict, prior = the previous posterior + (ux, uy) of the same cycle in the
          imu_ file, to 1e-3 m.
"""
import csv
import os
import sys


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def rows(d, n, f):
    return list(csv.DictReader(open(os.path.join(d, n, f))))


def main():
    if len(sys.argv) != 2:
        print("usage: case_b_imu_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    seeds = (1, 2, 3, 4)

    def c_runs():
        bad = [f"{k}{s}" for s in seeds for k in ("imu", "cv") if rc(d, f"{k}{s}") != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_label():
        bad, n = [], 0
        for s in seeds:
            for k, want in (("imu", "1"), ("cv", "0")):
                for r in rows(d, f"{k}{s}", f"sparse_ca_tbl_full_seed{s}.csv"):
                    n += 1
                    if r["priorSource"] != want:
                        bad.append((k, s, r["t"], r["priorSource"]))
        check("LABEL", n > 0 and not bad,
              f"{n} sparse rows; mislabelled prior source {len(bad)} (need 0)"
              f"{': ' + str(bad[:3]) if bad else ''}")
    safe("LABEL", c_label)

    def c_caseb():
        nb = sum(1 for s in seeds for r in rows(d, f"imu{s}", f"sparse_ca_tbl_full_seed{s}.csv")
                 if r["fixKind"] == "3")
        check("CASEB", nb > 0, f"Case B solves under --imuPredict over seeds 1-4: {nb} (need > 0)")
    safe("CASEB", c_caseb)

    def c_prior():
        bad, n = [], 0
        for s in seeds:
            imu = {"%.4f" % float(r["t"]): r for r in rows(d, f"imu{s}", f"imu_ca_tbl_full_seed{s}.csv")}
            for r in rows(d, f"imu{s}", f"sparse_ca_tbl_full_seed{s}.csv"):
                n += 1
                u = imu[r["t"]]
                wx = float(r["prevX"]) + float(u["ux"])
                wy = float(r["prevY"]) + float(u["uy"])
                if abs(float(r["priorX"]) - wx) > 1e-3 or abs(float(r["priorY"]) - wy) > 1e-3:
                    bad.append((s, r["t"], r["priorX"], wx))
        check("PRIOR", n > 0 and not bad,
              f"{n} sparse rows under --imuPredict; prior not the previous posterior + u: "
              f"{len(bad)} (need 0){': ' + str(bad[:3]) if bad else ''}")
    safe("PRIOR", c_prior)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G42 {tag:7s} {name:5s} {why}")
    print(f"  G42: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
