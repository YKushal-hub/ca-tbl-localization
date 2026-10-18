# ACC-B2 — --subjectPreciseOnly (scripts/acc_b2_analyse.py)

Mean [95 % seed bootstrap], seeds 1-20, paired by seed.

## 5 km (A)

| | availability | within 25 m | CEP50 | CEP95 | J/node | ranges into EKF / cycle | tag-0 relay legs | admitted |
|---|---|---|---|---|---|---|---|---|
| ca_tbl_full off | 0.285 | 0.613 | 24.4 | 57.8 | 16.69 | 2.82 | 113 | 108 |
| ca_tbl_full on | 0.282 | 0.602 | 20.9 | 59.0 | 17.94 | 2.81 | 116 | 108 |
| static_2p4 | 0.031 | 0.683 | 24.5 | 54.9 | 14.21 | 0.57 | 261 | 250 |

on − off: av -0.003 [-0.014, 0.008]; w25 -0.011 [-0.046, 0.024]; c50 -3.56 [-9.71, 1.55]; c95 1.24 [-6.32, 7.94]; J 1.25 [-0.07, 2.97]; smed 0.74 [-4.90, 5.59]

on − static_2p4: av 0.251 [0.220, 0.287]; w25 -0.082 [-0.136, -0.025]; c50 -3.58 [-22.01, 7.22]; c95 4.08 [-16.19, 21.85]
; off − static_2p4 within 25 m -0.071 [-0.131, -0.009]

subject median error (m), mean over seeds, subjects 2-5: off [25.3, 33.1, 21.6, 25.5], on [26.8, 26.0, 30.4, 25.2]

## endgame (B)

| | availability | within 25 m | CEP50 | CEP95 | J/node | ranges into EKF / cycle | tag-0 relay legs | admitted |
|---|---|---|---|---|---|---|---|---|
| ca_tbl_full off | 0.562 | 0.968 | 5.7 | 12.0 | 22.97 | 7.53 | 198 | 170 |
| ca_tbl_full on | 0.520 | 0.973 | 5.6 | 11.8 | 25.28 | 7.61 | 211 | 178 |
| static_2p4 | 0.271 | 0.968 | 4.9 | 10.8 | 5.81 | 3.49 | 1202 | 1099 |

on − off: av -0.042 [-0.051, -0.034]; w25 0.005 [-0.012, 0.023]; c50 -0.15 [-0.58, 0.26]; c95 -0.18 [-1.04, 0.55]; J 2.30 [0.62, 4.00]; smed 0.74 [0.28, 1.25]

on − static_2p4: av 0.249 [0.224, 0.272]; w25 0.006 [-0.011, 0.020]; c50 0.71 [0.10, 1.36]; c95 1.01 [-0.74, 2.72]
; off − static_2p4 within 25 m 0.000 [-0.012, 0.013]

subject median error (m), mean over seeds, subjects 2-5: off [5.8, 5.6, 6.0, 6.1], on [6.4, 6.2, 6.1, 7.8]

Isolation: static_2p4 files or rows differing on vs off: 0; on cells where F2 never fired: 0

## Registered predictions

- Q1: FAILED
- Q2: FAILED
- Q3: FAILED
- Q4: FAILED
- Q5: HELD
- Q6: HELD

Problems: 0 (none)
