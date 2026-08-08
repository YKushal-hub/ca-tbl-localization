#!/usr/bin/env python3
"""
train_stage2_parallel.py -- Parallel Stage 2 Fusion Evaluation (safe for laptop)

Runs up to PARALLEL_JOBS=2 variants concurrently, each with LGBM_THREADS=4
so total CPU threads = 8. Memory per worker ~1.5 GB, so total ~3 GB safe for 8GB RAM.

Crash-safe: each variant saves its own .pkl and result .json immediately on
completion. Re-running skips already-done variants automatically.

BUG 1 FIX: Removed all leaked features from feature matrix:
  - PositionErrorM    (IS FinalPositionErrorM for 97% of rows)
  - RangeErrorM       (ground-truth, not observable)
  - Stage2RangeErrorM (ground-truth fine-range error, not observable)
  - Stage1HandoffTimeS (derived from ground-truth timing, not observable)

BUG 2 NOTE: TDOA_RATES in generator are literature-derived (not from
  hybrid_lorawan_mesh.cc). Topology comparisons are illustrative.
"""
import sys, time, json, pickle, os, signal
import numpy as np
import pandas as pd
from pathlib import Path
from concurrent.futures import ProcessPoolExecutor, as_completed
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_absolute_error, r2_score
import lightgbm as lgb

# ── Configuration ──────────────────────────────────────────────────────────────
DST          = "./results"
OUT_TBL      = "./results/final_comparison_table.csv"
CHUNK        = 500_000
SAMPLE_FRAC  = 0.40        # 40% of 5M = 2M rows per variant
PARALLEL_JOBS = 2          # concurrent variants — keep at 2 for laptop safety
LGBM_THREADS  = 4          # threads per LightGBM model (2×4 = 8 total)
NICE_LEVEL    = 10         # reduce OS priority so system stays responsive

# Observable-only features — ground-truth leakage removed:
# REMOVED: PositionErrorM    (= FinalPositionErrorM for 97% of rows)
# REMOVED: RangeErrorM       (ground-truth LoRa range error)
# REMOVED: Stage2RangeErrorM (ground-truth fine-range error)
# REMOVED: Stage1HandoffTimeS (derived from ground-truth timing)
LORA_FEAT = [
    "DistanceM","SpreadingFactor","TxPowerDbm","GatewayHeightM","NodeHeightM",
    "PathLossDb","ShadowingDb","FadingDb","NlosDb","TotalLossDb",
    "TrueRssiDbm","MeasuredRssiDbm","NoiseFloorDbm","RawSnrDb","FinalSnrDb",
    "EffectiveSensitivityDbm","SensitivityMarginDb",
    "Environment",
]
STAGE2_FEAT = [
    "Stage2RangeEstimateM",  # measured range (observable)
    "Stage2IsNLOS",          # NLOS flag (observable)
    "ImuActive",             # IMU in use (observable)
    "TimeSinceLastLoRaFixS", # time since last fix (observable)
    "ImuDriftEstimateM",     # estimated drift (observable)
]
TARGET = "FinalPositionErrorM"

LGBM_PARAMS = dict(
    objective="regression_l1", metric="mae",
    n_estimators=400, learning_rate=0.05, num_leaves=63,
    feature_fraction=0.8, bagging_fraction=0.8, bagging_freq=5,
    min_child_samples=50,
    n_jobs=LGBM_THREADS,     # <-- capped, not -1
    random_state=42, verbose=-1,
)

COMBOS = [(m, t) for m in [1,2,3,4] for t in ["WAN","MESH","HYBRID"]]


def worker_init():
    """Lower process priority so the laptop stays usable."""
    try:
        os.nice(NICE_LEVEL)
    except Exception:
        pass
    # Ignore Ctrl+C in workers; parent handles it
    signal.signal(signal.SIGINT, signal.SIG_IGN)


def load_combo(method, topo):
    """Load stratified 40% sample from one multimodal CSV, minimising RAM usage."""
    path = Path(DST) / "multimodal_M{}_T{}.csv".format(method, topo)
    if not path.exists():
        return None
    chunks = []
    rng = np.random.default_rng(42)
    for chunk in pd.read_csv(path, chunksize=CHUNK):
        n = max(1, int(len(chunk) * SAMPLE_FRAC))
        idx = rng.choice(len(chunk), size=n, replace=False)
        chunks.append(chunk.iloc[idx])
        del chunk   # free immediately
    df = pd.concat(chunks, ignore_index=True)
    del chunks
    return df


def eval_combo(method, topo):
    """Train + evaluate one variant. Returns result dict or None on error."""
    label = "M{}×{}".format(method, topo)
    result_path = Path(DST) / "stage2_result_M{}_T{}.json".format(method, topo)
    model_path  = Path(DST) / "stage2_M{}_T{}.pkl".format(method, topo)

    # Resume: skip if already done
    if result_path.exists() and model_path.exists():
        print("[{}] Already done — skipping.".format(label), flush=True)
        with open(result_path) as f:
            return json.load(f)

    t0 = time.time()
    print("[{}] Loading data...".format(label), flush=True)

    try:
        df = load_combo(method, topo)
        if df is None:
            print("[{}] MISSING CSV — skipped.".format(label), flush=True)
            return None

        feat_cols = LORA_FEAT + STAGE2_FEAT
        missing = [c for c in feat_cols + [TARGET] if c not in df.columns]
        if missing:
            print("[{}] Missing columns: {} — skipped.".format(label, missing), flush=True)
            return None

        valid = df[TARGET].notna()
        df = df[valid].copy()
        for c in STAGE2_FEAT:
            if c in df.columns:
                df[c] = df[c].fillna(0.0)

        # Keep mode column for FUSED/LORA_ONLY breakdown
        mode_col = df["FinalLocalizationMode"].copy()

        X = df[feat_cols]
        y = df[TARGET]

        print("[{}] {:,} rows — splitting & training...".format(label, len(df)), flush=True)

        # Shared deterministic split: written once, reused across all 15 variants.
        # Without this, each variant draws its own shuffled 15% test set from a
        # differently-sampled dataframe, making LORA_ONLY_MAE incomparable
        # across Method/Topology (observed: 32m gap between M1 and M2 variants).
        split_train_path = Path(DST) / "split_index_train.npy"
        split_test_path  = Path(DST) / "split_index_test.npy"
        n_total = len(X)
        if split_train_path.exists() and split_test_path.exists():
            # Load existing split (same indices for all variants)
            tr_idx = np.load(split_train_path, allow_pickle=True)
            te_idx = np.load(split_test_path,  allow_pickle=True)
            # Indices may be larger than current df if sample sizes differ slightly
            # — intersect with actual integer positions
            tr_idx = tr_idx[tr_idx < n_total]
            te_idx = te_idx[te_idx < n_total]
        else:
            # First variant: create and save the canonical split
            all_idx = np.arange(n_total)
            rng_split = np.random.default_rng(2026_07_11)  # fixed seed
            rng_split.shuffle(all_idx)
            cut = int(n_total * 0.85)
            tr_idx, te_idx = all_idx[:cut], all_idx[cut:]
            np.save(split_train_path, tr_idx)
            np.save(split_test_path,  te_idx)
            print("[{}] Saved shared split index: {:,} train / {:,} test".format(
                label, len(tr_idx), len(te_idx)), flush=True)

        X_arr = X.values
        y_arr = y.values
        X_tr = pd.DataFrame(X_arr[tr_idx], columns=X.columns)
        X_te = pd.DataFrame(X_arr[te_idx], columns=X.columns)
        y_tr = pd.Series(y_arr[tr_idx], name=y.name)
        y_te = pd.Series(y_arr[te_idx], name=y.name)
        mode_te = pd.Series(mode_col.values[te_idx], name="FinalLocalizationMode")

        tdoa_elig_rate = round((df["TdoaEligible"] == 1).mean() * 100, 1) if "TdoaEligible" in df.columns else 0.0

        # Free training data we no longer need
        del df, X, y

        model = lgb.LGBMRegressor(**LGBM_PARAMS)
        model.fit(X_tr, y_tr,
                  eval_set=[(X_te, y_te)],
                  callbacks=[lgb.early_stopping(30, verbose=False),
                             lgb.log_evaluation(999)])  # silence per-iteration logs

        preds  = model.predict(X_te)
        mae    = float(mean_absolute_error(y_te, preds))
        r2     = float(r2_score(y_te, preds))
        p50    = float(np.percentile(np.abs(preds - y_te), 50))
        p90    = float(np.percentile(np.abs(preds - y_te), 90))

        # Sanity check: MAE should be ~50-250m (not ~1-2m which indicates leakage)
        if mae < 10.0:
            print("[{}] WARNING: MAE={:.2f}m suspiciously small — residual leakage?".format(
                label, mae), flush=True)

        # FUSED / IMU_ONLY / LORA_ONLY split using FinalLocalizationMode
        fused_mask  = (mode_te == "FUSED")
        imu_mask    = (mode_te == "IMU_ONLY")    # new: handoff fired, Stage-2 ranging failed
        lora_mask   = (mode_te == "LORA_ONLY")
        fused_mae   = float(mean_absolute_error(y_te[fused_mask],  preds[fused_mask.values])) \
                      if fused_mask.any() else None
        imu_mae     = float(mean_absolute_error(y_te[imu_mask],    preds[imu_mask.values])) \
                      if imu_mask.any() else None
        lora_mae    = float(mean_absolute_error(y_te[lora_mask],   preds[lora_mask.values])) \
                      if lora_mask.any() else None
        fused_rate  = float(fused_mask.mean())
        imu_rate    = float(imu_mask.mean())

        # Improvement = FUSED MAE vs LORA_ONLY MAE (the real fusion benefit question)
        if fused_mae is not None and lora_mae is not None and lora_mae > 0:
            improvement_pct = (lora_mae - fused_mae) / lora_mae * 100
        else:
            improvement_pct = 0.0

        elapsed = time.time() - t0
        imu_pct = imu_rate * 100
        print("[{}] Done in {:.0f}s — MAE={:.3f}m  P90={:.3f}m  "
              "FUSED={:.0f}%  IMU={:.0f}%  LORA={:.0f}%  Improv={:.1f}%".format(
            label, elapsed, mae, p90,
            fused_rate*100, imu_pct, lora_mask.mean()*100,
            improvement_pct), flush=True)

        # Save model immediately (crash-safe)
        with open(model_path, "wb") as f:
            pickle.dump({"model": model, "features": feat_cols}, f)

        row = {
            "Method": method, "Topology": topo,
            "Stage2Modality": "2.4GHZ_RTT" if method in (1,2) else "UWB",
            "IMU": "Yes" if method in (2,4) else "No",
            "FusedRate_pct": round(fused_rate*100, 1),
            "MAE_m": round(mae, 3),
            "P50_m": round(p50, 3),
            "P90_m": round(p90, 3),
            "R2": round(r2, 4),
            "Improvement_pct": round(improvement_pct, 1),
            "FUSED_MAE_m": round(fused_mae, 3) if fused_mae is not None else None,
            "IMU_ONLY_MAE_m": round(imu_mae, 3) if imu_mae is not None else None,
            "LORA_ONLY_MAE_m": round(lora_mae, 3) if lora_mae is not None else None,
            "FusedRows_in_test": int(fused_mask.sum()),
            "IMUOnlyRows_in_test": int(imu_mask.sum()),
            "LoraOnlyRows_in_test": int(lora_mask.sum()),
            "TotalRows_in_test": int(len(y_te)),
            "TrainTimeSec": round(elapsed, 0),
            "TdoaEligRate_pct": tdoa_elig_rate,
            "TdoaRateSource": "actual_data",
        }

        # Save result JSON immediately (crash-safe)
        with open(result_path, "w") as f:
            json.dump(row, f, indent=2)

        return row

    except Exception as e:
        print("[{}] ERROR: {}".format(label, e), flush=True)
        return None


def collect_existing_results():
    """Collect any already-completed JSON results (in case of partial run)."""
    results = []
    for m, t in COMBOS:
        p = Path(DST) / "stage2_result_M{}_T{}.json".format(m, t)
        if p.exists():
            try:
                with open(p) as f:
                    results.append(json.load(f))
            except Exception:
                pass
    return results


def main():
    print("Stage 2 Parallel Training")
    print("  Parallel variants : {}".format(PARALLEL_JOBS))
    print("  LightGBM threads  : {} per model ({} total)".format(
        LGBM_THREADS, PARALLEL_JOBS * LGBM_THREADS))
    print("  Sample fraction   : {}% of 5M rows per variant".format(int(SAMPLE_FRAC*100)))
    print("  Nice level        : {} (laptop stays responsive)".format(NICE_LEVEL))
    print()

    # Check which variants still need training
    todo = []
    for m, t in COMBOS:
        result_path = Path(DST) / "stage2_result_M{}_T{}.json".format(m, t)
        model_path  = Path(DST) / "stage2_M{}_T{}.pkl".format(m, t)
        if not (result_path.exists() and model_path.exists()):
            todo.append((m, t))
        else:
            print("  [SKIP] M{}×{} already complete".format(m, t))

    print("\n{} variants to train, {} already done.\n".format(len(todo), 12-len(todo)))

    if todo:
        with ProcessPoolExecutor(max_workers=PARALLEL_JOBS,
                                 initializer=worker_init) as pool:
            futures = {pool.submit(eval_combo, m, t): (m, t) for m, t in todo}
            try:
                for fut in as_completed(futures):
                    m, t = futures[fut]
                    try:
                        row = fut.result()
                        if row is None:
                            print("  [FAILED] M{}×{}".format(m, t))
                    except Exception as e:
                        print("  [EXCEPTION] M{}×{}: {}".format(m, t, e))
            except KeyboardInterrupt:
                print("\nInterrupted — already-completed variants are saved.")
                pool.shutdown(wait=False)
                sys.exit(1)

    # Collect all results (including previously completed ones)
    results = collect_existing_results()

    if not results:
        print("No results. Ensure all 12 multimodal CSVs exist.")
        sys.exit(1)

    tbl = pd.DataFrame(results).sort_values(["Method","Topology"])
    tbl.to_csv(OUT_TBL, index=False)

    print("\n" + "="*70)
    print("FINAL COMPARISON TABLE")
    print("="*70)
    display_cols = ["Method","Topology","Stage2Modality","IMU",
                    "FusedRate_pct","MAE_m","P90_m","Improvement_pct"]
    print(tbl[display_cols].to_string(index=False))
    print("\nSaved: {}".format(OUT_TBL))

    if len(tbl) > 0:
        best = tbl.loc[tbl["MAE_m"].idxmin()]
        print("\nBest by MAE:")
        print("  Method {} x {} ({}) : MAE={:.3f}m  P90={:.3f}m".format(
            int(best["Method"]), best["Topology"], best["Stage2Modality"],
            best["MAE_m"], best["P90_m"]))


if __name__ == "__main__":
    main()
