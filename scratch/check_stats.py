import pandas as pd

df = pd.read_csv("/home/kushal/ns/ns-3-dev/results/tdoa_processed_dataset_fixed.csv")
print("TdoaEligible mean by Topology:")
print(df.groupby("Topology")["TdoaEligible"].mean())
print("\nPositionErrorM mean by Topology:")
print(df.groupby("Topology")["PositionErrorM"].mean())
