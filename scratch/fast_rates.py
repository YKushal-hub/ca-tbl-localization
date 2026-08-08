import pandas as pd
from pathlib import Path

DST = Path(results)
for m in [0, 1, 2, 3, 4]:
    for t in [WAN, MESH, HYBRID]:
        df = pd.read_csv(DST / fmultimodal_M{m}_T{t}.csv, usecols=[FinalLocalizationMode])
        total = len(df)
        fused = (df[FinalLocalizationMode] == FUSED).sum()
        imu_only = (df[FinalLocalizationMode] == IMU_ONLY).sum()
        print(fM{m}
