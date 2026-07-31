import pandas as pd
import numpy as np
import os
import sys

def run_phase_e_checks(seed=7, output_dir='./results'):
    print("Running Phase E Automated Validation Checks...")
    
    # Load Dataset A Raw
    ds_A_raw = f'{output_dir}/dataset_A_seed{seed}.csv'
    if not os.path.exists(ds_A_raw):
        print(f"Error: {ds_A_raw} not found.")
        return
        
    df_a = pd.read_csv(ds_A_raw)
    
    shadowing_mean = df_a['ShadowingSigmaEff'].mean()
    assert shadowing_mean >= 8.0, f"Shadowing: {shadowing_mean} < 8.0"
    print(f"[PASS] 1. Shadowing >= 8.0 (Actual: {shadowing_mean:.2f})")
    
    nlos_rate = df_a['NlosIsNlos'].mean()
    assert nlos_rate > 0.05, f"NLOS Rate: {nlos_rate} <= 0.05"
    print(f"[PASS] 3. NLOS Rate > 0.05 (Actual: {nlos_rate:.2f})")
    
    ds_A_sum = f'{output_dir}/dataset_A_summary_seed{seed}.csv'
    df_a_sum = pd.read_csv(ds_A_sum)
    for env in df_a_sum['environment'].unique():
        for sf in df_a_sum['spreading_factor'].unique():
            subset = df_a_sum[(df_a_sum['environment'] == env) & (df_a_sum['spreading_factor'] == sf)].sort_values('distance_m')
            pdr_diff = subset['pdr'].diff().dropna()
            assert (pdr_diff <= 0).all(), f"PDR not monotonic decreasing for {env} SF{sf}"
    print("[PASS] 4. PDR is monotonically decreasing")
    
    num_invalid_rssi = (df_a['measured_rssi_dbm'] == -999.0).sum()
    assert num_invalid_rssi == 0, f"Found {num_invalid_rssi} invalid RSSI values in Dataset A"
    print("[PASS] 5. RSSI != -999.0")
    
    ds_E_raw = f'{output_dir}/dataset_E_seed{seed}.csv'
    if os.path.exists(ds_E_raw):
        df_e = pd.read_csv(ds_E_raw)
        snow_df = df_e[df_e['weather_state'] == 'LIGHT_SNOW']
        if len(snow_df) > 0:
            rain_rate = snow_df['act_rain'].mean()
            assert rain_rate > 0.0, f"LIGHT_SNOW rain rate: {rain_rate} <= 0.0"
            print(f"[PASS] 6. Weather rates LIGHT_SNOW > 0 (Actual: {rain_rate:.2f})")
            
    ple_values = []
    for env in df_a_sum['environment'].unique():
        env_df = df_a_sum[df_a_sum['environment'] == env]
        d_log10 = np.log10(env_df['distance_m'])
        pl = env_df['path_loss_mean']
        coef = np.polyfit(10 * d_log10, pl, 1)
        ple_values.append(coef[0])
    ple_mean = np.mean(ple_values)
    assert ple_mean <= 3.0, f"PLE mean: {ple_mean} > 3.0"
    print(f"[PASS] 2. PLE <= 3.0 (Actual: {ple_mean:.2f})")
    
    print("All Phase E asserts passed!")

if __name__ == "__main__":
    run_phase_e_checks()
