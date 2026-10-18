# MAG — a magnetometer heading reference on the realistic-IMU preset (scripts/mag_analyse.py)

Sources: 4412fac3; without the magnetometer: REAL-IMU at /home/kushal/ns/catbl-work/real_imu_2026-10-18.

R1: 2 of 2 cells reproduce REAL-IMU -> HOLDS

| config | arm | CEP95 tag 0 no mag → mag (m) | Δ CEP95 tag 0 | Δ availability | heading err > 0.3 rad no mag → mag | coverage no mag → mag |
|---|---|---|---|---|---|---|
| A | ca_tbl_full | 865.1 → 586.1 | -279.0 [-657.8, 32.7] | 0.020 [0.005, 0.035] | 10 → 5 | 0.329 → 0.332 |
| A | fixed_threshold | 845.3 → 495.8 | -349.5 [-732.7, -60.0] | 0.012 [-0.007, 0.031] | 10 → 4 | 0.231 → 0.355 |
| A | static_868 | 1476.9 → 505.4 | -971.5 [-1310.5, -657.6] | 0.016 [0.002, 0.033] | 19 → 0 | 0.094 → 0.057 |
| A | static_2p4 | 1312.9 → 990.3 | -322.6 [-610.7, -91.0] | 0.002 [0.000, 0.004] | 15 → 1 | 0.799 → 0.792 |
| A | poucet_fusion | 664.8 → 586.5 | -78.3 [-222.5, 46.0] | 0.017 [0.006, 0.030] | 8 → 3 | 0.325 → 0.486 |
| B | ca_tbl_full | 7.2 → 4.3 | -2.9 [-6.9, 0.6] | 0.009 [-0.005, 0.025] | 0 → 0 | 0.737 → 0.837 |
| B | fixed_threshold | 7.9 → 8.9 | 1.0 [-4.2, 6.2] | 0.006 [-0.014, 0.032] | 0 → 0 | 0.707 → 0.735 |
| B | static_868 | 56.0 → 30.0 | -26.0 [-66.4, 2.8] | 0.011 [-0.004, 0.026] | 1 → 0 | 0.223 → 0.246 |
| B | static_2p4 | 12.7 → 5.7 | -7.0 [-12.2, -2.6] | -0.004 [-0.008, 0.001] | 0 → 0 | 0.589 → 0.773 |
| B | poucet_fusion | 2.6 → 2.6 | -0.0 [-0.2, 0.1] | 0.008 [-0.002, 0.018] | 0 → 0 | 0.940 → 0.953 |

## CA-TBL − comparator with the magnetometer, paired by seed

| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| A | fixed_threshold | 0.003 [-0.006, 0.015] | 90.2 [-53.6, 281.7] | -0.56 [-1.16, -0.05] |
| A | static_868 | 0.057 [0.040, 0.074] | 80.7 [-168.4, 390.0] | 12.04 [10.09, 13.99] |
| A | static_2p4 | 0.338 [0.308, 0.367] | -404.2 [-859.6, 41.1] | 6.89 [5.87, 8.06] |
| A | poucet_fusion | 0.009 [-0.007, 0.026] | -0.4 [-411.2, 427.1] | -444.01 [-446.15, -441.40] |
| B | fixed_threshold | 0.001 [-0.020, 0.015] | -4.6 [-9.3, -0.7] | 0.10 [-0.89, 1.06] |
| B | static_868 | 0.169 [0.151, 0.186] | -25.7 [-33.5, -18.7] | 13.13 [11.59, 14.79] |
| B | static_2p4 | 0.327 [0.299, 0.355] | -1.5 [-5.3, 1.7] | 15.89 [14.37, 17.50] |
| B | poucet_fusion | 0.021 [-0.008, 0.044] | 1.7 [0.4, 3.5] | -352.17 [-368.05, -334.90] |

## Registered predictions

- R1 2 of 2 cells reproduce REAL-IMU: HOLDS
- P1 the magnetometer lowers CA-TBL CEP95 in B: FAILS (-2.9 [-7.0, 0.5])
- P2 fewer CA-TBL heading divergences (> 0.3 rad), A and B pooled: HOLDS (10 -> 5 of 40)
- P3 B, CA-TBL - Poucet CEP95 upper bound below +1.0 m: FAILS (1.7 [0.4, 3.5])

Problems: 0 (none)
