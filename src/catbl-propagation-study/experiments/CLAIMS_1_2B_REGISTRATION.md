# Claims 1 and 2b, predicted from the mechanism — pre-registration

Written and committed **before the first cell**. These cells do not exist.

## Why this is a registration and not an investigation

Three independent results have landed on one lever:

| finding | how `nlosSource=legacy` acted |
|---|---|
| **W11** — Claim 3's PASS | parking on UWB **suppressed the floor**, making the tiered arm look cheap |
| **Claim 4** — floor optimisation | more time off tier 1 meant **more floor to suppress**, inflating the saving |
| **Claim 2** — energy | 623.8 J of 640.8 was UWB across **1170 escalation cycles**; the defect inflated the **baseline** |

One mechanism: **`legacy` inflates escalation, and every improvement claim here is a ratio whose
baseline was measured under inflated escalation.** Claims 1 and 2b follow from it without being
looked at first, which is what makes this a prediction rather than a search.

## Claim 1's ground truth, read from source before registering

`ca_tbl_causal_sim.cc:3218-3221`:

```
const uint32_t visUwb = VisibleCount(dep, CA_TBL_TIER_UWB, pos);
const bool shouldUwb = (visUwb >= 3);
```

and `VisibleCount` (`:328-343`) is **purely geometric** — Euclidean distance against
`nominalRangeM[tier]`, with no reference to `nNlos`, the NLOS source, or any physics outcome.

**So the ground truth is flag-invariant, and the comparison is sound.** Only the *prediction*
(`onUwb`) moves with the source; the label it is scored against does not. That was the caution and
it is answered: both numerator and denominator do not move together.

**One caveat that does not affect the comparison but does affect the absolute number.**
`nominalRangeM[UWB] = 30.0 m` (`:225`), while the same file records measured UWB **r80 = 12.9 m,
r50 = 29.5 m** (`:220`). The label therefore calls a cycle "should be on UWB" when three anchors sit
within a distance at which roughly **half** of packets get through. The ground truth is generous,
in the same family as `vis2p4`'s censored 900 m already recorded. **Absolute precision inherits
that; the legacy-against-propagation difference does not.**

## Registered predictions

1. **Claim 2b.** The escalation-efficiency **baseline** (published 0.071) **rises** under
   `propagation`, and the improvement to 0.827 **shrinks**. Mechanism: the defect drives useless
   escalations, which depress baseline efficiency.
2. **Claim 1.** The trigger-precision **baseline** (published 0.078) **rises** under `propagation`,
   and the improvement to 0.934 **shrinks**. Mechanism: precision is `tp/(tp+fp)`; the defect
   inflates `fp` by escalating when `shouldUwb` is false.

A baseline of 0.078 is roughly **thirteen false escalations per true one** — which is what a flag
driving 1170 escalation cycles where the corrected source drives 212 would produce.

**Falsifier, for both:** a baseline that does not move, or moves **down**. Then escalation inflation
does not reach these metrics, the mechanism is narrower than three instances suggested, and that is
said before either claim is touched.

## The mediator, named in advance and checked against what it measures

Per the `cycles_868` lesson — naming a mediator in advance does not exempt it from being checked
against what it actually measures:

- **`trig_fp`** — the false-positive count itself. If the baseline precision rises, `fp` **must**
  fall. If the baseline rises while `fp` does not fall, the account is wrong whatever the endpoint
  does, because precision could only have risen through `tp`.
- **`escalations`** — total escalation count per seed, the quantity the mechanism says is inflated.

**Both are checked against their definitions in source before use**, not accepted on their names:
`trig_fp` increments at `:3237-3240` exactly when `onUwb && !shouldUwb`, which is the false positive
the claim means. That check is the point of naming it here.

## Design

    --duration=3600 --boxHalf=2000 --gwGrid=5 --trace=true --arms=ca_tbl_full
    --escalationRequiresEvidence={false,true}  x  --nlosSource={legacy,propagation}
    seeds 1..20

**Identical to `claim2_rebase`.** Both metrics are columns of `arm_summary.csv` from that same
design — `trig_precision`, `trig_tp/fp/fn/tn`, and the escalation counters are all emitted by the
runs already performed. **So this needs no new campaign**, and the honest statement is that the
data already exists and these predictions are registered **before looking at those columns**, which
is weaker than registering before running and is labelled as such.

The columns were not inspected while scoring Claim 2, which used `totalJ`, `j_868`, `j_2p4`,
`j_uwb` and `cycles_uwb` only.

## Consequences

| outcome | consequence |
|---|---|
| both baselines rise, improvements shrink | Claims 1 and 2b **re-scoped**, as 2 and 4 were; magnitudes restated beside the published ones |
| baselines rise but improvements survive intact | the mechanism reaches the baseline without changing the claim; reported as such |
| a baseline does not move or moves down | **falsifier fires**; the mechanism is narrower than claimed and that is recorded before anything is re-scoped |

**Claim 9 is not touched.**
