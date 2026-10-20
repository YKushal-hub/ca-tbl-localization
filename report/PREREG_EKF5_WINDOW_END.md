# PREREG EKF5-WINDOW-END — the window smoother in the endgame, out of sample on seeds 81–100

**Registered before any cell runs** (rule 13). Written 2026-10-20. Engine: `work/2026-10-20-tech-debt` at `032b706a`.
The engine is unchanged since `762bad9c` (`--ekf5Window`, null control 12/12, G152). This commit adds this file,
`scripts/ekf5_window_end_run.sh` and `scripts/ekf5_window_end_analyse.py`, and nothing else. Outputs go outside the
repository, to `~/ns/catbl-work/ekf5_window_end_2026-10-20/`, which does not exist at registration.

**Why.** EKF5-WINDOW (`report/EKF5_WINDOW_RESULTS.md`) failed its decision rule at 5 km. In the endgame it gave an
accuracy gain it had not predicted: CA-TBL tag 0 CEP95 4.6 → 3.1 m [−3.3, −0.2], coverage 0.89 → 0.96, and every
policy arm improved. An unpredicted effect is a hypothesis, not a result. This registers it on seeds no run in this
line has used.

## 1. Cells (`ekf5_window_end_run.sh`)

EKF5-WINDOW's cells, endgame only:
- **Stack:** REAL-BEST's S stack in the realistic world.
- **Config:** B (`--boxHalf=750 --finderMode=1 --reportAirtime=true`).
- **Arms:** all five.
- **Variants:** **S**, and **W** = S + `--ekf5Window=60`.
- **Seeds:** 81–100.
- **Total:** 2 × 20 = **40 cells**, one frozen binary checked before every cell.

## 2. Quantities (`ekf5_window_end_analyse.py`)

EKF5-WINDOW's estimators, restricted to B.

## 3. Predictions, made before any cell

| # | prediction | basis (EKF5-WINDOW, B, seeds 61–80) |
|---|---|---|
| **P1** | W lowers CA-TBL's tag 0 CEP95 against S: interval below 0 | −1.5 [−3.3, −0.2] |
| **P2** | under W, CA-TBL's 95 % coverage is at least 0.93 | 0.961 |
| **P3** | W costs CA-TBL no availability: lower bound above −0.01 | +0.007 [−0.004, +0.020] |
| **P4** | under W, CA-TBL − Poucet: CEP95 interval contains 0, and availability interval above 0 | +0.1 [−0.2, +0.5]; +0.028 [0.014, 0.042] |
| **P5** | W lowers `fixed_threshold`'s CEP95 against S: interval below 0 | −1.5 [−3.1, −0.3] |

**Against them, stated now:** P1's basis interval ends at −0.2 m, close to 0. In the single-arm 600 s runs (seeds
1–6) the window's endgame p95 was worse than the EKF's on 2 of 6 seeds. The endgame floor is about 4 m CEP95 at 2 m
finder GNSS error (N.61), and EKF5-WINDOW's 3.1 m is already below it. So a smaller gain on new seeds would not
surprise.

**Decision rule.** P1 and P3 both hold: `--ekf5Window=60` is put to the owner for the endgame, with the
realistic-world preset. P2, P4 and P5 are reported either way. Its 5 km result stays as registered (not
supported), and will be stated beside any adoption.

## 4. Abort conditions

- not 40 cells, or a cell's `rc` not 0;
- a `run_id.txt` not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 81–100;
- a variant or realistic-world line that does not match its cell;
- any window solver failure;
- an arm with fewer than 20 seeds.

## 5. Disclosure

Seeds 81–100 have not been run by this line. Read before this commit: everything EKF5-WINDOW's disclosure lists, and
its results in full.

## 6. What this cannot establish

The 5 km behaviour (EKF5-WINDOW's); the window length; real sensors; the ring; Wolf ToF; compute cost on a device.
