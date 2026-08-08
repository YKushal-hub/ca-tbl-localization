#!/usr/bin/env python3
"""
verify_one_variant.py -- VERIFICATION_1 from the fix prompt.
Trains M1xWAN with Bug-1-fixed feature list, asserts MAE is in 50-250m range.
Run this BEFORE retraining all 12 variants to confirm fix works.
"""
import sys, time, pickle
import numpy as np
import pandas as pd
from pathlib import Path
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_absolute_error, r2_score
import lightgbm as lgb

DST   = "./results"
CHUNK = 500_000

# FIXED feature list (leakage-free)
LORA_FEAT = [
    "DistanceM","SpreadingFactor","TxPowerDbm","GatewayHeightM","NodeHeightM",
    "PathLossDb","ShadowingDb","FadingDb","NlosDb","TotalLossDb",
    "TrueRssiDbm","MeasuredRssiDbm","NoiseFloorDbm","RawSnrDb","FinalSnrDb",
    "EffectiveSensitivityDbm","SensitivityMarginDb",
    "Environment",
]
STAGE2_FEAT = [
    "Stage2RangeEstimateM","Stage2IsNLOS",
    "ImuActive","TimeSinceLastLoRaFixS","ImuDriftEstimateM",
]
TARGET = "FinalPositionErrorM"

LGBM_PARAMS = dict(
    objective="regression_l1", metric="mae",
    n_estimators=200, learning_rate=0.05, num_leaves=63,
    feature_fraction=0.8, bagging_fraction=0.8, bagging_freq=5,
    min_child_samples=50, n_jobs=4, random_state=42, verbose=-1,
)

print("=== VERIFICATION_1: Single-variant M1xWAN with Bug-1-fixed features ===\n")
print(f"Feature count: {len(LORA_FEAT + STAGE2_FEAT)} features")
print(f"Features: {LORA_FEAT + STAGE2_FEAT}")
print()

t0 = time.time()
path = Path(DST) / "multimodal_M1_TWAN.csv"
print(f"Loading {path.name} (10% sample for fast verification)...")
chunks = []
for chunk in pd.read_csv(path, chunksize=CHUNK):
    chunks.append(chunk.sample(frac=0.10, random_state=42))
df = pd.concat(chunks, ignore_index=True)
print(f"  {len(df):,} rows loaded")

feat_cols = LORA_FEAT + STAGE2_FEAT
valid = df[TARGET].notna()
df = df[valid].copy()
for c in STAGE2_FEAT:
    df[c] = df[c].fillna(0.0)

mode_col = df["FinalLocalizationMode"].copy()
X = df[feat_cols]
y = df[TARGET]

print(f"\nTarget (FinalPositionErrorM) stats:")
print(f"  Mean: {y.mean():.1f}m  Median: {y.median():.1f}m  P90: {y.quantile(0.9):.1f}m")
print(f"  (Should match Stage 1 PositionErrorM: mean~207m)")

fused_count = (mode_col == "FUSED").sum()
lora_count = (mode_col == "LORA_ONLY").sum()
print(f"\nRow breakdown:")
print(f"  LORA_ONLY: {lora_count:,} ({lora_count/len(df)*100:.1f}%)")
print(f"  FUSED:     {fused_count:,} ({fused_count/len(df)*100:.1f}%)")

X_tr, X_te, y_tr, y_te = train_test_split(X, y, test_size=0.20, random_state=42)
mode_te = mode_col.loc[y_te.index]

print(f"\nTraining LightGBM ({len(X_tr):,} train rows, {len(X_te):,} test rows)...")
model = lgb.LGBMRegressor(**LGBM_PARAMS)
model.fit(X_tr, y_tr,
          eval_set=[(X_te, y_te)],
          callbacks=[lgb.early_stopping(20, verbose=False),
                     lgb.log_evaluation(50)])

preds = model.predict(X_te)
mae = mean_absolute_error(y_te, preds)
p90 = np.percentile(np.abs(preds - y_te), 90)
r2  = r2_score(y_te, preds)

fused_mask = (mode_te == "FUSED")
lora_mask  = (mode_te == "LORA_ONLY")
fused_mae  = mean_absolute_error(y_te[fused_mask], preds[fused_mask.values]) if fused_mask.any() else None
lora_mae   = mean_absolute_error(y_te[lora_mask],  preds[lora_mask.values])  if lora_mask.any()  else None

elapsed = time.time() - t0
print(f"\n=== RESULTS ===")
print(f"  Overall MAE : {mae:.1f} m   (expected: 50-250m)")
print(f"  Overall P90 : {p90:.1f} m")
print(f"  R2          : {r2:.4f}")
print(f"  FUSED MAE   : {fused_mae:.3f} m  ({fused_mask.sum()} rows)" if fused_mae else "  FUSED MAE   : N/A")
print(f"  LORA_ONLY   : {lora_mae:.1f} m   ({lora_mask.sum()} rows)" if lora_mae else "  LORA_ONLY   : N/A")
print(f"  Time        : {elapsed:.0f}s")
print()

# ASSERTION per fix prompt VERIFICATION_1
if mae < 10.0:
    print(f"FAIL: MAE={mae:.2f}m < 10m — leakage NOT fully fixed.")
    print("      Check feature list for residual ground-truth columns.")
    sys.exit(1)
elif 50 <= mae <= 350:
    print(f"PASS: MAE={mae:.1f}m is consistent with 97% LORA_ONLY rows")
    print(f"      (Stage 1 baseline ~207m — model is learning residuals, not copying target)")
else:
    print(f"WARNING: MAE={mae:.1f}m — outside expected 50-350m range. Investigate.")

print()
print("Feature importances (top 10):")
fi = pd.Series(model.feature_importances_, index=feat_cols).sort_values(ascending=False)
print(fi.head(10).to_string())
print()
print("If PositionErrorM, RangeErrorM, Stage2RangeErrorM appear here:")
print("  -> those columns are STILL in the feature list (check for copy errors)")
