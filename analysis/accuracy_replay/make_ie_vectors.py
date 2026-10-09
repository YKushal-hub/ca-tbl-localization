"""Plan A, A2v: the Python reference (replay.AugEkf) on the stream scratch/ca_tbl_ie_test.cc builds.

AugEkf has no bias random walk; this adds q_b^2 dt on b, as the C++ filter does. Its
prediction Jacobian is a central difference (eps 1e-6), whose error alone moves pxy by ~1e-7
relative over this stream; the prediction is overridden here with the analytic Jacobian
(the same formulas the C++ filter uses, written independently), so the comparison can be
tight. Prints the same eight lines as the C++ test.
"""
import os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from replay import AugEkf


class AugEkfAnalytic(AugEkf):
    def predict(self, dt, ux, uy):
        x, y, psi, b, s = self.z
        c, sn = np.cos(psi), np.sin(psi)
        ax, ay = c * ux + sn * uy, -sn * ux + c * uy
        F = np.eye(5)
        F[0, 2], F[1, 2] = (1 - s) * ay, -(1 - s) * ax
        F[0, 4], F[1, 4] = -ax, -ay
        F[2, 3] = dt
        self.z = np.array([x + (1 - s) * ax, y + (1 - s) * ay, psi + b * dt, b, s])
        Q = np.zeros((5, 5))
        Q[0, 0] = Q[1, 1] = self.sig_w ** 2
        Q[2, 2] = self.arw ** 2 * dt
        self.P = F @ self.P @ F.T + Q

QB = 1e-6
e = AugEkfAnalytic(100.0, 50.0, posStd=10.0, sig_b=4.27e-5, sig_s=0.0266, arw=1.6682e-4, sig_w=0.05)
dt = 2.5
for k in range(200):
    ux = 1.0 * np.cos(0.01 * k) + 0.3
    uy = 0.8 * np.sin(0.013 * k)
    e.predict(dt, ux, uy)
    e.P[3, 3] += QB ** 2 * dt
    if k % 3 == 0:
        R = [(0.0, 0.0, 300.0 + 2.0 * np.sin(0.1 * k), 3.0, False, None, None),
             (500.0, 0.0, 260.0 + 3.0 * np.cos(0.07 * k), 4.0, False, None, None)]
        e.update_ranges(R, infl=1.0)
    if k % 7 == 0:
        e.fuse_pos((100.0 + 0.5 * k, 50.0 - 0.2 * k), 25.0, 3.0, 16.0)
for n, v in zip(["x", "y", "psi", "b", "s", "pxx", "pxy", "pyy"],
                [*e.z, e.P[0, 0], e.P[0, 1], e.P[1, 1]]):
    print(f"{n} {v:.12e}")
