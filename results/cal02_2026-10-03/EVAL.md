# CAL-02 evaluation on held-out seeds 21-40 (scripts/cal02_analyse.py eval)

## main

| arm | availability | CEP50 (m) | CEP95 (m) | escalations | occupancy 868 / 2.4 / UWB | subject J |
|---|---|---|---|---|---|---|
| calibrated | 0.337 | 19.8 | 42.3 | 103.0 | 0.166 / 0.833 / 0.001 | 14.95 |
| legacy | 0.333 | 19.6 | 43.4 | 104.0 | 0.167 / 0.833 / 0.000 | 14.71 |
| fixed_threshold | 0.376 | 18.1 | 33.0 | 42.5 | 0.694 / 0.304 / 0.001 | 11.53 |

Mean |occupancy difference| to fixed_threshold: calibrated 1.058, legacy 1.057.

## end

| arm | availability | CEP50 (m) | CEP95 (m) | escalations | occupancy 868 / 2.4 / UWB | subject J |
|---|---|---|---|---|---|---|
| calibrated | 0.635 | 5.3 | 9.4 | 14.7 | 0.003 / 0.908 / 0.088 | 35.45 |
| legacy | 0.637 | 5.4 | 9.2 | 14.0 | 0.005 / 0.911 / 0.084 | 33.48 |
| fixed_threshold | 0.642 | 4.2 | 7.3 | 49.7 | 0.638 / 0.319 / 0.043 | 22.41 |

Mean |occupancy difference| to fixed_threshold: calibrated 1.269, legacy 1.266.

