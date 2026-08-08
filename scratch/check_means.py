import pandas as pd
df = pd.read_csv('/home/kushal/ns/ns-3-dev/results/ml_training_dataset.csv')
cols = ['TotalLossDb', 'PathLossDb', 'ShadowingDb', 'FadingDb', 'NlosDb', 'BuildingEntryLossDb', 'WallPenetrationLossDb', 'FloorAttenuationDb', 'BodyShadowingLossDb', 'TemporalBlockageLossDb', 'AtmosphericDb', 'FoliageDb', 'TerrainDb', 'InterferencePenaltyDb', 'HwTxVariationDb']
print(df[cols].mean())
