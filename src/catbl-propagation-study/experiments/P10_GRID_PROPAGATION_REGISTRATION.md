# The p10 grid under the corrected NLOS source — registration

## What this is, and what it is not

**The propagation cells already exist on disk.** `results/p10_grid` was run with both
`nlosSource=legacy` and `nlosSource=propagation` variants at every `(ef, dwell, boxHalf)` point.
Nothing needs re-running.

So this is **not a pre-registration.** It is a prediction committed **before the cells are
scored**, on data that already exists and could in principle have been peeked at. That is
strictly weaker than registering before a run, and it is labelled as such rather than presented
as the same thing. What it still buys: the prediction and its falsifier are fixed in the
repository before any verdict is computed, so the reading cannot be chosen to fit.

The scorer is `scripts/claim3_m1.py`, already validated by reproducing the published legacy cell
exactly (0.9531, CI [0.9296, 0.9772] against [0.9296, 0.9775], energies to the digit).

## Registered prediction

> **No cell returns a PASS under `propagation`.**

Mechanism, from W11: the defect made `ca_tbl_full` look cheaper by suppressing its tier-1 floor —
the node parked on UWB and stopped paying the floor. Correcting the source restores the floor,
which costs four times what the UWB occupancy saved. So every legacy cell's M1 ratio should
**rise**. FAIL becomes more FAIL. INCONCLUSIVE may exchange with FAIL as spec-meeting changes,
but nothing crosses into PASS.

**Falsifier.** Any cell returning PASS under `propagation`. That would mean the defect was
*masking* an advantage rather than manufacturing one — the opposite of W11's mechanism — and it
would need its own explanation before being claimed as a result.

## Scope: what the rule can and cannot resolve

`b49df46` §4 is silent where no single-band arm qualifies. A flag that moves M1 in a
non-adjudicable cell **changes no verdict**. Only two things can change a verdict:

1. a cell becoming adjudicable, or ceasing to be, because the flag changes whether an arm meets
   (A = 0.95, R = 25 m);
2. an adjudicable cell's M1 crossing 1.0.

Both are measured. Cells that are non-adjudicable under both sources are reported as such and
counted, not silently dropped — an unadjudicable cell is not a FAIL.

## Seed depth, stated so it is not discovered later

The grid is staged: `ef0` and `ef1` carry 20 seeds, `ef3` and `ef4` carry 5. **Verdicts are
computed at 20 seeds only.** The 5-seed cells are reported as screening — adjudicable or not, M1
point estimate — and are **not** given a PASS/FAIL, because the registered rule's CI criterion is
not meaningful at that depth. That is the same staging the grid was built with.

## Not in scope

Claim 4 (`results/p1_condfloor`) records **no `--nlosSource` flag**, and the binary's default is
`legacy` (`ca_tbl_causal_sim.cc:3540`). It is therefore exposed to the same lever, and it is
**explicitly a claim about switching the floor off** — the exact mechanism W11 implicated. It gets
its own test and its own registration; it is not folded into this one.
