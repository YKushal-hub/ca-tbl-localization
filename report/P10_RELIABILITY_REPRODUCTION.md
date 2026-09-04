# The §4.1 reliability result does not reproduce, and it has no sidecar

**Written** 2026-09-05, from `results/p10_reliability`, 40 cells, `dirty 0`, commit `6d44895`.

## The claim under test

`CA-TBL_Technical_Report_SelfContained.md` §4.1, carried into `MASTER_PLAN.md` §7.1 as the
strongest surviving result:

> `ca_tbl_full` **19/20** vs `static_2p4` **0/20** deployments at `RURAL_FOREST`,
> `dwell = 800`, `boxHalf = 2000`, **1.56× energy**.

A deployment is one seed; it meets the requirement when availability `A ≥ 0.95` **and**
accuracy `R ≤ 25 m`, per `Architecture.md` §1.3. `A = fixCycles / steps`, whose denominator
note is explicit — all decision cycles count, and a cycle with no fix counts against.

## What was run

`envFacility=1` (RURAL_FOREST), `dwell=800`, `boxHalf=2000`, six arms, 20 seeds, traced.
`nlosSource` is **not stated in §4.1**, so both values were run rather than guessed. Every
other flag at its default.

## What came back

| nlosSource | arm | meets | median A | median R (m) | mean J |
|---|---|---|---|---|---|
| legacy | `ca_tbl_full` | **1/20** | 0.9007 | 4.61 | 459.8 |
| legacy | `static_2p4` | 0/20 | 0.7840 | 3.21 | 8.8 |
| propagation | `ca_tbl_full` | **0/20** | 0.8931 | 4.26 | 187.7 |
| propagation | `static_2p4` | 0/20 | 0.8785 | 3.04 | 8.8 |

Against a published 19/20, and a published 1.56× energy ratio measured here at **52×**
(legacy) and **21×** (propagation).

## The finding is about provenance, not about the claim

**This is not evidence that the §4.1 claim is false.** It is evidence that the claim
**cannot be checked**, for a reason that is itself the result:

> Every `dwell=800` sidecar in the repository is one this campaign just wrote.
> `find results -name run_id.txt -not -path "*p10_reliability*" | xargs grep -l "dwell=800"`
> returns **nothing**. No pre-existing run in the tree uses the dwell the claim is stated at.

So the flag set §4.1 used is unrecoverable from the repository. The qualitative shape does
survive — `static_2p4` fails on availability while ranging accurately (median R 3.0–3.2 m,
median A 0.78), and the tiered arm buys availability at a large energy multiple — but neither
arm crosses the 0.95 threshold under defaults, and the energy ratio is an order of magnitude
from the published one.

An energy ratio wrong by 13–33× is not a metric-definition disagreement. It is a different
configuration. The most likely candidates are flags this run left at defaults — `gwGrid`,
`pcsForm`, `escalationRequiresEvidence`, `floorOnDemand`, `mahalUseR` — but the repository
records no invocation to check against, so that is a hypothesis and it is labelled one.

## Why this matters more than the number

`README.md` states that `results/` contains every `arm_summary.csv` and `run_id.txt` so that
"every table in the technical report is sourced from a file in this repository." For §4.1 —
and for the two headline results sourced from the absent `results/p10_grid` — that is **not
true**. The gate suite does not catch it: no gate asserts that a cited results directory
exists, which is rule 9 exactly ("a check for a defect whose mode is *absence* cannot be a
scan of present data").

## Actions

1. §4.1 must be restated as **unsourced pending a recorded invocation**, or re-derived from a
   campaign that writes one. It should not continue to be cited as a measured result while no
   directory in the tree produced it.
2. A gate is needed that asserts every results directory cited by the report exists and carries
   a sidecar. It has to read the citations and test for absence, not scan what is present.
3. The 1.56× figure and the 19/20 count should be treated as belonging to the same unrecovered
   configuration — they stand or fall together.

## What this campaign cannot establish

Whether §4.1 is correct. It establishes only that the stated configuration, run at defaults on
a tree that reproduces `baseline_v7` byte-for-byte, does not produce the published numbers, and
that no invocation survives to explain the gap.
