#!/usr/bin/env python3
"""GEO-01: free placement of N gateways by local search (docs/SPEC_CONFORMANCE_PLAN.md MS-2
step 2). Analysis only. Uses geo_screen.py's coverage model and writes x,y CSVs that
--gwLayout=file reads, so the screen then asks the build where they went like any shape.

Swap local search from several starts (greedy, evenly spaced, 8 seeded random subsets), on
(mean P(>=3), mean P(>=2), mean P(>=1)) lexicographically over a coarse lattice -- the
registered screening criterion. A local optimum, so a lower bound on the best placement,
not the optimum (stated).

Candidate positions:
  realistic     every 250 m along the square of half-width 0.9 x boxHalf -- the
                vehicle-reachable band, the constraint behind the owner's rejection of the
                interior grid (HANDOFF_2026_09_09 sect 4);
  unrestricted  a 500 m grid over the whole box -- an upper bound, flagged unrealistic.

usage: geo_optimise.py --probe <probe_range_sweep.csv> --out <dir> [--boxHalf 5000]
                       [--N 3,4,5,6,8] [--lattice 200]
"""
import argparse
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import geo_screen as gs  # noqa: E402


def perimeter_band(box, frac=0.9, step=250.0):
    h = frac * box
    pts = []
    s = 0.0
    while s < 8 * h - 1e-6:
        if s < 2 * h:
            pts.append((-h + s, -h))
        elif s < 4 * h:
            pts.append((h, -h + (s - 2 * h)))
        elif s < 6 * h:
            pts.append((h - (s - 4 * h), h))
        else:
            pts.append((-h, h - (s - 6 * h)))
        s += step
    return pts


def grid(box, step=500.0):
    xs = np.arange(-box + step / 2, box, step)
    return [(float(x), float(y)) for x in xs for y in xs]


def objective(pts, pdr, sigma, box, lattice):
    m = gs.screen(pts, pdr, sigma, box, lattice)
    return (m["mean_p3"], m["mean_p2"], m["mean_p1"])


def local_search(chosen, cands, pdr, sigma, box, lattice, passes=20):
    """Move one gateway to any free candidate while that improves the objective, until no
    move does (or `passes` sweeps)."""
    cur = objective(chosen, pdr, sigma, box, lattice)
    n = len(chosen)
    for _ in range(passes):
        improved = False
        for i in range(n):
            for c in cands:
                if c in chosen:
                    continue
                trial = chosen[:i] + [c] + chosen[i + 1:]
                k = objective(trial, pdr, sigma, box, lattice)
                if k > cur:
                    chosen, cur, improved = trial, k, True
        if not improved:
            break
    return chosen, cur


def optimise(cands, n, pdr, sigma, box, lattice, restarts=8, seed=2026):
    """Local search from several starts, best kept: the greedy start (myopic -- P(>=3) is 0
    until the third gateway, so the first two optimise P(>=1) and P(>=2) and land side by
    side), n candidates evenly spaced through the list, and `restarts` random subsets from
    a fixed seed (reproducible). Single swaps cannot pull a tight cluster apart, which is
    why one start is not enough."""
    starts = []
    chosen = []
    for _ in range(n):
        best, best_key = None, None
        for c in cands:
            if c in chosen:
                continue
            k = objective(chosen + [c], pdr, sigma, box, lattice)
            if best_key is None or k > best_key:
                best, best_key = c, k
        chosen.append(best)
    starts.append(chosen)
    starts.append([cands[(i * len(cands)) // n] for i in range(n)])
    rng = np.random.default_rng(seed)
    for _ in range(restarts):
        idx = rng.choice(len(cands), size=n, replace=False)
        starts.append([cands[i] for i in sorted(idx)])
    best, best_key = None, None
    for st in starts:
        pts, k = local_search(list(st), cands, pdr, sigma, box, lattice)
        if best_key is None or k > best_key:
            best, best_key = pts, k
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--probe", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--boxHalf", type=float, default=5000.0)
    ap.add_argument("--N", default="3,4,5,6,8")
    ap.add_argument("--lattice", type=float, default=200.0)
    a = ap.parse_args()
    pdr, sigma = gs.probe_model(a.probe)
    os.makedirs(a.out, exist_ok=True)
    sets = {"realistic": perimeter_band(a.boxHalf), "unrestricted": grid(a.boxHalf)}
    for n in [int(x) for x in a.N.split(",")]:
        for name, cands in sets.items():
            pts = optimise(cands, n, pdr, sigma, a.boxHalf, a.lattice)
            path = os.path.join(a.out, f"geo_opt_{name}_N{n}.csv")
            with open(path, "w") as f:
                f.write("x,y\n")
                for x, y in pts:
                    f.write(f"{x:.1f},{y:.1f}\n")
            print(f"  {name:12s} N={n}: {pts}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
