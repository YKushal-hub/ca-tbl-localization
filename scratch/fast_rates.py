"""Per-variant localisation-mode rates for the multimodal datasets.

Reads results/multimodal_M{m}_T{t}.csv for every method m and topology t and prints
the FUSED / IMU_ONLY / LORA_ONLY row fractions. Run from anywhere; paths are relative
to the repository root.
"""
import pandas as pd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)
DST = ROOT / 'results'
COL = 'FinalLocalizationMode'

for m in [0, 1, 2, 3, 4]:
    for t in ['WAN', 'MESH', 'HYBRID']:
        path = DST / f'multimodal_M{m}_T{t}.csv'
        if not path.exists():
            print(f'M{m} T{t}: missing {path}')
            continue
        df = pd.read_csv(path, usecols=[COL])
        total = len(df)
        fused = int((df[COL] == 'FUSED').sum())
        imu_only = int((df[COL] == 'IMU_ONLY').sum())
        lora_only = int((df[COL] == 'LORA_ONLY').sum())
        if total == 0:
            print(f'M{m} T{t}: empty')
            continue
        print(f'M{m} T{t}: rows={total} '
              f'fused={fused} ({100 * fused / total:.2f}%) '
              f'imu_only={imu_only} ({100 * imu_only / total:.2f}%) '
              f'lora_only={lora_only} ({100 * lora_only / total:.2f}%)')
