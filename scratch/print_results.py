#!/usr/bin/env python3
import json, glob
rows = [json.load(open(f)) for f in sorted(glob.glob("results/stage2_result_M*.json"))]
print(f"{'Variant':12s} {'Modality':12s} {'IMU':3s} {'Fused%':7s} {'MAE':8s} {'FUSED_MAE':10s} {'LORA_MAE':9s} {'Impr%':7s}")
print("-"*75)
for r in rows:
    print(f"M{r['Method']}x{r['Topology']:6s}  {r['Stage2Modality']:12s}  {r['IMU']:3s}  "
          f"{r['FusedRate_pct']:5.1f}%  "
          f"{r['MAE_m']:7.2f}m  "
          f"{r.get('FUSED_MAE_m') or 0:7.3f}m  "
          f"{r.get('LORA_ONLY_MAE_m') or 0:7.1f}m  "
          f"{r['Improvement_pct']:5.1f}%")
print(f"\n{len(rows)}/12 complete")
