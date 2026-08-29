# abl_clean — pre-registration

Written before any result directory exists. Source `6906ad5`, tree clean, `dirty 0`.

## Why

`c9_clean` re-measured Claim 9 for `suspectRecovery=none` on clean provenance. The
other three recovery policies — `inflate`, `bounded`, `reinit` — still rest only on
`results/campaign_recovery`, which records `source a059203 dirty 4` and carries no
`ALLOW_DIRTY` markers. This is the last open provenance gap in the project.

## Design

Identical to `c9_clean` except for the policy under test. Flags from
`campaign_recovery`'s own sidecar:

    --boxHalf=2000 --gwGrid=5 --escalationRequiresEvidence=true
    --nlosSource=propagation --pcsForm=v2 --suspectForcesEscalation=false
    --mahalUseR=true --injectRangeBias=40 --injectBiasFraction=0.2
    --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true

Cells: `{inflate, bounded, reinit}` × `{F1, F3}` × 20 seeds = **120**.
`suspectRecovery=none` is not repeated; `c9_clean` holds it.

`suspectForcesEscalation=false` is in the flag set, so as in `c9_clean` this measures
**rejection-plus-recovery**, with no escalation following a rejection.

## Pre-flight

`degeneracy_check.py <dir> integrity` on seed 1 with `suspectRecovery=inflate`:
**LIVE** — 71 suspects, 37 fields differing from `no_integrity`.

## Registered prediction

**None of the three recovery policies beats `no_integrity` on availability**, as in
the original campaign.

## Explicitly NOT predicted

Any **magnitude**, or any **per-policy ordering** between `inflate`, `bounded` and
`reinit`. There is no basis for one, and predicting an order to have something to
score is what this project already corrected once.

## Provenance

Every cell records `dirty 0`, verified per sidecar at the end as well as at launch.
Nothing under the dirty counter's scope is edited while this runs. A `dirty != 0`
directory is discarded and re-run, never kept and qualified. Traces are not tracked.

## After it lands

`results/campaign_recovery` is superseded **in full**, with its reason, and every
number in the ledger traces to a `dirty 0` source.
