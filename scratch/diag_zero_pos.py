#!/usr/bin/env python3
"""
diag_zero_pos.py -- Investigate PositionErrorM=0 rows seen in Diag 1 FUSED sample.
Checks: are there zero-PositionErrorM rows in the base dataset?
        If so, is it physically meaningful or a generator artifact?
"""
import pandas as pd
import numpy as np

SRC   = "./results/ml_training_dataset.csv"
CHUNK = 500_000

print("=== Investigating PositionErrorM = 0 in base dataset ===\n")

chunks = []
for chunk in pd.read_csv(SRC, chunksize=CHUNK):
    chunks.append(chunk.sample(frac=0.02, random_state=42))
df = pd.concat(chunks, ignore_index=True)

pos = df["PositionErrorM"]
zero = pos == 0.0
near_zero = pos < 0.5

print(f"Total rows sampled: {len(df):,}")
print(f"PositionErrorM == 0.0: {zero.sum():,} ({zero.mean()*100:.2f}%)")
print(f"PositionErrorM < 0.5m: {near_zero.sum():,} ({near_zero.mean()*100:.2f}%)")
print()
print("PositionErrorM distribution:")
print(f"  Min:    {pos.min():.4f}m")
print(f"  P0.1%:  {pos.quantile(0.001):.2f}m")
print(f"  P1%:    {pos.quantile(0.01):.2f}m")
print(f"  Median: {pos.median():.1f}m")
print(f"  Mean:   {pos.mean():.1f}m")
print(f"  P90:    {pos.quantile(0.90):.1f}m")
print(f"  Max:    {pos.max():.1f}m")
print()

if zero.any():
    print("Sample rows where PositionErrorM == 0:")
    cols = [c for c in ["DistanceM","Environment","NlosDb","ShadowingDb",
                         "TxPowerDbm","SpreadingFactor","PositionErrorM"] if c in df.columns]
    print(df[zero][cols].head(10).to_string())
    print()
    print("VERDICT: Zero PositionErrorM is", end=" ")
    if zero.mean() < 0.001:
        print("RARE (<0.1%) — likely edge case in trilateration when geometry is perfect.")
        print("  Not a systematic artifact. Acceptable.")
    elif zero.mean() < 0.01:
        print("UNCOMMON (0.1-1%) — investigate if these correspond to specific environments.")
        print(f"  Environment breakdown of zero rows:")
        if "Environment" in df.columns:
            print(df[zero]["Environment"].value_counts().head(10))
    else:
        print("COMMON (>1%) — likely a generator artifact where exact node placement")
        print("  coincides with gateway, producing trivial zero-error trilateration.")
        print("  This inflates the FUSED row count at pos_err=0 → stage1_handoff always fires.")
else:
    print("No zero PositionErrorM rows in sample. The zeros seen in Diag 1 FUSED")
    print("rows may have been PositionErrorM=0.0 as a FUSED row fill value, not base data.")
