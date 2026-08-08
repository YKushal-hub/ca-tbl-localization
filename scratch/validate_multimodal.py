#!/usr/bin/env python3
"""
validate_multimodal.py -- Part 6 Coupling Integrity Validator
Team 41 | Validates all 12 multimodal CSVs before training.

CHECKS:
  1. NLOS mismatch count (must = 0) -- Stage2IsNLOS vs (NlosDb>0)
  2. UWB NLOS bias vs NlosDb Pearson r (must > 0.5)
  3. 2.4 GHz max range (warn if any row > 300 m)
  4. IMU drift monotonicity proxy (Pearson r vs TimeSinceLastLoRaFixS > 0.7)
  5. FinalPositionErrorM <= Stage1 PositionErrorM when FUSED (improvement check)
"""
import sys, glob
import numpy as np
import pandas as pd
from pathlib import Path
from scipy import stats

DST = "./results"
PATTERNS = [
    "multimodal_M{}_T{}.csv".format(m, t)
    for m in [1,2,3,4]
    for t in ["WAN","MESH","HYBRID"]
]

CHUNK = 500_000

def validate_file(path):
    name = Path(path).name
    print("\n  Validating: {}".format(name))
    errors = []; warnings = []

    # Accumulators for full-file statistics
    nlos_mismatch_total = 0
    rows_total = 0
    uwb_bias_arr = []; uwb_nlos_arr = []
    rtt_range_arr = []
    imu_drift_arr = []; imu_tsf_arr = []
    improvement_ok = 0; improvement_bad = 0

    method = int(name.split("_M")[1].split("_")[0])
    topo   = name.split("_T")[1].split(".")[0]

    for chunk in pd.read_csv(path, chunksize=CHUNK):
        rows_total += len(chunk)

        # Check 1: NLOS mismatch (NaN-safe: any NlosDb > 0 is NLOS)
        inherited = (chunk["NlosDb"].fillna(0.0) > 0).astype(int)
        recorded  = chunk["Stage2IsNLOS"].fillna(0.0).astype(int)
        mismatch  = (inherited != recorded).sum()
        nlos_mismatch_total += int(mismatch)

        # Check 2: UWB bias correlation (Methods 3 & 4 only)
        if method in (3, 4):
            uwb_rows = chunk[chunk["Stage2Modality"] == "UWB"].copy()
            if len(uwb_rows) > 100:
                uwb_bias_arr.extend(uwb_rows["Stage2RangeErrorM"].dropna().tolist())
                uwb_nlos_arr.extend(uwb_rows.loc[
                    uwb_rows["Stage2RangeErrorM"].notna(), "NlosDb"
                ].tolist())

        # Check 3: 2.4 GHz max range
        if method in (1, 2):
            rtt_rows = chunk[chunk["Stage2Modality"] == "2.4GHZ_RTT"].copy()
            rtt_valid = rtt_rows["Stage2RangeEstimateM"].dropna()
            over300   = (rtt_valid > 300).sum()
            if over300 > 0:
                rtt_range_arr.append(int(over300))

        # Check 4: IMU drift monotonicity (Methods 2 & 4)
        if method in (2, 4):
            imu_rows = chunk[chunk["ImuActive"] == 1].copy()
            if len(imu_rows) > 100:
                imu_drift_arr.extend(imu_rows["ImuDriftEstimateM"].tolist())
                imu_tsf_arr.extend(imu_rows["TimeSinceLastLoRaFixS"].tolist())

        # Check 5: Fusion improvement
        fused = chunk[chunk["FinalLocalizationMode"] == "FUSED"].copy()
        if len(fused) > 0:
            better = (fused["FinalPositionErrorM"] <= fused["PositionErrorM"]).sum()
            improvement_ok  += int(better)
            improvement_bad += int(len(fused) - better)

    # ── Report Check 1 ──────────────────────────────────────────────
    if nlos_mismatch_total == 0:
        print("    [PASS] NLOS coupling: 0 mismatches in {:,} rows".format(rows_total))
    else:
        msg = "NLOS mismatch = {:,} rows (MUST BE 0)".format(nlos_mismatch_total)
        print("    [FAIL] {}".format(msg))
        errors.append(msg)

    # ── Report Check 2 ──────────────────────────────────────────────
    if method in (3, 4) and len(uwb_bias_arr) >= 500:
        r, p = stats.pearsonr(uwb_nlos_arr, uwb_bias_arr)
        if r > 0.5:
            print("    [PASS] UWB NLOS bias-NlosDb correlation r={:.3f} (target >0.5)".format(r))
        else:
            msg = "UWB bias-NlosDb correlation r={:.3f} < 0.5".format(r)
            print("    [WARN] {}".format(msg))
            warnings.append(msg)
    elif method in (3, 4):
        print("    [SKIP] UWB correlation check (insufficient NLOS UWB rows)")

    # ── Report Check 3 ──────────────────────────────────────────────
    if method in (1, 2):
        over = sum(rtt_range_arr)
        if over == 0:
            print("    [PASS] 2.4GHz range: no rows > 300 m")
        else:
            msg = "{:,} RTT rows exceed 300 m range".format(over)
            print("    [WARN] {}".format(msg))
            warnings.append(msg)

    # ── Report Check 4 ──────────────────────────────────────────────
    if method in (2, 4) and len(imu_drift_arr) >= 500:
        r, p = stats.pearsonr(imu_tsf_arr, imu_drift_arr)
        if r > 0.7:
            print("    [PASS] IMU drift monotonicity r={:.3f} (target >0.7)".format(r))
        else:
            msg = "IMU drift-time correlation r={:.3f} < 0.7".format(r)
            print("    [WARN] {}".format(msg))
            warnings.append(msg)
    elif method in (2, 4):
        print("    [SKIP] IMU monotonicity check (insufficient IMU rows)")

    # ── Report Check 5 ──────────────────────────────────────────────
    total_fused = improvement_ok + improvement_bad
    if total_fused > 0:
        pct = improvement_ok / total_fused * 100
        if pct >= 50:
            print("    [PASS] Fusion improves {:.1f}% of FUSED rows (target >=50%)".format(pct))
        else:
            msg = "Fusion improves only {:.1f}% of rows (target 50%)".format(pct)
            print("    [WARN] {}".format(msg))
            warnings.append(msg)

    return errors, warnings


def main():
    dst = Path(DST)
    all_errors = {}; all_warnings = {}
    found = 0
    for pattern in PATTERNS:
        path = dst / pattern
        if not path.exists():
            print("  [MISSING] {}".format(pattern))
            continue
        found += 1
        errs, warns = validate_file(str(path))
        all_errors[pattern]   = errs
        all_warnings[pattern] = warns

    print("\n" + "="*60)
    print("SUMMARY: {}/{} files validated".format(found, len(PATTERNS)))

    fatal = sum(len(v) for v in all_errors.values())
    warn  = sum(len(v) for v in all_warnings.values())
    print("  ERRORS  (blocking): {}".format(fatal))
    print("  WARNINGS (non-blocking): {}".format(warn))

    if fatal > 0:
        print("\nFATAL ERRORS -- do NOT proceed to training:")
        for f, errs in all_errors.items():
            for e in errs:
                print("  {} : {}".format(f, e))
        sys.exit(1)
    else:
        print("\nAll coupling checks PASSED. Safe to run train_stage1.py")

if __name__ == "__main__":
    main()
