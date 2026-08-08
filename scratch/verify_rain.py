import pandas as pd
df = pd.read_csv('results/ml_training_dataset.csv', usecols=['RainRateMmHr','Environment','FoliageDb'])
print(f'RainRateMmHr max: {df["RainRateMmHr"].max():.2f} mm/hr')
print(f'RainRateMmHr mean: {df["RainRateMmHr"].mean():.3f} mm/hr')
print(f'Rows with rain > 15 mm/hr: {(df["RainRateMmHr"] > 15).sum():,}')
print(f'Rows with rain > 30 mm/hr: {(df["RainRateMmHr"] > 30).sum():,}')

# Coupling 4 at high rain rates (only measurable if rain > 10 mm/hr exists)
high_rain = df[(df['Environment']==1) & (df['RainRateMmHr'] > 10)]
if len(high_rain) > 100:
    r = high_rain[['RainRateMmHr','FoliageDb']].corr().iloc[0,1]
    print(f'Coupling 4 r at rain>10: {r:.3f} (target > 0.30)')
else:
    print('WARNING: insufficient high-rain rows for Coupling 4 validation')
    print('P3 fix (rain sweep to 100 mm/hr) was NOT applied to this dataset')
