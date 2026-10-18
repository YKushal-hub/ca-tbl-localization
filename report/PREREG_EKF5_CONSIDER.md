# PREREG EKF5-CONSIDER — do Schmidt updates for one-range fixes stop the 5-state filter diverging?

**Registered before any cell runs** (rule 13). Written 2026-10-18. Engine:
`work/2026-10-18-ekf5-consider` at `1e0d9aaa` (`--ekf5Robust`, `61d2f89a`, with its null control;
gate G96). This commit adds this file, `scripts/ekf5_consider_run.sh` and
`scripts/ekf5_consider_analyse.py`, and nothing else. Outputs go to
`results/ekf5_consider_2026-10-18/`, which does not exist at registration.

**Why.** EKF5-PRESET (`report/EKF5_PRESET_RESULTS.md`): the 5-state filter recovers most of the
preset's collapse under a realistic gyro, but a minority of seeds still diverge — every one with a
final heading offset error above 0.3 rad — and they set every mean. Where CA-TBL loses to a
comparator under ekf5 (endgame L3; the ring at L2/L3, by intervals that cross 0) those seeds are
why. TECH_DEBT N.33: at 5 km seed 17 the same filter and IMU end 0.05 rad wrong in `static_2p4`,
which fuses almost nothing, and 1.97 rad wrong in `ca_tbl_full`, which applied 101 single-range
Case D updates; at the ring's seed 4 only `ca_tbl_full` diverged (2.98 rad; every comparator
< 0.01). One range cannot observe heading, bias or scale; `--ekf5Robust=8` stops it trying.
This measures whether that removes the divergence, and what it costs. It adopts nothing.

## 1. Cells (`ekf5_consider_run.sh`)

`--specProfile=true --duration=1800 --trace=true --imuFilter=ekf5`, the six comparator arms per
invocation, configurations A (5 km, `--reportAirtime=true`), B (endgame, `--boxHalf=750
--finderMode=1 --reportAirtime=true`) and C (N8 ring, `--nGateways=8 --gwLayout=legacy`), exactly
as EKF5-PRESET:

| cells | flags | seeds | count |
|---|---|---|---|
| mode 8 × L0 / L2 / L3 | `--ekf5Robust=8`, `--imuGyroBiasSigmaDps` = none / 0.15 / 0.5 | 1–20 | 180 |
| R1: mode 0 × L0 / L2 / L3 | no `--ekf5Robust` | 1 | 9 |

**189 cells.** Mode 0 is EKF5-PRESET's ekf5 cells, paired by seed. That pairing crosses a binary
change (rule 8), so R1 re-runs one mode-0 cell per (config, level) here: it must reproduce
EKF5-PRESET's `arm_summary.csv` byte for byte. Mode 8 applies to every arm's filter (the
setting is in the shared policy configuration), so every arm is measured under it.

## 2. Quantities (`ekf5_consider_analyse.py`), `main`'s own estimators

`lora_cmp_analyse.per_seed` per (config, level, mode, arm): availability, tag 0 CEP95, subjects'
CEP95, subject J; paired mode 8 − mode 0 by seed with `camp_spec_analyse.ci`;
`camp_final_analyse.uwb_fixes`. Each arm's final heading offset error from its `N.28 ekf5:` line
(the line run.log prints before that arm's summary row), wrapped to [0, π]. The comparison
contrasts CA-TBL − `lora_fusion`, `poucet_fusion`, `static_2p4`, `fixed_threshold` at every level,
under mode 0 and mode 8. The considered-update count per `ca_tbl_full` cell.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **R1** | 9 of 9 mode-0 cells reproduce EKF5-PRESET's `arm_summary.csv` | the null control of `61d2f89a`: 14 of 14 byte-identical, two under ekf5 |
| **P1** | over A, B, C × L2, L3 (120 seed-cells), `ca_tbl_full` cells ending with heading error > 0.3 rad are **fewer** under mode 8, and the discordant pairs favour mode 8 **at least 2 : 1** | N.33's mechanism; G96's cell (1.600 → 0.062 rad at 600 s) |
| **P2** | at L2, mode 8 lowers `ca_tbl_full`'s tag 0 CEP95 in A and in C — paired interval entirely below 0 | the same; the 16 disclosed cells below, which are **not** evidence for it |
| **P3** | at L0, mode 8 costs at most 10 %: `ca_tbl_full`'s mean per-seed CEP95 ratio mode 8 / mode 0 − 1 is at most +0.10 in A, B and C | Case D's single ranges still correct position; only the sensor states lose them |

**Against these predictions, stated before running:** of the 16 disclosed cells, the two with the
most considered updates got worse (5 km s9 250.7 → 342.5 m, s19 1461.4 → 2131.1 m), and several
large improvements followed fewer than 11 considered updates, which a chaotic re-roll explains as
well as a cause. A Schmidt update discards information a consistent EKF would use; where the
filter was right, mode 8 can only cost.

**Reported, not predicted:** every arm's paired change at every level; whether any comparison
contrast changes sign or significance; availability and energy; UWB-delivered fixes.

**Decision rule, fixed here.** Mode 8 is put to the owner as part of `--imuFilter=ekf5` only if
R1, P1, P2 and P3 all hold. Any failure: recorded in N.33, not proposed. R1 failing does not
abort, but every paired number is then cross-binary and only populations are compared (rule 8).

## 4. Abort conditions

Not 189 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source;
a mode-8 cell whose log lacks its `N.33 ekf5 mode 8:` line, or a mode-0 cell with one; a cell
without six `N.28 ekf5:` lines paired with six arm rows; a (config, level, mode, arm) with fewer
than 20 seeds.

## 5. Disclosure

Before this commit, from the working tree that became `61d2f89a` (same engine arithmetic,
not citable — the tree was dirty): 16 `ca_tbl_full`-only cells at L2, 1800 s, under modes 8 and
10, of which 12 were chosen **because** they had diverged in EKF5-PRESET (5 km seeds 1, 4, 6, 9,
12, 14, 15, 17, 19; endgame 20; ring 4, 10) and 4 as controls (5 km 2, 13; ring 2, 7). They were
read in full; their numbers are in TECH_DEBT N.33. Also G96's run set (ring seed 4, 600 s) and
the validator's synthetic robustness study. EKF5-PRESET's results were read in full.

## 6. What this cannot establish

Whether a different update rule (first-estimates Jacobians, an observability-constrained EKF, an
unscented update) would do better; anything about a real IMU beyond DC-1's model; what the
default should be.
