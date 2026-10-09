# ACC-A2 results — the error-state IMU filter (Plan A, A2)

2026-10-09. Exploratory grade. Registered in `report/PREREG_ACC_A2_ERROR_STATES.md` (`9da4bd71`) before the first
evaluation cell.
- **Runs:** `results/acc_a2_cit_2026-10-09`, a citable re-run (`source 88b0a101 dirty 0`), byte-identical to the exploratory runs these numbers were first computed from.
- **Reference:** the citable `results/acc_b3_cit_2026-10-09`.

## Setup

- **Flag:** `--imuErrorStates` (`88b0a101`, gate G101) carries the IMU's heading error, gyro bias and scale error as
  filter states.
- **Bias random walk:** q_b = 0, picked on seeds 21–30.
- **Runs:** seeds 1–20, 1800 s, all five arms. A = 5 km; B = endgame.
- **Verdicts:** paired seed bootstrap. "w25" is five-node within 25 m; CEP is tag 0's.

## 1. The filter alone (IE) against the preset, `ca_tbl_full`

| | A (5 km) | B (endgame) |
| --- | --- | --- |
| tag 0 CEP50 | 16.2 → 6.5 m; −15.3 [−23.0, −7.9] **WIN** | 4.80 → 1.39 m; −4.18 [−5.67, −2.88] **WIN** |
| tag 0 CEP95, mean per seed | 57.8 → 56.0 m; −1.8, tie | 12.0 → 5.1 m; −6.94 [−10.40, −3.69] **WIN** |
| within 25 m, five nodes | 0.613 → 0.708; +0.095 **WIN** | 0.968 → 0.988; +0.020 **WIN** |
| tag 0 within 25 m | 0.635 → 0.857; +0.222 **WIN** | +0.010, tie |
| availability | +0.004, tie | −0.004, tie |
| J per node | +0.45, tie | +0.37, tie |

**Predictions**
- **E1–E5 hold:**
  - endgame CEP95 and CEP50 WIN;
  - 5 km CEP50 WIN;
  - no availability LOSS;
  - no energy LOSS.
- **E6 holds:** the change is filter-wide. `static_2p4`'s endgame CEP95 is −6.5 m (WIN), and its within 25 m rises
  in both configurations.

**Under IE alone, `ca_tbl_full` has no accuracy LOSS against any arm in either configuration.** This was not
registered (E8 was registered for IEB3):
- **5 km:**
  - beats `static_2p4` on CEP50, −9.0 m (WIN);
  - ties it on within 25 m, −0.046 [−0.111, +0.021];
  - beats `poucet_fusion` and `lora_fusion` on availability, within 25 m and CEP50.
- **Endgame:**
  - ties `static_2p4` and `fixed_threshold` on every accuracy metric;
  - beats `lora_fusion` on CEP50 and CEP95, and `poucet_fusion` on CEP50.

## 2. The filter with the B3 flags (IEB3)

**E7 fails.** IEB3 minus IE in the endgame:
- availability +0.020 (WIN);
- CEP50 +0.99 m [+0.44, +1.88] (**LOSS**);
- CEP95 +3.07 m [+0.37, +6.96] (**LOSS**).

At 5 km every accuracy metric is a tie.

**E8 fails.** Under IEB3, `ca_tbl_full` loses endgame CEP50 and CEP95 to `static_2p4` (+0.79, +3.57 m) and CEP50 to
`lora_fusion` (+0.89 m). At 5 km it has no LOSS, and WINs against `fixed_threshold`, `poucet_fusion` and
`lora_fusion` on within 25 m and CEP50.

**Reading.** The B3 flags were chosen against the old, over-confident filter.

| flag | old filter | consistent filter |
| --- | --- | --- |
| `--caseDInfoRatio` | refused lone ranges against a prior σ that understated the error | judges against a prior that is now honest (hypothesis, not tested) |
| `--dualBaseRelayTier` | supplied legs | supplies legs whose relays' covariances N.33 found over-confident |

**They are not to be combined with the error-state filter without re-tuning.**

## 3. What follows for the preset

The evidence supports adopting `--imuErrorStates` (q_b = 0), and not the B3 flags.

## 4. What this cannot establish

- **The IMU model.** The filter's priors are the generator's own parameters (`CaTblImuParams`), so it is given the
  IMU it is simulated with. Plan A's A3 stress cases (a mis-declared prior, a drifting bias) were not run. This is
  a statement about the estimator against this generator, not about hardware.
- **The start.** Seeded at truth (N.39). The unknown start was not run with the filter.
