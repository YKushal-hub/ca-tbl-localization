# EKF5-MAIN results — the 5-state filter wins under a realistic gyro and loses at 5 km under a near-perfect one

Registration: `report/PREREG_EKF5_MAIN.md` (`b26d0c85`, pushed before any cell). 160 cells, all `rc 0`,
`source b26d0c85 dirty 0`, one frozen binary checked before every cell; launched 2026-10-18T12:19:10Z,
done 12:51:05Z. Scored once, unedited, "Problems: 0". Tracked: `results/ekf5_main_2026-10-18/`
(arm and subject summaries, sidecars, the scorer's `EKF5_MAIN.md`; traces not tracked).

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| P1 | at L2 (0.15 °/s), ekf5 lowers CA-TBL tag 0 CEP95 in A and B | **held**: A −666.6 [−1157.7, −237.9] m (1692.1 → 1025.5), B −124.4 [−183.9, −77.6] m (130.1 → 5.8) |
| P2 | at L0 (the preset's gyro), mean per-seed CEP95 ratio − 1 ≤ +0.10 in A and B | **not held**: A **+1.795** (365.1 → 613.7 m), B −0.222 (5.0 → 3.8 m) |
| P3 | at A L0, ekf5 raises CA-TBL's 95 % coverage | **not held**: 0.578 → **0.278**, median NEES 4.5 → **789** |

**Decision rule (registered): not proposed as the preset's filter.**

## 2. What it shows

- **With a realistic gyro ekf5 is decisively better, for every arm** (A L2 and B L2, all five arms' paired
  CEP95 below 0 or spanning it for `static_868`). The endgame CA-TBL CEP95 at 0.15 °/s falls 130.1 → 5.8 m.
  The preset's own figures (CAMP-FINAL2, CORRW) are conditional on DC-1's bias instability, 8.816 °/h; at
  a BMI270's 0.15 °/s the CV preset's endgame is 130 m, not 5 m.
- **At 5 km with the near-perfect gyro ekf5 is grossly overconfident** (NEES 789, every arm affected:
  `poucet_fusion` 4.3 → 18.7, `fixed_threshold` 3.0 → 483.6). In the endgame it is consistent (2.3).
- **The mechanism, traced on A L0 seed 5 (exploratory, an environment-gated debug print not in any
  committed binary; the run reproduced the cell's CEP95, 505.36 m):** after the acquisition cloud hands
  over (σ ≈ 164 m at ≈ 230 m error), tag 0's claimed position σ collapses 178 → 8.3 m between t = 82 and
  122 s while its error is still 213 m; the fixes then pull the estimate to within 3 m by t = 282 s, but
  with position "known", the persistent innovations are absorbed as dynamics: stride scale 0 → +0.53
  (a 20σ move against its prior) and heading offset → −1.05 rad, after which it dead-reckons the wrong
  way (error 549 m at t = 882 s). The CV filter has no sensor states to absorb it into, and
  `--imuQuScale=30` keeps its covariance open. TECH_DEBT N.58.

## 3. CA-TBL against its comparators (tag 0 CEP95, CA-TBL − comparator, paired; cv / ekf5)

| config, gyro | fixed_threshold | static_868 | static_2p4 | poucet_fusion |
|---|---|---|---|---|
| A, L0 | +3.4 / −69.9 | −82.6 / +47.6 | −37.7 / +176.2 | +21.0 / −166.7 |
| A, L2 | +142.9 / +42.5 | −179.3 / −686.5 | −24.3 / −175.4 | +340.3 / +295.9 |
| B, L0 | +0.4 / +0.4 | −22.5 / −21.3 | −7.3 / +0.4 | +0.7 / **+1.6 [0.2, 3.2]** |
| B, L2 | −6.6 / −3.2 | −247.2 / −191.1 | +36.9 / −2.4 | −7.7 / **+3.2 [1.5, 5.1]** |

CA-TBL has the highest availability in every cell. The intervals are in `EKF5_MAIN.md`. Under ekf5 at
the realistic gyro, the endgame CA-TBL (5.8 m) beats `fixed_threshold` and ties `static_2p4`; Poucet
(2.6 m) is lower at 17× the energy.

## 4. What this cannot establish

Anything outside DC-1's gyro model (the filter's assumed model is that model); the ring; Wolf ToF;
which remedy fixes the 5 km collapse — candidates are listed in N.58, none is measured here.
