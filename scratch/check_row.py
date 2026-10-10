import pandas as pd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)

df = pd.read_csv(ROOT / 'results/ml_training_dataset.csv')
cols = ['PathLossDb', 'ShadowingDb', 'FadingDb', 'NlosDb', 'BuildingEntryLossDb', 'WallPenetrationLossDb', 'FloorAttenuationDb', 'BodyShadowingLossDb', 'TemporalBlockageLossDb', 'AtmosphericDb', 'FoliageDb', 'TerrainDb', 'InterferencePenaltyDb', 'HwTxVariationDb']
df['comp_sum'] = df[cols].sum(axis=1)
df['gap'] = df['TotalLossDb'] - df['comp_sum']
print(df[cols + ['TotalLossDb', 'gap']].iloc[2])
