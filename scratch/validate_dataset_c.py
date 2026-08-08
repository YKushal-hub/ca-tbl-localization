import pandas as pd
import numpy as np

# Dataset C is the per-packet file - check the summary file for aggregated stats
df_sum = pd.read_csv('results/dataset_C_summary_seed7.csv')
print("Summary columns:", df_sum.columns.tolist())
print()

for env, grp in df_sum.groupby('environment'):
    mu = grp['nlos_excess_attenuation_mean_db'].mean()
    sig = grp['nlos_excess_attenuation_stddev_db'].mean()
    prob = grp['empirical_nlos_probability'].mean()
    print(f"{env}: mean_attn={mu:.2f} dB  sigma={sig:.2f} dB  nlos_prob={prob:.3f}")

print()

# Also check per-packet file to compute raw stats
df = pd.read_csv('results/dataset_C_seed7.csv')
print("Per-packet columns:", df.columns.tolist())
nlos_only = df[df['was_nlos'] == 1]
print(f"\ndense_urban NLOS penalty: mean={nlos_only[nlos_only['environment']=='dense_urban']['nlos_penalty_db'].mean():.2f}  sigma={nlos_only[nlos_only['environment']=='dense_urban']['nlos_penalty_db'].std():.2f}")
print(f"urban_moderate NLOS penalty: mean={nlos_only[nlos_only['environment']=='urban_moderate']['nlos_penalty_db'].mean():.2f}  sigma={nlos_only[nlos_only['environment']=='urban_moderate']['nlos_penalty_db'].std():.2f}")
print(f"\ndense_urban NLOS probability: {(df[df['environment']=='dense_urban']['was_nlos'].mean()):.3f}")
print(f"urban_moderate NLOS probability: {(df[df['environment']=='urban_moderate']['was_nlos'].mean()):.3f}")

# Check schema now has 43 columns
print(f"\nTotal columns in per-packet CSV: {len(df.columns)}")
print("Last 14 cols:", df.columns.tolist()[-14:])
