# REAL-STACK — the realistic world, with and without the consistency stack (scripts/real_stack_analyse.py)

Sources: 8fc91dc9.

| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | subject J |
|---|---|---|---|---|---|---|
| A | M | ca_tbl_full | 0.343 | 594.3 (297.7) | 0.276 | 20.11 |
| A | M | fixed_threshold | 0.334 | 784.3 (385.2) | 0.315 | 20.25 |
| A | M | static_868 | 0.284 | 495.7 (382.3) | 0.059 | 7.98 |
| A | M | static_2p4 | 0.007 | 1159.0 (1062.6) | 0.785 | 13.20 |
| A | M | poucet_fusion | 0.335 | 654.5 (146.4) | 0.363 | 464.41 |
| A | S | ca_tbl_full | 0.333 | 620.8 (293.6) | 0.386 | 19.71 |
| A | S | fixed_threshold | 0.343 | 719.4 (293.6) | 0.382 | 20.62 |
| A | S | static_868 | 0.291 | 488.3 (373.7) | 0.070 | 7.96 |
| A | S | static_2p4 | 0.007 | 1065.0 (1151.5) | 0.872 | 13.22 |
| A | S | poucet_fusion | 0.337 | 684.3 (152.8) | 0.429 | 465.20 |
| B | M | ca_tbl_full | 0.586 | 10.5 (4.8) | 0.460 | 21.65 |
| B | M | fixed_threshold | 0.586 | 9.5 (5.1) | 0.448 | 21.37 |
| B | M | static_868 | 0.403 | 33.8 (26.8) | 0.291 | 8.20 |
| B | M | static_2p4 | 0.253 | 5.8 (4.7) | 0.491 | 5.44 |
| B | M | poucet_fusion | 0.565 | 4.2 (3.5) | 0.540 | 372.65 |
| B | S | ca_tbl_full | 0.593 | 4.4 (4.1) | 0.803 | 21.08 |
| B | S | fixed_threshold | 0.595 | 4.3 (4.0) | 0.797 | 20.68 |
| B | S | static_868 | 0.413 | 31.9 (23.9) | 0.306 | 8.20 |
| B | S | static_2p4 | 0.256 | 7.8 (6.1) | 0.509 | 5.38 |
| B | S | poucet_fusion | 0.570 | 3.7 (3.1) | 0.973 | 373.34 |

## S − M, ca_tbl_full

| config | Δ CEP95 tag 0 | Δ availability |
|---|---|---|
| A | 26.4 [-119.5, 203.5] | -0.011 [-0.027, 0.004] |
| B | -6.1 [-13.1, -0.8] | 0.008 [-0.003, 0.020] |

## CA-TBL − comparator under S, paired by seed

| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| A | fixed_threshold | -0.010 [-0.026, 0.004] | -98.6 [-239.6, 19.2] | -0.91 [-1.87, -0.20] |
| A | static_868 | 0.042 [0.025, 0.059] | 132.5 [-131.9, 474.6] | 11.75 [9.63, 13.93] |
| A | static_2p4 | 0.326 [0.297, 0.356] | -444.2 [-965.9, 94.4] | 6.49 [5.58, 7.56] |
| A | poucet_fusion | -0.004 [-0.030, 0.022] | -63.5 [-510.1, 388.2] | -445.49 [-446.89, -443.99] |
| B | fixed_threshold | -0.002 [-0.011, 0.008] | 0.1 [-0.5, 0.6] | 0.40 [-1.14, 2.00] |
| B | static_868 | 0.181 [0.160, 0.200] | -27.5 [-37.1, -19.6] | 12.88 [11.39, 14.40] |
| B | static_2p4 | 0.337 [0.307, 0.367] | -3.4 [-6.2, -1.2] | 15.70 [14.22, 17.24] |
| B | poucet_fusion | 0.023 [0.009, 0.037] | 0.8 [-0.1, 1.5] | -352.26 [-367.36, -335.88] |

## Registered predictions

- P1 S lowers CA-TBL CEP95 against M in B: HOLDS (-6.1 [-13.4, -0.8])
- P2 S costs CA-TBL no availability (lower bound > -0.01), A and B: FAILS (A -0.011 [-0.028, 0.004]; B 0.008 [-0.003, 0.021])
- P3 under S, CA-TBL availability above Poucet in B: HOLDS (0.023 [0.008, 0.037])
- P4 under S, CA-TBL - Poucet CEP95 in B contains 0: HOLDS (0.8 [-0.1, 1.5])

Problems: 0 (none)
