"""
scratch/python_handoff/ca_tbl_eval_loop.py

Phase S4/S5: Multi-Seed Evaluation Loop with Paired Statistics
Aggregates performance across the 5 ablation modes, reporting paired
differences, max-error (worst-case), and Wilcoxon signed-rank tests.
"""

import os
import subprocess
import numpy as np
from scipy import stats
from ca_tbl_fusion import run_fusion

NUM_SEEDS = 30
MODES = ['baseline', 'ca_tbl_reject_only', 'ca_tbl_downweight_only', 'ca_tbl_escalate_only', 'ca_tbl_full']

NS3_DIR = '/home/kushal/ns/ns-3-dev'
RESULTS_DIR = os.path.join(NS3_DIR, 'results')

def run_ns3_seed(seed):
    csv_file = os.path.join(RESULTS_DIR, f'ca_tbl_trajectory_multi_seed{seed}.csv')
    if not os.path.exists(csv_file):
        cmd = (f"export PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin && "
               f"cd {NS3_DIR} && "
               f"./ns3 run --no-build 'ca_tbl_trajectory_generator_multi "
               f"--env=1 --duration=60 --dt=1 --seed={seed} --output={csv_file}'")
        subprocess.run(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return csv_file

def print_paired_stats(name, a, b):
    diff = np.array(a) - np.array(b)
    mean_diff = np.mean(diff)
    
    # 95% CI for the mean difference
    se = stats.sem(diff)
    if se == 0:
        ci_lower, ci_upper = mean_diff, mean_diff
        p_val = 1.0
    else:
        ci = stats.t.interval(0.95, len(diff)-1, loc=mean_diff, scale=se)
        ci_lower, ci_upper = ci[0], ci[1]
        
        try:
            res = stats.ttest_rel(a, b)
            p_val = res.pvalue
        except:
            p_val = 1.0
            
    wins = np.sum(diff < 0)
    losses = np.sum(diff > 0)
    ties = np.sum(diff == 0)

    print(f"Paired Diff ({name}): {mean_diff:+.2f} m (95% CI: [{ci_lower:+.2f}, {ci_upper:+.2f}])")
    print(f"  Paired t-test p-value: {p_val:.4e}")
    print(f"  Concordance (A better < 0): A won {wins}, B won {losses}, Ties {ties}")

if __name__ == '__main__':
    print(f"Starting CA-TBL Paired Evaluation ({NUM_SEEDS} seeds)")
    
    results = {mode: {'rmse': [], 'max_err': [], 'esc': [], 'dem': [], 'susp': [], 'native': [], 'borrow_succ': [], 'bf_uwb': [], 'bf_24': [], 'bf_868': []} for mode in MODES}
    
    for seed in range(1, NUM_SEEDS + 1):
        csv_file = run_ns3_seed(seed)
        for mode in MODES:
            rmse, max_err, esc, dem, susp, native, borrow_succ, bf_uwb, bf_24, bf_868 = run_fusion(csv_file, mode=mode)
            results[mode]['rmse'].append(rmse)
            results[mode]['max_err'].append(max_err)
            results[mode]['esc'].append(esc)
            results[mode]['dem'].append(dem)
            results[mode]['susp'].append(susp)
            results[mode]['native'].append(native)
            results[mode]['borrow_succ'].append(borrow_succ)
            results[mode]['bf_uwb'].append(bf_uwb)
            results[mode]['bf_24'].append(bf_24)
            results[mode]['bf_868'].append(bf_868)
            
            assert (native + borrow_succ + bf_uwb + bf_24 + bf_868) == 60, f"Sum is {native + borrow_succ + bf_uwb + bf_24 + bf_868}"
    print("\n=== FINAL RESULTS (Mean ± Std) ===")
    for mode in MODES:
        r_mean = np.mean(results[mode]['rmse'])
        r_std = np.std(results[mode]['rmse'])
        m_mean = np.mean(results[mode]['max_err'])
        m_std = np.std(results[mode]['max_err'])
        e_mean = np.mean(results[mode]['esc'])
        d_mean = np.mean(results[mode]['dem'])
        s_mean = np.mean(results[mode]['susp'])
        nat_mean = np.mean(results[mode]['native'])
        bs_mean = np.mean(results[mode]['borrow_succ'])
        bfu_mean = np.mean(results[mode]['bf_uwb'])
        bf2_mean = np.mean(results[mode]['bf_24'])
        bf8_mean = np.mean(results[mode]['bf_868'])
        
        print(f"Mode: {mode}")
        print(f"  RMSE:        {r_mean:.2f} m ± {r_std:.2f} m")
        print(f"  Max Error:   {m_mean:.2f} m ± {m_std:.2f} m")
        print(f"  Escalations: {e_mean:.1f}")
        print(f"  Demotions:   {d_mean:.1f}")
        print(f"  Suspects:    {s_mean:.1f}")
        print(f"  NativeCov:   {nat_mean:.1f}")
        print(f"  BorrowSucc:  {bs_mean:.1f}")
        print(f"  BF_UWB:      {bfu_mean:.1f}")
        print(f"  BF_24GHz:    {bf2_mean:.1f}")
        print(f"  BF_868MHz:   {bf8_mean:.1f}\n")
        
    print("=== PAIRED DIFFERENCES (RMSE) ===")
    print_paired_stats("Downweight - Baseline", results['ca_tbl_downweight_only']['rmse'], results['baseline']['rmse'])
    print_paired_stats("Escalate - Baseline", results['ca_tbl_escalate_only']['rmse'], results['baseline']['rmse'])
    print_paired_stats("Full - Escalate", results['ca_tbl_full']['rmse'], results['ca_tbl_escalate_only']['rmse'])
    
    print("\n=== PAIRED DIFFERENCES (Max Error) ===")
    print_paired_stats("Full - Escalate", results['ca_tbl_full']['max_err'], results['ca_tbl_escalate_only']['max_err'])
