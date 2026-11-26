#!/usr/bin/env python3
"""G182 (plan "System gates use synthetic truth"): the B1 joint estimator and the B4 cooperative solver on
noise-free synthetic links built from the system's own model, and refusal fixtures.

  b1_exact      one worker, 16 rescuer points at two heights, noise-free RSS on 3 bands and ToF on 2.4 GHz with
                the rubble delay C/rho: joint recovers the position within 0.05 m and C_2400 within 0.5 dB.
  b1_bias       the same with ToF only and no excess model (tof): biased by >= 0.5 m (the bias B1 removes).
  b4_exact      four workers anchored by rescuers plus one worker heard only through those four (worker-to-worker
                868 links, C_ij = C_i + C_j): coop places the fifth within 0.10 m.
  b4_coplanar   the fifth worker hears only 2 neighbours: coop must report it unresolved (NaN).
  b4_disconn    the fifth worker hears nobody: unresolved (NaN).
Exit 0 only if every check passes.
"""
import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import estimate_collapse as E  # noqa: E402
import system_priors as P  # noqa: E402

LAW = {"868": (30.0, 2.0, 1.0, 100), "2400": (40.0, 2.0, 1.0, 100), "uwb": (50.0, 2.0, 1.0, 100)}
C_TRUE = {"868": 24.0, "2400": 78.0, "uwb": 180.0}
RHO_TRUE = {"2400": 0.5 * (55.33 + 65.21), "uwb": 0.5 * (130.15 + 142.40)}  # the synthetic World's own rho


def pwr(b, d, c):
    L0, n, _, _ = LAW[b]
    return P.TX_DBM[b] - (L0 + 10 * n * math.log10(d)) - c


def anchors():
    A = {}
    k = 10000
    for x in (-6.0, -2.0, 2.0, 6.0):
        for y in (-6.0, 6.0):
            for h in (0.3, 1.5):
                A[k] = (np.array([x, y, 4.0 + h]), np.array([0.01] * 3), "point")
                k += 1
    return A


def meas_for(xw, A, c, bands=("868", "2400"), tof=True):
    m = []
    for pid, (pa, _, _) in A.items():
        d = float(np.linalg.norm(xw - pa))
        for b in bands:
            r = d + c[b] / RHO_TRUE[b] if (tof and b in E.TOF_BANDS) else None
            m.append((b, pid, pwr(b, d, c[b]), r))
    return m


def main():
    fails = []
    fault = os.environ.get("G182_FAULT", "")
    if fault == "rho":  # selftest: the system's rho prior off by 20% -> b1 must fail
        orig = P.rho_mid
        P.rho_mid = lambda b: 1.2 * orig(b)
    if fault == "norank":  # selftest: rigidity check removed -> refusals must fail
        orig_coop = E.solve_coop

        def no_rank(*a, **k):
            X = orig_coop(*a, **k)
            return {w: (np.zeros(3) if np.any(np.isnan(v)) else v) for w, v in X.items()}
        E.solve_coop = no_rank
    A = anchors()
    xw = np.array([1.0, 0.5, 2.0])
    m = meas_for(xw, A, C_TRUE)
    x, _, _, _ = E.solve(m, LAW, A, "joint", 4.0)
    err = float(np.linalg.norm(x - xw))
    if err > 0.05:
        fails.append(f"b1_exact: joint error {err:.3f} m > 0.05")
    mt = [(b, a, None, r) for (b, a, p, r) in m]
    xt, _, _, _ = E.solve(mt, LAW, A, "tof", 4.0)
    if float(np.linalg.norm(xt - xw)) < 0.5:
        fails.append("b1_bias: ToF alone is not biased by the rubble delay; the fixture tests nothing")
    # B4: four anchored workers, the fifth only via them
    W = {1: np.array([-4.0, -4.0, 2.0]), 2: np.array([4.0, -4.0, 1.0]), 3: np.array([4.0, 4.0, 2.5]),
         4: np.array([-4.0, 4.0, 1.5]), 5: np.array([0.5, 0.5, 2.0])}
    C = {w: {"868": 15.0 + 3 * w, "2400": 0.0, "uwb": 0.0} for w in W}
    meas = {w: (meas_for(W[w], A, C[w], bands=("868", "2400")) if w != 5 else []) for w in W}
    for w in (1, 2, 3, 4):  # the anchored workers' 2.4 GHz excess, so their ToF is consistent
        C[w]["2400"] = 0.0

    def vv_links(nbrs):
        out = []
        for j in nbrs:
            d = float(np.linalg.norm(W[5] - W[j]))
            out.append((5, j, "868", pwr("868", d, C[5]["868"] + C[j]["868"])))
        return out

    init = {w: (W[w] + np.array([0.3, -0.2, 0.1]), np.array([C[w]["868"], 0.0, 0.0])) for w in W}
    init[5] = (np.array([0.0, 0.0, 2.5]), np.array([20.0, 0.0, 0.0]))
    X = E.solve_coop(meas, vv_links((1, 2, 3, 4)), LAW, A, 4.0, init)
    e5 = float(np.linalg.norm(X[5] - W[5])) if not np.any(np.isnan(X[5])) else float("inf")
    if e5 > 0.10:
        fails.append(f"b4_exact: worker 5 via its neighbours, error {e5:.3f} m > 0.10")
    X = E.solve_coop(meas, vv_links((1, 2)), LAW, A, 4.0, init)
    if not np.any(np.isnan(X[5])):
        fails.append("b4_coplanar: worker 5 with 2 neighbours reported as resolved")
    X = E.solve_coop(meas, [], LAW, A, 4.0, init)
    if not np.any(np.isnan(X[5])):
        fails.append("b4_disconn: a worker nobody hears reported as resolved")
    if fails:
        print("G182 FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print(f"G182 PASS (b1 joint error {err:.4f} m; b4 via-neighbours error {e5:.4f} m; refusals hold)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
