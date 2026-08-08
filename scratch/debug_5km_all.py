import pandas as pd
df = pd.read_csv('results/smoke_test_50k.csv')
urban = df[df['Environment'].isin([3,4])]
band = urban[(urban['DistanceM']>=4000) & (urban['DistanceM']<=6000)]
cols = ['DistanceM', 'PathLossDb', 'ShadowingDb', 'FadingDb', 'NlosDb', 'BuildingEntryLossDb', 'WallPenetrationLossDb', 'FloorAttenuationDb', 'BodyShadowingLossDb', 'TemporalBlockageLossDb', 'AtmosphericDb', 'FoliageDb', 'TerrainDb', 'InterferencePenaltyDb', 'DopplerPenaltyDb', 'HwTxVariationDb', 'TotalLossDb', 'TrueRssiDbm']
print(band[cols].mean())
