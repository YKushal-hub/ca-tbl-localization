# PREREG MAG — does a magnetometer heading reference fix the realistic-IMU preset's heading failures?

**Registered before any cell runs** (rule 13). Written 2026-10-18. Engine: `work/2026-10-18-magnetometer` at `c9248c77`
(`--imuMagSigmaDeg`, `937e8735`, with its null control; gate G99), which also carries REAL-IMU's results. This
commit adds this file, `scripts/mag_run.sh` and `scripts/mag_analyse.py`, and nothing else. Outputs go outside the
repository, to `~/ns/catbl-work/mag_2026-10-18/`, which does not exist at registration.

**Why.** REAL-IMU (`report/REAL_IMU_RESULTS.md`): with a realistic IMU, ekf5 is required, and its remaining 5 km
failures are heading-driven. Its gyro-only heading error grows without bound (seed 5: 1.87 rad), and DC-1 models no
heading reference, though body-worn IMUs carry a magnetometer. This measures one at 3° white noise plus a 3°,
120 s Gauss-Markov disturbance (all ours; outdoor magnetometer heading errors are a few degrees, Afzal, Renaudin,
Lachapelle 2011). It adopts nothing.

## 1. Cells (`mag_run.sh`)

REAL-IMU's ekf5 cells without the guard, exactly: `--specProfile=true --duration=1800 --trace=true
--traceCovariance=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5
--imuFilter=ekf5`, five arms, A (5 km) and B (endgame), with `--imuMagSigmaDeg=3`, seeds 1–20: 40 cells; and R1, the
same without the magnetometer at seed 1 in A and B: 2 cells. **42 cells**, one frozen binary. Paired by seed with
REAL-IMU's `A_ekf5_g0` / `B_ekf5_g0` cells (`results/real_imu_2026-10-18/`); that pairing crosses a binary change
(rule 8), which R1 checks.

## 2. Quantities (`mag_analyse.py`)

`lora_cmp_analyse.per_seed` (availability, tag 0 CEP95), `camp_spec_analyse.ci`, `v3_analyse.node_rows` coverage;
each arm's final heading offset error from its `N.28 ekf5:` line; CA-TBL − each comparator with the magnetometer.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **R1** | 2 of 2 no-magnetometer cells reproduce REAL-IMU's `arm_summary.csv` | the null control of `937e8735` |
| **P1** | the magnetometer lowers `ca_tbl_full`'s tag 0 CEP95 in B — paired interval below 0 | the pilot: endgame seeds 1–4, 6.1 → 2.5 m |
| **P2** | fewer `ca_tbl_full` seeds end with a heading offset error > 0.3 rad, A and B pooled (40 seed-cells) | G99's cell (1.869 → 0.074 rad); the pilot |
| **P3** | with the magnetometer, CA-TBL − `poucet_fusion` CEP95 in B has an interval upper bound below +1.0 m | the pilot: 2.5 against 2.2 m |

**Against them, stated now:** on 8 pilot seeds at 5 km the magnetometer made CA-TBL worse on three (seed 3 758 →
1535 m with a final heading error of 2.24 rad *despite* the reference; seeds 2 and 8) and better on three, a mean
of 323.5 → 336.2 m. The range innovations can still drive the heading state against the reference. The
independent-equivalent variance makes each reading weak (about 29° with the correlated part inflated).

**Reported, not predicted:** A's CEP95 for every arm; availability; coverage; energy; the comparison contrasts.

**Decision rule.** P1 and P2 → the magnetometer is put to the owner as part of a realistic preset. Otherwise it
stays a default-off option.

## 4. Abort conditions

Not 42 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source; a manifest
mismatch; a cell whose magnetometer line does not match it; an arm with fewer than 20 seeds.

## 5. Disclosure

Read before this commit, exploratory and not citable: the pilot (5 km seeds 1–8, endgame 1–4; three arms;
magnetometer alone and with `--acquireHandoverCurvature`); an adaptive fading factor (strong tracking), which
diverged catastrophically and was removed (5 km seed 5 at 402 km); G99's run set. REAL-IMU's results were read in full.

## 6. What this cannot establish

A real magnetometer's error (the disturbance model is ours), indoor or near-metal operation, the ring, Wolf ToF,
the default.
