"""Per-(radius, mode, physics) means of the regime sweep.

Usage: python3 average.py [path/to/regime_sweep_clean_vs_physics.csv]

The CSV has no header. Columns used: 1 radius_km, 2 mode, 3 physics_enabled,
6 PDR, 21 mean localisation error, 22 median localisation error, 24 TDoA fraction.
The default path is relative to the repository root, not to any one machine.
"""
import sys
from pathlib import Path

import pandas as pd

ROOT = Path(__file__).resolve().parent
DEFAULT = ROOT / 'regime_sweep_clean_vs_physics.csv'

path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT
if not path.exists():
    sys.exit(f'{path} not found; pass the sweep CSV as the first argument')

df = pd.read_csv(path, header=None)
res = df.groupby([1, 2, 3]).agg({6: 'mean', 21: 'mean', 22: 'mean', 24: 'mean'})
for (radius, mode, is_phys), row in res.iterrows():
    print(f'{radius}km {mode} phys={is_phys}: PDR={row[6] * 100:.2f}%, '
          f'TDOA={row[24] * 100:.2f}%, MeanErr={row[21]}, MedErr={row[22]}')
