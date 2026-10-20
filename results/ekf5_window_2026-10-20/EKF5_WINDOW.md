# EKF5-WINDOW — out of sample, seeds 61-80 (scripts/ekf5_window_analyse.py)

Sources: 61326646.

| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | median NEES | subject J |
|---|---|---|---|---|---|---|---|
| A | S | ca_tbl_full | 0.336 | 960.5 (443.8) | 0.391 | 23.84 | 20.81 |
| A | S | fixed_threshold | 0.332 | 948.5 (443.8) | 0.328 | 25.21 | 20.65 |
| A | S | static_868 | 0.295 | 785.3 (606.4) | 0.041 | 202.14 | 8.03 |
| A | S | static_2p4 | 0.012 | 1444.9 (1589.7) | 0.902 | 1.60 | 14.04 |
| A | S | poucet_fusion | 0.346 | 952.1 (517.9) | 0.326 | 186.22 | 465.29 |
| A | W | ca_tbl_full | 0.331 | 932.4 (505.1) | 0.596 | 3.17 | 20.68 |
| A | W | fixed_threshold | 0.335 | 1001.4 (505.1) | 0.588 | 3.68 | 21.28 |
| A | W | static_868 | 0.295 | 790.3 (659.7) | 0.057 | 213.30 | 8.08 |
| A | W | static_2p4 | 0.013 | 1451.7 (1568.0) | 0.880 | 1.94 | 14.02 |
| A | W | poucet_fusion | 0.342 | 640.3 (454.8) | 0.702 | 2.23 | 463.01 |
| B | S | ca_tbl_full | 0.581 | 4.6 (3.7) | 0.888 | 2.21 | 19.86 |
| B | S | fixed_threshold | 0.585 | 4.5 (3.6) | 0.887 | 2.40 | 19.59 |
| B | S | static_868 | 0.414 | 67.5 (34.3) | 0.295 | 17.84 | 8.16 |
| B | S | static_2p4 | 0.255 | 6.4 (5.2) | 0.568 | 6.38 | 5.23 |
| B | S | poucet_fusion | 0.573 | 3.6 (3.4) | 0.974 | 1.49 | 372.81 |
| B | W | ca_tbl_full | 0.588 | 3.1 (2.9) | 0.961 | 2.07 | 21.13 |
| B | W | fixed_threshold | 0.583 | 3.0 (2.9) | 0.962 | 1.61 | 22.25 |
| B | W | static_868 | 0.412 | 42.9 (22.4) | 0.469 | 9.44 | 8.21 |
| B | W | static_2p4 | 0.253 | 6.7 (5.6) | 0.670 | 5.16 | 5.22 |
| B | W | poucet_fusion | 0.560 | 3.0 (2.7) | 0.993 | 1.04 | 372.87 |

## W − S, paired by seed

| config | arm | Δ CEP95 tag 0 (m) | Δ availability | Δ subject J |
|---|---|---|---|---|
| A | ca_tbl_full | -28.1 [-243.9, 135.1] | -0.005 [-0.020, 0.009] | -0.13 [-1.23, 0.74] |
| A | fixed_threshold | 52.8 [-94.3, 232.6] | 0.002 [-0.006, 0.012] | 0.63 [-0.41, 1.97] |
| A | static_868 | 5.0 [-63.8, 70.6] | -0.000 [-0.013, 0.013] | 0.05 [-0.02, 0.12] |
| A | static_2p4 | 6.8 [-51.6, 73.7] | 0.000 [-0.000, 0.001] | -0.02 [-0.06, 0.02] |
| A | poucet_fusion | -311.8 [-658.8, -64.4] | -0.004 [-0.019, 0.012] | -2.27 [-4.41, -0.25] |
| B | ca_tbl_full | -1.5 [-3.3, -0.2] | 0.007 [-0.004, 0.019] | 1.26 [-0.51, 2.97] |
| B | fixed_threshold | -1.5 [-3.1, -0.3] | -0.002 [-0.014, 0.012] | 2.66 [0.85, 4.42] |
| B | static_868 | -24.6 [-52.6, -4.1] | -0.002 [-0.012, 0.009] | 0.05 [-0.01, 0.12] |
| B | static_2p4 | 0.3 [-1.1, 1.6] | -0.002 [-0.006, 0.002] | -0.01 [-0.06, 0.05] |
| B | poucet_fusion | -0.6 [-1.0, -0.1] | -0.013 [-0.027, 0.002] | 0.06 [-4.15, 3.75] |

## CA-TBL − comparator under W, paired by seed

| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| A | fixed_threshold | -0.004 [-0.017, 0.007] | -69.0 [-234.4, 47.1] | -0.60 [-1.99, 0.46] |
| A | static_868 | 0.036 [0.021, 0.053] | 142.0 [-162.4, 499.7] | 12.60 [11.32, 13.90] |
| A | static_2p4 | 0.318 [0.292, 0.344] | -519.4 [-880.2, -132.5] | 6.66 [5.83, 7.52] |
| A | poucet_fusion | -0.011 [-0.027, 0.004] | 292.1 [-16.2, 660.4] | -442.33 [-444.68, -439.76] |
| B | fixed_threshold | 0.005 [-0.006, 0.017] | 0.1 [-0.2, 0.5] | -1.12 [-3.10, 0.78] |
| B | static_868 | 0.176 [0.155, 0.196] | -39.7 [-67.9, -19.0] | 12.91 [10.79, 15.14] |
| B | static_2p4 | 0.335 [0.298, 0.372] | -3.5 [-5.1, -2.1] | 15.90 [13.95, 17.97] |
| B | poucet_fusion | 0.028 [0.014, 0.042] | 0.1 [-0.2, 0.5] | -351.75 [-366.12, -334.59] |

## Registered predictions

- P1 W lowers CA-TBL CEP95 against S in A: FAILS (-28.1 [-243.0, 138.1])
- P2 under W, CA-TBL coverage in A >= 0.90: FAILS (0.596)
- P3 W raises CA-TBL availability against S in B: FAILS (0.007 [-0.004, 0.020])
- P4 W costs CA-TBL no significant availability in A: HOLDS (-0.005 [-0.021, 0.009])
- P5 under W, CA-TBL - Poucet CEP95 not significantly above 0 in A and B: HOLDS (A 292.1 [-14.8, 661.8]; B 0.1 [-0.2, 0.5])

Decision rule: P1, P3 and P4 all hold -> --ekf5Window=60 is put to the owner with the realistic-world preset: NOT MET.

Problems: 0 (none)
