# D8-MAIN — tier 2's committed table against the SX1280 812.5 kHz row (scripts/d8_main_analyse.py)

Sources: 52b8ebac.

## R1 — off cells reproduce FIXES-N26's ALL cells (ca_tbl_full row)

identical 60, differing 0, missing 0 -> R1 HOLDS

## A — 5 km

| arm | availability off → on | Δ availability | Δ CEP95 tag 0 (m) | Δ CEP95 subjects (m) | Δ TTFF (s) | Δ subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.285 → 0.326 | 0.041 [0.033, 0.048] | -7.1 [-17.1, 1.9] | -15.4 [-22.8, -9.6] | -3 [-7, 2] | -3.21 [-3.99, -2.43] |
| static_868 | 0.167 → 0.158 | -0.009 [-0.024, 0.007] | 15.8 [-12.8, 47.3] | 14.6 [1.3, 28.4] | 28 [1, 68] | -0.03 [-0.17, 0.08] |
| static_2p4 | 0.031 → 0.074 | 0.043 [0.034, 0.053] | -13.5 [-40.7, 4.3] | -8.5 [-14.1, -3.9] | -42 [-86, -14] | -4.42 [-5.22, -3.69] |
| fixed_threshold | 0.274 → 0.312 | 0.038 [0.025, 0.051] | -7.0 [-13.5, -1.1] | -12.7 [-17.9, -6.8] | -3 [-7, 2] | -3.72 [-4.90, -2.70] |
| poucet_fusion | 0.239 → 0.245 | 0.006 [-0.012, 0.023] | 7.4 [-34.1, 69.6] | -20.3 [-40.3, -1.1] | 23 [-141, 202] | -3.35 [-4.28, -2.25] |
| lora_fusion | 0.226 → 0.238 | 0.012 [-0.005, 0.028] | -4.3 [-39.4, 39.5] | -14.3 [-25.5, -3.1] | 59 [-21, 170] | -3.90 [-4.59, -3.20] |

UWB-delivered fixes, ca_tbl_full: off 3 (2 seeds), on 14 (6 seeds).

Comparison-table contrasts, paired by seed, CA-TBL − comparator:

| contrast | condition | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| CA-TBL − lora_fusion | off | 0.060 [0.037, 0.081] | -36.5 [-63.2, -13.6] | -0.12 [-0.72, 0.55] |
| CA-TBL − lora_fusion | on | 0.088 [0.060, 0.116] | -39.3 [-78.4, -10.3] | 0.57 [0.13, 1.03] |
| CA-TBL − poucet_fusion | off | 0.046 [0.022, 0.070] | -38.3 [-59.4, -20.0] | -445.04 [-446.12, -443.84] |
| CA-TBL − poucet_fusion | on | 0.081 [0.056, 0.105] | -52.8 [-116.6, -7.0] | -444.91 [-445.80, -443.95] |

## B — endgame

| arm | availability off → on | Δ availability | Δ CEP95 tag 0 (m) | Δ CEP95 subjects (m) | Δ TTFF (s) | Δ subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.562 → 0.584 | 0.022 [0.013, 0.030] | -0.6 [-1.8, 0.6] | 2.5 [-4.0, 12.1] | -1 [-2, 0] | 1.08 [-1.36, 3.48] |
| static_868 | 0.371 → 0.379 | 0.008 [-0.005, 0.020] | 0.6 [-1.0, 2.3] | -1.1 [-13.5, 11.0] | 0 [0, 1] | 0.03 [-0.02, 0.07] |
| static_2p4 | 0.271 → 0.331 | 0.059 [0.054, 0.065] | -1.4 [-2.4, -0.3] | -2.8 [-4.7, -0.9] | -4 [-7, -0] | 0.07 [-0.09, 0.22] |
| fixed_threshold | 0.545 → 0.570 | 0.024 [0.016, 0.032] | -0.8 [-1.9, 0.3] | -0.7 [-3.0, 1.9] | -1 [-2, 0] | -1.60 [-2.90, -0.24] |
| poucet_fusion | 0.511 → 0.525 | 0.014 [-0.002, 0.028] | -2.7 [-4.5, -1.2] | -9.0 [-17.8, -1.7] | -1 [-2, 0] | -1.62 [-3.91, 0.78] |
| lora_fusion | 0.496 → 0.527 | 0.031 [0.016, 0.045] | -2.2 [-4.0, -0.5] | -5.4 [-11.9, 0.1] | -0 [-1, 0] | -0.09 [-0.30, 0.12] |

UWB-delivered fixes, ca_tbl_full: off 371 (20 seeds), on 377 (20 seeds).

Comparison-table contrasts, paired by seed, CA-TBL − comparator:

| contrast | condition | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| CA-TBL − lora_fusion | off | 0.066 [0.055, 0.077] | -2.5 [-4.9, -0.2] | 13.53 [12.02, 15.25] |
| CA-TBL − lora_fusion | on | 0.057 [0.045, 0.070] | -0.9 [-2.3, 0.6] | 14.70 [12.36, 17.41] |
| CA-TBL − poucet_fusion | off | 0.051 [0.041, 0.062] | -3.0 [-4.8, -1.2] | -392.47 [-400.00, -384.42] |
| CA-TBL − poucet_fusion | on | 0.059 [0.044, 0.074] | -0.9 [-2.0, 0.2] | -389.78 [-398.12, -381.60] |

## C — N8 ring

| arm | availability off → on | Δ availability | Δ CEP95 tag 0 (m) | Δ CEP95 subjects (m) | Δ TTFF (s) | Δ subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.414 → 0.458 | 0.044 [0.030, 0.057] | -7.4 [-13.7, -1.8] | -11.6 [-15.0, -8.4] | 0 [-1, 1] | -3.64 [-4.44, -2.79] |
| static_868 | 0.322 → 0.329 | 0.007 [-0.003, 0.018] | 0.0 [-3.5, 3.6] | -5.4 [-17.1, 4.1] | -1 [-2, 0] | -0.02 [-0.09, 0.04] |
| static_2p4 | 0.036 → 0.090 | 0.055 [0.049, 0.060] | -1.5 [-7.7, 6.3] | -9.0 [-12.4, -5.7] | -84 [-143, -35] | -4.50 [-5.17, -3.85] |
| fixed_threshold | 0.415 → 0.458 | 0.043 [0.031, 0.055] | -12.9 [-23.4, -4.5] | -10.7 [-17.2, -4.8] | 0 [-1, 1] | -3.76 [-4.59, -2.90] |
| poucet_fusion | 0.396 → 0.418 | 0.022 [0.007, 0.036] | -9.4 [-17.0, -1.3] | -25.9 [-40.6, -13.0] | -2 [-4, 0] | -3.77 [-4.55, -2.95] |
| lora_fusion | 0.393 → 0.421 | 0.028 [0.014, 0.042] | -11.9 [-20.2, -5.7] | -19.0 [-29.5, -9.9] | -1 [-4, 1] | -3.85 [-4.53, -3.16] |

UWB-delivered fixes, ca_tbl_full: off 2 (1 seeds), on 2 (2 seeds).

Comparison-table contrasts, paired by seed, CA-TBL − comparator:

| contrast | condition | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|
| CA-TBL − lora_fusion | off | 0.021 [0.005, 0.038] | -3.7 [-11.8, 4.0] | 0.14 [-0.46, 0.80] |
| CA-TBL − lora_fusion | on | 0.037 [0.016, 0.058] | 0.8 [-7.3, 6.6] | 0.36 [-0.06, 0.79] |
| CA-TBL − poucet_fusion | off | 0.018 [0.002, 0.035] | -6.1 [-12.9, -0.2] | -444.54 [-445.55, -443.49] |
| CA-TBL − poucet_fusion | on | 0.040 [0.016, 0.064] | -4.1 [-13.7, 3.2] | -444.40 [-445.39, -443.31] |

## Registered predictions

- R1 off cells reproduce FIXES-N26 ALL: HOLDS
- P1 static_2p4 availability rises in A, B, C: HOLDS
- P2 lora_fusion availability rises in A, B: FAILS
- P3 ca_tbl_full availability rises in A: HOLDS

Problems: 0 (none)
