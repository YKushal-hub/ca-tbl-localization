import pandas as pd
import numpy as np
import sys

def analyze_dataset(file_path):
    print(f"Loading dataset: {file_path}")
    df = pd.read_csv(file_path)
    
    print("\n--- Distribution of Environments ---")
    counts = df['Environment'].value_counts().sort_index()
    pcts = df['Environment'].value_counts(normalize=True).sort_index() * 100
    for env in counts.index:
        print(f"Env {env}: {counts[env]} ({pcts[env]:.2f}%)")
        
    print("\n--- PLE Analysis ---")
    print("Expected: n=2.0-2.3 (LOS), n=2.8-4.0 (Urban NLOS), n=1.2-1.8 (Tunnel)")
    fspl_d0 = 20 * np.log10(4 * np.pi * 1.0 / (299792458.0 / 868e6))
    df['PLE'] = (df['PathLossDb'] - fspl_d0) / (10 * np.log10(np.maximum(1.0, df['DistanceM'])))
    
    for env in sorted(df['Environment'].unique()):
        env_df = df[df['Environment'] == env]
        los_ple = env_df[env_df['NlosDb'] == 0.0]['PLE'].mean()
        nlos_ple = env_df[env_df['NlosDb'] > 0.0]['PLE'].mean()
        max_loss = (14.0 - env_df['TrueRssiDbm']).max()
        print(f"Env {env}: LOS PLE = {los_ple:.3f}, NLOS PLE = {nlos_ple:.3f} | Max PathLoss = {max_loss:.1f} dB")
        
    print("\n--- Sub.21 Range Error Check ---")
    sub21 = df[(df['Environment'] == 1) & (df['NlosDb'] == 0.0) & (df['RawSnrDb'] > 10)]
    if len(sub21) > 0:
        print(f"Sub.21 Mean RangeError: {sub21['RangeErrorM'].mean():.2f}m (Expected ~313.3m)")
    else:
        print("No Sub.21 cases found.")
        
    print("\n--- Body Shadowing Max Loss Check ---")
    print(f"Max Body Shadowing Loss: {df['BodyShadowingLossDb'].max():.2f} dB (Expected <= 6.0dB)")

if __name__ == "__main__":
    analyze_dataset(sys.argv[1])
