#!/usr/bin/env python3
"""
train_stage1.py -- Stage 1 LoRa-only ML Model
Team 41 | Trains on results/ml_training_dataset.csv (5M rows)

Models:
  A) Binary classifier: PacketReceived (RF link success)
  B) Regressor: PositionErrorM (localization accuracy proxy)

Both use LightGBM (fast, handles mixed numeric+categorical types,
interpretable feature importance). Output: results/stage1_model.pkl
"""
import sys, time, pickle
import numpy as np
import pandas as pd
from pathlib import Path
from sklearn.model_selection import train_test_split
from sklearn.metrics import (roc_auc_score, classification_report,
                              mean_absolute_error, r2_score)
import lightgbm as lgb

SRC   = "./results/ml_training_dataset.csv"
OUT   = "./results/stage1_model.pkl"
CHUNK = 500_000

FEATURE_COLS = [
    "DistanceM","SpreadingFactor","TxPowerDbm","GatewayHeightM","NodeHeightM",
    "PathLossDb","ShadowingDb","FadingDb","NlosDb","BuildingEntryLossDb",
    "WallPenetrationLossDb","FloorAttenuationDb","BodyShadowingLossDb",
    "TemporalBlockageLossDb","IsTwoRay","IsTunnel","SoilAttenuationDb",
    "AtmosphericDb","FoliageDb","TerrainDb","InterferencePenaltyDb",
    "DopplerPenaltyDb","HwTxVariationDb","EvmPercent","IqImageRejectionDb",
    "AdcClippingOccurred","TimeSyncErrorNs","TotalLossDb","TrueRssiDbm",
    "MeasuredRssiDbm","NoiseFloorDbm","RawSnrDb","FinalSnrDb",
    "EffectiveSensitivityDbm","SensitivityMarginDb","DelaySpreadNs",
    "RainRateMmHr","Environment",
]
TARGET_CLASS  = "PacketReceived"
TARGET_REG    = "PositionErrorM"

LGBM_PARAMS_CLASS = dict(
    objective="binary", metric="auc",
    n_estimators=500, learning_rate=0.05, num_leaves=127,
    feature_fraction=0.8, bagging_fraction=0.8, bagging_freq=5,
    min_child_samples=50, n_jobs=-1, random_state=42, verbose=-1,
)
LGBM_PARAMS_REG = dict(
    objective="regression_l1", metric="mae",
    n_estimators=500, learning_rate=0.05, num_leaves=127,
    feature_fraction=0.8, bagging_fraction=0.8, bagging_freq=5,
    min_child_samples=50, n_jobs=-1, random_state=42, verbose=-1,
)

def main():
    src = Path(SRC)
    if not src.exists():
        print("ERROR: {} not found".format(src)); sys.exit(1)

    print("Loading {} ...".format(src))
    t0 = time.time()
    df = pd.read_csv(src)
    print("  Loaded {:,} rows in {:.0f}s".format(len(df), time.time()-t0))

    # Guard: check columns present
    missing = [c for c in FEATURE_COLS + [TARGET_CLASS, TARGET_REG] if c not in df.columns]
    if missing:
        print("ERROR: Missing columns: {}".format(missing)); sys.exit(1)

    X = df[FEATURE_COLS].copy()
    y_class = df[TARGET_CLASS].astype(int)
    y_reg   = df[TARGET_REG].astype(float)

    # Drop rows where PositionErrorM is NaN
    valid = y_reg.notna()
    X = X[valid]; y_class = y_class[valid]; y_reg = y_reg[valid]
    print("  Using {:,} rows after dropping NaN targets".format(len(X)))

    X_tr, X_te, yc_tr, yc_te, yr_tr, yr_te = train_test_split(
        X, y_class, y_reg, test_size=0.15, random_state=42, stratify=y_class
    )
    print("  Train: {:,}  Test: {:,}".format(len(X_tr), len(X_te)))

    # ── Model A: PacketReceived classifier ───────────────────────────
    print("\nTraining Stage1-A (PacketReceived classifier)...")
    t0 = time.time()
    clf = lgb.LGBMClassifier(**LGBM_PARAMS_CLASS)
    clf.fit(X_tr, yc_tr,
            eval_set=[(X_te, yc_te)],
            callbacks=[lgb.early_stopping(30, verbose=False),
                       lgb.log_evaluation(100)])
    preds_prob = clf.predict_proba(X_te)[:, 1]
    auc = roc_auc_score(yc_te, preds_prob)
    preds_bin = (preds_prob > 0.5).astype(int)
    print("  AUC = {:.4f}  Time = {:.0f}s".format(auc, time.time()-t0))
    print(classification_report(yc_te, preds_bin, digits=4))

    fi_class = pd.Series(clf.feature_importances_, index=FEATURE_COLS).sort_values(ascending=False)
    print("  Top-10 features (classifier):")
    print(fi_class.head(10).to_string())

    # ── Model B: PositionErrorM regressor ────────────────────────────
    print("\nTraining Stage1-B (PositionErrorM regressor)...")
    t0 = time.time()
    reg = lgb.LGBMRegressor(**LGBM_PARAMS_REG)
    reg.fit(X_tr, yr_tr,
            eval_set=[(X_te, yr_te)],
            callbacks=[lgb.early_stopping(30, verbose=False),
                       lgb.log_evaluation(100)])
    preds_reg = reg.predict(X_te)
    mae = mean_absolute_error(yr_te, preds_reg)
    r2  = r2_score(yr_te, preds_reg)
    p50 = np.percentile(np.abs(preds_reg - yr_te), 50)
    p90 = np.percentile(np.abs(preds_reg - yr_te), 90)
    print("  MAE = {:.2f}m  R2 = {:.4f}  P50 = {:.2f}m  P90 = {:.2f}m  Time = {:.0f}s".format(
        mae, r2, p50, p90, time.time()-t0))

    fi_reg = pd.Series(reg.feature_importances_, index=FEATURE_COLS).sort_values(ascending=False)
    print("  Top-10 features (regressor):")
    print(fi_reg.head(10).to_string())

    # ── Save ─────────────────────────────────────────────────────────
    bundle = {
        "clf": clf, "reg": reg,
        "features": FEATURE_COLS,
        "metrics": {
            "auc": auc, "mae": mae, "r2": r2, "p50": p50, "p90": p90
        },
        "fi_class": fi_class.to_dict(),
        "fi_reg":   fi_reg.to_dict(),
    }
    with open(OUT, "wb") as f:
        pickle.dump(bundle, f)
    print("\nSaved: {}".format(OUT))
    print("Stage 1 training complete. Next: python3 scratch/train_stage2.py")

if __name__ == "__main__":
    main()
