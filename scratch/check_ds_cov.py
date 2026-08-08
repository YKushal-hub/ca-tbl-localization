import pandas as pd
df = pd.read_csv('results/geometric_training_dataset_0_1.csv')
df = df[df['Topology'] == 'WAN']
print('WAN Coverage:', (df['GwObsCount'] > 0).mean())
print('TDOA Coverage:', df['TdoaEligible'].mean())
