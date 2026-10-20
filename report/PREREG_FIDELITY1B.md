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
