import pandas as pd
import numpy as np

print("Loading 5M dataset...")
df = pd.read_csv('results/ml_training_dataset.csv')

components = ['PathLossDb', 'ShadowingDb', 'FadingDb', 'NlosDb', 'BuildingEntryLossDb', 
              'WallPenetrationLossDb', 'FloorAttenuationDb', 'BodyShadowingLossDb', 
              'TemporalBlockageLossDb', 'AtmosphericDb', 'FoliageDb', 'TerrainDb', 
              'InterferencePenaltyDb', 'HwTxVariationDb']

sum_loss = df[components].sum(axis=1)
gap = (df['TotalLossDb'] - sum_loss).abs().mean()

zero_body_shadow_frac = (df['BodyShadowingLossDb'] == 0).mean()

nonzero_err = df.loc[df['PositionErrorM'] > 0, 'PositionErrorM']
sub21_mean = nonzero_err.mean()

print(f"TotalLossDb mean gap: {gap:.6f} dB (target < 0.01)")
print(f"Body shadow zero fraction: {zero_body_shadow_frac * 100:.2f}% (target ~ 80%)")
print(f"Sub.21 non-zero mean: {sub21_mean:.2f} m (target ~ 313)")
