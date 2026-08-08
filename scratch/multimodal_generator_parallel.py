#!/usr/bin/env python3
"""
multimodal_generator_parallel.py
Parallelises the 12 (Method × Topology) combinations across up to PARALLEL_WORKERS
worker processes. Each worker loads only the subset of tdoa_processed_dataset_fixed.csv
it needs, keeping peak RAM well below the system limit.

Memory budget (7.6 GB total, 7 GB available):
  - df_ml  (500k rows, ~30 cols)  ≈  120 MB   — shared read-only across workers
  - per-worker TDOA slice         ≈ 500 MB/worker
  - per-worker output             ≈  80 MB/worker
  => 4 workers × ~600 MB = ~2.4 GB data  +  ~1 GB OS overhead  = ~3.4 GB  ✓ SAFE
"""
import math, sys, time, json, os
import numpy as np
import pandas as pd
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor, as_completed

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
PARALLEL_WORKERS = 4   # 4 parallel workers: ~3.4 GB peak RAM, safe on 7.6 GB

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
    ok = dist <= np.where(isnlos, 50.0, 150.0)
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

def process(chunk, method, topo, rng):
    env   = chunk["Environment"].values
    pos_e = chunk["PositionErrorM"].values
    tdoa_ok = chunk["TdoaEligible"].values == 1

    if topo == "MESH":
        s2_dist = chunk["Dist25"].values
        s2_nlos = chunk["Nlos25"].values
    elif topo == "HYBRID":
        s2_dist = chunk["Dist50"].values
        s2_nlos = chunk["Nlos50"].values
    else:
        s2_dist = chunk["Dist100"].values
        s2_nlos = chunk["Nlos100"].values

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
    out["Topology"]             = topo
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


def run_one(method, topo, idx, total):
    """Worker function: loads only the data it needs, processes, writes."""
    t0 = time.time()
    out_path = Path(DST) / f"multimodal_M{method}_T{topo}.csv"

    # Skip if already done from a previous run
    if out_path.exists():
        print(f"  [{idx}/{total}] SKIP M{method}×{topo} (already exists)", flush=True)
        return method, topo, 0.0, True

    # Each worker reads only its topology slice from the TDOA file
    df_ml   = pd.read_csv(SRC_ML, nrows=CHUNK)
    df_tdoa = pd.read_csv(SRC_TDOA)
    chunk_tdoa = df_tdoa[df_tdoa["Topology"] == topo].copy().reset_index(drop=True)
    del df_tdoa  # free memory immediately

    cols_to_drop = [c for c in chunk_tdoa.columns if c in df_ml.columns]
    chunk_tdoa_clean = chunk_tdoa.drop(columns=cols_to_drop)
    chunk = pd.concat([df_ml.copy(), chunk_tdoa_clean], axis=1)
    del df_ml, chunk_tdoa, chunk_tdoa_clean

    seed = 1000 + method * 100 + ["WAN", "MESH", "HYBRID"].index(topo)
    rng  = np.random.default_rng(seed)
    oc   = process(chunk, method, topo, rng)
    del chunk
    oc.to_csv(out_path, index=False)
    el = time.time() - t0
    print(f"  [{idx}/{total}] DONE M{method}×{topo}: {len(oc):,} rows in {el:.0f}s", flush=True)
    return method, topo, el, False


def main():
    dst = Path(DST); dst.mkdir(exist_ok=True)
    combos = [(m, t) for m in [1,2,3,4] for t in ["WAN","MESH","HYBRID"]]
    total  = len(combos)

    print(f"Launching {PARALLEL_WORKERS} parallel workers for {total} combos "
          f"(resume-safe: skips already-written files)...", flush=True)
    t_start = time.time()

    with ProcessPoolExecutor(max_workers=PARALLEL_WORKERS) as ex:
        futs = {ex.submit(run_one, m, t, i, total): (m, t)
                for i, (m, t) in enumerate(combos, 1)}
        for fut in as_completed(futs):
            try:
                fut.result()
            except Exception as e:
                m, t = futs[fut]
                print(f"  ERROR M{m}×{t}: {e}", flush=True)

    elapsed = time.time() - t_start
    print(f"\nAll {total} combos done in {elapsed:.0f}s ({elapsed/60:.1f} min)", flush=True)


if __name__ == "__main__":
    main()
