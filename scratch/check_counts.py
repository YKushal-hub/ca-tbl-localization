import pandas as pd
df = pd.read_csv("/home/kushal/ns/ns-3-dev/results/multimodal_M1_TWAN.csv", usecols=["TdoaEligible", "FinalLocalizationMode"])
print("TDOA Eligible Counts:")
print(df["TdoaEligible"].value_counts(normalize=True))
print("Final Mode Counts:")
print(df["FinalLocalizationMode"].value_counts(normalize=True))
