# A0 applied — the registered criterion returns FAIL everywhere, and the reason matters

**Written** 2026-09-05 from `results/p10_grid`, 320 cells regenerated at commit `6d44895`,
**320/320 recording `dirty 0`**, 64 configurations × 5 paired seeds. Criterion applied
verbatim from `report/PREREG_A0_JOINT_CRITERION.md` §2, committed **before** the grid ran.

## Result

| verdict at κ = 1.0 | configurations |
|---|---|
| PASS | **0** |
| FAIL | **64** |
| INCONCLUSIVE | 0 |

Robust across the entire registered sensitivity band — **no configuration flips** between
κ = 0.25 and κ = 2.0. Energy ratios ρ run from **1.45× to 8.24×**, every CI entirely above
1.0.

## The reason is not the energy. It is that nobody meets the requirement.

`dD = D_T − D_B = 0` in **all 64 configurations**, and it is zero because
**`D_T = 0` and `D_B = 0`** — not because they cancel. Across all 320 cells, neither
`ca_tbl_full` nor `static_2p4` meets `(A ≥ 0.95, R ≤ 25 m)` even once.

The binding constraint is availability, not accuracy. `R` is comfortable throughout —
medians around 3–5 m on the tiered arm. `A` is not: it sits near 0.78–0.90 against a
threshold of 0.95.

## A defect in the registered rule, found by applying it

The A0 rule maps `dD ≤ 0` to **FAIL**. Here `dD = 0` arises from the *degenerate* case
where the requirement is met by nothing at all, and calling that a FAIL **for the tiered
arm** attributes to it a failure that belongs to the criterion.

The project's own standing discipline says the opposite. `PREREG_C9_ESCALATE.md` §6 states
it explicitly:

> **if neither arm meets `(A ≥ 0.95, R ≤ 25 m)` anywhere in the grid, that is
> INCONCLUSIVE, not PASS.** A criterion no competitor can meet decides nothing.

A0 was written to fix exactly this class of problem — its §2 says the entire point is that
`rho` and `dD` are both defined when `D_B = 0`, so a configuration where the baseline meets
nothing still returns a verdict. It handles `D_B = 0`. It does **not** distinguish
`D_T = 0, D_B = 0` from `D_T = 3, D_B = 3`, and those are different situations: the second
is a genuine tie, the first is a criterion nothing can satisfy.

**The rule is applied verbatim above, as registered.** This section records that the
verdict it produces is degenerate, and that the amendment needed is a fourth branch:

> `D_T = 0` **and** `D_B = 0` → **INCONCLUSIVE (requirement unmet by any arm)**, never FAIL.

Amending it after seeing the data is exactly what pre-registration forbids, so the amendment
is proposed here and **not applied to this verdict**. The registered FAIL stands as the
registered result.

## What this corroborates

It independently confirms `report/P10_RELIABILITY_REPRODUCTION.md`. That campaign ran the
§4.1 configuration at 20 seeds and got `ca_tbl_full` 1/20 and 0/20 against a published
19/20. This grid, at 5 seeds across 64 configurations including that one, gets **0 out of
320**. Two independent campaigns agree that under default flags the requirement is not met
anywhere — and that §4.1's 19/20 is not reachable from any invocation recorded in this
repository.

Both results share one alternative explanation, and it is the same one: the flag set the
original campaigns used is not on disk. Availability is the axis that fails, and flags that
would raise it — `--floorOnDemand`, `--floorSkipK`, `--floorHeartbeatN`, `--gwGrid` — are
left at defaults here because nothing records what they were set to.

## What this campaign cannot establish

- **That the architecture fails.** It establishes that the registered criterion returns FAIL
  on a grid where the requirement is unreachable for every arm, which is a different claim.
- **That the requirement is wrong.** `(A ≥ 0.95, R ≤ 25 m)` is fixed in `Architecture.md`
  §1.3 as a design requirement and is not adjustable after the fact.
- **Anything about configurations outside the 64 tested**, or about flag settings the
  regenerated grid did not use.
