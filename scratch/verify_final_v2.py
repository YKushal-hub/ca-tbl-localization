import pandas as pd, numpy as np, math, sys
from scipy import stats

df = pd.read_csv('results/smoke_test_50k.csv')
FSPL_1m = 31.21; f_GHz = 0.868; TxPower = 14.0
fails = []; warns = []

def chk(cond, label, measured='', target=''):
    tag = 'PASS' if cond else 'FAIL'
    if not cond: fails.append(f'{label} [{measured}] target={target}')
    print(f'  [{tag}] {label} [{measured}] (target {target})')

print('=== FIX 1: Rademacher long-range calibration ===')
urban = df[df['Environment'].isin([3,4])]
for d_lo, d_hi, d_ref, label in [(800,1200,1000,'1km'),(4000,6000,5000,'5km')]:
    band = urban[(urban['DistanceM']>=d_lo) & (urban['DistanceM']<=d_hi)]
    if len(band) > 10:
        rad = TxPower - (132.41 + 10*1.58*math.log10(d_ref/1000))
        gap = band['TrueRssiDbm'].mean() - rad
        chk(abs(gap) < 7, f'Urban RSSI gap vs Rademacher at {label}', f'{gap:+.1f}dB', '<7dB')

print()
print('=== FIX 2: Rural Forested LOS delay spread ===')
rf_los = df[(df['Environment']==1) & (df['NlosDb']==0)]
if len(rf_los) > 20:
    ds = rf_los['DelaySpreadNs'].mean()
    chk(40 < ds < 150, 'RURAL_FORESTED LOS delay spread', f'{ds:.0f}ns', '40-150ns')

print()
print('=== COUPLING 3 after fix ===')
nlos_mask = df['NlosDb'] > 0
los_mask  = ~nlos_mask
if nlos_mask.sum() > 50 and los_mask.sum() > 50:
    r3 = df[nlos_mask]['DelaySpreadNs'].mean() / df[los_mask]['DelaySpreadNs'].mean()
    chk(2.5 < r3 < 5.0, 'Coupling 3 NLOS/LOS delay ratio', f'{r3:.2f}x', '2.5-5.0x')

print()
print('=== FIX 3: Coupling 2 urban shadowing ratios ===')
for env_id, target in [(3,1.40),(4,1.50)]:
    edf = df[df['Environment']==env_id]
    nlos_std = edf[edf['NlosDb']>0]['ShadowingDb'].std()
    los_std  = edf[edf['NlosDb']==0]['ShadowingDb'].std()
    if los_std > 0:
        r = nlos_std/los_std
        chk(abs(r-target)<0.10, f'Env {env_id} Coupling 2 ratio', f'{r:.3f}', f'{target}±0.10')

print()
print('=== FIX 4: NlosDb max cap ===')
for env_id, mu_spec, sigma in [(3,25,6),(4,28,6)]:
    edf = df[(df['Environment']==env_id) & (df['NlosDb']>0)]
    if len(edf) > 10:
        cap = mu_spec + 3*sigma
        over = (edf['NlosDb'] > cap).sum()
        chk(over == 0, f'Env {env_id} NlosDb rows over cap {cap} dB', str(over), '0')

print()
print('=== FIX 5: Coupling 5 terrain-shadow correlation ===')
blocked = df[df['TerrainDb'] > 3]['ShadowingDb']
clear   = df[df['TerrainDb'] == 0]['ShadowingDb']
if len(blocked) > 50 and clear.std() > 0:
    r5 = blocked.std() / clear.std()
    chk(r5 > 1.08, 'Coupling 5 terrain/clear shadow std ratio', f'{r5:.3f}', '>1.08')

print()
print('=== INVARIANT CHECKS (must still pass) ===')
gap = (df['TotalLossDb'] - df[['PathLossDb','ShadowingDb','FadingDb','NlosDb',
    'BuildingEntryLossDb','WallPenetrationLossDb','FloorAttenuationDb',
    'BodyShadowingLossDb','TemporalBlockageLossDb','AtmosphericDb',
    'FoliageDb','TerrainDb','InterferencePenaltyDb','HwTxVariationDb']].sum(axis=1)).abs()
chk(gap.mean() < 0.1, 'TotalLossDb integrity gap', f'{gap.mean():.4f}dB', '<0.1dB')
zero_body = (df['BodyShadowingLossDb']==0).mean()
chk(0.75 < zero_body < 0.85, 'Body shadow zero fraction', f'{zero_body*100:.1f}%', '75-85%')
nz_pos = df[df['PositionErrorM']>0]['PositionErrorM'].mean()
chk(250 < nz_pos < 380, 'Sub.21 non-zero mean', f'{nz_pos:.1f}m', '250-380m')

print()
print(f'FAILs: {len(fails)}')
if fails:
    for f in fails:
        print(f'  ✗ {f}')
    sys.exit(1)
else:
    print('ALL PASS - proceed to 5M generation')
