import pandas as pd

for top in ['WAN', 'MESH', 'HYBRID']:
    file = f'/home/kushal/ns/ns-3-dev/results/multimodal_M0_T{top}.csv'
    try:
        df = pd.read_csv(file)
        if 'Error_m' in df.columns:
            mae = df['Error_m'].mean()
            print(f"Method 0, {top} Exact Overall_MAE_m: {mae:.15f}")
        else:
            print(f"Method 0, {top} missing Error_m")
    except Exception as e:
        print(f"Failed {top}: {e}")
