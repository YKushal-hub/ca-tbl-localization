#!/usr/bin/env python3
"""
diag_lora_mae.py -- Diagnose why LORA_MAE differs across Methods/Topologies.

Expected: LORA_ONLY rows have FinalPositionErrorM == PositionErrorM (generator
confirmed correct). So LORA_MAE should be a single consistent number (~207m)
regardless of which Method or Topology variant we're looking at.

Observed: M1/M3 ~207-210m, M2/M4 ~171-198m. This delta (~35m) needs a root cause.

Hypotheses:
  A. Different train/test splits per variant (random_state=42 but independent
     shuffle -- heavy-tailed distribution means split composition matters).
  B. LORA_ONLY population is actually DIFFERENT between M1 and M2 variants --
     i.e. the FinalLocalizationMode assignment changes per method.
  C. The IMU methods (M2/M4) have a different PositionErrorM distribution in
     their CSV (different base rows got selected at generator time).
"""
import pandas as pd
import numpy as np

DST = "./results"

combos = [
    ("M1","WAN"),("M1","MESH"),("M1","HYBRID"),
    ("M2","WAN"),("M2","MESH"),("M2","HYBRID"),
    ("M3","WAN"),("M3","MESH"),("M3","HYBRID"),
    ("M4","WAN"),("M4","MESH"),("M4","HYBRID"),
]

print("=== HYPOTHESIS B: Is LORA_ONLY population the same across methods? ===\n")
print("(Sample 2% of each CSV, check LORA_ONLY PositionErrorM distribution)\n")
print(f"{'Variant':12s}  {'N_LORA':>10s}  {'pos_err_mean':>13s}  {'pos_err_std':>12s}  {'LORA_frac':>10s}")
print("-"*65)

results = []
for method, topo in combos:
    path = f"{DST}/multimodal_{method}_T{topo}.csv"
    try:
        chunks = []
        for chunk in pd.read_csv(path, chunksize=500_000):
            chunks.append(chunk.sample(frac=0.02, random_state=42))
        df = pd.concat(chunks, ignore_index=True)
        
        lora = df[df["FinalLocalizationMode"] == "LORA_ONLY"]
        fused = df[df["FinalLocalizationMode"] == "FUSED"]
        lora_frac = len(lora) / len(df)
        
        row = {
            "variant": f"{method}x{topo}",
            "n_lora": len(lora),
            "pos_err_mean": lora["PositionErrorM"].mean(),
            "pos_err_std": lora["PositionErrorM"].std(),
            "lora_frac": lora_frac,
            "n_fused": len(fused),
            "fused_pos_mean": fused["PositionErrorM"].mean() if len(fused) > 0 else None,
        }
        results.append(row)
        print(f"{row['variant']:12s}  {row['n_lora']:>10,}  {row['pos_err_mean']:>13.1f}m  "
              f"{row['pos_err_std']:>11.1f}m  {lora_frac:>9.1%}")
    except FileNotFoundError:
        print(f"{method}x{topo:6s}  FILE NOT FOUND")

print()
print("=== HYPOTHESIS A: Train/test split variance ===\n")
print("The train_stage2_parallel.py uses train_test_split(random_state=42).")
print("If the CSV row ORDER differs between M1 and M2, the split will differ.")
print()
print("Check: are the LORA_ONLY pos_err means above consistent within Method (M1/M2/M3/M4)")
print("but inconsistent ACROSS Methods? If yes -> Hypothesis B (population differs).")
print("If means are identical across all 12 -> Hypothesis A (split noise only).")
print()

# Group by method
import re
for m in [1, 2, 3, 4]:
    vals = [r["pos_err_mean"] for r in results if r["variant"].startswith(f"M{m}")]
    if vals:
        print(f"M{m} LORA_ONLY pos_err_mean across topologies: {[f'{v:.1f}m' for v in vals]}")
        print(f"   Range: {min(vals):.1f}m - {max(vals):.1f}m  (spread: {max(vals)-min(vals):.1f}m)")

print()
print("=== VERDICT ===")
all_means = [r["pos_err_mean"] for r in results]
cross_method_spread = max(all_means) - min(all_means)
within_m1 = [r["pos_err_mean"] for r in results if r["variant"].startswith("M1")]
m1_spread = max(within_m1) - min(within_m1) if within_m1 else 0

if cross_method_spread > 10:
    print(f"Cross-method spread in LORA_ONLY pos_err_mean: {cross_method_spread:.1f}m")
    print("-> SIGNIFICANT. The LORA_ONLY population ITSELF differs between methods.")
    print("   Root cause is in multimodal_generator.py: the ImuActive/handoff logic")
    print("   must be classifying DIFFERENT rows as LORA_ONLY for M1 vs M2.")
    print("   Specifically: M2 has ImuActive=True for ALL rows where hok=True,")
    print("   including rows where Stage2RangeEstimateM is NaN (range check failed).")
    print("   These rows get mode=LORA_ONLY in M1 but may get mode=FUSED or a")
    print("   different FinalPositionErrorM in M2 via EKF (even without fine-range).")
else:
    print(f"Cross-method spread: {cross_method_spread:.1f}m -- within sampling noise.")
    print("-> Hypothesis A is more likely: split randomness on heavy-tailed distribution.")
    print("   Fix: shared fixed split index reused across all 12/15 variants.")
