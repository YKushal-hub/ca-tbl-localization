# PREREG EKF5-PRESET — does a filter that learns the gyro's errors recover the preset?

**Registered before any cell runs** (rule 13). Written 2026-10-17. Engine: `work/2026-10-17-imu-filter`
at `52dc7a02` (`--imuFilter=ekf5`, `86e376cf`, with its null control; gate G95). This commit adds
this file, `scripts/ekf5_preset_run.sh` and `scripts/ekf5_preset_analyse.py`, and nothing else.
Outputs go to `results/ekf5_preset_2026-10-17/`, which does not exist at registration.

**Why.** DC1-BIAS (`report/DC1_BIAS_RESULTS.md`, TECH_DEBT N.28): with a realistic gyro bias every
arm's accuracy on the preset collapses — CA-TBL tag 0 CEP95 57.8 → 1976.4 m at 5 km and 12.0 →
283.0 m in the endgame at 0.5 °/s — because DC-1's filter has no heading, bias or scale state and
DC-1's heading error is never corrected. `--imuFilter=ekf5` runs the 5-state filter on the same
IMU output. This measures what it recovers, and what it costs where the gyro is near-perfect. It
adopts nothing: the default stays `cv` unless the owner decides otherwise.

## 1. Cells (`ekf5_preset_run.sh`)

The preset, `--duration=1800 --trace=true`, the six comparator arms per invocation, configurations
A (5 km, `--reportAirtime=true`), B (endgame, `--boxHalf=750 --finderMode=1 --reportAirtime=true`)
and C (N8 ring, `--nGateways=8 --gwLayout=legacy`), exactly as D8-MAIN and DC1-BIAS:

| cells | flags | seeds | count |
|---|---|---|---|
| ekf5 × L0 / L2 / L3 | `--imuFilter=ekf5`, and `--imuGyroBiasSigmaDps` = none / 0.15 / 0.5 | 1–20 | 180 |
| R1: cv × L0 / L2 / L3 | `--imuGyroBiasSigmaDps` = none / 0.15 / 0.5 (no `--imuFilter`) | 1 | 9 |

**189 cells.** The `cv` comparison is DC1-BIAS's cells (`4739849b`), paired by seed. That pairing
crosses a binary change (rule 8), so R1 re-runs one `cv` cell per (config, level) on this binary:
it must reproduce DC1-BIAS's `arm_summary.csv` byte for byte.

## 2. Quantities (`ekf5_preset_analyse.py`), `main`'s own estimators

`lora_cmp_analyse.per_seed` per (config, level, filter, arm): availability, tag 0 CEP50 / CEP95,
subjects' CEP95, TTFF, subject J; `camp_final_analyse.uwb_fixes` for `ca_tbl_full`; paired
ekf5 − cv by seed with `camp_spec_analyse.ci`. And the comparison table's contrasts, CA-TBL −
LoRa fusion and CA-TBL − Poucet, under ekf5 at every level.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **R1** | 9 of 9 `cv` cells reproduce DC1-BIAS's `arm_summary.csv` | the null control of `86e376cf`: summaries identical in 12 of 12 reference cases |
| **P1** | at L3, ekf5 lowers `ca_tbl_full`'s tag 0 CEP95 against cv in A and in B — paired interval entirely below 0 | the filter estimates the bias and heading offset DC-1's filter cannot; G95's run |
| **P2** | in B, ekf5 at L3 recovers at least 80 % of the loss: its mean tag 0 CEP95 is at most 12.0 + 0.2 × (283.0 − 12.0) = **66.2 m** | the smoke run (B, L3, seed 7): 24.5 m |
| **P3** | at L0, ekf5 costs at most 10 % in accuracy: `ca_tbl_full`'s mean per-seed CEP95 ratio ekf5 / cv − 1 is at most +0.10 in A, B and C | with a near-perfect gyro there is little to learn; the states add uncertainty, not error |

**Reported, not predicted:** every arm's paired change at every level; availability and energy;
the comparison contrasts under ekf5 against those under cv (DC1-BIAS); UWB-delivered fixes.

**R1 failing does not abort**, but every paired number is then a cross-binary comparison, stated
as such, and only populations are compared (rule 8).

## 4. Abort conditions

Not 189 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source; an
ekf5 cell whose log lacks its `N.28 ekf5:` lines, or a cv cell with one; a (config, level, arm)
with fewer than 20 seeds; DC-1's drawn bias in an ekf5 cell not equal to its DC1-BIAS twin's.

## 5. Disclosure

Before this commit: the G95 run (B, L3, seed 7, 900 s) and the smoke run it came from (same
configuration, 900 s): bias est −4.48e-4 against −4.71e-4 rad/s true, tag 0 CEP95 195.3 m (cv) →
24.5 m (ekf5). Both at seed 7, which is inside 1–20 at 900 s rather than 1800 s; disclosed, not
data. DC1-BIAS's results were read in full before this was written.

## 6. What this cannot establish

What the default should be; anything about a real BMI270 or MPU-6050 beyond DC-1's error model
(the filter's assumed model *is* that model, so this is a best case for model fit); scale-error
or noise-density misspecification.
