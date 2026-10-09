# TOF-FIELD — 2.4 GHz ToF ranging at measured outdoor error (scripts/tof_field_analyse.py)

Settings: D0 = default (3.0 m floor, no bias); R1 = rural: sigma 83.6 m, bias +46.4 m; R0 = rural spread, bias removed: sigma 83.6 m, bias 0; S = suburban: sigma 45 m, bias +39 m.

Mean [95 % bootstrap] over seeds 1-20; differences paired by seed.

## 5 km (A_main)

### D0: default (3.0 m floor, no bias)

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.285 [0.251, 0.320] | 24.4 / 57.8 | 65.1 | 16.69 |
| fixed threshold (same preset) | 0.274 [0.240, 0.310] | 20.7 / 57.8 | 61.4 | 16.94 |
| static 2.4 GHz | 0.031 [0.022, 0.041] | 24.5 / 54.9 | 53.1 | 14.21 |
| dual-band LoRa fusion | 0.226 [0.201, 0.251] | 39.9 / 94.3 | 84.0 | 16.81 |
| Poucet (LoRa + UWB fused) | 0.239 [0.211, 0.269] | 36.6 / 96.1 | 97.0 | 461.73 |

### R1: rural: sigma 83.6 m, bias +46.4 m

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.279 [0.247, 0.310] | 49.7 / 93.0 | 92.1 | 22.46 |
| fixed threshold (same preset) | 0.277 [0.245, 0.311] | 48.2 / 91.0 | 92.0 | 21.83 |
| static 2.4 GHz | 0.026 [0.019, 0.034] | 65.2 / 104.3 | 102.1 | 14.17 |
| dual-band LoRa fusion | 0.231 [0.202, 0.259] | 58.8 / 106.5 | 107.8 | 16.82 |
| Poucet (LoRa + UWB fused) | 0.233 [0.208, 0.257] | 58.5 / 129.8 | 122.0 | 462.44 |

### R0: rural spread, bias removed: sigma 83.6 m, bias 0

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.280 [0.246, 0.313] | 35.3 / 79.7 | 84.9 | 28.64 |
| fixed threshold (same preset) | 0.277 [0.243, 0.312] | 35.8 / 85.9 | 84.9 | 28.36 |
| static 2.4 GHz | 0.025 [0.018, 0.034] | 35.4 / 73.0 | 77.6 | 14.05 |
| dual-band LoRa fusion | 0.228 [0.196, 0.259] | 50.0 / 103.3 | 100.7 | 16.90 |
| Poucet (LoRa + UWB fused) | 0.226 [0.198, 0.256] | 59.0 / 121.4 | 107.5 | 462.21 |

### S: suburban: sigma 45 m, bias +39 m

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.287 [0.253, 0.321] | 54.5 / 96.2 | 92.5 | 19.31 |
| fixed threshold (same preset) | 0.287 [0.254, 0.320] | 53.2 / 97.6 | 89.0 | 18.89 |
| static 2.4 GHz | 0.029 [0.020, 0.040] | 62.5 / 94.2 | 96.4 | 14.01 |
| dual-band LoRa fusion | 0.223 [0.190, 0.255] | 46.7 / 90.9 | 100.8 | 16.79 |
| Poucet (LoRa + UWB fused) | 0.220 [0.196, 0.244] | 65.3 / 120.4 | 113.4 | 462.40 |

Paired, setting − D0, per arm (5 km (A_main)):

| arm | setting | Δ availability | Δ CEP95 tag 0 (m) | CEP95 ratio − 1 | Δ subject J |
|---|---|---|---|---|---|
| CA-TBL (preset) | R1 | -0.006 [-0.014, 0.002] | 35.2 [18.0, 52.7] | +1.296 | 5.77 [3.60, 8.03] |
| CA-TBL (preset) | R0 | -0.006 [-0.019, 0.006] | 21.9 [9.3, 35.9] | +0.660 | 11.95 [8.67, 15.76] |
| CA-TBL (preset) | S | 0.002 [-0.009, 0.013] | 38.4 [24.3, 52.9] | +1.102 | 2.62 [0.73, 5.37] |
| fixed threshold (same preset) | R1 | 0.003 [-0.008, 0.015] | 33.2 [16.1, 50.4] | +1.113 | 4.88 [2.77, 7.32] |
| fixed threshold (same preset) | R0 | 0.003 [-0.011, 0.015] | 28.1 [12.7, 46.4] | +0.722 | 11.42 [7.70, 15.34] |
| fixed threshold (same preset) | S | 0.013 [-0.001, 0.026] | 39.8 [19.4, 61.8] | +1.215 | 1.95 [0.35, 4.40] |
| static 2.4 GHz | R1 | -0.005 [-0.008, -0.002] | 49.3 [19.0, 76.5] | +1.782 | -0.04 [-0.24, 0.19] |
| static 2.4 GHz | R0 | -0.005 [-0.009, -0.002] | 18.1 [-11.0, 43.8] | +1.074 | -0.16 [-0.38, 0.05] |
| static 2.4 GHz | S | -0.001 [-0.004, 0.001] | 39.2 [12.9, 60.6] | +1.393 | -0.19 [-0.48, 0.06] |
| dual-band LoRa fusion | R1 | 0.005 [-0.011, 0.022] | 12.2 [-5.8, 29.7] | +0.336 | 0.01 [-0.33, 0.33] |
| dual-band LoRa fusion | R0 | 0.002 [-0.012, 0.017] | 9.0 [-14.7, 33.1] | +0.284 | 0.09 [-0.16, 0.35] |
| dual-band LoRa fusion | S | -0.002 [-0.024, 0.018] | -3.4 [-27.8, 19.2] | +0.308 | -0.02 [-0.37, 0.34] |
| Poucet (LoRa + UWB fused) | R1 | -0.006 [-0.020, 0.006] | 33.7 [-5.5, 84.4] | +0.781 | 0.71 [0.07, 1.39] |
| Poucet (LoRa + UWB fused) | R0 | -0.014 [-0.026, -0.001] | 25.3 [3.1, 45.9] | +0.512 | 0.48 [-0.14, 1.09] |
| Poucet (LoRa + UWB fused) | S | -0.020 [-0.034, -0.006] | 24.3 [-3.6, 56.4] | +0.545 | 0.67 [-0.11, 1.51] |

Paired comparisons within each setting (5 km (A_main)):

| comparison | setting | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| CA-TBL (preset) − fixed threshold (same preset) | D0 | 0.011 [0.001, 0.020] | -0.1 [-9.2, 10.4] | -0.25 [-0.94, 0.29] |
| CA-TBL (preset) − fixed threshold (same preset) | R1 | 0.002 [-0.003, 0.006] | 2.0 [-1.1, 5.7] | 0.63 [-0.47, 1.88] |
| CA-TBL (preset) − fixed threshold (same preset) | R0 | 0.002 [-0.002, 0.008] | -6.2 [-12.9, -0.7] | 0.28 [-0.96, 1.59] |
| CA-TBL (preset) − fixed threshold (same preset) | S | 0.000 [-0.007, 0.008] | -1.4 [-6.9, 3.9] | 0.41 [-0.06, 0.90] |
| CA-TBL (preset) − dual-band LoRa fusion | D0 | 0.060 [0.037, 0.081] | -36.5 [-62.4, -13.5] | -0.12 [-0.70, 0.55] |
| CA-TBL (preset) − dual-band LoRa fusion | R1 | 0.048 [0.028, 0.068] | -13.5 [-44.1, 13.5] | 5.63 [3.31, 8.02] |
| CA-TBL (preset) − dual-band LoRa fusion | R0 | 0.052 [0.029, 0.075] | -23.6 [-52.2, 1.1] | 11.73 [8.33, 15.73] |
| CA-TBL (preset) − dual-band LoRa fusion | S | 0.064 [0.038, 0.088] | 5.3 [-15.1, 27.9] | 2.52 [0.53, 5.63] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | D0 | 0.046 [0.022, 0.070] | -38.3 [-59.8, -20.3] | -445.04 [-446.12, -443.83] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | R1 | 0.046 [0.025, 0.066] | -36.8 [-81.5, -4.5] | -439.98 [-442.58, -437.25] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | R0 | 0.054 [0.033, 0.075] | -41.7 [-61.0, -24.0] | -433.57 [-437.23, -429.41] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | S | 0.067 [0.045, 0.091] | -24.2 [-64.4, 9.2] | -443.10 [-445.27, -439.89] |
| static 2.4 GHz − CA-TBL (preset) | D0 | -0.255 [-0.288, -0.222] | -2.8 [-22.8, 19.6] | -2.48 [-3.50, -1.28] |
| static 2.4 GHz − CA-TBL (preset) | R1 | -0.253 [-0.283, -0.224] | 11.3 [-8.7, 35.0] | -8.28 [-10.94, -5.65] |
| static 2.4 GHz − CA-TBL (preset) | R0 | -0.254 [-0.287, -0.223] | -6.7 [-29.6, 17.9] | -14.59 [-18.27, -11.13] |
| static 2.4 GHz − CA-TBL (preset) | S | -0.258 [-0.290, -0.226] | -2.0 [-22.9, 16.2] | -5.29 [-8.59, -2.74] |

## endgame (B_end)

### D0: default (3.0 m floor, no bias)

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.562 [0.538, 0.585] | 5.7 / 12.0 | 18.1 | 22.97 |
| fixed threshold (same preset) | 0.545 [0.520, 0.569] | 5.6 / 12.1 | 17.6 | 22.02 |
| static 2.4 GHz | 0.271 [0.261, 0.282] | 4.9 / 10.8 | 16.2 | 5.81 |
| dual-band LoRa fusion | 0.496 [0.474, 0.519] | 6.6 / 14.5 | 32.6 | 9.45 |
| Poucet (LoRa + UWB fused) | 0.511 [0.491, 0.530] | 6.6 / 15.0 | 38.5 | 415.45 |

### R1: rural: sigma 83.6 m, bias +46.4 m

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.564 [0.537, 0.591] | 18.6 / 39.3 | 62.9 | 38.72 |
| fixed threshold (same preset) | 0.557 [0.532, 0.582] | 17.6 / 38.2 | 65.1 | 26.93 |
| static 2.4 GHz | 0.240 [0.230, 0.251] | 47.3 / 70.0 | 91.9 | 5.66 |
| dual-band LoRa fusion | 0.493 [0.473, 0.513] | 16.4 / 39.6 | 66.6 | 8.57 |
| Poucet (LoRa + UWB fused) | 0.494 [0.476, 0.512] | 22.0 / 49.1 | 71.9 | 420.36 |

### R0: rural spread, bias removed: sigma 83.6 m, bias 0

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.545 [0.519, 0.570] | 14.5 / 35.1 | 50.6 | 52.96 |
| fixed threshold (same preset) | 0.532 [0.508, 0.554] | 16.3 / 37.8 | 50.4 | 42.10 |
| static 2.4 GHz | 0.231 [0.220, 0.243] | 21.2 / 41.5 | 55.4 | 5.59 |
| dual-band LoRa fusion | 0.476 [0.461, 0.493] | 18.5 / 40.0 | 56.3 | 8.61 |
| Poucet (LoRa + UWB fused) | 0.489 [0.470, 0.506] | 15.3 / 36.5 | 57.9 | 417.66 |

### S: suburban: sigma 45 m, bias +39 m

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |
|---|---|---|---|---|
| CA-TBL (preset) | 0.581 [0.557, 0.605] | 21.2 / 39.1 | 59.1 | 31.54 |
| fixed threshold (same preset) | 0.558 [0.534, 0.581] | 19.9 / 41.0 | 53.1 | 22.50 |
| static 2.4 GHz | 0.258 [0.246, 0.269] | 36.7 / 55.6 | 72.1 | 5.64 |
| dual-band LoRa fusion | 0.511 [0.491, 0.530] | 16.9 / 41.1 | 63.3 | 8.58 |
| Poucet (LoRa + UWB fused) | 0.494 [0.475, 0.514] | 20.8 / 43.1 | 68.1 | 420.40 |

Paired, setting − D0, per arm (endgame (B_end)):

| arm | setting | Δ availability | Δ CEP95 tag 0 (m) | CEP95 ratio − 1 | Δ subject J |
|---|---|---|---|---|---|
| CA-TBL (preset) | R1 | 0.002 [-0.006, 0.010] | 27.3 [22.1, 33.4] | +2.614 | 15.74 [12.34, 19.42] |
| CA-TBL (preset) | R0 | -0.017 [-0.026, -0.008] | 23.1 [17.2, 29.7] | +1.998 | 29.98 [26.49, 33.48] |
| CA-TBL (preset) | S | 0.019 [0.009, 0.028] | 27.1 [20.4, 34.9] | +2.516 | 8.56 [5.81, 11.50] |
| fixed threshold (same preset) | R1 | 0.012 [0.003, 0.022] | 26.0 [21.0, 32.2] | +2.599 | 4.91 [2.81, 7.13] |
| fixed threshold (same preset) | R0 | -0.013 [-0.024, -0.004] | 25.7 [17.9, 34.6] | +2.301 | 20.07 [17.49, 22.57] |
| fixed threshold (same preset) | S | 0.013 [0.002, 0.024] | 28.9 [23.3, 35.2] | +2.911 | 0.47 [-1.01, 1.86] |
| static 2.4 GHz | R1 | -0.031 [-0.037, -0.025] | 59.2 [49.3, 69.1] | +6.160 | -0.14 [-0.24, -0.03] |
| static 2.4 GHz | R0 | -0.040 [-0.047, -0.034] | 30.7 [24.6, 37.6] | +3.145 | -0.22 [-0.29, -0.14] |
| static 2.4 GHz | S | -0.014 [-0.019, -0.009] | 44.8 [37.9, 52.9] | +4.794 | -0.17 [-0.24, -0.09] |
| dual-band LoRa fusion | R1 | -0.003 [-0.021, 0.014] | 25.1 [17.0, 33.9] | +1.971 | -0.87 [-1.06, -0.71] |
| dual-band LoRa fusion | R0 | -0.021 [-0.038, -0.004] | 25.5 [18.0, 33.7] | +2.318 | -0.84 [-0.94, -0.74] |
| dual-band LoRa fusion | S | 0.014 [0.000, 0.028] | 26.6 [19.4, 33.9] | +2.174 | -0.87 [-1.02, -0.73] |
| Poucet (LoRa + UWB fused) | R1 | -0.017 [-0.030, -0.004] | 34.0 [23.2, 45.7] | +2.420 | 4.91 [1.87, 8.33] |
| Poucet (LoRa + UWB fused) | R0 | -0.022 [-0.034, -0.011] | 21.5 [13.9, 29.9] | +1.456 | 2.21 [-1.03, 4.86] |
| Poucet (LoRa + UWB fused) | S | -0.016 [-0.030, -0.003] | 28.0 [20.5, 35.7] | +2.067 | 4.95 [2.08, 7.61] |

Paired comparisons within each setting (endgame (B_end)):

| comparison | setting | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| CA-TBL (preset) − fixed threshold (same preset) | D0 | 0.017 [0.013, 0.021] | -0.1 [-1.6, 1.6] | 0.95 [-0.43, 2.34] |
| CA-TBL (preset) − fixed threshold (same preset) | R1 | 0.007 [-0.004, 0.017] | 1.1 [-0.5, 3.0] | 11.79 [8.17, 15.52] |
| CA-TBL (preset) − fixed threshold (same preset) | R0 | 0.013 [0.004, 0.021] | -2.7 [-6.4, 0.5] | 10.86 [7.93, 13.57] |
| CA-TBL (preset) − fixed threshold (same preset) | S | 0.023 [0.012, 0.035] | -2.0 [-5.0, 1.2] | 9.04 [6.90, 11.21] |
| CA-TBL (preset) − dual-band LoRa fusion | D0 | 0.066 [0.055, 0.077] | -2.5 [-4.9, -0.3] | 13.53 [11.97, 15.27] |
| CA-TBL (preset) − dual-band LoRa fusion | R1 | 0.072 [0.054, 0.089] | -0.3 [-7.8, 7.0] | 30.15 [26.68, 33.68] |
| CA-TBL (preset) − dual-band LoRa fusion | R0 | 0.069 [0.050, 0.088] | -4.9 [-12.2, 1.9] | 44.35 [40.54, 48.13] |
| CA-TBL (preset) − dual-band LoRa fusion | S | 0.070 [0.058, 0.084] | -2.0 [-9.0, 5.0] | 22.96 [20.56, 25.59] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | D0 | 0.051 [0.041, 0.062] | -3.0 [-4.9, -1.1] | -392.47 [-400.05, -384.64] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | R1 | 0.070 [0.054, 0.088] | -9.8 [-22.5, 2.5] | -381.64 [-387.51, -375.36] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | R0 | 0.056 [0.042, 0.072] | -1.4 [-9.2, 5.1] | -364.70 [-372.64, -356.32] |
| CA-TBL (preset) − Poucet (LoRa + UWB fused) | S | 0.087 [0.073, 0.100] | -4.0 [-13.1, 4.8] | -388.86 [-395.98, -381.30] |
| static 2.4 GHz − CA-TBL (preset) | D0 | -0.291 [-0.317, -0.264] | -1.2 [-3.0, 0.7] | -17.17 [-18.90, -15.64] |
| static 2.4 GHz − CA-TBL (preset) | R1 | -0.324 [-0.353, -0.295] | 30.7 [22.6, 39.2] | -33.06 [-36.69, -29.57] |
| static 2.4 GHz − CA-TBL (preset) | R0 | -0.314 [-0.342, -0.285] | 6.4 [-0.7, 13.3] | -47.37 [-51.17, -43.50] |
| static 2.4 GHz − CA-TBL (preset) | S | -0.323 [-0.352, -0.294] | 16.6 [9.2, 23.5] | -25.89 [-28.49, -23.54] |

## Registered predictions

- Q1: HELD
- Q2: FAILED
- Q3: FAILED
- Q4: HELD
- Q5: HELD
- Q6: HELD
- Q7: HELD
- Q8: FAILED
- Q9: FAILED
- Q10: HELD
- Q11: HELD
- Q12: FAILED
- Q13: HELD
- Q14: HELD

Problems: 0 (none)
