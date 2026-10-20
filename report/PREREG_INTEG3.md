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

---

# RESULT — after the registration above (`3de64c22`)

**Run.** Launched 2026-10-20T16:06:20Z from `3de64c22`, dirty 0. Done 16:30:48Z. 60 of 60 cells, every `rc` 0;
**problems none**, and no window refusal in any arm. Scored once: `results/integ3_2026-10-20/INTEG3.md`.

| # | verdict | measured, paired, 20 seeds |
|---|---|---|
| I1 | **FAILS** | no-integrity − full, five-node, A: **−0.024 [−0.056, 0.005]** |
| I2 | HOLDS | B −0.002 [−0.007, 0.003]; **C +0.036 [0.004, 0.070]** |
| I3 | HOLDS | no-integrity − `fixed_threshold`, A: −0.022 [−0.068, 0.017] |
| I4 | **FAILS** | J in A: +0.28 [−0.79, 1.55]; the upper bound exceeds +1.0 J |

## What it says

- **The hypothesis is falsified at 5 km.** Removing the integrity check does not raise CA-TBL's availability there.
- **FIDELITY-1b's 5 km deficit does not replicate.** On these fresh seeds, under the same preset, CA-TBL −
  `fixed_threshold` is **+0.001** [−0.045, 0.046] five-node (the table reads `fixed_threshold` − CA-TBL = −0.001).
  FIDELITY-1b's −0.056 [−0.106, −0.006] and the exploratory diagnostic's −0.052 both came from seeds 161–180. This is
  rule 8 at campaign scale: one 20-seed interval excluding 0 was not a result about the policy.
- **On the 8-gateway ring the check costs availability:** no-integrity +0.036 [0.004, 0.070] five-node and +0.045
  [−0.006, 0.142] tag 0, with CEP95 −62.6 m.
- **In the endgame** CA-TBL beats Poucet on five-node A(25 m), +0.097 [0.050, 0.143], using 354 J less per run.

## Consequence

- **No recommendation to remove the check on 5 km grounds.**
- **On the ring it costs availability, with no fault injected.** What it catches under the faults it exists for was
  already measured by INTEG2 (registered, fault-injected): recall 0.03–0.10 at the preset's threshold. So O13 is
  recommended as removal from the preset, for the owner.
- **CR-22's 5 km statement is scoped to seeds 161–180,** where it held. Across both campaigns, 40 seeds, CA-TBL is
  level with `fixed_threshold` at 5 km.
