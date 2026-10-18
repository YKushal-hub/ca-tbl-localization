# PREREG REAL-IMU — the final preset with a realistic IMU: which filter, and does the acquisition guard help?

**Registered before any cell runs** (rule 13). Written 2026-10-18. Engine: `work/2026-10-18-ekf5-on-main` at
`7c2cf440` (`--imuStrideSigmaPct`, `aa538bb8`; `--acquireHandoverCurvature`, `bbfc0de8`; both with their null
control; gates G97, G98). This commit adds this file, `scripts/real_imu_run.sh` and `scripts/real_imu_analyse.py`,
and nothing else. Outputs go outside the repository, to `~/ns/catbl-work/real_imu_2026-10-18/`, which does not exist
at registration; summaries and sidecars will be copied to `results/real_imu_2026-10-18/`.

**Why.** Every headline on `main` is measured with DC-1 at the MPU-6050's bias *instability* (8.816 °/h) as its
turn-on bias, and with a stride that is exact up to one per-run scale error. Both are better than any IMU a
searcher or casualty carries (TECH_DEBT N.28; `aa538bb8`). EKF5-MAIN showed the realistic gyro alone moves the
endgame's CEP95 from 5 m to 130 m under the preset's filter, and that the 5-state filter recovers it; it also showed
that filter collapsing at 5 km through an acquisition handover far from the truth (N.58), which
`--acquireHandoverCurvature` addresses. This measures the preset with a realistic IMU — a BMI270-class gyro
(0.15 °/s, as DC1-BIAS's L2) and 5 % per-step stride noise (the value is ours, inside the "few percent" of Jahn et
al. 2010 and Harle 2013) — under both filters and with and without the guard. It adopts nothing.

## 1. Cells (`real_imu_run.sh`)

CORRW's base flags (`--specProfile=true --duration=1800 --trace=true --unknownStart=true --fixedVarThreshold=12`),
`--traceCovariance=true`, `--imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5`, five arms (`ca_tbl_full,
fixed_threshold, static_868, static_2p4, poucet_fusion`), A (5 km) and B (endgame, `--boxHalf=750 --finderMode=1`),
both `--reportAirtime=true`; filter cv / ekf5 (`--imuFilter=ekf5`); guard off / on (`--acquireHandoverCurvature`);
seeds 1–20. 2 × 2 × 2 × 20 = **160 cells**, one frozen binary checked before every cell. The guard and the filter
apply to every arm, so the comparators run under the same conditions.

## 2. Quantities (`real_imu_analyse.py`), `main`'s own estimators

As EKF5-MAIN: availability and tag 0 CEP95 (`lora_cmp_analyse.per_seed`), CEP95 over fix cycles after 300 s
(`post_acq`), 95 % coverage and median NEES (`v3_analyse.node_rows`), subject J; paired by seed with
`camp_spec_analyse.ci`; CA-TBL − each comparator under ekf5 with the guard.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **P1** | ekf5 lowers `ca_tbl_full`'s tag 0 CEP95 against cv (guard off) in A and in B — paired interval below 0 | EKF5-MAIN P1 (gyro alone); the pilot below |
| **P2** | under ekf5, the guard lowers `ca_tbl_full`'s CEP95 in A (interval below 0), and in B costs at most 10 % (mean per-seed ratio − 1 ≤ +0.10) | N.58's traced mechanism; the pilot below |
| **P3** | under ekf5 with the guard, `ca_tbl_full`'s availability exceeds every comparator's in A and in B — paired interval above 0 | CA-TBL led availability in every EKF5-MAIN cell |
| **P4** | under ekf5 with the guard, CA-TBL − `poucet_fusion` CEP95: interval below 0 in A, **above 0 in B** (Poucet more accurate in the endgame) | the pilot below |

**Against them, stated now:** the pilot is 6 and 4 seeds and two of its 5 km seeds failed under every variant
(seed 3: a 268° gyro-heading drift that ekf5's locked bias estimate cannot follow; neither filter has a heading
reference). The guard made Poucet worse on one pilot seed (5 km seed 3, 215 → 894 m).

**Reported, not predicted:** every arm's paired change; coverage and NEES; energy; the guard under cv.

**Decision rule, fixed here.** P1 → ekf5 is put to the owner as the filter for a realistic IMU. P2 → the guard is
put to the owner for the preset. Whether the preset's IMU becomes realistic is the owner's decision; this informs it.

## 4. Abort conditions

Not 160 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source; a manifest
mismatch; a cell whose filter, guard or realistic-IMU lines do not match it; an arm with fewer than 20 seeds.

## 5. Disclosure

Read before this commit, exploratory, from working trees that were not citable (dirty, single or two arms):
the realistic-IMU pilot (5 km seeds 1–6, endgame 1–4; `ca_tbl_full` and `poucet_fusion`): cv 785.5 / 128.1 m,
ekf5 360.9 / 6.1 m, ekf5 with the guard 194.4 / 4.5 m; Poucet under ekf5 136.0 / 2.1 m, with the guard 249.2 / 2.3 m.
Also every N.58 diagnostic (ekf5 on the preset's gyro, seeds 1–6: `--rssiCorrInflate`, `--ekf5Robust=4/16`, an
iterated update, `--acquireNegInfo`), the seed-5 and seed-3 traces, and G97/G98's run sets. EKF5-MAIN's results
were read in full.

## 6. What this cannot establish

Anything about a real BMI270 beyond DC-1's model with these two parameters; whether a magnetometer or other heading
reference would remove the 5 km heading failures (none is modelled); the ring; Wolf ToF; the default.
