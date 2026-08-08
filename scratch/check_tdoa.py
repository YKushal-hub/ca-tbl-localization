import pandas as pd
df = pd.read_csv("/home/kushal/ns/ns-3-dev/results/tdoa_processed_dataset_fixed.csv", usecols=["Topology", "TdoaEligible"])
print(df[df["Topology"] == "WAN"].value_counts(normalize=True))
