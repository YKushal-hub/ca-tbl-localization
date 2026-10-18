# REAL-BEST — out of sample, seeds 21-40 (scripts/real_best_analyse.py)

Sources: f581e9f1.

| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | subject J |
|---|---|---|---|---|---|---|
| A | M | ca_tbl_full | 0.325 | 771.7 (441.6) | 0.240 | 20.70 |
| A | M | fixed_threshold | 0.325 | 702.9 (386.2) | 0.295 | 20.97 |
| A | M | static_868 | 0.282 | 693.1 (606.2) | 0.038 | 8.03 |
| A | M | static_2p4 | 0.012 | 1287.0 (1333.4) | 0.571 | 14.13 |
| A | M | poucet_fusion | 0.324 | 528.9 (307.1) | 0.330 | 467.62 |
| A | S | ca_tbl_full | 0.335 | 514.8 (381.8) | 0.396 | 20.61 |
| A | S | fixed_threshold | 0.330 | 586.1 (381.8) | 0.390 | 20.82 |
| A | S | static_868 | 0.282 | 690.0 (608.4) | 0.043 | 8.05 |
| A | S | static_2p4 | 0.012 | 1212.1 (1155.7) | 0.573 | 14.15 |
| A | S | poucet_fusion | 0.330 | 567.5 (397.1) | 0.364 | 467.38 |
| A | F | ca_tbl_full | 0.338 | 498.1 (381.8) | 0.389 | 20.56 |
| A | F | fixed_threshold | 0.332 | 482.8 (381.8) | 0.356 | 20.94 |
| A | F | static_868 | 0.281 | 683.0 (608.4) | 0.042 | 8.03 |
| A | F | static_2p4 | 0.012 | 1212.1 (1155.7) | 0.636 | 14.16 |
| A | F | poucet_fusion | 0.333 | 635.2 (386.9) | 0.428 | 466.66 |
| B | M | ca_tbl_full | 0.576 | 10.6 (4.5) | 0.415 | 20.60 |
| B | M | fixed_threshold | 0.582 | 8.7 (4.8) | 0.436 | 20.77 |
| B | M | static_868 | 0.428 | 44.3 (34.8) | 0.276 | 8.27 |
| B | M | static_2p4 | 0.256 | 6.7 (5.1) | 0.419 | 5.41 |
| B | M | poucet_fusion | 0.567 | 4.7 (4.0) | 0.528 | 385.82 |
| B | S | ca_tbl_full | 0.578 | 4.2 (4.1) | 0.823 | 18.93 |
| B | S | fixed_threshold | 0.587 | 4.1 (3.9) | 0.876 | 19.60 |
| B | S | static_868 | 0.428 | 47.9 (41.5) | 0.236 | 8.26 |
| B | S | static_2p4 | 0.260 | 5.8 (5.4) | 0.569 | 5.29 |
| B | S | poucet_fusion | 0.566 | 4.4 (3.7) | 0.955 | 390.72 |
| B | F | ca_tbl_full | 0.579 | 4.0 (3.9) | 0.851 | 18.78 |
| B | F | fixed_threshold | 0.589 | 4.1 (4.0) | 0.878 | 20.18 |
| B | F | static_868 | 0.427 | 46.8 (40.1) | 0.252 | 8.24 |
| B | F | static_2p4 | 0.259 | 6.3 (5.4) | 0.559 | 5.29 |
| B | F | poucet_fusion | 0.568 | 4.4 (3.7) | 0.952 | 390.37 |

## Variants, ca_tbl_full

| config | contrast | Δ CEP95 tag 0 | Δ availability |
|---|---|---|---|
| A | S − M | -256.9 [-568.8, -31.4] | 0.010 [-0.003, 0.023] |
| A | F − M | -273.6 [-557.5, -52.4] | 0.013 [-0.001, 0.027] |
| A | F − S | -16.7 [-108.3, 72.2] | 0.003 [-0.008, 0.013] |
| B | S − M | -6.4 [-12.9, -1.4] | 0.002 [-0.007, 0.011] |
| B | F − M | -6.6 [-13.1, -1.5] | 0.003 [-0.004, 0.011] |
| B | F − S | -0.2 [-0.5, 0.1] | 0.001 [-0.002, 0.005] |

## CA-TBL − comparator under F, paired by seed

| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| A | fixed_threshold | 0.006 [-0.000, 0.013] | 15.4 [-15.3, 60.3] | -0.38 [-1.22, 0.60] |
| A | static_868 | 0.057 [0.038, 0.078] | -184.9 [-386.6, 29.2] | 12.53 [11.42, 13.71] |
| A | static_2p4 | 0.326 [0.288, 0.366] | -713.9 [-1108.4, -345.8] | 6.41 [5.81, 7.10] |
| A | poucet_fusion | 0.005 [-0.011, 0.024] | -137.1 [-442.7, 134.8] | -446.09 [-448.11, -443.71] |
| B | fixed_threshold | -0.010 [-0.017, -0.002] | -0.1 [-0.6, 0.5] | -1.40 [-2.46, -0.31] |
| B | static_868 | 0.152 [0.135, 0.168] | -42.8 [-60.0, -28.9] | 10.53 [9.16, 11.96] |
| B | static_2p4 | 0.319 [0.294, 0.344] | -2.3 [-3.8, -1.1] | 13.48 [12.17, 14.86] |
| B | poucet_fusion | 0.011 [0.002, 0.020] | -0.3 [-1.4, 0.5] | -371.59 [-380.34, -361.01] |

## Registered predictions

- P1 F lowers CA-TBL CEP95 against M in B: HOLDS (-6.6 [-13.0, -1.3])
- P2 F costs CA-TBL no availability against M (lower bound > -0.01), A and B: HOLDS (A 0.013 [-0.001, 0.027]; B 0.003 [-0.004, 0.011])
- P3 under F in B: CA-TBL - Poucet CEP95 contains 0, availability above 0: HOLDS (CEP95 -0.3 [-1.4, 0.5]; availability 0.011 [0.002, 0.020])
- P4 under F no comparator significantly more accurate than CA-TBL, A and B: HOLDS (8 of 8 contrasts with 20 paired seeds)
- P5 F lowers CA-TBL CEP95 against S in A: FAILS (-16.7 [-110.4, 71.7])

Problems: 0 (none)
