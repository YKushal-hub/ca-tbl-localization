# LIMITS L2 -- the two-tier arm (scripts/limits_analyse.py)

Sources of the two-tier cells: 83519595. Comparator arms are the ENV-SWEEP's per-arm results (/home/kushal/ns/catbl-work/envsweep_2026-11-14), reused after the per-arm independence check (VERIFY cell). Window 1800 s / 720 cycles per node.

| config | env | arm | A(25 m) tag 0 | A(25 m) five-node | runs >= 0.95 | CEP95 tag 0 (m) | subject J |
|---|---|---|---|---|---|---|---|
| A | E0 | ca_tbl_two_tier | 0.847 | 0.509 | 0.00 | 370.9 | 21.25 |
| A | E0 | ca_tbl_full | 0.994 | 0.960 | 0.85 | 1.9 | 15.90 |
| A | E0 | fixed_threshold | 0.994 | 0.967 | 0.90 | 2.0 | 15.82 |
| A | E0 | static_868 | 0.830 | 0.479 | 0.00 | 372.1 | 8.46 |
| A | E0 | static_2p4 | 0.994 | 0.964 | 0.75 | 2.7 | 7.62 |
| A | E0 | poucet_fusion | 0.995 | 0.938 | 0.60 | 1.9 | 454.20 |
| A | E1 | ca_tbl_two_tier | 0.904 | 0.356 | 0.00 | 46.7 | 23.46 |
| A | E1 | ca_tbl_full | 0.989 | 0.962 | 0.90 | 4.6 | 13.74 |
| A | E1 | fixed_threshold | 0.989 | 0.963 | 0.90 | 4.6 | 13.69 |
| A | E1 | static_868 | 0.889 | 0.337 | 0.00 | 59.4 | 8.52 |
| A | E1 | static_2p4 | 0.985 | 0.951 | 0.65 | 5.9 | 6.09 |
| A | E1 | poucet_fusion | 0.987 | 0.946 | 0.75 | 4.2 | 457.64 |
| A | E2 | ca_tbl_two_tier | 0.041 | 0.089 | 0.00 | 415.9 | 17.23 |
| A | E2 | ca_tbl_full | 0.974 | 0.905 | 0.45 | 11.3 | 14.72 |
| A | E2 | fixed_threshold | 0.957 | 0.890 | 0.40 | 12.7 | 14.93 |
| A | E2 | static_868 | 0.021 | 0.090 | 0.00 | 433.2 | 8.48 |
| A | E2 | static_2p4 | 0.889 | 0.766 | 0.00 | 141.6 | 7.71 |
| A | E2 | poucet_fusion | 0.915 | 0.860 | 0.15 | 119.0 | 460.51 |
| A | E3 | ca_tbl_two_tier | 0.144 | 0.053 | 0.00 | 1435.2 | 15.93 |
| A | E3 | ca_tbl_full | 0.023 | 0.024 | 0.00 | 1078.9 | 24.10 |
| A | E3 | fixed_threshold | 0.028 | 0.029 | 0.00 | 1036.7 | 23.30 |
| A | E3 | static_868 | 0.103 | 0.033 | 0.00 | 1219.7 | 9.80 |
| A | E3 | static_2p4 | 0.049 | 0.024 | 0.00 | 846.9 | 12.97 |
| A | E3 | poucet_fusion | 0.041 | 0.036 | 0.00 | 1245.8 | 457.18 |
| B | E0 | ca_tbl_two_tier | 0.877 | 0.664 | 0.00 | 21.8 | 73.94 |
| B | E0 | ca_tbl_full | 0.999 | 0.992 | 1.00 | 0.9 | 25.48 |
| B | E0 | fixed_threshold | 0.999 | 0.992 | 1.00 | 0.9 | 25.60 |
| B | E0 | static_868 | 0.954 | 0.587 | 0.00 | 12.8 | 10.35 |
| B | E0 | static_2p4 | 0.999 | 0.987 | 1.00 | 0.9 | 11.39 |
| B | E0 | poucet_fusion | 0.999 | 0.974 | 0.90 | 1.1 | 332.91 |
| B | E1 | ca_tbl_two_tier | 0.948 | 0.826 | 0.05 | 15.2 | 67.38 |
| B | E1 | ca_tbl_full | 0.998 | 0.993 | 1.00 | 1.5 | 25.67 |
| B | E1 | fixed_threshold | 0.998 | 0.993 | 1.00 | 1.5 | 25.47 |
| B | E1 | static_868 | 0.975 | 0.799 | 0.00 | 12.6 | 9.85 |
| B | E1 | static_2p4 | 0.999 | 0.993 | 1.00 | 1.5 | 10.22 |
| B | E1 | poucet_fusion | 0.998 | 0.988 | 1.00 | 1.5 | 398.30 |
| B | E2 | ca_tbl_two_tier | 0.965 | 0.779 | 0.00 | 20.1 | 70.42 |
| B | E2 | ca_tbl_full | 0.998 | 0.990 | 1.00 | 1.9 | 23.09 |
| B | E2 | fixed_threshold | 0.998 | 0.990 | 1.00 | 1.9 | 23.54 |
| B | E2 | static_868 | 0.899 | 0.720 | 0.00 | 27.8 | 9.13 |
| B | E2 | static_2p4 | 0.998 | 0.988 | 1.00 | 1.8 | 8.54 |
| B | E2 | poucet_fusion | 0.998 | 0.983 | 0.95 | 1.9 | 412.84 |
| B | E3 | ca_tbl_two_tier | 0.818 | 0.678 | 0.05 | 47.5 | 30.49 |
| B | E3 | ca_tbl_full | 0.278 | 0.163 | 0.00 | 120.6 | 30.71 |
| B | E3 | fixed_threshold | 0.306 | 0.157 | 0.00 | 109.9 | 30.77 |
| B | E3 | static_868 | 0.630 | 0.541 | 0.00 | 44.9 | 9.22 |
| B | E3 | static_2p4 | 0.267 | 0.144 | 0.00 | 95.6 | 8.50 |
| B | E3 | poucet_fusion | 0.338 | 0.265 | 0.00 | 103.1 | 302.69 |

## CA-TBL (ca_tbl_full) minus ca_tbl_two_tier, paired over seeds

| config | env | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ CEP95 tag 0 (m) | Δ subject J |
|---|---|---|---|---|---|
| A | E0 | 0.452 [0.391, 0.517] | 0.147 [0.040, 0.288] | -368.9 [-684.2, -116.6] | -5.35 [-10.01, -1.21] |
| A | E1 | 0.606 [0.565, 0.647] | 0.085 [0.009, 0.207] | -42.1 [-102.6, -9.8] | -9.71 [-12.81, -6.66] |
| A | E2 | 0.816 [0.765, 0.861] | 0.933 [0.892, 0.963] | -404.6 [-493.6, -331.0] | -2.51 [-5.32, -0.04] |
| A | E3 | -0.029 [-0.049, -0.009] | -0.121 [-0.207, -0.047] | -356.3 [-1215.8, 450.9] | 8.17 [4.28, 11.16] |
| B | E0 | 0.328 [0.280, 0.374] | 0.121 [0.019, 0.258] | -20.9 [-35.9, -9.0] | -48.46 [-54.89, -42.30] |
| B | E1 | 0.167 [0.135, 0.201] | 0.051 [0.010, 0.112] | -13.7 [-26.4, -5.5] | -41.71 [-46.51, -37.23] |
| B | E2 | 0.211 [0.175, 0.248] | 0.033 [0.011, 0.071] | -18.2 [-31.7, -10.2] | -47.33 [-51.26, -43.59] |
| B | E3 | -0.515 [-0.565, -0.466] | -0.541 [-0.630, -0.436] | 73.1 [33.8, 108.4] | 0.21 [-2.45, 2.69] |

## Two-tier minus static_868, paired (is the UWB rung worth anything without tier 2?)

| config | env | Δ A(25 m) five-node | Δ CEP95 tag 0 (m) |
|---|---|---|---|
| A | E0 | 0.030 [-0.014, 0.076] | -1.3 [-9.7, 4.1] |
| A | E1 | 0.019 [-0.007, 0.045] | -12.7 [-35.2, 1.6] |
| A | E2 | -0.000 [-0.022, 0.021] | -17.3 [-52.1, 16.9] |
| A | E3 | 0.020 [0.001, 0.040] | 215.5 [-412.3, 1011.7] |
| B | E0 | 0.077 [0.035, 0.117] | 9.0 [-7.4, 25.4] |
| B | E1 | 0.028 [-0.011, 0.063] | 2.7 [-14.0, 18.9] |
| B | E2 | 0.059 [0.012, 0.103] | -7.7 [-19.9, 0.4] |
| B | E3 | 0.136 [0.065, 0.200] | 2.5 [-17.7, 29.5] |

## Mechanism, tag 0 (both arms; trace_<arm>_seed<N>.csv, all 720 cycles per seed)

UWB cycle = activeTier UWB. 'no UWB fix' = fixTier != UWB on it (the node fell back to the floor). 'no anchor in reach' = visUwb == 0, truth-based count of UWB anchors within the deployment's nominal UWB range.

| config | env | arm | share of cycles with a UWB fix | UWB cycles (pooled) | of which no UWB fix | of which no anchor in reach |
|---|---|---|---|---|---|---|
| A | E0 | ca_tbl_two_tier | 0.003 | 611 | 0.923 | 0.858 |
| A | E0 | ca_tbl_full | 0.000 | 35 | 0.800 | 0.314 |
| A | E1 | ca_tbl_two_tier | 0.001 | 900 | 0.983 | 0.914 |
| A | E1 | ca_tbl_full | 0.000 | 35 | 1.000 | 0.429 |
| A | E2 | ca_tbl_two_tier | 0.001 | 354 | 0.960 | 0.859 |
| A | E2 | ca_tbl_full | 0.000 | 20 | 0.900 | 0.000 |
| A | E3 | ca_tbl_two_tier | 0.003 | 233 | 0.807 | 0.794 |
| A | E3 | ca_tbl_full | 0.001 | 26 | 0.423 | 0.231 |
| B | E0 | ca_tbl_two_tier | 0.030 | 2475 | 0.825 | 0.713 |
| B | E0 | ca_tbl_full | 0.009 | 293 | 0.563 | 0.195 |
| B | E1 | ca_tbl_two_tier | 0.022 | 2423 | 0.866 | 0.641 |
| B | E1 | ca_tbl_full | 0.009 | 324 | 0.611 | 0.228 |
| B | E2 | ca_tbl_two_tier | 0.016 | 2163 | 0.891 | 0.576 |
| B | E2 | ca_tbl_full | 0.008 | 329 | 0.650 | 0.100 |
| B | E3 | ca_tbl_two_tier | 0.056 | 1961 | 0.585 | 0.556 |
| B | E3 | ca_tbl_full | 0.001 | 25 | 0.520 | 0.360 |

## Registered predictions (L2)

- Q1 full - two-tier five-node A(25 m), A, E1 (forest): interval above 0: HOLDS (0.606 [0.565, 0.647])
- Q2 the same in at least 3 of 4 environments in A: HOLDS (environments with the interval above 0: E0, E1, E2)
- Q3 the E1 difference is smaller in B than in A: HOLDS (A 0.606, B 0.167)
- Q4 CEP95 tag 0, A, E1: full better than two-tier (interval of full - two-tier below 0): HOLDS (-42.1 [-102.6, -9.8] m)
- Q5 subject J, A, E1: two-tier cheaper than full (interval of full - two-tier above 0): FAILS (-9.71 [-12.81, -6.66] J)
- Q6 mechanism: at least half of two-tier's tag-0 UWB cycles in A, E1 have no UWB fix: HOLDS (885/900 = 0.983)

Problems: 0 (none)
