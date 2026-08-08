import pandas as pd
import numpy as np

df = pd.read_csv("./results/final_comparison_table.csv")
print("=== FINAL COMPARISON TABLE ===")
print(df.to_string())

print("\n=== SANITY: FinalPositionErrorM stats per method (first 50k rows of WAN) ===")
for m in [1, 2, 3, 4]:
    f = "./results/multimodal_M{}_TWAN.csv".format(m)
    chunk = pd.read_csv(f, nrows=50000)
    fpe = chunk["FinalPositionErrorM"]
    pe  = chunk["PositionErrorM"]
    mode = chunk["FinalLocalizationMode"].value_counts().to_dict()
    print("M{}: FinalPosErr mean={:.3f} std={:.3f} min={:.4f} max={:.1f} | PositionErr mean={:.3f} | Modes={}".format(
        m, fpe.mean(), fpe.std(), fpe.min(), fpe.max(), pe.mean(), mode))

print("\n=== M2 FUSED rows sample ===")
chunk2 = pd.read_csv("./results/multimodal_M2_TWAN.csv", nrows=50000)
fused = chunk2[chunk2["FinalLocalizationMode"] == "FUSED"]
print("FUSED rows:", len(fused))
if len(fused) > 0:
    print(fused[["PositionErrorM","FinalPositionErrorM","Stage2RangeErrorM",
                  "ImuDriftEstimateM","TimeSinceLastLoRaFixS"]].describe())
