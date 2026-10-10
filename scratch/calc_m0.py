import pandas as pd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)


for top in ['WAN', 'MESH', 'HYBRID']:
    file = ROOT / f'results/multimodal_M0_T{top}.csv'
    try:
        df = pd.read_csv(file)
        if 'Error_m' in df.columns:
            mae = df['Error_m'].mean()
            print(f"Method 0, {top} Exact Overall_MAE_m: {mae:.15f}")
        else:
            print(f"Method 0, {top} missing Error_m")
    except Exception as e:
        print(f"Failed {top}: {e}")
