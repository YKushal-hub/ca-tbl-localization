#!/usr/bin/env python3
"""
watch_retrain.py -- Poll for completed stage2 result JSONs.
Prints a running summary table. Assembles final_comparison_table.csv once done.
Run as: python3 scratch/watch_retrain.py
"""
import json, glob, time, os
import pandas as pd

DST = "./results"
EXPECTED = 12

def load_results():
    rows = []
    for f in sorted(glob.glob(f"{DST}/stage2_result_M*.json")):
        try:
            rows.append(json.load(open(f)))
        except Exception:
            pass
    return rows

def fmt_row(r):
    fused_mae = r.get("FUSED_MAE_m")
    lora_mae  = r.get("LORA_ONLY_MAE_m")
    return (
        f"  M{r['Method']}x{r['Topology']:6s}  {r.get('Stage2Modality','?'):10s}  "
        f"IMU={r.get('IMU','?'):3s}  "
        f"FusedRate={r.get('FusedRate_pct',0):4.1f}%  "
        f"MAE={r.get('MAE_m',0):6.1f}m  "
        f"FUSED_MAE={fused_mae:5.3f}m  "
        f"LORA_MAE={lora_mae:6.1f}m  "
        f"Impr={r.get('Improvement_pct',0):4.1f}%"
        if fused_mae and lora_mae else
        f"  M{r['Method']}x{r['Topology']:6s}  MAE={r.get('MAE_m',0):6.1f}m"
    )

while True:
    rows = load_results()
    n = len(rows)
    print(f"\n[{time.strftime('%H:%M:%S')}] {n}/{EXPECTED} variants complete")
    print("-" * 100)
    for r in rows:
        print(fmt_row(r))
    
    if n == EXPECTED:
        print("\n=== ALL DONE — assembling final_comparison_table.csv ===")
        tbl = pd.DataFrame(rows).sort_values(["Method", "Topology"])
        out = f"{DST}/final_comparison_table.csv"
        tbl.to_csv(out, index=False)
        print(f"Saved: {out}")
        print()
        display_cols = ["Method","Topology","Stage2Modality","IMU",
                        "FusedRate_pct","MAE_m","P90_m",
                        "FUSED_MAE_m","LORA_ONLY_MAE_m","Improvement_pct"]
        print(tbl[display_cols].to_string(index=False))
        print()
        best = tbl.loc[tbl["Improvement_pct"].idxmax()]
        print(f"Best Improvement: M{int(best['Method'])}x{best['Topology']} "
              f"({best['Stage2Modality']}, IMU={best['IMU']}) "
              f"Impr={best['Improvement_pct']:.1f}% "
              f"FUSED_MAE={best['FUSED_MAE_m']:.3f}m")
        print()
        print("TOPOLOGY CAVEAT: Rates are literature-derived, not from simulator.")
        break
    
    if n < EXPECTED:
        remaining = EXPECTED - n
        print(f"  Waiting for {remaining} more... (checking again in 5 min)")
        time.sleep(300)
