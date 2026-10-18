# PREREG EKF5-MAIN — the 5-state IMU filter on the final preset, with the preset's gyro and a realistic one

**Registered before any cell runs** (rule 13). Written 2026-10-18. Engine:
`work/2026-10-18-ekf5-on-main` at `dee4e969` (main `a89aed8d` with the IMU line merged and the ekf5
wiring for `--acquireParticles`, `--acquireReinit` and `--relayNisInflate`, `f1c13707`, with its
null control; gates G95, G96). This commit adds this file, `scripts/ekf5_main_run.sh` and
`scripts/ekf5_main_analyse.py`, and nothing else. Outputs go outside the repository, to
`~/ns/catbl-work/ekf5_main_2026-10-18/`, which does not exist at registration; the tracked
summaries and sidecars will be copied to `results/ekf5_main_2026-10-18/`.

**Why.** EKF5-PRESET measured the 5-state filter recovering most of the preset's collapse under a
realistic gyro — on the preset of 2026-10-17 at `93695ac`. `main` has since adopted DESIGN_DECISIONS
10–16 (keyed RNG, reply positions, particle acquisition, per-band fading, relay NIS inflation,
log-normal 868 MHz σ, `--imuQuScale=30`), and its campaigns start unknown. None of that was measured
with ekf5. On `main`, CA-TBL's 5 km 95 % ellipse covers 0.69, and 0.50 while coasting (TECH_DEBT
O.12); `--imuQuScale=30` is the CV filter's patch for IMU errors that enter as independent
increments (N.30), which ekf5 models as states instead. And the preset's gyro is still DC-1's
bias instability, 8.816 °/h (N.28), not what a field IMU has. This measures ekf5 on the final
preset at both. It adopts nothing.

## 1. Cells (`ekf5_main_run.sh`)

CORRW's base flags (`--specProfile=true --duration=1800 --trace=true --unknownStart=true
--fixedVarThreshold=12`) plus `--traceCovariance=true`, five arms per invocation (`ca_tbl_full,
fixed_threshold, static_868, static_2p4, poucet_fusion`), A (5 km, `--reportAirtime=true`) and B
(endgame, `--boxHalf=750 --finderMode=1 --reportAirtime=true`):

| axis | values |
|---|---|
| gyro | L0: the preset (8.816 °/h); L2: `--imuGyroBiasSigmaDps=0.15` (the BMI270 data sheet, as DC1-BIAS) |
| filter | cv: the preset; ekf5: `--imuFilter=ekf5` |
| seeds | 1–20 |

2 × 2 × 2 × 20 = **160 cells**, paired by seed. One binary, frozen with a SHA-256 manifest checked
before every cell. `--imuQuScale=30` stays as the preset sets it; ekf5 does not read it (its
process noise is DC-1's assumed model).

## 2. Quantities (`ekf5_main_analyse.py`), `main`'s own estimators

Per (config, level, filter, arm): availability and tag 0 CEP95 (`lora_cmp_analyse.per_seed`),
CEP95 over fix cycles after 300 s (`post_acq_metrics.post_acq`), 95 % coverage and median NEES over
tag 0 and subjects after their first fix and t ≥ 120 s (`v3_analyse.node_rows`, as B2CONS), subject
J. Paired ekf5 − cv by seed (`camp_spec_analyse.ci`). CA-TBL − each comparator, under cv and ekf5.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **P1** | at L2, ekf5 lowers `ca_tbl_full`'s tag 0 CEP95 against cv in A and in B — paired interval entirely below 0 | EKF5-PRESET P1 (old preset): 1976 → 766 m at 5 km, 283 → 44 m endgame at 0.5 °/s |
| **P2** | at L0, ekf5 costs at most 10 %: `ca_tbl_full`'s mean per-seed CEP95 ratio ekf5 / cv − 1 is at most +0.10 in A and in B | EKF5-PRESET P3 held on the old preset |
| **P3** | at L0 in A, ekf5 raises `ca_tbl_full`'s 95 % coverage above cv's | O.12: coasting coverage 0.50; ekf5 carries the bias and scale as states rather than as inflated noise |

**Against these, stated now:** the old preset had no particle acquisition and a known start; the
cv filter here already runs `--imuQuScale=30`, which bought most of O.12's coverage; EKF5-PRESET's
gains came where the cv filter collapsed, and at L0 on this preset it does not collapse; and the
two filters' tag 0 NIS differ by run (G95's endgame ekf5 run at 0.5 °/s read 4.59).

**Reported, not predicted:** every arm's paired change; availability; energy; NEES; whether any
CA-TBL − comparator contrast changes sign or significance between cv and ekf5 (the goal is a
CA-TBL that beats every comparator on accuracy and availability; this is where that is read).

**Decision rule, fixed here.** ekf5 is put to the owner as the preset's filter only if P1, P2 and
P3 all hold. A realistic preset gyro is a separate owner decision this informs.

## 4. Abort conditions

Not 160 cells, or a cell's `rc` not 0; a `run_id.txt` not `dirty 0`, or more than one source; an
ekf5 cell whose log lacks its `N.28 ekf5:` line or a cv cell with one; an L2 cell without its
`DC-1: gyro bias sigma` line or an L0 cell with one; a manifest mismatch; an arm with fewer than 20
seeds in a cell.

## 5. Disclosure

Read before this commit, all on this branch's binary and not citable (600–900 s, single seeds, or
a different arm set): G95's run set (endgame, seed 7, 0.5 °/s, 900 s: ekf5 learned the bias to 5 %
and the heading to 0.021 rad, tag 0 NIS 4.59); G96's (5 km, seed 19, 0.15 °/s, 600 s, mode 8);
four `--ekf5Robust=8` reachability runs (5 km 9 and 19, endgame 7, ring 4; 0.15 °/s, 600 s); one
ekf5 smoke run (5 km, seed 7, 600 s: CEP95 40.7 m); and the N.57 pilot (cv only, seeds 1–6, 1800 s,
`static_868` and `ca_tbl_full`). EKF5-PRESET, EKF5-CONSIDER, CORRW, B2CONS and CAMP-FINAL2 results
were read in full.

## 6. What this cannot establish

Anything about a gyro other than DC-1's model (the filter's assumed model is that model: a best
case for model fit); the ring (C), Wolf ToF errors, or the fault matrix; what the default should be.
