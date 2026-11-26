#!/usr/bin/env python3
"""Realistic-deployment study: emergency and quiet-shift measures for one run directory per seed.

Emergency (needs --searchLaunchAtS, the alarm time, read from the run's "party deployed at t=" line):
  t25 / t5   seconds from the alarm to the lost worker's (node 1) first estimate within 25 m / 5 m (NaN: never)
  treach     seconds from the alarm until a searcher (node >= 500) is within REACH_M of the worker (NaN: never)
  err_alarm  the lost worker's estimate error at the alarm, m
Quiet shift (any run): subject energy per hour (J/h), and the share of all worker-cycles with an estimate within
  25 m and within 250 m (a usable rough position), pooled over the five workers.
usage: realistic_analyse.py <run dir> <arm> [reach_m=10]  -> prints one CSV line
       (imported by the campaign analysers; nothing here runs the simulator)
"""
import csv
import glob
import math
import os
import re
import sys

REACH_M = 10.0


def alarm_time(d):
    for lg in (os.path.join(d, "run.log"), d.rstrip("/") + ".log"):
        if os.path.exists(lg):
            m = re.search(r"party deployed at t=([0-9.]+) s", open(lg, errors="replace").read())
            if m:
                return float(m.group(1))
    return float("nan")


def emergency(d, arm, reach_m=REACH_M):
    seed = re.search(r"s(\d+)/?$", d).group(1)
    ta = alarm_time(d)
    err = {}
    for r in csv.DictReader(open(os.path.join(d, f"trace_{arm}_seed{seed}.csv"))):
        err[round(float(r["t"]), 2)] = float(r["errM"])
    pos = {}
    for r in csv.DictReader(open(os.path.join(d, f"nodes_{arm}_seed{seed}.csv"))):
        pos.setdefault(round(float(r["t"]), 2), {})[int(r["nodeId"])] = (float(r["x"]), float(r["y"]))
    ts = sorted(t for t in pos if not math.isnan(ta) and t >= ta)
    nan = float("nan")
    t25 = next((t - ta for t in ts if err.get(t, 1e18) <= 25.0), nan)
    t5 = next((t - ta for t in ts if err.get(t, 1e18) <= 5.0), nan)
    treach = next((t - ta for t in ts if 1 in pos[t] and any(
        i >= 500 and math.dist(p, pos[t][1]) <= reach_m for i, p in pos[t].items())), nan)
    e0 = err.get(ts[0], nan) if ts else nan
    return {"alarm_s": ta, "t25_s": t25, "t5_s": t5, "treach_s": treach, "err_alarm_m": e0}


def quiet(d, arm):
    seed = re.search(r"s(\d+)/?$", d).group(1)
    e = []
    for f in [os.path.join(d, f"trace_{arm}_seed{seed}.csv")] + sorted(glob.glob(os.path.join(d, f"subjtrace_{arm}_seed{seed}_subj*.csv"))):
        if os.path.exists(f):
            e += [float(r["errM"]) for r in csv.DictReader(open(f))]
    rows = [r for r in csv.DictReader(open(os.path.join(d, "subject_summary.csv"))) if r["arm"] == arm]
    hours = float(rows[0]["steps"]) * 2.5 / 3600.0 if rows else float("nan")
    jph = (sum(float(r["totalJ"]) for r in rows) / len(rows)) / hours if rows else float("nan")
    n = len(e) or 1
    return {"subjJ_per_h": jph, "A25_five": sum(x <= 25 for x in e) / n, "A250_five": sum(x <= 250 for x in e) / n}


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(2)
    d, arm = sys.argv[1], sys.argv[2]
    r = float(sys.argv[3]) if len(sys.argv) > 3 else REACH_M
    out = {**emergency(d, arm, r), **quiet(d, arm)}
    print(",".join(out.keys()))
    print(",".join(f"{v:.4g}" for v in out.values()))
