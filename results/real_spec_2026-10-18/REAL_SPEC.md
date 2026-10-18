# REAL-SPEC — out of sample, seeds 41-60 (scripts/real_spec_analyse.py)

Sources: 85b7a0da.

| config | variant | arm | availability | CEP95 tag 0 mean (median) | coverage | subject J |
|---|---|---|---|---|---|---|
| A | S | ca_tbl_full | 0.339 | 663.5 (282.7) | 0.376 | 22.47 |
| A | S | fixed_threshold | 0.342 | 665.1 (282.7) | 0.375 | 21.89 |
| A | S | static_868 | 0.280 | 891.0 (820.3) | 0.030 | 8.22 |
| A | S | static_2p4 | 0.009 | 886.4 (886.6) | 0.876 | 15.68 |
| A | S | poucet_fusion | 0.320 | 806.3 (475.1) | 0.404 | 468.26 |
| A | N | ca_tbl_full | 0.343 | 627.8 (277.0) | 0.426 | 21.94 |
| A | N | fixed_threshold | 0.342 | 665.1 (282.7) | 0.375 | 21.89 |
| A | N | static_868 | 0.280 | 891.0 (820.3) | 0.030 | 8.22 |
| A | N | static_2p4 | 0.009 | 886.4 (886.6) | 0.876 | 15.68 |
| A | N | poucet_fusion | 0.320 | 806.3 (475.1) | 0.404 | 468.26 |
| B | S | ca_tbl_full | 0.577 | 3.9 (3.9) | 0.871 | 20.68 |
| B | S | fixed_threshold | 0.576 | 4.1 (3.6) | 0.849 | 22.07 |
| B | S | static_868 | 0.408 | 47.0 (27.8) | 0.293 | 8.20 |
| B | S | static_2p4 | 0.258 | 7.7 (6.3) | 0.516 | 5.54 |
| B | S | poucet_fusion | 0.565 | 3.6 (3.5) | 0.965 | 393.82 |
| B | N | ca_tbl_full | 0.589 | 5.3 (4.6) | 0.742 | 26.90 |
| B | N | fixed_threshold | 0.576 | 4.1 (3.6) | 0.849 | 22.07 |
| B | N | static_868 | 0.408 | 47.0 (27.8) | 0.293 | 8.20 |
| B | N | static_2p4 | 0.258 | 7.7 (6.3) | 0.516 | 5.54 |
| B | N | poucet_fusion | 0.565 | 3.6 (3.5) | 0.965 | 393.82 |

## N − S, ca_tbl_full

| config | Δ CEP95 tag 0 | mean per-seed ratio − 1 | Δ availability |
|---|---|---|---|
| A | -35.8 [-162.2, 72.3] | -0.068 | 0.005 [-0.007, 0.018] |
| B | 1.4 [0.2, 3.0] | +0.415 | 0.012 [0.001, 0.024] |

## CA-TBL − comparator under N, paired by seed

| config | comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| A | fixed_threshold | 0.001 [-0.011, 0.013] | -37.3 [-245.7, 176.7] | 0.05 [-0.54, 0.60] |
| A | static_868 | 0.063 [0.045, 0.082] | -263.2 [-499.2, 15.6] | 13.72 [11.61, 16.11] |
| A | static_2p4 | 0.335 [0.300, 0.369] | -258.6 [-665.1, 178.6] | 6.25 [4.97, 7.87] |
| A | poucet_fusion | 0.023 [0.014, 0.033] | -178.5 [-417.3, 48.0] | -446.32 [-448.02, -444.18] |
| B | fixed_threshold | 0.013 [0.004, 0.023] | 1.3 [0.5, 2.1] | 4.83 [2.31, 7.54] |
| B | static_868 | 0.181 [0.164, 0.197] | -41.7 [-66.2, -22.4] | 18.70 [16.00, 22.08] |
| B | static_2p4 | 0.331 [0.295, 0.370] | -2.3 [-4.6, -0.2] | 21.36 [18.67, 24.70] |
| B | poucet_fusion | 0.024 [0.013, 0.035] | 1.7 [0.4, 3.3] | -366.92 [-374.51, -359.49] |

## Registered predictions

- P1 N raises CA-TBL availability in B, costs none in A: HOLDS (B 0.012 [0.001, 0.024]; A 0.005 [-0.007, 0.018])
- P2 N costs CA-TBL at most +10 % CEP95 (mean per-seed ratio), A and B: FAILS (A -0.068; B +0.415)
- P3 under N no comparator significantly higher availability, A and B: HOLDS (8 of 8 complete)
- P4 under N no comparator significantly more accurate, A and B: FAILS (above 0: B fixed_threshold, B poucet_fusion)
- P5 under N CA-TBL availability above Poucet in B: HOLDS (0.024 [0.013, 0.035])

Problems: 0 (none)
