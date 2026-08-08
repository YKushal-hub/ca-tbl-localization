"""
Sweep several TIMING_SIGMA_NS values on 1000 rows to find which one
lands in the validated 11-42m median range for this deployment geometry.
"""
import pandas as pd
import numpy as np
from scipy.optimize import least_squares

def tdoa_residuals(pos, gateways, measured_arrivals, c=299792458.0):
    dists = np.linalg.norm(gateways - pos, axis=1)
    expected_tdoa = (dists[1:] - dists[0])
    measured_tdoa = (measured_arrivals[1:] - measured_arrivals[0]) * c * 1e-9
    return expected_tdoa - measured_tdoa

def solve_batch(elig_rows, sigma_ns):
    c = 299792458.0
    errors = []
    for row in elig_rows:
        idx, node_x, node_y, num_gw, gw_data = row
        rng = np.random.default_rng(seed=int(idx) & 0xFFFFFFFF)
        gateways, arrivals = [], []
        for k in range(min(10, num_gw)):
            gx = gw_data[k][0]; gy = gw_data[k][1]; ge = gw_data[k][2]
            if np.isnan(gx): continue
            gateways.append([gx, gy])
            td = np.linalg.norm(np.array([gx, gy]) - np.array([node_x, node_y]))
            arrivals.append((td / c) * 1e9 + ge + rng.normal(0.0, sigma_ns))
        if len(gateways) < 3:
            errors.append(9999.0); continue
        gw = np.array(gateways); arr = np.array(arrivals)
        init = np.mean(gw, axis=0)
        res = least_squares(tdoa_residuals, init, args=(gw, arr, c), method='lm')
        if not res.success:
            errors.append(9999.0); continue
        err = np.linalg.norm(res.x - np.array([node_x, node_y]))
        errors.append(err if err <= 5000.0 else 9999.0)
    return np.array(errors)

df = pd.read_csv("results/geometric_training_dataset.csv")
elig = df[df["TdoaEligible"] == 1].head(1000)

# pre-extract row data
rows = []
for row in elig.itertuples():
    gw_data = [(getattr(row, f"Gw{k+1}X", np.nan),
                getattr(row, f"Gw{k+1}Y", np.nan),
                getattr(row, f"Gw{k+1}ErrNs", 0.0)) for k in range(10)]
    rows.append((row.Index, row.NodeX_m, row.NodeY_m, row.GwObsCount, gw_data))

print(f"{'sigma_ns':>10} {'median_m':>10} {'P10_m':>10} {'P90_m':>10} {'conv%':>8} {'verdict':>25}")
print("-" * 75)
for sigma in [25, 50, 75, 100, 150, 200]:
    errs = solve_batch(rows, sigma)
    conv = errs[errs < 9000]
    if len(conv) == 0:
        print(f"{sigma:>10}   all failed"); continue
    med = np.median(conv)
    p10 = np.percentile(conv, 10)
    p90 = np.percentile(conv, 90)
    pct = len(conv) / len(errs) * 100
    if med < 11:
        v = "below Fine (11m)"
    elif med <= 42:
        v = "IN RANGE ✓"
    else:
        v = f"above Standard ({med:.0f}m)"
    print(f"{sigma:>10} {med:>10.2f} {p10:>10.2f} {p90:>10.2f} {pct:>7.1f}%  {v}")
