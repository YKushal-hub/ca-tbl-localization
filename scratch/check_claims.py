import pandas as pd
import json

# Check Column Count
print("--- Column Count Check ---")
try:
    df = pd.read_csv('/home/kushal/ns/ns-3-dev/datasets/dataset_C_main.csv', nrows=1)
    print(f"Dataset C has {len(df.columns)} columns.")
    if len(df.columns) != 44:
        print(f"Missing/Extra columns compared to 44. Expected 44, got {len(df.columns)}.")
except Exception as e:
    print(f"Error checking columns: {e}")

# Check Shadowing Stochasticity
print("--- Shadowing Stochasticity ---")
try:
    df_s = pd.read_csv('/home/kushal/ns/ns-3-dev/datasets/dataset_D_shadowing.csv', nrows=100)
    if 'Shadowing_dB' in df_s.columns:
        std = df_s['Shadowing_dB'].std()
        print(f"Shadowing standard deviation: {std:.2f} dB")
        print("Shadowing is stochastic: " + str(std > 0))
    else:
        print("Shadowing_dB column missing.")
except Exception as e:
    print(f"Error checking shadowing: {e}")

# Check Dataset E Weather
print("--- Dataset E Weather Variation ---")
try:
    df_e = pd.read_csv('/home/kushal/ns/ns-3-dev/datasets/dataset_E_weather.csv', usecols=['Weather_State'])
    counts = df_e['Weather_State'].value_counts().to_dict()
    print(f"Weather states: {counts}")
except Exception as e:
    print(f"Error checking weather: {e}")

# Check Dataset D Row Count
print("--- Dataset D Row Count ---")
try:
    df_d = pd.read_csv('/home/kushal/ns/ns-3-dev/datasets/dataset_D_shadowing.csv', usecols=[0])
    print(f"Dataset D row count: {len(df_d)}")
except Exception as e:
    print(f"Error checking Dataset D: {e}")
