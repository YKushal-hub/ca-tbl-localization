import pandas as pd
import numpy as np

df = pd.read_csv("results/tdoa_processed_dataset_fixed.csv", nrows=500_000)

# Converged rows: TdoaEligible=1 AND PositionErrorM < 9000
converged = df[(df["TdoaEligible"] == 1) & (df["PositionErrorM"] < 9000)].copy()
failed    = df[(df["TdoaEligible"] == 1) & (df["PositionErrorM"] >= 9000)]
inelig    = df[df["TdoaEligible"] == 0]

print("=" * 65)
print("tdoa_processed_dataset_fixed.csv — converged TDOA solve distribution")
print("=" * 65)
total = len(df)
print(f"\nTotal rows: {total:,}")
print(f"  TdoaEligible=1, converged (PosErr<9000): {len(converged):,}  "
      f"({len(converged)/total*100:.1f}%)")
print(f"  TdoaEligible=1, failed    (PosErr>=9000): {len(failed):,}  "
      f"({len(failed)/total*100:.1f}%)")
print(f"  TdoaEligible=0 (ineligible):              {len(inelig):,}  "
      f"({len(inelig)/total*100:.1f}%)")

print()
print("─" * 65)
print("Converged-solve PositionErrorM distribution")
print("─" * 65)
pe = converged["PositionErrorM"]
ptiles = [0, 5, 10, 25, 50, 75, 90, 95, 99, 100]
vals   = np.percentile(pe, ptiles)
print(f"  n = {len(pe):,}  mean = {pe.mean():.4f}m  std = {pe.std():.4f}m")
print()
print(f"  {'Percentile':>12}  {'PositionErrorM (m)':>20}")
print(f"  {'-'*12}  {'-'*20}")
for p, v in zip(ptiles, vals):
    marker = " ← median" if p == 50 else ""
    print(f"  {p:>11}%  {v:>20.6f}m{marker}")

# Breakdown by GwObsCount
print()
print("─" * 65)
print("Converged-solve median by GwObsCount")
print("─" * 65)
if "GwObsCount" in converged.columns:
    g = converged.groupby("GwObsCount")["PositionErrorM"]
    print(f"  {'GwObsCount':>12}  {'n':>8}  {'median (m)':>12}  {'mean (m)':>12}  {'std (m)':>12}")
    print(f"  {'-'*12}  {'-'*8}  {'-'*12}  {'-'*12}  {'-'*12}")
    for gw, grp in g:
        print(f"  {gw:>12}  {len(grp):>8,}  "
              f"{grp.median():>12.6f}  {grp.mean():>12.6f}  {grp.std():>12.6f}")

# Reference comparison
print()
print("─" * 65)
print("Reference check against already-validated Podevijn figures")
print("─" * 65)
print(f"  Validated 'Standard' median : ~42.43m")
print(f"  Validated 'Fine'    median  : ~11.18m")
print(f"  This file's overall median  : {pe.median():.6f}m")
print()
if pe.median() < 1.0:
    print("  VERDICT: Suspiciously near-zero — timing-jitter noise likely not")
    print("           reaching PositionErrorM. Trace timeSyncErrorNs wiring.")
elif pe.median() < 11.0:
    print("  VERDICT: Below Fine-mode reference (11.18m). Timing noise may be")
    print("           underweighted relative to validated configuration.")
elif pe.median() <= 50.0:
    print("  VERDICT: Within or near the validated 11-42m range. File is")
    print("           consistent with established reference. Use as pos_e source.")
else:
    print(f"  VERDICT: Above reference range ({pe.median():.1f}m > 42.43m).")
    print("           May indicate solver noise overhead or geometry issues.")
