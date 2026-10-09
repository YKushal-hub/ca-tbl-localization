#!/usr/bin/env python3
"""Tier-1 (868 MHz) validation plot: the engine's loss against measured LoRa path loss.

usage: tier1_validation.py <lora-bonn/survey dir> <txDbm> <sensDbm> <outdir> <label>=<probe_range_sweep.csv> ...

Data: Rademacher et al., "Path Loss in Urban LoRa Networks: A Large-Scale Measurement Study",
VTC2021-Fall (arXiv 2109.07768); the public set github.com/mclab-hbrs/lora-bonn, survey/samples.csv
and gateways.csv (licence dl-de/by-2-0). The repository is NOT vendored; the caller passes its
path and this script prints the SHA-256 of both files.

Measured path loss is the dataset notebook's own definition (analyze.ipynb cells 3, 10, 12):
  rpp = rssi + min(snr, 0);  PL = (14 dBm TX + 3 dBi GW antenna - 2 dB cable) - rpp.
Distance is great-circle (haversine, R = 6371.0088 km); the notebook uses geopy's geodesic,
which differs by < 0.5 % at these ranges.

Engine (probe_range_sweep.csv, tier 868MHz rows), one probe per environment class given:
  - mean_pl_db: the path-loss TERM only (result.pathLossDb), over ALL trials;
  - total loss reconstructed from mean_margin_db, also over all trials:
      L = tx - sens - margin  ...  tx - sens - margin + 1.5,
    a band because the margin is taken against an effective sensitivity that is 1.5 dB lower
    whenever the link was receiving (linkHysteresisMarginDb, CLAUDE.md sect 5). Antenna gain
    is 0 dB in the engine, so L is comparable with the measured PL.

Censoring (rules 1-2): the measured PL exists only for received packets. Past the distance
where reception thins out, the measured median is biased LOW (the lossy draws are missing),
while the engine's means are over all trials. The plot marks where the engine's own PDR falls
below 0.9, and the measured median beyond SF12's sensitivity limit cannot exceed ~152 dB
(15 dB + 137 dBm, SX1276 SF12/125 kHz datasheet sensitivity).
"""
import csv
import hashlib
import math
import os
import sys

import numpy as np

EIRP_AND_RX_GAIN_LOSS = 14 + 3 - 2
R_EARTH_M = 6371008.8
FIT_N, FIT_PL1KM = 1.58, 132.41  # Rademacher 2021 sect IV, verbatim


def sha(p):
    return hashlib.sha256(open(p, "rb").read()).hexdigest()


def hav(lat1, lon1, lat2, lon2):
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = p2 - p1, math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * R_EARTH_M * math.asin(math.sqrt(a))


def measured(survey):
    gw = {r["id"]: (float(r["latitude"]), float(r["longitude"])) for r in csv.DictReader(open(os.path.join(survey, "gateways.csv")))}
    d, pl = [], []
    for r in csv.DictReader(open(os.path.join(survey, "samples.csv"))):
        g = gw[r["gw"]]
        dist = hav(float(r["latitude"]), float(r["longitude"]), g[0], g[1])
        if dist <= 0.0:
            continue
        d.append(dist)
        pl.append(EIRP_AND_RX_GAIN_LOSS - (float(r["rssi"]) + min(float(r["snr"]), 0.0)))
    return np.array(d), np.array(pl)


def refit(d, pl):
    """The paper's fit procedure: group by distance rounded to 10 m, median per group, least
    squares of PL = PL1km + 10 n log10(d_km)."""
    key = np.round(d / 1000.0, 2)
    ks = np.unique(key)
    ks = ks[ks > 0]
    med = np.array([np.median(pl[key == k]) for k in ks])
    slope, icpt = np.polyfit(np.log10(ks), med, 1)
    resid = pl - (icpt + slope * np.log10(np.maximum(d, 1.0) / 1000.0))
    return slope / 10.0, icpt, float(np.std(resid))


def main():
    if len(sys.argv) < 6:
        print(__doc__)
        return 2
    survey, tx, sens, out = sys.argv[1], float(sys.argv[2]), float(sys.argv[3]), sys.argv[4]
    probes = [a.rsplit("=", 1) for a in sys.argv[5:]]
    os.makedirs(out, exist_ok=True)
    d, pl = measured(survey)
    n, p1, sd = refit(d, pl)
    lines = [f"samples.csv sha256 {sha(os.path.join(survey, 'samples.csv'))}",
             f"gateways.csv sha256 {sha(os.path.join(survey, 'gateways.csv'))}",
             *[f"probe {lab} sha256 {sha(p)}" for lab, p in probes],
             f"measured samples: {len(d)} (paper: 112,372 after its post-processing; Table I: 112,404)",
             f"refit on this file, the paper's procedure: n = {n:.3f}, PL1km = {p1:.2f} dB, residual SD = {sd:.2f} dB "
             f"(paper: n = {FIT_N}, PL1km = {FIT_PL1KM}, sigma = 9.9)"]
    edges = np.logspace(np.log10(100.0), np.log10(13000.0), 23)
    rows = []
    for lo, hi in zip(edges[:-1], edges[1:]):
        m = (d >= lo) & (d < hi)
        if m.sum() < 30:
            continue
        rows.append(dict(kind="measured", distance_m=f"{math.sqrt(lo * hi):.1f}", n=int(m.sum()),
                         pl_p10_db=f"{np.percentile(pl[m], 10):.2f}", pl_median_db=f"{np.median(pl[m]):.2f}",
                         pl_p90_db=f"{np.percentile(pl[m], 90):.2f}", pl_term_db="", total_lo_db="", total_hi_db="", pdr=""))
    engs = {}
    for lab, p in probes:
        engs[lab] = [r for r in csv.DictReader(open(p)) if r["tier"] == "868MHz"]
        for r in engs[lab]:
            lo = tx - sens - float(r["mean_margin_db"])
            rows.append(dict(kind=f"engine {lab}", distance_m=r["distance_m"], n=r["n"], pl_p10_db="", pl_median_db="",
                             pl_p90_db="", pl_term_db=r["mean_pl_db"], total_lo_db=f"{lo:.2f}", total_hi_db=f"{lo + 1.5:.2f}",
                             pdr=r["pdr"]))
    with open(os.path.join(out, "tier1_validation.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    # Engine minus measured median at the engine's distances, where a measured bin holds them.
    meas = [r for r in rows if r["kind"] == "measured"]
    for lab, eng in engs.items():
        lines.append(f"engine ({lab}) - measured median, per engine distance (bin holding it):")
        for r in eng:
            x = float(r["distance_m"])
            b = next((m for m in meas if float(m["distance_m"]) / 1.26 <= x < float(m["distance_m"]) * 1.26), None)
            if b is None:
                continue
            lo = tx - sens - float(r["mean_margin_db"])
            lines.append(f"  {x:7.0f} m: PL term {float(r['mean_pl_db']) - float(b['pl_median_db']):+6.2f} dB, "
                         f"total {lo - float(b['pl_median_db']):+6.2f} .. {lo + 1.5 - float(b['pl_median_db']):+6.2f} dB "
                         f"(engine pdr {float(r['pdr']):.3f}, measured n {b['n']})")
    open(os.path.join(out, "tier1_validation.txt"), "w").write("\n".join(lines) + "\n")
    print("\n".join(lines))

    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    ink, ink2, grid, surf = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"
    c_meas, c_term, c_total = "#2a78d6", "#eb6834", "#1baf7a"
    fig, ax = plt.subplots(figsize=(8.0, 5.0), dpi=150)
    fig.patch.set_facecolor(surf)
    ax.set_facecolor(surf)
    mx = np.array([float(r["distance_m"]) for r in meas])
    ax.fill_between(mx, [float(r["pl_p10_db"]) for r in meas], [float(r["pl_p90_db"]) for r in meas],
                    color=c_meas, alpha=0.15, linewidth=0)
    ax.plot(mx, [float(r["pl_median_db"]) for r in meas], color=c_meas, lw=2, marker="o", ms=4,
            label="measured, Bonn (median, 10-90 % band; received packets only)")
    xs = np.logspace(np.log10(100), np.log10(13000), 100)
    ax.plot(xs, FIT_PL1KM + 10 * FIT_N * np.log10(xs / 1000), color=c_meas, lw=1, ls=":",
            label=f"Rademacher 2021 fit, n = {FIT_N}, PL1km = {FIT_PL1KM} dB")
    for (lab, eng), col, mk in zip(engs.items(), (c_total, c_term), ("s", "^")):
        ex = np.array([float(r["distance_m"]) for r in eng])
        lo = np.array([tx - sens - float(r["mean_margin_db"]) for r in eng])
        ax.fill_between(ex, lo, lo + 1.5, color=col, alpha=0.35, linewidth=0)
        ax.plot(ex, lo + 0.75, color=col, lw=2, marker=mk, ms=4, ls="-" if mk == "s" else "--",
                label=f"engine total loss, {lab} (band: hysteresis)")
        cut = next((float(r["distance_m"]) for r in eng if float(r["pdr"]) < 0.9), None)
        if cut:
            ax.axvline(cut, color=col, lw=1, ls="-.")
    ax.set_xscale("log")
    ax.set_xlabel("distance (m)", color=ink)
    ax.set_ylabel("path loss (dB)", color=ink)
    ax.set_title("Tier 1, 868 MHz: engine total loss against measured urban path loss (Bonn)", color=ink, fontsize=10)
    ax.grid(True, which="major", color=grid, lw=0.8)
    for s in ax.spines.values():
        s.set_color(grid)
    ax.tick_params(colors=ink2)
    ax.legend(fontsize=8, frameon=False, labelcolor=ink, loc="upper left")
    fig.text(0.01, 0.01, "Bonn is urban; the scenario's tier 1 is RURAL_FORESTED. Dash-dot lines: engine PDR < 0.9. "
             "Data: github.com/mclab-hbrs/lora-bonn (dl-de/by-2-0).", fontsize=7, color=ink2)
    fig.tight_layout(rect=(0, 0.03, 1, 1))
    fig.savefig(os.path.join(out, "tier1_validation.png"), facecolor=surf)
    return 0


if __name__ == "__main__":
    sys.exit(main())
