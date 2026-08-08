#!/usr/bin/env python3
"""
fast_fix_improvement.py
Reads the 12 already-saved stage2 result JSONs and produces a corrected
final_comparison_table.csv in seconds — no CSV reloading needed.

Improvement_pct = how much better FUSED_MAE is vs LORA_ONLY_MAE
(on fused rows vs non-fused rows of the same model).
This is the meaningful metric: does fusion help on the rows it activates?
"""
import json
import pandas as pd
from pathlib import Path

DST     = "./results"
OUT_TBL = "./results/final_comparison_table.csv"

COMBOS = [(m, t) for m in [1,2,3,4] for t in ["WAN","MESH","HYBRID"]]

results = []
for m, t in COMBOS:
    p = Path(DST) / "stage2_result_M{}_T{}.json".format(m, t)
    if not p.exists():
        print("MISSING: M{} x {}".format(m, t))
        continue
    with open(p) as f:
        row = json.load(f)

    fused_mae    = row.get("FUSED_MAE_m")
    lora_mae     = row.get("LORA_ONLY_MAE_m")
    fused_rate   = row.get("FusedRate_pct", 0.0)

    # Correct improvement: fused rows MAE vs lora-only rows MAE
    # (proxy for: does fusion help relative to what lora-only rows show?)
    # More meaningful: fused MAE vs LORA_ONLY_MAE (same-model lora rows as baseline)
    if fused_mae is not None and lora_mae is not None and lora_mae > 0:
        improvement_pct = (lora_mae - fused_mae) / lora_mae * 100
    else:
        improvement_pct = 0.0

    row["Improvement_pct"] = round(improvement_pct, 1)
    results.append(row)

    print("M{} x {:6s} | Fused={:.1f}% | FUSED_MAE={:>7.3f}m | LORA_MAE={:>7.3f}m | Improvement={:+.1f}%".format(
        m, t,
        fused_rate,
        fused_mae if fused_mae is not None else float("nan"),
        lora_mae  if lora_mae  is not None else float("nan"),
        improvement_pct))

tbl = pd.DataFrame(results).sort_values(["Method","Topology"])
tbl.to_csv(OUT_TBL, index=False)

print()
print("="*70)
print("FINAL COMPARISON TABLE (corrected improvement)")
print("="*70)
cols = ["Method","Topology","Stage2Modality","IMU",
        "FusedRate_pct","MAE_m","P90_m",
        "FUSED_MAE_m","LORA_ONLY_MAE_m","Improvement_pct"]
avail = [c for c in cols if c in tbl.columns]
print(tbl[avail].to_string(index=False))
print()
print("Saved:", OUT_TBL)

if len(tbl) > 0:
    best_imp = tbl.loc[tbl["Improvement_pct"].idxmax()]
    best_mae = tbl.loc[tbl["MAE_m"].idxmin()]
    print()
    print("Best fusion improvement (fused rows):")
    print("  Method {} x {} ({}, IMU={}) : Improvement={:.1f}%  FUSED_MAE={:.3f}m".format(
        int(best_imp["Method"]), best_imp["Topology"], best_imp["Stage2Modality"],
        best_imp["IMU"], best_imp["Improvement_pct"],
        best_imp.get("FUSED_MAE_m") or 0))
    print()
    print("Best overall MAE:")
    print("  Method {} x {} ({}, IMU={}) : MAE={:.3f}m  P90={:.3f}m".format(
        int(best_mae["Method"]), best_mae["Topology"], best_mae["Stage2Modality"],
        best_mae["IMU"], best_mae["MAE_m"], best_mae["P90_m"]))
