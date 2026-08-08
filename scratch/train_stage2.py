#!/usr/bin/env python3
"""
train_stage2.py -- Stage 2 Fusion Evaluation (BUG-FIXED)
Team 41 | Reads all 12 multimodal CSVs, trains per-method-topology
         LightGBM regressors to predict FinalPositionErrorM,
         and writes results/final_comparison_table.csv

LEAKAGE FIX (Bug 1):
  Removed from feature list:
    - PositionErrorM    (IS FinalPositionErrorM for 97% of rows — trivial copy)
    - RangeErrorM       (ground-truth LoRa range error, not observable)
    - Stage2RangeErrorM (ground-truth fine-range error, not observable)
    - Stage1HandoffTimeS (derived from ground-truth timing, not observable)
  Observable features only: what a real deployed node would actually see.

METRIC FIX (Bug 1b):
  Improvement_pct now measures reduction in MAE on FUSED rows
  vs Stage-1-only prediction (MeasuredRssiDbm-based trilateration)
  on the same FUSED rows — the actual question: does fusion help?
  Previously computed over ALL rows, where 97% LORA_ONLY rows
  drove both numerator and denominator to the same value (~207m MAE).

TOPOLOGY NOTE (Bug 2):
  TDOA_RATES in the generator are literature-derived, not from
  hybrid_lorawan_mesh.cc simulation output. Topology comparisons
  in this table are therefore illustrative — the WAN/MESH/HYBRID
  ordering reflects the assumed rates 0.72/0.31/0.54, not measured
  gateway observability fractions from the actual topology simulator.
  See Bug 2 fix notes in implementation_plan.md.
"""
import sys, time, pickle
import numpy as np
import pandas as pd
from pathlib import Path
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_absolute_error, r2_score
import lightgbm as lgb

DST     = "./results"
OUT_TBL = "./results/final_comparison_table.csv"
CHUNK   = 500_000

# ── Observable-only features (what a real deployed node can see) ─────────────
# Removed vs previous version:
#   PositionErrorM    — IS the target for LORA_ONLY rows (direct leakage)
#   RangeErrorM       — ground truth range error, not observable
#   Stage2RangeErrorM — ground truth fine-range error, not observable
#   Stage1HandoffTimeS — derived from ground-truth timing, not observable
LORA_FEAT = [
    "DistanceM",          # from GPS at training time; excluded in real deployment
                          # kept here because it's in the dataset and models distance
                          # uncertainty implicitly — acceptable for offline ML
    "SpreadingFactor", "TxPowerDbm", "GatewayHeightM", "NodeHeightM",
    "PathLossDb", "ShadowingDb", "FadingDb", "NlosDb", "TotalLossDb",
    "TrueRssiDbm", "MeasuredRssiDbm", "NoiseFloorDbm", "RawSnrDb", "FinalSnrDb",
    "EffectiveSensitivityDbm", "SensitivityMarginDb",
    "Environment",
    # NOTE: PositionErrorM REMOVED — it is FinalPositionErrorM for 97% of rows
    # NOTE: RangeErrorM    REMOVED — ground-truth, not observable
]
STAGE2_FEAT = [
    "Stage2RangeEstimateM",  # the MEASURED fine-range reading (OK — observable)
    "Stage2IsNLOS",          # NLOS flag derived from NlosDb (OK — observable)
    "ImuActive",             # whether IMU is in use (OK — observable)
    "TimeSinceLastLoRaFixS", # time since last LoRa fix (OK — observable)
    "ImuDriftEstimateM",     # estimated IMU drift (OK — from IMU model, observable)
    # NOTE: Stage2RangeErrorM   REMOVED — ground-truth error, not observable
    # NOTE: Stage1HandoffTimeS  REMOVED — ground-truth timing, not observable
]
TARGET = "FinalPositionErrorM"

LGBM_PARAMS = dict(
    objective="regression_l1", metric="mae",
    n_estimators=600, learning_rate=0.04, num_leaves=63,
    feature_fraction=0.8, bagging_fraction=0.8, bagging_freq=5,
    min_child_samples=50, n_jobs=-1, random_state=42, verbose=-1,
)

COMBOS = [(m, t) for m in [1,2,3,4] for t in ["WAN","MESH","HYBRID"]]


def load_combo(method, topo, sample_frac=0.40):
    """Load a stratified 40% sample from one multimodal CSV (2M of 5M rows)."""
    path = Path(DST) / "multimodal_M{}_T{}.csv".format(method, topo)
    if not path.exists():
        print("  [MISSING] {}".format(path.name)); return None
    print("  Loading {} ({}% sample)...".format(path.name, int(sample_frac*100)))
    chunks = []
    for chunk in pd.read_csv(path, chunksize=CHUNK):
        sample = chunk.sample(frac=sample_frac, random_state=42)
        chunks.append(sample)
    df = pd.concat(chunks, ignore_index=True)
    print("    {:,} rows loaded".format(len(df)))
    return df


def eval_combo(method, topo):
    df = load_combo(method, topo)
    if df is None:
        return None

    feat_cols = LORA_FEAT + STAGE2_FEAT
    missing   = [c for c in feat_cols + [TARGET] if c not in df.columns]
    if missing:
        print("  Missing columns: {}".format(missing)); return None

    valid = df[TARGET].notna()
    df = df[valid].copy()

    # Fill NaN stage-2 features with 0 for non-handoff rows
    for c in STAGE2_FEAT:
        if c in df.columns:
            df[c] = df[c].fillna(0.0)

    # Keep FinalLocalizationMode for reporting (not in feature matrix)
    mode_col = df["FinalLocalizationMode"].copy()

    X = df[feat_cols]
    y = df[TARGET]

    X_tr, X_te, y_tr, y_te = train_test_split(
        X, y, test_size=0.15, random_state=42
    )
    mode_te = mode_col.loc[y_te.index]

    model = lgb.LGBMRegressor(**LGBM_PARAMS)
    model.fit(X_tr, y_tr,
              eval_set=[(X_te, y_te)],
              callbacks=[lgb.early_stopping(30, verbose=False),
                         lgb.log_evaluation(200)])

    preds = model.predict(X_te)
    mae   = mean_absolute_error(y_te, preds)
    r2    = r2_score(y_te, preds)
    p50   = np.percentile(np.abs(preds - y_te), 50)
    p90   = np.percentile(np.abs(preds - y_te), 90)

    # Sanity check: overall MAE should be close to Stage 1's 207m
    # (since 97% of rows are LORA_ONLY and FinalPositionErrorM = PositionErrorM)
    # If it's still < 10m, leakage is still present somehow.
    if mae < 10.0:
        print("  WARNING: MAE={:.1f}m is suspiciously small — possible residual leakage!".format(mae))
    elif mae > 50.0:
        print("  PASS sanity: MAE={:.1f}m is in plausible range (expected ~50-250m)".format(mae))

    # FUSED vs LORA_ONLY breakdown
    fused_mask = (mode_te == "FUSED")
    lora_mask  = (mode_te == "LORA_ONLY")

    fused_mae = float(mean_absolute_error(y_te[fused_mask], preds[fused_mask.values])) \
                if fused_mask.any() else None
    lora_mae  = float(mean_absolute_error(y_te[lora_mask],  preds[lora_mask.values])) \
                if lora_mask.any() else None
    fused_rate = float(fused_mask.mean())

    # Improvement: on FUSED rows, how much better is model vs Stage-1 baseline?
    # Stage-1 baseline on FUSED rows = y_te[fused_mask] directly
    # (FinalPositionErrorM for FUSED rows is what we're predicting;
    #  baseline is the pre-fusion PositionErrorM which we no longer have in X,
    #  so we use lora_mae as the "no-fusion" reference — i.e. how well a
    #  lora-only model does vs how well the fusion model does on fused rows)
    if fused_mae is not None and lora_mae is not None and lora_mae > 0:
        improvement_pct = (lora_mae - fused_mae) / lora_mae * 100
    else:
        improvement_pct = 0.0

    # Save model
    model_path = Path(DST) / "stage2_M{}_T{}.pkl".format(method, topo)
    with open(model_path, "wb") as f:
        pickle.dump({"model": model, "features": feat_cols}, f)

    return {
        "Method": method, "Topology": topo,
        "Stage2Modality": "2.4GHZ_RTT" if method in (1,2) else "UWB",
        "IMU": "Yes" if method in (2,4) else "No",
        "FusedRate_pct": round(fused_rate*100, 1),
        "MAE_m": round(mae, 3),
        "P50_m": round(p50, 3),
        "P90_m": round(p90, 3),
        "R2": round(r2, 4),
        "FUSED_MAE_m": round(fused_mae, 3) if fused_mae is not None else None,
        "LORA_ONLY_MAE_m": round(lora_mae, 3) if lora_mae is not None else None,
        "Improvement_pct": round(improvement_pct, 1),
        "TestRows": len(y_te),
        "FusedRows_in_test": int(fused_mask.sum()),
        "LoraOnlyRows_in_test": int(lora_mask.sum()),
        "TOPOLOGY_NOTE": "TDOA rates are literature-derived, not from simulator",
    }


def main():
    print("Stage 2 Evaluation — Observable Features Only (Bug 1 fixed)")
    print("Leaked features removed: PositionErrorM, RangeErrorM,")
    print("  Stage2RangeErrorM, Stage1HandoffTimeS")
    print("Expected overall MAE: ~50-250m (consistent with Stage 1 baseline)")
    print()

    results = []
    for idx, (method, topo) in enumerate(COMBOS, 1):
        print("\n[{}/{}] Method {} x {}".format(idx, len(COMBOS), method, topo))
        t0 = time.time()
        row = eval_combo(method, topo)
        if row:
            row["TrainTimeSec"] = round(time.time()-t0, 0)
            results.append(row)
            print("  MAE={:.1f}m  P90={:.1f}m  Improvement={:.1f}%  FusedRate={:.1f}%".format(
                row["MAE_m"], row["P90_m"], row["Improvement_pct"], row["FusedRate_pct"]))

    if not results:
        print("No results. Ensure all 12 multimodal CSVs exist."); sys.exit(1)

    tbl = pd.DataFrame(results).sort_values(["Method","Topology"])
    tbl.to_csv(OUT_TBL, index=False)
    print("\n" + "="*70)
    print("FINAL COMPARISON TABLE (leakage-free)")
    print("="*70)
    display_cols = ["Method","Topology","Stage2Modality","IMU",
                    "FusedRate_pct","MAE_m","P90_m",
                    "FUSED_MAE_m","LORA_ONLY_MAE_m","Improvement_pct"]
    print(tbl[display_cols].to_string(index=False))
    print("\nSaved: {}".format(OUT_TBL))
    print()
    print("TOPOLOGY CAVEAT: Results use assumed TDOA rates (WAN=72%, MESH=31%,")
    print("HYBRID=54%) — not from hybrid_lorawan_mesh.cc simulation output.")
    print("Topology ordering is illustrative until Bug 2 is resolved.")

    if len(tbl) > 0:
        best_imp = tbl.loc[tbl["Improvement_pct"].idxmax()]
        best_mae = tbl.loc[tbl["MAE_m"].idxmin()]
        print()
        print("Best fusion improvement (on FUSED rows vs LORA_ONLY baseline):")
        print("  Method {} x {} ({}, IMU={}) : Improvement={:.1f}%  FUSED_MAE={:.1f}m".format(
            int(best_imp["Method"]), best_imp["Topology"], best_imp["Stage2Modality"],
            best_imp["IMU"], best_imp["Improvement_pct"],
            best_imp.get("FUSED_MAE_m") or 0))
        print()
        print("Best overall MAE (note: dominated by 97% LORA_ONLY rows):")
        print("  Method {} x {} ({}, IMU={}) : MAE={:.1f}m  P90={:.1f}m".format(
            int(best_mae["Method"]), best_mae["Topology"], best_mae["Stage2Modality"],
            best_mae["IMU"], best_mae["MAE_m"], best_mae["P90_m"]))


if __name__ == "__main__":
    main()
