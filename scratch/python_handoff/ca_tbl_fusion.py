"""
scratch/python_handoff/ca_tbl_fusion.py

Phase S4/S5: Evaluation Loop Fusion Logic.
This module processes a multi-tier CSV using 4 ablation modes to isolate the
contributions of tier escalation vs. EKF integrity rejection.
"""

import pandas as pd
import numpy as np
from ekf_tracker import RangeEKF

TIERS = ['868MHz', '2.4GHz', 'UWB']
PCS_THRESHOLD_UP = {'868MHz': 1.55, '2.4GHz': 1.70, 'UWB': 2.30}
PCS_THRESHOLD_DOWN = {'868MHz': 1.75, '2.4GHz': 1.90, 'UWB': 2.55}
N_DEESCALATE = 5
MAHALANOBIS_THRESHOLD = 9.21  # Chi-square 99% for 2 DOF

LOS_SIGMA_M = {
    '868MHz': 50.0,
    '2.4GHz': 15.0,
    'UWB': 0.1
}
NLOS_SIGMA_M = {
    '868MHz': 200.0,
    '2.4GHz': 50.0,
    'UWB': 5.0
}

def compute_gdop(H):
    """
    Computes Geometric Dilution of Precision (GDOP) from measurement matrix H.
    H shape is (n_anchors, 4). We only care about x,y components (first 2 cols).
    """
    H_pos = H[:, :2]
    try:
        HTH_inv = np.linalg.inv(H_pos.T @ H_pos)
        return np.sqrt(np.trace(HTH_inv))
    except np.linalg.LinAlgError:
        return 100.0  # Bad geometry

def compute_pcs(P_upd, H, nlos_flags):
    """
    Original 3-term PCS Formula:
    1) 1 / trace(Sigma_pos)
    2) 1 / GDOP
    3) LOS Fraction
    """
    trace_sigma = np.trace(P_upd[:2, :2])
    trace_ref = 10.0
    term1 = trace_ref / (trace_ref + trace_sigma)
    
    gdop = compute_gdop(H)
    term2 = 1.0 / max(gdop, 1.0)
    
    n_total = len(nlos_flags)
    n_los = n_total - sum(nlos_flags)
    term3 = n_los / max(n_total, 1)
    
    pcs = term1 + term2 + term3
    return pcs

def compute_mahalanobis(x_pred, P_pred, z_est):
    diff = z_est - x_pred[:2]
    try:
        inv_P = np.linalg.inv(P_pred[:2, :2])
        dist = diff.T @ inv_P @ diff
        return dist[0, 0]
    except np.linalg.LinAlgError:
        return 0.0

def raw_multilateration(ranges, anchors, x0):
    from scipy.optimize import least_squares
    def residuals(p):
        return [np.hypot(p[0] - ax, p[1] - ay) - r for (ax, ay), r in zip(anchors, ranges)]
    res = least_squares(residuals, x0)
    return np.array(res.x).reshape(2, 1)

def build_H_matrix(x_pred, anchors):
    n = len(anchors)
    H = np.zeros((n, 4))
    x, y = x_pred[0, 0], x_pred[1, 0]
    for i, (ax, ay) in enumerate(anchors):
        dx, dy = x - ax, y - ay
        r_pred = np.sqrt(dx**2 + dy**2)
        r_safe = max(r_pred, 1e-6)
        H[i, 0] = dx / r_safe
        H[i, 1] = dy / r_safe
    return H

def run_fusion(csv_path, mode='baseline'):
    df = pd.read_csv(csv_path)
    timestamps = sorted(df['TimestampS'].unique())
    
    start_row = df.iloc[0]
    ekf = RangeEKF(initial_state=[start_row['TrueX'], start_row['TrueY'], 0, 0],
                   initial_cov=np.diag([100**2, 100**2, 10**2, 10**2]),
                   process_noise_std_pos=1.0, process_noise_std_vel=0.5)
                   
    current_tier_idx = 0
    active_tier = TIERS[current_tier_idx]
    deescalate_counter = 0
    
    track = []
    escalations = 0
    demotions = 0
    suspect_flags = 0
    native_coverage = 0
    borrowed_successfully = 0
    borrow_failed_uwb = 0
    borrow_failed_24ghz = 0
    borrow_failed_868mhz = 0
    dt = 1.0
    
    anchor_map = {101: (0.0, 0.0), 102: (100.0, 0.0), 103: (50.0, 86.6)}
    
    for t in timestamps:
        t_df = df[df['TimestampS'] == t]
        
        x_pred, P_pred = ekf.predict(dt)
        escalated_this_step = False
        
        # Borrow Logic
        fusion_tier_idx = current_tier_idx
        tier_data = t_df[(t_df['Tier'] == TIERS[fusion_tier_idx]) & (t_df['PacketReceived'] == 1)]
        
        while len(tier_data) < 3 and fusion_tier_idx > 0:
            fusion_tier_idx -= 1
            tier_data = t_df[(t_df['Tier'] == TIERS[fusion_tier_idx]) & (t_df['PacketReceived'] == 1)]
            
        fusion_tier = TIERS[fusion_tier_idx]
        is_native = (fusion_tier_idx == current_tier_idx)
        
        if len(tier_data) >= 3:
            anchors, ranges, r_diag, nlos_flags = [], [], [], []
            for _, row in tier_data.iterrows():
                aid = int(row['AnchorId'])
                if aid in anchor_map:
                    anchors.append(anchor_map[aid])
                    ranges.append(row['EstimatedRangeM'])
                    is_nlos = (row['NlosDb'] > 0) or (row['HiddenChannelState'] > 0)
                    nlos_flags.append(is_nlos)
                    r_diag.append(NLOS_SIGMA_M[fusion_tier]**2 if is_nlos else LOS_SIGMA_M[fusion_tier]**2)
                        
            x_upd, P_upd, _ = ekf.update(ranges, anchors, r_diag)
            H = build_H_matrix(x_pred, anchors)
            pcs = compute_pcs(P_upd, H, nlos_flags)
            
            suspect = False
            if mode != 'baseline':
                z_est = raw_multilateration(ranges, anchors, [x_pred[0,0], x_pred[1,0]])
                mahal = compute_mahalanobis(x_pred, P_pred, z_est)
                n_los_frac = (len(nlos_flags) - sum(nlos_flags)) / max(len(nlos_flags), 1)
                if mahal > MAHALANOBIS_THRESHOLD and n_los_frac == 1.0:
                    suspect = True
                    suspect_flags += 1
            
            should_escalate = False
            # Mechanism A: Only evaluate PCS and Suspect-escalation if native coverage
            if is_native:
                if pcs < PCS_THRESHOLD_UP[active_tier]:
                    should_escalate = True
                
                if suspect:
                    if mode in ['ca_tbl_reject_only', 'ca_tbl_downweight_only']:
                        pass
                    else:
                        should_escalate = True
                    
            if should_escalate:
                if current_tier_idx < 2 and not escalated_this_step:
                    current_tier_idx += 1
                    active_tier = TIERS[current_tier_idx]
                    escalations += 1
                    deescalate_counter = 0 # reset on escalate
                    
                    tier_data2 = t_df[(t_df['Tier'] == active_tier) & (t_df['PacketReceived'] == 1)]
                    if len(tier_data2) >= 3:
                        anchors2, ranges2, r_diag2 = [], [], []
                        for _, row in tier_data2.iterrows():
                            aid = int(row['AnchorId'])
                            if aid in anchor_map:
                                anchors2.append(anchor_map[aid])
                                ranges2.append(row['EstimatedRangeM'])
                                is_nlos2 = (row['NlosDb'] > 0) or (row['HiddenChannelState'] > 0)
                                r_diag2.append(NLOS_SIGMA_M[active_tier]**2 if is_nlos2 else LOS_SIGMA_M[active_tier]**2)
                        x_upd, P_upd, _ = ekf.update(ranges2, anchors2, r_diag2)
                    else:
                        if mode == 'ca_tbl_escalate_only':
                            pass
                        elif mode == 'ca_tbl_full' or mode == 'ca_tbl_reject_only' or mode == 'baseline':
                            x_upd, P_upd = x_pred, P_pred
                else:
                    if suspect and mode in ['ca_tbl_full', 'ca_tbl_reject_only']:
                        x_upd, P_upd = x_pred, P_pred
            else:
                if suspect and mode == 'ca_tbl_downweight_only':
                    r_diag_inflated = [r * 25.0 for r in r_diag]
                    x_upd, P_upd, _ = ekf.update(ranges, anchors, r_diag_inflated)
                elif suspect and mode == 'ca_tbl_reject_only':
                    x_upd, P_upd = x_pred, P_pred

            ekf.x = x_upd
            ekf.P = P_upd
            
            if is_native:
                native_coverage += 1
                # De-escalation hysteresis check ONLY if natively covered
                if not should_escalate and not suspect and pcs > PCS_THRESHOLD_DOWN[active_tier]:
                    deescalate_counter += 1
                else:
                    deescalate_counter = 0
            else:
                borrowed_successfully += 1
                deescalate_counter += 1 # borrowed tier means dropout
                
            if deescalate_counter >= N_DEESCALATE:
                if current_tier_idx > 0:
                    current_tier_idx -= 1
                    active_tier = TIERS[current_tier_idx]
                    demotions += 1
                deescalate_counter = 0
                
        else:
            x_upd, P_upd = x_pred, P_pred
            ekf.x = x_upd
            ekf.P = P_upd
            
            if current_tier_idx == 2:
                borrow_failed_uwb += 1
            elif current_tier_idx == 1:
                borrow_failed_24ghz += 1
            else:
                borrow_failed_868mhz += 1
                
            deescalate_counter += 1
            if deescalate_counter >= N_DEESCALATE:
                if current_tier_idx > 0:
                    current_tier_idx -= 1
                    active_tier = TIERS[current_tier_idx]
                    demotions += 1
                deescalate_counter = 0
            
        true_x = t_df.iloc[0]['TrueX']
        true_y = t_df.iloc[0]['TrueY']
        track.append({'est_x': ekf.x[0, 0], 'est_y': ekf.x[1, 0], 'true_x': true_x, 'true_y': true_y})
        
    errs = [np.hypot(p['est_x'] - p['true_x'], p['est_y'] - p['true_y']) for p in track]
    rmse = np.sqrt(np.mean(np.square(errs))) if errs else 0.0
    max_err = np.max(errs) if errs else 0.0
    return rmse, max_err, escalations, demotions, suspect_flags, native_coverage, borrowed_successfully, borrow_failed_uwb, borrow_failed_24ghz, borrow_failed_868mhz
if __name__ == '__main__':
    # Local test
    import sys
    if len(sys.argv) > 1:
        path = sys.argv[1]
    else:
        path = '/home/kushal/ns/ns-3-dev/results/ca_tbl_trajectory_multi.csv'
        
    print(run_fusion(path, 'baseline'))
    print(run_fusion(path, 'ca_tbl_full'))
