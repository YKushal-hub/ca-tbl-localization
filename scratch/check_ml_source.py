import pandas as pd
import numpy as np

df = pd.read_csv("results/ml_training_dataset.csv", nrows=10000)

print("Columns in ml_training_dataset.csv:")
print([c for c in df.columns if any(k in c for k in ["Pos","Error","TDOA","Eligible"])])
print()
print("PositionErrorM stats (first 10k rows):")
print(df["PositionErrorM"].describe())
print()
print("Fraction near zero (<0.01):", (df["PositionErrorM"] < 0.01).mean())
print("Fraction near 9999 (>9000):", (df["PositionErrorM"] > 9000).mean())
print("Fraction in [1, 678]:", ((df["PositionErrorM"] >= 1) & (df["PositionErrorM"] <= 678)).mean())
print()
print("TdoaEligible in ml_training_dataset?", "TdoaEligible" in df.columns)
if "TdoaEligible" in df.columns:
    print(df.groupby("TdoaEligible")["PositionErrorM"].describe())
print()
# Also check if Dist25/50/100 are in df_ml
print("Dist25/50/100 present?", all(c in df.columns for c in ["Dist25","Dist50","Dist100"]))
if "Dist25" in df.columns:
    print("Dist25 median:", df["Dist25"].median())
    print("Dist50 median:", df["Dist50"].median())
    print("Dist100 median:", df["Dist100"].median())
