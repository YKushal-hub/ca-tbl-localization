import pandas as pd
df = pd.read_csv('results/ml_training_dataset_test_500k.csv')
u3 = df[(df['Environment'] == 3) & (df['DistanceM'] > 950) & (df['DistanceM'] < 1050)]
print(u3[['DistanceM', 'PathLossDb', 'NlosDb', 'ShadowingDb', 'FadingDb', 'BuildingEntryLossDb', 'TotalLossDb', 'TrueRssiDbm']].head(10))
print('\nMEANS:')
print(u3[['DistanceM', 'PathLossDb', 'NlosDb', 'ShadowingDb', 'FadingDb', 'BuildingEntryLossDb', 'TotalLossDb', 'TrueRssiDbm']].mean())
