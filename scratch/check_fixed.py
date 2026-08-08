import pandas as pd
import json

# Check topology rates
rates = json.load(open("results/topology_rates_summary.json"))
print("Topology rates:", rates)

# Check fix_dataset output - do topologies differ?
df = pd.read_csv("results/tdoa_processed_dataset_fixed.csv", usecols=["Topology", "TdoaEligible", "PositionErrorM"])
print("\nTdoaEligible rate by Topology (should differ: WAN=35%, MESH=100%, HYBRID=47%):")
print(df.groupby("Topology")["TdoaEligible"].mean())
print("\nPositionErrorM stats by Topology (eligible rows only):")
print(df[df.TdoaEligible==1].groupby("Topology")["PositionErrorM"].describe())
print("\nPositionErrorM stats by Topology (ineligible rows only):")
print(df[df.TdoaEligible==0].groupby("Topology")["PositionErrorM"].describe())
