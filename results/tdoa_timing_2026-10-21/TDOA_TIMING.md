# TDOA-TIMING — seeds 341-360 (scripts/tdoa_timing_analyse.py)

Source(s): c6f3b4ad. Intervals: paired seed bootstrap, 10000 resamples, 95 %.

## Geometry A (5 km)

| arm | config | five-node A(25 m) | tag-0 A(25 m) | tag-0 CEP95 (m) | five-node J |
|---|---|---|---|---|---|
| ca_tbl_full | L | 0.868 | 0.969 | 76.1 | 12.91 |
| ca_tbl_full | S0 | 0.869 | 0.969 | 76.2 | 12.92 |
| ca_tbl_full | T0 | 0.853 | 0.968 | 75.4 | 12.90 |
| ca_tbl_full | N | 0.862 | 0.969 | 76.9 | 12.91 |
| static_868 | L | 0.025 | 0.022 | 710.2 | 8.34 |
| static_868 | S0 | 0.018 | 0.018 | 698.6 | 8.32 |
| static_868 | T0 | 0.046 | 0.126 | 645.1 | 8.34 |
| static_868 | N | 0.016 | 0.013 | 740.9 | 8.30 |

| arm | contrast | five-node A(25 m) | tag-0 A(25 m) | tag-0 CEP95 (m) | five-node J | verdict |
|---|---|---|---|---|---|---|
| ca_tbl_full | SYNC (S0 - L) | +0.001 [-0.000, +0.002] | +0.000 [+0.000, +0.000] | +0.1 [-0.0, +0.3] | +0.01 [+0.00, +0.02] | NONE |
| ca_tbl_full | TIMING (T0 - L) | -0.015 [-0.037, -0.001] | -0.002 [-0.011, +0.005] | -0.7 [-2.6, +1.1] | -0.01 [-0.26, +0.32] | NONE |
| ca_tbl_full | TDOA VALUE (L - N) | +0.006 [-0.000, +0.016] | +0.001 [+0.000, +0.002] | -0.8 [-2.2, +0.0] | -0.00 [-0.02, +0.01] | NONE |
| static_868 | SYNC (S0 - L) | -0.007 [-0.014, -0.001] | -0.004 [-0.018, +0.006] | -11.6 [-27.1, +2.0] | -0.02 [-0.05, +0.01] | NONE |
| static_868 | TIMING (T0 - L) | +0.021 [+0.000, +0.047] | +0.104 [+0.007, +0.232] | -65.1 [-156.0, +5.8] | +0.00 [-0.06, +0.06] | MATERIAL |
| static_868 | TDOA VALUE (L - N) | +0.009 [+0.003, +0.016] | +0.009 [-0.002, +0.024] | -30.7 [-75.3, +6.2] | +0.05 [-0.00, +0.09] | SMALL |

## Geometry B (endgame)

| arm | config | five-node A(25 m) | tag-0 A(25 m) | tag-0 CEP95 (m) | five-node J |
|---|---|---|---|---|---|
| ca_tbl_full | L | 0.982 | 0.997 | 2.8 | 17.89 |
| ca_tbl_full | S0 | 0.981 | 0.997 | 2.8 | 17.93 |
| ca_tbl_full | T0 | 0.980 | 0.997 | 2.4 | 18.14 |
| ca_tbl_full | N | 0.981 | 0.997 | 2.7 | 17.85 |
| static_868 | L | 0.250 | 0.653 | 63.6 | 8.55 |
| static_868 | S0 | 0.269 | 0.757 | 54.0 | 8.67 |
| static_868 | T0 | 0.325 | 0.851 | 25.8 | 13.12 |
| static_868 | N | 0.097 | 0.046 | 210.9 | 8.73 |

| arm | contrast | five-node A(25 m) | tag-0 A(25 m) | tag-0 CEP95 (m) | five-node J | verdict |
|---|---|---|---|---|---|---|
| ca_tbl_full | SYNC (S0 - L) | -0.000 [-0.001, +0.000] | +0.000 [+0.000, +0.000] | -0.0 [-0.1, +0.1] | +0.04 [-0.46, +0.55] | NONE |
| ca_tbl_full | TIMING (T0 - L) | -0.001 [-0.004, +0.000] | +0.000 [+0.000, +0.000] | -0.4 [-0.9, -0.1] | +0.25 [-0.24, +0.80] | NONE |
| ca_tbl_full | TDOA VALUE (L - N) | +0.000 [-0.001, +0.002] | +0.000 [+0.000, +0.000] | +0.1 [+0.0, +0.4] | +0.04 [-0.47, +0.53] | NONE |
| static_868 | SYNC (S0 - L) | +0.018 [-0.024, +0.063] | +0.104 [-0.006, +0.211] | -9.6 [-22.9, +4.4] | +0.13 [+0.06, +0.20] | NONE |
| static_868 | TIMING (T0 - L) | +0.074 [+0.032, +0.117] | +0.198 [+0.096, +0.303] | -37.8 [-57.8, -19.9] | +4.57 [+3.69, +5.33] | MATERIAL |
| static_868 | TDOA VALUE (L - N) | +0.153 [+0.106, +0.199] | +0.607 [+0.496, +0.712] | -147.3 [-175.8, -120.4] | -0.18 [-0.66, +0.08] | MATERIAL |

## Registered verdicts (ca_tbl_full, five-node A(25 m))

- SYNC, geometry A: **NONE**
- TIMING, geometry A: **NONE**
- TDOA VALUE, geometry A: **NONE**
- SYNC, geometry B: **NONE**
- TIMING, geometry B: **NONE**
- TDOA VALUE, geometry B: **NONE**
