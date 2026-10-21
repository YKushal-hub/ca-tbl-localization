#!/usr/bin/env python3
"""ML-PATHLOSS-POS (report/PREREG_ML_PATHLOSS_POS.md): ML-PATHLOSS's range models used for what an 868 MHz-only
RSSI localiser does with them -- multilaterate one packet from every gateway that heard it -- on the real Bonn
measurements, at locations the models were not trained on.

Ranges: scripts/pathloss_ml_bonn.py's models and its spatially blocked 5-fold cross-validation, imported unchanged
(same seed, same blocks, same folds). Packets: rows sharing date, packet number and GPS fix; those heard by >= 3
gateways after its 10 m filter.

usage: pathloss_ml_bonn_pos.py <lora-bonn/survey dir> <outdir>
Writes <outdir>/ML_PATHLOSS_POS.md and prints it. Committed with the registration, before any position is solved.
"""
import math
import os
import sys

import numpy as np
import pandas as pd
from scipy.optimize import least_squares

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pathloss_ml_bonn as pm  # noqa: E402

NBOOT = 10000
MIN_GW = 3


def enu(lat, lon, lat0):
    return pm.R_EARTH * np.radians(lon) * math.cos(lat0), pm.R_EARTH * np.radians(lat)


def solve(gx, gy, ld):
    w = 10.0 ** (-ld)
    p0 = np.array([np.sum(w * gx) / w.sum(), np.sum(w * gy) / w.sum()])

    def res(p):
        return np.log10(np.maximum(np.hypot(p[0] - gx, p[1] - gy), 1.0)) - ld

    return least_squares(res, p0).x


def block_ci(blocks, diff):
    nb = blocks.max() + 1
    sums = np.bincount(blocks, weights=diff, minlength=nb)
    cnts = np.bincount(blocks, minlength=nb).astype(float)
    keep = cnts > 0
    sums, cnts = sums[keep], cnts[keep]
    idx = np.random.default_rng(pm.SEED).integers(0, len(sums), size=(NBOOT, len(sums)))
    m = sums[idx].sum(axis=1) / cnts[idx].sum(axis=1)
    return float(diff.mean()), float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975)), int(len(sums))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    src, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    s = pm.load(src)
    rng = np.random.default_rng(pm.SEED)
    nblk = s["block"].max() + 1
    bfold = rng.integers(0, pm.K, size=nblk)
    pred = pm.cross_val(s, bfold[s["block"].values])
    gws = pd.read_csv(os.path.join(src, "gateways.csv"), dtype={"id": str}).set_index("id")
    lat0 = math.radians(s["latitude"].mean())
    gx_all, gy_all = enu(gws.loc[s["gw"], "latitude"].values, gws.loc[s["gw"], "longitude"].values, lat0)
    key = s.groupby(["transceived_at", "pkt_number", "latitude", "longitude"]).ngroup().values
    order = np.argsort(key, kind="stable")
    bounds = np.flatnonzero(np.diff(key[order])) + 1
    groups = [g for g in np.split(order, bounds) if s["gw"].values[g].size >= MIN_GW
              and len(set(s["gw"].values[g])) == s["gw"].values[g].size]
    tx, ty = enu(s["latitude"].values, s["longitude"].values, lat0)
    names = list(pred)
    err = {n: np.empty(len(groups)) for n in names}
    err["G0 centroid of receiving gateways"] = np.empty(len(groups))
    blocks = np.empty(len(groups), dtype=int)
    ngw = np.empty(len(groups), dtype=int)
    for i, g in enumerate(groups):
        gx, gy = gx_all[g], gy_all[g]
        px, py = tx[g[0]], ty[g[0]]
        blocks[i] = s["block"].values[g[0]]
        ngw[i] = len(g)
        err["G0 centroid of receiving gateways"][i] = math.hypot(gx.mean() - px, gy.mean() - py)
        for n in names:
            p = solve(gx, gy, pred[n][g])
            err[n][i] = math.hypot(p[0] - px, p[1] - py)
    le = {n: np.log10(np.maximum(e, 1.0)) for n, e in err.items()}
    md = ["# ML-PATHLOSS-POS — Bonn 868 MHz, packet multilateration (scripts/pathloss_ml_bonn_pos.py)", "",
          f"{len(groups)} packets heard by >= {MIN_GW} gateways (of {key.max() + 1}), {len(set(blocks))} spatial blocks; "
          f"ranges from ML-PATHLOSS's blocked {pm.K}-fold cross-validation. Gateways per packet: "
          + ", ".join(f"{k}: {int((ngw == k).sum())}" for k in sorted(set(ngw))) + ".", "",
          "| position from | median error (m) | 68 % error (m) | 95 % error (m) | mean log10 error |",
          "|---|---|---|---|---|"]
    for n, e in err.items():
        md.append(f"| {n} | {np.median(e):.0f} | {np.quantile(e, 0.68):.0f} | {np.quantile(e, 0.95):.0f} | "
                  f"{le[n].mean():.4f} |")
    classical = [n for n in names if n.startswith("C")]
    best = min(classical, key=lambda n: le[n].mean())
    inv = min([n for n in classical if "inverted" in n], key=lambda n: le[n].mean())
    mln = [n for n in names if n.startswith("ML")][0]
    lin = [n for n in names if n.startswith("C4")][0]
    d1 = block_ci(blocks, le[mln] - le[best])
    r1 = np.median(err[mln]) / np.median(err[best])
    helps = bool(d1[2] < 0 and r1 <= 0.90)
    small = bool(d1[2] < 0 and r1 > 0.90)
    d2 = block_ci(blocks, le[mln] - le[inv])
    d3 = block_ci(blocks, le[lin] - le[inv])
    md += ["", f"Best classical (lowest mean log10 error, chosen on the test packets): **{best}**. "
           f"Best inversion: **{inv}**.", "",
           "| contrast (mean log10 position error) | difference [95 %] | median error ratio | verdict |",
           "|---|---|---|---|",
           f"| ML - best classical | {d1[0]:+.4f} [{d1[1]:+.4f}, {d1[2]:+.4f}] | {r1:.3f} | "
           f"**{'HELPS' if helps else 'SMALL' if small else 'NO GAIN'}** |",
           f"| ML - best inversion | {d2[0]:+.4f} [{d2[1]:+.4f}, {d2[2]:+.4f}] | "
           f"{np.median(err[mln]) / np.median(err[inv]):.3f} | **{'GAIN' if d2[2] < 0 else 'NO GAIN'}** |",
           f"| C4 - best inversion | {d3[0]:+.4f} [{d3[1]:+.4f}, {d3[2]:+.4f}] | "
           f"{np.median(err[lin]) / np.median(err[inv]):.3f} | **{'GAIN' if d3[2] < 0 else 'NO GAIN'}** |", "",
           f"Bootstrap over {d1[3]} blocks, {NBOOT} resamples. HELPS: interval below 0 and median ratio <= 0.90; "
           "SMALL: interval below 0, ratio > 0.90."]
    print("\n".join(md))
    open(os.path.join(out, "ML_PATHLOSS_POS.md"), "w").write("\n".join(md) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
