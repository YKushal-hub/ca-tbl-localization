import pandas as pd
df = pd.read_csv('/home/kushal/ns/ns-3-dev/results/ml_training_dataset.csv')
cols = ['PathLossDb', 'ShadowingDb', 'FadingDb', 'NlosDb', 'BuildingEntryLossDb', 'WallPenetrationLossDb', 'FloorAttenuationDb', 'BodyShadowingLossDb', 'TemporalBlockageLossDb', 'AtmosphericDb', 'FoliageDb', 'TerrainDb', 'InterferencePenaltyDb', 'HwTxVariationDb']
df['comp_sum'] = df[cols].sum(axis=1)
df['gap'] = df['TotalLossDb'] - df['comp_sum']
print(df[['TotalLossDb', 'comp_sum', 'gap']].head(20))
