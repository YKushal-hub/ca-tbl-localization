# ORACLE-ESC — seeds 321-340 (scripts/oracle_esc_analyse.py)

Source(s): e8195863. Intervals: paired seed bootstrap, 10000 resamples, 99 %.

## Geometry A (5 km)

| config | five-node A(25 m) | tag-0 A(25 m) | five-node J | tag-0 CEP95 (m) | five-node escalations |
|---|---|---|---|---|---|
| K T3 | 0.809 | 0.985 | 14.35 | 7.3 | 65.7 |
| K T12 | 0.826 | 0.986 | 13.48 | 6.7 | 107.1 |
| K T50 | 0.785 | 0.977 | 13.47 | 9.0 | 143.5 |
| K T200 | 0.832 | 0.984 | 13.71 | 7.9 | 151.4 |
| O T3 | 0.792 | 0.985 | 14.04 | 6.8 | 86.8 |
| O T12 | 0.814 | 0.986 | 13.32 | 7.1 | 118.4 |
| O T50 | 0.778 | 0.982 | 13.61 | 8.0 | 141.5 |
| O T200 | 0.805 | 0.986 | 13.36 | 7.0 | 148.1 |

P1 (O escalates more than K at the same T): paired difference in five-node escalations

| T | O - K escalations | P1 holds |
|---|---|---|
| 3 | +21.1 [+11.5, +31.3] | True |
| 12 | +11.3 [-0.3, +24.0] | False |
| 50 | -2.0 [-8.5, +6.2] | False |
| 200 | -3.4 [-12.2, +4.8] | False |

| O T | K T | dA five-node A(25 m) | dJ five-node J | accuracy-qualifying | energy-qualifying | saving lower bound |
|---|---|---|---|---|---|---|
| 3 | 3 | -0.017 [-0.072, +0.043] | -0.30 [-0.86, +0.25] | False | False | - |
| 3 | 12 | -0.034 [-0.109, +0.033] | +0.57 [+0.01, +1.07] | False | False | - |
| 3 | 50 | +0.007 [-0.109, +0.128] | +0.57 [-0.66, +1.43] | False | False | - |
| 3 | 200 | -0.040 [-0.148, +0.056] | +0.33 [-0.74, +1.29] | False | False | - |
| 12 | 3 | +0.005 [-0.068, +0.089] | -1.03 [-1.69, -0.41] | False | False | - |
| 12 | 12 | -0.013 [-0.076, +0.057] | -0.16 [-0.73, +0.40] | False | False | - |
| 12 | 50 | +0.029 [-0.052, +0.132] | -0.15 [-1.23, +0.47] | False | False | - |
| 12 | 200 | -0.019 [-0.075, +0.036] | -0.39 [-1.27, +0.29] | False | False | - |
| 50 | 3 | -0.031 [-0.094, +0.043] | -0.73 [-1.32, -0.16] | False | False | - |
| 50 | 12 | -0.048 [-0.092, -0.008] | +0.14 [-0.51, +0.86] | False | False | - |
| 50 | 50 | -0.007 [-0.099, +0.096] | +0.14 [-1.18, +1.00] | False | False | - |
| 50 | 200 | -0.055 [-0.140, +0.017] | -0.10 [-1.01, +0.69] | False | False | - |
| 200 | 3 | -0.004 [-0.119, +0.100] | -0.98 [-1.66, -0.30] | False | False | - |
| 200 | 12 | -0.022 [-0.107, +0.040] | -0.11 [-0.78, +0.57] | False | False | - |
| 200 | 50 | +0.019 [-0.069, +0.126] | -0.11 [-1.01, +0.43] | False | False | - |
| 200 | 200 | -0.028 [-0.108, +0.035] | -0.35 [-1.19, +0.25] | False | False | - |

**Accuracy headroom A: NONE**. **Energy headroom A: NONE**. P1 holds at every T: False.

## Geometry B (endgame)

| config | five-node A(25 m) | tag-0 A(25 m) | five-node J | tag-0 CEP95 (m) | five-node escalations |
|---|---|---|---|---|---|
| K T3 | 0.962 | 0.997 | 20.30 | 2.9 | 123.6 |
| K T12 | 0.977 | 0.997 | 16.81 | 2.6 | 167.5 |
| K T50 | 0.975 | 0.997 | 16.49 | 2.6 | 179.2 |
| K T200 | 0.955 | 0.997 | 16.70 | 2.8 | 170.7 |
| O T3 | 0.944 | 0.997 | 20.82 | 2.8 | 131.8 |
| O T12 | 0.976 | 0.997 | 17.72 | 2.8 | 166.1 |
| O T50 | 0.972 | 0.997 | 16.73 | 2.9 | 179.5 |
| O T200 | 0.977 | 0.997 | 16.25 | 2.7 | 177.2 |

P1 (O escalates more than K at the same T): paired difference in five-node escalations

| T | O - K escalations | P1 holds |
|---|---|---|
| 3 | +8.2 [-3.4, +19.9] | False |
| 12 | -1.4 [-11.6, +8.3] | False |
| 50 | +0.2 [-9.7, +11.2] | False |
| 200 | +6.5 [-3.1, +16.3] | False |

| O T | K T | dA five-node A(25 m) | dJ five-node J | accuracy-qualifying | energy-qualifying | saving lower bound |
|---|---|---|---|---|---|---|
| 3 | 3 | -0.018 [-0.052, +0.003] | +0.51 [-1.85, +2.32] | False | False | - |
| 3 | 12 | -0.033 [-0.119, +0.004] | +4.01 [+2.53, +5.49] | False | False | - |
| 3 | 50 | -0.031 [-0.118, +0.009] | +4.33 [+2.81, +5.88] | False | False | - |
| 3 | 200 | -0.011 [-0.102, +0.052] | +4.11 [+2.40, +5.85] | False | False | - |
| 12 | 3 | +0.014 [-0.011, +0.071] | -2.59 [-4.90, -0.66] | False | True | 0.032 |
| 12 | 12 | -0.002 [-0.006, +0.002] | +0.91 [-0.18, +2.13] | False | False | - |
| 12 | 50 | +0.000 [-0.009, +0.007] | +1.23 [+0.07, +2.55] | False | False | - |
| 12 | 200 | +0.020 [-0.003, +0.060] | +1.01 [-0.13, +2.25] | False | False | - |
| 50 | 3 | +0.010 [-0.017, +0.069] | -3.57 [-5.26, -1.88] | False | True | 0.092 |
| 50 | 12 | -0.006 [-0.015, +0.001] | -0.07 [-1.14, +0.94] | False | False | - |
| 50 | 50 | -0.004 [-0.011, +0.002] | +0.25 [-0.75, +1.26] | False | False | - |
| 50 | 200 | +0.017 [-0.007, +0.055] | +0.03 [-0.92, +1.05] | False | False | - |
| 200 | 3 | +0.015 [-0.008, +0.072] | -4.05 [-5.51, -2.67] | False | True | 0.131 |
| 200 | 12 | -0.000 [-0.003, +0.003] | -0.56 [-1.46, +0.31] | False | False | - |
| 200 | 50 | +0.002 [-0.004, +0.007] | -0.24 [-1.30, +0.74] | False | False | - |
| 200 | 200 | +0.022 [+0.000, +0.059] | -0.45 [-1.50, +0.51] | True | False | - |

**Accuracy headroom B: SMALL** (H = +0.000). **Energy headroom B: MATERIAL** (E = 0.131). P1 holds at every T: False.

## Registered reading

- A learned confidence input cannot improve this trigger in this simulator: False
- A learned confidence input has room (some headroom MATERIAL): True

