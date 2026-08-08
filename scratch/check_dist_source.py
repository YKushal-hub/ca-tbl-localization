import pandas as pd
import numpy as np

df_tdoa = pd.read_csv("results/tdoa_processed_dataset_fixed.csv", nrows=5000)
print("Dist25/50/100 in tdoa_fixed?:", all(c in df_tdoa.columns for c in ["Dist25","Dist50","Dist100"]))
print("Dist25 median:", df_tdoa["Dist25"].median())
print("Dist50 median:", df_tdoa["Dist50"].median())
print("Dist100 median:", df_tdoa["Dist100"].median())
print()
for topo in ["WAN", "MESH", "HYBRID"]:
    sub = df_tdoa[df_tdoa["Topology"] == topo]
    print(f"{topo}: Dist25={sub['Dist25'].median():.1f}m  "
          f"Dist50={sub['Dist50'].median():.1f}m  "
          f"Dist100={sub['Dist100'].median():.1f}m")
print()
print("PositionErrorM in tdoa_fixed by topology:")
print(df_tdoa.groupby("Topology")["PositionErrorM"].describe())
print()
# Check ml_training_dataset PositionErrorM more carefully
df_ml = pd.read_csv("results/ml_training_dataset.csv", nrows=10000)
print("\nPositionErrorM breakdown in ml_training_dataset (first 10k):")
print("  near zero (<0.01):  ", (df_ml["PositionErrorM"] < 0.01).sum(),
      "rows = {:.1f}%".format((df_ml["PositionErrorM"] < 0.01).mean()*100))
print("  very small [0.01,50]:", ((df_ml["PositionErrorM"] >= 0.01) & (df_ml["PositionErrorM"] <= 50)).sum(),
      "rows = {:.1f}%".format(((df_ml["PositionErrorM"] >= 0.01) & (df_ml["PositionErrorM"] <= 50)).mean()*100))
print("  [50, 678]:           ", ((df_ml["PositionErrorM"] > 50) & (df_ml["PositionErrorM"] <= 678)).sum(),
      "rows = {:.1f}%".format(((df_ml["PositionErrorM"] > 50) & (df_ml["PositionErrorM"] <= 678)).mean()*100))
print("  >678:                ", (df_ml["PositionErrorM"] > 678).sum(),
      "rows = {:.1f}%".format((df_ml["PositionErrorM"] > 678).mean()*100))
