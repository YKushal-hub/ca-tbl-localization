# PREREG FIDELITY-1b — the size of the two receiver corrections, re-measured after FIDELITY-1's abort

**Registered before any cell runs** (rule 13). Written 2026-10-20. This commit adds this file,
`scripts/fidelity1b_run.sh` and `scripts/fidelity1b_analyse.py`, and nothing else. Outputs go to
`~/ns/catbl-work/fidelity1b_2026-10-20/`, which does not exist at registration.

## Why

FIDELITY-1 (`report/PREREG_FIDELITY1.md`) fired its registered abort, "any window solver failure": one failure
in each of three `static_2p4` cells, none in any other arm.

**The failure path is a refusal.** The window solve returns without touching the filter when the undamped Hessian at
the solution is not positive definite, or a marginal is non-finite (`ca-tbl-imu-ekf.cc`, the two `++m_winFails`
sites). An unobservable window is declined, and the EKF stands.

Both corrections joined the preset by FIDELITY-1's rule (DESIGN_DECISIONS 18). Their size is owed before any claim
states it, and this campaign measures it.

## Cells

FIDELITY-1's design, with these changes:
- **seeds** 161–180;
- **base** W (`--ekf5Window=60`), the only base, because PRESET-2 was not adopted;
- **both variants pass their settings explicitly:**
  - F0 = `--uwbSensitivityDbm=-104 --iqOverloadOnly=false`, the pre-adoption receiver;
  - F1 = `--uwbSensitivityDbm=-99.3 --iqOverloadOnly=true`, the preset's.

Configurations A, B and C; all five arms; 120 cells; one frozen binary checked before every cell.

## Predictions

R1–R3, unchanged from FIDELITY-1 and set before its cells ran. FIDELITY-1's aborted reading is disclosed: all three
would have held. They are therefore not blind here, and are re-tested on fresh seeds.

- **R1:** F1 lowers CA-TBL's five-node A(25 m) in no configuration; every lower bound is above −0.03.
- **R2:** F1 moves CA-TBL's tag-0 A(25 m) in B by under 0.02 either way.
- **R3:** F1 lowers CA-TBL's subject J in A; the interval is below 0.

## Abort conditions

FIDELITY-1's, except one:
- **dropped:** "any window solver failure";
- **instead:** **a window refusal in any `ca_tbl_full` cell aborts.** Refusals in the other four arms are counted
  and reported, not aborted on.

**Reason, stated before running.** A refusal leaves the state unchanged, so it cannot corrupt a comparator's
numbers. It can, however, change CA-TBL's, which every claim is about.

The other conditions are unchanged:
- not 120 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 161–180;
- a variant whose flags do not match;
- fewer than 20 seeds for any arm.

---

# RESULT — after the registration above (`266af58b`)

**Run.** Launched 2026-10-20T14:42:12Z from `266af58b`, dirty 0, base W. Done 15:56:57Z. 120 of 120 cells, every `rc` 0,
one source; **problems none**. There was no window refusal in any CA-TBL cell, so the abort condition did not fire.
Other arms had 2 refusals in 480 arm-runs, reported as registered. Scored once:
`results/fidelity1b_2026-10-20/FIDELITY1B.md`.

| # | verdict | measured, CA-TBL, F1 − F0, paired |
|---|---|---|
| R1 | HOLDS | five-node A(25 m): A +0.074 [−0.026, 0.157]; B +0.017 [−0.018, 0.052]; **C +0.152 [0.071, 0.228]** |
| R2 | HOLDS | tag 0 in B: +0.002 [0.000, 0.004] |
| R3 | HOLDS | subject J in A: **−5.06 J [−6.15, −4.13]** |

## The size of the correction, every arm (five-node A(25 m), F1 − F0)

- **5 km:**
  - Poucet **+0.187** [0.116, 0.260]
  - `fixed_threshold` **+0.137** [0.047, 0.221]
  - CA-TBL +0.074, spanning 0
- **8-gateway ring:**
  - CA-TBL **+0.152**
  - `fixed_threshold` +0.153
  - Poucet +0.076, spanning 0

**The corrected receiver helps every arm. At 5 km it helps the comparators more than CA-TBL.**

## What that does to the comparisons, under F1 (the preset since `39c1dfee`)

CA-TBL − comparator, five-node A(25 m), paired:

| config | − `fixed_threshold` | − Poucet |
|---|---|---|
| A, 5 km | **−0.056 [−0.106, −0.006]** | −0.075 [−0.177, 0.015] |
| B, endgame | −0.008 [−0.039, 0.024] | **+0.053 [0.009, 0.095]** |
| C, 8-gateway ring | −0.008 [−0.045, 0.029] | +0.065 [−0.021, 0.149] |

- **At 5 km, under the corrected receiver, CA-TBL's five-node availability is below `fixed_threshold`'s,** and the
  interval excludes 0. Its tag-0 point estimate against Poucet is −0.163 [−0.365, 0.041].
- **In the endgame, CA-TBL still beats Poucet** (+0.053), at about 1/20 of Poucet's energy (−351 J).
- **Energy:** CA-TBL against Poucet is −442 J (A), −351 J (B) and −442 J (C). Against `fixed_threshold` it is level.
