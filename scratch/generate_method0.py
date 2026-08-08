#!/usr/bin/env python3
"""
generate_method0.py -- Method 0: TDOA Stage-1 baseline (no Stage-2).

Method 0 = pure TDOA Stage-1 positioning, no fine-ranging, no EKF, no IMU.
  FinalPositionErrorM    ≡ PositionErrorM from tdoa_processed_dataset_fixed.csv
                           (TDOA-solved, calibrated to ~39m median via 100ns timing sigma)
  FinalLocalizationMode  ≡ "LORA_ONLY"  (kept for backward compatibility naming)
  Stage2Modality         = "NONE"
  ImuActive              = 0

pos_e source change (same as multimodal_generator.py):
  Previously: ml_training_dataset.csv PositionErrorM (sigmaM-cycling artifact,
              corr(sigmaM,mean)=1.0000, NLOS/LOS=0.999 — entirely synthetic).
  Now:        tdoa_processed_dataset_fixed.csv PositionErrorM (TDOA-solved, with
              100ns timing sigma calibrated to Podevijn Standard-mode reference).

Output (unchanged filenames):
  results/multimodal_M0_TWAN.csv
  results/multimodal_M0_TMESH.csv
  results/multimodal_M0_THYBRID.csv
"""
import sys, time
import numpy as np
import pandas as pd
from pathlib import Path

SRC_TDOA = "./results/tdoa_processed_dataset_fixed.csv"
SRC_ML   = "./results/ml_training_dataset.csv"
DST      = "./results"

def main():
    src_tdoa = Path(SRC_TDOA)
    src_ml   = Path(SRC_ML)
    if not src_tdoa.exists():
        print(f"ERROR: {src_tdoa} not found — run process_tdoa.py + fix_dataset.py first")
        sys.exit(1)

    print("Loading tdoa_processed_dataset_fixed.csv (pos_e source)...")
    df_tdoa = pd.read_csv(src_tdoa)

    print("Loading ml_training_dataset.csv (LoRa feature columns)...")
    df_ml = pd.read_csv(src_ml, nrows=500_000)

    topos = ["WAN", "MESH", "HYBRID"]

    for idx, topo in enumerate(topos, 1):
        out = Path(DST) / f"multimodal_M0_T{topo}.csv"
        print(f"\n[{idx}/3] Method 0 x {topo} -> {out.name}")

        # Get TDOA slice for this topology (provides pos_e + TdoaEligible + GwObsCount)
        tdoa_slice = df_tdoa[df_tdoa["Topology"] == topo].copy().reset_index(drop=True)

        # Merge: ml features + TDOA geometry/pos_e
        # Drop PositionErrorM from ml (sigmaM artifact) — keep TDOA version
        df_ml_no_pose = df_ml.drop(columns=["PositionErrorM"], errors="ignore")
        cols_to_drop_tdoa = [c for c in tdoa_slice.columns if c in df_ml_no_pose.columns]
        tdoa_clean = tdoa_slice.drop(columns=cols_to_drop_tdoa)

        chunk = pd.concat([df_ml_no_pose, tdoa_clean], axis=1)

        # Method 0: no Stage-2 at all. FinalPositionErrorM = TDOA Stage-1 pos_e.
        pos_e   = chunk["PositionErrorM"].values
        tdoa_ok = chunk["TdoaEligible"].values

        out_df = chunk.copy()
        out_df["Method"]               = 0
        out_df["Topology"]             = topo
        out_df["Stage2Modality"]       = "NONE"
        out_df["Stage2RangeEstimateM"] = float("nan")
        out_df["Stage2RangeErrorM"]    = float("nan")
        out_df["Stage2IsNLOS"]         = 0
        out_df["ImuActive"]            = 0
        out_df["TimeSinceLastLoRaFixS"]= 0.0
        out_df["ImuDriftEstimateM"]    = 0.0
        out_df["Stage1HandoffTimeS"]   = float("nan")
        out_df["FinalPositionErrorM"]  = pos_e
        out_df["FinalLocalizationMode"]= "LORA_ONLY"

        t0 = time.time()
        out_df.to_csv(out, index=False)
        el = time.time() - t0

        n_eligible = tdoa_ok.sum()
        n_converged = ((pos_e < 9000) & (tdoa_ok == 1)).sum()
        pe_med = np.median(pos_e[pos_e < 9000]) if (pos_e < 9000).any() else float("nan")
        print(f"  {len(out_df):,} rows in {el:.1f}s")
        print(f"  TdoaEligible: {n_eligible:,} ({n_eligible/len(out_df)*100:.1f}%)")
        print(f"  Converged solves: {n_converged:,}  |  Converged median pos_e: {pe_med:.2f}m")
        print(f"  Written: {out}")

    print("\nMethod 0 generation complete.")

if __name__ == "__main__":
    main()
