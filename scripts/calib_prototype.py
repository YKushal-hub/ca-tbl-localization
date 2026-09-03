#!/usr/bin/env python3
"""B2 prototype: weighted least-squares anchor calibration, pure Python, no deps.

Built OUTSIDE the simulator so the degeneracy claim is a demonstrated rank result
rather than an algebraic DOF count. Nothing here ships; the C++ solve is written
against what this establishes.

Gauge: partial survey. Surveyed anchors are FIXED (not free parameters), which is
what makes the covariance absolute rather than relative to an arbitrary frame.
"""
import math, itertools, random

def solve_spd(A, b):
    n = len(A)
    M = [row[:] + [1.0 if i == j else 0.0 for j in range(n)] + [b[i]] for i, row in enumerate(A)]
    rank = 0
    pivots = []
    for c in range(n):
        p = max(range(c, n), key=lambda r: abs(M[r][c]))
        if abs(M[p][c]) < 1e-12:
            continue
        M[c], M[p] = M[p], M[c]
        pv = M[c][c]
        pivots.append(abs(pv))
        M[c] = [v / pv for v in M[c]]
        for r in range(n):
            if r != c and abs(M[r][c]) > 0.0:
                f = M[r][c]
                M[r] = [vr - f * vc for vr, vc in zip(M[r], M[c])]
        rank += 1
    x = [M[i][2 * n] for i in range(n)]
    inv = [M[i][n:2 * n] for i in range(n)]
    return x, inv, rank, (min(pivots) if pivots else 0.0)

def _informative(ranges, surveyed):
    """Ranges with at least one free endpoint. A surveyed-surveyed range contributes
    no row to J and cannot constrain anything, so counting it inflates redundancy --
    which is exactly how a zero-redundancy solve first came to report +1."""
    return sum(1 for (i, j, _) in ranges if i not in surveyed or j not in surveyed)

def calibrate(true_pos, surveyed, ranges, sigma, init=None, iters=40):
    free = [i for i in range(len(true_pos)) if i not in surveyed]
    idx = {a: k for k, a in enumerate(free)}
    n = 2 * len(free)
    rank = 0; inv = []; minpiv = 0.0
    p = {i: list(true_pos[i]) for i in range(len(true_pos))}
    if init:
        for i, v in init.items():
            p[i] = list(v)
    w = 1.0 / (sigma * sigma)
    for _ in range(iters):
        JtWJ = [[0.0] * n for _ in range(n)]
        JtWr = [0.0] * n
        for (i, j, dm) in ranges:
            dx = p[i][0] - p[j][0]; dy = p[i][1] - p[j][1]
            dd = math.hypot(dx, dy)
            if dd < 1e-9:
                continue
            r = dd - dm
            row = [0.0] * n
            if i in idx:
                row[2 * idx[i]] += dx / dd; row[2 * idx[i] + 1] += dy / dd
            if j in idx:
                row[2 * idx[j]] -= dx / dd; row[2 * idx[j] + 1] -= dy / dd
            for a in range(n):
                if row[a] == 0.0: continue
                JtWr[a] += w * row[a] * r
                for b in range(n):
                    if row[b] != 0.0:
                        JtWJ[a][b] += w * row[a] * row[b]
        if n == 0:
            break
        step, inv, rank, minpiv = solve_spd(JtWJ, [-v for v in JtWr])
        if rank < n:
            return dict(rank=rank, n=n, singular=True, redundancy=_informative(ranges, surveyed) - n,
                        cov=None, est={i: tuple(p[i]) for i in free}, resid=None, minpiv=minpiv)
        for a in free:
            p[a][0] += step[2 * idx[a]]; p[a][1] += step[2 * idx[a] + 1]
        if max(abs(v) for v in step) < 1e-12:
            break
    resid = []
    for (i, j, dm) in ranges:
        resid.append(math.dist(p[i][:2], p[j][:2]) - dm)
    rn = math.sqrt(sum(r * r for r in resid))
    return dict(rank=rank, n=n, singular=False,
                redundancy=_informative(ranges, surveyed) - n,
                cov=[[inv[a][b] for b in range(n)] for a in range(n)],
                est={i: tuple(p[i]) for i in free}, resid=rn, minpiv=minpiv,
                free=free, idx=idx)

def ring(n, r, ph=0.3):
    return [(r * math.cos(2 * math.pi * k / n + ph), r * math.sin(2 * math.pi * k / n + ph))
            for k in range(n)]

def all_ranges(pos, noise=0.0, rng=None):
    out = []
    for i, j in itertools.combinations(range(len(pos)), 2):
        d = math.dist(pos[i], pos[j])
        if noise and rng: d += rng.gauss(0.0, noise)
        out.append((i, j, d))
    return out


def _demo():
    """Reproduces the numbers cited in experiments/B2_GAUGE.md sections 2 and 2b."""
    rng = random.Random(7)
    print("=== identity + partial-survey recovery (noiseless, displaced start)")
    for name, n, r, s in (("tier 2", 3, 150.0, 2), ("tier 3", 5, 10.0, 2),
                          ("tier 3", 5, 10.0, 3), ("tier 3", 5, 10.0, 1)):
        pos = ring(n, r)
        surv = set(range(s))
        init = {i: (pos[i][0] + rng.gauss(0, r * 0.05), pos[i][1] + rng.gauss(0, r * 0.05))
                for i in range(len(pos)) if i not in surv}
        res = calibrate(pos, surv, all_ranges(pos), sigma=0.5, init=init)
        if res["singular"]:
            print("  %-7s n=%d surveyed=%d  SINGULAR (rank %d of %d)"
                  % (name, n, s, res["rank"], res["n"]))
            continue
        err = max(math.dist(res["est"][i], pos[i]) for i in res["est"])
        print("  %-7s n=%d surveyed=%d  max coord error %.2e m  residual %.2e  redundancy %+d"
              % (name, n, s, err, res["resid"], res["redundancy"]))

    print("\n=== a 5 m fault injected into one range -- can the residual see it?")
    for name, n, r, s in (("tier 2", 3, 150.0, 2), ("tier 3", 5, 10.0, 2), ("tier 3", 5, 10.0, 3)):
        pos = ring(n, r)
        surv = set(range(s))
        rs = all_ranges(pos)
        for k, (i, j, d) in enumerate(rs):
            if i not in surv or j not in surv:
                rs[k] = (i, j, d + 5.0)
                break
        res = calibrate(pos, surv, rs, sigma=0.5)
        err = max(math.dist(res["est"][i], pos[i]) for i in res["est"])
        print("  %-7s n=%d surveyed=%d  redundancy %+d | residual after fault %.3e m | "
              "position error %.3f m" % (name, n, s, res["redundancy"], res["resid"], err))


if __name__ == "__main__":
    _demo()
