#!/usr/bin/env python3
src = open('/home/kushal/ns/ns-3-dev/scratch/multimodal_generator.py').read()
assert 'import math, sys, time, json' in src, 'Patch 1 FAILED'
assert '_load_real_tdoa_rates' in src, 'Patch 2 FAILED'
assert 'IMU_ONLY' in src, 'Patch 3 FAILED'
assert '"WAN": 0.72' not in src, 'Old hardcoded TDOA_RATES still present'
bad = [l for l in src.splitlines() if 'fpe = np.where(hok, fpe_ekf' in l]
assert not bad, f'Old EKF assignment still present: {bad}'
print('All patches verified OK')
print(f'  IMU_ONLY occurrences:           {src.count("IMU_ONLY")}')
print(f'  _load_real_tdoa_rates calls:    {src.count("_load_real_tdoa_rates")}')
print(f'  Old TDOA_RATES literal absent:  True')
print(f'  Old EKF line absent:            True')
