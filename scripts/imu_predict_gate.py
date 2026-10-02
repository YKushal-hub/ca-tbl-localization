#!/usr/bin/env python3
"""G40: the filter's prediction takes a synthetic-IMU control input
(docs/SPEC_CONFORMANCE_PLAN.md DC-1, --imuPredict).

Physical 2.2 predicts x_k|k-1 = F x + B u_k with u_k IMU-derived. Under the flag a
dead-reckoning error model (CaTblImuDeadReckoner) turns each cycle's true displacement Delta
into u = (1 + s) R(psi) Delta -- s a per-run scale error, psi a heading error from a per-run
gyro bias plus angle random walk -- and Step() predicts with it, the covariance being the
increment of the ACCUMULATED error since the last fused fix:
  Sigma_acc = var_psi(t) |D|^2 p p^T + sigma_s^2 D D^T,  var_psi = sigma_b^2 t^2 + N^2 t.
imu_<arm>_seed<N>.csv records every cycle; the DC-1 stdout line the run's bias and scale.

Reads a directory written by scripts/imu_predict_gate.sh. Every check runs and is reported;
exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL     --imuPredict=false IS the default: every file byte-identical; no DC-1 line, no
           imu_ file.
  MOTION   the prediction moved by u: on every coasted cycle (hadFix 0) the estimate moved
           by exactly (ux, uy) from the previous cycle's (1e-3 m) -- and every cycle used the
           control (imuPredicted 1). At least one coasted cycle.
  MODEL    every residual is the stated error model: u = (1 + s) R(psi) Delta with s from the
           DC-1 line and psi from the row (1e-5 m); and Sigma_acc is the formula above from
           the row's D and elapsed time, with the cited constants (N = 1.6682e-4,
           sigma_b = 8.816 deg/h, sigma_s = 0.0266), to 1e-6 relative.
  QU       each Q_u is the PSD part of Sigma_acc(k) - Sigma_acc(k-1), with Sigma_acc(k-1)
           zero after a reset, or sigma_w^2 I = 0.0025 on a dwelling step -- and a reset row
           is exactly a fused fix (hadFix 1, rejected 0).
  NEES     the covariance is CONSISTENT across runs: in each of 20 seeds, at the end of its
           longest dead-reckoned stretch, the accumulated residual E = sum (u - Delta) since
           the last reset against Sigma_acc gives NEES = E^T Sigma_acc^-1 E; the mean of the 20
           lies in the 95% band of chi^2_40 / 20 = [1.22, 2.97]. One sample per seed, because
           the bias and scale are per run: samples within one run are not independent.
"""
import csv
import math
import os
import re
import sys

N_ARW = 1.6682e-4
SIG_B = 8.816 * math.pi / 180.0 / 3600.0
SIG_S = 0.0266
SIG_W2 = 0.05 ** 2
LINE = re.compile(r"DC-1 IMU: bias ([0-9.eE+-]+) rad/s, scale ([0-9.eE+-]+), cycles (\d+), resets (\d+)")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def imu(d, n):
    return list(csv.DictReader(open(os.path.join(d, n, "imu_ca_tbl_full_seed%s.csv" % seed_of(n)))))


def seed_of(n):
    return "7" if n in ("def", "off", "on") else n[1:]


def psd(m):
    a, b, c = m
    tr = a + c
    disc = math.sqrt(max(0.0, (a - c) ** 2 / 4 + b * b))
    l1, l2 = tr / 2 + disc, tr / 2 - disc
    if l2 >= 0:
        return [a, b, c]
    if l1 <= 0:
        return [0.0, 0.0, 0.0]
    if abs(b) > 1e-300:
        vx, vy = l1 - c, b
    else:
        vx, vy = (1.0, 0.0) if a >= c else (0.0, 1.0)
    n = math.hypot(vx, vy)
    vx, vy = vx / n, vy / n
    return [l1 * vx * vx, l1 * vx * vy, l1 * vy * vy]


def sigma(Dx, Dy, t):
    v = SIG_B ** 2 * t * t + N_ARW ** 2 * t
    ss = SIG_S ** 2
    return [v * Dy * Dy + ss * Dx * Dx, -v * Dx * Dy + ss * Dx * Dy, v * Dx * Dx + ss * Dy * Dy]


def main():
    if len(sys.argv) != 2:
        print("usage: imu_predict_gate.py <dir>", file=sys.stderr)
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
        names = ["def", "off", "on"] + [f"s{s}" for s in range(1, 21)]
        bad = [n for n in names if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fo = sorted(f for f in os.listdir(os.path.join(d, "off")) if f != "run_id.txt")
        same = fd == fo and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "off", f), "rb").read() for f in fd)
        lines = log(d, "def").count("DC-1")
        im = [f for f in fd if f.startswith("imu_")]
        check("NULL", same and fd and lines == 0 and not im,
              f"--imuPredict=false vs default: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"DC-1 lines {lines}, imu_ files {len(im)} (need 0, 0)")
    safe("NULL", c_null)

    def c_motion():
        r = imu(d, "on")
        bad, coast = [], 0
        for a, b in zip(r, r[1:]):
            if b["imuPredicted"] != "1":
                bad.append((b["t"], "no control"))
            if b["hadFix"] == "0":
                coast += 1
                mx = float(b["estX"]) - float(a["estX"])
                my = float(b["estY"]) - float(a["estY"])
                if abs(mx - float(b["ux"])) > 1e-3 or abs(my - float(b["uy"])) > 1e-3:
                    bad.append((b["t"], mx, float(b["ux"]), my, float(b["uy"])))
        check("MOTION", coast > 0 and not bad,
              f"on: {coast} coasted cycles; estimate not moved by u, or a cycle without the "
              f"control: {len(bad)} (need 0){': ' + str(bad[:3]) if bad else ''}")
    safe("MOTION", c_motion)

    def c_model():
        m = LINE.search(log(d, "on"))
        s = float(m.group(2))
        bad = []
        for r in imu(d, "on"):
            dx, dy, psi = float(r["dxTrue"]), float(r["dyTrue"]), float(r["psi"])
            wx = (1 + s) * (math.cos(psi) * dx - math.sin(psi) * dy)
            wy = (1 + s) * (math.sin(psi) * dx + math.cos(psi) * dy)
            if abs(wx - float(r["ux"])) > 1e-5 or abs(wy - float(r["uy"])) > 1e-5:
                bad.append((r["t"], "u"))
            want = sigma(float(r["accX"]), float(r["accY"]), float(r["elapsedS"]))
            got = [float(r["sigXX"]), float(r["sigXY"]), float(r["sigYY"])]
            if any(abs(g - w) > 1e-6 * (abs(w) + 1e-9) + 1e-9 for g, w in zip(got, want)):
                bad.append((r["t"], "sigma", got, want))
        check("MODEL", m and not bad,
              f"on: scale {s if m else '?'}; rows off the stated model: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("MODEL", c_model)

    def c_qu():
        bad = []
        prev = [0.0, 0.0, 0.0]
        for r in imu(d, "on"):
            got = [float(r["quxx"]), float(r["quxy"]), float(r["quyy"])]
            cur = [float(r["sigXX"]), float(r["sigXY"]), float(r["sigYY"])]
            moving = float(r["dxTrue"]) != 0.0 or float(r["dyTrue"]) != 0.0
            want = psd([c - p for c, p in zip(cur, prev)]) if moving else [SIG_W2, 0.0, SIG_W2]
            if any(abs(g - w) > 1e-6 * (abs(w) + 1e-9) + 1e-9 for g, w in zip(got, want)):
                bad.append((r["t"], got, want))
            if (r["reset"] == "1") != (r["hadFix"] == "1" and r["rejected"] == "0"):
                bad.append((r["t"], "reset"))
            prev = [0.0, 0.0, 0.0] if r["reset"] == "1" else cur
        check("QU", not bad,
              f"on: Q_u not the PSD increment of Sigma_acc, or a reset that is not a fused fix: "
              f"{len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("QU", c_qu)

    def c_nees():
        vals = []
        for s in range(1, 21):
            rows = imu(d, f"s{s}")
            best, run, start = (0, None), 0, 0
            ex = ey = 0.0
            for i, r in enumerate(rows):
                ex += float(r["ux"]) - float(r["dxTrue"])
                ey += float(r["uy"]) - float(r["dyTrue"])
                r["_E"] = (ex, ey)
                if r["reset"] == "1":
                    ex = ey = 0.0
                    run = 0
                else:
                    run += 1
                    if run > best[0]:
                        best = (run, i)
            if best[1] is None:
                continue
            r = rows[best[1]]
            Ex, Ey = r["_E"]
            a, b, c = float(r["sigXX"]), float(r["sigXY"]), float(r["sigYY"])
            det = a * c - b * b
            if det <= 0:
                continue
            vals.append((c * Ex * Ex - 2 * b * Ex * Ey + a * Ey * Ey) / det)
        mean = sum(vals) / len(vals) if vals else float("nan")
        frac = sum(v <= 5.991 for v in vals) / len(vals) if vals else float("nan")
        check("NEES", len(vals) == 20 and 1.22 <= mean <= 2.97,
              f"{len(vals)} seeds (need 20); mean NEES at each seed's longest dead-reckoned "
              f"stretch {mean:.3f} (95% band [1.22, 2.97]); under chi^2_2's 95% point {frac:.2f}")
    safe("NEES", c_nees)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G40 {tag:7s} {name:6s} {why}")
    print(f"  G40: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
