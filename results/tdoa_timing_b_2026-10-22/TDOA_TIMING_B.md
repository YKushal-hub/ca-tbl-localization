# TDOA-TIMING-B — seeds 361-380 (scripts/tdoa_timing_b_analyse.py)

Source(s): 91dd6e65. Intervals: paired seed bootstrap, 10000 resamples, 95 %.
Availability in percentage points (pp).

## Geometry A (5 km, 3 gateways)

| arm | config | availability (%) | CEP95 (m) | energy (J) |
|---|---|---|---|---|
| fixed_threshold | L | 62.3 | 9.0 | 13.89 |
| fixed_threshold | S0 | 62.3 | 9.0 | 13.89 |
| fixed_threshold | T0 | 62.2 | 7.7 | 13.94 |
| fixed_threshold | N | 62.4 | 9.1 | 13.87 |
| poucet_fusion | L | 57.7 | 10.9 | 459.60 |
| poucet_fusion | S0 | 58.0 | 10.8 | 459.51 |
| poucet_fusion | T0 | 57.5 | 10.6 | 459.71 |
| poucet_fusion | N | 57.8 | 10.9 | 459.57 |
| lora_fusion | L | 57.8 | 11.0 | 14.06 |
| lora_fusion | S0 | 57.8 | 10.8 | 14.05 |
| lora_fusion | T0 | 57.6 | 11.0 | 14.06 |
| lora_fusion | N | 57.9 | 11.0 | 14.03 |
| static_868 | L | 49.7 | 761.5 | 8.30 |
| static_868 | S0 | 49.3 | 761.7 | 8.34 |
| static_868 | T0 | 50.2 | 730.4 | 8.33 |
| static_868 | N | 49.4 | 774.6 | 8.32 |
| static_2p4 | L | 25.6 | 57.8 | 6.68 |
| static_2p4 | S0 | 25.6 | 57.8 | 6.68 |
| static_2p4 | T0 | 25.6 | 57.8 | 6.68 |
| static_2p4 | N | 25.6 | 57.8 | 6.68 |

| arm | contrast | Δ availability (pp) | Δ CEP95 (m) | Δ energy (J) | verdict |
|---|---|---|---|---|---|
| fixed_threshold | SYNC (S0 - L) | +0.0 [+0.0, +0.0] | +0.0 [+0.0, +0.0] | +0.00 [+0.00, +0.00] | NONE |
| fixed_threshold | TIMING (T0 - L) | -0.1 [-0.6, +0.3] | -1.3 [-3.8, +0.3] | +0.05 [-0.16, +0.38] | NONE |
| fixed_threshold | TDOA VALUE (L - N) | -0.1 [-0.2, +0.0] | -0.1 [-0.3, +0.1] | +0.02 [+0.00, +0.05] | NONE |
| poucet_fusion | SYNC (S0 - L) | +0.3 [-0.0, +0.9] | -0.1 [-0.2, +0.0] | -0.09 [-0.29, +0.01] | NONE |
| poucet_fusion | TIMING (T0 - L) | -0.2 [-0.8, +0.3] | -0.3 [-1.3, +0.4] | +0.10 [-0.23, +0.51] | NONE |
| poucet_fusion | TDOA VALUE (L - N) | -0.1 [-0.3, +0.0] | -0.1 [-0.2, +0.0] | +0.03 [+0.00, +0.07] | NONE |
| lora_fusion | SYNC (S0 - L) | +0.0 [-0.1, +0.1] | -0.2 [-0.5, +0.0] | -0.01 [-0.03, +0.01] | NONE |
| lora_fusion | TIMING (T0 - L) | -0.2 [-0.9, +0.5] | +0.0 [-1.3, +1.3] | +0.01 [-0.08, +0.12] | NONE |
| lora_fusion | TDOA VALUE (L - N) | -0.0 [-0.1, +0.0] | -0.1 [-0.2, +0.0] | +0.02 [+0.00, +0.06] | NONE |
| static_868 | SYNC (S0 - L) | -0.4 [-0.9, +0.2] | +0.2 [-12.5, +12.4] | +0.04 [-0.00, +0.09] | NONE |
| static_868 | TIMING (T0 - L) | +0.5 [-0.6, +1.7] | -31.1 [-60.2, +0.5] | +0.03 [-0.03, +0.10] | NONE |
| static_868 | TDOA VALUE (L - N) | +0.3 [-0.3, +1.0] | -13.0 [-48.6, +19.1] | -0.03 [-0.10, +0.05] | NONE |
| static_2p4 | SYNC (S0 - L) | +0.0 [+0.0, +0.0] | +0.0 [+0.0, +0.0] | +0.00 [+0.00, +0.00] | NONE |
| static_2p4 | TIMING (T0 - L) | +0.0 [+0.0, +0.0] | +0.0 [+0.0, +0.0] | +0.00 [+0.00, +0.00] | NONE |
| static_2p4 | TDOA VALUE (L - N) | +0.0 [+0.0, +0.0] | +0.0 [+0.0, +0.0] | +0.00 [+0.00, +0.00] | NONE |

## Geometry B (endgame)

| arm | config | availability (%) | CEP95 (m) | energy (J) |
|---|---|---|---|---|
| fixed_threshold | L | 69.9 | 2.7 | 17.13 |
| fixed_threshold | S0 | 69.5 | 2.8 | 17.42 |
| fixed_threshold | T0 | 70.1 | 2.5 | 17.62 |
| fixed_threshold | N | 69.8 | 2.8 | 17.09 |
| poucet_fusion | L | 67.6 | 2.5 | 413.02 |
| poucet_fusion | S0 | 67.6 | 2.5 | 412.31 |
| poucet_fusion | T0 | 67.7 | 2.3 | 412.26 |
| poucet_fusion | N | 67.1 | 2.4 | 410.72 |
| lora_fusion | L | 66.8 | 2.4 | 13.82 |
| lora_fusion | S0 | 66.4 | 2.4 | 13.88 |
| lora_fusion | T0 | 66.3 | 2.4 | 14.44 |
| lora_fusion | N | 66.8 | 2.4 | 13.88 |
| static_868 | L | 52.8 | 63.8 | 8.83 |
| static_868 | S0 | 52.5 | 45.0 | 8.88 |
| static_868 | T0 | 52.2 | 39.9 | 13.25 |
| static_868 | N | 52.9 | 224.1 | 8.79 |
| static_2p4 | L | 49.1 | 3.7 | 6.80 |
| static_2p4 | S0 | 49.1 | 3.7 | 6.80 |
| static_2p4 | T0 | 49.1 | 3.7 | 6.80 |
| static_2p4 | N | 49.1 | 3.7 | 6.80 |

| arm | contrast | Δ availability (pp) | Δ CEP95 (m) | Δ energy (J) | verdict |
|---|---|---|---|---|---|
| fixed_threshold | SYNC (S0 - L) | -0.4 [-1.4, +0.5] | +0.1 [-0.1, +0.3] | +0.29 [-0.02, +0.65] | NONE |
| fixed_threshold | TIMING (T0 - L) | +0.2 [-0.7, +1.1] | -0.2 [-0.4, +0.0] | +0.49 [-0.08, +1.05] | NONE |
| fixed_threshold | TDOA VALUE (L - N) | +0.1 [-0.6, +0.7] | -0.0 [-0.2, +0.1] | +0.04 [-0.34, +0.40] | NONE |
| poucet_fusion | SYNC (S0 - L) | +0.0 [-0.6, +0.6] | -0.0 [-0.2, +0.1] | -0.71 [-1.66, +0.10] | NONE |
| poucet_fusion | TIMING (T0 - L) | +0.1 [-1.0, +1.3] | -0.2 [-0.4, -0.1] | -0.76 [-2.65, +0.97] | NONE |
| poucet_fusion | TDOA VALUE (L - N) | +0.5 [-0.4, +1.4] | +0.1 [-0.0, +0.4] | +2.30 [+0.20, +5.31] | NONE |
| lora_fusion | SYNC (S0 - L) | -0.4 [-1.0, +0.2] | +0.0 [-0.0, +0.1] | +0.06 [-0.09, +0.28] | NONE |
| lora_fusion | TIMING (T0 - L) | -0.5 [-1.8, +1.0] | +0.0 [-0.3, +0.4] | +0.62 [+0.36, +0.97] | NONE |
| lora_fusion | TDOA VALUE (L - N) | -0.0 [-0.6, +0.6] | -0.0 [-0.1, +0.0] | -0.06 [-0.28, +0.09] | NONE |
| static_868 | SYNC (S0 - L) | -0.3 [-2.0, +1.5] | -18.8 [-33.9, -4.0] | +0.06 [-0.43, +0.51] | NONE |
| static_868 | TIMING (T0 - L) | -0.6 [-1.8, +0.4] | -23.9 [-48.6, +6.7] | +4.43 [+3.43, +5.27] | NONE |
| static_868 | TDOA VALUE (L - N) | -0.1 [-2.0, +1.8] | -160.2 [-189.6, -129.7] | +0.04 [-0.63, +0.62] | NONE |
| static_2p4 | SYNC (S0 - L) | +0.0 [+0.0, +0.0] | +0.0 [+0.0, +0.0] | +0.00 [+0.00, +0.00] | NONE |
| static_2p4 | TIMING (T0 - L) | +0.0 [+0.0, +0.0] | +0.0 [+0.0, +0.0] | +0.00 [+0.00, +0.00] | NONE |
| static_2p4 | TDOA VALUE (L - N) | +0.0 [+0.0, +0.0] | +0.0 [+0.0, +0.0] | +0.00 [+0.00, +0.00] | NONE |

## Geometry C (8 gateways (ring))

| arm | config | availability (%) | CEP95 (m) | energy (J) |
|---|---|---|---|---|
| ca_tbl_full | L | 72.8 | 6.2 | 12.79 |
| ca_tbl_full | S0 | 72.5 | 6.0 | 12.70 |
| ca_tbl_full | T0 | 72.1 | 4.4 | 13.91 |
| ca_tbl_full | N | 72.5 | 5.6 | 12.97 |
| static_868 | L | 60.5 | 73.1 | 8.27 |
| static_868 | S0 | 59.9 | 71.5 | 8.49 |
| static_868 | T0 | 58.4 | 40.5 | 13.33 |
| static_868 | N | 60.3 | 581.9 | 8.27 |
| lora_fusion | L | 72.0 | 6.3 | 12.63 |
| lora_fusion | S0 | 71.8 | 6.2 | 12.72 |
| lora_fusion | T0 | 71.1 | 4.5 | 14.59 |
| lora_fusion | N | 71.7 | 6.5 | 12.79 |

| arm | contrast | Δ availability (pp) | Δ CEP95 (m) | Δ energy (J) | verdict |
|---|---|---|---|---|---|
| ca_tbl_full | SYNC (S0 - L) | -0.3 [-0.7, +0.1] | -0.3 [-0.9, +0.2] | -0.09 [-0.67, +0.39] | NONE |
| ca_tbl_full | TIMING (T0 - L) | -0.7 [-2.4, +1.0] | -1.9 [-3.1, -0.6] | +1.11 [+0.47, +1.66] | NONE |
| ca_tbl_full | TDOA VALUE (L - N) | +0.3 [-0.6, +1.3] | +0.6 [-0.1, +1.5] | -0.18 [-0.75, +0.25] | NONE |
| static_868 | SYNC (S0 - L) | -0.7 [-2.0, +0.7] | -1.6 [-3.6, +0.7] | +0.22 [-0.02, +0.61] | NONE |
| static_868 | TIMING (T0 - L) | -2.1 [-3.4, -0.9] | -32.6 [-96.6, +15.1] | +5.06 [+4.30, +5.69] | ADVERSE |
| static_868 | TDOA VALUE (L - N) | +0.2 [-1.2, +1.9] | -508.8 [-616.6, -396.5] | +0.00 [-0.05, +0.05] | NONE |
| lora_fusion | SYNC (S0 - L) | -0.2 [-0.7, +0.2] | -0.2 [-0.5, +0.1] | +0.09 [-0.68, +0.86] | NONE |
| lora_fusion | TIMING (T0 - L) | -0.8 [-2.1, +0.5] | -1.8 [-2.7, -1.0] | +1.96 [+0.94, +3.03] | NONE |
| lora_fusion | TDOA VALUE (L - N) | +0.3 [-0.6, +1.3] | -0.2 [-0.5, +0.1] | -0.16 [-0.96, +0.57] | NONE |

## Registered verdicts (availability)

| geometry | arm | SYNC | TIMING | TDOA VALUE |
|---|---|---|---|---|
| A | fixed_threshold | NONE | NONE | NONE |
| A | poucet_fusion | NONE | NONE | NONE |
| A | lora_fusion | NONE | NONE | NONE |
| A | static_868 | NONE | NONE | NONE |
| A | static_2p4 | NONE | NONE | NONE |
| B | fixed_threshold | NONE | NONE | NONE |
| B | poucet_fusion | NONE | NONE | NONE |
| B | lora_fusion | NONE | NONE | NONE |
| B | static_868 | NONE | NONE | NONE |
| B | static_2p4 | NONE | NONE | NONE |
| C | ca_tbl_full | NONE | NONE | NONE |
| C | static_868 | NONE | ADVERSE | NONE |
| C | lora_fusion | NONE | NONE | NONE |
