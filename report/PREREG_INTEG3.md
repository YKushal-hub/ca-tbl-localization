# PREREG INTEG3 — does the Switching-Point Integrity Check cost CA-TBL its availability under the preset?

**Registered before any cell runs** (rule 13). Written 2026-10-20. This commit adds this file, `scripts/integ3_run.sh`
and `scripts/integ3_analyse.py`, and nothing else. Outputs go to `~/ns/catbl-work/integ3_2026-10-20/`, which does not
exist at registration.

**The mechanism.** This is the Switching-Point Integrity Check: Mahalanobis distance against the preset's χ² 1992.9,
with `--suspectRecovery=downweight`, so a SUSPECT fix is fused at R × 4 and PCS × 0.5. It is **not** the evidence gate
(RULES.md §2).

## Why

FIDELITY-1b (`report/PREREG_FIDELITY1B.md`) found that under the preset's corrected receiver, CA-TBL's five-node
A(25 m) at 5 km falls below `fixed_threshold`'s: −0.056 [−0.106, −0.006] (TECH_DEBT R.11).

**Read post hoc from its tracked summaries:**
- tag 0's tier usage is near identical between the two arms;
- CA-TBL flags 71 SUSPECT fixes per run under the corrected receiver, against 40 under the old one;
- `fixed_threshold` flags none.

**An exploratory diagnostic was then run, uncommitted and not citable:** `~/ns/catbl-work/diag_integrity`, seeds
161–170, 5 km, the corrected receiver, adding `ca_tbl_no_integrity`. Its `ca_tbl_full` rows were byte-identical to
FIDELITY-1b's.

| contrast, five-node A(25 m) | 10 seeds |
|---|---|
| ca_tbl_full − fixed_threshold | −0.052 [−0.124, 0.019] |
| ca_tbl_no_integrity − fixed_threshold | −0.001 [−0.043, 0.045] |
| ca_tbl_no_integrity − ca_tbl_full | +0.051 [−0.035, 0.140] |

**Hypothesis:** the deficit is the integrity check's downweighting of fixes that are not faulty. In this
environmental fault model nothing is injected in these runs, so every SUSPECT is a false alarm by construction.

## Cells (`integ3_run.sh`)

**Every cell** runs the preset as it stands at this commit, with the receiver corrections in (DESIGN_DECISIONS 18),
plus REAL-BEST's realistic stack and `--ekf5Window=60`:

    --specProfile=true --duration=1800 --trace=true --traceCovariance=true --unknownStart=true --fixedVarThreshold=12
    --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3
    --ekf5FinderConsider=true --ekf5RelayGate=true --reportAirtime=true --ekf5Window=60

**Arms:** `ca_tbl_full`, `ca_tbl_no_integrity`, `fixed_threshold`, `poucet_fusion`.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.
- C: the 8-gateway ring.

**Seeds:** 181–200, fresh. **Total:** 60 cells.

## Predictions

| # | prediction | basis |
|---|---|---|
| I1 | no-integrity − full, five-node A(25 m), in A: interval above 0 | diagnostic +0.051, at twice the seeds |
| I2 | no-integrity − full, five-node A(25 m), in B and C: lower bounds above −0.02 | no fault is injected, so every SUSPECT is a false alarm |
| I3 | no-integrity − `fixed_threshold`, five-node A(25 m), in A: the interval contains 0 | diagnostic −0.001: the check is the whole deficit |
| I4 | no-integrity − full, subject J, in A: upper bound below +1.0 J | diagnostic +0.13 [−0.20, 0.54] |

## What follows, stated now

**If I1 and I2 hold:**
- the preset's integrity check costs availability and buys nothing measurable, because there is nothing to detect
  without a fault;
- removing it from the preset is recommended to the owner (O13);
- what it is meant to catch (environmental faults, the S7 injection flags) is a separate question. That needs a
  fault-injected campaign before any removal, so the result is a recommendation, not an adoption rule.

**If they fail:** the deficit is not the check, and R.11 stays open.

## Abort conditions

- not 60 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 181–200;
- a cell whose `spec_profile.txt` lacks the receiver corrections;
- a window refusal in either CA-TBL arm;
- fewer than 20 seeds for any arm and configuration.
