#!/usr/bin/env python3
"""
multimodal_generator.py -- Multi-Modal Localization Dataset Generator
Team 41 | Methods 1-4 x 3 Topologies x 3 Densities | Part 0-4 physics grounding

Changes from previous version:
  - pos_e source: switched from ml_training_dataset PositionErrorM (sigmaM-cycling
    artifact) to tdoa_processed_dataset_fixed PositionErrorM (TDOA-solved, calibrated
    to validated Podevijn 42m Standard-mode reference via 100ns timing sigma).
    Fix 1 (RangeErrorM decontamination) is now obsolete: TDOA pos_e bypasses the
    sigmaM problem entirely.
  - Fix 2: UWB range ceiling corrected from 150m LOS / 50m NLOS to 100m LOS / 30m NLOS
    (established, citation-backed figures).
  - Fix 3: Independent 3x3 topology x density grid. Dist columns (Dist100/Dist50/Dist25)
    are swept across all 3 topologies instead of being locked per topology.
    Output: 36 CSVs  (4 methods x 3 topologies x 3 densities).
"""
import math, sys, time, json
import numpy as np
import pandas as pd
from pathlib import Path

LORA_PLE = {0:2.20,1:2.30,2:2.75,3:2.39,4:2.45,5:1.80,6:2.80,7:3.50,8:1.30}
RF24_TX_DBM  = 8.0
RF24_SENS_DBM = -90.0
RF24_FSPL_1M  = 40.05
UWB_LOS_BIAS = 0.030; UWB_LOS_STD = 0.100; UWB_NLOS_STD_RATIO = 0.60
UWB_SLOPE = 0.139; UWB_INTERCEPT = -2.139
IMU_SIGMA_A    = 0.00294
IMU_GYRO_DPH   = 5.0
IMU_PEDESTRIAN = 1.4
BASE_INTERVAL_S = 15.0
CHUNK  = 500_000
SRC_ML   = "./results/ml_training_dataset.csv"
SRC_TDOA = "./results/tdoa_processed_dataset_fixed.csv"
DST      = "./results"

# Fix 3: Density tiers — each maps to a (Dist_col, Nlos_col) pair.
# Topology and density are independent infrastructure decisions; all 3 density
# tiers are swept across all 3 topologies.
# D100 → dense facility deployment (median Stage-2 dist ≈ 47m)
# D50  → medium facility deployment (median Stage-2 dist ≈ 67m)
# D25  → sparse facility deployment (median Stage-2 dist ≈ 95m)
DENSITIES = {
    "D100": ("Dist100", "Nlos100"),
    "D50":  ("Dist50",  "Nlos50"),
    "D25":  ("Dist25",  "Nlos25"),
}

def ple24(n):
    return n * (1.0 + 0.15 * (n - 1.3) / (3.7 - 1.3))

def rtt_range(dist, env, nlos, rng):
    ln  = np.array([LORA_PLE.get(int(e), 2.20) for e in env])
    n24 = np.array([ple24(x) for x in ln])
    pl  = RF24_FSPL_1M + 10.0 * n24 * np.log10(np.maximum(dist, 1.0))
    pl += np.where(nlos > 0, nlos * 1.3, 0.0)
    rssi = RF24_TX_DBM - pl
    ok   = rssi > RF24_SENS_DBM
    mg   = rssi - RF24_SENS_DBM
    std  = np.where(mg > 10, 0.5, np.maximum(0.3, 2.0 - mg * 0.15))
    noise = rng.normal(0.0, std, len(dist))
    return np.where(ok, dist + noise, np.nan), np.where(ok, noise, np.nan), ok

def uwb_range(dist, nlos, rng):
    isnlos = nlos > 0
    # Fix 2: Corrected UWB range ceiling.
    # Previous: 150m LOS / 50m NLOS (overly optimistic).
    # Now:      100m LOS / 30m NLOS (established, citation-backed figures).
    ok = dist <= np.where(isnlos, 30.0, 100.0)
    nb = np.maximum(0.05, UWB_SLOPE * nlos + UWB_INTERCEPT)
    los_n  = rng.normal(0, UWB_LOS_STD, len(dist))
    nlos_n = rng.normal(0, nb * UWB_NLOS_STD_RATIO, len(dist))
    noise  = np.where(isnlos, nlos_n, los_n)
    bias   = np.where(isnlos, nb, UWB_LOS_BIAS)
    return np.where(ok, dist+bias+noise, np.nan), np.where(ok, bias+noise, np.nan), ok

def imu_drift(t):
    t = np.maximum(t, 0.0)
    acc = IMU_SIGMA_A * (t ** 1.5) / math.sqrt(3.0)
    gbr = IMU_GYRO_DPH * math.pi / 180.0 / 3600.0
    ct  = IMU_PEDESTRIAN * t * np.sin(gbr * t)
    return np.sqrt(acc**2 + ct**2)

def ekf_fuse(lora_err, s2_err, tsf):
    imu = np.maximum(imu_drift(tsf), 0.5)
    rl  = np.maximum(lora_err, 1.0)
    ok2 = ~np.isnan(s2_err)
    rs2 = np.where(ok2, np.abs(s2_err) + 0.3, 1e6)
    vi = imu**2; vl = rl**2; vs = rs2**2
    vli = (vi * vl) / (vi + vl + 1e-9)
    v3  = (vli * vs) / (vli + vs + 1e-9)
    return np.sqrt(np.where(ok2, v3, vli))

def process(chunk, method, density, rng):
    """
    Generate one (method, density) slice of the multimodal dataset.

    pos_e source: chunk["PositionErrorM"] is the TDOA-solved position error
    (from tdoa_processed_dataset_fixed.csv, calibrated to ~39m median after
    100ns timing sigma fix). Rows where TDOA did not converge have pos_e=9999m
    and will not pass the handoff gate.

    density: key into DENSITIES dict, selects Dist*/Nlos* column pair.
    topology is now purely a propagation/routing property of the TDOA dataset
    and does NOT determine Stage-2 facility density.
    """
    env     = chunk["Environment"].values
    pos_e   = chunk["PositionErrorM"].values        # TDOA-based pos_e (fixed)
    tdoa_ok = chunk["TdoaEligible"].values == 1

    # Fix 3: density is independent of topology
    dist_col, nlos_col = DENSITIES[density]
    s2_dist = chunk[dist_col].values
    s2_nlos = chunk[nlos_col].values

    # Handoff fires when Stage-1 TDOA achieved < 50m AND the node was TDOA-eligible.
    # With σ_t=400ns (CRLB-grounded, SNR=20.8dB, 125kHz BW):
    #   MESH median pos_e ≈ 134m → ~10-12% of converged MESH rows pass (P10 ≤ 50m)
    #   WAN/HYBRID median pos_e ≈ 234-254m → ~3-5% pass
    # This is the correct finding for a 10-gateway/2500km² sparse deployment:
    # Stage-2 handoff benefits only the minority of nodes with excellent TDOA geometry.
    hok = (pos_e <= 50.0) & tdoa_ok
    rate = tdoa_ok.mean()
    if rate < 0.01: rate = 0.01
    interval = min(BASE_INTERVAL_S / rate, 120.0)
    ht  = np.where(hok, rng.exponential(interval, len(pos_e)), np.nan)

    if method in (1, 2):
        re, rerr, s2ok = rtt_range(s2_dist, env, s2_nlos, rng)
        mod = "2.4GHZ_RTT"
    else:
        re, rerr, s2ok = uwb_range(s2_dist, s2_nlos, rng)
        mod = "UWB"

    act  = hok & s2ok
    re   = np.where(act, re,   np.nan)
    rerr = np.where(act, rerr, np.nan)

    n = len(chunk)
    iact = np.zeros(n, bool); dr = np.zeros(n); tsf = np.zeros(n)
    if method in (2, 4):
        iact = hok
        tsf  = np.where(iact, rng.exponential(interval, n), 0.0)
        dr   = np.where(iact, imu_drift(tsf), 0.0)

    if method in (2, 4):
        nan_rerr = np.full_like(rerr, np.nan)
        fpe_full = ekf_fuse(pos_e, rerr,     tsf)
        fpe_imu  = ekf_fuse(pos_e, nan_rerr, tsf)
        fpe = np.where(act, fpe_full, np.where(iact & ~act, fpe_imu, pos_e))
    else:
        lv   = np.maximum(pos_e, 1.0)**2
        sv   = np.where(~np.isnan(rerr), (np.abs(rerr)+0.3)**2, 1e12)
        fpe  = np.where(act, np.sqrt((lv*sv)/(lv+sv+1e-9)), pos_e)

    mode = np.where(act, "FUSED", np.where(iact & ~act, "IMU_ONLY", "LORA_ONLY"))

    out = chunk.copy()
    out["Method"]               = method
    out["Density"]              = density
    out["Stage2DistCol"]        = dist_col      # which Dist column was used
    out["Stage1HandoffTimeS"]   = ht
    out["Stage2Modality"]       = np.where(act, mod, "NONE")
    out["Stage2RangeEstimateM"] = re
    out["Stage2RangeErrorM"]    = rerr
    out["Stage2IsNLOS"]         = (s2_nlos > 0).astype(int)
    out["ImuActive"]            = iact.astype(int)
    out["TimeSinceLastLoRaFixS"]= tsf
    out["ImuDriftEstimateM"]    = dr
    out["FinalPositionErrorM"]  = fpe
    out["FinalLocalizationMode"]= mode
    return out

def main():
    src_ml   = Path(SRC_ML)
    src_tdoa = Path(SRC_TDOA)
    dst = Path(DST); dst.mkdir(exist_ok=True)

    print("Loading ML features (first 500k)...")
    df_ml = pd.read_csv(src_ml, nrows=CHUNK)

    print("Loading TDOA geometry + pos_e (all topologies)...")
    df_tdoa = pd.read_csv(src_tdoa)

    # Fix — pos_e source switch:
    # Drop PositionErrorM from the ml dataset so the TDOA-based version
    # (from tdoa_processed_dataset_fixed.csv) survives the concat.
    # The ml PositionErrorM was driven entirely by the sigmaM-cycling test harness
    # (NLOS/LOS ratio = 0.999 within each stratum, corr(sigmaM,meanPosErr) = 1.000000).
    df_ml_no_pose = df_ml.drop(columns=["PositionErrorM"], errors="ignore")

    # Fix 3: 3 topologies x 3 densities x 4 methods = 36 output files
    topologies = ["WAN", "MESH", "HYBRID"]
    methods    = [1, 2, 3, 4]
    combos     = [(m, t, d) for m in methods for t in topologies for d in DENSITIES]

    # Cache per-topology TDOA slice (avoid re-filtering 1.5M rows 12× per topology)
    tdoa_by_topo = {}
    for topo in topologies:
        slice_tdoa = df_tdoa[df_tdoa["Topology"] == topo].copy().reset_index(drop=True)
        # Drop columns that already exist in df_ml_no_pose (except PositionErrorM
        # which was explicitly removed from df_ml — we want the TDOA version).
        cols_to_drop = [c for c in slice_tdoa.columns if c in df_ml_no_pose.columns]
        tdoa_by_topo[topo] = slice_tdoa.drop(columns=cols_to_drop)

    print(f"Generating {len(combos)} datasets (M1-4 x 3 topologies x 3 densities)...\n")

    for i, (method, topo, density) in enumerate(combos, 1):
        out_path = dst / f"multimodal_M{method}_T{topo}_{density}.csv"
        print(f"[{i:02d}/{len(combos)}] M{method} x {topo} x {density} -> {out_path.name}")

        chunk = pd.concat([df_ml_no_pose.copy(), tdoa_by_topo[topo]], axis=1)

        seed = 1000 + method * 100 + topologies.index(topo) * 10 + list(DENSITIES).index(density)
        rng  = np.random.default_rng(seed)

        t0 = time.time()
        oc = process(chunk, method, density, rng)
        oc.to_csv(out_path, index=False)
        el = time.time() - t0
        n_fused = (oc["FinalLocalizationMode"] == "FUSED").sum()
        pct_fused = n_fused / len(oc) * 100
        print(f"   {len(oc):,} rows in {el:.1f}s  |  fused={n_fused:,} ({pct_fused:.1f}%)")

    print("\nDone.")

if __name__ == "__main__":
    main()
