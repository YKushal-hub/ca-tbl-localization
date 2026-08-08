import pandas as pd
import numpy as np

print("Loading dataset...")
df = pd.read_csv('results/ml_training_dataset.csv')

nlos_mask = df['NlosDb'] > 0
los_mask  = df['NlosDb'] == 0

print("Global NLOS fading std: ", df[nlos_mask]['FadingDb'].std())
print("Global LOS fading std:  ", df[los_mask]['FadingDb'].std())

print("\nFading std by environment:")
for env in sorted(df['Environment'].unique()):
    edf = df[df['Environment'] == env]
    env_nlos = edf[edf['NlosDb'] > 0]
    env_los  = edf[edf['NlosDb'] == 0]
    
    nlos_std = env_nlos['FadingDb'].std() if len(env_nlos) > 0 else 0
    los_std  = env_los['FadingDb'].std() if len(env_los) > 0 else 0
    ratio    = (nlos_std / los_std) if los_std > 0 else 0
    print(f"Env {env}: NLOS={nlos_std:.2f}, LOS={los_std:.2f}, Ratio={ratio:.2f}")

