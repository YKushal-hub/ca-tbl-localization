# ACC-A-OPPOINT

fixed_threshold@T − ca_tbl_full (ACC-A M_k10), paired seeds 1-20, mean [95 % bootstrap]. Control byte-identical on 20 of 20 seeds.

## B, T = 8

| metric | fixed_threshold | ca_tbl_full | difference |
|---|---|---|---|
| availability | 0.586 | 0.575 | 0.011 [0.002, 0.021] |
| J per node | 30.040 | 26.011 | 4.028 [0.843, 7.237] |
| tag 0 CEP50 | 2.813 | 2.877 | -0.064 [-0.274, 0.158] |
| tag 0 CEP95 | 7.170 | 7.499 | -0.329 [-1.409, 0.954] |
| within 25 m | 0.975 | 0.977 | -0.002 [-0.018, 0.014] |
| escalations | 37.450 | 56.800 | -19.350 [-23.450, -15.199] |
| UWB cycles | 42.650 | 41.450 | 1.200 [-8.850, 11.900] |
| UWB-solved fixes | 8.500 | 7.550 | 0.950 [-1.850, 3.700] |

Escalation decisions by fixed_threshold's own rule (trace > 8): 63 of 788 (8.0 %)

## B, T = 12

| metric | fixed_threshold | ca_tbl_full | difference |
|---|---|---|---|
| availability | 0.586 | 0.575 | 0.011 [0.003, 0.020] |
| J per node | 28.138 | 26.011 | 2.127 [-0.423, 4.584] |
| tag 0 CEP50 | 2.906 | 2.877 | 0.029 [-0.241, 0.297] |
| tag 0 CEP95 | 7.273 | 7.499 | -0.226 [-1.224, 0.762] |
| within 25 m | 0.973 | 0.977 | -0.004 [-0.016, 0.007] |
| escalations | 46.900 | 56.800 | -9.900 [-13.400, -6.050] |
| UWB cycles | 42.800 | 41.450 | 1.350 [-8.350, 10.550] |
| UWB-solved fixes | 7.100 | 7.550 | -0.450 [-3.250, 2.600] |

Escalation decisions by fixed_threshold's own rule (trace > 12): 26 of 916 (2.8 %)

## B, T = 25

| metric | fixed_threshold | ca_tbl_full | difference |
|---|---|---|---|
| availability | 0.573 | 0.575 | -0.002 [-0.011, 0.008] |
| J per node | 21.822 | 26.011 | -4.190 [-6.332, -1.679] |
| tag 0 CEP50 | 3.069 | 2.877 | 0.192 [-0.033, 0.415] |
| tag 0 CEP95 | 8.019 | 7.499 | 0.521 [-0.916, 2.042] |
| within 25 m | 0.979 | 0.977 | 0.002 [-0.008, 0.011] |
| escalations | 53.400 | 56.800 | -3.400 [-7.200, 0.700] |
| UWB cycles | 36.050 | 41.450 | -5.400 [-15.800, 4.800] |
| UWB-solved fixes | 5.450 | 7.550 | -2.100 [-3.750, -0.300] |

Escalation decisions by fixed_threshold's own rule (trace > 25): 6 of 1058 (0.6 %)

## B, T = 100

| metric | fixed_threshold | ca_tbl_full | difference |
|---|---|---|---|
| availability | 0.544 | 0.575 | -0.031 [-0.042, -0.021] |
| J per node | 19.898 | 26.011 | -6.114 [-8.227, -4.021] |
| tag 0 CEP50 | 3.024 | 2.877 | 0.147 [-0.239, 0.551] |
| tag 0 CEP95 | 7.558 | 7.499 | 0.059 [-1.310, 1.468] |
| within 25 m | 0.975 | 0.977 | -0.002 [-0.015, 0.009] |
| escalations | 55.950 | 56.800 | -0.850 [-4.000, 2.350] |
| UWB cycles | 33.050 | 41.450 | -8.400 [-17.300, 0.100] |
| UWB-solved fixes | 4.600 | 7.550 | -2.950 [-5.500, -0.400] |

Escalation decisions by fixed_threshold's own rule (trace > 100): 5 of 1090 (0.5 %)

## A, T = 12

| metric | fixed_threshold | ca_tbl_full | difference |
|---|---|---|---|
| availability | 0.285 | 0.278 | 0.007 [0.000, 0.013] |
| J per node | 16.857 | 17.290 | -0.434 [-1.009, 0.140] |
| tag 0 CEP50 | 15.516 | 16.588 | -1.072 [-3.457, 1.089] |
| tag 0 CEP95 | 39.126 | 48.258 | -9.132 [-22.700, -0.039] |
| within 25 m | 0.649 | 0.638 | 0.011 [-0.012, 0.035] |
| escalations | 14.500 | 19.200 | -4.700 [-8.500, -1.200] |
| UWB cycles | 1.750 | 4.050 | -2.300 [-6.600, 0.600] |
| UWB-solved fixes | 0.000 | 0.000 | 0.000 [0.000, 0.000] |

Escalation decisions by fixed_threshold's own rule (trace > 12): 200 of 357 (56.0 %)

## Registered predictions

| # | prediction | verdict |
|---|---|---|
| R1 | control byte-identical, 20 of 20 | held |
| R2 | B T12 availability contains 0 | failed |
| R3 | B T12 J contains 0 | held |
| R4 | B T12 CEP50 and CEP95 contain 0 | held |
| R5 | B availability and J non-increasing in T | held |
| R6 | B some T in {8,12,25} matches availability and J | failed |
| R7 | A T12 availability, CEP50, J contain 0 | failed |
| R8 | B T12 UWB cycles contain 0 | held |

Problems: 0
