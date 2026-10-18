# ACC-B — --dualBaseRelayTier (scripts/acc_b_analyse.py)

Mean [95 % seed bootstrap], seeds 1-20, paired by seed.

## 5 km (A)

| | availability | within 25 m | CEP50 | CEP95 | J/node | ranges into EKF / cycle | tag-0 relay legs | admitted |
|---|---|---|---|---|---|---|---|---|
| ca_tbl_full off | 0.285 | 0.613 | 24.4 | 57.8 | 16.69 | 2.82 | 113 | 108 |
| ca_tbl_full on | 0.286 | 0.619 | 19.2 | 52.2 | 17.51 | 2.97 | 214 | 204 |
| static_2p4 | 0.031 | 0.683 | 24.5 | 54.9 | 14.21 | 0.57 | 261 | 250 |

on − off: av 0.001 [-0.012, 0.014]; w25 0.006 [-0.034, 0.046]; c50 -5.21 [-9.95, -1.47]; c95 -5.58 [-15.09, 2.55]; J 0.82 [0.36, 1.31]; nr 0.16 [0.02, 0.29]; legs 101.25 [58.45, 156.90]; adm 95.65 [52.15, 149.90]

on − static_2p4: av 0.256 [0.227, 0.287]; w25 -0.064 [-0.122, -0.012]; c50 -5.22 [-23.58, 5.58]; c95 -2.73 [-21.85, 15.61]
; off − static_2p4 within 25 m -0.071 [-0.134, -0.007]

subject median error (m), mean over seeds, subjects 2-5: off [25.3, 33.1, 21.6, 25.5], on [27.9, 23.5, 28.8, 23.1]

## endgame (B)

| | availability | within 25 m | CEP50 | CEP95 | J/node | ranges into EKF / cycle | tag-0 relay legs | admitted |
|---|---|---|---|---|---|---|---|---|
| ca_tbl_full off | 0.562 | 0.968 | 5.7 | 12.0 | 22.97 | 7.53 | 198 | 170 |
| ca_tbl_full on | 0.592 | 0.958 | 5.4 | 12.2 | 23.63 | 8.50 | 894 | 765 |
| static_2p4 | 0.271 | 0.968 | 4.9 | 10.8 | 5.81 | 3.49 | 1202 | 1099 |

on − off: av 0.030 [0.019, 0.043]; w25 -0.010 [-0.032, 0.013]; c50 -0.36 [-0.92, 0.29]; c95 0.21 [-1.76, 2.11]; J 0.65 [-1.35, 2.70]; nr 0.97 [0.73, 1.21]; legs 696.30 [610.55, 785.10]; adm 595.55 [524.70, 672.45]

on − static_2p4: av 0.321 [0.290, 0.351]; w25 -0.010 [-0.030, 0.009]; c50 0.50 [-0.07, 1.13]; c95 1.40 [0.07, 2.69]
; off − static_2p4 within 25 m 0.000 [-0.012, 0.013]

subject median error (m), mean over seeds, subjects 2-5: off [5.8, 5.6, 6.0, 6.1], on [6.5, 6.7, 6.1, 7.8]

P-B5 isolation: static_2p4 files or rows differing on vs off: 0

## Registered predictions

- Q1: HELD
- Q2: HELD
- Q3: FAILED
- Q4: FAILED
- Q5: HELD
- Q6: HELD

Problems: 0 (none)
