import pandas as pd
import numpy as np

# Check source dataset first - what NlosDb values cause the mismatch?
src = "./results/ml_training_dataset.csv"
print("=== SOURCE DATASET: NlosDb distribution near 0 ===")
for chunk in pd.read_csv(src, chunksize=500_000):
    tiny = chunk[(chunk["NlosDb"] > 0) & (chunk["NlosDb"] <= 0.05)]
    if len(tiny) > 0:
        print("Rows with 0 < NlosDb <= 0.05: {}".format(len(tiny)))
        print(tiny["NlosDb"].value_counts().head(20))
        break

# Now check multimodal file - what is Stage2IsNLOS for those rows?
path = "./results/multimodal_M1_TWAN.csv"
print("\n=== M1_TWAN: mismatched rows ===")
count = 0
for chunk in pd.read_csv(path, chunksize=500_000):
    inherited_old = (chunk["NlosDb"].fillna(0.0) > 0).astype(int)    # original threshold
    inherited_new = (chunk["NlosDb"].fillna(0.0) > 0.01).astype(int)  # new threshold
    recorded      = chunk["Stage2IsNLOS"].fillna(0.0).astype(int)
    # Old logic mismatch (what caused the original 1-row fail)
    old_mismatch = chunk[inherited_old != recorded]
    # New logic mismatch (what's failing now)
    new_mismatch = chunk[inherited_new != recorded]
    if len(new_mismatch) > 0 and count == 0:
        print("New-threshold mismatches in first chunk: {}".format(len(new_mismatch)))
        print("NlosDb values in mismatched rows:")
        print(new_mismatch["NlosDb"].describe())
        print(new_mismatch["NlosDb"].value_counts().head(10))
        print("Stage2IsNLOS values:", new_mismatch["Stage2IsNLOS"].value_counts().to_dict())
        count += 1
    if count >= 1:
        break

# The real question: what threshold would give 0 mismatches?
print("\n=== Finding correct threshold ===")
for chunk in pd.read_csv(path, chunksize=500_000):
    recorded = chunk["Stage2IsNLOS"].fillna(0.0).astype(int)
    nlos = chunk["NlosDb"].fillna(0.0)
    # rows where Stage2IsNLOS=1 but NlosDb is tiny
    s2_nlos1 = chunk[recorded == 1]
    if len(s2_nlos1) > 0:
        print("Min NlosDb where Stage2IsNLOS=1:", s2_nlos1["NlosDb"].min())
        print("Max NlosDb where Stage2IsNLOS=0:", chunk[recorded == 0]["NlosDb"].max())
    break
