# Pre-registration — C3: an inertial dead-reckoning prior in the predict step

Written 2026-10-14. Code under test: commit `9f5085a` (`--imuPrior`). Committed
**before the first cell** of the campaign below; every cell's `run_id.txt` must post-date
this file's commit. Runner `scripts/sweeps/imu_prior_campaign.sh`, scorer
`scripts/imu_prior_analysis.py`, both committed with this file and not edited after the
first cell. A0 v2 (`report/PREREG_A0_V2.md`) is applied to the same cells.

---

## 1. Question

Does a body-worn IMU doing pedestrian dead reckoning, used as the EKF's prior instead of a
constant-velocity extrapolation:

1. reduce position error on cycles where the radio produced no fix;
2. change availability — under the registered `A_fix`, and under the new `A_est`;
3. change escalation behaviour, since every decision rule here reads the filter covariance;
4. and at what energy?

And, as a control on the experiment itself: does switching the IMU on reach anything on
the radio side that it has no business reaching?

**Not asked here:** whether the IMU makes the integrity check a better detector (CAL-05
with an independent prior). That needs the fault-injection grid and is a separate
registration.

## 2. Design

| | |
|---|---|
| configurations | `C_ref` = `--boxHalf=2000`; `C_sparse` = `--boxHalf=3000`; both with `--duration=3600 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true` |
| arms | all six. The IMU, when on, is fitted to **every** arm: giving it only to the tiered arm would manufacture an advantage |
| treatment | `--imuPrior=true` (cond `imu`) against default (cond `cv`), **paired by seed** |
| seeds | **301–330, N = 30.** No tracked sidecar uses a seed above 20; the registrations on record use 1–72. Run exactly; score once; do not extend |
| IMU parameters | defaults, BMI270 where cited, ours where not (`ca-tbl-imu-model.h`): gyro noise 0.007 dps/√Hz, residual turn-on bias 0.15 dps, bias instability 5 °/h τ 300 s, scale 0.4 %, alignment 10°, stride 0.75 m, stride scale 3 %, per-step 5 %, 0.970 mA |
| provenance | optimized build, `dirty 0`, digest block in every sidecar; the runner refuses a dirty tree and any existing output directory |

Why these configurations. `C_ref` is the configuration of the joint-criterion result and of
the project's most soundly-evidenced negative; it is where availability is already ≈ 0.998
and the IMU has least room. `C_sparse` is the same with the anchor field spread wider, where
`static_2p4` loses coverage between sites. One IMU-off control seed at `C_sparse` (§9)
showed `static_2p4` with 206 no-fix cycles in outages up to 26 cycles long and `static_uwb`
with outages up to 689 cycles, while the dynamic arms, which fall back to the tier-1 floor,
had 15–40 no-fix cycles, all single-cycle.

## 3. Population, denominators, window, truncation

Per arm per seed: **all 1440 decision cycles** of a 3600 s run. A **no-fix cycle** is a
trace row with `hadFix = 0`. A **deep** cycle is a no-fix cycle that is at least the 10th
consecutive one (≥ 25 s into an outage).

Truncation: an outage cut off by the end of the run is not extended or dropped. Each cycle
is scored on its own error; its deep/not-deep label depends only on cycles before it, which
are observed. So the window cannot relabel any cycle, and a truncated outage settles every
threshold it has already crossed (working rules 1 and 2).

A per-seed median enters a ratio only if that seed has **≥ 5** qualifying cycles in **both**
conditions. The count of qualifying seeds is reported beside every ratio; a metric with no
qualifying seeds is reported **not testable**, never as zero.

## 4. Metrics and estimators

| id | quantity | estimator |
|---|---|---|
| M1 | median error over no-fix cycles, imu / cv | geometric mean of per-seed ratios, seed-level bootstrap 95 % CI, 10 000 resamples |
| M2 | median error over deep cycles, imu / cv | as M1 |
| M3 | `A_fix` and `A_est`, imu − cv | mean of per-seed differences, same bootstrap |
| M4 | radio energy `totalJ`, imu / cv | as M1 |
| M5 | escalations, imu − cv (dynamic arms) | as M3 |
| M6 | position NEES of the IMU filter in the simulation | mean over seeds, descriptive only |
| R0 | static arms: every `arm_summary.csv` column except the seven error columns, cv vs imu | count of differing row pairs |

The seven error columns are `rmseM cep50M cep95M maxErrM rmseFixedM cep50FixedM
accuracyPerJoule`. No other column of a static arm is permitted to move.

## 5. Predictions and falsifiers

**P1 (control, R0) — exact.** For `static_868`, `static_2p4`, `static_uwb`, every non-error
column is byte-identical between cv and imu, every seed, both configurations: **0 of 180**
row pairs differ.
*Mechanism:* a static arm never escalates; the integrity check runs only in `ca_tbl_full`;
nothing on the radio side reads the estimate; the IMU's generator is private and consumes no
ns-3 stream index. An exact zero is a control-flow claim (rule 4); its reachability is shown
by the IMU summary's `imuPredicts = steps` on these same rows.
*Falsifier:* any differing row. **That aborts interpretation of everything below** — it
would mean the treatment reached the channel, and every cv/imu comparison would carry a
radio component.

**P2 — exact.** Every IMU row has `imuJ = 0.970e-3 × 3.3 × 3600 = 11.523600 J` and
`imuPredicts = steps = 1440`. *Falsifier:* any row off.

**P3 — direction.** M1 < 1, with the CI entirely below 1, for every (configuration, arm)
that has ≥ 10 qualifying seeds.
*Mechanism:* constant velocity extrapolates the last velocity estimate in a straight line;
the walker turns, steers between sites and mills about at sites, so the extrapolation error
grows with the outage. The IMU measures the turns. Its own drift over a 25 s outage at the
nominal parameters is metres (closed forms, `scripts/imu_closed_form_check.py`), against the
tens of metres a 1.5 m/s walker covers.
*Falsifier:* any testable cell with M1 ≥ 1 or a CI reaching 1.

**P4 — magnitude.** M2 ≤ 0.5 (point estimate) for `static_2p4` at `C_sparse`.
*Mechanism:* deep cycles are 25–65 s into outages of a tier-2 arm whose fixes carry ~3–4 m
error. The IMU's added error over that span is ~1–2 m at a converged heading offset
(≈ 1° on ≤ 97 m of path); a straight-line extrapolation over 37–97 m of a steering walker
is tens of metres. A factor of two is a conservative margin on that arithmetic.
*Falsifier:* M2 > 0.5. *Not testable* if fewer than 10 seeds qualify; then it is reported as
such, not scored.

**P5 — direction.** `A_est` rises: M3(`A_est`) > 0 with the CI entirely above 0, for
`static_2p4` and `static_uwb` at `C_sparse`. *Falsifier:* CI reaching or below 0.
Not predicted: any arm at `C_ref`, where `A_est` is already near its ceiling.

**P6 — direction.** Escalations fall: M5 < 0 with the CI entirely below 0, for
`fixed_threshold`, `ca_tbl_no_integrity` and `ca_tbl_full`, in any configuration where that
arm's cv mean is ≥ 10 escalations.
*Mechanism:* `fixed_threshold` escalates on `traceSigma > 100 m²`, and PCS `min`'s first
term is `traceRef / (traceRef + traceSigma)`. A dead-reckoning prior grows the position
covariance more slowly than constant velocity's 1 m²/s per axis, so both rules see a filter
that is more confident between fixes, and request escalation less.
*Falsifier:* a qualifying cell with the CI reaching or above 0.
**The consequence is deliberately not predicted.** Fewer escalations could cost tier-2/3
service and therefore accuracy and `A_fix` on the dynamic arms — an IMU making a
covariance-triggered policy complacent. M3 and M4 on the dynamic arms are reported
two-sided, as findings either way.

**Not predicted, reported:** M3(`A_fix`) on every arm; M4 on dynamic arms; M6. On static
arms M4 = 1 exactly is already implied by P1.

## 6. Decision rule — what each outcome means

| outcome | reading |
|---|---|
| P1 fails | the experiment is contaminated; no other result is interpreted; find the leak |
| P3, P4 hold | the inertial prior does what C3 specifies; the size is the M1/M2 table |
| P3 holds, P4 fails | the prior helps, but less than the error budget implies — a mis-specification to locate before any claim is sized |
| P3 fails | the dead-reckoning prior does not beat constant velocity in this scenario; C3 is built and does not pay here, and that is reported as such |
| P6 holds, with dynamic-arm `A_fix` down | the IMU degrades covariance-triggered escalation; a finding about the decision rules, not about the IMU |

No outcome here is a verdict on the architecture. That is A0 v2's job, on the same cells.

## 7. Abort conditions

- The flag-off null control of `9f5085a` against its parent did not hold → do not run.
- Any directory fails completeness (digest, `dirty 0`, six rows counted, six traces, and
  under imu six IMU rows and six IMU traces): it is **named and excluded**, and the exclusion
  count is reported beside every estimate. If more than 10 % of the 120 are excluded, abort.
- P1 fails → stop interpreting (above).
- Never: extending seeds, re-running a cell to replace it, or editing the scorer after the
  first cell. Any deviation is reported as a deviation.

## 8. Secondary sensitivity — registered now, run after the primary

The residual turn-on bias is the dominant heading error and rests on an assumption of ours
(a 10 K temperature change after calibration). At `C_sparse`, arms `static_2p4` and
`ca_tbl_full` only, seeds 301–310: `--imuGyroBiasInitDps` ∈ {0.05, 0.5} beside the nominal
0.15 already in the primary. 0.5 dps is the data sheet's uncalibrated zero-rate offset, the
worst case. Reported as M1/M2 against bias, **no verdict**.

## 9. Disclosure — what was looked at before this was written

- **G17's synthetic filter test** (sensor model + `CaTblImuEkf`, 4 anchors, σ = 2 m, 20-cycle
  outage blocks): mean NEES 2.021 on fix cycles and 2.057 in outages; without the
  stride-scale state 24.6 / 31.0. Not the scenario.
- **One smoke run** of the scenario with the IMU on: seed 99, 600 s, `static_2p4` only, `C_ref`
  flags — to confirm the mechanism executes. Read: 240 predicts in 240 cycles, NEES 1.92,
  4.2 % above 5.991, final heading-offset error −0.14°, `imuJ` 1.9206 J. No error or
  availability figure from it was compared with anything.
- **IMU-off control runs at seed 3** for `C_ref` and `C_sparse` (null-control references).
  At `C_sparse`: no-fix cycles `static_868` 2, `static_2p4` 206, `static_uwb` 1382,
  `fixed_threshold` 40, `ca_tbl_no_integrity` 15, `ca_tbl_full` 15; longest outage 26 cycles
  on `static_2p4`, 689 on `static_uwb`, 1 on the dynamic arms; `static_2p4` median no-fix
  error 5.8 m. These sized §2 and §5 and are control-condition data only.
- **The runner and the scorer were exercised end to end** on seeds 401–402 at 300 s, both
  configurations, both conditions, written to `/tmp` and not kept. Read: completeness 8/8;
  R0 0 of 12 static-arm row pairs differing; P2 flagging 24 of 24 rows — correctly, since
  `imuJ` at 300 s is 0.96 J and not 11.52 J, which incidentally shows that check firing. The
  M1–M6 and A0 v2 sections were produced and **not read**.
- **No IMU-on run at either campaign configuration at 3600 s, and no run at any seed in
  301–330, was made before this file's commit.**

## 10. What this cannot establish

- Anything about real hardware. The sensor is a model; its dominant term rests on our 10 K.
- Anything with a magnetometer, zero-velocity updates, or a stationary dwell — the scenario
  has none (`ca-tbl-imu-model.h`, scope limits).
- Whether the IMU improves the integrity check (§1; separate registration).
- MCU energy for running PDR, which is not charged, matching the energy model's scope.
