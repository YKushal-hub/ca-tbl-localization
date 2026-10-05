# LORA-CMP-2 — CA-TBL against LoRa localisation on both LoRa bands (scripts/lora_cmp2_analyse.py)

## 5 km (A_main)

| comparator | source | seeds | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | J per subject fix |
|---|---|---|---|---|---|---|---|---|
| CA-TBL | A_main/ca_tbl_full | 20 | 0.174 [0.152, 0.197] | 70.9 / 209.6 | 93.5 | 32 | 12.37 | 0.1116 |
| LoRa 868 RSSI | R_main/static_868 | 20 | 0.159 [0.137, 0.183] | 51.7 / 120.4 | 111.3 | 34 | 6.07 | 0.0595 |
| LoRa 868 RSSI + gateway TDoA | A_main/static_868 | 20 | 0.165 [0.144, 0.189] | 289.2 / 559.2 | 204.6 | 34 | 6.10 | 0.0566 |
| LoRa 2.4 GHz ToF | A_main/static_2p4 | 20 | 0.031 [0.022, 0.041] | 24.5 / 54.9 | 53.1 | 31 | 14.21 | 2.0937 |
| LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF | F_main/lora_fusion | 20 | 0.226 [0.204, 0.249] | 193.1 / 410.9 | 148.3 | 30 | 16.75 | 0.1088 |
| LoRa dual-band fusion, 868 RSSI + 2.4 ToF | FR_main/lora_fusion | 20 | 0.228 [0.199, 0.258] | 42.4 / 106.7 | 85.4 | 30 | 16.75 | 0.1145 |
| fixed-threshold band switching | A_main/fixed_threshold | 20 | 0.186 [0.163, 0.211] | 69.3 / 179.4 | 83.3 | 32 | 11.04 | 0.0938 |
| Poucet-style LoRa+UWB fusion | A_main/poucet_fusion | 20 | 0.238 [0.206, 0.270] | 161.8 / 219.4 | 119.9 | 20 | 462.46 | 3.0182 |
| CA-TBL single-hop (LoRaWAPS-style) | G_single/ca_tbl_full | 20 | 0.152 [0.129, 0.175] | 53.5 / 111.1 | 78.4 | 51 | 12.27 | 0.1423 |

Paired by seed, comparator − CA-TBL, mean [95 %] (availability > 0: the comparator fixes more often; CEP95 < 0: the comparator's tag 0 error is lower):

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J | seeds |
|---|---|---|---|---|
| LoRa 868 RSSI | -0.015 [-0.035, 0.004] | -89.2 [-250.3, 48.8] | -6.3 [-7.5, -5.1] | 20 |
| LoRa 868 RSSI + gateway TDoA | -0.009 [-0.031, 0.011] | 349.7 [-0.3, 763.2] | -6.3 [-7.4, -5.1] | 20 |
| LoRa 2.4 GHz ToF | -0.143 [-0.164, -0.125] | -154.6 [-295.2, -40.7] | 1.8 [0.6, 3.2] | 20 |
| LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF | 0.052 [0.033, 0.071] | 201.3 [-44.1, 485.6] | 4.4 [3.2, 5.4] | 20 |
| LoRa dual-band fusion, 868 RSSI + 2.4 ToF | 0.054 [0.029, 0.079] | -102.9 [-254.2, 22.8] | 4.4 [3.2, 5.5] | 20 |
| fixed-threshold band switching | 0.012 [-0.000, 0.025] | -30.2 [-138.1, 66.3] | -1.3 [-1.8, -0.9] | 20 |
| Poucet-style LoRa+UWB fusion | 0.064 [0.038, 0.090] | 9.8 [-223.1, 291.1] | 450.1 [448.8, 451.3] | 20 |
| CA-TBL single-hop (LoRaWAPS-style) | -0.022 [-0.036, -0.008] | -98.5 [-245.9, 13.6] | -0.1 [-1.2, 1.6] | 20 |

What UWB adds to Poucet (poucet_fusion − lora_fusion with TDoA, paired, 20 seeds): availability 0.012 [-0.009, 0.035], CEP95 tag 0 -191.5 [-596.9, 171.0] m, subject J 445.7 [445.2, 446.2].

## endgame (B_end)

| comparator | source | seeds | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | J per subject fix |
|---|---|---|---|---|---|---|---|---|
| CA-TBL | B_end/ca_tbl_full | 20 | 0.423 [0.405, 0.441] | 8.4 / 18.4 | 21.1 | 10 | 24.91 | 0.0826 |
| LoRa 868 RSSI | R_end/static_868 | 20 | 0.371 [0.356, 0.386] | 64.6 / 106.0 | 83.1 | 8 | 6.35 | 0.0240 |
| LoRa 868 RSSI + gateway TDoA | B_end/static_868 | 20 | 0.371 [0.356, 0.386] | 9.1 / 20.6 | 56.4 | 8 | 6.43 | 0.0243 |
| LoRa 2.4 GHz ToF | B_end/static_2p4 | 20 | 0.271 [0.261, 0.281] | 4.9 / 10.8 | 16.2 | 18 | 5.81 | 0.0300 |
| LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF | F_end/lora_fusion | 20 | 0.501 [0.480, 0.523] | 6.6 / 15.6 | 35.9 | 9 | 9.45 | 0.0265 |
| LoRa dual-band fusion, 868 RSSI + 2.4 ToF | FR_end/lora_fusion | 20 | 0.500 [0.477, 0.522] | 7.1 / 19.5 | 36.0 | 9 | 9.32 | 0.0262 |
| fixed-threshold band switching | B_end/fixed_threshold | 20 | 0.439 [0.425, 0.456] | 7.7 / 16.6 | 18.9 | 10 | 15.44 | 0.0492 |
| Poucet-style LoRa+UWB fusion | B_end/poucet_fusion | 20 | 0.506 [0.482, 0.529] | 6.4 / 13.5 | 32.2 | 8 | 416.24 | 1.1583 |
| CA-TBL single-hop (LoRaWAPS-style) | G_single_end/ca_tbl_full | 20 | 0.415 [0.398, 0.432] | 8.9 / 19.8 | 24.2 | 10 | 21.16 | 0.0719 |

Paired by seed, comparator − CA-TBL, mean [95 %] (availability > 0: the comparator fixes more often; CEP95 < 0: the comparator's tag 0 error is lower):

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J | seeds |
|---|---|---|---|---|
| LoRa 868 RSSI | -0.052 [-0.064, -0.039] | 87.6 [56.3, 124.2] | -18.6 [-20.9, -16.5] | 20 |
| LoRa 868 RSSI + gateway TDoA | -0.052 [-0.064, -0.039] | 2.2 [0.3, 4.2] | -18.5 [-20.7, -16.4] | 20 |
| LoRa 2.4 GHz ToF | -0.151 [-0.173, -0.130] | -7.6 [-12.0, -3.7] | -19.1 [-21.5, -17.0] | 20 |
| LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF | 0.079 [0.060, 0.099] | -2.8 [-6.2, 0.1] | -15.5 [-17.9, -13.3] | 20 |
| LoRa dual-band fusion, 868 RSSI + 2.4 ToF | 0.077 [0.056, 0.100] | 1.1 [-5.6, 11.1] | -15.6 [-18.0, -13.4] | 20 |
| fixed-threshold band switching | 0.017 [0.006, 0.028] | -1.9 [-4.2, 0.1] | -9.5 [-11.7, -7.5] | 20 |
| Poucet-style LoRa+UWB fusion | 0.083 [0.065, 0.102] | -4.9 [-7.6, -2.4] | 391.3 [383.7, 398.5] | 20 |
| CA-TBL single-hop (LoRaWAPS-style) | -0.008 [-0.018, 0.001] | 1.3 [-0.3, 2.9] | -3.7 [-6.2, -1.3] | 20 |

What UWB adds to Poucet (poucet_fusion − lora_fusion with TDoA, paired, 20 seeds): availability 0.004 [-0.009, 0.017], CEP95 tag 0 -2.1 [-4.6, 0.1] m, subject J 406.8 [399.8, 413.3].

## N8_ring (C_n8)

| comparator | source | seeds | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | J per subject fix |
|---|---|---|---|---|---|---|---|---|
| CA-TBL | C_n8/ca_tbl_full | 20 | 0.281 [0.261, 0.304] | 19.5 / 43.7 | 47.5 | 10 | 12.78 | 0.0650 |
| LoRa 868 RSSI | R_n8/static_868 | 20 | 0.311 [0.290, 0.331] | 51.7 / 106.8 | 98.9 | 14 | 6.04 | 0.0277 |
| LoRa 868 RSSI + gateway TDoA | C_n8/static_868 | 20 | 0.319 [0.290, 0.347] | 16.3 / 33.3 | 79.9 | 14 | 6.05 | 0.0276 |
| LoRa 2.4 GHz ToF | S2_n8/static_2p4 | 20 | 0.036 [0.027, 0.045] | 12.2 / 31.5 | 37.0 | 42 | 12.43 | 0.7908 |
| LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF | F_n8/lora_fusion | 20 | 0.391 [0.367, 0.417] | 17.5 / 38.1 | 53.6 | 10 | 15.10 | 0.0545 |
| LoRa dual-band fusion, 868 RSSI + 2.4 ToF | FR_n8/lora_fusion | 20 | 0.386 [0.360, 0.411] | 26.7 / 68.2 | 63.3 | 10 | 15.08 | 0.0558 |

Paired by seed, comparator − CA-TBL, mean [95 %] (availability > 0: the comparator fixes more often; CEP95 < 0: the comparator's tag 0 error is lower):

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J | seeds |
|---|---|---|---|---|
| LoRa 868 RSSI | 0.030 [0.012, 0.048] | 63.0 [46.4, 81.3] | -6.7 [-7.8, -5.7] | 20 |
| LoRa 868 RSSI + gateway TDoA | 0.038 [0.014, 0.061] | -10.4 [-17.7, -2.9] | -6.7 [-7.8, -5.7] | 20 |
| LoRa 2.4 GHz ToF | -0.246 [-0.269, -0.224] | -12.2 [-21.3, -4.1] | -0.3 [-1.3, 0.6] | 20 |
| LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF | 0.110 [0.087, 0.132] | -5.7 [-13.6, 2.1] | 2.3 [1.4, 3.2] | 20 |
| LoRa dual-band fusion, 868 RSSI + 2.4 ToF | 0.104 [0.083, 0.124] | 24.5 [13.8, 35.9] | 2.3 [1.5, 3.1] | 20 |

Problems: 0 (none)
