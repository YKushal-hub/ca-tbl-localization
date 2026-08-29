# c9_clean — pre-registration

Written before any result directory in this campaign exists. Source: `2f0f0ac`, tree
clean, `dirty 0` in the counter's scope.

## Why

Claim 9's falsification rests on `results/campaign_recovery`, which records
`source a059203 dirty 4` — a binary no commit reproduces — and carries **no**
`ALLOW_DIRTY` markers, so a reader checking for the marker would conclude it was
clean. No campaign anywhere under `results/` has both `dirty 0` and fault injection.
The project's own standard is that a number measured on a dirty tree is unciteable,
and this is the most consequential negative result in the ledger.

## Design — flags read from `campaign_recovery`'s own sidecar, not inferred

    --boxHalf=2000 --gwGrid=5 --escalationRequiresEvidence=true
    --nlosSource=propagation --pcsForm=v2 --suspectForcesEscalation=false
    --mahalUseR=true --injectRangeBias=40 --injectBiasFraction=0.2
    --suspectRecovery=none --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true

Cells: `F1` (`injectFaultAnchors=1`) and `F3` (`injectFaultAnchors=0`), 20 seeds each.

**Design difference from `campaign_recovery`, stated because it is unavoidable here:**
that campaign ran four `suspectRecovery` levels × three fault sets = 240 cells. This
runs `suspectRecovery=none` × two fault sets = 40 cells. `none` is the level Claim 9's
verdict turns on — whether the gate pays at all — and `F0` is the null (no injected
bias). The recovery-policy ablation is not re-measured; if the falsification reverses
here, that ablation must be re-run before anything is concluded about the policies.

**`suspectForcesEscalation=false` is in that flag set.** `campaign_recovery` therefore
already isolated rejection from escalation — the thing D7 was built to do and could
not, because with the flag false `ca_tbl_full` stopped escalating entirely in the D1
config-E pre-check. Claim 9's falsification should be read as a statement about
**rejection alone**, not about the combined detect-and-escalate mechanism. That is a
stronger and narrower reading than the ledger currently carries.

## Pre-flight

`degeneracy_check.py <dir> integrity` on seed 1: **LIVE** — 86 suspects, 37 fields
differing from `no_integrity`. The same screen returns LIVE for `tiering` (99.5% on
2.4 GHz but 2 escalations). `campaign_recovery` is LIVE for `integrity` on 240/240
cells, so the comparison is like-for-like.

## Registered prediction

**The falsification reproduces.** `ca_tbl_full` does not beat `ca_tbl_no_integrity`
under injected range bias on the corrected engine, and the arms differ in every cell
as they did before.

## Explicitly NOT predicted

The **direction** of any change in suspect counts, `trig_precision`, or
`trig_recall`. D1 changed which anchors are heard, which changes `nNlos`, which
changes the gate's eligible population; the sign and magnitude of that are unknown
and there is no basis for guessing. These are recorded as **measured values**, not as
hits or misses.

This project has already published a "recall falls" that turned out to be a hardcoded
string in an analysis script rather than a derived result. A second one is not created
here by predicting a direction in order to have something to score.

## The three outcomes

| outcome | consequence |
|---|---|
| falsification reproduces | ledger keeps its verdict, gains a citeable source; `campaign_recovery` marked superseded with its reason |
| falsification reverses | **stop and report before writing anything.** Both halves go in the ledger, and the D1 defect becomes the named cause |
| campaign degenerate | that is the finding; parameters are not adjusted to make it measurable |

## Provenance requirement

Every cell records `dirty 0`, re-verified at the end and not only at launch. Nothing
under the dirty counter's scope is edited while this runs. A `dirty != 0` directory is
discarded and re-run, never kept and qualified.
