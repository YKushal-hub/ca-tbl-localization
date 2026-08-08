import pandas as pd
df = pd.read_csv('results/smoke_test_50k.csv')
for env_id in [3, 4]:
    edf = df[df['Environment']==env_id]
    nlos_std = edf[edf['NlosDb']>0]['ShadowingDb'].std()
    los_std  = edf[edf['NlosDb']==0]['ShadowingDb'].std()
    print(f"Env {env_id}: LOS std={los_std:.3f}, NLOS std={nlos_std:.3f}, ratio={nlos_std/los_std:.3f}")
