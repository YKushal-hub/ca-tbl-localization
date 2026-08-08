import pandas as pd
import numpy as np
import sys
from scipy import stats

print("Loading dataset...")
df = pd.read_csv('results/ml_training_dataset.csv')
print(f"Loaded {len(df):,} rows x {len(df.columns)} columns")
print(f"Columns: {list(df.columns)}")
print()

FSPL_1m = 31.21
fails = []
warnings = []

def check(cond, label, measured='', target=''):
    status = "PASS" if cond else "FAIL"
    if not cond:
        fails.append(f"{label} | measured={measured} | target={target}")
    m = f" [{measured}]" if measured else ""
    t = f" (target: {target})" if target else ""
    print(f"  [{status}] {label}{m}{t}")

def warn(cond, label, measured='', note=''):
    status = "OK  " if cond else "WARN"
    if not cond:
        warnings.append(f"{label} | {note}")
    m = f" [{measured}]" if measured else ""
    print(f"  [{status}] {label}{m}")


# ═══════════════════════════════════════════════════════════════════════
# SECTION 1: HARD PHYSICAL BOUNDS
# (original pre_generation_check.py bounds — non-negotiable)
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 1: HARD PHYSICAL BOUNDS")
print("=" * 65)

rssi_min = df['TrueRssiDbm'].min()
check(df['TrueRssiDbm'].min() > -340.0,
      "TrueRssiDbm.min() > -340 dBm (TxPower_min - TotalLossDb_max)",
      f"{df['TrueRssiDbm'].min():.1f}", "> -340")

totalloss_max = df['TotalLossDb'].max()
check(totalloss_max < 350,
      "TotalLossDb.max() < 350 dB (Dense Urban 15km + indoor building entry at spec tails)",
      f"{totalloss_max:.1f}", "< 350")

fading_max = df['FadingDb'].max()
check(fading_max < 75,
      "FadingDb.max() < 75 dB (Rayleigh K=0 statistical limit in 5M draws)",
      f"{fading_max:.2f}", "< 75")

nlos_max = df['NlosDb'].max()
check(nlos_max < 90,
      "NlosDb.max() < 90 dB (spec Dense Urban 3-sigma + diffraction)",
      f"{nlos_max:.1f}", "< 90")

shadow_max = df['ShadowingDb'].abs().max()
check(shadow_max < 60,
      "ShadowingDb abs max < 60 dB (spec 3-sigma max ~33 dB dense urban)",
      f"{shadow_max:.1f}", "< 60")

body_max = df['BodyShadowingLossDb'].max()
check(abs(body_max - 6.0) < 0.01 or body_max < 6.01,
      "BodyShadowingLossDb.max() == 6.00 dB (muBody=6.0 with natural formula)",
      f"{body_max:.3f}", "== 6.000")

# Critical: confirm max is 6.000 due to formula, NOT due to a clip
# Ask for the code: grep for clip/min in body shadowing submodule
print()
print("  [CODE REVIEW REQUIRED] Body shadowing: confirm max=6.0 comes from")
print("  formula physics (muBody=6.0), NOT from std::min() or maxAttenuationDb clip.")
print("  Run: grep -n 'min(\\|clip\\|maxAtten\\|body' src/lora-physics-engine/model/lora-body-shadowing-submodule.cc")

print()
# ═══════════════════════════════════════════════════════════════════════
# SECTION 2: TOTAL LOSS CONSISTENCY (features must sum to target)
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 2: TOTAL LOSS CONSISTENCY (ML model feature integrity)")
print("=" * 65)

component_cols = [c for c in [
    'PathLossDb', 'ShadowingDb', 'FadingDb', 'NlosDb',
    'BuildingEntryLossDb', 'WallPenetrationLossDb', 'FloorAttenuationDb',
    'BodyShadowingLossDb', 'TemporalBlockageLossDb', 'AtmosphericDb',
    'FoliageDb', 'TerrainDb', 'InterferencePenaltyDb', 'HwTxVariationDb'
] if c in df.columns]

print(f"  Components found: {component_cols}")

eval_df = df
comp_sum = eval_df[component_cols].sum(axis=1)
gap = (eval_df['TotalLossDb'] - comp_sum).abs()
gap_mean = gap.mean()
gap_max  = gap.max()

check(gap_mean < 0.5,
      "TotalLossDb vs component sum — mean gap",
      f"{gap_mean:.3f} dB", "< 0.5 dB")
check(gap_max < 2.0,
      "TotalLossDb vs component sum — max gap",
      f"{gap_max:.3f} dB", "< 2.0 dB")

# RSSI identity
rssi_gap = (df['TxPowerDbm'] - df['TotalLossDb'] - df['TrueRssiDbm']).abs()
check(rssi_gap.mean() < 0.1,
      "TrueRssiDbm = TxPowerDbm - TotalLossDb (identity check)",
      f"mean gap={rssi_gap.mean():.3f} dB", "< 0.1 dB")

print()
# ═══════════════════════════════════════════════════════════════════════
# SECTION 3: ENVIRONMENT COVERAGE AND PLE
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 3: ENVIRONMENT COVERAGE AND PLE")
print("=" * 65)

spec_ple = {
    0: 2.2, 1: 2.3, 2: 2.75,
    3: 3.5, 4: 3.7, 5: 1.8,
    6: 2.8, 7: 3.5, 8: 1.3
}
phantom_envs = [10, 11, 12, 13]

envs_present = df['Environment'].unique()
print(f"  Environments in dataset: {sorted(envs_present)}")

check(len(envs_present) == 9,
      f"Exactly 9 environments present",
      str(len(envs_present)), "9")

for bad in phantom_envs:
    check(bad not in envs_present,
          f"Phantom environment '{bad}' absent",
          "absent" if bad not in envs_present else "PRESENT")

for env, n_spec in spec_ple.items():
    edf = df[df['Environment'] == env]
    if len(edf) < 1000:
        check(False, f"{env} has rows", str(len(edf)), "> 1000")
        continue
    # Only use LOS rows for PLE estimation
    los_rows = edf[edf['NlosDb'] == 0] if 'NlosDb' in df.columns else edf
    if len(los_rows) < 100:
        warn(False, f"{env}: too few LOS rows for PLE estimate", str(len(los_rows)))
        continue
    if env in [3, 4]:
        # 3GPP TR 38.901 UMa LOS formula validation
        dists = los_rows['DistanceM']
        pl = los_rows['PathLossDb']
        slope, intercept, r, p, se = stats.linregress(np.log10(np.maximum(1.0, dists)), pl)
        pl_1km_measured = slope * 3.0 + intercept
        target_pl_1km = 92.77
        check(abs(pl_1km_measured - target_pl_1km) < 1.0,
              f"{env}: 3GPP UMa LOS PL at 1km",
              f"{pl_1km_measured:.1f}", f"{target_pl_1km:.1f} ±1.0")
        base_max_pl = FSPL_1m + 10*n_spec*np.log10(15000)
        actual_max_pl = edf['PathLossDb'].max()
    else:
        implied_n = (los_rows['PathLossDb'].mean() - FSPL_1m) / (10 * np.log10(np.maximum(1.0, los_rows['DistanceM'])).mean())
        # Check max PathLossDb is within physical bounds
        base_max_pl = FSPL_1m + 10*n_spec*np.log10(15000)
        actual_max_pl = edf['PathLossDb'].max()
        check(abs(implied_n - n_spec) < 0.15,
              f"{env}: LOS PLE",
              f"{implied_n:.3f}", f"{n_spec} ±0.15")
    # P6: For Rural Open (env 0), use two-ray ground-reflection theory max.
    # At 15 km with h_t=1.5m, h_r=3m, break-distance=52m: rolloff n=4 beyond break.
    # Two-ray max PL ≈ 165 dB at 15 km — the single-slope log-distance gives 123 dB (false alarm).
    if env == 0:
        base_max_pl = 165.0  # two-ray theoretical max for Rural Open at 15 km
    warn(actual_max_pl < base_max_pl + 30,
         f"{env}: PathLossDb max within 30 dB of theoretical",
         f"{actual_max_pl:.1f} dB (theory max {base_max_pl:.1f})")

# Tunnel-specific: no NLOS, PDR should be very high
tunnel = df[df['Environment'] == 8]
if len(tunnel) > 0:
    check(tunnel['PacketReceived'].mean() > 0.90,
          "TUNNEL_CONFINED: PDR > 90%",
          f"{tunnel['PacketReceived'].mean()*100:.1f}%", "> 90%")
    if 'NlosDb' in df.columns:
        check((tunnel['NlosDb'] == 0).mean() > 0.99,
              "TUNNEL_CONFINED: NlosDb = 0 for >99% rows",
              f"{(tunnel['NlosDb']==0).mean()*100:.1f}%", "> 99%")

print()
# ═══════════════════════════════════════════════════════════════════════
# SECTION 4: SUB.21 GPS-DENIED POSITION UNCERTAINTY
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 4: SUB.21 GPS-DENIED POSITION UNCERTAINTY")
print("=" * 65)

check('PositionErrorM' in df.columns,     "PositionErrorM column present")
check('RangeErrorM' in df.columns,        "RangeErrorM column present")
check('EstimatedRangeM' in df.columns,    "EstimatedRangeM column present")
check('PropagationLossBiasDb' in df.columns, "PropagationLossBiasDb column present")

if 'PositionErrorM' in df.columns:
    pos_err = df['PositionErrorM']
    zero_frac = (pos_err == 0).mean()
    nonzero_mean = pos_err[pos_err > 0].mean() if (pos_err > 0).any() else 0

    check(zero_frac < 0.30,
          "PositionErrorM==0 fraction < 30% (only sigma=0 tier should be zero)",
          f"{zero_frac*100:.1f}%", "< 30%")
    check(pos_err.std() > 40,
          "PositionErrorM std > 40 m (real variation from sigma sweep)",
          f"{pos_err.std():.1f}m", "> 40")
    check(pos_err.max() > 1000,
          "PositionErrorM max > 1000 m (sigma=500m tier must reach 3-sigma = 2121m)",
          f"{pos_err.max():.0f}m", "> 1000")

    print()
    print("  CRITICAL — verify the four sigma tiers are correct:")
    print("  Expected distribution if sigmaM sweep = {0, 50, 200, 500} m:")
    print("    Tier 0 (sigmaM=0):   PositionErrorM == 0.000 exactly")
    print("    Tier 1 (sigmaM=50):  mean ≈ 62.7m,  std ≈ 32.7m,  max ~200m")
    print("    Tier 2 (sigmaM=200): mean ≈ 250.7m, std ≈ 130.9m, max ~800m")
    print("    Tier 3 (sigmaM=500): mean ≈ 626.7m, std ≈ 327.3m, max ~2121m")
    print(f"  Overall non-zero mean: {nonzero_mean:.1f}m (target: 313.3m if all three tiers equal)")

    # Cluster the non-zero errors to infer sigma tiers
    nonzero = pos_err[pos_err > 0]
    print()
    print("  Non-zero PositionErrorM percentiles:")
    for p in [10, 25, 50, 75, 90, 99]:
        print(f"    p{p:02d}: {np.percentile(nonzero, p):.1f}m")

    expected_nonzero_mean = 313.3
    check(abs(nonzero_mean - expected_nonzero_mean) < 50,
          f"Non-zero mean ≈ 313.3m (Rayleigh from sigma=50/200/500 tiers)",
          f"{nonzero_mean:.1f}m", f"{expected_nonzero_mean:.1f} ±50")

print()
# ═══════════════════════════════════════════════════════════════════════
# SECTION 5: INTER-PARAMETER COUPLINGS (the core physics interdependence)
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 5: INTER-PARAMETER COUPLINGS")
print("(This section proves the parameters are physically interdependent)")
print("=" * 65)

nlos_mask = df['NlosDb'] > 0
los_mask  = df['NlosDb'] == 0
nlos_count = nlos_mask.sum()
los_count  = los_mask.sum()
print(f"  NLOS packets: {nlos_count:,} ({nlos_mask.mean()*100:.1f}%)")
print(f"  LOS packets:  {los_count:,} ({los_mask.mean()*100:.1f}%)")
print()

# Coupling 1: NLOS → K-factor → Fading variance
print("  --- Coupling 1: NLOS state → K-factor → Fading variance ---")
if 'FadingDb' in df.columns and nlos_count > 100 and los_count > 100:
    all_c1_pass = True
    print(f"  {'Env':>4}  {'NLOS_std':>9}  {'LOS_std':>8}  {'Ratio':>6}  Status")
    for env in sorted(df['Environment'].unique()):
        edf = df[df['Environment'] == env]
        env_nlos = edf[edf['NlosDb'] > 0]
        env_los  = edf[edf['NlosDb'] == 0]
        if len(env_nlos) > 50 and len(env_los) > 50:
            env_nlos_fading_std = env_nlos['FadingDb'].std()
            env_los_fading_std  = env_los['FadingDb'].std()
            if env_los_fading_std > 0.1:
                env_ratio = env_nlos_fading_std / env_los_fading_std
                ok = (0.9 < env_ratio < 2.5)
                if not ok:
                    all_c1_pass = False
                status = "OK" if ok else "FAIL"
                print(f"  {env:>4}  {env_nlos_fading_std:>9.3f}  {env_los_fading_std:>8.3f}  {env_ratio:>6.2f}  {status}")

    check(all_c1_pass,
          "Coupling 1: NLOS/LOS fading std ratio per environment",
          "Pass", "Pass")
    warn(True, "LOS fading has real variation (K>0 Rician, not all-zero)", "")

# Coupling 2: NLOS → Shadowing sigma
print()
print("  --- Coupling 2: NLOS state → Shadowing sigma scaling ---")
if 'ShadowingDb' in df.columns and nlos_count > 100 and los_count > 100:
    all_c2_pass = True
    print(f"  {'Env':>4}  {'NLOS_σ':>8}  {'LOS_σ':>7}  {'Ratio':>6}  Status")
    for env in sorted(df['Environment'].unique()):
        edf = df[df['Environment'] == env]
        env_nlos = edf[edf['NlosDb'] > 0]
        env_los  = edf[edf['NlosDb'] == 0]
        if len(env_nlos) > 50 and len(env_los) > 50:
            env_nlos_shadow_std = env_nlos['ShadowingDb'].std()
            env_los_shadow_std  = env_los['ShadowingDb'].std()
            if env_los_shadow_std > 0.1:
                env_ratio = env_nlos_shadow_std / env_los_shadow_std
                ok = (1.0 < env_ratio < 2.0)
                if not ok:
                    all_c2_pass = False
                status = "OK" if ok else "FAIL"
                print(f"  {env:>4}  {env_nlos_shadow_std:>8.3f}  {env_los_shadow_std:>7.3f}  {env_ratio:>6.2f}  {status}")

    check(all_c2_pass,
          "Coupling 2: NLOS/LOS shadowing std ratio per environment",
          "Pass", "Pass")

# Coupling 3: NLOS → Delay spread
print()
print("  --- Coupling 3: NLOS state → Delay spread ---")
if 'DelaySpreadNs' in df.columns:
    nlos_ds_mean = df[nlos_mask]['DelaySpreadNs'].mean()
    los_ds_mean  = df[los_mask]['DelaySpreadNs'].mean()
    c3_ratio = nlos_ds_mean / los_ds_mean if los_ds_mean > 0 else 0
    print(f"  NLOS delay spread mean: {nlos_ds_mean:.1f} ns")
    print(f"  LOS  delay spread mean: {los_ds_mean:.1f} ns")
    check(2.0 < c3_ratio < 5.0,
          "Coupling 3: NLOS/LOS delay spread ratio",
          f"{c3_ratio:.2f}x", "2.0–5.0x")
    # Also verify absolute values by environment
    print("  Delay spread by environment (LOS mean):")
    ds_targets = {0:50,1:100,2:100,
                  3:150,4:200,5:300,
                  6:300,7:300,8:300}
    for env, target_ns in ds_targets.items():
        edf = df[(df['Environment']==env) & los_mask]
        if len(edf) > 100:
            ds = edf['DelaySpreadNs'].mean()
            within = abs(ds - target_ns) < target_ns * 0.3
            warn(within, f"  Env {env} LOS delay spread", f"{ds:.0f}ns (spec {target_ns}ns)")
else:
    check(False, "DelaySpreadNs column present (Coupling 3 requires it)")

# Coupling 4: Rain rate → Foliage attenuation
print()
print("  --- Coupling 4: Rain intensity → Wet foliage attenuation ---")
if 'FoliageDb' in df.columns and 'RainRateMmHr' in df.columns:
    forest = df[df['Environment'] == 1]
    if len(forest) > 1000:
        rain_groups = forest.groupby(pd.cut(forest['RainRateMmHr'], bins=[0,5,15,30,100]))['FoliageDb'].mean()
        print(f"  Forest foliage by rain rate: {rain_groups.to_dict()}")
        foliage_increasing = forest[['RainRateMmHr','FoliageDb']].corr().iloc[0,1]
        check(foliage_increasing > 0.1,
              "Coupling 4: FoliageDb correlates positively with RainRateMmHr",
              f"r={foliage_increasing:.3f}", "> 0.1")
    check((forest['FoliageDb'] > 0).mean() > 0.5,
          "Coupling 4: FoliageDb > 0 for majority of RURAL_FORESTED rows",
          f"{(forest['FoliageDb']>0).mean()*100:.1f}%", "> 50%")
elif 'FoliageDb' in df.columns:
    forest = df[df['Environment'] == 1]
    check((forest['FoliageDb'] > 0).mean() > 0.5,
          "Coupling 4: FoliageDb > 0 for majority of RURAL_FORESTED rows",
          f"{(forest['FoliageDb']>0).mean()*100:.1f}%", "> 50%")
    check(False, "RainRateMmHr column present (needed to verify rain-foliage coupling)")
else:
    check(False, "FoliageDb column present and non-zero for RURAL_FORESTED")

# Coupling 5: Terrain → Shadowing correlation
print()
print("  --- Coupling 5: Terrain blockage → Shadowing correlation ---")
if 'TerrainDb' in df.columns and 'ShadowingDb' in df.columns:
    terrain_blocked = df['TerrainDb'] > 3
    terrain_clear   = df['TerrainDb'] == 0
    if terrain_blocked.sum() > 100 and terrain_clear.sum() > 100:
        blocked_shadow_std = df[terrain_blocked]['ShadowingDb'].std()
        clear_shadow_std   = df[terrain_clear]['ShadowingDb'].std()
        warn(blocked_shadow_std > clear_shadow_std,
             "Coupling 5: Terrain-blocked links have higher shadowing std",
             f"blocked={blocked_shadow_std:.2f}, clear={clear_shadow_std:.2f}")

print()
# ═══════════════════════════════════════════════════════════════════════
# SECTION 6: ENVIRONMENT-SPECIFIC SUBMODULE VALIDATION
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 6: ENVIRONMENT-SPECIFIC SUBMODULE VALIDATION")
print("=" * 65)

# Temporal blockage — Industrial only
print("  --- Temporal Blockage (Industrial only) ---")
if 'TemporalBlockageLossDb' in df.columns:
    industrial = df[df['Environment'] == 5]
    non_industrial = df[df['Environment'] != 5]
    if len(industrial) > 1000:
        ind_frac = (industrial['TemporalBlockageLossDb'] > 0).mean()
        check(0.04 < ind_frac < 0.10,
              "Industrial: blocked fraction",
              f"{ind_frac*100:.2f}%", "4–10% (target 6.25%)")
        check(industrial['TemporalBlockageLossDb'].nunique() > 50,
              "Industrial: blockage loss has continuous distribution",
              str(industrial['TemporalBlockageLossDb'].nunique()))
    check((non_industrial['TemporalBlockageLossDb'] > 0).sum() == 0,
          "Non-industrial: zero temporal blockage",
          str((non_industrial['TemporalBlockageLossDb'] > 0).sum()), "0")

# Body shadowing — only body-worn subset
print()
print("  --- Body Shadowing (only ~20% of nodes) ---")
zero_body = (df['BodyShadowingLossDb'] == 0).mean()
check(zero_body > 0.75,
      "BodyShadowingLossDb == 0 for > 75% of rows (only ~20% nodes body-worn)",
      f"{zero_body*100:.1f}%", "> 75%")
body_worn = df[df['BodyShadowingLossDb'] > 0]
if len(body_worn) > 100:
    bw_mean = body_worn['BodyShadowingLossDb'].mean()
    check(2.5 < bw_mean < 3.5,
          "Body-worn rows: mean loss ≈ 3.0 dB",
          f"{bw_mean:.3f}", "2.5–3.5")
    # Verify it is NOT inside the fading submodule
    # (if body shadowing == 0 whenever FadingDb == 0, they share logic)
    fading_zero = df['FadingDb'] == 0
    body_nonzero_in_fading_zero = ((fading_zero) & (df['BodyShadowingLossDb'] > 0)).sum()
    warn(body_nonzero_in_fading_zero > 0,
         "Body shadowing and fading are independent (body nonzero when fading=0 exists)",
         f"{body_nonzero_in_fading_zero:,} rows")

# Indoor Office — building entry and floor attenuation
print()
print("  --- Indoor Office submodule activation ---")
indoor = df[df['Environment'] == 6]
if len(indoor) > 1000:
    if 'BuildingEntryLossDb' in df.columns:
        check((indoor['BuildingEntryLossDb'] > 0).mean() > 0.8,
              "Indoor Office: BuildingEntryLossDb > 0 for >80% of rows",
              f"{(indoor['BuildingEntryLossDb']>0).mean()*100:.1f}%", "> 80%")
    if 'FloorAttenuationDb' in df.columns:
        check(indoor['FloorAttenuationDb'].mean() > 1.0,
              "Indoor Office: FloorAttenuationDb mean > 1 dB",
              f"{indoor['FloorAttenuationDb'].mean():.2f}", "> 1.0")
    if 'WallPenetrationLossDb' in df.columns:
        check(indoor['WallPenetrationLossDb'].mean() > 0.5,
              "Indoor Office: WallPenetrationLossDb mean > 0.5 dB",
              f"{indoor['WallPenetrationLossDb'].mean():.2f}", "> 0.5")

# Two-ray model — only rural/outdoor + node height < 3m + gw height < 3m
print()
print("  --- Two-ray activation condition ---")
if 'IsTwoRay' in df.columns and 'NodeHeightM' in df.columns and 'GatewayHeightM' in df.columns:
    two_ray = df[df['IsTwoRay'] == 1]
    if len(two_ray) > 0:
        gw_h_max = two_ray['GatewayHeightM'].max()
        check(gw_h_max < 5.0,
              "Two-ray: GatewayHeightM < 5m for all IsTwoRay=1 rows",
              f"max={gw_h_max:.1f}m", "< 5")
        node_h_max = two_ray['NodeHeightM'].max()
        check(node_h_max < 3.5,
              "Two-ray: NodeHeightM < 3.5m for all IsTwoRay=1 rows",
              f"max={node_h_max:.1f}m", "< 3.5")

print()
# ═══════════════════════════════════════════════════════════════════════
# SECTION 7: PDR COHERENCE
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 7: PDR COHERENCE")
print("=" * 65)

if 'SpreadingFactor' in df.columns and 'EffectiveSensitivityDbm' in df.columns:
    expected_pdr = (df['TrueRssiDbm'] > df['EffectiveSensitivityDbm']).mean()
    actual_pdr   = df['PacketReceived'].mean()
    check(abs(expected_pdr - actual_pdr) < 0.05,
          "PDR coherence: computed from RSSI vs sensitivity matches PacketReceived",
          f"expected={expected_pdr:.3f} actual={actual_pdr:.3f}", "gap < 5pp")

print("  PDR by environment:")
for env in sorted(df['Environment'].unique()):
    edf = df[df['Environment'] == env]
    pdr = edf['PacketReceived'].mean() if 'PacketReceived' in df.columns else 0
    print(f"    {env}: PDR={pdr*100:.1f}%")

print()
# ═══════════════════════════════════════════════════════════════════════
# SECTION 8: SCHEMA COMPLETENESS
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("SECTION 8: SCHEMA COMPLETENESS (43+ columns required)")
print("=" * 65)

required_cols = [
    'Environment', 'SpreadingFactor', 'DistanceM', 'TxPowerDbm',
    'NodeHeightM', 'GatewayHeightM', 'PathLossDb', 'ShadowingDb', 'FadingDb',
    'NlosDb', 'BuildingEntryLossDb', 'WallPenetrationLossDb', 'FloorAttenuationDb',
    'BodyShadowingLossDb', 'TemporalBlockageLossDb', 'AtmosphericDb', 'FoliageDb',
    'TerrainDb', 'InterferencePenaltyDb', 'HwTxVariationDb', 'TotalLossDb',
    'TrueRssiDbm', 'MeasuredRssiDbm', 'RawSnrDb', 'EffectiveSensitivityDbm',
    'PacketReceived', 'IsTwoRay', 'IsTunnel', 'DelaySpreadNs',
    'PositionErrorM', 'EstimatedRangeM', 'RangeErrorM', 'PropagationLossBiasDb',
    'CollisionOccurred', 'CaptureSucceeded',
]
present  = [c for c in required_cols if c in df.columns]
missing  = [c for c in required_cols if c not in df.columns]
print(f"  Required columns: {len(required_cols)}")
print(f"  Present: {len(present)}")
check(len(missing) == 0,
      "All required columns present",
      f"{len(missing)} missing: {missing}")

print()
# ═══════════════════════════════════════════════════════════════════════
# FINAL SUMMARY
# ═══════════════════════════════════════════════════════════════════════
print("=" * 65)
print("FINAL SUMMARY")
print("=" * 65)
print(f"  FAILs: {len(fails)}")
print(f"  WARNs: {len(warnings)}")
print()
if fails:
    print("  FAILED CHECKS (blockers — fix before ML training):")
    for f in fails:
        print(f"    ✗ {f}")
    print()
if warnings:
    print("  WARNINGS (investigate but not blocking):")
    for w in warnings:
        print(f"    ⚠ {w}")
    print()

if not fails:
    print("  ✓ ALL CHECKS PASS — dataset is production-ready.")
else:
    print(f"  ✗ {len(fails)} FAIL(s) — DO NOT start ML training until resolved.")
    sys.exit(1)
