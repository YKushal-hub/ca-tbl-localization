import pandas as pd
df = pd.read_csv('results/ml_training_dataset.csv')
print(f"Max Rain Rate: {df['RainRateMmHr'].max()}")
print(f"Tunnel Environment Count: {len(df[df['Environment'] == 8])}")
print(f"IsTunnel == 1 Count: {len(df[df['IsTunnel'] == 1])}")
