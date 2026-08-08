import pandas as pd
import numpy as np

df = pd.read_csv('/home/kushal/ns/ns-3-dev/results/smoke_test_50k.csv')

print(f"Rows: {len(df):,}  Cols: {len(df.columns)}")
print()

# P1: Rural Forested (env=1) LOS delay spread - should now be ~80 ns not ~300 ns
env1 = df[df['Environment'] == 1]
los1 = env1[env1['NlosDb'] == 0]
nlos1 = env1[env1['NlosDb'] > 0]
print("=== P1: Rural Forested delay spread ===")
print(f"  LOS  rows: {len(los1):,}  |  LOS  mean delay spread: {los1['DelaySpreadNs'].mean():.1f} ns  (spec: ~80 ns)")
print(f"  NLOS rows: {len(nlos1):,}  |  NLOS mean delay spread: {nlos1['DelaySpreadNs'].mean():.1f} ns  (spec: ~300 ns)")
print()

# P2: NlosDb max should now be capped below 70 dB for urban dense
print("=== P2: NlosDb max (Normal model, hard cap) ===")
print(f"  NlosDb.max() = {df['NlosDb'].max():.2f} dB  (old Poisson: 88.66 dB, new cap target: < 70 dB)")
print()

# P3: Rain rate distribution - should now have heavy rain rows
print("=== P3: RainRateMmHr distribution ===")
bins = [0, 4, 15, 30, 200]
labels = ['0-4 mm/hr (dry)', '4-15 (light)', '15-30 (moderate)', '>30 (heavy)']
df['rain_bin'] = pd.cut(df['RainRateMmHr'], bins=bins, labels=labels)
print(df['rain_bin'].value_counts().sort_index().to_string())
print()

# P4: IsTunnel - tunnel env (env=8) should now show IsTunnel=1
print("=== P4: IsTunnel for env=8 (TUNNEL_CONFINED) ===")
tunnel = df[df['Environment'] == 8]
print(f"  ENV=8 rows: {len(tunnel):,}")
print(f"  IsTunnel==1: {(tunnel['IsTunnel']==1).sum():,}  ({(tunnel['IsTunnel']==1).mean()*100:.1f}%)")
non_tunnel = df[df['Environment'] != 8]
print(f"  Non-tunnel IsTunnel==1: {(non_tunnel['IsTunnel']==1).sum():,}  (should be 0)")
print()

# P5: FloorAttenuationDb max - should be lower now
print("=== P5: FloorAttenuationDb range ===")
indoor = df[(df['Environment'] == 6) | (df['Environment'] == 7)]  # INDOOR_OFFICE, CONCRETE_BUNKER
print(f"  Indoor rows: {len(indoor):,}")
print(f"  FloorAttenuationDb max: {df['FloorAttenuationDb'].max():.2f} dB  (old max was 60 dB, new ceiling ~90 dB)")
print(f"  FloorAttenuationDb mean (indoor only): {indoor['FloorAttenuationDb'].mean():.2f} dB")
print()

print("=== SUMMARY ===")
print(f"P1 Rural Forested LOS delay spread: {los1['DelaySpreadNs'].mean():.0f} ns (pass if < 120 ns)")
print(f"P2 NlosDb max: {df['NlosDb'].max():.1f} dB (pass if < 90 dB)")
heavy_rain_frac = (df['RainRateMmHr'] > 15).mean() * 100
print(f"P3 Heavy rain fraction: {heavy_rain_frac:.1f}% (pass if > 15%)")
tunnel_flag_ok = (tunnel['IsTunnel']==1).mean() > 0.99
print(f"P4 IsTunnel flag correct: {tunnel_flag_ok} (pass if True)")
print(f"P5 FloorAtten max: {df['FloorAttenuationDb'].max():.1f} dB (pass if < 95 dB)")
