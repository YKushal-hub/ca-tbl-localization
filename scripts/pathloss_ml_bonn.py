#!/usr/bin/env python3
"""ML-PATHLOSS (report/PREREG_ML_PATHLOSS.md): does a machine-learned distance estimator beat classical
log-distance inversion on REAL 868 MHz LoRa measurements, at locations it was not trained on?

Data: Rademacher et al. 2021 (VTC2021-Fall, arXiv:2109.07768), github.com/mclab-hbrs/lora-bonn, survey/samples.csv
and survey/gateways.csv (Data licence Germany - attribution - 2.0). Not copied into this repository.

usage: pathloss_ml_bonn.py <lora-bonn/survey dir> <outdir>
Writes <outdir>/ML_PATHLOSS.md and prints it. Committed with the registration, before any model is fitted.
"""
import math
import os
import sys

import numpy as np
import pandas as pd
from sklearn.ensemble import HistGradientBoostingRegressor

SEED = 20261021
K = 5
BLOCK_M = 1000.0
NBOOT = 10000
MIN_D = 10.0
MIN_GW_TRAIN = 50
R_EARTH = 6371008.8


def load(src):
    s = pd.read_csv(os.path.join(src, "samples.csv"), dtype={"gw": str})
    g = pd.read_csv(os.path.join(src, "gateways.csv"), dtype={"id": str}).set_index("id")
    s = s.dropna(subset=["latitude", "longitude", "rssi", "snr", "gw"])
    s = s[s["gw"].isin(g.index)].copy()
    la1, lo1 = np.radians(s["latitude"].values), np.radians(s["longitude"].values)
    la2, lo2 = np.radians(g.loc[s["gw"], "latitude"].values), np.radians(g.loc[s["gw"], "longitude"].values)
    a = np.sin((la2 - la1) / 2) ** 2 + np.cos(la1) * np.cos(la2) * np.sin((lo2 - lo1) / 2) ** 2
    s["d"] = 2 * R_EARTH * np.arcsin(np.sqrt(a))
    s = s[s["d"] >= MIN_D].copy()
    lat0 = math.radians(s["latitude"].mean())
    x = R_EARTH * np.radians(s["longitude"].values) * math.cos(lat0)
    y = R_EARTH * np.radians(s["latitude"].values)
    bx, by = np.floor(x / BLOCK_M).astype(np.int64), np.floor(y / BLOCK_M).astype(np.int64)
    s["block"] = pd.factorize(bx * 1000003 + by)[0]
    s["gwi"] = pd.factorize(s["gw"])[0]
    s["ld"] = np.log10(s["d"].values)
    return s.reset_index(drop=True)


def fit_line(xv, yv):
    b, a = np.polyfit(xv, yv, 1)
    return a, b


def predict(tr, te):
    """log10 distance predictions for te from models fitted on tr: C1-C4 classical, ML."""
    out = {}
    a, b = fit_line(tr["ld"].values, tr["rssi"].values)  # rssi = a + b log10 d
    out["C1 global LDPL inverted"] = (te["rssi"].values - a) / b
    c2 = np.empty(len(te)); c3 = np.empty(len(te)); c4 = np.empty(len(te))
    ci, ck = fit_line(tr["rssi"].values, tr["ld"].values)  # log10 d = ci + ck rssi
    X = np.c_[np.ones(len(tr)), tr["rssi"].values, tr["snr"].values]
    w4 = np.linalg.lstsq(X, tr["ld"].values, rcond=None)[0]
    for gi in np.unique(te["gwi"].values):
        m = te["gwi"].values == gi
        t = tr[tr["gwi"] == gi]
        if len(t) >= MIN_GW_TRAIN:
            ag, bg = fit_line(t["ld"].values, t["rssi"].values)
            c2[m] = (te["rssi"].values[m] - ag) / bg
            cg, kg = fit_line(t["rssi"].values, t["ld"].values)
            c3[m] = cg + kg * te["rssi"].values[m]
            Xg = np.c_[np.ones(len(t)), t["rssi"].values, t["snr"].values]
            wg = np.linalg.lstsq(Xg, t["ld"].values, rcond=None)[0]
            c4[m] = wg[0] + wg[1] * te["rssi"].values[m] + wg[2] * te["snr"].values[m]
        else:
            c2[m] = (te["rssi"].values[m] - a) / b
            c3[m] = ci + ck * te["rssi"].values[m]
            c4[m] = w4[0] + w4[1] * te["rssi"].values[m] + w4[2] * te["snr"].values[m]
    out["C2 per-gateway LDPL inverted"] = c2
    out["C3 per-gateway inverse regression (RSSI)"] = c3
    out["C4 per-gateway inverse regression (RSSI, SNR)"] = c4
    ml = HistGradientBoostingRegressor(max_iter=300, learning_rate=0.1, categorical_features=[2],
                                       random_state=SEED)
    ml.fit(tr[["rssi", "snr", "gwi"]].values, tr["ld"].values)
    out["ML gradient boosting (RSSI, SNR, gateway)"] = ml.predict(te[["rssi", "snr", "gwi"]].values)
    return out


def cross_val(s, folds):
    pred = {}
    for k in range(K):
        te, tr = s[folds == k], s[folds != k]
        for n, p in predict(tr, te).items():
            pred.setdefault(n, np.empty(len(s)))[np.where(folds == k)[0]] = p
    return pred


def summary(s, p):
    e = np.abs(p - s["ld"].values)
    ae = np.abs(10 ** p - s["d"].values)
    return 10 ** np.median(e), 10 ** np.quantile(e, 0.68), float(np.median(ae)), float(e.mean())


def block_ci(s, diff):
    blocks = s["block"].values
    nb = blocks.max() + 1
    sums = np.bincount(blocks, weights=diff, minlength=nb)
    cnts = np.bincount(blocks, minlength=nb).astype(float)
    keep = cnts > 0
    sums, cnts = sums[keep], cnts[keep]
    idx = np.random.default_rng(SEED).integers(0, len(sums), size=(NBOOT, len(sums)))
    m = sums[idx].sum(axis=1) / cnts[idx].sum(axis=1)
    return float(diff.mean()), float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975)), int(len(sums))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    s = load(sys.argv[1])
    out = sys.argv[2]
    os.makedirs(out, exist_ok=True)
    rng = np.random.default_rng(SEED)
    nblk = s["block"].max() + 1
    bfold = rng.integers(0, K, size=nblk)
    pred = cross_val(s, bfold[s["block"].values])
    md = ["# ML-PATHLOSS — Bonn 868 MHz measurements (scripts/pathloss_ml_bonn.py)", "",
          f"{len(s)} packets with d >= {MIN_D:g} m, {s['gw'].nunique()} gateways, {nblk} spatial blocks of "
          f"{BLOCK_M:g} m, {K}-fold cross-validation by block (no test block seen in training).", "",
          "| model | median factor | 68 % factor | median abs error (m) | mean abs log10 error |",
          "|---|---|---|---|---|"]
    res = {}
    for n, p in pred.items():
        res[n] = summary(s, p)
        r = res[n]
        md.append(f"| {n} | x/÷{r[0]:.2f} | x/÷{r[1]:.2f} | {r[2]:.0f} | {r[3]:.4f} |")
    classical = [n for n in res if n.startswith("C")]
    best = min(classical, key=lambda n: res[n][3])
    mln = [n for n in res if n.startswith("ML")][0]
    e_ml = np.abs(pred[mln] - s["ld"].values)
    e_c = np.abs(pred[best] - s["ld"].values)
    d = block_ci(s, e_ml - e_c)
    ratio = res[mln][1] / res[best][1]
    helps = bool(d[2] < 0 and ratio <= 0.90)
    small = bool(d[2] < 0 and ratio > 0.90)
    md += ["", f"Best classical (lowest mean abs log10 error, chosen on the test predictions): **{best}**.", "",
           f"ML - best classical, mean abs log10 error: {d[0]:+.4f} [{d[1]:+.4f}, {d[2]:+.4f}] "
           f"(95 %, bootstrap over {d[3]} blocks). 68 % factor ratio ML / best classical: {ratio:.3f}.", "",
           f"**Verdict: {'HELPS' if helps else 'SMALL' if small else 'NO GAIN'}** "
           f"(HELPS: interval below 0 and ratio <= 0.90; SMALL: interval below 0, ratio > 0.90).", "",
           "## By true distance (68 % factor, cross-validated by block)", "",
           "| distance | packets | " + " | ".join(n.split(" ")[0] for n in res) + " |",
           "|---|---|" + "---|" * len(res)]
    for lo, hi in ((10, 500), (500, 1000), (1000, 2000), (2000, 1e9)):
        m = (s["d"].values >= lo) & (s["d"].values < hi)
        if m.sum() == 0:
            continue
        cells = [f"{10 ** np.quantile(np.abs(pred[n][m] - s['ld'].values[m]), 0.68):.2f}" for n in res]
        md.append(f"| {lo:g}-{'' if hi > 1e8 else f'{hi:g}'} m | {int(m.sum())} | " + " | ".join(cells) + " |")
    rf = rng.integers(0, K, size=len(s))
    pr = cross_val(s, rf)
    md += ["", "## Contrast (not the registered test): random-packet folds, which leak location", "",
           "| model | 68 % factor |", "|---|---|"]
    for n, p in pr.items():
        md.append(f"| {n} | x/÷{summary(s, p)[1]:.2f} |")
    print("\n".join(md))
    open(os.path.join(out, "ML_PATHLOSS.md"), "w").write("\n".join(md) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
