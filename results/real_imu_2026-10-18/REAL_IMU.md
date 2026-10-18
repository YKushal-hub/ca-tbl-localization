# REAL-IMU — the preset with a realistic IMU (scripts/real_imu_analyse.py)

Sources: 92152dc7.

| config | filter | guard | arm | availability | CEP95 tag 0 (m) | CEP95 after 300 s, fix cycles (m) | coverage | median NEES | subject J |
|---|---|---|---|---|---|---|---|---|---|
| A | cv | g0 | ca_tbl_full | 0.319 | 1545.0 | 1497.7 | 0.394 | 48.50 | 19.82 |
| A | cv | g0 | fixed_threshold | 0.308 | 1518.7 | 1471.1 | 0.399 | 53.88 | 19.88 |
| A | cv | g0 | static_868 | 0.267 | 2005.4 | 1835.1 | 0.211 | 92.13 | 8.52 |
| A | cv | g0 | static_2p4 | 0.004 | 1658.6 | 124.4 | 0.987 | 0.26 | 13.17 |
| A | cv | g0 | poucet_fusion | 0.314 | 1450.2 | 1457.4 | 0.426 | 50.31 | 468.17 |
| A | cv | g1 | ca_tbl_full | 0.322 | 1412.3 | 1402.1 | 0.413 | 48.63 | 19.80 |
| A | cv | g1 | fixed_threshold | 0.322 | 1280.4 | 1233.5 | 0.432 | 39.93 | 19.97 |
| A | cv | g1 | static_868 | 0.266 | 1943.0 | 1782.0 | 0.232 | 87.58 | 8.51 |
| A | cv | g1 | static_2p4 | 0.003 | 1634.4 | 198.8 | 0.993 | 0.20 | 13.19 |
| A | cv | g1 | poucet_fusion | 0.304 | 1436.9 | 1405.4 | 0.447 | 40.23 | 468.31 |
| A | ekf5 | g0 | ca_tbl_full | 0.326 | 865.1 | 822.9 | 0.329 | 570.97 | 20.04 |
| A | ekf5 | g0 | fixed_threshold | 0.330 | 845.3 | 779.7 | 0.231 | 1526.70 | 20.70 |
| A | ekf5 | g0 | static_868 | 0.272 | 1476.9 | 1441.2 | 0.094 | 230.88 | 8.31 |
| A | ekf5 | g0 | static_2p4 | 0.006 | 1312.9 | 127.5 | 0.799 | 0.69 | 13.21 |
| A | ekf5 | g0 | poucet_fusion | 0.319 | 664.8 | 590.0 | 0.325 | 229.11 | 466.32 |
| A | ekf5 | g1 | ca_tbl_full | 0.308 | 960.3 | 908.4 | 0.307 | 737.49 | 20.57 |
| A | ekf5 | g1 | fixed_threshold | 0.319 | 1039.6 | 979.8 | 0.257 | 1335.57 | 20.73 |
| A | ekf5 | g1 | static_868 | 0.273 | 1585.1 | 1511.8 | 0.123 | 307.97 | 8.42 |
| A | ekf5 | g1 | static_2p4 | 0.004 | 1439.8 | 137.2 | 0.880 | 0.37 | 13.20 |
| A | ekf5 | g1 | poucet_fusion | 0.324 | 708.7 | 668.5 | 0.474 | 16.73 | 465.91 |
| B | cv | g0 | ca_tbl_full | 0.554 | 121.8 | 133.8 | 0.620 | 7.39 | 20.60 |
| B | cv | g0 | fixed_threshold | 0.554 | 145.1 | 152.6 | 0.614 | 7.97 | 20.86 |
| B | cv | g0 | static_868 | 0.383 | 364.8 | 373.9 | 0.595 | 12.30 | 10.62 |
| B | cv | g0 | static_2p4 | 0.229 | 94.6 | 80.5 | 0.680 | 6.21 | 5.90 |
| B | cv | g0 | poucet_fusion | 0.503 | 118.4 | 126.3 | 0.625 | 6.73 | 396.85 |
| B | cv | g1 | ca_tbl_full | 0.550 | 149.8 | 162.0 | 0.613 | 6.80 | 21.10 |
| B | cv | g1 | fixed_threshold | 0.547 | 154.0 | 163.4 | 0.620 | 7.30 | 20.58 |
| B | cv | g1 | static_868 | 0.392 | 390.0 | 399.6 | 0.605 | 11.54 | 10.69 |
| B | cv | g1 | static_2p4 | 0.219 | 139.4 | 133.1 | 0.680 | 6.31 | 5.82 |
| B | cv | g1 | poucet_fusion | 0.501 | 117.1 | 131.4 | 0.599 | 7.77 | 397.73 |
| B | ekf5 | g0 | ca_tbl_full | 0.572 | 7.2 | 6.3 | 0.737 | 3.24 | 20.70 |
| B | ekf5 | g0 | fixed_threshold | 0.574 | 7.9 | 7.2 | 0.707 | 3.38 | 21.97 |
| B | ekf5 | g0 | static_868 | 0.400 | 56.0 | 50.7 | 0.223 | 21.12 | 8.36 |
| B | ekf5 | g0 | static_2p4 | 0.257 | 12.7 | 10.2 | 0.589 | 5.14 | 5.46 |
| B | ekf5 | g0 | poucet_fusion | 0.552 | 2.6 | 1.9 | 0.940 | 1.83 | 377.57 |
| B | ekf5 | g1 | ca_tbl_full | 0.576 | 5.3 | 4.2 | 0.767 | 2.93 | 21.87 |
| B | ekf5 | g1 | fixed_threshold | 0.583 | 6.5 | 6.0 | 0.741 | 2.98 | 21.90 |
| B | ekf5 | g1 | static_868 | 0.404 | 117.9 | 114.6 | 0.220 | 25.78 | 8.34 |
| B | ekf5 | g1 | static_2p4 | 0.239 | 15.9 | 15.4 | 0.680 | 3.82 | 5.43 |
| B | ekf5 | g1 | poucet_fusion | 0.563 | 2.6 | 1.9 | 0.953 | 1.78 | 374.30 |

## Paired contrasts, ca_tbl_full tag 0 CEP95

| contrast | config | Δ (m) | mean per-seed ratio − 1 |
|---|---|---|---|
| ekf5 − cv (guard off) | A | -680.0 [-951.5, -382.9] | -0.422 |
| guard on − off (ekf5) | A | 95.3 [-201.4, 432.6] | +9.591 |
| guard on − off (cv) | A | -132.7 [-364.0, 93.4] | -0.041 |
| ekf5 − cv (guard off) | B | -114.7 [-154.3, -79.6] | -0.896 |
| guard on − off (ekf5) | B | -1.9 [-5.4, 1.4] | +0.529 |
| guard on − off (cv) | B | 28.0 [-6.2, 64.1] | +0.247 |

## CA-TBL − comparator under ekf5 with the guard, paired by seed

| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ CEP95 after 300 s (m) | Δ subject J |
|---|---|---|---|---|---|
| A | fixed_threshold | -0.011 [-0.020, -0.003] | -79.2 [-232.0, 71.5] | -71.5 [-220.8, 77.0] | -0.16 [-0.83, 0.43] |
| A | static_868 | 0.036 [0.020, 0.051] | -624.8 [-1060.1, -182.4] | -603.5 [-1025.2, -183.2] | 12.15 [10.29, 14.01] |
| A | static_2p4 | 0.304 [0.275, 0.334] | -479.5 [-1031.6, 100.3] | 545.3 [-116.0, 1332.2] | 7.37 [6.55, 8.21] |
| A | poucet_fusion | -0.016 [-0.024, -0.007] | 251.6 [-109.9, 668.7] | 239.8 [-115.8, 658.9] | -445.34 [-447.01, -443.48] |
| B | fixed_threshold | -0.007 [-0.018, 0.003] | -1.3 [-4.3, 1.5] | -1.8 [-5.1, 1.4] | -0.03 [-1.44, 1.43] |
| B | static_868 | 0.172 [0.148, 0.196] | -112.6 [-212.0, -34.4] | -110.4 [-209.5, -32.7] | 13.53 [12.04, 15.07] |
| B | static_2p4 | 0.336 [0.311, 0.362] | -10.6 [-27.9, 0.9] | -11.2 [-30.6, 0.8] | 16.45 [14.99, 18.00] |
| B | poucet_fusion | 0.013 [-0.017, 0.038] | 2.6 [0.6, 5.2] | 2.3 [0.9, 4.3] | -352.43 [-367.96, -335.73] |

## Registered predictions

- P1 ekf5 lowers CA-TBL CEP95 against cv in A and B: HOLDS (A -680.0 [-951.5, -382.9], B -114.7 [-154.3, -79.6])
- P2 the guard lowers CA-TBL CEP95 under ekf5 in A, costs <= +10 % in B: FAILS (A 95.3 [-201.4, 432.6]; B ratio +0.529)
- P3 CA-TBL availability above every comparator in A and B (ekf5, guard): FAILS
- P4 CA-TBL - Poucet CEP95 below 0 in A, above 0 in B (ekf5, guard): FAILS (A 251.6 [-109.9, 668.7]; B 2.6 [0.6, 5.2])

Problems: 0 (none)
