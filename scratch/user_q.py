import pandas as pd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)

df = pd.read_csv(ROOT / 'results/ml_training_dataset.csv')
print(f'Max distance: {df["DistanceM"].max()}')
print(f'AtmosphericDb min: {df["AtmosphericDb"].min()}')
print(f'AtmosphericDb mean: {df["AtmosphericDb"].mean()}')
