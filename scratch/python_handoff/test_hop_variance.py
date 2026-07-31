"""
scratch/python_handoff/test_hop_variance.py

Phase S4/S5: Targeted synthetic test for Multi-Hop Variance conditioning.
Evaluates four variance construction strategies for multi-hop range measurements:
1. hop_blind: Ignores hops, assumes flat LOS variance.
2. hop_aware_naive: Sums probability-weighted expected variance across hops.
3. hop_aware_conditioned (Ideal): Sums true state-conditioned variance across hops.
4. hop_aware_noisy (Realistic): Sums estimated state-conditioned variance (80% accuracy).

Evaluates the impact on both EKF RMSE and Mahalanobis Integrity Check (Precision/Recall).
"""

import numpy as np
from ekf_tracker import RangeEKF

# --- Config ---
NUM_STEPS = 200
DT = 1.0
TRUE_START = (20.0, 15.0)
TRUE_SPEED = (0.8, 0.5)

ANCHORS = [(0.0, 0.0), (100.0, 0.0), (50.0, 86.6)]
HOPS = [1, 2, 3]  

LOS_SIGMA = 2.0
NLOS_SIGMA = 15.0
NLOS_BIAS = 0.0
P_NLOS = 0.4 

ANOMALY_PROB = 0.05
ANOMALY_BIAS = 30.0 
MAHALANOBIS_THRESHOLD = 9.21 

def generate_data(seed):
    rng = np.random.default_rng(seed)
    true_positions = [(TRUE_START[0] + TRUE_SPEED[0]*k*DT, TRUE_START[1] + TRUE_SPEED[1]*k*DT) for k in range(NUM_STEPS)]
    measurements = []
    
    for k, (tx, ty) in enumerate(true_positions):
        step_data = {'true_pos': (tx, ty), 'ranges': [], 'true_nlos_states': [], 'anomaly': False}
        is_anomaly = rng.random() < ANOMALY_PROB
        step_data['anomaly'] = is_anomaly
        anomaly_anchor = rng.integers(0, 3) if is_anomaly else -1
        
        for i, (ax, ay) in enumerate(ANCHORS):
            true_r = np.hypot(tx-ax, ty-ay)
            num_hops = HOPS[i]
            nlos_states = []
            total_noise = 0.0
            
            for h in range(num_hops):
                is_nlos = rng.random() < P_NLOS
                nlos_states.append(is_nlos)
                if is_nlos:
                    total_noise += NLOS_BIAS + rng.normal(0, NLOS_SIGMA)
                else:
                    total_noise += rng.normal(0, LOS_SIGMA)
                    
            r = true_r + total_noise
            if i == anomaly_anchor:
                r += ANOMALY_BIAS
                
            step_data['ranges'].append(max(r, 0.1))
            step_data['true_nlos_states'].append(nlos_states)
            
        measurements.append(step_data)
    return true_positions, measurements

def run_filter(mode, measurements, true_positions, rng, accuracy=None):
    ekf = RangeEKF(initial_state=[TRUE_START[0], TRUE_START[1], 0, 0],
                   initial_cov=np.diag([50**2, 50**2, 5**2, 5**2]),
                   process_noise_std_pos=1.0, process_noise_std_vel=0.5)
    track = []
    tp = fp = tn = fn = 0
    expected_var = (1 - P_NLOS) * (LOS_SIGMA**2) + P_NLOS * (NLOS_SIGMA**2)
    
    from ca_tbl_fusion import raw_multilateration, compute_mahalanobis
    
    for k, step in enumerate(measurements):
        ekf.predict(DT)
        ranges = step['ranges']
        true_states = step['true_nlos_states']
        r_diag = []
        for i, num_hops in enumerate(HOPS):
            if mode == 'hop_blind':
                final_hop_state = true_states[i][-1]
                r_diag.append(NLOS_SIGMA**2 if final_hop_state else LOS_SIGMA**2)
            elif mode == 'hop_aware_naive':
                r_diag.append(num_hops * expected_var)
            elif mode == 'hop_aware_conditioned':
                var = 0.0
                for state in true_states[i]:
                    var += NLOS_SIGMA**2 if state else LOS_SIGMA**2
                r_diag.append(var)
            elif mode == 'hop_aware_noisy':
                var = 0.0
                for state in true_states[i]:
                    estimated_state = state if rng.random() < accuracy else not state
                    var += NLOS_SIGMA**2 if estimated_state else LOS_SIGMA**2
                r_diag.append(var)
        
        z_est = raw_multilateration(ranges, ANCHORS, [ekf.x[0,0], ekf.x[1,0]])
        mahal = compute_mahalanobis(ekf.x, ekf.P, z_est)
        is_suspect = mahal > MAHALANOBIS_THRESHOLD
        has_anomaly = step['anomaly']
        
        if is_suspect and has_anomaly: tp += 1
        elif is_suspect and not has_anomaly: fp += 1
        elif not is_suspect and not has_anomaly: tn += 1
        elif not is_suspect and has_anomaly: fn += 1
            
        x, P, _ = ekf.update(ranges, ANCHORS, r_diag)
        track.append((x[0,0], x[1,0]))
        
    errs = [np.hypot(track[k][0]-true_positions[k][0], track[k][1]-true_positions[k][1]) for k in range(NUM_STEPS)]
    rmse = np.sqrt(np.mean(np.square(errs)))
    precision = tp / (tp + fp) if (tp + fp) > 0 else 0.0
    recall = tp / (tp + fn) if (tp + fn) > 0 else 0.0
    fpr = fp / (fp + tn) if (fp + tn) > 0 else 0.0
    return rmse, precision, recall, fpr

if __name__ == '__main__':
    NUM_SEEDS = 20
    modes = ['hop_blind', 'hop_aware_naive', 'hop_aware_conditioned']
    accuracies = [0.5, 0.6, 0.7, 0.8, 0.9, 0.91, 0.92, 0.93, 0.94, 0.95, 1.0]
    all_runs = modes + [f'noisy_{acc}' for acc in accuracies]
    
    results = {m: {'rmse': [], 'prec': [], 'rec': [], 'fpr': []} for m in all_runs}
    
    print(f"Running Multi-Hop Accuracy Sweep across {NUM_SEEDS} seeds...")
    
    for seed in range(1, NUM_SEEDS + 1):
        true_pos, measurements = generate_data(seed)
        
        for mode in modes:
            rng = np.random.default_rng(seed)
            rmse, prec, rec, fpr = run_filter(mode, measurements, true_pos, rng)
            results[mode]['rmse'].append(rmse)
            results[mode]['prec'].append(prec)
            results[mode]['rec'].append(rec)
            results[mode]['fpr'].append(fpr)
            
        for acc in accuracies:
            rng = np.random.default_rng(seed)
            run_id = f'noisy_{acc}'
            rmse, prec, rec, fpr = run_filter('hop_aware_noisy', measurements, true_pos, rng, accuracy=acc)
            results[run_id]['rmse'].append(rmse)
            results[run_id]['prec'].append(prec)
            results[run_id]['rec'].append(rec)
            results[run_id]['fpr'].append(fpr)
            
    print("\nEvaluating Modes (Mean +/- Std over 20 seeds):")
    print(f"{'Mode':<25} {'RMSE (m)':<15} {'Precision':<15} {'Recall':<15} {'FPR':<15}")
    print("-" * 85)
    
    for run_id in all_runs:
        r_mean, r_std = np.mean(results[run_id]['rmse']), np.std(results[run_id]['rmse'])
        p_mean, p_std = np.mean(results[run_id]['prec']), np.std(results[run_id]['prec'])
        c_mean, c_std = np.mean(results[run_id]['rec']), np.std(results[run_id]['rec'])
        f_mean, f_std = np.mean(results[run_id]['fpr']), np.std(results[run_id]['fpr'])
        
        print(f"{run_id:<25} {r_mean:5.2f} +/- {r_std:4.2f}  {p_mean:4.3f} +/- {p_std:4.3f}   {c_mean:4.3f} +/- {c_std:4.3f}   {f_mean:4.3f} +/- {f_std:4.3f}")

    print("\nPaired t-test vs hop_aware_naive (RMSE Difference: Naive - Noisy):")
    print("Positive difference means Noisy is BETTER (lower RMSE).")
    import scipy.stats as stats
    naive_rmse = np.array(results['hop_aware_naive']['rmse'])
    for acc in accuracies:
        run_id = f'noisy_{acc}'
        noisy_rmse = np.array(results[run_id]['rmse'])
        diff = naive_rmse - noisy_rmse
        mean_diff = np.mean(diff)
        std_diff = np.std(diff, ddof=1)
        n = len(diff)
        
        if std_diff == 0:
            p_val = 1.0
            ci_lower = mean_diff
            ci_upper = mean_diff
        else:
            t_stat, p_val = stats.ttest_rel(naive_rmse, noisy_rmse)
            margin = stats.t.ppf(0.975, n-1) * (std_diff / np.sqrt(n))
            ci_lower = mean_diff - margin
            ci_upper = mean_diff + margin
            
        distinguishable = ci_lower > 0
        print(f"{run_id:<15} Diff: {mean_diff:5.2f}m   95% CI: [{ci_lower:6.2f}, {ci_upper:6.2f}]   p-val: {p_val:.4f}   Distinguishable Win? {distinguishable}")
