# PREREG EKF5-WINDOW — the sliding-window smoother on the realistic stack, out of sample on seeds 61–80

**Registered before any cell runs** (rule 13). Written 2026-10-20. Engine: `work/2026-10-20-tech-debt` at `762bad9c`
(`--ekf5Window`, default off, with its null control, 12/12 byte-identical, and gate G152). This commit adds this file,
`scripts/ekf5_window_run.sh` and `scripts/ekf5_window_analyse.py`, and nothing else. Outputs go outside the repository,
to `~/ns/catbl-work/ekf5_window_2026-10-20/`, which does not exist at registration.

**Why.** TECH_DEBT N.62: at 5 km tag 0 often has one precise anchor. Single EKF updates turn range innovations into
heading, bias and scale errors the magnetometer cannot pull back. On seed 5 the heading ended 37° off, the stride
scale −0.25, and the error ~400 m at a claimed σ of 3–8 m. Every single-update remedy has been measured and rejected:
- θ owned by the magnetometer;
- state bounds (REAL-BEST P5);
- one-range consider updates (EKF5-CONSIDER);
- the iterated update (N.58).
An offline replay and an exploratory in-scenario pilot say a trajectory-level smoother recovers it. This tests that
on seeds never run in this line.

## 1. Cells (`ekf5_window_run.sh`)

Every cell runs REAL-BEST's S stack, the realistic world:

    --specProfile=true --duration=1800 --trace=true --traceCovariance=true --unknownStart=true --fixedVarThreshold=12
    --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3
    --ekf5FinderConsider=true --ekf5RelayGate=true

- **Arms:** all five.
- **Configs:** A (5 km) and B (endgame, `--boxHalf=750 --finderMode=1`), both `--reportAirtime=true`.
- **Variants:** **S**, the stack as given, and **W**, S + `--ekf5Window=60` (150 s).
- **Seeds:** 61–80.
- **Total:** 2 × 2 × 20 = **80 cells**, one frozen binary checked before every cell. The window applies to every
  arm's filter, so the comparators run under the same conditions.

## 2. Quantities (`ekf5_window_analyse.py`), the REAL-* estimators

- availability and tag 0 CEP95 (`lora_cmp_analyse.per_seed`);
- 95 % coverage and median NEES (`v3_analyse.node_rows`);
- subject J;
- W − S paired by seed (`camp_spec_analyse.ci`), and CA-TBL − each comparator under W.

## 3. Predictions, made before any cell

| # | prediction | basis (exploratory pilot, seeds 1–6, `~/ns/catbl-work/win_pilot`) |
|---|---|---|
| **P1** | W lowers CA-TBL's tag 0 CEP95 against S in A: interval below 0 | −188.7 [−456.3, −11.5]; mean 216.2 → 27.5 m |
| **P2** | under W, CA-TBL's 95 % coverage in A is at least 0.90 | 0.419 → 0.972 |
| **P3** | W raises CA-TBL's availability against S in B: interval above 0 | +0.015 [0.003, 0.031] |
| **P4** | W costs CA-TBL no significant availability in A: interval contains 0 or is above it | −0.014 [−0.062, 0.026] |
| **P5** | under W, CA-TBL − Poucet CEP95 is not significantly above 0, in A and in B | A −36.0 [−97.1, 1.1]; B 0.0 [−0.6, 0.6] |

**Against them, stated now:**
- Seed 6 was worse under W in every look at it (pilot 18 → 65 m at 5 km). New seeds bring new seed 6s.
- In the pilot, `fixed_threshold` under W was slightly MORE accurate than CA-TBL at 5 km (+2.7 [0.1, 5.7] m). No
  prediction claims CA-TBL is the most accurate arm.
- `static_868` does not benefit: its 868 MHz-only ranges give the window nothing precise to resolve.
- In the endgame, the window's tag-0 p95 was worse than the EKF's on 2 of 6 single-arm 600 s runs.

**Decision rule.** P1, P3 and P4 all hold: `--ekf5Window=60` is put to the owner together with the realistic-world
preset. P2 and P5 are reported either way. W is never adopted on accuracy alone.

## 4. Abort conditions

- not 80 cells, or a cell's `rc` not 0;
- a `run_id.txt` not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 61–80;
- a cell whose variant or realistic-world lines do not match it;
- any window solver failure;
- an arm with fewer than 20 seeds.

## 5. Disclosure

Seeds 61–80 have not been run by this line. Read before this commit, all exploratory:
- the IEKF pilot (`~/ns/catbl-work/iekf_pilot`);
- the N.62 diagnosis runs on seed 5;
- the six-seed 868 MHz range-error study;
- the offline replays (`analysis/smoother_replay/`);
- the single-arm window runs at 5 km and in the endgame (seeds 1–6, 600 s);
- the five-arm pilot above.

## 6. What this cannot establish

Real sensors beyond these models; the window length (60 is ours, not swept); the ring; Wolf ToF; the code default;
compute cost on a field device (the window solves about 130 unknowns per node per cycle).
