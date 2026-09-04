# Claims 1 and 2b — result

Registered in `CLAIMS_1_2B_REGISTRATION.md` before the columns were looked at, on the
`claim2_rebase` cells. Scored once.

## Claim 1 — the falsifier FIRES

Registered: *the baseline precision (0.078) **rises** under `propagation`.*

| source | arm | precision | 95% CI |
|---|---|---|---|
| legacy | ev=false (**baseline**) | **0.0755** | [0.0706, 0.0803] |
| legacy | ev=true | 0.9152 | [0.8937, 0.9367] |
| **propagation** | ev=false (**baseline**) | **0.0129** | [0.0010, 0.0248] |
| propagation | ev=true | **0.0000** | [0.0000, 0.0000] |

**The baseline moves DOWN, by −0.0626. The registered falsifier fires.**

## The mediator was right and my inference from it was wrong

This is the distinction the mediator existed to draw, and it draws it cleanly.

| ev=false arm | legacy | propagation |
|---|---|---|
| `trig_fp` | 1081.8 | **210.2** |
| `trig_tp` | 88.3 | **2.0** |
| escalations | 219.2 | 177.0 |

**`fp` fell 5×, exactly as the escalation-inflation mechanism predicts.** The mechanism is not
falsified — it is confirmed on its own terms.

**What was falsified is my inference from it.** Precision is `tp/(tp+fp)`, and I reasoned only
about `fp`. **`tp` fell 44×** — far harder than `fp` fell — because the corrected source suppresses
*all* escalation into UWB, not merely the wrong escalations. A flag that stops the node going to
UWB stops it going when it *should* just as effectively as when it should not.

> **A mechanism can be correct and still not license a prediction about a ratio, when the mechanism
> moves both the numerator and the denominator.** The mediator is what separates those two
> readings; without it, a falsified endpoint would have looked like a falsified mechanism.

## What the corrected source actually does to Claim 1

The full confusion matrix, mean per seed:

| source | arm | tp | fp | fn | tn | recall | F1 |
|---|---|---|---|---|---|---|---|
| legacy | ev=false | 88.3 | 1081.8 | 2.5 | 267.4 | 0.9740 | 0.1399 |
| legacy | ev=true | 66.8 | 6.1 | 24.1 | 1343.0 | 0.7296 | 0.8043 |
| propagation | ev=false | 2.0 | 210.2 | 88.8 | 1138.9 | 0.0239 | 0.0153 |
| **propagation** | **ev=true** | **0.0** | **0.0** | 90.9 | 1349.1 | **0.0000** | **0.0000** |

**Under `propagation` with the evidence gate on, the trigger never fires into UWB at all** — zero
true positives *and* zero false positives out of 90.9 ground-truth opportunities per seed. Precision
is `0/0`, reported as 0.

**So Claim 1 is not "smaller" under the corrected source. The mechanism it describes does not
operate.** That is a different and more serious finding than a shrunken magnitude, and it is not a
re-scoping.

**Ground-truth invariance, verified empirically as well as from source.** `tp + fn = 90.9` in **all
four cells**, identical — the label does not move with the flag, as `VisibleCount`'s pure geometry
predicted. The comparison is sound; only the behaviour being scored moved.

## Claim 2b — NOT SCOREABLE: the estimator cannot be recovered

Published: escalation efficiency **0.071 → 0.827**, footnoted as *"fraction of escalations that
produced a usable precision fix."* There is **no such column** in `arm_summary.csv`.

Every candidate estimator computable from the emitted columns, evaluated against the published
directories:

| candidate | `p10_plane` | `p10_plane_fix` | matches 0.071 → 0.827? |
|---|---|---|---|
| `tp/(tp+fp)` | 0.0776 | 0.9280 | no |
| `trig_precision` column | 0.0776 | 0.9279 | no |
| `trig_f1` column | 0.1436 | 0.7879 | no |
| `trig_recall` column | 0.9784 | 0.7005 | no |
| `tp/escalations` | 0.4114 | 16.2087 | no |
| `cycles_uwb/escalations` | 5.2936 | 17.3443 | no |

**None matches.** The same scan does confirm **Claim 1's** estimator — `trig_precision` gives
0.0776 → 0.9279 against a published 0.078 → 0.934, which is the same quantity to within the pre-D1
binary difference.

So Claim 2b's headline is a number whose **estimator is not recoverable from the evidence**, in the
same family as `[M-11]` citing a file type rather than a path. **Scoring a similar-looking estimator
and reporting that would be substituting a quantity for the one the claim makes.** Not done.
Reported as not scoreable, which is the estimator-level form of *an empty population is VACUOUS,
not a pass*.

## Provenance: the `dirty 1` cell, resolved per precedent

`legacy_evfalse/seed_2` recorded `dirty 1`. The `causal_v7` precedent is **discard and re-run**, and
that is what was done — re-run on a clean tree at `source 7053eba dirty 0`, and the result is
**byte-identical** to the discarded cell (`9f30f27f…`), so the provenance record is repaired without
any number changing. The clean cell replaces it; **0 cells in `claim2_rebase` now carry a non-zero
dirty counter**, and Claim 1's figures are unchanged after the swap.

Byte-identity is what makes discard-and-re-run costless here; had it differed, the honest course
would have been to re-run the whole stratum.
