import pandas as pd

path = "./results/multimodal_M1_TWAN.csv"
chunk = pd.read_csv(path, nrows=5000)
print("NlosDb NaNs:", chunk["NlosDb"].isna().sum())
print("Stage2IsNLOS NaNs:", chunk["Stage2IsNLOS"].isna().sum())
print("NlosDb stats:")
print(chunk["NlosDb"].describe())
print("Stage2IsNLOS stats:")
print(chunk["Stage2IsNLOS"].describe())
