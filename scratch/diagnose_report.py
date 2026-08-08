#!/usr/bin/env python3
"""
diagnose_report.py
Three-part diagnostic answering the reviewer's three questions:
  Q1. Are Median/P90 genuinely recomputed per-cell, and do they land at
      exactly the M0 values (confirming the invariance theorem)?
  Q2. Why does UWB show higher handoff rate than 2.4GHz, and are
      Dist25/Dist50/Dist100 (Poisson facility distances) actually in use?
  Q3. What is the actual Stage-2 conditional accuracy for fused rows,
      and how does it compare to the 0.3-0.4m theoretical estimate?
"""
import numpy as np
import pandas as pd
from pathlib import Path

DST = Path("./results")
TOPOS   = ["WAN", "MESH", "HYBRID"]
METHODS = [0, 1, 2, 3, 4]

print("=" * 70)
print("Q1 — Per-cell Median and P90 (direct computation, no aggregation)")
print("=" * 70)

m0_stats = {}
for topo in TOPOS:
    path = DST / f"multimodal_M0_T{topo}.csv"
    if not path.exists():
        print(f"  MISSING: {path.name}")
        continue
    # Stream through; collect FinalPositionErrorM
    chunks = []
    for chunk in pd.read_csv(path, usecols=["FinalPositionErrorM"], chunksize=500_000):
        chunks.append(chunk["FinalPositionErrorM"].values)
    arr = np.concatenate(chunks)
    med = np.median(arr)
    p90 = np.percentile(arr, 90)
    m0_stats[topo] = (med, p90)
    print(f"  M0×{topo}  n={len(arr):,}  Median={med:.4f}m  P90={p90:.4f}m")

print()
print(f"  {'Cell':<18} {'n':>9} {'Fused%':>7} {'Median':>10} {'P90':>10} "
      f"{'ΔMed vs M0':>12} {'ΔP90 vs M0':>12}")
print(f"  {'-'*18} {'-'*9} {'-'*7} {'-'*10} {'-'*10} {'-'*12} {'-'*12}")

for method in [1, 2, 3, 4]:
    for topo in TOPOS:
        path = DST / f"multimodal_M{method}_T{topo}.csv"
        if not path.exists():
            print(f"  MISSING: {path.name}")
            continue
        chunks_err = []
        fused_count = 0
        total = 0
        for chunk in pd.read_csv(path, usecols=["FinalPositionErrorM","Stage2Modality"],
                                  chunksize=500_000):
            chunks_err.append(chunk["FinalPositionErrorM"].values)
            fused_count += (chunk["Stage2Modality"] != "NONE").sum()
            total += len(chunk)
        arr = np.concatenate(chunks_err)
        med = np.median(arr)
        p90 = np.percentile(arr, 90)
        fused_pct = fused_count / total * 100
        m0_med, m0_p90 = m0_stats.get(topo, (np.nan, np.nan))
        print(f"  M{method}×{topo:<14} {len(arr):>9,} {fused_pct:>6.2f}%"
              f" {med:>10.4f} {p90:>10.4f}"
              f" {med-m0_med:>+12.4f} {p90-m0_p90:>+12.4f}")

print()
print("  Expected by invariance theorem:")
print("  If all fused rows have pos_e <= 50m and FinalPositionErrorM <= pos_e,")
print("  and handoff rate < 50% (median) and < 90% (P90), then:")
print("  ΔMed = 0.0000m exactly,  ΔP90 = 0.0000m exactly — for every cell.")

print()
print("=" * 70)
print("Q2 — UWB vs 2.4GHz handoff rates; Dist25/50/100 in use?")
print("=" * 70)

for topo in TOPOS:
    print(f"\n  --- Topology: {topo} ---")
    # Load sample from M1 (2.4GHz) and M3 (UWB) for this topology
    for method, label in [(1, "2.4GHz RTT"), (3, "UWB")]:
        path = DST / f"multimodal_M{method}_T{topo}.csv"
        if not path.exists():
            continue
        # Read only first 200k rows for speed
        df = pd.read_csv(path, nrows=200_000,
                         usecols=["Stage2Modality", "Stage2IsNLOS",
                                  "Stage2RangeEstimateM", "FinalPositionErrorM",
                                  "TdoaEligible"])
        fused = df["Stage2Modality"] != "NONE"
        fused_pct = fused.mean() * 100
        # Check which distance column is being used by inspecting Stage2RangeEstimateM
        # for fused rows
        fused_rows = df[fused]
        if len(fused_rows) > 0:
            range_med = fused_rows["Stage2RangeEstimateM"].median()
            nlos_pct  = fused_rows["Stage2IsNLOS"].mean() * 100
        else:
            range_med = np.nan
            nlos_pct  = np.nan
        print(f"    M{method} ({label:<12}): Fused={fused_pct:.2f}%  "
              f"MedianRangeEstimate={range_med:.1f}m  NLOS%={nlos_pct:.1f}%")

    # Also check the actual Dist25/Dist50/Dist100 in one of the source files
    path_m1 = DST / f"multimodal_M1_T{topo}.csv"
    if path_m1.exists():
        df_check = pd.read_csv(path_m1, nrows=5000,
                               usecols=["Dist25","Dist50","Dist100"],
                               on_bad_lines='skip')
        print(f"    Dist25 median={df_check['Dist25'].median():.1f}m  "
              f"Dist50 median={df_check['Dist50'].median():.1f}m  "
              f"Dist100 median={df_check['Dist100'].median():.1f}m  "
              f"(confirming Poisson facility columns are present)")

print()
print("=" * 70)
print("Q3 — Stage-2 conditional accuracy for fused rows")
print("=" * 70)

for method, label in [(1, "2.4GHz RTT"), (2, "2.4GHz RTT+IMU"),
                      (3, "UWB"), (4, "UWB+IMU")]:
    for topo in TOPOS:
        path = DST / f"multimodal_M{method}_T{topo}.csv"
        if not path.exists():
            continue
        df = pd.read_csv(path, nrows=200_000,
                         usecols=["Stage2Modality", "Stage2RangeEstimateM",
                                  "Stage2RangeErrorM", "FinalPositionErrorM",
                                  "PositionErrorM"])
        fused = df["Stage2Modality"] != "NONE"
        if fused.sum() < 10:
            continue
        fd = df[fused]
        # Stage-2 range error (observed ranging noise)
        re_abs = fd["Stage2RangeErrorM"].abs()
        # Final position error improvement over Stage-1
        s1_err = fd["PositionErrorM"]
        fpe    = fd["FinalPositionErrorM"]
        print(f"  M{method}×{topo} ({label}):")
        print(f"    Stage2RangeError: median={re_abs.median():.3f}m  "
              f"P90={re_abs.quantile(0.90):.3f}m  mean={re_abs.mean():.3f}m")
        print(f"    Stage1 err (fused rows): median={s1_err.median():.2f}m")
        print(f"    FinalPosErr  (fused rows): median={fpe.median():.2f}m")
        print(f"    Improvement: {(s1_err.median()-fpe.median()):.2f}m "
              f"({(1-fpe.median()/s1_err.median())*100:.1f}%)")
        break  # one topology per method is enough to illustrate

print("\nDone.")
