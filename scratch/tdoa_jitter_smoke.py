"""
Quick smoke test: run TDOA solve on first 1000 eligible rows with the new
timing jitter, and print the resulting PositionErrorM distribution.
Expected: converged median in ~11-42m range.
"""
import pandas as pd
import numpy as np
from scipy.optimize import least_squares

TIMING_SIGMA_NS = 400.0

def tdoa_residuals(pos, gateways, measured_arrivals, c=299792458.0):
    dists = np.linalg.norm(gateways - pos, axis=1)
    expected_tdoa = (dists[1:] - dists[0])
    measured_tdoa = (measured_arrivals[1:] - measured_arrivals[0]) * c * 1e-9
    return expected_tdoa - measured_tdoa

def solve_row(idx, node_x, node_y, num_gw, gw_data_dict):
    c = 299792458.0
    rng = np.random.default_rng(seed=int(idx) & 0xFFFFFFFF)
    gateways, arrivals = [], []
    for k in range(min(10, num_gw)):
        gx = gw_data_dict[k][0]
        gy = gw_data_dict[k][1]
        ge = gw_data_dict[k][2]
        if np.isnan(gx):
            continue
        gateways.append([gx, gy])
        td = np.linalg.norm(np.array([gx, gy]) - np.array([node_x, node_y]))
        jitter = rng.normal(0.0, TIMING_SIGMA_NS)
        arrivals.append((td / c) * 1e9 + ge + jitter)
    if len(gateways) < 3:
        return 9999.0
    gateways = np.array(gateways)
    arrivals = np.array(arrivals)
    init = np.mean(gateways, axis=0)
    res = least_squares(tdoa_residuals, init, args=(gateways, arrivals, c), method='lm')
    if not res.success:
        return 9999.0
    err = np.linalg.norm(res.x - np.array([node_x, node_y]))
    return err if err <= 5000.0 else 9999.0

df = pd.read_csv("results/geometric_training_dataset.csv")
gw_cols = []
for k in range(1, 11):
    gw_cols.extend([f"Gw{k}Id", f"Gw{k}X", f"Gw{k}Y", f"Gw{k}ErrNs"])

elig = df[df["TdoaEligible"] == 1].head(1000)
print(f"Solving {len(elig)} eligible rows...")

errors = []
for row in elig.itertuples():
    gw_data = {}
    for k in range(10):
        gw_data[k] = (
            getattr(row, f"Gw{k+1}X", np.nan),
            getattr(row, f"Gw{k+1}Y", np.nan),
            getattr(row, f"Gw{k+1}ErrNs", 0.0)
        )
    err = solve_row(row.Index, row.NodeX_m, row.NodeY_m, row.GwObsCount, gw_data)
    errors.append(err)

errors = np.array(errors)
converged = errors[errors < 9000]
failed    = errors[errors >= 9000]

print(f"\nResults (n={len(errors)}):")
print(f"  Converged: {len(converged)} ({len(converged)/len(errors)*100:.1f}%)")
print(f"  Failed:    {len(failed)}    ({len(failed)/len(errors)*100:.1f}%)")
print()
if len(converged) > 0:
    ptiles = [5, 10, 25, 50, 75, 90, 95]
    print("  Converged PositionErrorM distribution:")
    for p in ptiles:
        v = np.percentile(converged, p)
        ref = "← median" if p == 50 else ""
        print(f"    P{p:2d}: {v:8.3f}m  {ref}")
    print(f"  Mean: {converged.mean():.3f}m  Std: {converged.std():.3f}m")
    print()
    print(f"Reference range: ~11m (Fine) to ~42m (Standard) median")
    med = np.median(converged)
    if med < 1.0:
        print(f"VERDICT: Still near-zero ({med:.4f}m) — jitter not reaching solver?")
    elif med < 11.0:
        print(f"VERDICT: {med:.1f}m — below Fine reference; sigma_t may need tuning down.")
    elif med <= 50.0:
        print(f"VERDICT: {med:.1f}m — within validated 11-42m range. Proceed.")
    else:
        print(f"VERDICT: {med:.1f}m — above Standard reference; sigma_t too large.")
