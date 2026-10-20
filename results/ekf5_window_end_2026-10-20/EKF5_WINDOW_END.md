# EKF5-WINDOW-END — the endgame, out of sample, seeds 81-100 (scripts/ekf5_window_end_analyse.py)

Sources: d3e9b581.

| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | median NEES | subject J |
|---|---|---|---|---|---|---|---|
| B | S | ca_tbl_full | 0.558 | 3.9 (3.9) | 0.879 | 2.14 | 20.32 |
| B | S | fixed_threshold | 0.559 | 3.7 (3.8) | 0.853 | 2.16 | 20.62 |
| B | S | static_868 | 0.400 | 87.3 (30.3) | 0.208 | 31.67 | 8.23 |
| B | S | static_2p4 | 0.264 | 6.4 (5.8) | 0.597 | 5.71 | 5.49 |
| B | S | poucet_fusion | 0.536 | 3.3 (3.0) | 0.979 | 1.46 | 389.75 |
| B | W | ca_tbl_full | 0.558 | 3.1 (3.1) | 0.965 | 1.77 | 19.53 |
| B | W | fixed_threshold | 0.561 | 3.1 (3.1) | 0.958 | 1.68 | 20.24 |
| B | W | static_868 | 0.392 | 58.0 (30.8) | 0.411 | 13.83 | 8.31 |
| B | W | static_2p4 | 0.262 | 6.2 (6.1) | 0.661 | 5.01 | 5.42 |
| B | W | poucet_fusion | 0.528 | 3.0 (2.8) | 0.984 | 1.04 | 389.59 |

## W − S, paired by seed

| config | arm | Δ CEP95 tag 0 (m) | Δ availability | Δ subject J |
|---|---|---|---|---|
| B | ca_tbl_full | -0.8 [-1.4, -0.2] | 0.000 [-0.010, 0.010] | -0.78 [-2.11, 0.54] |
| B | fixed_threshold | -0.6 [-1.1, -0.1] | 0.003 [-0.008, 0.013] | -0.38 [-1.83, 1.09] |
| B | static_868 | -29.4 [-76.1, -1.5] | -0.007 [-0.018, 0.003] | 0.07 [-0.01, 0.18] |
| B | static_2p4 | -0.3 [-1.9, 1.1] | -0.002 [-0.007, 0.003] | -0.07 [-0.14, -0.01] |
| B | poucet_fusion | -0.3 [-0.7, -0.0] | -0.008 [-0.019, 0.002] | -0.16 [-3.73, 3.35] |

## CA-TBL − comparator under W, paired by seed

| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| B | fixed_threshold | -0.003 [-0.015, 0.007] | -0.0 [-0.3, 0.2] | -0.70 [-1.87, 0.45] |
| B | static_868 | 0.165 [0.139, 0.190] | -54.9 [-86.4, -28.0] | 11.23 [10.10, 12.45] |
| B | static_2p4 | 0.296 [0.263, 0.327] | -3.1 [-4.3, -2.0] | 14.12 [13.02, 15.31] |
| B | poucet_fusion | 0.030 [0.014, 0.044] | 0.1 [-0.3, 0.5] | -370.06 [-378.25, -361.19] |

## Registered predictions

- P1 W lowers CA-TBL CEP95 against S: HOLDS (-0.8 [-1.4, -0.2])
- P2 under W, CA-TBL coverage >= 0.93: HOLDS (0.965)
- P3 W costs CA-TBL no availability (lower bound > -0.01): FAILS (0.000 [-0.010, 0.010])
- P4 under W, CA-TBL ties Poucet's CEP95 and beats its availability: HOLDS (CEP95 0.1 [-0.3, 0.5]; availability 0.030 [0.015, 0.044])
- P5 W lowers fixed_threshold's CEP95 against S: HOLDS (-0.6 [-1.1, -0.1])

Decision rule: P1 and P3 hold -> --ekf5Window=60 is put to the owner for the endgame with the realistic preset: NOT MET.

Problems: 0 (none)
