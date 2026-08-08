import pandas as pd
import numpy as np

# Load dataset A to check PLE
df = pd.read_csv('results/dataset_A_seed7.csv')
FSPL_1m = 31.21
df['implied_n'] = (df['path_loss_db'] - FSPL_1m) / (10 * np.log10(df['distance_m']))
print('Implied PLE distribution Dataset A:')
print(df.groupby('environment')['implied_n'].describe())
