import pandas as pd
import numpy as np

def generate_report():
    print("Loading dataset...")
    df = pd.read_csv('/home/kushal/ns/ns-3-dev/results/ml_training_dataset.csv')
    
    with open('/home/kushal/ns/ns-3-dev/scratch/dataset_5M_full_report.md', 'w') as f:
        f.write("# 5M ML Training Dataset: Comprehensive Analysis Report\n\n")
        f.write("This report provides a detailed statistical summary of the final 5 Million row ML training dataset.\n\n")
        
        # 1. High-level Summary
        f.write("## 1. High-Level Summary\n")
        f.write(f"- **Total Rows**: {len(df):,}\n")
        f.write(f"- **Total Columns**: {len(df.columns)}\n")
        f.write("- **Memory Usage**: {:.2f} MB\n\n".format(df.memory_usage(deep=True).sum() / (1024 * 1024)))
        
        # 2. Categorical / Boolean Distributions
        f.write("## 2. Categorical & Boolean Distributions\n")
        categorical_cols = [
            'Environment', 'SpreadingFactor', 'IsTwoRay', 'IsTunnel', 
            'AdcClippingOccurred', 'PreambleLocked', 'PayloadCrcSuccess', 
            'CollisionOccurred', 'CaptureSucceeded', 'PacketReceived'
        ]
        
        for col in categorical_cols:
            if col in df.columns:
                f.write(f"### {col}\n")
                counts = df[col].value_counts(dropna=False).sort_index()
                percentages = df[col].value_counts(normalize=True, dropna=False).sort_index() * 100
                
                f.write("| Value | Count | Percentage |\n")
                f.write("| :--- | :--- | :--- |\n")
                for val in counts.index:
                    f.write(f"| {val} | {counts[val]:,} | {percentages[val]:.2f}% |\n")
                f.write("\n")
                
        # 3. Numerical Distributions
        f.write("## 3. Numerical Feature Statistics\n")
        f.write("The following table provides the distribution statistics for all continuous numerical features. ")
        f.write("All counts represent the number of non-null entries.\n\n")
        
        num_cols = [c for c in df.columns if c not in categorical_cols]
        stats = df[num_cols].describe().T
        
        f.write("| Feature | Count | Mean | Std Dev | Min | 25% | Median (50%) | 75% | Max |\n")
        f.write("| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |\n")
        
        for idx, row in stats.iterrows():
            f.write(f"| **{idx}** | {int(row['count']):,} | {row['mean']:,.3f} | {row['std']:,.3f} | {row['min']:,.3f} | {row['25%']:,.3f} | {row['50%']:,.3f} | {row['75%']:,.3f} | {row['max']:,.3f} |\n")
            
        f.write("\n## 4. Sub-Module Activation Analysis\n")
        
        # Specific sub-module checks
        if 'TemporalBlockageLossDb' in df.columns:
            blocked = len(df[df['TemporalBlockageLossDb'] > 0])
            f.write(f"- **Temporal Blockage**: Active in {blocked:,} rows ({(blocked/len(df))*100:.2f}% of total dataset).\n")
            
        if 'BodyShadowingLossDb' in df.columns:
            body = len(df[df['BodyShadowingLossDb'] > 0])
            f.write(f"- **Body Shadowing**: Active in {body:,} rows ({(body/len(df))*100:.2f}% of total dataset).\n")
            
        if 'BuildingEntryLossDb' in df.columns:
            indoor = len(df[df['BuildingEntryLossDb'] > 0])
            f.write(f"- **Indoor (Building Entry)**: Active in {indoor:,} rows ({(indoor/len(df))*100:.2f}% of total dataset).\n")
            
        if 'NlosDb' in df.columns:
            nlos = len(df[df['NlosDb'] > 0])
            f.write(f"- **NLOS**: Active in {nlos:,} rows ({(nlos/len(df))*100:.2f}% of total dataset).\n")
            
    print("Report generated successfully at /home/kushal/ns/ns-3-dev/scratch/dataset_5M_full_report.md")

if __name__ == '__main__':
    generate_report()
