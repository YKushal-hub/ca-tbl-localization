#!/usr/bin/env python3
"""GEO-01 stage 1: geometric screening of gateway layouts (docs/SPEC_CONFORMANCE_PLAN.md
sect 3.4). Analysis only -- no simulation of the arms. The rule it applies is registered in
report/PREREG_GEO01_STAGE1.md, committed before this script's first run.

For each candidate (shape x N) it asks the BUILD where the gateways go (one 2.5 s run with
--vizEvents, reading deployment.json), then, over a lattice of the search area, computes
from the probe's tier-1 PDR and range error against distance (--probe=true,
probe_range_sweep.csv):
  - P(>=1), P(>=2), P(>=3 gateways decode) -- the Case D/B/A coverage -- from a
    Poisson-binomial over the gateways, links independent (stated: shadowing is in fact
    correlated between nearby links, which this ignores);
  - the predicted position error where P(>=3) >= 0.5: the Fisher information
    sum_i p_i u_i u_i^T / sigma_i^2 over every gateway, weighted by its PDR, inverted;
    CEP50 of that covariance, 0.5887 (sigma_major + sigma_minor) (stated approximation).
sigma_i = sqrt(pi/2) x the probe's mean absolute range error at that slant distance
(normal errors). Beyond the probe's last distance PDR and error are extrapolated
log-linearly from its last two points, PDR floored at 0 (stated).

usage: geo_screen.py --bin <binary> --probe <probe_range_sweep.csv> --out <dir>
                     [--boxHalf 5000] [--lattice 50] [--N 3,4,5,6,8] [--files a.csv,b.csv]
Writes <out>/geo_screen.csv (one row per candidate) and <out>/geo_screen.md (the table and
the pruned set), and prints the pruned set.
"""
import argparse
import csv
import json
import math
import os
import subprocess
import sys
import tempfile

import numpy as np

GW_H, NODE_H = 15.0, 1.5


def probe_model(path):
    rows = [r for r in csv.DictReader(open(path)) if r["tier"] == "868MHz"]
    d = np.array([float(r["distance_m"]) for r in rows])
    p = np.array([float(r["pdr"]) for r in rows])
    e = np.array([float(r["mean_abs_range_err_m"]) for r in rows])
    order = np.argsort(d)
    d, p, e = d[order], p[order], e[order]

    def interp(x, y, xs):
        out = np.interp(xs, x, y)
        hi = xs > x[-1]
        if np.any(hi):
            slope = (y[-1] - y[-2]) / (math.log(x[-1]) - math.log(x[-2]))
            out[hi] = y[-1] + slope * (np.log(xs[hi]) - math.log(x[-1]))
        return out

    def pdr(dist):
        return np.clip(interp(d, p, dist), 0.0, 1.0)

    def sigma(dist):
        return np.maximum(interp(d, e, dist), 0.5) * math.sqrt(math.pi / 2.0)

    return pdr, sigma


def layout_positions(binary, flags, box):
    with tempfile.TemporaryDirectory() as td:
        cmd = [binary, "--arms=static_868", "--duration=2.5", "--trace=false", "--vizEvents=true",
               f"--boxHalf={box}", f"--outdir={td}"] + flags
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            raise RuntimeError(f"{' '.join(flags)}: exit {r.returncode}: {r.stderr.strip()[-200:]}")
        dj = json.load(open(os.path.join(td, "deployment.json")))
    return [(a["x"], a["y"]) for a in dj["anchors"]["0"]]


def screen(gws, pdr, sigma, box, step):
    xs = np.arange(-box + step / 2, box, step)
    X, Y = np.meshgrid(xs, xs)
    X, Y = X.ravel(), Y.ravel()
    n = len(gws)
    P = np.zeros((n, X.size))
    J11 = np.zeros(X.size)
    J12 = np.zeros(X.size)
    J22 = np.zeros(X.size)
    for i, (gx, gy) in enumerate(gws):
        dx, dy = X - gx, Y - gy
        hd = np.hypot(dx, dy)
        slant = np.sqrt(hd ** 2 + (GW_H - NODE_H) ** 2)
        p = pdr(slant)
        s = sigma(slant)
        ux, uy = dx / np.maximum(hd, 1e-9), dy / np.maximum(hd, 1e-9)
        w = p / s ** 2
        J11 += w * ux * ux
        J12 += w * ux * uy
        J22 += w * uy * uy
        P[i] = p
    # Poisson-binomial: dist[k] = P(exactly k decode), k = 0..3 (3 = 3 or more)
    dist = np.zeros((4, X.size))
    dist[0] = 1.0
    for i in range(n):
        p = P[i]
        new = np.zeros_like(dist)
        new[0] = dist[0] * (1 - p)
        for k in range(1, 4):
            new[k] = dist[k] * (1 - p) + dist[k - 1] * p
        new[3] += dist[3] * p
        dist = new
    p3 = dist[3]
    p2 = dist[2] + p3
    p1 = 1.0 - dist[0]
    det = J11 * J22 - J12 ** 2
    ok = det > 0
    tr = np.where(ok, (J11 + J22) / np.where(ok, det, 1.0), np.inf)
    disc = np.where(ok, np.sqrt(np.maximum(((J11 - J22) / 2) ** 2 + J12 ** 2, 0.0)), 0.0)
    lam_max = np.where(ok, ((J11 + J22) / 2 + disc) / np.where(ok, det, 1.0), np.inf)
    lam_min = np.where(ok, ((J11 + J22) / 2 - disc) / np.where(ok, det, 1.0), np.inf)
    cep = 0.5887 * (np.sqrt(lam_max) + np.sqrt(np.maximum(lam_min, 0.0)))
    fix = p3 >= 0.5
    ce = cep[fix & np.isfinite(cep)]
    return {
        "area_p3_ge_095": float(np.mean(p3 >= 0.95)),
        "mean_p3": float(np.mean(p3)),
        "mean_p2": float(np.mean(p2)),
        "mean_p1": float(np.mean(p1)),
        "area_fixable": float(np.mean(fix)),
        "cep50_median_m": float(np.median(ce)) if ce.size else float("inf"),
        "cep50_p95_m": float(np.percentile(ce, 95)) if ce.size else float("inf"),
    }


def candidates(Ns, files):
    out = []
    for n in Ns:
        out.append(("ring", n, True, [f"--nGateways={n}"]))
        for arc in (90, 140, 180, 240):
            out.append((f"arc{arc}", n, True,
                        [f"--nGateways={n}", f"--gwArcDeg={arc}", "--gwArcCentreDeg=180"]))
        for lay in ("perimeter", "cluster", "star", "lshape"):
            out.append((lay, n, True, [f"--nGateways={n}", f"--gwLayout={lay}"]))
        out.append(("ridge", n, True, [f"--nGateways={n}", "--gwRidge=true"]))
    for g in (2, 3):
        out.append((f"grid{g}x{g}", g * g, False, [f"--gwGrid={g}"]))
    for f in files:
        name = os.path.splitext(os.path.basename(f))[0]
        with open(f) as fh:
            n = sum(1 for line in fh if line.strip() and line[0] not in "#x")
        realistic = "unrestricted" not in name
        out.append((name, n, realistic, ["--gwLayout=file", f"--gwLayoutFile={os.path.abspath(f)}"]))
    return out


def prune(rows):
    """The registered rule (report/PREREG_GEO01_STAGE1.md sect 4): per N, among REALISTIC
    candidates, rank by the mean P(>=3) over the area, highest first; two within 0.005 of
    each other are ordered by the lower median predicted CEP50; keep the best two."""
    keep = []
    for n in sorted({r["N"] for r in rows}):
        cand = [r for r in rows if r["N"] == n and r["realistic"]]

        def key(r):
            return (-round(r["mean_p3"] / 0.005), r["cep50_median_m"])
        cand.sort(key=key)
        keep += cand[:2]
    return keep


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", required=True)
    ap.add_argument("--probe", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--boxHalf", type=float, default=5000.0)
    ap.add_argument("--lattice", type=float, default=50.0)
    ap.add_argument("--N", default="3,4,5,6,8")
    ap.add_argument("--files", default="")
    a = ap.parse_args()
    pdr, sigma = probe_model(a.probe)
    Ns = [int(x) for x in a.N.split(",")]
    files = [f for f in a.files.split(",") if f]
    os.makedirs(a.out, exist_ok=True)
    rows = []
    for name, n, realistic, flags in candidates(Ns, files):
        gws = layout_positions(a.bin, flags, a.boxHalf)
        m = screen(gws, pdr, sigma, a.boxHalf, a.lattice)
        rows.append(dict(shape=name, N=len(gws), realistic=realistic, flags=" ".join(flags),
                         gateways=";".join(f"{x:.1f}:{y:.1f}" for x, y in gws), **m))
        print(f"  {name:22s} N={len(gws)} P3>=.95 {m['area_p3_ge_095']:.3f} meanP3 "
              f"{m['mean_p3']:.3f} P2 {m['mean_p2']:.3f} P1 {m['mean_p1']:.3f} CEP50 med "
              f"{m['cep50_median_m']:.0f} p95 {m['cep50_p95_m']:.0f}", flush=True)
    keys = ["shape", "N", "realistic", "area_p3_ge_095", "mean_p3", "mean_p2", "mean_p1",
            "area_fixable", "cep50_median_m", "cep50_p95_m", "flags", "gateways"]
    with open(os.path.join(a.out, "geo_screen.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=keys, lineterminator="\n")
        w.writeheader()
        for r in rows:
            w.writerow({k: r[k] for k in keys})
    keep = prune(rows)
    with open(os.path.join(a.out, "geo_screen.md"), "w") as f:
        f.write("| shape | N | realistic | area P(>=3)>=0.95 | mean P3 | mean P2 | mean P1 | "
                "CEP50 median m | CEP50 p95 m |\n|---|---|---|---|---|---|---|---|---|\n")
        for r in rows:
            f.write(f"| {r['shape']} | {r['N']} | {'yes' if r['realistic'] else 'no'} | "
                    f"{r['area_p3_ge_095']:.3f} | {r['mean_p3']:.3f} | {r['mean_p2']:.3f} | "
                    f"{r['mean_p1']:.3f} | {r['cep50_median_m']:.0f} | {r['cep50_p95_m']:.0f} |\n")
        f.write("\nPruned set (registered rule):\n\n")
        for r in keep:
            f.write(f"- N={r['N']}: {r['shape']} ({r['flags']})\n")
    print("PRUNED:", [(r["N"], r["shape"]) for r in keep])
    return 0


if __name__ == "__main__":
    sys.exit(main())
