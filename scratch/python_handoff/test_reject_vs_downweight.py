import numpy as np
from ekf_tracker import RangeEKF

rng = np.random.default_rng(7)

anchors = [(0.0, 0.0), (100.0, 0.0), (50.0, 86.6)]
n_steps = 70
dt = 1.0
true_speed = (0.8, 0.5)
true_start = (20.0, 15.0)

true_positions = [(true_start[0] + true_speed[0]*k*dt, true_start[1] + true_speed[1]*k*dt) for k in range(n_steps)]

LOS_SIGMA_M = 8.0
NLOS_SIGMA_M = 35.0
NLOS_BIAS_M = 20.0
CORRUPT_WINDOW = range(20, 45)          # 25 consecutive steps, longer than the Stage 1 test

measured_ranges, is_bad = [], []
for k, (tx, ty) in enumerate(true_positions):
    ranges_k, bad_k = [], []
    for i, (ax, ay) in enumerate(anchors):
        true_r = np.hypot(tx-ax, ty-ay)
        bad = (i == 1) and (k in CORRUPT_WINDOW)
        r = true_r + (NLOS_BIAS_M + rng.normal(0, NLOS_SIGMA_M)) if bad else true_r + rng.normal(0, LOS_SIGMA_M)
        ranges_k.append(max(r, 0.1)); bad_k.append(bad)
    measured_ranges.append(ranges_k); is_bad.append(bad_k)

def run(strategy):
    ekf = RangeEKF([true_start[0], true_start[1], 0, 0],
                    np.diag([50**2,50**2,5**2,5**2]), 0.3, 0.3)
    track = []
    for k in range(n_steps):
        ekf.predict(dt)
        if strategy == "hard_reject" and any(is_bad[k]):
            track.append((ekf.x[0,0], ekf.x[1,0]))   # skip update entirely
            continue
        if strategy == "downweight":
            r_diag = [(NLOS_SIGMA_M**2 * 25 if is_bad[k][i] else LOS_SIGMA_M**2) for i in range(3)]
        elif strategy == "aware":  # normal NLOS-conditioned inflation, from Stage 1
            r_diag = [(NLOS_SIGMA_M**2 if is_bad[k][i] else LOS_SIGMA_M**2) for i in range(3)]
        else:  # naive
            r_diag = [LOS_SIGMA_M**2]*3
        x, P, S = ekf.update(measured_ranges[k], anchors, r_diag)
        track.append((x[0,0], x[1,0]))
    return track

def rmse(track, idx=None):
    idx = idx or range(len(track))
    errs = [np.hypot(track[k][0]-true_positions[k][0], track[k][1]-true_positions[k][1]) for k in idx]
    return np.sqrt(np.mean(np.square(errs))), np.max(errs)

for strat in ["naive", "aware", "downweight", "hard_reject"]:
    track = run(strat)
    overall = rmse(track)
    during = rmse(track, CORRUPT_WINDOW)
    after = rmse(track, range(45, 70))
    print(f"{strat:12s}  overall RMSE={overall[0]:6.2f}m (max {overall[1]:6.2f}m)  "
          f"during-corruption={during[0]:6.2f}m  post-recovery={after[0]:6.2f}m")
