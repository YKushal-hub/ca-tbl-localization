# Pricing the PCS repair — result

Registered in `REPAIR_ENERGY_REGISTRATION.md` (`ec1a3c1`) before the energy columns were read.

## The registered predictions, scored — a split, and the split is the lesson

| prediction | registered | measured | |
|---|---|---|---|
| `j_uwb` | **0.20 – 0.28 J** | **0.2396 J**, CI [0.175, 0.304] | **HELD** |
| `totalJ` | ~23.0 J, **below 23.5** | **27.090 J**, CI [26.73, 27.45] | **FALSIFIED** |

**The sub-prediction I derived carefully held to three decimals. The endpoint I derived by addition
failed by 3.6 J.**

| `ca_tbl_full` | as-is | repaired | delta |
|---|---|---|---|
| `j_868` | 14.620 | 14.808 | +0.19 |
| **`j_2p4`** | **8.127** | **12.043** | **+3.92** |
| `j_uwb` | 0.008 | 0.240 | +0.23 |
| **`totalJ`** | 22.755 | 27.090 | **+4.34** |

**The repair's tier-2 cost is sixteen times its UWB cost.** I predicted `totalJ` as
*base + UWB cost*, and that assumed the base was unchanged. It was not: the repaired predicate wants
to escalate on 51% of tier-2 cycles (`SITING_REPAIR_RESULT.md`) and is refused on nearly all of
them — and the attempts are not free.

> **This is the ratio lesson in a third place, and it is not the same mistake twice.** For precision
> I predicted an endpoint whose mechanism moved *both terms of the expression I wrote down*. Here I
> wrote down `totalJ = base + UWB`, constrained both terms of the UWB part correctly, and was wrong
> because **a term I had not written down moved.** Decomposing an endpoint does not license
> predicting it unless every term is constrained — **including the ones the decomposition omits.**

The registration claimed *"the mechanism constrains both terms"*. It constrained both terms of
`j_uwb`, which is why that prediction is exact. It did not constrain `j_2p4`, and nothing in the
registration noticed that `j_2p4` was a term.

## The verdict under `b49df46` §4, applied verbatim: **INCONCLUSIVE**

| arm | meets (A = 0.95, R = 25 m) |
|---|---|
| `ca_tbl_full`, repaired | **20/20** |
| `static_2p4` | **19/20** |
| `static_868`, `static_uwb` | 0/20 |

> *INCONCLUSIVE — the CI spans 1.0, or **no arm other than `ca_tbl_full` meets (A, R)**…*

`static_2p4` misses on one seed, so no single-band arm qualifies, so there is nothing to compare
against and the rule returns INCONCLUSIVE — **deliberately not PASS**, as it was written to.

**And that is the finding, not an inconvenience.** The repaired arm's advantage *is* that it meets
the requirement more often than the single-band arm. **The registered rule cannot express that**: it
compares energy among arms that both already meet the requirement, so an advantage consisting of
meeting it is invisible to it. This is precisely the single-axis mis-specification this project's
own future-work section identifies.

## Both axes together — not the registered verdict, and labelled as such

| cell | availability | meets 20/20 | `totalJ` | M1 vs `static_2p4` |
|---|---|---|---|---|
| as-is | 0.9852 | 20/20 | 22.755 | 1.7970 [1.7547, 1.8432] |
| **repaired** | **0.9989** | **20/20** | **27.090** | **2.1390** [2.1134, 2.1673] |
| `static_2p4` | 0.9917 | **19/20** | 12.679 | — |

**Outcome 2 of the work order's table, with both numbers attached:**

> **The architecture buys reliability with energy, and the amount is now measured.** The repaired
> tiered arm reaches **0.9989** availability and meets the requirement on **20 of 20** seeds against
> the single-band arm's **19 of 20**, at **2.14×** its energy — up from 1.80× before the repair.

The repair improves the tiered arm on the axis it was designed for and makes it **more expensive
relative to the baseline**, because the 4.34 J it adds is 19% of its own total while the baseline is
unchanged.

## What this settles

**The repair is real and its price is measured.** It is not a free improvement, and the earlier
observation that the repaired arm "moves from behind to ahead on availability" is now paired with
the cost of doing so — which is what makes it a trade-off statement rather than a selective one.

**Claim 2 is not reopened.** Its 6.53× is a ratio between two `ca_tbl_full` configurations under a
different flag; this is a ratio between arms. Different quantities, and the repair does not bear on
the first.

**Nothing here is claimed as a PASS.** The registered criterion returned INCONCLUSIVE and is reported
as such; the trade-off table is explicitly not the verdict.
