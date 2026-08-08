import pandas as pd

# Calculate theoretical NLOS caps per env
envs = {
    'URBAN_DENSE':     {'atten': 28, 'sigma': 14, 'diff': 12},
    'CONCRETE_BUNKER': {'atten': 20, 'sigma': 8,  'diff': 2},
    'URBAN_MODERATE':  {'atten': 25, 'sigma': 12, 'diff': 8},
}
print("=== Theoretical NLOS caps ===")
for env, p in envs.items():
    nlos_max = p['atten'] + 3*p['sigma']
    diff_max = p['diff'] + 8  # Normal(diff, 3.0, max(0,diff-5), diff+8)
    total_cap = nlos_max + diff_max
    print(f"  {env}: nlosAtten max={nlos_max} dB, diffraction max={diff_max} dB, total={total_cap} dB")

print()
df = pd.read_csv('/home/kushal/ns/ns-3-dev/results/smoke_test_50k.csv')
print("=== Actual NlosDb max by environment (50k smoke test) ===")
print(df.groupby('Environment')['NlosDb'].max().sort_values(ascending=False).to_string())
print()
print(f"Overall NlosDb max:          {df['NlosDb'].max():.2f} dB")
print()

# Body shadowing — confirm formula, no clip
print("=== Body shadowing analysis ===")
bw = df[df['BodyShadowingLossDb'] > 0]
print(f"  Body-worn rows: {len(bw):,}")
print(f"  Max: {bw['BodyShadowingLossDb'].max():.6f} dB  (formula: 6.0*(1-cos(0))/2 = 6.0)")
print(f"  Min non-zero: {bw['BodyShadowingLossDb'].min():.6f} dB")
print(f"  Mean: {bw['BodyShadowingLossDb'].mean():.4f} dB  (theoretical: 3.000 dB)")
# If clipped at 6.0 by std::min, we'd see a pile-up at exactly 6.0
exactly6 = (bw['BodyShadowingLossDb'].round(4) == 6.0).sum()
print(f"  Rows exactly at 6.0000 dB: {exactly6} (should be ~0 if formula, non-zero if clip)")
print("  Conclusion:", "FORMULA (no clip)" if exactly6 == 0 else "POSSIBLE CLIP — investigate")
print()
# Confirm distribution is continuous between 0 and 6
import numpy as np
bins = np.linspace(0, 6, 13)
hist, _ = np.histogram(bw['BodyShadowingLossDb'], bins=bins)
print("  Distribution across [0,6] (12 bins of 0.5 dB each):")
for i, count in enumerate(hist):
    print(f"    {bins[i]:.1f}–{bins[i+1]:.1f} dB: {count:,}")
