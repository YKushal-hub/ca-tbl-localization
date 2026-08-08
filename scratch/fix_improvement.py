#!/usr/bin/env python3
"""
fix_improvement.py -- Recompute improvement_pct for all 12 stage2 results.

The original improvement compared overall MAE vs PositionErrorM across ALL rows,
but since ~97% of rows are LORA_ONLY (FinalPositionErrorM == PositionErrorM),
baseline ≈ 0 and denominator collapses, causing huge negative values.

Correct metric: on FUSED rows only, compare LightGBM-predicted error
vs raw PositionErrorM on the same rows.
"""
import json, pickle
import numpy as np
import pandas as pd
from pathlib import Path
from sklearn.metrics import mean_absolute_error

DST      = "./results"
OUT_TBL  = "./results/final_comparison_table.csv"
CHUNK    = 500_000
SAMPLE_FRAC = 0.40

LORA_FEAT = [
    "DistanceM","SpreadingFactor","TxPowerDbm","GatewayHeightM","NodeHeightM",
    "PathLossDb","ShadowingDb","FadingDb","NlosDb","TotalLossDb",
    "TrueRssiDbm","MeasuredRssiDbm","NoiseFloorDbm","RawSnrDb","FinalSnrDb",
    "EffectiveSensitivityDbm","SensitivityMarginDb","PositionErrorM",
    "RangeErrorM","Environment",
]
STAGE2_FEAT = [
    "Stage2RangeEstimateM","Stage2RangeErrorM","Stage2IsNLOS",
    "ImuActive","TimeSinceLastLoRaFixS","ImuDriftEstimateM",
    "Stage1HandoffTimeS",
]
TARGET = "FinalPositionErrorM"

COMBOS = [(m, t) for m in [1,2,3,4] for t in ["WAN","MESH","HYBRID"]]


def recompute_improvement(method, topo):
    result_path = Path(DST) / "stage2_result_M{}_T{}.json".format(method, topo)
    model_path  = Path(DST) / "stage2_M{}_T{}.pkl".format(method, topo)
    csv_path    = Path(DST) / "multimodal_M{}_T{}.csv".format(method, topo)

    if not (result_path.exists() and model_path.exists() and csv_path.exists()):
        print("M{}×{}: missing files — skipping".format(method, topo))
        return None

    with open(result_path) as f:
        row = json.load(f)

    # Load model
    with open(model_path, "rb") as f:
        saved = pickle.load(f)
    model     = saved["model"]
    feat_cols = saved["features"]

    # Sample same data (same seed = same rows)
    rng = np.random.default_rng(42)
    chunks = []
    for chunk in pd.read_csv(csv_path, chunksize=CHUNK):
        n = max(1, int(len(chunk) * SAMPLE_FRAC))
        idx = rng.choice(len(chunk), size=n, replace=False)
        chunks.append(chunk.iloc[idx])
        del chunk
    df = pd.concat(chunks, ignore_index=True)

    valid = df[TARGET].notna() & df["PositionErrorM"].notna()
    df = df[valid].copy()
    for c in STAGE2_FEAT:
        if c in df.columns:
            df[c] = df[c].fillna(0.0)

    X = df[feat_cols]
    y = df[TARGET]

    # Reproduce same train/test split
    from sklearn.model_selection import train_test_split
    X_tr, X_te, y_tr, y_te = train_test_split(X, y, test_size=0.15, random_state=42)

    preds      = model.predict(X_te)
    fused_mask = X_te["Stage2RangeEstimateM"] != 0

    if fused_mask.any():
        fused_mae = float(mean_absolute_error(y_te[fused_mask], preds[fused_mask.values]))
        fused_baseline = float(mean_absolute_error(
            y_te[fused_mask],
            X_te.loc[fused_mask.index[fused_mask], "PositionErrorM"]))
        improvement_pct = (fused_baseline - fused_mae) / max(fused_baseline, 1e-9) * 100
        fused_rate = float(fused_mask.mean())
    else:
        fused_mae = None
        fused_baseline = None
        improvement_pct = 0.0
        fused_rate = 0.0

    lora_mae = float(mean_absolute_error(y_te[~fused_mask], preds[~fused_mask.values])) \
               if (~fused_mask).any() else None

    row["Improvement_pct"]  = round(improvement_pct, 1)
    row["FUSED_MAE_m"]      = round(fused_mae, 3) if fused_mae is not None else None
    row["LORA_ONLY_MAE_m"]  = round(lora_mae, 3) if lora_mae is not None else None
    row["FusedRate_pct"]    = round(fused_rate * 100, 1)
    row["FusedBaseline_MAE_m"] = round(fused_baseline, 3) if fused_baseline is not None else None

    # Save fixed JSON
    with open(result_path, "w") as f:
        json.dump(row, f, indent=2)

    print("M{}×{}: Improvement={:.1f}%  FusedMAE={:.3f}m  FusedBaseline={:.3f}m  Rate={:.1f}%".format(
        method, topo,
        improvement_pct,
        fused_mae if fused_mae else 0,
        fused_baseline if fused_baseline else 0,
        fused_rate*100))

    return row


def main():
    print("Recomputing improvement_pct for all 12 variants...\n")
    results = []
    for m, t in COMBOS:
        row = recompute_improvement(m, t)
        if row:
            results.append(row)

    if not results:
        print("No results found."); return

    tbl = pd.DataFrame(results).sort_values(["Method","Topology"])
    tbl.to_csv(OUT_TBL, index=False)

    print("\n" + "="*70)
    print("FINAL COMPARISON TABLE (corrected)")
    print("="*70)
    cols = ["Method","Topology","Stage2Modality","IMU",
            "FusedRate_pct","MAE_m","P90_m","Improvement_pct",
            "FUSED_MAE_m","FusedBaseline_MAE_m"]
    available = [c for c in cols if c in tbl.columns]
    print(tbl[available].to_string(index=False))
    print("\nSaved:", OUT_TBL)

    best = tbl.loc[tbl["Improvement_pct"].idxmax()]
    print("\nBest fusion improvement:")
    print("  Method {} x {} ({}) : Improvement={:.1f}%  FUSED_MAE={:.3f}m".format(
        int(best["Method"]), best["Topology"], best["Stage2Modality"],
        best["Improvement_pct"], best.get("FUSED_MAE_m", 0) or 0))


if __name__ == "__main__":
    main()
