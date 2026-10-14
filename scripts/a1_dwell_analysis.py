#!/usr/bin/env python3
"""Score A1 (report/PREREG_A1_DWELL.md). Committed before the first cell; not edited after.

  f = cyclesInUwbCoverage / steps per (dwell, seed), from arm_summary.csv (ca_tbl_full's row;
      the label is geometric, a property of the trajectory, so every arm's row is identical --
      asserted, not assumed).
  T fitted on dwell 90 alone: T = 90 (1 - mean f90) / mean f90; predicted f = d / (d + T).
  P1: predicted f inside the measured mean's 95 % CI or within 1 pp, at 150/245/400/800.
  P2: measured mean f strictly increasing in dwell.
  A0 v2 per dwell: A_fix from traces, radio totalJ, fourth branch, kappa band.

Usage: a1_dwell_analysis.py <root> <outdir>
"""

import csv
import hashlib
import math
import os
import random
import sys

DWELLS = [90, 150, 245, 400, 800]
SEEDS = list(range(501, 521))
ARMS = ["ca_tbl_full", "static_2p4", "fixed_threshold", "static_868"]
A_REQ, R_REQ, CYC, NBOOT = 0.95, 25.0, 1440, 10000
KAPPAS = [0.25, 0.5, 1.0, 2.0]
REPORT, OUT = [], []


def say(s=""):
    print(s)
    REPORT.append(s)


def check(d):
    rid = os.path.join(d, "run_id.txt")
    if not os.path.exists(rid):
        return ["no sidecar"]
    text = open(rid).read()
    probs = [] if " dirty 0 " in text else ["not dirty 0"]
    if "digest sha256" not in text:
        return probs + ["no digest"]
    for line in text.split("digest sha256", 1)[1].strip().splitlines():
        h, name = line.split(None, 1)
        p = os.path.join(d, name.strip())
        if not os.path.exists(p) or hashlib.sha256(open(p, "rb").read()).hexdigest() != h:
            probs.append("digest " + name.strip())
    rows = list(csv.DictReader(open(os.path.join(d, "arm_summary.csv"))))
    if sorted(r["arm"] for r in rows) != sorted(ARMS):
        probs.append("rows %d" % len(rows))
    return probs


def a_fix(path):
    n = ok = 0
    for r in csv.DictReader(open(path)):
        n += 1
        if r["hadFix"] == "1" and float(r["errM"]) <= R_REQ:
            ok += 1
    return ok / CYC


def boot(vals, rng, f=lambda v: v):
    m = sum(vals) / len(vals)
    bs = sorted(sum(vals[rng.randrange(len(vals))] for _ in vals) / len(vals) for _ in range(NBOOT))
    return f(m), f(bs[int(0.025 * NBOOT)]), f(bs[int(0.975 * NBOOT) - 1])


def verdict(dT, dB, N, lo, hi, k):
    if dT == 0 and dB == 0:
        return "INCONCLUSIVE (requirement unmet by any arm)"
    dD = dT - dB
    allow = min(2.0, 1.0 + k * dD / N)
    if dD > 0 and hi < allow:
        return "PASS"
    if dD <= 0 or lo > allow:
        return "FAIL"
    return "INCONCLUSIVE"


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    root, outdir = argv[1:]
    os.makedirs(outdir, exist_ok=True)
    rng = random.Random(20261017)
    data, excluded = {}, []
    for d in DWELLS:
        for s in SEEDS:
            path = os.path.join(root, "d%d_s%d" % (d, s))
            probs = check(path) if os.path.isdir(path) else ["missing"]
            if probs:
                excluded.append((path, probs))
                continue
            rows = {r["arm"]: r for r in csv.DictReader(open(os.path.join(path, "arm_summary.csv")))}
            fs = {a: int(rows[a]["cyclesInUwbCoverage"]) / int(rows[a]["steps"]) for a in ARMS}
            if len(set(fs.values())) != 1:
                excluded.append((path, ["coverage label differs across arms -- trajectory not shared"]))
                continue
            data[(d, s)] = {"f": fs["ca_tbl_full"], "rows": rows,
                            "afix": {a: a_fix(os.path.join(path, "trace_%s_seed%d.csv" % (a, s)))
                                     for a in ("ca_tbl_full", "static_2p4")}}
    total = len(DWELLS) * len(SEEDS)
    say("=== completeness: usable %d of %d" % (len(data), total))
    for p, why in excluded:
        say("  EXCLUDED %s: %s" % (p, "; ".join(why)))
    if len(excluded) > 0.10 * total:
        say("ABORT: more than 10% excluded")
        return 1

    say("\n=== coverage fraction f against dwell; T fitted on 90 s only")
    means = {}
    for d in DWELLS:
        fs = [data[(d, s)]["f"] for s in SEEDS if (d, s) in data]
        means[d] = boot(fs, rng)
    f90 = means[90][0]
    T = 90.0 * (1.0 - f90) / f90 if f90 > 0 else float("nan")
    say("  fitted T = %.1f s (from mean f at 90 s = %.4f)" % (T, f90))
    p1 = True
    for d in DWELLS:
        m, lo, hi = means[d]
        pred = d / (d + T)
        inside = (lo <= pred <= hi) or abs(pred - m) <= 0.01
        if d != 90 and not inside:
            p1 = False
        say("  dwell %4d  measured %.4f [%.4f, %.4f]  predicted %.4f  %s" % (
            d, m, lo, hi, pred, "fit point" if d == 90 else ("OK" if inside else "MISS")))
        OUT.append({"metric": "coverage_fraction", "dwell": d, "estimate": "%.6g" % m,
                    "lo": "%.6g" % lo, "hi": "%.6g" % hi, "note": "predicted %.6g" % pred})
    p2 = all(means[a][0] < means[b][0] for a, b in zip(DWELLS, DWELLS[1:]))
    say("  P1 %s   P2 %s" % ("HOLDS" if p1 else "FAILS", "HOLDS" if p2 else "FAILS"))

    say("\n=== A0 v2 per dwell (primary: A_fix, radio energy)")
    for d in DWELLS:
        ss = [s for s in SEEDS if (d, s) in data]
        N = len(ss)
        dT = sum(1 for s in ss if data[(d, s)]["afix"]["ca_tbl_full"] >= A_REQ)
        dB = sum(1 for s in ss if data[(d, s)]["afix"]["static_2p4"] >= A_REQ)
        logs = [math.log(float(data[(d, s)]["rows"]["ca_tbl_full"]["totalJ"]) /
                         float(data[(d, s)]["rows"]["static_2p4"]["totalJ"])) for s in ss]
        g, lo, hi = boot(logs, rng, math.exp)
        v = {k: verdict(dT, dB, N, lo, hi, k) for k in KAPPAS}
        say("  dwell %4d  D_T %2d/%d  D_B %2d/%d  rho %.3f [%.3f, %.3f]  -> %s%s" % (
            d, dT, N, dB, N, g, lo, hi, v[1.0],
            "" if len(set(v.values())) == 1 else "   band: " + ", ".join("%g:%s" % (k, v[k]) for k in KAPPAS)))
        OUT.append({"metric": "A0v2", "dwell": d, "estimate": "%.6g" % g, "lo": "%.6g" % lo,
                    "hi": "%.6g" % hi, "note": "D_T=%d D_B=%d N=%d verdict=%s" % (dT, dB, N, v[1.0])})

    with open(os.path.join(outdir, "a1_analysis.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=["metric", "dwell", "estimate", "lo", "hi", "note"])
        w.writeheader()
        w.writerows(OUT)
    with open(os.path.join(outdir, "a1_report.txt"), "w") as fh:
        fh.write("\n".join(REPORT) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
