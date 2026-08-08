#!/usr/bin/env python3
import pandas as pd, numpy as np

for meth in ['M1','M2']:
    df = next(pd.read_csv(f'results/multimodal_{meth}_TWAN.csv', chunksize=500000))
    lo = df[df['FinalLocalizationMode']=='LORA_ONLY']
    diff = lo['FinalPositionErrorM'] - lo['PositionErrorM']
    print(f"{meth} LORA_ONLY (first 500k): n={len(lo):,}")
    print(f"  FinalPosErr mean={lo['FinalPositionErrorM'].mean():.1f}m")
    print(f"  PosErr      mean={lo['PositionErrorM'].mean():.1f}m")
    print(f"  diff(F-P):  mean={diff.mean():.5f}  max={diff.max():.5f}  nonzero%={(diff!=0).mean()*100:.2f}%")
    print()

# Also check what the RETRAIN reported as LORA_ONLY_MAE (from the JSON)
import json
for meth in ['M1','M2']:
    j = json.load(open(f'results/stage2_result_{meth}_TWAN.json'))
    print(f"{meth}xWAN JSON: LORA_ONLY_MAE={j.get('LORA_ONLY_MAE_m','?')}m  LORA_rows={j.get('LoraOnlyRows_in_test','?')}")
