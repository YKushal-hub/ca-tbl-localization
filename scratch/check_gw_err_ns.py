import pandas as pd
import numpy as np

df = pd.read_csv("results/geometric_training_dataset.csv", nrows=50_000)

print("Columns:", list(df.columns[:20]))
print("Total columns:", len(df.columns))
print()

# Check all Gw*ErrNs columns
err_cols = [c for c in df.columns if "ErrNs" in c]
print(f"Gateway ErrNs columns: {err_cols}")
print()

# What are the actual values?
for col in err_cols[:5]:
    vals = df[col].dropna()
    print(f"  {col}: min={vals.min():.4f}  median={vals.median():.4f}  "
          f"max={vals.max():.4f}  all_zero={(vals==0).all()}  "
          f"nonzero_frac={(vals!=0).mean()*100:.1f}%")

print()
# Check specifically for eligible rows
elig = df[df["TdoaEligible"] == 1]
print(f"TdoaEligible=1 rows: {len(elig):,}")
if len(elig) > 0 and "Gw1ErrNs" in df.columns:
    g1 = elig["Gw1ErrNs"].dropna()
    print(f"  Gw1ErrNs (eligible): min={g1.min():.6f}  median={g1.median():.6f}  "
          f"max={g1.max():.6f}  std={g1.std():.6f}")
    print(f"  All zero? {(g1==0).all()}")
    print(f"  Fraction non-zero: {(g1!=0).mean()*100:.1f}%")
    print()

print("─" * 60)
print("Manual LM solve trace on first eligible row to verify noise wiring")
print("─" * 60)
c_light = 299792458.0

first_elig = elig.iloc[0]
print(f"NodeX={first_elig['NodeX_m']:.2f}, NodeY={first_elig['NodeY_m']:.2f}, "
      f"GwObsCount={first_elig['GwObsCount']}")

gateways = []
arrivals = []
for k in range(1, int(first_elig["GwObsCount"]) + 1):
    gx = first_elig.get(f"Gw{k}X", np.nan)
    gy = first_elig.get(f"Gw{k}Y", np.nan)
    ge = first_elig.get(f"Gw{k}ErrNs", 0.0)
    if np.isnan(gx):
        break
    true_dist = np.linalg.norm(np.array([gx, gy]) - 
                                np.array([first_elig["NodeX_m"], first_elig["NodeY_m"]]))
    arrival = (true_dist / c_light) * 1e9 + ge
    gateways.append([gx, gy])
    arrivals.append(arrival)
    print(f"  GW{k}: pos=({gx:.1f},{gy:.1f})  true_dist={true_dist:.2f}m  "
          f"errNs={ge:.4f}ns  arrival={arrival:.6f}ns")

# What would zero errNs do?
print()
print("  With all ErrNs=0 (no noise):")
gateways_arr = np.array(gateways)
node_pos = np.array([first_elig["NodeX_m"], first_elig["NodeY_m"]])
for i, gw in enumerate(gateways_arr):
    dist = np.linalg.norm(gw - node_pos)
    print(f"  GW{i+1}: arrival_noiseless={(dist/c_light)*1e9:.6f}ns  "
          f"arrival_with_noise={arrivals[i]:.6f}ns  diff={arrivals[i]-(dist/c_light)*1e9:.6f}ns")

print()
print("─" * 60)
print("Key: if ErrNs is always 0, TDOA solve uses exact arrival times → exact position → err=0m")
print("─" * 60)
