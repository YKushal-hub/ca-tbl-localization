# D_prefix_VOID — EVIDENCE OF A DEFECT, NOT A RESULT

**This directory must never be cited as a measurement.** It is retained so that the
defect it demonstrates stays auditable.

## What this is

Cell D of the v6 ladder, re-run on **commit `5994de53c`** — the arbitration commit
*before* the arbiter was corrected. Regenerated 2026-08-04 because the original
pre-fix D was destroyed by re-running over it in place; that loss is what motivated
the standing rule "never delete a results directory in place, `mv` it aside".

Invocation (see `run_id.txt` for the exact sidecar):

```
--seed=7 --duration=3600 --nlosSource=propagation --arbitrateDutyBudget=true \
--pcsForm=legacy --trace=true
```

## The defect it contains

`ca_tbl_full` records **`duty_868` = 2.5080%** against the WPC G.S.R. 853(E)
Table-II field-node budget of **2.5%**. This is a regulatory violation. The
verifier rejects it:

```
X seed 7/ca_tbl_full: 868 duty cycle within WPC 2.5%: duty=2.5080% exceeds the
  Table-II field-node budget -- some transmitting call site is not consulting
  CanTransmit()
```

The cause: at `5994de53c` the arbiter surrendered `rangeTier(active)`. Once the
tier change has committed, `active` is the **new** tier, and tiers 2 and 3 carry
no duty-cycle cap in this model — so it sacrificed an *uncapped* transmission to
make room on the *capped* one, and freed nothing. Meanwhile
`runFloor = hybridArm && (active != CA_TBL_TIER_868)` kept ranging tier 1 on
exactly those cycles, and the handshake was charged on tier 1 on top of that.

Attribution, read from `arm_summary.csv` in this directory — excess tier-1 airtime
over `static_868` against `handshakePreemptions x 41.22 ms`:

| arm | airtime_868 | excess | preemptions x 41.22 ms | ratio |
|---|---|---|---|---|
| `fixed_threshold` | 88.8883 s | 7.4189 s | 7.4196 s | 0.9999 |
| `ca_tbl_no_integrity` | 82.0052 s | 0.5358 s | 0.5359 s | 0.9999 |
| `ca_tbl_full` | 90.2897 s | 8.8202 s | 8.8211 s | 0.9999 |

`ca_tbl_full` spends more tier-1 airtime (90.2897 s) across 362 tier-1 cycles than
`static_868` spends across all 1440 (81.4694 s).

Fixed at `0be15a19b`, which pre-empts the ranging exchange on `pendingHandshakeTier`
— the capped tier the handshake departs. Post-fix `duty_868` = 0.021700.

## Reconciliation

Every figure above was re-read from this directory on 2026-08-04 and matches
`report/PCS_REDESIGN_RESULTS.md` Part 5 §5.0 exactly, including the 2.5080% and all
three 0.9999 ratios. The regenerated run also reproduces the original pre-fix D's
`rmseM` 3.6544 / `totalJ` 28.7306 / 215 escalations, so the rebuild is deterministic
against the destroyed original.

## Rules for this directory

- Do **not** cite any number here as a result. The energy axis is wrong by
  construction for every escalating arm.
- Do **not** include it in any sweep, Pareto front, or paired comparison.
- Do **not** "fix" it by re-running on current source — that would destroy the
  evidence. If you want the corrected numbers, they are in `results/baseline_v6`.
