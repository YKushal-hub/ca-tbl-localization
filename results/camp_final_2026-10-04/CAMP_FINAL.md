# CAMP-FINAL — generated tables (scripts/camp_final_analyse.py)

## A_main

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.174 [0.152, 0.197] | 70.9 / 209.6 | 93.5 | 32 | 0.183 / 0.815 / 0.002 | 12.37 |
| ca_tbl_no_integrity | 0.177 [0.154, 0.201] | 76.5 / 217.3 | 93.3 | 32 | 0.230 / 0.767 / 0.003 | 12.61 |
| fixed_threshold | 0.186 [0.163, 0.210] | 69.3 / 179.4 | 83.3 | 32 | 0.517 / 0.482 / 0.001 | 11.04 |
| static_868 | 0.165 [0.143, 0.189] | 289.2 / 559.2 | 204.6 | 34 | 1.000 / 0.000 / 0.000 | 6.10 |
| static_2p4 | 0.031 [0.022, 0.041] | 24.5 / 54.9 | 53.1 | 31 | 0.000 / 1.000 / 0.000 | 14.21 |
| static_uwb | 0.000 [0.000, 0.000] | 32.2 / 88.3 | 81.0 | 1800 | 0.000 / 0.000 / 1.000 | 446.19 |
| poucet_fusion | 0.238 [0.206, 0.271] | 161.8 / 219.4 | 119.9 | 20 | 1.000 / 0.000 / 0.000 | 462.46 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = -0.0013 [-0.0115, 0.0068] over 19 seeds -> INDISTINGUISHABLE
- ca_tbl_no_integrity: A = 0.0017 [-0.0008, 0.0058] over 19 seeds -> INDISTINGUISHABLE

Base-station batch session: time to first coarse median 162 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 20 of 20 seeds.

## B_end

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.423 [0.405, 0.441] | 8.4 / 18.4 | 21.1 | 10 | 0.080 / 0.857 / 0.062 | 24.91 |
| ca_tbl_no_integrity | 0.464 [0.451, 0.479] | 7.9 / 17.5 | 17.2 | 10 | 0.696 / 0.276 / 0.028 | 17.77 |
| fixed_threshold | 0.439 [0.425, 0.455] | 7.7 / 16.6 | 18.9 | 10 | 0.707 / 0.265 / 0.028 | 15.44 |
| static_868 | 0.371 [0.356, 0.386] | 9.1 / 20.6 | 56.4 | 8 | 1.000 / 0.000 / 0.000 | 6.43 |
| static_2p4 | 0.271 [0.261, 0.282] | 4.9 / 10.8 | 16.2 | 18 | 0.000 / 1.000 / 0.000 | 5.81 |
| static_uwb | 0.000 [0.000, 0.000] | 12.8 / 31.6 | 30.2 | 1800 | 0.000 / 0.000 / 1.000 | 418.19 |
| poucet_fusion | 0.506 [0.482, 0.529] | 6.4 / 13.5 | 32.2 | 8 | 1.000 / 0.000 / 0.000 | 416.24 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = 0.0283 [0.0183, 0.0390] over 20 seeds -> BETTER
- ca_tbl_no_integrity: A = 0.0001 [-0.0000, 0.0002] over 20 seeds -> INDISTINGUISHABLE

Base-station batch session: time to first coarse median 34 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 11 of 20 seeds.

## C_n8

| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | tier occupancy 868/2.4/UWB | subject J |
|---|---|---|---|---|---|---|
| ca_tbl_full | 0.281 [0.261, 0.303] | 19.5 / 43.7 | 47.5 | 10 | 0.187 / 0.809 / 0.004 | 12.78 |
| static_868 | 0.319 [0.291, 0.348] | 16.3 / 33.3 | 79.9 | 14 | 1.000 / 0.000 / 0.000 | 6.05 |

PCS-error correlation against the trace control (per seed Spearman on tag 0's fix cycles; A = rho_control - rho_PCS, so > 0 means PCS ranks error better):

- ca_tbl_full: A = 0.0043 [-0.0024, 0.0109] over 20 seeds -> INDISTINGUISHABLE

Base-station batch session: time to first coarse median 107 s, to precise median 1800 s (never = 1800); precision spread UNDEFINED on 20 of 20 seeds.

Sensitivity geometry: the spec's three gateways (A_main) against N8_ring (C_n8), ca_tbl_full: availability 0.174 vs 0.281, CEP95 209.6 vs 43.7 m.

A_main: batch precision spread defined on 0 of 20 seeds.

B_end: batch precision spread defined on 9 of 20 seeds, median 663 s.

A_main: ca_tbl_full UWB-delivered fixes (tag 0 and subjects) 0, on 0 of 20 seeds.

B_end: ca_tbl_full UWB-delivered fixes (tag 0 and subjects) 0, on 0 of 20 seeds.

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.149 | 0.725 | 462.7 | 573.1 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | 0.000 | 62.3 | 65.3 | 10 |
| antenna | none | ca_tbl_full | 0.124 | 0.564 | 375.4 | 663.5 | 10 |
| antenna | none | ca_tbl_no_integrity | nan | 0.000 | 62.3 | 65.3 | 10 |
| rb_common | downweight | ca_tbl_full | 0.307 | 0.900 | 494.0 | 573.0 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | 0.000 | 66.5 | 70.2 | 10 |
| rb_common | none | ca_tbl_full | 0.307 | 0.842 | 534.0 | 658.7 | 10 |
| rb_common | none | ca_tbl_no_integrity | nan | 0.000 | 66.5 | 70.2 | 10 |
| rb_single | downweight | ca_tbl_full | 0.305 | 0.907 | 494.0 | 573.0 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | 0.000 | 71.8 | 72.0 | 10 |
| rb_single | none | ca_tbl_full | 0.299 | 0.833 | 534.1 | 659.0 | 10 |
| rb_single | none | ca_tbl_no_integrity | nan | 0.000 | 71.8 | 72.0 | 10 |
| txdrop | downweight | ca_tbl_full | 0.247 | 0.822 | 538.0 | 573.5 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | 0.000 | 70.9 | 65.1 | 10 |
| txdrop | none | ca_tbl_full | 0.236 | 0.771 | 358.7 | 665.2 | 10 |
| txdrop | none | ca_tbl_no_integrity | nan | 0.000 | 70.9 | 65.1 | 10 |

## G: the single-hop confidence baseline (LoRaWAPS-style) against the preset (ca_tbl_full, tag 0)

| configuration | hop depth of the fused set | fix cycles | median error (m) | rho(PCS, error) |
|---|---|---|---|---|
| preset 5 km | 0 | 5475 | 27.3 | -0.351 |
| preset 5 km | 1 | 1142 | 16.1 | -0.087 |
| preset 5 km | 2 | 7 | 459.6 | 0.204 |
| preset endgame | 0 | 9560 | 6.9 | -0.188 |
| preset endgame | 1 | 2648 | 5.0 | -0.305 |
| preset endgame | 2 | 1898 | 6.2 | -0.330 |
| single-hop 5 km | 0 | 5521 | 24.0 | -0.192 |
| single-hop endgame | 0 | 13909 | 6.7 | -0.323 |

## Airtime (ca_tbl_full; --reportAirtime)

The digest against full pushes, per tier: saving = FULL_PUSH_CF - (DIGEST + ENTRY_PUSH), per seed, so > 0 means the digest exchange took less airtime than pushing the whole table at every digest would have.

| cell | tier | DIGEST + ENTRY_PUSH (s) | full-push counterfactual (s) | saving (s), mean [95 %] | seeds |
|---|---|---|---|---|---|
| A_main | 0 | 80.63 | 50.62 | -30.01 [-35.66, -24.35] | 20 |
| A_main | 1 | 6.06 | 9.64 | 3.58 [2.99, 4.13] | 20 |
| B_end | 0 | 96.76 | 37.98 | -58.78 [-61.28, -56.11] | 20 |
| B_end | 1 | 15.21 | 21.03 | 5.82 [5.29, 6.35] | 20 |

The reverse path against flooding (5 km): A_main's ca_tbl_full against H_flood (the same preset with --reversePath=false), paired by seed. RESPONSE airtime summed over the tiers; per answer is that over the base station's answers delivered.

| quantity | reverse path (A) | flooding (H) | A - H, mean [95 %] | seeds |
|---|---|---|---|---|
| RESPONSE airtime (s) | 73.41 | 58.51 | 14.91 [11.70, 18.13] | 20 |
| RESPONSE airtime per answer delivered (s) | 3.044 | 2.444 | 0.600 [0.233, 0.969] | 20 |

Time-on-air table: results/camp_final_2026-10-04/T_toa/s1/toa_table.csv (180 rows)

Problems: 0 (none)
