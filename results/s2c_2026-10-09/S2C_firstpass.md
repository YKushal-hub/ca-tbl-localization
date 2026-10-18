# S2-C

s* = 10. Training NIS > 9.21 share (subjects, ca_tbl_full, seeds 21-30): s = 1: 0.4207, s = 3: 0.3137, s = 10: 0.2078.
Stage C reuse check: 18 of 18 seed-arm pairs byte-identical.

## U_A: unknown start, means per arm (seeds 1-20)

| metric | ca_tbl_full | fixed_threshold | static_2p4 | known start (control) ca_tbl_full / fixed_threshold / static_2p4 |
|---|---|---|---|---|
| first | 418.000 | 389.175 | 1063.275 | 418.525 / 405.250 / 981.275 |
| never | 0.110 | 0.080 | 0.460 | 0.090 / 0.110 / 0.420 |
| av | 0.278 | 0.274 | 0.019 | 0.278 / 0.285 / 0.024 |
| w25 | 0.229 | 0.206 | 0.161 | 0.638 / 0.649 / 0.705 |
| w25acq | 0.274 | 0.241 | 0.308 | 0.643 / 0.649 / 0.775 |
| med | 298.539 | 323.351 | 373.301 | 25.044 / 22.342 / 17.759 |
| c95 | 512.369 | 530.912 | 522.293 | 48.258 / 39.126 / 48.360 |
| rec | 185.357 | 81.429 | 59.167 | 21.500 / 22.688 / 70.438 |
| J | 18.473 | 18.420 | 14.233 | 17.290 / 16.857 / 14.276 |
| nis | 0.446 | nan | nan | nan / nan / nan |
| anees | 2028.706 | 2054.399 | 557.767 | nan / nan / nan |

| paired difference | value |
|---|---|
| ca_tbl_full − static_2p4 (U): first (n 20) | -645.275 [-785.976, -509.321] |
| ca_tbl_full − static_2p4 (U): never (n 20) | -0.350 [-0.470, -0.240] |
| ca_tbl_full − static_2p4 (U): av (n 20) | 0.259 [0.229, 0.290] |
| ca_tbl_full − static_2p4 (U): w25 (n 20) | 0.068 [0.003, 0.135] |
| ca_tbl_full − static_2p4 (U): w25acq (n 20) | -0.034 [-0.155, 0.084] |
| ca_tbl_full − static_2p4 (U): med (n 20) | -74.762 [-158.607, 11.125] |
| ca_tbl_full − static_2p4 (U): c95 (n 20) | -9.924 [-166.414, 150.568] |
| ca_tbl_full − static_2p4 (U): rec (n 20) | 89.286 [-30.536, 228.036] |
| ca_tbl_full − static_2p4 (U): J (n 20) | 4.240 [3.090, 5.449] |
| ca_tbl_full − fixed_threshold@12 (U): first (n 20) | 28.825 [-32.076, 90.150] |
| ca_tbl_full − fixed_threshold@12 (U): never (n 20) | 0.030 [-0.010, 0.070] |
| ca_tbl_full − fixed_threshold@12 (U): av (n 20) | 0.004 [-0.009, 0.018] |
| ca_tbl_full − fixed_threshold@12 (U): w25 (n 20) | 0.023 [-0.016, 0.068] |
| ca_tbl_full − fixed_threshold@12 (U): w25acq (n 20) | 0.032 [-0.018, 0.088] |
| ca_tbl_full − fixed_threshold@12 (U): med (n 20) | -24.812 [-94.042, 25.291] |
| ca_tbl_full − fixed_threshold@12 (U): c95 (n 20) | -18.543 [-45.494, 0.141] |
| ca_tbl_full − fixed_threshold@12 (U): rec (n 20) | 76.667 [-25.836, 234.167] |
| ca_tbl_full − fixed_threshold@12 (U): J (n 20) | 0.052 [-0.883, 1.008] |
| ca_tbl_full U − known start: first (n 20) | -0.525 [-88.901, 72.751] |
| ca_tbl_full U − known start: never (n 20) | 0.020 [-0.030, 0.070] |
| ca_tbl_full U − known start: av (n 20) | 0.000 [-0.034, 0.033] |
| ca_tbl_full U − known start: w25 (n 20) | -0.409 [-0.477, -0.344] |
| ca_tbl_full U − known start: w25acq (n 20) | -0.369 [-0.443, -0.297] |
| ca_tbl_full U − known start: med (n 20) | 273.496 [199.917, 348.171] |
| ca_tbl_full U − known start: c95 (n 20) | 464.111 [292.179, 655.573] |
| ca_tbl_full U − known start: rec (n 20) | 164.196 [48.033, 329.734] |
| ca_tbl_full U − known start: J (n 20) | 1.182 [-0.213, 2.757] |
| static_2p4 U − known start: first (n 20) | 82.000 [-12.551, 170.679] |
| static_2p4 U − known start: never (n 20) | 0.040 [-0.040, 0.120] |
| static_2p4 U − known start: av (n 20) | -0.005 [-0.010, -0.001] |
| static_2p4 U − known start: w25 (n 20) | -0.545 [-0.601, -0.485] |
| static_2p4 U − known start: w25acq (n 20) | -0.467 [-0.593, -0.344] |
| static_2p4 U − known start: med (n 20) | 355.542 [279.523, 431.887] |
| static_2p4 U − known start: c95 (n 20) | 473.933 [329.865, 630.393] |
| static_2p4 U − known start: rec (n 20) | -22.708 [-111.979, 62.083] |
| static_2p4 U − known start: J (n 20) | -0.043 [-0.347, 0.254] |

## U_B: unknown start, means per arm (seeds 1-20)

| metric | ca_tbl_full | fixed_threshold | static_2p4 | known start (control) ca_tbl_full / fixed_threshold / static_2p4 |
|---|---|---|---|---|
| first | 65.275 | 78.175 | 230.100 | 76.075 / 76.600 / 148.750 |
| never | 0.000 | 0.000 | 0.020 | 0.000 / 0.000 / 0.000 |
| av | 0.576 | 0.577 | 0.269 | 0.575 / 0.586 / 0.268 |
| w25 | 0.868 | 0.876 | 0.859 | 0.977 / 0.973 / 0.969 |
| w25acq | 0.893 | 0.910 | 0.913 | 0.984 / 0.975 / 0.965 |
| med | 6.239 | 6.716 | 8.559 | 4.052 / 4.068 / 4.639 |
| c95 | 20.195 | 16.548 | 18.797 | 7.499 / 7.273 / 7.223 |
| rec | 55.461 | 47.375 | 89.853 | 23.062 / 13.812 / 33.750 |
| J | 25.363 | 27.256 | 5.718 | 26.011 / 28.138 / 5.669 |
| nis | 0.327 | nan | nan | nan / nan / nan |
| anees | 45.403 | 95.979 | 138.205 | nan / nan / nan |

| paired difference | value |
|---|---|
| ca_tbl_full − static_2p4 (U): first (n 20) | -164.825 [-242.954, -96.098] |
| ca_tbl_full − static_2p4 (U): never (n 20) | -0.020 [-0.050, 0.000] |
| ca_tbl_full − static_2p4 (U): av (n 20) | 0.308 [0.279, 0.336] |
| ca_tbl_full − static_2p4 (U): w25 (n 20) | 0.009 [-0.029, 0.047] |
| ca_tbl_full − static_2p4 (U): w25acq (n 20) | -0.020 [-0.055, 0.014] |
| ca_tbl_full − static_2p4 (U): med (n 20) | -2.320 [-4.282, -0.817] |
| ca_tbl_full − static_2p4 (U): c95 (n 20) | 1.398 [-10.346, 16.219] |
| ca_tbl_full − static_2p4 (U): rec (n 20) | -42.656 [-129.379, 25.547] |
| ca_tbl_full − static_2p4 (U): J (n 20) | 19.645 [17.376, 21.983] |
| ca_tbl_full − fixed_threshold@12 (U): first (n 20) | -12.900 [-44.626, 14.101] |
| ca_tbl_full − fixed_threshold@12 (U): never (n 20) | 0.000 [0.000, 0.000] |
| ca_tbl_full − fixed_threshold@12 (U): av (n 20) | -0.001 [-0.010, 0.008] |
| ca_tbl_full − fixed_threshold@12 (U): w25 (n 20) | -0.008 [-0.039, 0.020] |
| ca_tbl_full − fixed_threshold@12 (U): w25acq (n 20) | -0.016 [-0.049, 0.014] |
| ca_tbl_full − fixed_threshold@12 (U): med (n 20) | -0.477 [-3.089, 1.775] |
| ca_tbl_full − fixed_threshold@12 (U): c95 (n 20) | 3.646 [-0.469, 8.582] |
| ca_tbl_full − fixed_threshold@12 (U): rec (n 20) | 11.776 [-7.895, 35.265] |
| ca_tbl_full − fixed_threshold@12 (U): J (n 20) | -1.893 [-3.882, -0.034] |
| ca_tbl_full U − known start: first (n 20) | -10.800 [-41.251, 14.601] |
| ca_tbl_full U − known start: never (n 20) | 0.000 [0.000, 0.000] |
| ca_tbl_full U − known start: av (n 20) | 0.001 [-0.010, 0.013] |
| ca_tbl_full U − known start: w25 (n 20) | -0.109 [-0.146, -0.073] |
| ca_tbl_full U − known start: w25acq (n 20) | -0.091 [-0.123, -0.058] |
| ca_tbl_full U − known start: med (n 20) | 2.188 [1.088, 3.487] |
| ca_tbl_full U − known start: c95 (n 20) | 12.696 [4.088, 25.533] |
| ca_tbl_full U − known start: rec (n 20) | 38.421 [5.461, 76.776] |
| ca_tbl_full U − known start: J (n 20) | -0.648 [-2.441, 1.417] |
| static_2p4 U − known start: first (n 20) | 81.350 [7.024, 161.079] |
| static_2p4 U − known start: never (n 20) | 0.020 [0.000, 0.050] |
| static_2p4 U − known start: av (n 20) | 0.001 [-0.003, 0.004] |
| static_2p4 U − known start: w25 (n 20) | -0.110 [-0.153, -0.072] |
| static_2p4 U − known start: w25acq (n 20) | -0.052 [-0.086, -0.018] |
| static_2p4 U − known start: med (n 20) | 3.920 [1.588, 7.094] |
| static_2p4 U − known start: c95 (n 20) | 11.574 [6.302, 17.580] |
| static_2p4 U − known start: rec (n 20) | 54.559 [-0.149, 130.956] |
| static_2p4 U − known start: J (n 20) | 0.049 [-0.031, 0.127] |

## H_A: held-out, s* = 10 − s = 1 (control: ACC-A M_A_k10)

| contrast | value |
|---|---|
| ca_tbl_full: av (n 20) | 0.008 [-0.004, 0.020] |
| ca_tbl_full: w25 (n 20) | 0.042 [-0.000, 0.089] |
| ca_tbl_full: w25acq (n 20) | 0.018 [-0.038, 0.075] |
| ca_tbl_full: med (n 20) | -5.821 [-12.363, -1.186] |
| ca_tbl_full: c95 (n 20) | 8.951 [-1.468, 17.376] |
| ca_tbl_full: rec (n 20) | 70.000 [25.872, 122.877] |
| ca_tbl_full: J (n 20) | 0.420 [-0.005, 0.864] |
| ca_tbl_full: first (n 20) | -8.000 [-79.002, 73.700] |
| static_2p4: av (n 20) | -0.001 [-0.002, 0.001] |
| static_2p4: w25 (n 20) | 0.007 [-0.013, 0.028] |
| static_2p4: w25acq (n 20) | -0.005 [-0.040, 0.036] |
| static_2p4: med (n 20) | 0.664 [-0.255, 1.627] |
| static_2p4: c95 (n 20) | -0.817 [-16.766, 9.380] |
| static_2p4: rec (n 20) | 96.458 [17.639, 178.337] |
| static_2p4: J (n 20) | -0.035 [-0.117, 0.040] |
| static_2p4: first (n 20) | 14.350 [1.150, 33.325] |

ca_tbl_full subjects at s*: NIS > 9.21 share 0.1656, ANEES 4.52

## H_B: held-out, s* = 10 − s = 1 (control: ACC-A M_B_k10)

| contrast | value |
|---|---|
| ca_tbl_full: av (n 20) | 0.014 [0.004, 0.024] |
| ca_tbl_full: w25 (n 20) | -0.001 [-0.010, 0.010] |
| ca_tbl_full: w25acq (n 20) | -0.008 [-0.016, -0.001] |
| ca_tbl_full: med (n 20) | 1.590 [1.262, 1.956] |
| ca_tbl_full: c95 (n 20) | 3.730 [2.027, 5.518] |
| ca_tbl_full: rec (n 20) | 22.368 [7.434, 41.515] |
| ca_tbl_full: J (n 20) | 1.500 [-0.999, 4.053] |
| ca_tbl_full: first (n 20) | -3.400 [-30.701, 20.226] |
| static_2p4: av (n 20) | 0.002 [-0.004, 0.007] |
| static_2p4: w25 (n 20) | 0.005 [-0.011, 0.020] |
| static_2p4: w25acq (n 20) | 0.007 [-0.011, 0.025] |
| static_2p4: med (n 20) | 0.653 [0.439, 0.863] |
| static_2p4: c95 (n 20) | 2.494 [0.618, 4.305] |
| static_2p4: rec (n 20) | 30.921 [-11.645, 90.921] |
| static_2p4: J (n 20) | -0.022 [-0.104, 0.054] |
| static_2p4: first (n 20) | 11.950 [-34.325, 56.801] |

ca_tbl_full subjects at s*: NIS > 9.21 share 0.0905, ANEES 4.13

## T: training (seeds 21-30, A, ca_tbl_full)

| s | NIS > 9.21 share | ANEES | w25 | med | J |
|---|---|---|---|---|---|
| 1 | 0.4207 | 12.67 | 0.595 | 26.7 | 19.28 |
| 3 | 0.3137 | 8.99 | 0.601 | 23.7 | 19.10 |
| 10 | 0.2078 | 6.83 | 0.642 | 21.4 | 18.59 |

## Registered predictions

| # | prediction | verdict |
|---|---|---|
| P1 | U_A w25 ca − static above 0 | held |
| P2 | U_A never-fix: static ≥ 0.40 and ca ≤ 0.15 | not evaluable |
| P3 | U_A and U_B first fix ca − static below 0 | held |
| P4 | U − C w25acq within ±0.05 (ca_tbl_full) | failed |
| P5 | U w25 and av, ca − fixed@12 contain 0 | held |
| P6 | stage C byte-identity 18/18 | held |
| P7 | T s=1 NIS > 9.21 share > 0.05 | held |
| P8 | T share strictly decreasing in s | held |
| P9 | H_A ca w25 s* − s1 above 0 | held |
| P10 | H_B ca CEP95 s* − s1 ≤ +10 % | not evaluable |
| P11 | T |ANEES − 2| smaller at s* than s = 1 | not evaluable |

Problems: 0
