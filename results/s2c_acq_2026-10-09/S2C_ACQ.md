# S2-C-ACQ

Reuse check: 4 of 4 seed-arm pairs byte-identical to S2-C U_A.

| arm | metric | U_A (off) | Q3 (acquire at 3) | Q3 − U_A |
|---|---|---|---|---|
| ca_tbl_full | first | 467.650 | 703.550 | 235.900 [144.249, 325.602] |
| ca_tbl_full | never | 0.140 | 0.280 | 0.140 [0.060, 0.220] |
| ca_tbl_full | av | 0.270 | 0.257 | -0.013 [-0.048, 0.019] |
| ca_tbl_full | w25 | 0.180 | 0.164 | -0.015 [-0.101, 0.099] |
| ca_tbl_full | w25acq | 0.230 | 0.236 | 0.007 [-0.117, 0.165] |
| ca_tbl_full | med | 325.266 | 416.977 | 91.711 [-10.617, 186.330] |
| ca_tbl_full | c95 | 406.272 | 432.439 | 26.166 [-141.525, 205.382] |
| ca_tbl_full | rec | 272.708 | 153.250 | -342.500 [-871.250, -32.500] |
| ca_tbl_full | J | 18.608 | 20.410 | 1.802 [0.032, 4.058] |
| static_2p4 | first | 1060.600 | 1739.000 | 678.400 [557.500, 791.806] |
| static_2p4 | never | 0.440 | 0.940 | 0.500 [0.400, 0.600] |
| static_2p4 | av | 0.019 | 0.001 | -0.018 [-0.025, -0.011] |
| static_2p4 | w25 | 0.139 | 0.010 | -0.129 [-0.180, -0.079] |
| static_2p4 | w25acq | 0.283 | 0.239 | -0.184 [-0.542, 0.241] |
| static_2p4 | med | 334.130 | 702.523 | 368.393 [289.636, 436.798] |
| static_2p4 | c95 | 398.394 | 644.856 | 246.462 [-61.372, 509.883] |
| static_2p4 | rec | 93.333 | nan | n/a |
| static_2p4 | J | 14.272 | 14.385 | 0.113 [-0.341, 0.568] |

| # | prediction | verdict |
|---|---|---|
| S1 | ca_tbl_full w25 Q3 − U_A above 0 | failed |
| S2 | ca_tbl_full median error Q3 − U_A below 0 | failed |
| S3 | ca_tbl_full first fix Q3 − U_A mean ≥ 0 | held |
| S4 | static 2.4 GHz never-fix share Q3 ≥ U_A | held |
| S5 | reuse check 4 of 4 | held |

Problems: 0
