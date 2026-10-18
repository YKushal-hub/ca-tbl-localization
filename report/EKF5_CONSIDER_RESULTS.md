# EKF5-CONSIDER results — Schmidt updates for one-range fixes make the 5-state filter worse

Registration: `report/PREREG_EKF5_CONSIDER.md` (`672ecebf`, before any cell). 189 cells, all `rc 0`,
`source 672ecebf dirty 0`. Scorer output: `results/ekf5_consider_2026-10-18/EKF5_CONSIDER.md`.
How the directory was made, including an operator stop and resume: its `RUN.md`.

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| R1 | 9 mode-0 cells reproduce EKF5-PRESET | **held**, 9 of 9 identical |
| P1 | `ca_tbl_full` heading divergence (> 0.3 rad) fewer over A/B/C × L2/L3, discordant pairs ≥ 2 : 1 | **not held**: 19 → 15 of 120, discordant 11 against 7 |
| P2 | at L2, CEP95 lower in A and in C (interval below 0) | **not held**: A +16.7 [−234.3, +293.5] m, C −75.0 [−365.3, +99.1] m |
| P3 | at L0, mean per-seed CEP95 ratio − 1 ≤ +0.10 in A, B, C | **not held**: A **+1.057**, B +0.006, C −0.013 |

**Decision rule (registered): not proposed.** `--ekf5Robust=8` stays a default-off diagnostic mode.

## 2. What it shows

- **The one-range updates carry information the sensor states need.** Withholding it from θ, b, s
  raises CA-TBL's 5 km CEP95 where the gyro is near-perfect — mean 28.1 → 40.6 m (Δ +12.6
  [+0.5, +31.5], +44 %), mean per-seed ratio +105.7 %, so a few seeds more than double — and the subjects' CEP95 rises almost everywhere at L2 and L3, for every arm
  (5 km CA-TBL subjects +398.8 [+256.6, +550.8] m at L2).
- **It is worst where two-range updates dominate.** In the ring, `static_2p4`'s heading divergences
  go 3 → 10 at L2 and **3 → 18** at L3 (CEP95 317.1 → 868.6 m).
- **The 16-cell pilot was regression to the mean**, as the registration warned. The cells chosen
  for having diverged improved under any perturbation; the unselected population does not.
- **The divergence is not caused by sparse updates as such.** TECH_DEBT N.33's mechanism — a single
  range teaching θ through a wrong linearisation — happens, but the same updates teach θ correctly
  far more often. A remedy has to tell the two apart (consistency of the update, not its rank).

## 3. The comparison table under mode 8 (CA-TBL − comparator, tag 0 CEP95, paired; m0 / m8)

- 5 km: CA-TBL beats LoRa fusion and Poucet at every level under both modes.
- Endgame L3: CA-TBL still worse than LoRa fusion (+34.4 / +30.7) and Poucet (+36.6 / +32.8).
- Ring L2: the ring's s4 outlier moved; intervals still cross 0 under both.

## 4. What this cannot establish

Whether an update rule that is consistent rather than rank-limited (first-estimates Jacobians,
an observability-constrained EKF, an iterated or unscented update) would remove the divergence;
anything on `main` after `a89aed8d`, whose preset differs (DESIGN_DECISIONS 10–16).
