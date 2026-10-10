import pandas as pd
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # repository root (this file lives in scratch/)

df = pd.read_csv(ROOT / "results/multimodal_M1_TWAN.csv", usecols=["TdoaEligible", "FinalLocalizationMode"])
print("TDOA Eligible Counts:")
print(df["TdoaEligible"].value_counts(normalize=True))
print("Final Mode Counts:")
print(df["FinalLocalizationMode"].value_counts(normalize=True))
