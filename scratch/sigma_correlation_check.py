"""
sigma_correlation_check.py
One-line check: does row_index % 4 == 0 (sigmaM=0 stratum) predict RangeErrorM==0?

SIGMA_M_LEVELS = {0.0, 50.0, 200.0, 500.0}, cycling via i % 4.
Row index 0,4,8,...  => sigmaM=0.0  => expect RangeErrorM=0
Row index 1,5,9,...  => sigmaM=50.0 => expect RangeErrorM != 0
Row index 2,6,10,... => sigmaM=200.0
Row index 3,7,11,... => sigmaM=500.0
"""
import pandas as pd
import numpy as np

df = pd.read_csv("results/ml_training_dataset.csv", nrows=200_000)

# Reconstruct which SIGMA_M stratum each row belongs to
# (row_index in the original file = df.index, since we start from row 0)
df["sigma_stratum"] = df.index % 4          # 0→sigmaM=0, 1→50, 2→200, 3→500
df["sigma_zero"]    = (df["sigma_stratum"] == 0)
df["range_zero"]    = (df["RangeErrorM"] == 0.0)

print("=" * 60)
print("SIGMA_M_LEVELS cycling vs RangeErrorM==0")
print("=" * 60)
print()
print("Crosstab: sigma_zero (stratum==0) × range_zero")
ct = pd.crosstab(df["sigma_zero"], df["range_zero"],
                 rownames=["sigma_zero (i%4==0)"],
                 colnames=["RangeErrorM==0"])
print(ct)
print()

# Perfect correlation test
both_zero      = (df["sigma_zero"] & df["range_zero"]).sum()
sigma_only     = (df["sigma_zero"] & ~df["range_zero"]).sum()
range_only     = (~df["sigma_zero"] & df["range_zero"]).sum()
neither        = (~df["sigma_zero"] & ~df["range_zero"]).sum()
total          = len(df)

print(f"sigma_zero=T, range_zero=T  (both match):  {both_zero:>7,}  ({both_zero/total*100:.2f}%)")
print(f"sigma_zero=T, range_zero=F  (sigma but not range): {sigma_only:>7,}  ({sigma_only/total*100:.2f}%)")
print(f"sigma_zero=F, range_zero=T  (range but not sigma): {range_only:>7,}  ({range_only/total*100:.2f}%)")
print(f"sigma_zero=F, range_zero=F  (neither):     {neither:>7,}  ({neither/total*100:.2f}%)")
print()

# If the mechanism is correct:
# sigma_only and range_only should both be ~0
agreement_pct = (both_zero + neither) / total * 100
print(f"Agreement rate: {agreement_pct:.4f}%")
if sigma_only == 0 and range_only == 0:
    print("VERDICT: Perfect 1:1 correspondence confirmed.")
    print("  RangeErrorM==0  ↔  i%4==0 (sigmaM=0 stratum) — exact, deterministic.")
    print("  This is the SIGMA_M_LEVELS cycling, NOT a C++ degenerate-path artifact.")
elif agreement_pct > 99.9:
    print(f"VERDICT: Near-perfect match ({agreement_pct:.3f}%). ")
    print(f"  Exceptions: {sigma_only} sigma-only, {range_only} range-only rows.")
    print("  Effectively confirms the SIGMA_M_LEVELS mechanism.")
else:
    print(f"VERDICT: Imperfect match ({agreement_pct:.2f}%).")
    print("  The two conditions are NOT perfectly correlated — original degenerate-path")
    print("  diagnosis may be at least partially correct.")

print()
print("=" * 60)
print("Per-stratum RangeErrorM and PositionErrorM statistics")
print("=" * 60)
SIGMA_VALUES = {0: 0.0, 1: 50.0, 2: 200.0, 3: 500.0}
for s in range(4):
    sub = df[df["sigma_stratum"] == s]
    re_zero = (sub["RangeErrorM"] == 0).mean() * 100
    pe_zero = (sub["PositionErrorM"] < 0.01).mean() * 100
    pe_med  = sub["PositionErrorM"].median()
    re_med  = sub["RangeErrorM"].abs().median()
    print(f"  Stratum {s} (sigmaM={SIGMA_VALUES[s]:5.1f}m): "
          f"n={len(sub):,}  RangeErr==0: {re_zero:.1f}%  "
          f"PosErr<0.01: {pe_zero:.1f}%  "
          f"PosErr_med={pe_med:.2f}m  |RangeErr|_med={re_med:.2f}m")
