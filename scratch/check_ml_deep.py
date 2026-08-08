import pandas as pd
import numpy as np

df = pd.read_csv("results/ml_training_dataset.csv", nrows=200000)
print("All columns:", list(df.columns))
print()
print("PositionErrorM distribution (200k rows):")
print(df["PositionErrorM"].describe(percentiles=[.10,.25,.50,.75,.90,.95,.99]))
print()
print("Correlation PositionErrorM ~ DistanceM:", df["PositionErrorM"].corr(df["DistanceM"]))
print("Correlation PositionErrorM ~ TotalLossDb:", df["PositionErrorM"].corr(df["TotalLossDb"]))
print()

near_zero = df[df["PositionErrorM"] < 0.01]
positive  = df[df["PositionErrorM"] >= 1.0]

print(f"Near-zero rows ({len(near_zero)} = {len(near_zero)/len(df)*100:.1f}%):")
print(f"  DistanceM  median={near_zero['DistanceM'].median():.0f}m  mean={near_zero['DistanceM'].mean():.0f}m")
print(f"  TotalLossDb median={near_zero['TotalLossDb'].median():.1f}dB")
print()
print(f"Normal rows ({len(positive)} = {len(positive)/len(df)*100:.1f}%):")
print(f"  DistanceM  median={positive['DistanceM'].median():.0f}m  mean={positive['DistanceM'].mean():.0f}m")
print(f"  TotalLossDb median={positive['TotalLossDb'].median():.1f}dB")
print()

# Key question: can we compute authentic LoRa Stage-1 error from
# available features? Check DistanceM as a proxy.
print("PositionErrorM vs DistanceM (normal rows only, percentile cut):")
pos = df[df["PositionErrorM"] >= 1.0].copy()
pos["dist_bin"] = pd.cut(pos["DistanceM"], bins=[0,100,500,1000,5000,15000])
g = pos.groupby("dist_bin", observed=True)["PositionErrorM"]
print(g.median().round(1))
print()

# What is RangeErrorM?
print("RangeErrorM stats (all rows):")
print(df["RangeErrorM"].describe(percentiles=[.25,.50,.75,.90]))
print()
print("RangeErrorM for near-zero PositionError rows:")
print(near_zero["RangeErrorM"].describe(percentiles=[.25,.50,.75,.90]))
