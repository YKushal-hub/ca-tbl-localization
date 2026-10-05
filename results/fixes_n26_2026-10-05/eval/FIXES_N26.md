# FIXES-N26 — the four N.26 fixes on the preset (scripts/fixes_n26_analyse.py eval)

Integrity threshold from the calibration: 1992.9.

## 5 km (A_main)

| comparator | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | UWB-delivered fixes (seeds) |
|---|---|---|---|---|---|---|
| CA-TBL (preset) | 0.174 [0.152, 0.197] | 70.9 / 209.6 | 93.5 | 32 | 12.37 | 0 (0) |
| CA-TBL, all four fixes | 0.285 [0.251, 0.320] | 24.4 / 57.8 | 65.1 | 12 | 16.69 | 3 (2) |
| + dual LoRa base only | 0.292 [0.258, 0.326] | 28.9 / 103.2 | 74.7 | 12 | 17.72 | 7 (5) |
| + UWB cross-tier poll only | 0.171 [0.150, 0.192] | 71.1 / 209.5 | 92.4 | 32 | 12.13 | 0 (0) |
| + TDoA innovation gate only | 0.171 [0.150, 0.193] | 24.0 / 70.2 | 64.7 | 32 | 12.30 | 0 (0) |
| + integrity (mahalUseR + fitted chi2) only | 0.177 [0.154, 0.201] | 76.5 / 217.3 | 93.3 | 32 | 12.61 | 0 (0) |
| LoRa dual-band fusion (TDoA) | 0.226 [0.204, 0.250] | 193.1 / 410.9 | 148.3 | 30 | 16.75 |  |
| LoRa 868 RSSI | 0.159 [0.137, 0.183] | 51.7 / 120.4 | 111.3 | 34 | 6.07 |  |
| LoRa 868 RSSI + TDoA | 0.165 [0.143, 0.188] | 289.2 / 559.2 | 204.6 | 34 | 6.10 |  |
| fixed-threshold switching | 0.186 [0.163, 0.211] | 69.3 / 179.4 | 83.3 | 32 | 11.04 |  |
| Poucet (LoRa + UWB fused) | 0.238 [0.207, 0.270] | 161.8 / 219.4 | 119.9 | 20 | 462.46 |  |

Paired by seed, comparator − CA-TBL (preset), mean [95 %]:

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|
| CA-TBL, all four fixes | 0.111 [0.088, 0.135] | -151.8 [-302.3, -31.8] | 4.32 [3.34, 5.28] |
| + dual LoRa base only | 0.118 [0.095, 0.142] | -106.4 [-209.9, -21.9] | 5.35 [3.92, 6.93] |
| + UWB cross-tier poll only | -0.003 [-0.008, 0.002] | -0.1 [-6.1, 5.8] | -0.24 [-0.56, 0.05] |
| + TDoA innovation gate only | -0.003 [-0.010, 0.004] | -139.4 [-284.7, -17.2] | -0.06 [-0.35, 0.21] |
| + integrity (mahalUseR + fitted chi2) only | 0.003 [-0.005, 0.011] | 7.7 [-99.2, 102.8] | 0.24 [-0.38, 0.96] |
| LoRa dual-band fusion (TDoA) | 0.052 [0.033, 0.071] | 201.3 [-45.7, 488.7] | 4.38 [3.23, 5.46] |
| LoRa 868 RSSI | -0.015 [-0.035, 0.004] | -89.2 [-251.5, 47.7] | -6.30 [-7.45, -5.16] |
| LoRa 868 RSSI + TDoA | -0.009 [-0.032, 0.012] | 349.7 [-4.0, 754.0] | -6.26 [-7.43, -5.13] |
| fixed-threshold switching | 0.012 [-0.000, 0.025] | -30.2 [-137.7, 67.7] | -1.33 [-1.74, -0.94] |
| Poucet (LoRa + UWB fused) | 0.064 [0.037, 0.090] | 9.8 [-222.3, 295.3] | 450.09 [448.78, 451.37] |

All four fixes − LoRa dual-band fusion (TDoA), paired: availability 0.059 [0.033, 0.086], CEP95 tag 0 -353.1 [-700.2, -83.0] m, subject J -0.06 [-0.71, 0.65].

All four fixes − Poucet (LoRa + UWB fused), paired: availability 0.047 [0.024, 0.072], CEP95 tag 0 -161.6 [-384.7, -30.9] m, subject J -445.77 [-446.50, -444.99].

Adoption terms (A): availability gain interval 0.111 [0.089, 0.135] (need lower > 0); tag 0 CEP95 mean per-seed change -0.235 (need <= +0.10); subject J 16.69 against 12.37 (need <=).

## endgame (B_end)

| comparator | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | UWB-delivered fixes (seeds) |
|---|---|---|---|---|---|---|
| CA-TBL (preset) | 0.423 [0.405, 0.441] | 8.4 / 18.4 | 21.1 | 10 | 24.91 | 0 (0) |
| CA-TBL, all four fixes | 0.562 [0.538, 0.586] | 5.7 / 12.0 | 18.1 | 8 | 22.97 | 371 (20) |
| + dual LoRa base only | 0.593 [0.562, 0.624] | 7.7 / 16.7 | 19.7 | 8 | 38.74 | 412 (20) |
| + UWB cross-tier poll only | 0.423 [0.406, 0.440] | 8.8 / 17.4 | 20.0 | 10 | 25.43 | 1 (1) |
| + TDoA innovation gate only | 0.426 [0.411, 0.441] | 8.3 / 18.0 | 20.7 | 10 | 24.66 | 0 (0) |
| + integrity (mahalUseR + fitted chi2) only | 0.463 [0.448, 0.479] | 8.0 / 15.8 | 16.4 | 10 | 17.40 | 0 (0) |
| LoRa dual-band fusion (TDoA) | 0.501 [0.479, 0.524] | 6.6 / 15.6 | 35.9 | 9 | 9.45 |  |
| LoRa 868 RSSI | 0.371 [0.356, 0.386] | 64.6 / 106.0 | 83.1 | 8 | 6.35 |  |
| LoRa 868 RSSI + TDoA | 0.371 [0.356, 0.386] | 9.1 / 20.6 | 56.4 | 8 | 6.43 |  |
| fixed-threshold switching | 0.439 [0.425, 0.455] | 7.7 / 16.6 | 18.9 | 10 | 15.44 |  |
| Poucet (LoRa + UWB fused) | 0.506 [0.482, 0.529] | 6.4 / 13.5 | 32.2 | 8 | 416.24 |  |

Paired by seed, comparator − CA-TBL (preset), mean [95 %]:

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|
| CA-TBL, all four fixes | 0.139 [0.121, 0.159] | -6.4 [-10.0, -3.5] | -1.93 [-4.30, 0.22] |
| + dual LoRa base only | 0.170 [0.144, 0.197] | -1.7 [-3.1, -0.5] | 13.83 [10.07, 17.77] |
| + UWB cross-tier poll only | -0.000 [-0.006, 0.006] | -1.1 [-3.9, 1.4] | 0.52 [-1.63, 2.74] |
| + TDoA innovation gate only | 0.003 [-0.003, 0.009] | -0.4 [-1.0, 0.1] | -0.25 [-0.90, 0.31] |
| + integrity (mahalUseR + fitted chi2) only | 0.040 [0.029, 0.052] | -2.6 [-5.6, -0.1] | -7.51 [-10.17, -5.08] |
| LoRa dual-band fusion (TDoA) | 0.079 [0.060, 0.098] | -2.8 [-6.1, 0.1] | -15.45 [-17.87, -13.29] |
| LoRa 868 RSSI | -0.052 [-0.063, -0.039] | 87.6 [56.9, 123.3] | -18.55 [-20.85, -16.50] |
| LoRa 868 RSSI + TDoA | -0.052 [-0.064, -0.039] | 2.2 [0.3, 4.2] | -18.48 [-20.79, -16.38] |
| fixed-threshold switching | 0.017 [0.006, 0.028] | -1.9 [-4.3, 0.1] | -9.47 [-11.68, -7.42] |
| Poucet (LoRa + UWB fused) | 0.083 [0.065, 0.102] | -4.9 [-7.7, -2.4] | 391.34 [384.00, 398.38] |

All four fixes − LoRa dual-band fusion (TDoA), paired: availability 0.061 [0.049, 0.073], CEP95 tag 0 -3.6 [-6.6, -0.9] m, subject J 13.52 [11.97, 15.28].

All four fixes − Poucet (LoRa + UWB fused), paired: availability 0.057 [0.047, 0.066], CEP95 tag 0 -1.5 [-2.9, 0.0] m, subject J -393.27 [-399.79, -386.12].

Adoption terms (B): availability gain interval 0.139 [0.121, 0.159] (need lower > 0); tag 0 CEP95 mean per-seed change -0.293 (need <= +0.10); subject J 22.97 against 24.91 (need <=).

## N8_ring (C_n8)

| comparator | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | UWB-delivered fixes (seeds) |
|---|---|---|---|---|---|---|
| CA-TBL (preset) | 0.281 [0.261, 0.304] | 19.5 / 43.7 | 47.5 | 10 | 12.78 | 0 (0) |
| CA-TBL, all four fixes | 0.414 [0.388, 0.441] | 18.5 / 44.7 | 43.7 | 8 | 15.18 | 2 (1) |
| LoRa dual-band fusion (TDoA) | 0.391 [0.367, 0.417] | 17.5 / 38.1 | 53.6 | 10 | 15.10 |  |
| LoRa 868 RSSI | 0.311 [0.291, 0.332] | 51.7 / 106.8 | 98.9 | 14 | 6.04 |  |
| LoRa 868 RSSI + TDoA | 0.319 [0.290, 0.347] | 16.3 / 33.3 | 79.9 | 14 | 6.05 |  |

Paired by seed, comparator − CA-TBL (preset), mean [95 %]:

| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|
| CA-TBL, all four fixes | 0.133 [0.116, 0.152] | 1.0 [-5.4, 7.9] | 2.41 [1.63, 3.16] |
| LoRa dual-band fusion (TDoA) | 0.110 [0.088, 0.132] | -5.7 [-13.8, 2.1] | 2.32 [1.37, 3.23] |
| LoRa 868 RSSI | 0.030 [0.012, 0.048] | 63.0 [46.0, 81.6] | -6.73 [-7.78, -5.68] |
| LoRa 868 RSSI + TDoA | 0.038 [0.014, 0.062] | -10.4 [-17.6, -3.0] | -6.73 [-7.81, -5.72] |

All four fixes − LoRa dual-band fusion (TDoA), paired: availability 0.023 [0.005, 0.041], CEP95 tag 0 6.6 [-1.1, 15.0] m, subject J 0.09 [-0.51, 0.79].

## Integrity check, fault-free: share of tag 0 fix cycles SUSPECT

- preset: A_main 0.775, B_end 0.969
- all four fixes: ALL_main 0.016, ALL_end 0.005
- integrity fix alone: M_main 0.000, M_end 0.000

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.143 | 0.015 | 25.1 | 58.9 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | nan | nan | nan | 10 |
| rb_common | downweight | ca_tbl_full | 0.375 | 0.031 | 60.3 | 67.2 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | nan | nan | nan | 10 |
| rb_single | downweight | ca_tbl_full | 0.265 | 0.023 | 59.5 | 57.7 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | nan | nan | nan | 10 |
| txdrop | downweight | ca_tbl_full | 0.129 | 0.009 | 61.2 | 59.8 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | nan | nan | nan | 10 |

**Registered adoption rule: DO NOT ADOPT** (A: (True, True, False), B: (True, True, True); terms = availability gain, CEP95 within +10 %, subject J not higher).

Problems: 0 (none)
