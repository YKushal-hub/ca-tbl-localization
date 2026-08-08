import pandas as pd
import numpy as np

def fix_dataset():
    print("Loading tdoa_processed_dataset.csv...")
    df = pd.read_csv("results/tdoa_processed_dataset.csv")

    # PREVIOUS BEHAVIOUR (now retired):
    # This script used to copy WAN's PositionErrorM and GwObsCount into MESH and HYBRID rows
    # because an older version of the simulator only placed WAN gateways.
    # topology_observability_sim.cc now generates per-topology geometry correctly,
    # so MESH and HYBRID already have their own GwObsCount and TdoaEligible values.
    # The WAN-copy logic is therefore removed; we simply pass through the dataset as-is.

    print("Saving fixed dataset (per-topology geometry preserved)...")
    df.to_csv("results/tdoa_processed_dataset_fixed.csv", index=False)

    # Sanity check
    print("TdoaEligible rate by Topology:")
    print(df.groupby("Topology")["TdoaEligible"].mean())
    print("\nTdoaEligible count by Topology:")
    print(df.groupby("Topology")["TdoaEligible"].sum())
    print("\nPositionErrorM median by Topology (eligible only):")
    print(df[df.TdoaEligible==1].groupby("Topology")["PositionErrorM"].median())
    print("\nDone!")

if __name__ == "__main__":
    fix_dataset()
