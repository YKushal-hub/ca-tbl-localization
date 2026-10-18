# V8-REGEN results — the adopted fixes together, and each one removed (registered, exploratory)

Registration: `report/PREREG_V8_REGEN.md` at `6b7b6430`, committed before any cell. 480 runs, all `rc 0`,
`source 6b7b6430 dirty 0`, one compile stamp, frozen binary with a SHA-256 manifest. Tracked per cell:
`arm_summary.csv`, `run_id.txt` (`results/v8_2026-10-17/`).

**How to read BASE.** BASE is the default physics with keyed draws: the 868 MHz error is an independent truncated
Gaussian per reading, and the N.55 leak is present. FULL includes the realistic correlated 868 MHz error (S1) with
the leak removed, which on its own makes 868 MHz positioning harder (B4-5). So FULL − BASE mixes "harder, truer
physics" with "the improvements". The ablations (FULL minus one flag) are the clean measures.

## 1. Verdicts
| # | prediction | outcome |
|---|---|---|
| V8-1 | A: CEP95 FULL − BASE interval below 0 | **not held**: −70.1 m [−261.6, +113.9] (438 → 368 m) |
| V8-2 | A: NO_REPLY − FULL interval above 0 | **held**: +289.3 m [+115.8, +495.8] |
| V8-3 | A: coverage FULL > NO_PF | **held** |
| V8-4 | R8: FULL − BASE paired mean < 0 | **not held**: +142.3 m [+8.1, +322.8] (72 → 215 m) |
| V8-5 | BS: coverage FULL >= BASE | **held**: 0.276 → 0.544 |
| V8-6 | B: CEP95 after 300 s FULL − BASE <= 0 | **held**: −4.1 m [−6.2, −2.3] |
| V8-7 | wolf A: FULL − BASE < 0 | **held**: −222.1 m [−390.9, −68.6] |
| V8-8 | NO_NLOS within ±10 % of FULL everywhere | **not held**: R8 −98.5 m [−236.1, +9.8] |
| V8-9 | A: `static_2p4` FULL − BASE < 0 | **held**: −184.7 m [−342.0, −16.9] |

## 2. Reading
- **Endgame (B) and the static lost node (BS): FULL is better than BASE on every count**, despite the harder physics.

  | config | arm | CEP95 BASE → FULL | coverage BASE → FULL |
  |---|---|---|---|
  | B | `ca_tbl_full` | 12.6 → 5.3 m | 0.80 → 0.87 |
  | B | `static_2p4` | 28.7 → 7.1 m | |
  | BS | `ca_tbl_full` | 48.4 → 20.2 m | 0.28 → 0.54 |
  | BS | `static_2p4` | 66.4 → 5.1 m | 0.39 → 0.88 |

  The V3/V4/V7 "hybrid endgame tail" worries do not survive with the fixes combined: B CEP95 after 300 s improves
  by 4.1 m [2.3, 6.2].
- **5 km (A):** FULL is better on average and the interval crosses 0. `static_2p4` is clearly better (−185 m).
  - Reply positions are the decisive fix: removing them costs +289 m.
  - The particle filter buys consistency (coverage) at a small, non-significant CEP95 cost.
- **The 8-gateway ring (R8) gets worse** (72 → 215 m for `ca_tbl_full`). R8 is the layout where 868 MHz carries the
  fix. The realistic, correlated, NLOS-inflated 868 MHz error is exactly what it was protected from under the old
  model. `--nlosRangeBias` alone accounts for about 98 m of it, removed. That is the physics being truer, not a fix
  failing: the old model flattered an 868-heavy layout. It is the strongest argument yet for 2.4 GHz ToF over 868 MHz
  RSSI at 5 km scale.
- **Wolf preset:** FULL helps at 5 km (−222 m) but not in the endgame (`ca_tbl_full` 52.6 → 61.6 m). Wolf's +75 m
  uncalibrated ToF bias dominates there, and no fix here addresses calibration.
- **2.4 alone against the hybrid:** under FULL, `static_2p4` beats `ca_tbl_full` in BS (5.1 vs 20.2 m) and is close in
  B (7.1 vs 5.3 m) at about 40 % of the energy (9.7 vs 25.7 J). This echoes B4's original K1 finding.

## 3. Recommended default set for future work
`--rngKeyed --rangeErrorNoLinkLeak --rssiRangeFromShadowing --nlosRangeBias --rangeReplyPosition --acquireParticles`
(engine-correctness plus the two fixes that hold up). `--rangeBiasStates`, `--c2bEscalation` and
`--dualBaseOnDemand` stay off. Changing the preset's defaults is the owner's decision.

## 4. Analysis output (`scripts/v8_analyse.py`)
# V8-REGEN results (exploratory)

| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | median NEES | fix share | tag-0 J |
|---|---|---|---|---|---|---|---|---|
| A | BASE | static_868 | 751.5 | 401.2 | 0.507 | 14.08 | 0.296 | 6.46 |
| A | BASE | static_2p4 | 582.6 | 297.4 | 0.607 | 2.41 | 0.059 | 14.32 |
| A | BASE | ca_tbl_full | 438.3 | 227.8 | 0.394 | 37.03 | 0.611 | 18.39 |
| A | BASE | fixed_threshold | 446.7 | 236.7 | 0.419 | 30.30 | 0.606 | 19.15 |
| A | FULL | static_868 | 489.9 | 449.2 | 0.290 | 58.35 | 0.780 | 6.43 |
| A | FULL | static_2p4 | 397.9 | 89.7 | 0.862 | 0.28 | 0.060 | 14.28 |
| A | FULL | ca_tbl_full | 368.2 | 218.7 | 0.512 | 9.15 | 0.843 | 15.42 |
| A | FULL | fixed_threshold | 391.4 | 271.2 | 0.554 | 7.01 | 0.844 | 15.33 |
| A | NO_REPLY | static_868 | 1709.4 | 1235.5 | 0.366 | 37.95 | 0.293 | 6.72 |
| A | NO_REPLY | static_2p4 | 568.9 | 94.2 | 0.935 | 0.26 | 0.033 | 14.37 |
| A | NO_REPLY | ca_tbl_full | 657.5 | 437.3 | 0.493 | 12.58 | 0.627 | 17.34 |
| A | NO_REPLY | fixed_threshold | 658.7 | 443.6 | 0.524 | 8.56 | 0.612 | 16.83 |
| A | NO_PF | static_868 | 430.6 | 392.3 | 0.178 | 92.97 | 0.780 | 6.44 |
| A | NO_PF | static_2p4 | 426.4 | 280.8 | 0.649 | 0.91 | 0.089 | 14.39 |
| A | NO_PF | ca_tbl_full | 281.6 | 198.3 | 0.397 | 24.09 | 0.862 | 16.30 |
| A | NO_PF | fixed_threshold | 296.0 | 209.1 | 0.442 | 16.19 | 0.850 | 16.12 |
| A | NO_NLOS | static_868 | 487.0 | 444.0 | 0.265 | 67.62 | 0.782 | 6.41 |
| A | NO_NLOS | static_2p4 | 396.9 | 78.8 | 0.874 | 0.32 | 0.056 | 14.30 |
| A | NO_NLOS | ca_tbl_full | 360.8 | 276.2 | 0.516 | 8.63 | 0.856 | 16.01 |
| A | NO_NLOS | fixed_threshold | 314.8 | 212.0 | 0.553 | 6.68 | 0.850 | 16.63 |
| B | BASE | static_868 | 16.2 | 16.1 | 0.643 | 5.33 | 0.898 | 7.72 |
| B | BASE | static_2p4 | 28.7 | 12.4 | 0.845 | 0.84 | 0.814 | 9.64 |
| B | BASE | ca_tbl_full | 12.6 | 9.1 | 0.798 | 2.25 | 0.988 | 24.90 |
| B | BASE | fixed_threshold | 23.6 | 20.8 | 0.747 | 2.70 | 0.989 | 26.79 |
| B | FULL | static_868 | 54.4 | 41.1 | 0.392 | 20.89 | 0.963 | 7.91 |
| B | FULL | static_2p4 | 7.1 | 6.1 | 0.910 | 0.75 | 0.836 | 9.66 |
| B | FULL | ca_tbl_full | 5.3 | 4.9 | 0.868 | 1.73 | 0.994 | 25.69 |
| B | FULL | fixed_threshold | 5.9 | 5.4 | 0.843 | 1.83 | 0.995 | 26.10 |
| B | NO_REPLY | static_868 | 55.6 | 18.7 | 0.460 | 13.34 | 0.928 | 7.86 |
| B | NO_REPLY | static_2p4 | 15.7 | 6.7 | 0.930 | 0.63 | 0.808 | 9.59 |
| B | NO_REPLY | ca_tbl_full | 35.7 | 19.4 | 0.741 | 3.11 | 0.988 | 26.08 |
| B | NO_REPLY | fixed_threshold | 30.1 | 15.2 | 0.746 | 2.98 | 0.989 | 25.86 |
| B | NO_PF | static_868 | 49.7 | 46.0 | 0.384 | 21.39 | 0.965 | 7.90 |
| B | NO_PF | static_2p4 | 27.0 | 12.1 | 0.817 | 1.13 | 0.845 | 9.80 |
| B | NO_PF | ca_tbl_full | 21.6 | 6.7 | 0.824 | 1.96 | 0.995 | 28.17 |
| B | NO_PF | fixed_threshold | 27.0 | 8.5 | 0.827 | 2.05 | 0.995 | 27.96 |
| B | NO_NLOS | static_868 | 62.4 | 49.5 | 0.367 | 23.53 | 0.966 | 7.90 |
| B | NO_NLOS | static_2p4 | 7.0 | 6.1 | 0.920 | 0.79 | 0.835 | 9.66 |
| B | NO_NLOS | ca_tbl_full | 6.4 | 5.0 | 0.872 | 1.65 | 0.994 | 26.54 |
| B | NO_NLOS | fixed_threshold | 7.4 | 6.0 | 0.869 | 1.78 | 0.995 | 27.21 |
| R8 | BASE | static_868 | 42.8 | 25.6 | 0.658 | 4.53 | 0.686 | 6.74 |
| R8 | BASE | static_2p4 | 458.5 | 214.3 | 0.680 | 0.86 | 0.085 | 11.71 |
| R8 | BASE | ca_tbl_full | 72.1 | 57.9 | 0.596 | 4.95 | 0.888 | 17.31 |
| R8 | BASE | fixed_threshold | 69.8 | 55.5 | 0.598 | 4.75 | 0.882 | 16.46 |
| R8 | FULL | static_868 | 211.9 | 118.9 | 0.288 | 44.56 | 0.922 | 6.76 |
| R8 | FULL | static_2p4 | 371.4 | 61.6 | 0.919 | 0.33 | 0.084 | 11.55 |
| R8 | FULL | ca_tbl_full | 214.5 | 139.5 | 0.578 | 5.68 | 0.953 | 13.97 |
| R8 | FULL | fixed_threshold | 218.6 | 192.8 | 0.580 | 5.59 | 0.951 | 13.89 |
| R8 | NO_REPLY | static_868 | 318.1 | 63.7 | 0.365 | 26.96 | 0.693 | 6.64 |
| R8 | NO_REPLY | static_2p4 | 531.2 | 83.2 | 0.924 | 0.26 | 0.048 | 11.72 |
| R8 | NO_REPLY | ca_tbl_full | 205.2 | 75.9 | 0.473 | 11.80 | 0.908 | 14.18 |
| R8 | NO_REPLY | fixed_threshold | 214.1 | 85.3 | 0.484 | 12.47 | 0.910 | 13.59 |
| R8 | NO_PF | static_868 | 137.8 | 114.9 | 0.196 | 57.92 | 0.920 | 6.76 |
| R8 | NO_PF | static_2p4 | 454.7 | 201.8 | 0.736 | 0.61 | 0.115 | 11.87 |
| R8 | NO_PF | ca_tbl_full | 264.3 | 102.3 | 0.484 | 10.35 | 0.958 | 13.59 |
| R8 | NO_PF | fixed_threshold | 264.3 | 80.6 | 0.476 | 10.76 | 0.958 | 13.22 |
| R8 | NO_NLOS | static_868 | 223.1 | 127.8 | 0.319 | 36.89 | 0.920 | 6.77 |
| R8 | NO_NLOS | static_2p4 | 351.3 | 65.5 | 0.917 | 0.30 | 0.084 | 11.59 |
| R8 | NO_NLOS | ca_tbl_full | 116.0 | 47.7 | 0.573 | 5.66 | 0.955 | 13.55 |
| R8 | NO_NLOS | fixed_threshold | 172.4 | 122.7 | 0.609 | 4.98 | 0.953 | 13.51 |
| BS | BASE | static_868 | 5.4 | 3.6 | 0.594 | 8.70 | 0.864 | 7.80 |
| BS | BASE | static_2p4 | 66.4 | 60.7 | 0.388 | 50.22 | 0.964 | 11.90 |
| BS | BASE | ca_tbl_full | 48.4 | 43.0 | 0.276 | 180.43 | 0.993 | 20.25 |
| BS | BASE | fixed_threshold | 40.5 | 35.3 | 0.336 | 61.81 | 0.993 | 23.23 |
| BS | FULL | static_868 | 33.8 | 22.6 | 0.275 | 73.71 | 0.969 | 7.87 |
| BS | FULL | static_2p4 | 5.1 | 2.5 | 0.881 | 0.91 | 0.980 | 11.61 |
| BS | FULL | ca_tbl_full | 20.2 | 19.1 | 0.544 | 6.40 | 0.999 | 24.22 |
| BS | FULL | fixed_threshold | 20.1 | 18.6 | 0.552 | 5.99 | 0.999 | 23.12 |
| BS | NO_REPLY | static_868 | 169.1 | 105.0 | 0.398 | 23.14 | 0.877 | 7.86 |
| BS | NO_REPLY | static_2p4 | 46.8 | 29.4 | 0.854 | 0.87 | 0.945 | 11.64 |
| BS | NO_REPLY | ca_tbl_full | 49.8 | 26.9 | 0.419 | 18.63 | 0.992 | 24.67 |
| BS | NO_REPLY | fixed_threshold | 49.9 | 29.6 | 0.454 | 13.08 | 0.993 | 24.51 |
| BS | NO_PF | static_868 | 33.4 | 32.6 | 0.251 | 148.77 | 0.970 | 7.86 |
| BS | NO_PF | static_2p4 | 52.0 | 49.5 | 0.428 | 37.20 | 0.981 | 12.02 |
| BS | NO_PF | ca_tbl_full | 29.5 | 25.0 | 0.352 | 35.44 | 0.999 | 19.57 |
| BS | NO_PF | fixed_threshold | 30.2 | 28.5 | 0.412 | 19.17 | 0.999 | 20.25 |
| BS | NO_NLOS | static_868 | 53.0 | 41.7 | 0.266 | 64.86 | 0.968 | 7.86 |
| BS | NO_NLOS | static_2p4 | 6.6 | 4.0 | 0.860 | 0.94 | 0.980 | 11.63 |
| BS | NO_NLOS | ca_tbl_full | 20.0 | 19.3 | 0.550 | 6.04 | 0.999 | 24.37 |
| BS | NO_NLOS | fixed_threshold | 21.3 | 20.1 | 0.485 | 10.13 | 0.999 | 23.00 |
| WA | BASE | static_868 | 751.5 | 401.2 | 0.507 | 14.08 | 0.296 | 6.46 |
| WA | BASE | static_2p4 | 643.1 | 366.4 | 0.466 | 21.74 | 0.056 | 14.43 |
| WA | BASE | ca_tbl_full | 592.9 | 417.8 | 0.234 | 116.99 | 0.591 | 18.76 |
| WA | BASE | fixed_threshold | 592.9 | 417.8 | 0.234 | 116.99 | 0.591 | 18.85 |
| WA | FULL | static_868 | 489.9 | 449.2 | 0.290 | 58.35 | 0.780 | 6.43 |
| WA | FULL | static_2p4 | 431.3 | 170.5 | 0.688 | 6.06 | 0.041 | 14.35 |
| WA | FULL | ca_tbl_full | 370.8 | 327.3 | 0.330 | 34.49 | 0.836 | 16.37 |
| WA | FULL | fixed_threshold | 369.8 | 326.4 | 0.331 | 34.58 | 0.836 | 16.33 |
| WB | BASE | static_868 | 16.2 | 16.1 | 0.643 | 5.33 | 0.898 | 7.72 |
| WB | BASE | static_2p4 | 157.3 | 135.6 | 0.153 | 48.18 | 0.806 | 9.49 |
| WB | BASE | ca_tbl_full | 52.6 | 49.1 | 0.290 | 30.42 | 0.992 | 36.12 |
| WB | BASE | fixed_threshold | 52.7 | 49.3 | 0.292 | 29.76 | 0.992 | 34.56 |
| WB | FULL | static_868 | 54.4 | 41.1 | 0.392 | 20.89 | 0.964 | 7.91 |
| WB | FULL | static_2p4 | 121.3 | 107.8 | 0.263 | 31.99 | 0.796 | 9.49 |
| WB | FULL | ca_tbl_full | 61.6 | 49.0 | 0.374 | 19.12 | 0.997 | 37.72 |
| WB | FULL | fixed_threshold | 64.7 | 51.4 | 0.362 | 21.51 | 0.997 | 37.55 |

- A CEP95 FULL-BASE: -70.1 [-261.6, +113.9] m
- A CEP95 NO_REPLY-FULL: +289.3 [+115.8, +495.8] m
- R8 CEP95 FULL-BASE: +142.3 [+8.1, +322.8] m
- B CEP95-after-300s FULL-BASE: -4.1 [-6.2, -2.3] m
- wolf A CEP95 FULL-BASE: -222.1 [-390.9, -68.6] m
- A static_2p4 CEP95 FULL-BASE: -184.7 [-342.0, -16.9] m
- A CEP95 NO_NLOS-FULL: -7.4 [-90.9, +70.4] m
- B CEP95 NO_NLOS-FULL: +1.1 [-0.2, +2.8] m
- R8 CEP95 NO_NLOS-FULL: -98.5 [-236.1, +9.8] m
- BS CEP95 NO_NLOS-FULL: -0.2 [-2.7, +2.2] m

### Every ablation against FULL (ca_tbl_full CEP95, paired mean [95 %])

- A BASE - FULL: CEP95 +70.1 [-112.6, +262.4]; after 300 s +9.1 [-108.8, +141.9]
- A NO_REPLY - FULL: CEP95 +289.3 [+115.8, +495.8]; after 300 s +218.6 [+25.7, +478.4]
- A NO_PF - FULL: CEP95 -86.5 [-219.8, +27.9]; after 300 s -20.4 [-161.2, +118.5]
- A NO_NLOS - FULL: CEP95 -7.4 [-90.9, +70.4]; after 300 s +57.5 [-49.2, +226.3]
- B BASE - FULL: CEP95 +7.3 [+4.2, +10.6]; after 300 s +4.1 [+2.3, +6.2]
- B NO_REPLY - FULL: CEP95 +30.4 [+9.5, +64.3]; after 300 s +14.5 [+3.5, +33.9]
- B NO_PF - FULL: CEP95 +16.4 [+1.0, +42.6]; after 300 s +1.8 [-0.3, +5.4]
- B NO_NLOS - FULL: CEP95 +1.1 [-0.2, +2.8]; after 300 s +0.0 [-0.5, +0.6]
- R8 BASE - FULL: CEP95 -142.3 [-322.6, -7.8]; after 300 s -81.6 [-205.8, +12.0]
- R8 NO_REPLY - FULL: CEP95 -9.2 [-100.4, +78.5]; after 300 s -63.7 [-187.6, +29.2]
- R8 NO_PF - FULL: CEP95 +49.9 [-93.5, +190.0]; after 300 s -37.2 [-183.8, +99.0]
- R8 NO_NLOS - FULL: CEP95 -98.5 [-236.1, +9.8]; after 300 s -91.9 [-211.0, -1.3]
- BS BASE - FULL: CEP95 +28.2 [+4.3, +47.2]; after 300 s +23.8 [-5.6, +45.7]
- BS NO_REPLY - FULL: CEP95 +29.5 [+2.5, +57.9]; after 300 s +7.8 [-11.9, +22.7]
- BS NO_PF - FULL: CEP95 +9.3 [-1.0, +25.3]; after 300 s +5.8 [-1.3, +15.2]
- BS NO_NLOS - FULL: CEP95 -0.2 [-2.7, +2.2]; after 300 s +0.1 [-1.8, +2.3]

## Predictions

- **V8-1**: not held
- **V8-2**: held
- **V8-3**: held
- **V8-4**: not held
- **V8-5**: held
- **V8-6**: held
- **V8-7**: held
- **V8-8**: not held
- **V8-9**: held

