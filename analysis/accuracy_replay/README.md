# Accuracy replay — exploratory analysis behind `context.md` and `plan.md` (2026-10-08)

**Status: exploratory, not citable.** Nothing here is a registered campaign (RULES.md §1 rule 13).
The binary was built from the tree with `rangelog_patch.txt` applied, so every run reads `dirty`.
The patch is inert: it writes one extra CSV per arm and draws no random number. Its effect on the
simulator was checked, not assumed (see `context.md` §9).

## What the patch adds
`CATBL_RANGELOG=1` makes tag 0 write `ranges_<arm>_seed<N>.csv` beside the trace:
`T` rows (true position, dt, pre-step estimate, active tier, usedFloor), `R` rows (every range handed to
`Step()`: anchor position, range, sigma, nlos, sourceKind, fromFloor, anchorId), `A` rows (the tier-1
TDoA position fix queued for the next `T` row).

## Reproduce
```bash
git apply analysis/accuracy_replay/rangelog_patch.txt      # on a clean tree
CC=gcc CXX=g++ ./ns3 configure --build-profile=optimized --disable-tests --disable-python
./ns3 build scratch/ca_tbl_causal_sim
git checkout -- scratch/ca_tbl_causal_sim.cc               # revert the patch after building
cd analysis/accuracy_replay
ARMS=--arms=ca_tbl_full,static_868,static_2p4,fixed_threshold
for s in $(seq 1 20); do
  ./runcell.sh A $s $ARMS                                  # 5 km, the preset
  ./runcell.sh B $s $ARMS --boxHalf=750 --finderMode=1     # endgame
done                                                       # about 85 s per cell per 3 arms
```
Then, from this directory: `variants.py`, `paired.py`, `ctl_init_covariance.py`, `ctl_qscale.py`,
`synth_imu.py`, `range_stats.py`, `ml_tdoa_reliability.py`, `ml_fix_error.py`. Needs numpy, pandas,
scipy, scikit-learn.

## Files
| file | what it does |
|---|---|
| `replay.py` | parses the logs; a Python EKF that mirrors `CaTblEkf` (IMU control prediction, sequential range updates, TDoA position fusion); an augmented-state EKF (heading bias and scale as states) |
| `harness.py`, `variants.py` | pooled CEP per variant against the simulator's own trace |
| `paired.py` | per-seed paired differences with a seed bootstrap, and NEES |
| `ctl_init_covariance.py`, `ctl_qscale.py` | controls: initial covariance, and plain inflation of the IMU process noise |
| `synth_imu.py` | re-draws the IMU error with larger gyro bias to test robustness to IMU quality |
| `range_stats.py` | range-error calibration, autocorrelation, bias against distance |
| `ml_tdoa_reliability.py`, `ml_fix_error.py` | the two ML feasibility tests (seed-grouped cross-validation) |

## Limits of the replay
- Open loop: the estimate does not feed back into tier choice, finder motion or PCS.
- The Python filter omits the integrity check's R x 4 inflation (DC-7), the Case A/B two-range
  solves and the NIS-guarded Case D update. Fidelity against the simulator's own estimate is high
  for `static_2p4` in the endgame and poor for `ca_tbl_full` at 5 km (numbers in `context.md` §6.1).
  Read differences between variants, not absolute levels.
- The IMU error model (bias and scale constant for the run, plus angle random walk) is the
  simulator's. An estimator that models exactly that structure is matched to the generator.
