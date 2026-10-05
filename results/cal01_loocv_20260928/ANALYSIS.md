# CAL-01 analysis, applying report/PREREG_CAL01_LOOCV.md

`python3 scripts/cal01_loocv.py --root results/cal01_loocv_20260928 --source c019c0a`

3 regimes x 20 seeds x 2 arms. Bootstrap: 10000 seed-level resamples, seed 20260929 + cell index. Primary CI level 0.9833 (Bonferroni over 3 primary cells).

## 0. Abort conditions

A1 (provenance and invocation), A2 (completeness, rows counted), A3 (t3 range), A8 (errM against hypot(est - true)): none fired. 120 traces x 720 rows.

## 1. Populations (descriptive)

Population: fix cycles (`hadFix == 1`) of the measured tag; window t = 2.5 ... 1800 s of each run; each cycle's error and terms are instantaneous, so the window truncates nothing.

| regime | arm | fix cycles / rows | fix tier | t3 fed (A4) | err p50 / p90 / p99 m |
|---|---|---|---|---|---|
| default | ca_tbl_full | 12193 / 14400 | 2.4GHz 509, 868MHz 11684 | 12077 / 12193 = 0.990 | 71.9 / 205.3 / 337.7 |
| default | static_868 | 14311 / 14400 | 868MHz 14311 | 14203 / 14311 = 0.992 | 122.8 / 241.4 / 373.2 |
| arc | ca_tbl_full | 8063 / 14400 | 2.4GHz 33, 868MHz 8030 | 7932 / 8063 = 0.984 | 216.2 / 620.2 / 1584.5 |
| arc | static_868 | 9758 / 14400 | 868MHz 9758 | 9627 / 9758 = 0.987 | 218.7 / 544.0 / 1058.1 |
| searcher | ca_tbl_full | 14042 / 14400 | 2.4GHz 10658, 868MHz 3384 | 13915 / 14042 = 0.991 | 5.1 / 27.2 / 310.7 |
| searcher | static_868 | 14298 / 14400 | 868MHz 14298 | 14192 / 14298 = 0.993 | 60.9 / 209.1 / 2619.5 |

| regime | arm | t1 mean / sd [min, max] | t2 mean / sd [min, max] | t3 mean / sd [min, max] |
|---|---|---|---|---|
| default | ca_tbl_full | 0.007008 / 0.02471 [8.764e-05, 0.4639] | 0.9791 / 0.07083 [0.01309, 1] | 0.9162 / 0.1087 [-9.321e-05, 1] |
| default | static_868 | 0.0003128 / 0.0004658 [0.0001031, 0.01263] | 0.9881 / 0.05053 [0.2072, 1] | 0.9129 / 0.1081 [-9.414e-05, 1] |
| arc | ca_tbl_full | 0.0004935 / 0.004343 [8.817e-07, 0.1307] | 0.7789 / 0.1878 [0.09298, 1] | 0.9075 / 0.1452 [-0.0001281, 1] |
| arc | static_868 | 0.0001085 / 8.653e-05 [8.632e-07, 0.0005029] | 0.7915 / 0.1799 [0.1001, 1] | 0.9137 / 0.1385 [-0.0001241, 1] |
| searcher | ca_tbl_full | 0.08997 / 0.09146 [5.999e-05, 0.5632] | 0.5769 / 0.3286 [0.003107, 1] | 0.94 / 0.09959 [-0.0001102, 1] |
| searcher | static_868 | 0.002011 / 0.005462 [1.064e-05, 0.2914] | 0.9196 / 0.1594 [0.01863, 1] | 0.9135 / 0.1089 [-7.204e-05, 1] |

## 2. Rule 3: t3 against a quantity its derivation never read (A7), and A9

| regime | arm | fix cycles | t3 mismatches > 5e-04 | share | fix-set size matches nlos_links | cycles with a scored link | A9 max abs(rho(t1-only) - rho_ctl) |
|---|---|---|---|---|---|---|---|
| default | ca_tbl_full | 12193 | 0 | 0.0000 | 12193 | 12077 | 0.0e+00 |
| default | static_868 | 14311 | 0 | 0.0000 | 14311 | 14203 | 0.0e+00 |
| arc | ca_tbl_full | 8063 | 0 | 0.0000 | 8063 | 7932 | 0.0e+00 |
| arc | static_868 | 9758 | 0 | 0.0000 | 9758 | 9627 | 0.0e+00 |
| searcher | ca_tbl_full | 14042 | 0 | 0.0000 | 14042 | 13915 | 0.0e+00 |
| searcher | static_868 | 14298 | 0 | 0.0000 | 14298 | 14192 | 0.0e+00 |

## 3. The registered test: FIT-R against the control, leave-one-seed-out

Per held-out seed, A = rho_control - rho_model (positive = the calibrated score ranks error better). Mean over evaluable seeds, seed-bootstrap CI. `As` is against the sign-refitted control; `A` against the registered fixed-sign rho(-traceSigma). The verdict is read from the Bonferroni CI of As, and BEATS also requires A's Bonferroni CI above zero.

| regime | arm | status | eval seeds | rho_ctl | rho_ctl (sign-refitted) | rho FIT-R | As, 95% CI | As, 98.33% CI | A, 98.33% CI | verdict |
|---|---|---|---|---|---|---|---|---|---|---|
| default | ca_tbl_full | OK | 20 | -0.6863 | -0.6863 | -0.6847 | -0.0016 [-0.0055, +0.0022] | -0.0016 [-0.0066, +0.0030] | -0.0016 [-0.0066, +0.0030] | INDISTINGUISHABLE |
| default | static_868 | OK | 20 | -0.1392 | -0.1392 | -0.1527 | +0.0135 [-0.0155, +0.0436] | +0.0135 [-0.0217, +0.0499] | +0.0135 [-0.0217, +0.0499] | INDISTINGUISHABLE |
| arc | ca_tbl_full | OK | 20 | -0.4346 | -0.4346 | -0.4390 | +0.0045 [-0.0096, +0.0186] | +0.0045 [-0.0129, +0.0217] | +0.0045 [-0.0129, +0.0217] | INDISTINGUISHABLE |
| arc | static_868 | OK | 20 | -0.3114 | -0.3114 | -0.3168 | +0.0055 [-0.0089, +0.0208] | +0.0055 [-0.0116, +0.0251] | +0.0055 [-0.0116, +0.0251] | INDISTINGUISHABLE |
| searcher | ca_tbl_full | OK | 20 | -0.4684 | -0.4684 | -0.4673 | -0.0011 [-0.0034, +0.0001] | -0.0011 [-0.0044, +0.0001] | -0.0011 [-0.0044, +0.0001] | INDISTINGUISHABLE |
| searcher | static_868 | OK | 20 | -0.4889 | -0.4889 | -0.4956 | +0.0067 [-0.0024, +0.0165] | +0.0067 [-0.0044, +0.0187] | +0.0067 [-0.0044, +0.0187] | INDISTINGUISHABLE |

Folds whose training seeds' mean rho(-traceSigma, error) was POSITIVE (the sign-refitted control flipped): default/ca_tbl_full 0, default/static_868 0, arc/ca_tbl_full 0, arc/static_868 0, searcher/ca_tbl_full 0, searcher/static_868 0

FIT-R fold choices: distinct directions / folds that chose the control itself: default/ca_tbl_full 9/0, default/static_868 3/0, arc/ca_tbl_full 9/0, arc/static_868 8/0, searcher/ca_tbl_full 3/0, searcher/static_868 3/0

## 4. Every model, every cell (descriptive except FIT-R on the primary arm)

| regime | arm | model | mean rho(score, error) | As, 95% CI | verdict at 95% |
|---|---|---|---|---|---|
| default | ca_tbl_full | FIT-R | -0.6847 | -0.0016 [-0.0055, +0.0022] | INDISTINGUISHABLE |
| default | ca_tbl_full | FIT-LS | -0.4236 | -0.2627 [-0.2884, -0.2372] | LOSES |
| default | ca_tbl_full | FIT-LS1 | -0.4866 | -0.1996 [-0.2249, -0.1749] | LOSES |
| default | ca_tbl_full | HAND | -0.2121 | -0.4742 [-0.5107, -0.4372] | LOSES |
| default | ca_tbl_full | LEG | -0.0533 | -0.6330 [-0.6827, -0.5812] | LOSES |
| default | static_868 | FIT-R | -0.1527 | +0.0135 [-0.0155, +0.0436] | INDISTINGUISHABLE |
| default | static_868 | FIT-LS | -0.1388 | -0.0004 [-0.0437, +0.0432] | INDISTINGUISHABLE |
| default | static_868 | FIT-LS1 | -0.1480 | +0.0088 [-0.0301, +0.0494] | INDISTINGUISHABLE |
| default | static_868 | HAND | 0.0376 | -0.1768 [-0.2656, -0.0878] | LOSES |
| default | static_868 | LEG | 0.0418 | -0.1810 [-0.2705, -0.0906] | LOSES |
| arc | ca_tbl_full | FIT-R | -0.4390 | +0.0045 [-0.0096, +0.0186] | INDISTINGUISHABLE |
| arc | ca_tbl_full | FIT-LS | -0.1941 | -0.2405 [-0.3369, -0.1506] | LOSES |
| arc | ca_tbl_full | FIT-LS1 | -0.2418 | -0.1928 [-0.3181, -0.0854] | LOSES |
| arc | ca_tbl_full | HAND | -0.1383 | -0.2963 [-0.4113, -0.1942] | LOSES |
| arc | ca_tbl_full | LEG | -0.1317 | -0.3029 [-0.4213, -0.1984] | LOSES |
| arc | static_868 | FIT-R | -0.3168 | +0.0055 [-0.0089, +0.0208] | INDISTINGUISHABLE |
| arc | static_868 | FIT-LS | -0.2077 | -0.1037 [-0.1653, -0.0392] | LOSES |
| arc | static_868 | FIT-LS1 | -0.2788 | -0.0325 [-0.0751, +0.0103] | INDISTINGUISHABLE |
| arc | static_868 | HAND | -0.1600 | -0.1514 [-0.2082, -0.0953] | LOSES |
| arc | static_868 | LEG | -0.1599 | -0.1515 [-0.2083, -0.0954] | LOSES |
| searcher | ca_tbl_full | FIT-R | -0.4673 | -0.0011 [-0.0034, +0.0001] | INDISTINGUISHABLE |
| searcher | ca_tbl_full | FIT-LS | -0.2802 | -0.1882 [-0.2214, -0.1548] | LOSES |
| searcher | ca_tbl_full | FIT-LS1 | -0.2881 | -0.1803 [-0.2123, -0.1483] | LOSES |
| searcher | ca_tbl_full | HAND | -0.2569 | -0.2116 [-0.3034, -0.1266] | LOSES |
| searcher | ca_tbl_full | LEG | -0.1460 | -0.3224 [-0.4466, -0.2052] | LOSES |
| searcher | static_868 | FIT-R | -0.4956 | +0.0067 [-0.0024, +0.0165] | INDISTINGUISHABLE |
| searcher | static_868 | FIT-LS | -0.1862 | -0.3026 [-0.3449, -0.2539] | LOSES |
| searcher | static_868 | FIT-LS1 | -0.3237 | -0.1651 [-0.2060, -0.1251] | LOSES |
| searcher | static_868 | HAND | -0.1959 | -0.2929 [-0.3462, -0.2375] | LOSES |
| searcher | static_868 | LEG | -0.1707 | -0.3181 [-0.3755, -0.2583] | LOSES |

Single terms, mean per-seed rho with error (confidence convention): default/ca_tbl_full t2 0.1493, t3 -0.0499; default/static_868 t2 -0.0099, t3 0.0526; arc/ca_tbl_full t2 -0.2121, t3 0.0552; arc/static_868 t2 -0.2467, t3 0.0557; searcher/ca_tbl_full t2 -0.0117, t3 -0.0590; searcher/static_868 t2 -0.2155, t3 0.0025

## 5. Fitted weights on all 20 seeds (confidence convention: higher = more confident)

- **default / ca_tbl_full**. FIT-R +(1, -0.0004217, -1.334e-06), normalised to sum 3: (3.001, -0.001266, -4.002e-06); training mean rho -0.6877; mean per-seed rank agreement with t1 (i.e. with the trace) 0.9844. Bootstrap (10000): w1 > 0 in 10000, w1 < 0 in 0, w1 = 0 in 0, the control itself in 0; among w1 > 0, w2/w1 95% CI [-0.0007499, 0.0001778], w3/w1 [-0.0003162, 0.0001778].
  FIT-LS (machinery model, sign flipped to confidence) (847.5, -68.63, -31.82), 95% CI [760.6, 943.6; -80.85, -56.78; -45.74, -17.68]; the machinery's own sum-3 normalisation of the error-convention fit (3.403, -0.2756, -0.1278). FIT-LS1 (with intercept) (882.9, -28.54, -17.73).
- **default / static_868**. FIT-R +(1, 0.0005623, -0.0007499), normalised to sum 3: (3.001, 0.001687, -0.00225); training mean rho -0.1567; mean per-seed rank agreement with t1 (i.e. with the trace) 0.6737. Bootstrap (10000): w1 > 0 in 10000, w1 < 0 in 0, w1 = 0 in 0, the control itself in 0; among w1 > 0, w2/w1 95% CI [-0.001334, 0.002371], w3/w1 [-0.001778, 4.217e-05].
  FIT-LS (machinery model, sign flipped to confidence) (3.119e+04, -82.07, -68.98), 95% CI [2.448e+04, 4.488e+04; -102.2, -62.06; -91.19, -48.68]; the machinery's own sum-3 normalisation of the error-convention fit (3.015, -0.007932, -0.006666). FIT-LS1 (with intercept) (3.147e+04, 11.07, -51.36).
- **arc / ca_tbl_full**. FIT-R +(1, 5.623e-05, -1.334e-05), normalised to sum 3: (3, 0.0001687, -4e-05); training mean rho -0.4447; mean per-seed rank agreement with t1 (i.e. with the trace) 0.9628. Bootstrap (10000): w1 > 0 in 10000, w1 < 0 in 0, w1 = 0 in 0, the control itself in 0; among w1 > 0, w2/w1 95% CI [1e-05, 0.0001334], w3/w1 [-5.623e-05, 3.162e-05].
  FIT-LS (machinery model, sign flipped to confidence) (5967, 350, -619.9), 95% CI [4602, 8435; 83.21, 633.6; -924.1, -337.6]; the machinery's own sum-3 normalisation of the error-convention fit (3.142, 0.1843, -0.3264). FIT-LS1 (with intercept) (6314, 627.7, -102.9).
- **arc / static_868**. FIT-R +(1, 4.217e-05, -1e-05), normalised to sum 3: (3, 0.0001265, -3e-05); training mean rho -0.3218; mean per-seed rank agreement with t1 (i.e. with the trace) 0.9801. Bootstrap (10000): w1 > 0 in 9891, w1 < 0 in 99, w1 = 0 in 10, the control itself in 0; among w1 > 0, w2/w1 95% CI [7.499e-06, 0.0001334], w3/w1 [-5.623e-05, 3.162e-05].
  FIT-LS (machinery model, sign flipped to confidence) (4.679e+05, 169.8, -494.6), 95% CI [2.913e+05, 6.608e+05; 46.45, 302.3; -621.2, -367.6]; the machinery's own sum-3 normalisation of the error-convention fit (3.002, 0.001089, -0.003173). FIT-LS1 (with intercept) (5.889e+05, 380.8, -48.44).
- **searcher / ca_tbl_full**. FIT-R +(1, -0.0002371, -0.0007499), normalised to sum 3: (3.003, -0.0007121, -0.002252); training mean rho -0.4687; mean per-seed rank agreement with t1 (i.e. with the trace) 0.9997. Bootstrap (10000): w1 > 0 in 10000, w1 < 0 in 0, w1 = 0 in 0, the control itself in 0; among w1 > 0, w2/w1 95% CI [-0.0005623, 0.007499], w3/w1 [-0.004217, -0.0001].
  FIT-LS (machinery model, sign flipped to confidence) (168.1, -34, -16.85), 95% CI [42.88, 349.4; -78.55, -4.666; -26.15, -9.224]; the machinery's own sum-3 normalisation of the error-convention fit (4.301, -0.8697, -0.4309). FIT-LS1 (with intercept) (167.3, -33.08, -7.432).
- **searcher / static_868**. FIT-R +(1, 0.0007499, -0.0004217), normalised to sum 3: (2.999, 0.002249, -0.001265); training mean rho -0.4963; mean per-seed rank agreement with t1 (i.e. with the trace) 0.9687. Bootstrap (10000): w1 > 0 in 10000, w1 < 0 in 0, w1 = 0 in 0, the control itself in 0; among w1 > 0, w2/w1 95% CI [-1.334e-05, 0.001], w3/w1 [-0.0005623, 4.217e-05].
  FIT-LS (machinery model, sign flipped to confidence) (5055, 790.2, -977.3), 95% CI [1653, 1.616e+04; 4.318, 1814; -2177, -90.39]; the machinery's own sum-3 normalisation of the error-convention fit (3.115, 0.487, -0.6023). FIT-LS1 (with intercept) (4705, 1207, -94.71).

## 6. Secondary: RMSE, the machinery's own loss

| regime | arm | seed-LOO RMSE FIT-LS | t1-only | FIT-LS1 | t1 + intercept | machinery, cycle-level LOO: fit / t1-only / verdict |
|---|---|---|---|---|---|---|
| default | ca_tbl_full | 77.52 | 121.14 | 77.47 | 77.50 | 77.36 / 121.13 / OK |
| default | static_868 | 78.59 | 145.12 | 78.44 | 78.61 | 78.40 / 143.50 / OK |
| arc | ca_tbl_full | 306.53 | 429.95 | 293.23 | 313.30 | 298.34 / 429.90 / OK |
| arc | static_868 | 213.49 | 308.98 | 200.76 | 210.47 | 211.36 / 308.23 / OK |
| searcher | ca_tbl_full | 80.16 | 82.38 | 80.17 | 80.18 | 78.32 / 82.33 / OK |
| searcher | static_868 | 489.38 | 486.59 | 490.57 | 473.97 | 429.12 / 486.57 / OK |

## 7. The registered decision rule, applied

- default / ca_tbl_full: FIT-R INDISTINGUISHABLE
- arc / ca_tbl_full: FIT-R INDISTINGUISHABLE
- searcher / ca_tbl_full: FIT-R INDISTINGUISHABLE

**VERDICT: CALIBRATION DOES NOT RESCUE PCS**
