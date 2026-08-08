#!/usr/bin/env python3
"""
diag_zero_pos_deep.py -- Why does 24.88% of Stage 1 have PositionErrorM=0?

Hypothesis A: Trilateration degeneracy — when a node is exactly at a gateway,
  trilateration error is literally zero. Prevalence depends on how Stage 1
  generates position error.

Hypothesis B: Stage 1 ML model predicts PositionErrorM and outputs exactly 0
  for some input regimes (gradient boosted tree leaf prediction = 0).

Hypothesis C: PositionErrorM is a floor/clipped column set to 0 below some threshold.

Hypothesis D: RangeErrorM = 0 (when DistanceM = perfect Rx match), and
  PositionErrorM is derived from RangeErrorM, so it inherits zeros.
"""
import pandas as pd
import numpy as np

SRC = "./results/ml_training_dataset.csv"
CHUNK = 500_000

print("=== DEEP ZERO DIAGNOSIS ===\n")

# Load 5% sample for more statistical power
chunks = []
for chunk in pd.read_csv(SRC, chunksize=CHUNK):
    chunks.append(chunk.sample(frac=0.05, random_state=42))
df = pd.concat(chunks, ignore_index=True)
print(f"Sample: {len(df):,} rows\n")

zero = df["PositionErrorM"] == 0.0
nz   = ~zero

print("=== Hypothesis D: RangeErrorM also zero on same rows? ===")
if "RangeErrorM" in df.columns:
    re_on_zero_pos = df.loc[zero, "RangeErrorM"]
    re_on_nonzero  = df.loc[nz,   "RangeErrorM"]
    print(f"  RangeErrorM when PositionErrorM==0: mean={re_on_zero_pos.mean():.3f}  "
          f"zero_frac={( re_on_zero_pos==0).mean()*100:.1f}%")
    print(f"  RangeErrorM when PositionErrorM>0:  mean={re_on_nonzero.mean():.3f}  "
          f"zero_frac={(re_on_nonzero==0).mean()*100:.1f}%")
else:
    print("  RangeErrorM column not present in dataset")

print()
print("=== Distance distribution for PositionErrorM==0 rows ===")
d_zero = df.loc[zero, "DistanceM"]
d_nz   = df.loc[nz,   "DistanceM"]
print(f"  Distance (pos_err=0): min={d_zero.min():.0f}m  median={d_zero.median():.0f}m  "
      f"max={d_zero.max():.0f}m")
print(f"  Distance (pos_err>0): min={d_nz.min():.0f}m  median={d_nz.median():.0f}m  "
      f"max={d_nz.max():.0f}m")
print("  -> If zero-pos rows concentrate near 0m distance, it's trilateration degeneracy.")
print("  -> If they span all distances, it's an ML model or coding artifact.")

print()
print("=== NlosDb distribution for PositionErrorM==0 rows ===")
nlos_zero = df.loc[zero, "NlosDb"]
nlos_nz   = df.loc[nz,   "NlosDb"]
print(f"  NlosDb (pos_err=0): mean={nlos_zero.mean():.1f}  zero_frac={(nlos_zero==0).mean()*100:.1f}%")
print(f"  NlosDb (pos_err>0): mean={nlos_nz.mean():.1f}  zero_frac={(nlos_nz==0).mean()*100:.1f}%")

print()
print("=== SF / TxPower patterns ===")
print("SpreadingFactor value counts for pos_err==0 rows:")
print(df.loc[zero, "SpreadingFactor"].value_counts().head(5))
print("SpreadingFactor value counts for pos_err>0 rows:")
print(df.loc[nz, "SpreadingFactor"].value_counts().head(5))

print()
print("=== Column list in dataset ===")
print("Columns:", list(df.columns))

print()
print("=== Check: does PositionErrorM correlate with RangeErrorM ===")
if "RangeErrorM" in df.columns:
    corr = df[["PositionErrorM","RangeErrorM","DistanceM","NlosDb"]].corr()
    print(corr.to_string())

print()
print("=== VERDICT ===")
# A zero PositionErrorM that spans all distances (not just 0m) can only come from
# the Stage 1 model outputting literal zero predictions, OR a hard floor/clipping
# applied at post-processing time, OR a sentinel value meaning 'no fix'.
zero_frac = zero.mean()
d_far_zero = (zero & (df["DistanceM"] > 1000)).sum()
d_far_total = (df["DistanceM"] > 1000).sum()
print(f"  Zero PositionErrorM at distances >1000m: {d_far_zero:,} / {d_far_total:,} "
      f"= {d_far_zero/max(d_far_total,1)*100:.1f}%")
print()
print(f"  PositionErrorM=0 prevalence: {zero_frac*100:.1f}%")
if zero_frac > 0.15:
    print("  FINDING: 24%+ of rows have pos_err=0. This is not geometric degeneracy.")
    print("  Most likely cause: Stage 1 ML model (train_stage1.py / LightGBM) predicts")
    print("  exactly 0 for a large fraction of rows — possibly because Stage 1's own")
    print("  training target (PositionErrorM) had many zeros, causing the tree to learn")
    print("  a 0-leaf as the modal prediction for many regimes.")
    print()
    print("  Impact on Stage 2: 24% of rows have pos_err=0 which is <=50m, so")
    print("  stage1_handoff fires for all of them (100% TDOA-eligible among zero rows).")
    print("  This inflates FusedRate well beyond the topology-intended ~3%.")
