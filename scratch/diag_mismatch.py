import pandas as pd
import numpy as np

path = "./results/multimodal_M4_THYBRID.csv"
print("Scanning {} for NLOS mismatch...".format(path))

found = 0
for chunk in pd.read_csv(path, chunksize=500_000):
    inherited = (chunk["NlosDb"].fillna(0.0) > 0).astype(int)
    recorded  = chunk["Stage2IsNLOS"].fillna(0.0).astype(int)
    bad = chunk[inherited != recorded]
    if len(bad) > 0:
        found += len(bad)
        print("MISMATCH ROWS:")
        print(bad[["NlosDb","Stage2IsNLOS","FinalLocalizationMode",
                    "Stage2Modality","ImuActive","TimeSinceLastLoRaFixS",
                    "FinalPositionErrorM","PositionErrorM"]].to_string())
        print("NlosDb value:", bad["NlosDb"].values)
        print("Stage2IsNLOS value:", bad["Stage2IsNLOS"].values)

print("Total mismatches found:", found)

# ── Fusion improvement rate diagnosis ────────────────────────────────
print("\n--- Fusion improvement rate diagnosis ---")
chunk = pd.read_csv(path, nrows=100_000)
fused = chunk[chunk["FinalLocalizationMode"] == "FUSED"].copy()
print("FUSED rows in first 100k: {}".format(len(fused)))
if len(fused) > 0:
    better = (fused["FinalPositionErrorM"] <= fused["PositionErrorM"])
    print("Improvement rate: {:.1f}%".format(better.mean()*100))
    print("\nFinalPositionErrorM stats (FUSED rows):")
    print(fused["FinalPositionErrorM"].describe())
    print("\nPositionErrorM stats (FUSED rows):")
    print(fused["PositionErrorM"].describe())
    # Show cases where fusion made it WORSE
    worse = fused[~better]
    print("\nWorse rows sample (FinalPosErr > LoRa PosErr):")
    print(worse[["PositionErrorM","FinalPositionErrorM",
                  "Stage2RangeErrorM","ImuDriftEstimateM",
                  "TimeSinceLastLoRaFixS"]].head(10).to_string())
