"""
scratch/python_handoff/ekf_tracker.py

Phase S3, Stage 1. Range-only 2D EKF for CA-TBL. Built and validated against
SYNTHETIC ground-truth data first, deliberately -- this isolates "is the EKF
math correct" from "is the physics-engine trajectory data realistic," so a
problem later against real multi-tier CSVs can be diagnosed as one or the
other, not both at once. The unified 3-anchor C++ generator is the right
next step, but only after this layer is independently proven.

State: [x, y, vx, vy]  (constant-velocity model, per the plan)
Measurement: [r_1 ... r_n]  -- ranges to n anchors, n varies call to call
(built from whichever anchors reported a fix this cycle -- H and R are
constructed fresh per update, not fixed-size).

This is a genuine Extended KF: h(x) = ||pos - anchor_i|| is nonlinear in
[x, y], so the Jacobian H = dh/dx is computed and re-linearized every update,
not assumed constant. Implemented directly with numpy rather than filterpy so
every step is auditable -- the whole project's approach has been "check the
actual math," and this is the most math-sensitive layer built so far, easy to
get subtly wrong (wrong Jacobian sign/shape is the single most common EKF bug).
"""

import numpy as np


class RangeEKF:
    def __init__(self, initial_state, initial_cov, process_noise_std_pos=0.5, process_noise_std_vel=0.5):
        self.x = np.array(initial_state, dtype=float).reshape(4, 1)
        self.P = np.array(initial_cov, dtype=float)
        self.q_pos = process_noise_std_pos
        self.q_vel = process_noise_std_vel

    def predict(self, dt):
        F = np.array([
            [1, 0, dt, 0],
            [0, 1, 0, dt],
            [0, 0, 1, 0],
            [0, 0, 0, 1],
        ])
        # Diagonal process noise, position/velocity terms scaled separately.
        # Not the "proper" Van Loan discretization (which couples position
        # and velocity noise off-diagonal) -- adequate for this validation,
        # flagged here so it's not mistaken for more rigorous than it is.
        Q = np.diag([self.q_pos**2 * dt, self.q_pos**2 * dt, self.q_vel**2 * dt, self.q_vel**2 * dt])
        self.x = F @ self.x
        self.P = F @ self.P @ F.T + Q
        return self.x.copy(), self.P.copy()

    def update(self, ranges, anchors, r_diag):
        """
        ranges:  list of measured ranges, len n
        anchors: list of (ax, ay), len n
        r_diag:  list of measurement VARIANCES, len n -- this is the hook for
                 NLOS-conditioned inflation: pass a larger variance for any
                 anchor whose reading is NLOS-tagged this cycle, per the
                 stated plan ("dynamically scaled using HiddenChannelState/NlosDb").
        """
        x, y = self.x[0, 0], self.x[1, 0]
        n = len(ranges)
        h = np.zeros((n, 1))
        H = np.zeros((n, 4))
        for i, (ax, ay) in enumerate(anchors):
            dx, dy = x - ax, y - ay
            r_pred = np.sqrt(dx**2 + dy**2)
            h[i, 0] = r_pred
            r_safe = max(r_pred, 1e-6)  # avoid divide-by-zero if the estimate lands on an anchor
            H[i, 0] = dx / r_safe
            H[i, 1] = dy / r_safe
            # H[i, 2], H[i, 3] = 0, 0 -- range has no direct velocity dependence

        z = np.array(ranges, dtype=float).reshape(n, 1)
        innovation = z - h
        R = np.diag(r_diag)
        S = H @ self.P @ H.T + R          # innovation covariance -- PCS/GDOP inputs live here
        K = self.P @ H.T @ np.linalg.inv(S)
        self.x = self.x + K @ innovation
        self.P = (np.eye(4) - K @ H) @ self.P
        return self.x.copy(), self.P.copy(), S


def raw_multilateration(ranges, anchors, x0):
    """No-filter baseline: independent per-step nonlinear least-squares solve,
    zero temporal fusion. This is what a single raw fix looks like before any
    EKF smoothing -- the comparison point that makes the EKF's value visible."""
    from scipy.optimize import least_squares

    def residuals(p):
        return [np.hypot(p[0] - ax, p[1] - ay) - r for (ax, ay), r in zip(anchors, ranges)]

    res = least_squares(residuals, x0)
    return res.x
