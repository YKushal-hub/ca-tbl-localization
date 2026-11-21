#!/usr/bin/env python3
"""Digging order (plan B6, DD 41): who to dig out first, and how many are reached alive.

Analysis side (reads the truth in the traces). The ORDERING POLICIES see only what the system knows: each
worker's estimated position and its uncertainty (trace / subjtrace columns estX, estY, traceSigma) at the
decision time. The SCORE uses the truth: a dig at the estimate finds the worker if the true horizontal
error is within the dig radius r; otherwise the team widens the hole, which costs time in proportion to the
extra area: t = t_dig * (1 + max(0, (e^2 - r^2) / r^2)).

Sourced or swept inputs:
  dig teams       2 sites (INSARAG-HEAVY: a heavy team runs "24 hour operations at 2 separate sites"),
                  1 (INSARAG-MED)
  t_dig           swept 0.5 h / 2 h / 8 h: the span of De Bruycker 1983's extrication times
                  (DEBRUY-TIMES: 24.4% within 1/2 h, 50% by 8 h). No per-victim extrication-effort
                  source was found; this is a sweep, not a measured constant.
  r               dig radius, swept 1 m / 2 m (assumption, reported as such)
  survival S(t)   DEBRUY-T2 step function: day 0 0.879, day 1 0.351, days 2-3 0.0857, day >= 4 0
  decision time   when localisation stops and digging starts (the run's end by default)

Policies:
  id          worker id order (arbitrary baseline)
  nearest     nearest estimate to the base camp first
  confident   smallest positional uncertainty first
  spt         shortest EXPECTED dig time first, from the system's own uncertainty:
              E[t] = t_dig (1 + (s^2 / r^2) exp(-r^2 / s^2)) for a circular error with E[e^2] = s^2
              (minimises total completion time on parallel teams for equal weights; the weighted form
              is Wex et al., EJOR 2014)
  oracle      shortest TRUE dig time first (a bound, not a policy)

usage: score_dig.py RUN_DIR ARM [--teams 2] [--tdig 2] [--radius 2] [--decide END|seconds] [--base X,Y]
"""
import argparse
import csv
import glob
import math
import os

DEBRUY_T2 = [(1.0, 333 / 379), (2.0, 20 / 57), (4.0, 3 / 35)]  # (until day, P alive); day >= 4: 0


def survival(t_h):
    d = t_h / 24.0
    for until, p in DEBRUY_T2:
        if d < until:
            return p
    return 0.0


def load(run, arm, decide):
    workers = {}
    files = [(1, os.path.join(run, f"trace_{arm}_seed{{}}.csv"))]
    tr = glob.glob(os.path.join(run, f"trace_{arm}_seed*.csv"))
    sub = glob.glob(os.path.join(run, f"subjtrace_{arm}_seed*_subj*.csv"))
    rows_by = {}
    for f in tr:
        rows_by[1] = list(csv.DictReader(open(f)))
    for f in sub:
        sid = int(f.rsplit("subj", 1)[1].split(".")[0])
        rows_by[sid] = list(csv.DictReader(open(f)))
    for sid, rows in rows_by.items():
        if not rows:
            continue
        pick = rows[-1]
        if decide != "END":
            cand = [r for r in rows if float(r["t"]) <= float(decide)]
            if cand:
                pick = cand[-1]
        workers[sid] = dict(est=(float(pick["estX"]), float(pick["estY"])),
                            true=(float(pick["trueX"]), float(pick["trueY"])),
                            sigma=float(pick["traceSigma"]), t=float(pick["t"]))
    return workers


def dig_time(err, tdig, r):
    return tdig * (1.0 + max(0.0, (err * err - r * r) / (r * r)))


def schedule(order, W, teams, tdig, r, start_h):
    free = [start_h] * teams
    alive = 0.0
    wasted = 0.0
    done = []
    for sid in order:
        k = min(range(teams), key=lambda i: free[i])
        e = math.dist(W[sid]["est"], W[sid]["true"])
        t = dig_time(e, tdig, r)
        free[k] += t
        wasted += t - tdig
        p = survival(free[k])
        alive += p
        done.append((sid, free[k], p))
    return alive, wasted, done


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run")
    ap.add_argument("arm")
    ap.add_argument("--teams", type=int, default=2)
    ap.add_argument("--tdig", type=float, default=2.0)
    ap.add_argument("--radius", type=float, default=2.0)
    ap.add_argument("--decide", default="END")
    ap.add_argument("--base", default="0,0")
    a = ap.parse_args()
    W = load(a.run, a.arm, a.decide)
    if not W:
        raise SystemExit("no traces for that arm")
    bx, by = (float(v) for v in a.base.split(","))
    start_h = max(w["t"] for w in W.values()) / 3600.0
    s2 = lambda s: max(s, 1e-6)
    exp_t = {k: a.tdig * (1 + (s2(w["sigma"]) ** 2 / a.radius ** 2) * math.exp(-a.radius ** 2 / s2(w["sigma"]) ** 2))
             for k, w in W.items()}
    true_t = {k: dig_time(math.dist(w["est"], w["true"]), a.tdig, a.radius) for k, w in W.items()}
    pol = {
        "id": sorted(W),
        "nearest": sorted(W, key=lambda k: math.dist(W[k]["est"], (bx, by))),
        "confident": sorted(W, key=lambda k: W[k]["sigma"]),
        "spt": sorted(W, key=lambda k: exp_t[k]),
        "oracle": sorted(W, key=lambda k: true_t[k]),
    }
    print(f"{len(W)} workers, {a.teams} teams, t_dig {a.tdig} h, radius {a.radius} m, digging starts at "
          f"{start_h:.2f} h")
    print("policy     alive_expected  wasted_dig_h  last_done_h")
    for name, order in pol.items():
        alive, wasted, done = schedule(order, W, a.teams, a.tdig, a.radius, start_h)
        print(f"{name:10} {alive:14.2f} {wasted:13.1f} {max(d[1] for d in done):12.1f}")


if __name__ == "__main__":
    main()
