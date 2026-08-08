import pandas as pd
df = pd.read_csv('/home/kushal/ns/ns-3-dev/results/ml_training_dataset.csv')
print(f'Max distance: {df["DistanceM"].max()}')
print(f'AtmosphericDb min: {df["AtmosphericDb"].min()}')
print(f'AtmosphericDb mean: {df["AtmosphericDb"].mean()}')
