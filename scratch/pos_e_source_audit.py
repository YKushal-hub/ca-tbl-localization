"""
pos_e_source_audit.py

Two-part audit:
  Part 1: Confirm ml_training_dataset.csv strata 1-3 are equally artificial
           (near-deterministic sigmaM → PositionErrorM mapping, tiny real variance)
  Part 2: Audit tdoa_processed_dataset_fixed.csv PositionErrorM for the same
           class of problem — row-index patterns, artificial cycling, lack of
           real-input correlation.
"""
import pandas as pd
import numpy as np

SIGMA_VALUES = {0: 0.0, 1: 50.0, 2: 200.0, 3: 500.0}

# ─────────────────────────────────────────────────────────────────────────────
print("=" * 70)
print("PART 1 — ml_training_dataset.csv: per-stratum PositionErrorM spread")
print("=" * 70)

df_ml = pd.read_csv("results/ml_training_dataset.csv", nrows=200_000)
df_ml["sigma_stratum"] = df_ml.index % 4

print()
print(f"{'Stratum':>8} {'sigmaM':>8} {'PosErr mean':>12} {'PosErr std':>12} "
      f"{'PosErr std/mean':>16} {'5th pct':>9} {'95th pct':>9}")
print("-" * 80)

stratum_stats = {}
for s in [1, 2, 3]:
    sub = df_ml[df_ml["sigma_stratum"] == s]["PositionErrorM"]
    mean_ = sub.mean()
    std_  = sub.std()
    cv    = std_ / mean_ * 100
    p5    = sub.quantile(0.05)
    p95   = sub.quantile(0.95)
    stratum_stats[s] = {"mean": mean_, "std": std_, "cv": cv, "p5": p5, "p95": p95}
    print(f"  {s:>6}  {SIGMA_VALUES[s]:>8.1f}  {mean_:>12.2f}  {std_:>12.2f}  "
          f"{cv:>14.2f}%  {p5:>9.2f}  {p95:>9.2f}")

print()
print("Key question: does PositionErrorM vary meaningfully WITH channel conditions")
print("(NLOS, shadowing, fading) within a stratum, or is it essentially fixed?")
print()

# Within stratum 2, check if NLOS rows have higher PositionErrorM than LOS rows
s2 = df_ml[df_ml["sigma_stratum"] == 2].copy()
# Use NlosDb as proxy for NLOS condition (>0 = NLOS)
s2["is_nlos"] = s2["NlosDb"] > 0
nlos_pe  = s2[s2["is_nlos"]]["PositionErrorM"].median()
los_pe   = s2[~s2["is_nlos"]]["PositionErrorM"].median()
print(f"Stratum 2 (sigmaM=200m): within-stratum NLOS vs LOS split:")
print(f"  NLOS rows ({s2['is_nlos'].sum():,}): PositionErrorM median = {nlos_pe:.2f}m")
print(f"  LOS  rows ({(~s2['is_nlos']).sum():,}): PositionErrorM median = {los_pe:.2f}m")
print(f"  Ratio NLOS/LOS: {nlos_pe/los_pe:.3f}  (1.0 = no channel sensitivity)")
print()

# Cross-stratum correlation: does PositionErrorM increase predictably?
means = [stratum_stats[s]["mean"] for s in [1, 2, 3]]
sigmas = [SIGMA_VALUES[s] for s in [1, 2, 3]]
r = np.corrcoef(sigmas, means)[0, 1]
print(f"Correlation(sigmaM, mean PositionErrorM) across strata 1-3: {r:.6f}")
print(f"=> PositionErrorM is {'DOMINATED BY sigmaM' if r > 0.99 else 'NOT dominated by sigmaM'}")

# ─────────────────────────────────────────────────────────────────────────────
print()
print("=" * 70)
print("PART 2 — tdoa_processed_dataset_fixed.csv: PositionErrorM integrity audit")
print("=" * 70)
print()

df_tdoa = pd.read_csv("results/tdoa_processed_dataset_fixed.csv", nrows=100_000)
print(f"Columns: {[c for c in df_tdoa.columns if 'Error' in c or 'Pos' in c or 'Gw' in c or 'Tdoa' in c or 'GDOP' in c or 'gdop' in c]}")
print()

# 2a. Row-index cycling check: does any modulo pattern predict PositionErrorM near-perfectly?
print("--- 2a. Row-index cycling check ---")
df_tdoa["row_mod4"]  = df_tdoa.index % 4
df_tdoa["row_mod5"]  = df_tdoa.index % 5
df_tdoa["row_mod10"] = df_tdoa.index % 10

for mod_col in ["row_mod4", "row_mod5", "row_mod10"]:
    group_means = df_tdoa.groupby(mod_col)["PositionErrorM"].mean()
    cv_of_means = group_means.std() / group_means.mean() * 100
    print(f"  {mod_col}: group mean spread (CV of group means) = {cv_of_means:.3f}%  "
          f"{'[LOW — possible cycling]' if cv_of_means < 5 else '[OK — random]'}")
print()

# 2b. Distribution of PositionErrorM — is it continuous or clustered?
print("--- 2b. PositionErrorM distribution ---")
pe = df_tdoa["PositionErrorM"]
print(f"  count={len(pe):,}  min={pe.min():.4f}m  median={pe.median():.2f}m  "
      f"mean={pe.mean():.2f}m  max={pe.max():.2f}m")
print(f"  std={pe.std():.2f}m  CV={pe.std()/pe.mean()*100:.1f}%")
print(f"  Fraction near-zero (<0.01m): {(pe < 0.01).mean()*100:.2f}%")
print(f"  Fraction sentinel (>9000m):  {(pe > 9000).mean()*100:.2f}%")
print()

# Separate eligible vs ineligible
eligible   = df_tdoa[df_tdoa["TdoaEligible"] == 1]["PositionErrorM"]
ineligible = df_tdoa[df_tdoa["TdoaEligible"] == 0]["PositionErrorM"]
print(f"  Eligible rows ({len(eligible):,}): "
      f"median={eligible.median():.6f}m  std={eligible.std():.4f}m  "
      f"max={eligible.max():.4f}m")
print(f"  Ineligible rows ({len(ineligible):,}): "
      f"median={ineligible.median():.2f}m  std={ineligible.std():.2f}m")
print()

# 2c. Correlation with real physical inputs
print("--- 2c. Correlation of PositionErrorM (eligible rows) with real inputs ---")
if len(eligible) > 100:
    elig_df = df_tdoa[df_tdoa["TdoaEligible"] == 1].copy()

    # GwObsCount should positively correlate (more gateways → better geometry → lower error? or higher?)
    if "GwObsCount" in elig_df.columns:
        r_gw = elig_df["PositionErrorM"].corr(elig_df["GwObsCount"])
        print(f"  Corr(PositionErrorM, GwObsCount):   {r_gw:+.4f}  "
              f"(expect negative — more GW = better TDOA = lower error)")

    # u_lambda: node density parameter
    if "u_lambda" in elig_df.columns:
        r_ul = elig_df["PositionErrorM"].corr(elig_df["u_lambda"])
        print(f"  Corr(PositionErrorM, u_lambda):     {r_ul:+.4f}")

    # TimeSyncErrorNs proxy column
    for col in ["TimeSyncErrorNs", "Gw1ErrNs", "GDOP", "GdopValue"]:
        if col in elig_df.columns:
            r = elig_df["PositionErrorM"].corr(elig_df[col])
            print(f"  Corr(PositionErrorM, {col}):  {r:+.4f}")

    # Row-index cycling within eligible rows
    elig_df["row_mod4"] = elig_df.index % 4
    gm = elig_df.groupby("row_mod4")["PositionErrorM"].mean()
    cv_elig = gm.std() / gm.mean() * 100
    print()
    print(f"  Row-index cycling within eligible rows (mod4 group-mean CV): {cv_elig:.3f}%")
    print(f"  {'[LOW — possible cycling]' if cv_elig < 5 else '[OK — no detectable cycling]'}")

    # Check: does PositionErrorM vary continuously across GwObsCount bins?
    if "GwObsCount" in elig_df.columns:
        print()
        print("  PositionErrorM by GwObsCount (eligible rows):")
        g = elig_df.groupby("GwObsCount")["PositionErrorM"]
        for gw_ct, grp in g:
            print(f"    GwObsCount={gw_ct}: n={len(grp):,}  "
                  f"median={grp.median():.6f}m  std={grp.std():.6f}m")

print()
print("=" * 70)
print("SUMMARY")
print("=" * 70)
print()
print("ml_training_dataset.csv pos_e:")
r_sigma_mean = np.corrcoef(sigmas, means)[0, 1]
avg_cv = np.mean([stratum_stats[s]["cv"] for s in [1, 2, 3]])
print(f"  corr(sigmaM, mean PosErr) = {r_sigma_mean:.6f}")
print(f"  mean within-stratum CV   = {avg_cv:.2f}%")
print(f"  Verdict: {'DOMINATED BY TEST HARNESS CYCLING' if r_sigma_mean > 0.999 and avg_cv < 50 else 'acceptable variability'}")
print()
print("tdoa_processed_dataset_fixed.csv pos_e:")
pe_el = df_tdoa[df_tdoa["TdoaEligible"] == 1]["PositionErrorM"]
print(f"  Eligible rows PositionErrorM: median={pe_el.median():.4f}m  "
      f"std={pe_el.std():.4f}m  CV={pe_el.std()/max(pe_el.mean(),1e-9)*100:.1f}%")
