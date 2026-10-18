# B4-SWITCHING results — switching under realistic ranging error (registered, exploratory)

Registration: `report/PREREG_B4_SWITCHING.md` at `e1c90578`, committed before any cell. 360 runs, all
`rc 0`, all `source 0dac638f dirty 0`, one compile stamp (`build Oct 16 2026 11:37:16`), frozen binary
with a SHA-256 manifest checked before each cell. Flags recorded in RUN.md before launch:
`acquireReinit=false` (R1a and R1c did not hold), `onDemandR50M=328 onDemandSigmaDb=9.1` (fitted by
`scripts/b4_fit_ondemand.py` to B3F-FOLIAGE's clamped probe).

**Disclosure (run continuity):** the runner was killed at the end of a session with 331 cells complete.
The 29 cells without `rc 0` (12 partial, 17 never started) were deleted and re-run with the same frozen
binary and the same invocations (a resume script that skips `rc 0` cells; noted in RUN.md). No cell was
dropped or chosen; the abort conditions in §5 of the registration were not triggered.

Tracked per cell: `arm_summary.csv`, `run_id.txt` (`results/b4_2026-10-16/`; `rc` is ignored by `.gitignore` and was 0 for all 360). Traces were in
`~/catbl_work/campaigns/b4` (untracked; deleted after this read-out). The analysis
(`scripts/b4_analyse.py`, `scripts/nees_check.py`) is reproduced in §4.

## 1. Verdicts
| # | prediction | outcome |
|---|---|---|
| B4-1 | B: `ca_tbl_full` CEP95 ≤ `static_2p4`, ≥ 3 of 4 presets | **held** (mean ≤ 0 under gottschalk, muller, wolf). Only wolf's interval excludes 0 (−88.6 m [−119.6, −59.1]); engine is +19.3 m [−3.4, +55.1] |
| B4-2 | B: `ca_tbl_full` w25 > `static_868`, all 4 presets | **held** (+0.57, +0.52, +0.22, +0.05; all intervals above 0) |
| B4-3 | on-demand saves tag-0 energy on ≥ 15/20 seeds, ≥ 3 presets | **not held** (0/20 under every preset; see §2) |
| B4-4 | `ca_tbl_full` CEP95 < `fixed_threshold`@12 (sign test), ≥ 3 presets | **not held** (7/19, 4/18 p = 0.031 *against*, 5/13, 8/11). The switching contribution is **not separated** from a trace-only rule |
| B4-5 | A, muller: S1 on raises `static_868` CEP95 | **held** (+1148 m [+778, +1502]; 826 → 1974 m) |
| B4-6 | B: the `static_868` − `static_2p4` gap is smaller under wolf than engine | **held**, and the gap changes sign: +49.0 m (engine) → −78.6 m (wolf) |
| B4-7 | S3: `ca_tbl_full` R95 coverage in B < 0.92, every preset | **held** (0.733, 0.671, 0.433, 0.314) |

## 2. Reading
- **Single-band 2.4 GHz is not always better in the endgame.** Under the wolf preset (Wolf 2019 Table II
  errors), `static_2p4` CEP95 is 149 m against `static_868`'s 70 m in config B. Under engine, gottschalk and
  muller, 2.4 GHz is better or tied. This matches the professor-facing bound, which holds only beyond about
  250 m. Inside a 750 m box, measured SX1280 error at short range is not small enough to beat RSSI.
- **Tiering is robust across presets; single-band 2.4 GHz is not.** In B, `ca_tbl_full` CEP95 spans
  32–61 m over the four presets, while `static_2p4` spans 21–149 m. `ca_tbl_full` is never the worst arm in
  B, but it is not the best everywhere: under engine, `static_2p4` is better (21 vs 41 m). This is the
  strongest B4 evidence for keeping both tiers. The energy cost is large: in B, tag-0 J is 24–48 against
  about 9.6 for `static_2p4`. In A (5 km), `static_2p4` has a lower CEP95 (595–703 m against 1149–1218 m),
  but over only 1.7–2.0 % availability, so its CEP95 there covers a few fix cycles near the anchors.
- **The switching logic itself earns nothing measurable.** `fixed_threshold` (a trace-only variance rule)
  matches `ca_tbl_full` in every cell and beats it under gottschalk (sign test p = 0.031). Because both arms
  range both LoRa tiers each cycle (`--loraDualBase`), the gain comes from ranging both tiers
  opportunistically, not from PCS. This agrees with Stage A.
- **On-demand (B2) almost never sleeps.** The 2.4 GHz radio slept on 186–211 of 14 400 tag-0 cycles in A
  (1.3–1.5 %) and 1 of 14 400 in B. With the wake threshold `onDemandPSkip = 0.05` and five or more
  anchors, P(any 2.4 GHz link) stays above 0.05 almost everywhere. B4-3 fails because the rule rarely fires,
  not because sleeping costs accuracy. A useful test needs a stricter threshold, registered separately.
- **S1 (868 error from correlated shadowing) is the largest single effect.** With the old independent
  truncated-Gaussian error, `static_868` endgame CEP95 was 17 m, better than any 2.4 preset; with S1 it is
  70 m. Earlier results that favoured 868 RSSI at short range rest partly on the optimistic old model.
- **Covariance is overconfident everywhere (S3).** No arm passes the NEES band. `ca_tbl_full` median NEES
  is 3–25 in B and 74–99 in A, with R95 coverage 0.21–0.29 in A. Any rule that acts on Σ (PCS, fixed
  threshold) decides on a quantity that is too small by an order of magnitude in variance. This remains
  the main open defect (master plan S3).

## 3. Consequences
- L.3 "tiering": supported in the robustness sense (no preset breaks it), at a 2.5–5× tag-0 energy cost.
- L.3 "confidence-aware switching (PCS)": **still not supported**; indistinguishable from `fixed_threshold`.
- `--dualBaseOnDemand` stays in the engine, default off; the B4 setting is a recorded null.
- Next registered work: S3 covariance consistency (inflate the 868 RSSI measurement variance to the S1
  error, then re-check NEES), and on-demand with a stricter `onDemandPSkip`.

## 4. Analysis output
### `scripts/b4_analyse.py ~/catbl_work/campaigns/b4`
```
# B4-SWITCHING results (exploratory)

## engine, config A

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 1974.3 | 0.018 | 0.194 | 6.89 | 20 |
| static_2p4 | 664.8 | 0.158 | 0.018 | 14.40 | 20 |
| lora_fusion | 2315.8 | 0.129 | 0.240 | 15.72 | 20 |
| ca_tbl_full | 1217.9 | 0.153 | 0.280 | 17.21 | 20 |
| fixed_threshold | 1151.5 | 0.132 | 0.275 | 15.82 | 20 |
| on_demand | 1270.9 | 0.158 | 0.283 | 17.48 | 20 |

## engine, config B

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 70.4 | 0.308 | 0.373 | 7.64 | 20 |
| static_2p4 | 21.4 | 0.855 | 0.272 | 9.78 | 20 |
| lora_fusion | 59.4 | 0.674 | 0.501 | 11.30 | 20 |
| ca_tbl_full | 40.7 | 0.877 | 0.580 | 24.42 | 20 |
| fixed_threshold | 41.9 | 0.847 | 0.589 | 23.47 | 20 |
| on_demand | 40.7 | 0.877 | 0.580 | 24.42 | 20 |

## gottschalk, config A

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 1974.3 | 0.018 | 0.194 | 6.89 | 20 |
| static_2p4 | 595.3 | 0.133 | 0.020 | 14.24 | 20 |
| lora_fusion | 2308.3 | 0.109 | 0.238 | 16.22 | 20 |
| ca_tbl_full | 1165.4 | 0.160 | 0.273 | 16.70 | 20 |
| fixed_threshold | 1190.7 | 0.183 | 0.276 | 17.51 | 20 |
| on_demand | 1264.8 | 0.172 | 0.274 | 16.60 | 20 |

## gottschalk, config B

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 70.4 | 0.308 | 0.373 | 7.64 | 20 |
| static_2p4 | 47.9 | 0.835 | 0.273 | 9.67 | 20 |
| lora_fusion | 41.7 | 0.665 | 0.497 | 11.04 | 20 |
| ca_tbl_full | 31.9 | 0.832 | 0.578 | 24.29 | 20 |
| fixed_threshold | 28.4 | 0.846 | 0.587 | 25.55 | 20 |
| on_demand | 31.9 | 0.832 | 0.578 | 24.29 | 20 |

## muller, config A

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 1974.3 | 0.018 | 0.194 | 6.89 | 20 |
| static_2p4 | 641.2 | 0.052 | 0.017 | 14.15 | 20 |
| lora_fusion | 2286.5 | 0.055 | 0.243 | 16.25 | 20 |
| ca_tbl_full | 1148.6 | 0.050 | 0.279 | 16.14 | 20 |
| fixed_threshold | 1153.4 | 0.050 | 0.278 | 16.16 | 20 |
| on_demand | 1200.0 | 0.055 | 0.272 | 16.19 | 20 |

## muller, config B

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 70.4 | 0.308 | 0.373 | 7.64 | 20 |
| static_2p4 | 56.9 | 0.523 | 0.250 | 9.49 | 20 |
| lora_fusion | 131.5 | 0.324 | 0.488 | 10.85 | 20 |
| ca_tbl_full | 56.1 | 0.532 | 0.578 | 48.26 | 20 |
| fixed_threshold | 51.1 | 0.545 | 0.582 | 44.95 | 20 |
| on_demand | 56.1 | 0.532 | 0.578 | 48.26 | 20 |

## wolf, config A

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 1974.3 | 0.018 | 0.194 | 6.89 | 20 |
| static_2p4 | 702.9 | 0.000 | 0.017 | 14.63 | 20 |
| lora_fusion | 2181.8 | 0.044 | 0.250 | 15.95 | 20 |
| ca_tbl_full | 1179.5 | 0.048 | 0.268 | 19.52 | 20 |
| fixed_threshold | 1179.5 | 0.048 | 0.268 | 19.52 | 20 |
| on_demand | 1215.5 | 0.048 | 0.271 | 19.54 | 20 |

## wolf, config B

| arm | CEP95 tag0 (m) | w25 (5 nodes) | availability | tag-0 J | n |
|---|---|---|---|---|---|
| static_868 | 70.4 | 0.308 | 0.373 | 7.64 | 20 |
| static_2p4 | 149.1 | 0.019 | 0.248 | 9.58 | 20 |
| lora_fusion | 131.2 | 0.274 | 0.507 | 10.86 | 20 |
| ca_tbl_full | 60.5 | 0.359 | 0.570 | 42.11 | 20 |
| fixed_threshold | 65.6 | 0.361 | 0.561 | 42.62 | 20 |
| on_demand | 60.5 | 0.359 | 0.570 | 42.11 | 20 |

B4-1 engine: CEP95 ca_tbl_full − static_2p4 = +19.25 [-3.38, +55.10] (n 20)
B4-1 gottschalk: CEP95 ca_tbl_full − static_2p4 = -15.99 [-42.54, +3.42] (n 20)
B4-1 muller: CEP95 ca_tbl_full − static_2p4 = -0.77 [-27.06, +22.55] (n 20)
B4-1 wolf: CEP95 ca_tbl_full − static_2p4 = -88.55 [-119.58, -59.08] (n 20)
B4-2 engine: w25 ca_tbl_full − static_868 = +0.57 [+0.54, +0.60]
B4-2 gottschalk: w25 ca_tbl_full − static_868 = +0.52 [+0.49, +0.56]
B4-2 muller: w25 ca_tbl_full − static_868 = +0.22 [+0.17, +0.28]
B4-2 wolf: w25 ca_tbl_full − static_868 = +0.05 [+0.01, +0.09]
B4-3 engine: on-demand uses less tag-0 J on 0/20 seeds; CEP95 40.7 vs 40.7 m
B4-3 gottschalk: on-demand uses less tag-0 J on 0/20 seeds; CEP95 31.9 vs 31.9 m
B4-3 muller: on-demand uses less tag-0 J on 0/20 seeds; CEP95 56.1 vs 56.1 m
B4-3 wolf: on-demand uses less tag-0 J on 0/20 seeds; CEP95 60.5 vs 60.5 m
B4-4 engine: ca_tbl_full CEP95 below fixed_threshold@12 on 7/19 seeds, sign-test p = 0.3593
B4-4 gottschalk: ca_tbl_full CEP95 below fixed_threshold@12 on 4/18 seeds, sign-test p = 0.0309
B4-4 muller: ca_tbl_full CEP95 below fixed_threshold@12 on 5/13 seeds, sign-test p = 0.5811
B4-4 wolf: ca_tbl_full CEP95 below fixed_threshold@12 on 8/11 seeds, sign-test p = 0.2266
B4-5: static_868 CEP95 S1 on − off (muller, A) = +1148.36 [+778.37, +1502.31]
B4-6: static_868 − static_2p4 CEP95 gap in B: engine 49.0 m, wolf -78.6 m

## Predictions

- **B4-1**: held
- **B4-2**: held
- **B4-3**: not held
- **B4-4**: not held
- **B4-5**: held
- **B4-6**: held
```

### `scripts/nees_check.py` (all cells)
| cell | arm | rows | mean NEES | median NEES | R95 coverage | fix rows: mean NEES | fix rows: coverage | verdict |
|---|---|---|---|---|---|---|---|---|
| P_engine_A | static_868 | 56089 | 306.88 | 103.82 | 0.152 | 455.47 | 0.102 | FAIL |
| P_engine_A | static_2p4 | 29550 | 722.65 | 3.09 | 0.575 | 2762.49 | 0.307 | FAIL |
| P_engine_A | lora_fusion | 56997 | 2588.29 | 72.63 | 0.250 | 3486.34 | 0.280 | FAIL |
| P_engine_A | ca_tbl_full | 54647 | 3541.88 | 73.92 | 0.273 | 5480.81 | 0.249 | FAIL |
| P_engine_A | fixed_threshold | 54913 | 3983.81 | 101.40 | 0.238 | 6413.01 | 0.221 | FAIL |
| P_gottschalk_A | static_868 | 56089 | 306.88 | 103.82 | 0.152 | 455.47 | 0.102 | FAIL |
| P_gottschalk_A | static_2p4 | 28083 | 876.09 | 8.02 | 0.513 | 3324.01 | 0.262 | FAIL |
| P_gottschalk_A | lora_fusion | 55892 | 1914.61 | 80.19 | 0.246 | 2856.40 | 0.252 | FAIL |
| P_gottschalk_A | ca_tbl_full | 54169 | 3459.83 | 97.36 | 0.286 | 5861.11 | 0.203 | FAIL |
| P_gottschalk_A | fixed_threshold | 54703 | 3482.57 | 69.33 | 0.315 | 5651.78 | 0.277 | FAIL |
| P_muller_A | static_868 | 56089 | 306.88 | 103.82 | 0.152 | 455.47 | 0.102 | FAIL |
| P_muller_A | static_2p4 | 26233 | 232.49 | 12.18 | 0.478 | 546.59 | 0.202 | FAIL |
| P_muller_A | lora_fusion | 55540 | 1593.23 | 89.51 | 0.178 | 2194.51 | 0.178 | FAIL |
| P_muller_A | ca_tbl_full | 56640 | 459.76 | 99.00 | 0.207 | 730.67 | 0.127 | FAIL |
| P_muller_A | fixed_threshold | 56640 | 468.51 | 98.76 | 0.199 | 754.34 | 0.119 | FAIL |
| P_wolf_A | static_868 | 56089 | 306.88 | 103.82 | 0.152 | 455.47 | 0.102 | FAIL |
| P_wolf_A | static_2p4 | 28633 | 310.33 | 34.83 | 0.341 | 707.39 | 0.051 | FAIL |
| P_wolf_A | lora_fusion | 55287 | 1225.38 | 90.07 | 0.155 | 1350.70 | 0.171 | FAIL |
| P_wolf_A | ca_tbl_full | 55208 | 1020.43 | 95.64 | 0.212 | 1331.54 | 0.131 | FAIL |
| P_wolf_A | fixed_threshold | 55208 | 1020.43 | 95.64 | 0.212 | 1331.54 | 0.131 | FAIL |
| P_engine_B | static_868 | 57483 | 43.17 | 15.54 | 0.448 | 57.54 | 0.371 | FAIL |
| P_engine_B | static_2p4 | 61266 | 29.66 | 1.02 | 0.825 | 55.93 | 0.695 | FAIL |
| P_engine_B | lora_fusion | 60301 | 281.84 | 4.28 | 0.658 | 290.51 | 0.617 | FAIL |
| P_engine_B | ca_tbl_full | 65884 | 128.26 | 3.01 | 0.733 | 169.50 | 0.680 | FAIL |
| P_engine_B | fixed_threshold | 65402 | 245.26 | 3.40 | 0.704 | 306.78 | 0.662 | FAIL |
| P_gottschalk_B | static_868 | 57483 | 43.17 | 15.54 | 0.448 | 57.54 | 0.371 | FAIL |
| P_gottschalk_B | static_2p4 | 62819 | 158.74 | 1.14 | 0.795 | 438.83 | 0.618 | FAIL |
| P_gottschalk_B | lora_fusion | 60991 | 86.33 | 5.15 | 0.634 | 105.04 | 0.594 | FAIL |
| P_gottschalk_B | ca_tbl_full | 65906 | 67.42 | 4.06 | 0.671 | 72.72 | 0.621 | FAIL |
| P_gottschalk_B | fixed_threshold | 66144 | 66.62 | 3.91 | 0.676 | 55.13 | 0.630 | FAIL |
| P_muller_B | static_868 | 57483 | 43.17 | 15.54 | 0.448 | 57.54 | 0.371 | FAIL |
| P_muller_B | static_2p4 | 62986 | 33.54 | 2.30 | 0.709 | 73.09 | 0.447 | FAIL |
| P_muller_B | lora_fusion | 60933 | 100.83 | 19.11 | 0.369 | 143.25 | 0.268 | FAIL |
| P_muller_B | ca_tbl_full | 66681 | 86.87 | 14.65 | 0.433 | 118.33 | 0.307 | FAIL |
| P_muller_B | fixed_threshold | 66263 | 79.46 | 12.20 | 0.461 | 107.95 | 0.350 | FAIL |
| P_wolf_B | static_868 | 57483 | 43.17 | 15.54 | 0.448 | 57.54 | 0.371 | FAIL |
| P_wolf_B | static_2p4 | 62178 | 131.06 | 38.10 | 0.181 | 313.04 | 0.037 | FAIL |
| P_wolf_B | lora_fusion | 59102 | 89.42 | 24.75 | 0.319 | 123.09 | 0.233 | FAIL |
| P_wolf_B | ca_tbl_full | 66492 | 65.52 | 25.46 | 0.314 | 78.04 | 0.258 | FAIL |
| P_wolf_B | fixed_threshold | 66483 | 64.19 | 24.19 | 0.306 | 76.09 | 0.247 | FAIL |
| S1off_muller_A | static_868 | 53364 | 114.82 | 10.43 | 0.541 | 211.11 | 0.414 | FAIL |
| S1off_muller_A | static_2p4 | 29898 | 175.67 | 4.78 | 0.572 | 419.15 | 0.226 | FAIL |
| S1off_muller_A | lora_fusion | 56825 | 211.27 | 13.67 | 0.511 | 374.07 | 0.417 | FAIL |
| S1off_muller_A | ca_tbl_full | 51725 | 301.68 | 23.44 | 0.455 | 433.49 | 0.264 | FAIL |
| S1off_muller_A | fixed_threshold | 51897 | 299.63 | 23.66 | 0.456 | 429.12 | 0.262 | FAIL |
| S1off_muller_B | static_868 | 58722 | 20.69 | 4.34 | 0.699 | 27.69 | 0.629 | FAIL |
| S1off_muller_B | static_2p4 | 61957 | 43.44 | 2.57 | 0.702 | 91.20 | 0.448 | FAIL |
| S1off_muller_B | lora_fusion | 60026 | 86.64 | 9.86 | 0.541 | 122.78 | 0.402 | FAIL |
| S1off_muller_B | ca_tbl_full | 66775 | 44.92 | 8.79 | 0.548 | 55.85 | 0.458 | FAIL |
| S1off_muller_B | fixed_threshold | 66830 | 37.42 | 7.98 | 0.579 | 47.11 | 0.490 | FAIL |

### S1 on/off, preset muller, CEP95 tag 0 (m), mean of 20 seeds
| cell | static_868 | static_2p4 | ca_tbl_full |
|---|---|---|---|
| S1off_muller_A | 826.0 | 669.6 | 500.3 |
| P_muller_A | 1974.3 | 641.2 | 1148.6 |
| S1off_muller_B | 17.2 | 48.7 | 45.1 |
| P_muller_B | 70.4 | 56.9 | 56.1 |

### On-demand sleep counts (`B2 dualBaseOnDemand` line, run.log, 20 seeds)
| cell | 2.4 GHz asleep / tag-0 cycles |
|---|---|
| P_engine_A_OD | 186 / 14400 |
| P_gottschalk_A_OD | 193 / 14400 |
| P_muller_A_OD | 203 / 14400 |
| P_wolf_A_OD | 211 / 14400 |
| P_*_B_OD (each preset) | 1 / 14400 |
