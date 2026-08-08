import pandas as pd
df = pd.read_csv('results/geometric_training_dataset.csv')
print("Total rows:", len(df))
print(df.groupby('Topology')['TdoaEligible'].sum())
