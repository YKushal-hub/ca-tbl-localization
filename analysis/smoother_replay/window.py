#!/usr/bin/env python3
"""EXPLORATORY: causal sliding-window smoother replay. For every cycle k (step 'every'), solve the window
[k-W+1, k] from that window's data only and report the estimate AT k (fixed lag 0, no future data).
Initialisation: the previous window's solution for overlapping cycles, dead reckoning for the new ones; the first
window starts at the EKF's own causal estimates. Same factors and synthetic magnetometer as replay.py.
usage: window.py <run dir> <seed> <W cycles> <every>"""
import csv, json, math, sys, collections
import numpy as np
from scipy.optimize import least_squares
d, seed, W, every = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
rng = np.random.default_rng(1000 + seed)
T = list(csv.DictReader(open(f"{d}/trace_ca_tbl_full_seed{seed}.csv")))
I = {round(float(r['t']) / 2.5): r for r in csv.DictReader(open(f"{d}/imu_ca_tbl_full_seed{seed}.csv"))}
K = len(T); tk = np.array([float(r['t']) for r in T]); ks = [round(t / 2.5) for t in tk]; idx = {k: i for i, k in enumerate(ks)}
truth = np.array([[float(r['trueX']), float(r['trueY'])] for r in T]); ekf = np.array([[float(r['estX']), float(r['estY'])] for r in T])
dep = json.load(open(f"{d}/deployment.json")); gw = {a['id']: (a['x'], a['y']) for a in dep['anchors']['0']}
nodes = collections.defaultdict(dict)
for r in csv.DictReader(open(f"{d}/nodes_ca_tbl_full_seed{seed}.csv")):
    nodes[round(float(r['t']) / 2.5)][int(r['nodeId'])] = (float(r['x']), float(r['y']))
finders = sorted({n for k in nodes for n in nodes[k] if n >= 501}); off = {f: rng.normal(0, 2.0, 2) for f in finders}
Rk = collections.defaultdict(list)
for r in csv.DictReader(open(f"{d}/ranges_ca_tbl_full_seed{seed}.csv")):
    if r['inSet'] != '1' or r['src'] not in ('floor', 'active'): continue
    k = round(float(r['t']) / 2.5); a = int(r['anchorId'])
    if k not in idx: continue
    if a in gw: pos, fi = np.array(gw[a]), None
    elif a >= 501 and a in nodes.get(k, {}): pos, fi = np.array(nodes[k][a]) + off[a], finders.index(a)
    else: continue
    Rk[idx[k]].append((pos, fi, float(r['rangeM']), float(r['sigmaM'])))
psi = np.array([float(I[k]['psi']) if k in I else 0.0 for k in ks])
gm = np.zeros(K); a = math.exp(-2.5 / 120.0); sg = math.radians(3)
for i in range(1, K): gm[i] = a * gm[i - 1] + math.sqrt(1 - a * a) * sg * rng.normal()
mag = psi + gm + rng.normal(0, math.radians(3), K); sig_mag = math.radians(math.sqrt(18))
U = np.array([[float(I[k]['ux']), float(I[k]['uy'])] if k in I else [0.0, 0.0] for k in ks]); nf = len(finders)
def solve(lo, hi, x0P, x0rest):
    n = hi - lo + 1
    def res(x):
        P = x[:2 * n].reshape(n, 2); th0, b, s = x[2 * n:2 * n + 3]; C = x[2 * n + 3:].reshape(nf, 2)
        out = []
        for j in range(n):
            for (pos, fi, r, sgm) in Rk.get(lo + j, []):
                q = pos + (C[fi] if fi is not None else 0)
                out.append((math.hypot(*(P[j] - q)) - r) / sgm)
        th = th0 + b * (tk[lo:hi + 1] - tk[lo])
        for j in range(1, n):
            c, sn = math.cos(-th[j]), math.sin(-th[j]); u = U[lo + j]
            step = np.array([c * u[0] - sn * u[1], sn * u[0] + c * u[1]]) / (1 + s)
            so = max(0.15, 0.05 * math.hypot(*u)); out.extend((P[j] - P[j - 1] - step) / so)
        out.extend((th - mag[lo:hi + 1]) / sig_mag)
        out.append(b / math.radians(0.15)); out.append(s / 0.0266); out.extend((C / 2.0).ravel())
        return np.array(out)
    x0 = np.concatenate([x0P.ravel(), x0rest])
    sol = least_squares(res, x0, method='trf', max_nfev=100, x_scale='jac')
    return sol.x[:2 * n].reshape(n, 2), sol.x[2 * n:]
est = {}; prevP = None; prevlo = None; rest = None
for k in range(W - 1, K, every):
    lo = k - W + 1
    if prevP is None:
        P0 = ekf[lo:k + 1].copy(); rest = np.concatenate([[mag[lo], 0.0, 0.0], np.zeros(2 * nf)])
    else:
        P0 = np.zeros((W, 2))
        for j in range(W):
            g = lo + j
            if prevlo <= g < prevlo + len(prevP): P0[j] = prevP[g - prevlo]
            else:
                c, sn = math.cos(-mag[g]), math.sin(-mag[g]); u = U[g]
                P0[j] = P0[j - 1] + [c * u[0] - sn * u[1], sn * u[0] + c * u[1]]
        rest = rest.copy(); rest[0] = mag[lo]
    P, rest = solve(lo, k, P0, rest); est[k] = P[-1]; prevP, prevlo = P, lo
ks2 = sorted(est); e = np.array([math.hypot(*(est[k] - truth[k])) for k in ks2]); ee = np.array([math.hypot(*(ekf[k] - truth[k])) for k in ks2])
print(f"seed {seed} W={W} ({W*2.5:.0f} s), {len(ks2)} estimates from t={tk[ks2[0]]:.0f} s: window median {np.median(e):.1f} p95 {np.percentile(e,95):.1f} | EKF same cycles median {np.median(ee):.1f} p95 {np.percentile(ee,95):.1f}")
