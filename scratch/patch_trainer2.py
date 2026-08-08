#!/usr/bin/env python3
"""patch_trainer2.py -- Add IMU_ONLY MAE to trainer and remove stale TOPOLOGY_NOTE."""
import sys

PATH = "/home/kushal/ns/ns-3-dev/scratch/train_stage2_parallel.py"
with open(PATH) as f:
    src = f.read()

# ── Patch 1: add IMU_ONLY mask alongside FUSED/LORA_ONLY ─────────────────
OLD_MASKS = """        # FUSED vs LORA_ONLY split using actual FinalLocalizationMode column
        fused_mask = (mode_te == "FUSED")
        lora_mask  = (mode_te == "LORA_ONLY")
        fused_mae  = float(mean_absolute_error(y_te[fused_mask], preds[fused_mask.values])) \\
                     if fused_mask.any() else None
        lora_mae   = float(mean_absolute_error(y_te[lora_mask],  preds[lora_mask.values])) \\
                     if lora_mask.any() else None
        fused_rate = float(fused_mask.mean())"""

NEW_MASKS = """        # FUSED / IMU_ONLY / LORA_ONLY split using FinalLocalizationMode
        fused_mask  = (mode_te == "FUSED")
        imu_mask    = (mode_te == "IMU_ONLY")    # new: handoff fired, Stage-2 ranging failed
        lora_mask   = (mode_te == "LORA_ONLY")
        fused_mae   = float(mean_absolute_error(y_te[fused_mask],  preds[fused_mask.values])) \\
                      if fused_mask.any() else None
        imu_mae     = float(mean_absolute_error(y_te[imu_mask],    preds[imu_mask.values])) \\
                      if imu_mask.any() else None
        lora_mae    = float(mean_absolute_error(y_te[lora_mask],   preds[lora_mask.values])) \\
                      if lora_mask.any() else None
        fused_rate  = float(fused_mask.mean())
        imu_rate    = float(imu_mask.mean())"""

if OLD_MASKS not in src:
    print("ERROR: mask pattern not found"); sys.exit(1)
src = src.replace(OLD_MASKS, NEW_MASKS, 1)

# ── Patch 2: add IMU_ONLY fields to result row, remove stale TOPOLOGY_NOTE ─
OLD_ROW = """            \"FUSED_MAE_m\": round(fused_mae, 3) if fused_mae is not None else None,
            \"LORA_ONLY_MAE_m\": round(lora_mae, 3) if lora_mae is not None else None,
            \"FusedRows_in_test\": int(fused_mask.sum()),
            \"LoraOnlyRows_in_test\": int(lora_mask.sum()),
            \"TestRows\": int(len(y_te)),
            \"TrainTimeSec\": round(elapsed, 0),
            \"TOPOLOGY_NOTE\": \"TDOA rates are literature-derived, not from simulator\","""

NEW_ROW = """            \"FUSED_MAE_m\": round(fused_mae, 3) if fused_mae is not None else None,
            \"IMU_ONLY_MAE_m\": round(imu_mae, 3) if imu_mae is not None else None,
            \"LORA_ONLY_MAE_m\": round(lora_mae, 3) if lora_mae is not None else None,
            \"FusedRows_in_test\": int(fused_mask.sum()),
            \"IMUOnlyRows_in_test\": int(imu_mask.sum()),
            \"LoraOnlyRows_in_test\": int(lora_mask.sum()),
            \"TotalRows_in_test\": int(len(y_te)),
            \"TrainTimeSec\": round(elapsed, 0),
            \"TdoaRateSource\": \"topology_observability_sim_100runs\","""

if OLD_ROW not in src:
    print("ERROR: result row pattern not found"); sys.exit(1)
src = src.replace(OLD_ROW, NEW_ROW, 1)

# ── Patch 3: update print statement to include IMU_ONLY count ────────────
OLD_PRINT = """        print(\"[{}] Done in {:.0f}s — MAE={:.3f}m  P90={:.3f}m  Improvement={:.1f}%\".format(
            label, elapsed, mae, p90, improvement_pct), flush=True)"""

NEW_PRINT = """        imu_pct = imu_rate * 100
        print(\"[{}] Done in {:.0f}s — MAE={:.3f}m  P90={:.3f}m  \"\n              \"FUSED={:.0f}%  IMU={:.0f}%  LORA={:.0f}%  Improv={:.1f}%\".format(
            label, elapsed, mae, p90,
            fused_rate*100, imu_pct, lora_mask.mean()*100,
            improvement_pct), flush=True)"""

if OLD_PRINT not in src:
    print("ERROR: print pattern not found"); sys.exit(1)
src = src.replace(OLD_PRINT, NEW_PRINT, 1)

with open(PATH, "w") as f:
    f.write(src)

assert "IMU_ONLY_MAE_m" in src, "Patch FAILED"
assert "TOPOLOGY_NOTE" not in src, "Old TOPOLOGY_NOTE still present"
assert "TdoaRateSource" in src, "New source field missing"
print("train_stage2_parallel.py patched: IMU_ONLY MAE added, TOPOLOGY_NOTE removed.")
