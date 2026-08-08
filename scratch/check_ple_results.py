import pandas as pd
import numpy as np

df = pd.read_csv('results/dataset_A_summary_seed7.csv')
ple_results = {}
for env in df['environment'].unique():
    env_df = df[df['environment'] == env]
    d_log10 = np.log10(env_df['distance_m'])
    pl = env_df['path_loss_mean']
    coef = np.polyfit(10 * d_log10, pl, 1)
    ple = coef[0]
    ple_results[env] = ple
print(ple_results)
print(np.mean(list(ple_results.values())))
