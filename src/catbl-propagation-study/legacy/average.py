import pandas as pd
import numpy as np

# Columns are not named, but we can infer:
# 0: seed, 1: radius, 2: mode, 3: is_physics
# 6: PDR, 24: TDOA fraction
# The columns are: seed, radius_km, mode, physics_enabled, packets_sent, packets_received, pdr, coverage_fraction, mean_snr_db, median_snr_db, std_snr_db, min_snr_db, mean_rssi, median_rssi, min_rssi, mean_ts_err, std_ts_err, max_ts_err, mean_loc_err, median_loc_err, max_loc_err
# Wait! Let's just read the file and let pandas label columns 0-24
df = pd.read_csv('/home/kushal/ns/ns-3-dev/regime_sweep_clean_vs_physics.csv', header=None)

# 1: radius, 2: mode, 3: is_physics, 6: pdr, 21: mean_loc_err, 22: median_loc_err, 24: tdoa_frac
res = df.groupby([1, 2, 3]).agg({6:'mean', 21:'mean', 22:'mean', 24:'mean'})
for i, row in res.iterrows():
    radius, mode, is_phys = i
    print(f'{radius}km {mode} phys={is_phys}: PDR={row[6]*100:.2f}%, TDOA={row[24]*100:.2f}%, MeanErr={row[21]}, MedErr={row[22]}')
