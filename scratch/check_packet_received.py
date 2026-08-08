import pandas as pd
import numpy as np

df = pd.read_csv("results/ml_training_dataset.csv", nrows=200000)

near_zero = df[df["PositionErrorM"] < 0.01]
received  = df[df["PositionErrorM"] >= 1.0]

print("=== Near-zero PositionErrorM rows ===")
print(f"Count: {len(near_zero):,} = {len(near_zero)/len(df)*100:.1f}% of all rows")
print(f"PacketReceived: {near_zero['PacketReceived'].value_counts().to_dict()}")
print(f"SensitivityMarginDb: min={near_zero['SensitivityMarginDb'].min():.1f}  "
      f"median={near_zero['SensitivityMarginDb'].median():.1f}  "
      f"max={near_zero['SensitivityMarginDb'].max():.1f}")
print(f"RangeErrorM: min={near_zero['RangeErrorM'].min():.4f}  "
      f"max={near_zero['RangeErrorM'].max():.4f}  "
      f"any non-zero: {(near_zero['RangeErrorM'] != 0).sum()}")
print(f"EstimatedRangeM: min={near_zero['EstimatedRangeM'].min():.1f}  "
      f"median={near_zero['EstimatedRangeM'].median():.1f}  "
      f"max={near_zero['EstimatedRangeM'].max():.1f}")
print()

print("=== Normal PositionErrorM rows (>=1m) ===")
print(f"Count: {len(received):,} = {len(received)/len(df)*100:.1f}% of all rows")
print(f"PacketReceived: {received['PacketReceived'].value_counts().to_dict()}")
print()

print("=== PacketReceived cross-tab ===")
df["zero_pos"] = df["PositionErrorM"] < 0.01
print(pd.crosstab(df["PacketReceived"], df["zero_pos"],
                  rownames=["PacketReceived"], colnames=["PositionErrorM<0.01"]))
print()

print("=== For PacketReceived=0 rows ===")
not_rcv = df[df["PacketReceived"] == 0]
print(f"  PositionErrorM < 0.01: {(not_rcv['PositionErrorM'] < 0.01).mean()*100:.1f}%")
print(f"  RangeErrorM == 0:      {(not_rcv['RangeErrorM'] == 0).mean()*100:.1f}%")
print(f"  EstimatedRangeM median: {not_rcv['EstimatedRangeM'].median():.1f}m")
print()

print("=== For PacketReceived=1 rows ===")
rcv = df[df["PacketReceived"] == 1]
print(f"  PositionErrorM < 0.01: {(rcv['PositionErrorM'] < 0.01).mean()*100:.1f}%")
print(f"  RangeErrorM == 0:      {(rcv['RangeErrorM'] == 0).mean()*100:.1f}%")
print(f"  PositionErrorM median: {rcv['PositionErrorM'].median():.2f}m")
print(f"  PositionErrorM mean:   {rcv['PositionErrorM'].mean():.2f}m")
