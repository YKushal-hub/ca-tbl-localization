# LORA-CMP — CA-TBL against outdoor LoRa localisation (scripts/lora_cmp_analyse.py)

## 5 km (A_main)

| comparator | source | seeds | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | J per subject fix |
|---|---|---|---|---|---|---|---|---|
| CA-TBL | A_main/ca_tbl_full | 20 | 0.174 [0.152, 0.197] | 70.9 / 209.6 | 93.5 | 32 | 12.37 | 0.1116 |
| LoRa RSSI multilateration | R_main/static_868 | 20 | 0.159 [0.137, 0.183] | 51.7 / 120.4 | 111.3 | 34 | 6.07 | 0.0595 |
| LoRa RSSI + gateway TDoA | A_main/static_868 | 20 | 0.165 [0.144, 0.189] | 289.2 / 559.2 | 204.6 | 34 | 6.10 | 0.0566 |
| fixed-threshold band switching | A_main/fixed_threshold | 20 | 0.186 [0.162, 0.211] | 69.3 / 179.4 | 83.3 | 32 | 11.04 | 0.0938 |
| Poucet-style LoRa+UWB fusion | A_main/poucet_fusion | 20 | 0.238 [0.206, 0.270] | 161.8 / 219.4 | 119.9 | 20 | 462.46 | 3.0182 |
| CA-TBL single-hop (LoRaWAPS-style) | G_single/ca_tbl_full | 20 | 0.152 [0.128, 0.176] | 53.5 / 111.1 | 78.4 | 51 | 12.27 | 0.1423 |

Paired by seed, comparator − CA-TBL, mean [95 %] (availability > 0: the comparator fixes more often; CEP95 < 0: the comparator's tag 0 error is lower):

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J | seeds |
|---|---|---|---|---|
| LoRa RSSI multilateration | -0.015 [-0.035, 0.004] | -89.2 [-247.7, 48.7] | -6.3 [-7.5, -5.1] | 20 |
| LoRa RSSI + gateway TDoA | -0.009 [-0.032, 0.012] | 349.7 [-8.7, 752.8] | -6.3 [-7.4, -5.1] | 20 |
| fixed-threshold band switching | 0.012 [-0.000, 0.024] | -30.2 [-140.7, 67.1] | -1.3 [-1.7, -0.9] | 20 |
| Poucet-style LoRa+UWB fusion | 0.064 [0.037, 0.090] | 9.8 [-219.7, 293.6] | 450.1 [448.8, 451.3] | 20 |
| CA-TBL single-hop (LoRaWAPS-style) | -0.022 [-0.036, -0.008] | -98.5 [-240.9, 13.8] | -0.1 [-1.2, 1.6] | 20 |

## endgame (B_end)

| comparator | source | seeds | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | J per subject fix |
|---|---|---|---|---|---|---|---|---|
| CA-TBL | B_end/ca_tbl_full | 20 | 0.423 [0.405, 0.441] | 8.4 / 18.4 | 21.1 | 10 | 24.91 | 0.0826 |
| LoRa RSSI multilateration | R_end/static_868 | 20 | 0.371 [0.356, 0.386] | 64.6 / 106.0 | 83.1 | 8 | 6.35 | 0.0240 |
| LoRa RSSI + gateway TDoA | B_end/static_868 | 20 | 0.371 [0.356, 0.386] | 9.1 / 20.6 | 56.4 | 8 | 6.43 | 0.0243 |
| fixed-threshold band switching | B_end/fixed_threshold | 20 | 0.439 [0.425, 0.456] | 7.7 / 16.6 | 18.9 | 10 | 15.44 | 0.0492 |
| Poucet-style LoRa+UWB fusion | B_end/poucet_fusion | 20 | 0.506 [0.482, 0.529] | 6.4 / 13.5 | 32.2 | 8 | 416.24 | 1.1583 |
| CA-TBL single-hop (LoRaWAPS-style) | G_single_end/ca_tbl_full | 20 | 0.415 [0.398, 0.432] | 8.9 / 19.8 | 24.2 | 10 | 21.16 | 0.0719 |

Paired by seed, comparator − CA-TBL, mean [95 %] (availability > 0: the comparator fixes more often; CEP95 < 0: the comparator's tag 0 error is lower):

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J | seeds |
|---|---|---|---|---|
| LoRa RSSI multilateration | -0.052 [-0.063, -0.039] | 87.6 [55.9, 125.0] | -18.6 [-20.9, -16.5] | 20 |
| LoRa RSSI + gateway TDoA | -0.052 [-0.064, -0.039] | 2.2 [0.3, 4.2] | -18.5 [-20.8, -16.4] | 20 |
| fixed-threshold band switching | 0.017 [0.006, 0.027] | -1.9 [-4.2, 0.1] | -9.5 [-11.7, -7.5] | 20 |
| Poucet-style LoRa+UWB fusion | 0.083 [0.065, 0.102] | -4.9 [-7.7, -2.4] | 391.3 [383.9, 398.4] | 20 |
| CA-TBL single-hop (LoRaWAPS-style) | -0.008 [-0.018, 0.001] | 1.3 [-0.3, 2.9] | -3.7 [-6.2, -1.2] | 20 |

## N8_ring (C_n8)

| comparator | source | seeds | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | J per subject fix |
|---|---|---|---|---|---|---|---|---|
| CA-TBL | C_n8/ca_tbl_full | 20 | 0.281 [0.261, 0.303] | 19.5 / 43.7 | 47.5 | 10 | 12.78 | 0.0650 |
| LoRa RSSI multilateration | R_n8/static_868 | 20 | 0.311 [0.290, 0.332] | 51.7 / 106.8 | 98.9 | 14 | 6.04 | 0.0277 |
| LoRa RSSI + gateway TDoA | C_n8/static_868 | 20 | 0.319 [0.291, 0.348] | 16.3 / 33.3 | 79.9 | 14 | 6.05 | 0.0276 |

Paired by seed, comparator − CA-TBL, mean [95 %] (availability > 0: the comparator fixes more often; CEP95 < 0: the comparator's tag 0 error is lower):

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J | seeds |
|---|---|---|---|---|
| LoRa RSSI multilateration | 0.030 [0.012, 0.049] | 63.0 [46.2, 81.6] | -6.7 [-7.8, -5.7] | 20 |
| LoRa RSSI + gateway TDoA | 0.038 [0.014, 0.062] | -10.4 [-17.7, -2.9] | -6.7 [-7.8, -5.7] | 20 |

Problems: 0 (none)
