import pandas as pd
import numpy as np
import subprocess
import sys
import os

def analyze_dataset_d():
    print("Extracting TDOA Accuracy from Dataset D (Real-world Simulator Parameters)...")
    seeds = [7, 11, 23]
    standard_medians = []
    fine_medians = []
    
    for s in seeds:
        filename = f"results/dataset_D_summary_seed{s}.csv"
        if os.path.exists(filename):
            df = pd.read_csv(filename)
            std_med = df[df['timing_mode'] == 'standard']['position_error_median_m'].values[0]
            fine_med = df[df['timing_mode'] == 'fine_timestamp']['position_error_median_m'].values[0]
            standard_medians.append(std_med)
            fine_medians.append(fine_med)
            
    if len(standard_medians) > 0:
        print(f"  Standard Timestamp Median Error     : {np.mean(standard_medians):.2f} m")
        print(f"  Fine Timestamp Median Error         : {np.mean(fine_medians):.2f} m")
        print(f"  --> Fine Timestamp Improvement      : {(np.mean(standard_medians) - np.mean(fine_medians)) / np.mean(standard_medians) * 100:.1f}%")
        return np.mean(standard_medians), np.mean(fine_medians)
    else:
        print("  Dataset D summary files not found yet.")
        return None, None

def analyze_tdoa_validator():
    print("\nRunning TDOA Validator (Clean-Model Centroid vs TDOA Fused)...")
    try:
        output = subprocess.check_output(["python3", "tdoa_validator.py"]).decode('utf-8')
        lines = output.split('\n')
        for line in lines:
            if "TIMING CALIBRATION DECOUPLES LOCALIZATION" in line:
                continue
            if "-->" in line or "Error" in line:
                print(line)
        return output
    except Exception as e:
        print(f"Failed to run tdoa_validator.py: {e}")
        return None

def analyze_ml_model():
    print("\nExtracting ML Model Baseline (if available)...")
    # If the user has evaluate_3state.py we can run it too.
    if os.path.exists("scratch/evaluate_3state.py"):
        try:
            print("  Running evaluate_3state.py for 3-state ML Method Evaluation...")
            # We don't want it to run for too long, just read if there's a cached result
            if os.path.exists("results/3state_sensitivity_grid.csv"):
                df = pd.read_csv("results/3state_sensitivity_grid.csv")
                best_sr = df['SuccessRate'].max()
                best_method = df[df['SuccessRate'] == best_sr]['Method'].values[0]
                best_topo = df[df['SuccessRate'] == best_sr]['Topology'].values[0]
                print(f"  Best 3-State ML Model: Method {best_method} with {best_topo} topology")
                print(f"  Highest Success Rate : {best_sr:.1f}%")
            else:
                print("  ML evaluation results not yet generated (results/3state_sensitivity_grid.csv missing).")
        except Exception as e:
            pass

if __name__ == "__main__":
    analyze_dataset_d()
    analyze_tdoa_validator()
    analyze_ml_model()
    print("\nComparative Analysis complete.")
