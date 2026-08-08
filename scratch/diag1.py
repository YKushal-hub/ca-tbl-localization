import pandas as pd
import numpy as np

print("=== DIAGNOSTIC 1: Where does Bug 1 live? ===\n")

chunks = []
for chunk in pd.read_csv("results/multimodal_M1_TWAN.csv", chunksize=500_000):
    chunks.append(chunk.sample(frac=0.02, random_state=42))
df = pd.concat(chunks, ignore_index=True)

lora_only = df[df["FinalLocalizationMode"] == "LORA_ONLY"]
fused     = df[df["FinalLocalizationMode"] == "FUSED"]

print("Row breakdown:")
print(f"  LORA_ONLY: {len(lora_only):,} ({len(lora_only)/len(df)*100:.1f}%)")
print(f"  FUSED:     {len(fused):,} ({len(fused)/len(df)*100:.1f}%)")
print(f"  Total:     {len(df):,}")
print()

print("FinalPositionErrorM stats for LORA_ONLY rows (should be ~207m mean):")
print(lora_only["FinalPositionErrorM"].describe())
print()

print("PositionErrorM stats for LORA_ONLY rows (Stage 1 baseline):")
print(lora_only["PositionErrorM"].describe())
print()

# The critical check
mismatch = (lora_only["FinalPositionErrorM"] - lora_only["PositionErrorM"]).abs()
print(f"FinalPositionErrorM - PositionErrorM (LORA_ONLY rows):")
print(f"  Mean abs difference: {mismatch.mean():.6f} m  (must be 0.0 if generator is correct)")
print(f"  Max  abs difference: {mismatch.max():.6f} m")
print(f"  Rows with diff > 0.001m: {(mismatch > 0.001).sum():,}")
print()

# Check FUSED rows too
if len(fused) > 0:
    print("FinalPositionErrorM stats for FUSED rows:")
    print(fused["FinalPositionErrorM"].describe())
    print()
    print("Sample FUSED row values:")
    print(fused[["PositionErrorM","FinalPositionErrorM","Stage2RangeErrorM",
                  "ImuDriftEstimateM","TimeSinceLastLoRaFixS"]].head(10))
