#!/usr/bin/env python3
"""EXPLORATORY (not citable): does a trajectory-level solve recover tag 0 where the 5-state EKF locks wrong?
Full-batch least squares over every cycle of one run, from the run's own logs: ranges (with stated sigma),
IMU displacement u_k, the true heading error psi_k (to synthesise the magnetometer: white 3 deg + Gauss-Markov
3 deg / 120 s, the run's model), finder positions (true + a constant N(0, 2^2) offset, the run's model).
Unknowns: p_k (2 per cycle), theta0, b, s, one offset per finder. Initialised at the EKF's trajectory.
usage: replay.py <run dir> <seed> [window_s]"""
import csv, json, math, sys, collections
import numpy as np
from scipy.optimize import least_squares
from scipy.sparse import lil_matrix
d, seed = sys.argv[1], int(sys.argv[2])
rng = np.random.default_rng(1000 + seed)
T = list(csv.DictReader(open(f"{d}/trace_ca_tbl_full_seed{seed}.csv")))
I = {round(float(r['t']) / 2.5): r for r in csv.DictReader(open(f"{d}/imu_ca_tbl_full_seed{seed}.csv"))}
K = len(T)
tk = np.array([float(r['t']) for r in T]); ks = [round(t / 2.5) for t in tk]
idx = {k: i for i, k in enumerate(ks)}
truth = np.array([[float(r['trueX']), float(r['trueY'])] for r in T])
ekf = np.array([[float(r['estX']), float(r['estY'])] for r in T])
dep = json.load(open(f"{d}/deployment.json"))
gw = {a['id']: (a['x'], a['y']) for a in dep['anchors']['0']}
nodes = collections.defaultdict(dict)
for r in csv.DictReader(open(f"{d}/nodes_ca_tbl_full_seed{seed}.csv")):
    nodes[round(float(r['t']) / 2.5)][int(r['nodeId'])] = (float(r['x']), float(r['y']))
finders = sorted({n for k in nodes for n in nodes[k] if n >= 501})
off = {f: rng.normal(0, 2.0, 2) for f in finders}
R = []
for r in csv.DictReader(open(f"{d}/ranges_ca_tbl_full_seed{seed}.csv")):
    if r['inSet'] != '1' or r['src'] not in ('floor', 'active'):
        continue
    k = round(float(r['t']) / 2.5); a = int(r['anchorId'])
    if k not in idx:
        continue
    if a in gw:
        pos, fi = gw[a], None
    elif a >= 501 and a in nodes.get(k, {}):
        pos, fi = nodes[k][a], finders.index(a)
    else:
        continue
    R.append((idx[k], np.array(pos), fi, float(r['rangeM']), float(r['sigmaM'])))
# magnetometer
psi = np.array([float(I[k]['psi']) if k in I else 0.0 for k in ks])
gm = np.zeros(K); a = math.exp(-2.5 / 120.0); sg = math.radians(3)
for i in range(1, K):
    gm[i] = a * gm[i - 1] + math.sqrt(1 - a * a) * sg * rng.normal()
mag = psi + gm + rng.normal(0, math.radians(3), K)
sig_mag = math.radians(math.sqrt(3**2 + 3**2))
U = np.array([[float(I[k]['ux']), float(I[k]['uy'])] if k in I else [0.0, 0.0] for k in ks])
nf = len(finders); NP = 2 * K
def unpack(x):
    return x[:NP].reshape(K, 2), x[NP], x[NP + 1], x[NP + 2], x[NP + 3:].reshape(nf, 2)
def res(x):
    P, th0, b, s, C = unpack(x)
    out = []
    for (i, pos, fi, r, sgm) in R:
        q = pos + (C[fi] if fi is not None else 0)
        out.append((np.hypot(*(P[i] - q)) - r) / sgm)
    th = th0 + b * tk
    for i in range(1, K):
        c, sn = math.cos(-th[i]), math.sin(-th[i])
        u = U[i]; step = np.array([c * u[0] - sn * u[1], sn * u[0] + c * u[1]]) / (1 + s)
        so = max(0.15, 0.05 * np.hypot(*u))
        out.extend((P[i] - P[i - 1] - step) / so)
    out.extend((th - mag) / sig_mag)
    out.append(b / math.radians(0.15)); out.append(s / 0.0266)
    out.extend((C / 2.0).ravel())
    return np.array(out)
x0 = np.concatenate([ekf.ravel(), [mag[0], 0.0, 0.0], np.zeros(2 * nf)])
sol = least_squares(res, x0, method='trf', loss='linear', max_nfev=200, x_scale='jac')
P, th0, b, s, C = unpack(sol.x)
e_s = np.hypot(*(P - truth).T); e_e = np.hypot(*(ekf - truth).T)
true_b = (psi[-1] - psi[0]) / (tk[-1] - tk[0])
print(f"seed {seed}: ranges {len(R)}, cycles {K}, finders {finders}; solver {sol.status} cost {sol.cost:.1f}")
print(f"  smoother err: median {np.median(e_s):.1f} m, p95 {np.percentile(e_s,95):.1f}, last {e_s[-1]:.1f}")
print(f"  EKF      err: median {np.median(e_e):.1f} m, p95 {np.percentile(e_e,95):.1f}, last {e_e[-1]:.1f}")
print(f"  smoother b {b:.5f} (true slope {true_b:.5f}), s {s:+.4f}, theta0 {th0:.3f} (psi0 {psi[0]:.3f})")
