import pandas as pd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)

df = pd.read_csv(ROOT / "results/tdoa_processed_dataset_fixed.csv", usecols=["Topology", "TdoaEligible"])
print(df[df["Topology"] == "WAN"].value_counts(normalize=True))
