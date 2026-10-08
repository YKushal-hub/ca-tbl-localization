"""Open-loop replay of logged range streams through alternative estimators.

Input: ranges_<arm>_seed<N>.csv written by the exploratory CATBL_RANGELOG patch
  T,t,trueX,trueY,dt,estPreX,estPreY,activeTier,usedFloor,0
  R,t,anchorX,anchorY,rangeM,sigmaM,nlos,sourceKind,fromFloor,anchorId
  A,-1,x,y,rxx,rxy,ryy,0,0,0         (TDoA position fix queued for the NEXT T row)
"""
import sys, os, glob
import numpy as np


def parse(path):
    cyc = []
    aux = None
    cur = None
    for line in open(path):
        p = line.rstrip('\n').split(',')
        k = p[0]
        if k == 'kind':
            continue
        if k == 'A':
            aux = tuple(float(v) for v in p[2:7])
        elif k == 'T':
            cur = dict(t=float(p[1]), tx=float(p[2]), ty=float(p[3]), dt=float(p[4]),
                       ex=float(p[5]), ey=float(p[6]), tier=int(float(p[7])), floor=int(float(p[8])),
                       aux=aux, R=[])
            aux = None
            cyc.append(cur)
        elif k == 'R':
            cur['R'].append((float(p[2]), float(p[3]), float(p[4]), float(p[5]), int(p[6]), int(p[7]), int(p[8])))
    return cyc


class Ekf:
    def __init__(self, x, y, posStd=100.0, velStd=5.0, qpos=1.0, qvel=0.5):
        self.x = np.array([x, y, 0.0, 0.0])
        self.P = np.diag([posStd ** 2, posStd ** 2, velStd ** 2, velStd ** 2])
        self.qp, self.qv = qpos, qvel

    def predict(self, dt):
        F = np.eye(4); F[0, 2] = dt; F[1, 3] = dt
        self.x = F @ self.x
        self.P = F @ self.P @ F.T
        self.P[0, 0] += self.qp ** 2 * dt; self.P[1, 1] += self.qp ** 2 * dt
        self.P[2, 2] += self.qv ** 2 * dt; self.P[3, 3] += self.qv ** 2 * dt

    def update_ranges(self, R, infl=3.0, gate=None):
        for (ax, ay, r, s, nlos, _, _) in R:
            sig = s * (infl if nlos else 1.0)
            dx = self.x[0] - ax; dy = self.x[1] - ay
            rp = max(np.hypot(dx, dy), 1e-6)
            H = np.array([dx / rp, dy / rp, 0, 0])
            PH = self.P @ H
            S = H @ PH + sig ** 2
            if not (S > 0): continue
            if gate is not None and (r - rp) ** 2 / S > gate: continue
            K = PH / S
            self.x = self.x + K * (r - rp)
            self.P = self.P - np.outer(K, H @ self.P)
            self.P = 0.5 * (self.P + self.P.T)

    def fuse_pos(self, z, rxx, rxy, ryy, gate=None):
        S = np.array([[self.P[0, 0] + rxx, 0.5 * (self.P[0, 1] + self.P[1, 0]) + rxy],
                      [0, self.P[1, 1] + ryy]])
        S[1, 0] = S[0, 1]
        det = np.linalg.det(S)
        if not (det > 0): return
        Si = np.linalg.inv(S)
        if gate is not None:
            v0 = np.array(z) - self.x[:2]
            if v0 @ Si @ v0 > gate: return
        K = self.P[:, :2] @ Si
        v = np.array(z) - self.x[:2]
        self.x = self.x + K @ v
        self.P = self.P - K @ self.P[:2, :]
        self.P = 0.5 * (self.P + self.P.T)


def run_ekf(cyc, qpos=1.0, qvel=0.5, min_ranges=3, use_aux=True, infl=3.0):
    c0 = cyc[0]
    e = Ekf(c0['tx'], c0['ty'], qpos=qpos, qvel=qvel)
    est = []
    for c in cyc:
        e.predict(c['dt'])
        if len(c['R']) >= min_ranges:
            e.update_ranges(c['R'], infl)
            if use_aux and c['aux'] is not None:
                a = c['aux']; e.fuse_pos((a[0], a[1]), a[2], a[3], a[4])
        est.append((e.x[0], e.x[1], e.P[0, 0] + e.P[1, 1]))
    return np.array(est)


def run_pf(cyc, n=4000, qpos=1.0, qvel=0.5, min_ranges=1, use_aux=True, seed=0, infl=3.0, init_std=100.0):
    rng = np.random.default_rng(seed)
    c0 = cyc[0]
    X = np.zeros((n, 4))
    X[:, 0] = c0['tx'] + rng.normal(0, init_std, n)
    X[:, 1] = c0['ty'] + rng.normal(0, init_std, n)
    X[:, 2:] = rng.normal(0, 5.0, (n, 2))
    w = np.ones(n) / n
    est = []
    for c in cyc:
        dt = c['dt']
        X[:, 0] += X[:, 2] * dt; X[:, 1] += X[:, 3] * dt
        X[:, 0] += rng.normal(0, qpos * np.sqrt(dt), n); X[:, 1] += rng.normal(0, qpos * np.sqrt(dt), n)
        X[:, 2] += rng.normal(0, qvel * np.sqrt(dt), n); X[:, 3] += rng.normal(0, qvel * np.sqrt(dt), n)
        if len(c['R']) >= min_ranges:
            logw = np.zeros(n)
            for (ax, ay, r, s, nlos, _, _) in c['R']:
                sig = s * (infl if nlos else 1.0)
                d = np.hypot(X[:, 0] - ax, X[:, 1] - ay)
                logw += -0.5 * ((r - d) / sig) ** 2
            if use_aux and c['aux'] is not None and len(c['R']) >= 3:
                a = c['aux']
                R = np.array([[a[2], a[3]], [a[3], a[4]]])
                try:
                    Ri = np.linalg.inv(R)
                    v = X[:, :2] - np.array([a[0], a[1]])
                    logw += -0.5 * np.einsum('ni,ij,nj->n', v, Ri, v)
                except np.linalg.LinAlgError:
                    pass
            logw -= logw.max()
            w = w * np.exp(logw); sw = w.sum()
            if sw <= 0 or not np.isfinite(sw):
                w = np.ones(n) / n
            else:
                w /= sw
            ess = 1.0 / np.sum(w ** 2)
            if ess < 0.5 * n:
                idx = np.searchsorted(np.cumsum(w), (rng.random() + np.arange(n)) / n)
                idx = np.clip(idx, 0, n - 1)
                X = X[idx]; w = np.ones(n) / n
                # roughening to keep diversity (small, scaled by sample spread)
                X[:, 0] += rng.normal(0, 0.5, n); X[:, 1] += rng.normal(0, 0.5, n)
        mx = np.sum(w * X[:, 0]); my = np.sum(w * X[:, 1])
        est.append((mx, my, 0.0))
    return np.array(est)


def err(cyc, est):
    tx = np.array([c['tx'] for c in cyc]); ty = np.array([c['ty'] for c in cyc])
    return np.hypot(est[:, 0] - tx, est[:, 1] - ty)


def summarize(name, e, mask=None):
    if mask is not None: e = e[mask]
    return f'{name:28s} n={len(e):5d} CEP50={np.median(e):8.1f} CEP95={np.quantile(e, .95):8.1f} mean={e.mean():8.1f}'


# ---------------------------------------------------------------- IMU-driven filters
def parse_imu(path):
    import pandas as pd
    d = pd.read_csv(path)
    return d


class EkfImu(Ekf):
    """Exact replica of CaTblEkf::PredictWithControl + Update + FusePositionCov."""
    def predict_ctrl(self, dt, ux, uy, qxx, qxy, qyy):
        self.x[0] += ux; self.x[1] += uy
        inv = 1.0 / dt if dt > 0 else 0.0
        self.x[2] = ux * inv; self.x[3] = uy * inv
        q = [[qxx, qxy], [qxy, qyy]]
        for i in range(2):
            for j in range(2):
                self.P[i, j] += q[i][j]
                self.P[i, j + 2] = q[i][j] * inv
                self.P[i + 2, j] = q[i][j] * inv
                self.P[i + 2, j + 2] = q[i][j] * inv * inv


def run_ekf_imu(cyc, imu, min_ranges=3, use_aux=True, infl=3.0, qscale=1.0, dr_only=False, gate=None, aux_gate=None, pos0=100.0):
    c0 = cyc[0]
    e = EkfImu(c0['tx'], c0['ty'], posStd=pos0)
    est = []
    for k, c in enumerate(cyc):
        r = imu.iloc[k]
        e.predict_ctrl(c['dt'], r.ux, r.uy, r.quxx * qscale, r.quxy * qscale, r.quyy * qscale)
        if (not dr_only) and len(c['R']) >= min_ranges:
            e.update_ranges(c['R'], infl, gate)
            if use_aux and c['aux'] is not None:
                a = c['aux']; e.fuse_pos((a[0], a[1]), a[2], a[3], a[4], aux_gate)
        est.append((e.x[0], e.x[1], e.P[0, 0] + e.P[1, 1]))
    return np.array(est)


class AugEkf:
    """Error-state EKF with persistent heading bias b and scale s as states.
    z = [x, y, psi, b, s]; DR input u_k = (1+s) R(psi_k) Delta_k  =>  Delta_k ~ (1-s) R(-psi_k) u_k
    """
    def __init__(self, x, y, posStd=1.0, sig_b=4.27e-5, sig_s=0.0266, arw=1.6682e-4, sig_w=0.05):
        self.z = np.array([x, y, 0.0, 0.0, 0.0])
        self.P = np.diag([posStd ** 2, posStd ** 2, 1e-8, sig_b ** 2, sig_s ** 2])
        self.arw, self.sig_w = arw, sig_w

    def predict(self, dt, ux, uy):
        x, y, psi, b, s = self.z
        c, sn = np.cos(-psi), np.sin(-psi)
        dxn = (1 - s) * (c * ux - sn * uy)
        dyn = (1 - s) * (sn * ux + c * uy)
        # jacobian
        F = np.eye(5)
        dc, ds = -np.sin(-psi) * -1, np.cos(-psi) * -1   # d/dpsi of cos(-psi)= sin(psi)... handled numerically below
        eps = 1e-6
        def f(z):
            c2, s2 = np.cos(-z[2]), np.sin(-z[2])
            return np.array([z[0] + (1 - z[4]) * (c2 * ux - s2 * uy), z[1] + (1 - z[4]) * (s2 * ux + c2 * uy), z[2] + z[3] * dt, z[3], z[4]])
        for j in range(5):
            dz = np.zeros(5); dz[j] = eps
            F[:, j] = (f(self.z + dz) - f(self.z - dz)) / (2 * eps)
        self.z = f(self.z)
        Q = np.zeros((5, 5))
        Q[0, 0] = Q[1, 1] = (self.sig_w) ** 2
        Q[2, 2] = self.arw ** 2 * dt
        self.P = F @ self.P @ F.T + Q

    def update_ranges(self, R, infl=3.0, gate=None):
        for (ax, ay, r, s_, nlos, _, _) in R:
            sig = s_ * (infl if nlos else 1.0)
            dx = self.z[0] - ax; dy = self.z[1] - ay
            rp = max(np.hypot(dx, dy), 1e-6)
            H = np.array([dx / rp, dy / rp, 0, 0, 0])
            PH = self.P @ H
            S = H @ PH + sig ** 2
            if gate is not None and (r - rp) ** 2 / S > gate: continue
            K = PH / S
            self.z = self.z + K * (r - rp)
            self.P = self.P - np.outer(K, H @ self.P)
            self.P = 0.5 * (self.P + self.P.T)

    def fuse_pos(self, zz, rxx, rxy, ryy, gate=None):
        H = np.zeros((2, 5)); H[0, 0] = 1; H[1, 1] = 1
        Rm = np.array([[rxx, rxy], [rxy, ryy]])
        S = H @ self.P @ H.T + Rm
        if not (np.linalg.det(S) > 0): return
        if gate is not None:
            v0 = np.array(zz) - self.z[:2]
            if v0 @ np.linalg.inv(S) @ v0 > gate: return
        K = self.P @ H.T @ np.linalg.inv(S)
        self.z = self.z + K @ (np.array(zz) - H @ self.z)
        self.P = self.P - K @ H @ self.P
        self.P = 0.5 * (self.P + self.P.T)


def run_aug(cyc, imu, min_ranges=3, use_aux=True, infl=3.0, gate=None, aux_gate=None, **kw):
    c0 = cyc[0]
    e = AugEkf(c0['tx'], c0['ty'], **kw)
    est = []
    for k, c in enumerate(cyc):
        r = imu.iloc[k]
        e.predict(c['dt'], r.ux, r.uy)
        if len(c['R']) >= min_ranges:
            e.update_ranges(c['R'], infl, gate)
            if use_aux and c['aux'] is not None:
                a = c['aux']; e.fuse_pos((a[0], a[1]), a[2], a[3], a[4], aux_gate)
        est.append((e.z[0], e.z[1], e.P[0, 0] + e.P[1, 1]))
    return np.array(est), e
