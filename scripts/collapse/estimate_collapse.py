#!/usr/bin/env python3
"""System-side estimator for the collapse study (DD 38). Reads ONLY obs_*.csv and run.txt's
operational plan (hold heights); never the World's truth files (gate G178 leak check).

Rescuers sweep the surface spots in a serpentine order, split among --rescuers, one spot per slot,
measuring at every hold height there. After each slot every worker's position is re-estimated from
everything heard so far. Three estimators:

  tof    time-of-flight multilateration (2.4 GHz and UWB ranges), no rubble correction
  rss    received power at every band with an unknown per-band rubble excess C_b per worker
         (calibration-free RSS localisation; the air law is fitted from the system's own
         point-to-station links)
  joint  B1: rss + tof together, the ToF bias tied to the same excess through the rubble's
         loss-to-delay ratio rho_b (system_priors.RHO_RANGE): r = |x - a| + C_b / rho_b
  coop   B4: every worker at once, joint's residuals plus the worker-to-worker 868 MHz links. A
         worker-to-worker path leaves the pile above one worker and re-enters above the other (DD 37),
         so its rubble excess is the two workers' own excesses: P_ij = Ptx - L(|x_i - x_j|) - C_i - C_j.
         No new unknowns; workers that few rescuers heard are pulled into place by their neighbours.

Positions live in the rescuers' local frame (--frame local) or the GPS frame (--frame gps).
Output: estimates.csv (method, slot, worker, x, y, z, sd_h, sd_v, n_meas).
"""
import argparse
import csv
import math
import os
import sys

import numpy as np
from scipy.optimize import least_squares

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import system_priors as P  # noqa: E402

BANDS = ("868", "2400", "uwb")
TOF_BANDS = ("2400", "uwb")


def load_anchors(d, frame):
    A = {}
    for r in csv.DictReader(open(os.path.join(d, "obs_anchors.csv"))):
        if frame == "local" and False:
            pass
        if frame == "local":
            p = (float(r["loc_x"]), float(r["loc_y"]), float(r["loc_z"]))
            s = (float(r["sigma_loc"]),) * 3
        else:  # gps, and uwb (B3 starts from the GPS fixes and replaces the points' positions)
            p = (float(r["gps_x"]), float(r["gps_y"]), float(r["gps_z"]))
            s = (float(r["sigma_h"]), float(r["sigma_h"]), float(r["sigma_v"]))
        A[int(r["id"])] = (np.array(p), np.array(s), r["kind"])
    return A


def solve_rescuer_frame(d, A, links):
    """B3 (DD 44): the rescuer frame from the rescuers' own measurements, solved jointly.
    Unknowns: every rescuer point's position. Residuals:
      point-to-point ToF ranges (UWB sigma 0.10 m; 2.4 GHz 3.0 m), median over slots;
      point-to-station ToF ranges (same), the stations at their GPS fixes;
      each point's own GPS fix as a weak prior (sigma_h, sigma_v from the fix).
    The relative geometry comes from the ranges; GPS only places the whole frame (averaged over every point).
    Returns a copy of A with the solved points and the number of points solved."""
    pts = sorted(i for i, v in A.items() if v[2] == "point")
    idx = {p: k for k, p in enumerate(pts)}
    rng = {}
    for r in links:
        tx, rx = int(r["tx"]), int(r["rx"])
        if tx < 10000 or r["received"] != "1" or not r["range_m"] or r["band"] not in TOF_BANDS:
            continue
        if not (tx in idx and (rx in idx or A.get(rx, (None, None, ""))[2] == "station")):
            continue
        rng.setdefault((tx, rx), {}).setdefault(r["band"], []).append(float(r["range_m"]))
    obs = []
    for (tx, rx), bands in rng.items():
        b = "uwb" if "uwb" in bands else "2400"
        obs.append((tx, rx, float(np.median(bands[b])), P.TOF_SIGMA_M[b]))
    x0 = np.concatenate([np.array(A[p][0], dtype=float) for p in pts])
    gps = {p: (np.array(A[p][0], dtype=float), np.array(A[p][1], dtype=float)) for p in pts}

    def res(x):
        out = []
        for (tx, rx, r, s) in obs:
            a = x[3 * idx[tx]: 3 * idx[tx] + 3]
            b = x[3 * idx[rx]: 3 * idx[rx] + 3] if rx in idx else A[rx][0]
            out.append((np.linalg.norm(a - b) - r) / s)
        for p in pts:
            g, sg = gps[p]
            out.extend((x[3 * idx[p]: 3 * idx[p] + 3] - g) / sg)
        return np.array(out)

    sol = least_squares(res, x0, loss="soft_l1", f_scale=2.0, max_nfev=300)
    B = dict(A)
    nuwb = sum(1 for o in obs if o[3] < 0.2 and o[1] in idx)
    for p in pts:
        B[p] = (sol.x[3 * idx[p]: 3 * idx[p] + 3], np.array([0.10 if nuwb else 1.0] * 3), "point")
    return B, (len(pts), len(obs), nuwb)


def hold_heights(d):
    txt = open(os.path.join(d, "run.txt")).read()
    for tok in txt.split():
        if tok.startswith("holdHeights="):
            return [float(v) for v in tok.split("=", 1)[1].split(",") if v]
    raise SystemExit("run.txt has no holdHeights")


def fit_air_law(links, A):
    """Per band: total loss = L0 + 10 n log10(d), from point-to-station links (no rubble)."""
    law = {}
    for b in BANDS:
        xs, ys = [], []
        for r in links:
            if r["band"] != b or r["received"] != "1":
                continue
            tx, rx = int(r["tx"]), int(r["rx"])
            if tx < 10000 or rx < 20000:
                continue
            dd = float(np.linalg.norm(A[tx][0] - A[rx][0]))
            if dd < 1.0:
                continue
            xs.append(10.0 * math.log10(dd))
            ys.append(P.TX_DBM[b] - float(r["rx_dbm"]))
        if len(xs) >= 10:
            X = np.vstack([np.ones(len(xs)), xs]).T
            coef, *_ = np.linalg.lstsq(X, np.array(ys), rcond=None)
            res = np.array(ys) - X @ coef
            law[b] = (float(coef[0]), float(coef[1]), float(max(np.std(res), 1.0)), len(xs))
        else:  # too few air links heard: free space with a generic 6 dB spread
            law[b] = (P.free_space_l0(b), 2.0, 6.0, len(xs))
    return law


def serpentine_spots(A, nhold):
    pts = sorted(i for i, v in A.items() if v[2] == "point")
    spots = [pts[k:k + nhold] for k in range(0, len(pts), nhold)]
    # generator order is x-major, y inner: reverse every other x-column for a serpentine walk
    xs = sorted({round(A[s[0]][0][0], 0) for s in spots})
    cols = {}
    for s in spots:
        cols.setdefault(round(A[s[0]][0][0], 0), []).append(s)
    order = []
    for k, x in enumerate(xs):
        col = sorted(cols[x], key=lambda s: A[s[0]][0][1])
        order.extend(col if k % 2 == 0 else col[::-1])
    return order


def solve(meas, law, A, method, surf_z):
    """meas: list of (band, anchor_id, rx_dbm or None, range or None)."""
    rss = [(b, a, p) for (b, a, p, r) in meas if p is not None]
    tof = [(b, a, r) for (b, a, p, r) in meas if r is not None and b in TOF_BANDS]
    if method == "tof":
        rss = []
    if method == "rss":
        tof = []
    if method == "tof" and len(tof) < 4:
        return None
    if method != "tof" and len(rss) < 4:
        return None
    bands_c = sorted({b for (b, a, p) in rss} | ({b for (b, a, r) in tof} if method == "joint" else set()))
    # initial guess: strongest-heard anchors' centroid, 1.5 m below the surface
    if rss:
        best = sorted(rss, key=lambda t: -t[2])[:4]
        x0 = np.mean([A[a][0] for (_, a, _) in best], axis=0)
    else:
        x0 = np.mean([A[a][0] for (_, a, _) in tof], axis=0)
    x0[2] = surf_z - 1.5
    c0 = []
    for b in bands_c:
        L0, n, _, _ = law[b]
        vals = [P.TX_DBM[b] - p - (L0 + 10 * n * math.log10(max(np.linalg.norm(x0 - A[a][0]), 1.0)))
                for (bb, a, p) in rss if bb == b]
        c0.append(max(np.median(vals), 0.0) if vals else 10.0)
    theta0 = np.concatenate([x0, np.array(c0)])
    ci = {b: 3 + k for k, b in enumerate(bands_c)}

    def resid(th):
        x = th[:3]
        out = []
        for (b, a, p) in rss:
            L0, n, s, _ = law[b]
            pa, sa, _ = A[a]
            dd = max(np.linalg.norm(x - pa), 0.5)
            s_tot = math.hypot(s, 10 * n / math.log(10) * float(np.mean(sa)) / dd)
            out.append((P.TX_DBM[b] - p - (L0 + 10 * n * math.log10(dd)) - th[ci[b]]) / s_tot)
        for (b, a, r) in tof:
            pa, sa, _ = A[a]
            dd = np.linalg.norm(x - pa)
            pred = dd
            s_tot = math.hypot(P.TOF_SIGMA_M[b], float(np.mean(sa)))
            if method == "joint":
                c = max(th[ci[b]], 0.0)
                pred = dd + c / P.rho_mid(b)
                s_tot = math.hypot(s_tot, c / P.rho_mid(b) * P.rho_rel_sd(b))
            out.append((r - pred) / s_tot)
        # generic prior: a buried device is below the rescuers' surface
        out.append(max(0.0, x[2] - surf_z) / 0.1)
        # rubble excess is not negative
        for b in bands_c:
            out.append(max(0.0, -th[ci[b]]) / 1.0)
        return np.array(out)

    sol = least_squares(resid, theta0, loss="soft_l1", f_scale=2.0, max_nfev=400)
    J = sol.jac
    try:
        cov = np.linalg.pinv(J.T @ J)
        sd_h = math.sqrt(max(cov[0, 0] + cov[1, 1], 0.0))
        sd_v = math.sqrt(max(cov[2, 2], 0.0))
    except np.linalg.LinAlgError:
        sd_h = sd_v = float("inf")
    return sol.x[:3], sd_h, sd_v, len(rss) + len(tof)


def solve_coop(meas, vv, law, A, surf_z, init):
    """meas[w]: worker w's list as in solve(); vv: list of (i, j, band, rx_dbm); init[w]: (x, c868, c24, cuwb)."""
    W = sorted(init)
    idx = {w: k for k, w in enumerate(W)}
    bands = ("868", "2400", "uwb")
    nper = 3 + len(bands)
    th0 = np.concatenate([np.concatenate([init[w][0], init[w][1]]) for w in W])

    def resid(th):
        out = []
        for w in W:
            base = idx[w] * nper
            x = th[base:base + 3]
            C = dict(zip(bands, th[base + 3:base + nper]))
            for (b, a, p, r) in meas[w]:
                L0, n, s, _ = law[b]
                pa, sa, _ = A[a]
                dd = max(np.linalg.norm(x - pa), 0.5)
                if p is not None:
                    s_tot = math.hypot(s, 10 * n / math.log(10) * float(np.mean(sa)) / dd)
                    out.append((P.TX_DBM[b] - p - (L0 + 10 * n * math.log10(dd)) - C[b]) / s_tot)
                if r is not None and b in TOF_BANDS:
                    c = max(C[b], 0.0)
                    s_tot = math.hypot(P.TOF_SIGMA_M[b], float(np.mean(sa)), c / P.rho_mid(b) * P.rho_rel_sd(b))
                    out.append((r - (dd + c / P.rho_mid(b))) / s_tot)
            out.append(max(0.0, x[2] - surf_z) / 0.1)
            for b in bands:
                out.append(max(0.0, -C[b]) / 1.0)
        for (i, j, b, p) in vv:
            if i not in idx or j not in idx:
                continue
            bi, bj = idx[i] * nper, idx[j] * nper
            xi, xj = th[bi:bi + 3], th[bj:bj + 3]
            ci, cj = th[bi + 3 + bands.index(b)], th[bj + 3 + bands.index(b)]
            L0, n, s, _ = law[b]
            dd = max(np.linalg.norm(xi - xj), 0.5)
            out.append((P.TX_DBM[b] - p - (L0 + 10 * n * math.log10(dd)) - ci - cj) / s)
        return np.array(out)

    sol = least_squares(resid, th0, loss="soft_l1", f_scale=2.0, max_nfev=200)
    return {w: sol.x[idx[w] * nper: idx[w] * nper + 3] for w in W}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("run")
    ap.add_argument("--frame", choices=("local", "gps", "uwb"), default="local",
                    help="local: rescuers' frame given (--collapseLocalSigma); gps: GPS fixes; uwb: B3, solved from "
                         "the points' ToF ranges to GPS-placed stations")
    ap.add_argument("--rescuers", type=int, default=2)
    ap.add_argument("--methods", default="tof,rss,joint,coop")
    ap.add_argument("--spots", type=float, default=1.0,
                    help="fraction of the surface spots the rescuers reach (coverage); 1 = all")
    ap.add_argument("--every", type=int, default=1, help="estimate every k slots")
    a = ap.parse_args()
    A = load_anchors(a.run, a.frame)
    holds = hold_heights(a.run)
    links = list(csv.DictReader(open(os.path.join(a.run, "obs_links.csv"))))
    if a.frame == "uwb":
        A, info = solve_rescuer_frame(a.run, A, links)
        with open(os.path.join(a.run, "frame_uwb.csv"), "w") as ff:  # system output: the solved rescuer frame
            ff.write("id,x,y,z\n")
            for i, v in sorted(A.items()):
                if v[2] == "point":
                    ff.write(f"{i},{v[0][0]:.4f},{v[0][1]:.4f},{v[0][2]:.4f}\n")
        print(f"B3: {info[0]} rescuer points solved from {info[1]} range pairs ({info[2]} point-to-point UWB)")
    law = fit_air_law(links, A)
    order = serpentine_spots(A, len(holds))
    order = order[: max(1, int(round(a.spots * len(order))))]
    chunks = np.array_split(np.arange(len(order)), a.rescuers)
    nslots = max(len(c) for c in chunks)
    # which spot each rescuer stands at in each slot
    at_slot = {t: [order[c[t]] for c in chunks if t < len(c)] for t in range(nslots)}
    by_slot = {}
    workers = set()
    for r in links:
        tx, rx = int(r["tx"]), int(r["rx"])
        if tx >= 10000:
            continue
        workers.add(tx)
        by_slot.setdefault(int(r["slot"]), []).append(r)
    have = max(by_slot) + 1 if by_slot else 0
    if have < nslots:
        raise SystemExit(f"run has {have} slots; the sweep needs {nslots} with {a.rescuers} rescuers")
    surf_z = float(np.median([A[i][0][2] for i, v in A.items() if v[2] == "point"])) - min(holds)
    surf_z_est = surf_z  # rescuers know their hold heights; the surface is the points minus hold
    meas = {w: [] for w in workers}
    vv = []  # worker-to-worker 868 MHz links heard so far (B4)
    out = open(os.path.join(a.run, f"estimates_{a.frame}.csv"), "w")
    out.write("method,slot,worker,x,y,z,sd_h,sd_v,n_meas\n")
    methods = a.methods.split(",")
    for t in range(nslots):
        here = {pid for spot in at_slot[t] for pid in spot}
        for r in by_slot.get(t, []):
            rx = int(r["rx"])
            if rx < 10000 and r["received"] == "1" and r["band"] == "868":
                vv.append((int(r["tx"]), rx, "868", float(r["rx_dbm"])))
                continue
            if rx not in here:
                continue
            if r["received"] != "1":
                continue
            rng = float(r["range_m"]) if r["range_m"] else None
            meas[int(r["tx"])].append((r["band"], rx, float(r["rx_dbm"]), rng))
        if (t + 1) % a.every and t != nslots - 1:
            continue
        init = {}
        for w in sorted(workers):
            for m in methods:
                if m == "coop":
                    continue
                res = solve(meas[w], law, A, m, surf_z_est)
                if res is None:
                    continue
                x, sh, sv, n = res
                out.write(f"{m},{t},{w},{x[0]:.3f},{x[1]:.3f},{x[2]:.3f},{sh:.3f},{sv:.3f},{n}\n")
        if "coop" in methods:
            # B4: start each worker at its joint solution, or, if it has too few rescuer links, at the
            # centroid of the rescuer points that heard it (or the surface centre)
            for w in sorted(workers):
                res = solve(meas[w], law, A, "joint", surf_z_est)
                if res is not None:
                    x0 = res[0]
                else:
                    heard = [A[a][0] for (b, a, p, r) in meas[w]]
                    pts_all = [v[0] for v in A.values() if v[2] == "point"]
                    x0 = np.mean(heard if heard else pts_all, axis=0)
                    x0[2] = surf_z_est - 1.5
                init[w] = (np.array(x0, dtype=float), np.array([20.0, 60.0, 120.0]))
            # one residual per worker pair: the median received power over the slots heard. A pair is heard in
            # every slot and its shadowing is fixed per link, so repeated slots would only overweight it.
            agg = {}
            for (i, j, b, p) in vv:
                agg.setdefault((min(i, j), max(i, j), b), []).append(p)
            vv_seen = [(i, j, b, float(np.median(ps))) for (i, j, b), ps in agg.items()]
            X = solve_coop(meas, vv_seen, law, A, surf_z_est, init)
            for w, x in X.items():
                out.write(f"coop,{t},{w},{x[0]:.3f},{x[1]:.3f},{x[2]:.3f},nan,nan,{len(meas[w])}\n")
    out.close()
    lawtxt = "; ".join(f"{b}: L0={v[0]:.1f} n={v[1]:.2f} sd={v[2]:.1f} (n={v[3]})" for b, v in law.items())
    print(f"air law {lawtxt}; slots {nslots}")


if __name__ == "__main__":
    main()
