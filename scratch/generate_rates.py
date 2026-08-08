import pandas as pd
import json

df = pd.read_csv("results/geometric_training_dataset.csv")
rates = df.groupby("Topology")["TdoaEligible"].mean().to_dict()
rates["runs"] = df["Run"].nunique()

with open("results/topology_rates_summary.json", "w") as f:
    json.dump(rates, f, indent=2)
print("Saved topology_rates_summary.json")
