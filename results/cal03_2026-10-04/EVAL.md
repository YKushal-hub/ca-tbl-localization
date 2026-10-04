# CAL-03 evaluation on held-out seeds 21-40 (scripts/cal03_analyse.py eval)

## main

| arm | availability | CEP50 (m) | CEP95 (m) | escalations | occupancy 868 / 2.4 / UWB | subject J |
|---|---|---|---|---|---|---|
| calibrated | 0.141 | 22.9 | 64.3 | 61.5 | 0.194 / 0.805 / 0.001 | 13.24 |
| legacy | 0.142 | 20.7 | 64.1 | 58.1 | 0.175 / 0.824 / 0.001 | 13.47 |
| fixed_threshold | 0.151 | 39.3 | 113.1 | 46.0 | 0.465 / 0.535 / 0.000 | 11.66 |

Mean |occupancy difference| to fixed_threshold: calibrated 0.552, legacy 0.584.

## end

| arm | availability | CEP50 (m) | CEP95 (m) | escalations | occupancy 868 / 2.4 / UWB | subject J |
|---|---|---|---|---|---|---|
| calibrated | 0.417 | 7.6 | 18.4 | 53.0 | 0.085 / 0.870 / 0.046 | 21.84 |
| legacy | 0.410 | 7.5 | 17.8 | 54.1 | 0.080 / 0.876 / 0.044 | 23.26 |
| fixed_threshold | 0.438 | 7.8 | 19.1 | 36.3 | 0.728 / 0.250 / 0.023 | 14.08 |

Mean |occupancy difference| to fixed_threshold: calibrated 1.292, legacy 1.303.

