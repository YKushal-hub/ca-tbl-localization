import pandas as pd
import numpy as np
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)


file_path = ROOT / 'results/ml_training_dataset.csv'
output_report = ROOT / 'results/ml_dataset_analysis.md'

print(f"Reading {file_path}...")
df = pd.read_csv(file_path)

print(f"Dataset shape: {df.shape}")

with open(output_report, 'w') as f:
    f.write("# ML Training Dataset Analysis Report\n\n")
    f.write(f"**Total Rows:** {df.shape[0]:,}\n")
    f.write(f"**Total Columns:** {df.shape[1]}\n\n")
    
    f.write("## 1. Overview\n\n")
    f.write("This document provides a detailed summary of the `ml_training_dataset.csv` file generated for GPS-denied localization ML models.\n\n")

    # Group columns
    categorical_cols = ['Environment', 'SpreadingFactor', 'IsTwoRay', 'IsTunnel', 'AdcClippingOccurred', 
                        'PreambleLocked', 'PayloadCrcSuccess', 'CollisionOccurred', 'CaptureSucceeded', 'PacketReceived']
    
    numerical_cols = [c for c in df.columns if c not in categorical_cols]

    f.write("## 2. Categorical / Boolean Parameters\n\n")
    for col in categorical_cols:
        if col in df.columns:
            f.write(f"### {col}\n")
            counts = df[col].value_counts(dropna=False).sort_index()
            for val, count in counts.items():
                pct = (count / len(df)) * 100
                f.write(f"- **{val}**: {count:,} ({pct:.2f}%)\n")
            f.write("\n")

    f.write("## 3. Numerical Parameters Summary\n\n")
    f.write("| Parameter | Count | Mean | Std Dev | Min | 25% | Median | 75% | Max |\n")
    f.write("|---|---|---|---|---|---|---|---|---|\n")
    
    for col in numerical_cols:
        if col in df.columns:
            desc = df[col].describe()
            f.write(f"| {col} | {int(desc['count']):,} | {desc['mean']:.4f} | {desc['std']:.4f} | {desc['min']:.4f} | {desc['25%']:.4f} | {desc['50%']:.4f} | {desc['75%']:.4f} | {desc['max']:.4f} |\n")
            
    f.write("\n## 4. Key Target Variable Analysis\n\n")
    if 'PacketReceived' in df.columns:
        rx_df = df[df['PacketReceived'] == 1]
        f.write("### Packets Successfully Received (PDR)\n")
        f.write(f"Total packets received: {len(rx_df):,} out of {len(df):,} ({(len(rx_df)/len(df))*100:.2f}%)\n\n")
        
        if 'DistanceM' in rx_df.columns:
            f.write("#### Reception distance distribution (successful packets only):\n")
            desc = rx_df['DistanceM'].describe()
            f.write(f"- Mean Distance: {desc['mean']:.2f} m\n")
            f.write(f"- Max Distance: {desc['max']:.2f} m\n\n")

    f.write("## 5. Tier-2 Submodule Features (GPS-Denied Localization)\n\n")
    f.write("The dataset includes key Tier-2 submodule metrics that are essential for localization ML:\n\n")
    t2_cols = ['BodyShadowingLossDb', 'TemporalBlockageLossDb', 'EstimatedRangeM', 'PositionErrorM', 'TimeSyncErrorNs']
    for c in t2_cols:
        if c in df.columns:
            non_zero = (df[c] != 0).sum()
            f.write(f"- **{c}**: {non_zero:,} rows ({(non_zero/len(df))*100:.2f}%) have non-zero values.\n")
            
print("Analysis complete!")
