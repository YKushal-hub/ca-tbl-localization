#!/usr/bin/env python3
"""Cramer-Rao bound for a buried worker's 3-D position from time-of-flight ranges (plan B2, DD 41).

Rescuers range from surface points on a grid around the worker, holding the device at one or several heights.
Ranges are unbiased after the B1 correction, with per-range sigma s (bracketed: 2.4 GHz LOS floor 3.0 m and
the measured 1.27 m best case; UWB 0.10 m; rows in system_priors.TOF_SIGMA_M and VAL-4). The worker lies d
below the surface. The Fisher information for position x from ranges r_i = |x - a_i| + n_i is
J = sum_i u_i u_i^T / s^2, with u_i the unit vector from anchor i to x; the bound is CRB = J^-1. The depth
bound is sqrt(CRB[2,2]), horizontal sqrt(CRB[0,0] + CRB[1,1]).

Output: a table of the depth and horizontal bound against hold-height spread, grid radius, number of contacts
and sigma. It tells which operational advice ("hold the device high and low") matters, before any estimator.
"""
import itertools
import math

import numpy as np


def crb(worker, anchors, sigma):
    J = np.zeros((3, 3))
    for a in anchors:
        u = worker - a
        n = np.linalg.norm(u)
        if n < 1e-6:
            continue
        u = u / n
        J += np.outer(u, u) / sigma ** 2
    try:
        C = np.linalg.inv(J)
    except np.linalg.LinAlgError:
        return float("inf"), float("inf")
    return math.sqrt(max(C[2, 2], 0.0)), math.sqrt(max(C[0, 0] + C[1, 1], 0.0))


def grid(radius, step, holds, surface_z):
    pts = []
    k = int(radius // step)
    for i, j in itertools.product(range(-k, k + 1), repeat=2):
        x, y = i * step, j * step
        if x * x + y * y <= radius * radius:
            for h in holds:
                pts.append(np.array([x, y, surface_z + h]))
    return pts


def main():
    surface_z = 3.658  # 8-storey pile (HAZUS-STOREYS, PILE-ALPHA)
    print("depth_m sigma_m holds         grid_r_m contacts  CRB_depth_m  CRB_horiz_m")
    for depth in (1.0, 2.0, 3.5):
        worker = np.array([0.0, 0.0, surface_z - depth])
        for sigma in (3.0, 1.27, 0.10):
            for holds in ((0.3,), (1.5,), (0.3, 1.5), (0.3, 2.0)):
                for radius in (2.5, 5.0, 10.0):
                    a = grid(radius, 2.5, holds, surface_z)
                    dv, hv = crb(worker, a, sigma)
                    print(f"{depth:7.1f} {sigma:7.2f} {str(holds):13} {radius:8.1f} {len(a):8d} {dv:12.3f} {hv:12.3f}")


if __name__ == "__main__":
    main()
