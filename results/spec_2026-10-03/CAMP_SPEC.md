# CAMP-SPEC — generated tables (scripts/camp_spec_analyse.py)

## A_main

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.333 [0.311, 0.357] | 22.1 / 46.0 | 49.7 | 8 | 0.149 / 0.846 / 0.005 | 14.68 |
| ca_tbl_no_integrity | 0.331 [0.311, 0.354] | 18.4 / 31.8 | 45.5 | 8 | 0.164 / 0.832 / 0.004 | 14.51 |
| fixed_threshold | 0.371 [0.349, 0.397] | 18.8 / 36.3 | 43.8 | 8 | 0.672 / 0.326 / 0.002 | 11.32 |
| static_868 | 0.392 [0.368, 0.417] | 19.0 / 35.8 | 77.7 | 8 | 1.000 / 0.000 / 0.000 | 8.10 |
| static_2p4 | 0.042 [0.032, 0.053] | 14.3 / 39.8 | 38.6 | 8 | 0.000 / 1.000 / 0.000 | 11.80 |
| static_uwb | 0.000 [0.000, 0.000] | 31.0 / 84.2 | 79.6 | 1800 | 0.000 / 0.000 / 1.000 | 446.02 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = 0.3625 [0.1554, 0.5498] over 20 seeds -> BETTER
- ca_tbl_no_integrity: A = 0.0908 [-0.0723, 0.2513] over 20 seeds -> INDISTINGUISHABLE

Base-station batch session: time to first coarse median 165 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 20 of 20 seeds.

## B_end

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.630 [0.606, 0.652] | 5.6 / 9.8 | 14.0 | 2 | 0.003 / 0.878 / 0.119 | 39.11 |
| ca_tbl_no_integrity | 0.654 [0.632, 0.674] | 3.5 / 6.1 | 10.0 | 2 | 0.119 / 0.765 / 0.116 | 38.16 |
| fixed_threshold | 0.637 [0.616, 0.657] | 4.3 / 7.3 | 10.7 | 2 | 0.605 / 0.347 / 0.048 | 22.95 |
| static_868 | 0.587 [0.563, 0.609] | 5.5 / 9.3 | 35.9 | 2 | 1.000 / 0.000 / 0.000 | 9.98 |
| static_2p4 | 0.444 [0.426, 0.463] | 3.4 / 6.0 | 9.4 | 2 | 0.000 / 1.000 / 0.000 | 5.19 |
| static_uwb | 0.000 [0.000, 0.000] | 13.8 / 32.3 | 32.2 | 1800 | 0.000 / 0.000 / 1.000 | 410.70 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = -0.4207 [-0.5003, -0.3359] over 20 seeds -> WORSE
- ca_tbl_no_integrity: A = -0.3876 [-0.4627, -0.3101] over 20 seeds -> WORSE

Base-station batch session: time to first coarse median 53 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 2 of 20 seeds.

## C_runnerup

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.322 [0.300, 0.345] | 21.4 / 48.0 | 48.7 | 6 | 0.157 / 0.839 / 0.004 | 14.83 |
| static_868 | 0.386 [0.362, 0.412] | 19.7 / 40.7 | 79.1 | 5 | 1.000 / 0.000 / 0.000 | 8.15 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = 0.4773 [0.3395, 0.6214] over 20 seeds -> BETTER

Base-station batch session: time to first coarse median 160 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 20 of 20 seeds.

GEO-01 stage 3: preset geometry (A_main) against the runner-up (C_runnerup), ca_tbl_full: availability 0.333 vs 0.322, CEP95 46.0 vs 48.0 m.

## D: the NW-2 routing-weight sweep (endgame, ca_tbl_full)

| weights a/b/g/slack | delivered / originated (subjects) | diverged hops | seeds |
|---|---|---|---|
| a0_b0_g1_s1 | 0.594 | 6804 | 10 |
| a0_b1_g0_s1 | 0.422 | 23910 | 10 |
| a1_b0.01_g1_s1 | 0.589 | 7136 | 10 |
| a1_b0.01_g1_s2 | 0.585 | 6312 | 10 |
| a1_b0_g0_s1 | 0.284 | 28706 | 10 |
| a1_b0_g0_s2 | 0.215 | 31740 | 10 |
| a1_b1_g1_s1 | 0.479 | 20067 | 10 |
| a1_b1_g1_s2 | 0.482 | 20355 | 10 |

Registered criterion (highest delivery ratio; within 0.01 the default wins): **a0_b0_g1_s1** (0.594).

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.147 | 0.884 | 39.9 | 44.0 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | 0.000 | 30.9 | 28.8 | 10 |
| antenna | none | ca_tbl_full | 0.142 | 0.846 | 43.1 | 53.7 | 10 |
| antenna | none | ca_tbl_no_integrity | nan | 0.000 | 30.9 | 28.8 | 10 |
| rb_common | downweight | ca_tbl_full | 0.257 | 0.982 | 33.5 | 41.3 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | 0.000 | 27.5 | 35.5 | 10 |
| rb_common | none | ca_tbl_full | 0.257 | 0.978 | 50.4 | 68.1 | 10 |
| rb_common | none | ca_tbl_no_integrity | nan | 0.000 | 27.5 | 35.5 | 10 |
| rb_single | downweight | ca_tbl_full | 0.257 | 0.982 | 33.5 | 41.2 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | 0.000 | 27.2 | 32.9 | 10 |
| rb_single | none | ca_tbl_full | 0.257 | 0.969 | 39.6 | 64.0 | 10 |
| rb_single | none | ca_tbl_no_integrity | nan | 0.000 | 27.2 | 32.9 | 10 |
| txdrop | downweight | ca_tbl_full | 0.239 | 0.964 | 42.5 | 42.3 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | 0.000 | 34.2 | 29.3 | 10 |
| txdrop | none | ca_tbl_full | 0.234 | 0.958 | 35.9 | 52.2 | 10 |
| txdrop | none | ca_tbl_no_integrity | nan | 0.000 | 34.2 | 29.3 | 10 |

## G: the single-hop confidence baseline (LoRaWAPS-style) against the preset (ca_tbl_full, tag 0)

| configuration | hop depth of the fused set | fix cycles | median error (m) | rho(PCS, error) |
|---|---|---|---|---|
| preset 5 km | 0 | 12653 | 16.5 | -0.112 |
| preset 5 km | 1 | 401 | 9.7 | -0.353 |
| preset endgame | 0 | 4500 | 5.0 | -0.148 |
| preset endgame | 1 | 9819 | 4.3 | -0.113 |
| single-hop 5 km | 0 | 12931 | 15.4 | -0.117 |
| single-hop endgame | 0 | 14327 | 4.5 | -0.119 |

Problems: 0 (none)
