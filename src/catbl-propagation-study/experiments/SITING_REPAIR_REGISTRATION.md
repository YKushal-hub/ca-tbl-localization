# Is the anti-correlation repairable? — pre-registration

Written and committed **before the first cell**. Includes a degeneracy pre-flight that fired, with
its mechanism named, per the standing requirement to screen before spending a seed count.

## The placement rule, derived from the measured coverage surface

Not chosen by hand. The surface that matters is **where the node wants to escalate but cannot** —
tier-2 cycles with `pcs < 1.70` — taken from `results/claim2_rebase/propagation_evtrue`, 2128 such
cycles over 20 seeds.

Their distance from the nearest existing facility:

| p0 | p10 | p25 | **p50** | p75 | p90 | p100 |
|---|---|---|---|---|---|---|
| 4.7 m | 470 m | 563 m | **698 m** | 827 m | 914 m | 1327 m |

**99.91% lie beyond the 20 m UWB predict radius; 99.48% beyond the 150 m tier-2 ring.** Demand-weighted
k-means (k = 4, farthest-point seeding, deterministic) places complementary sites at the **edge
midpoints of the facility square** — (929, 56), (−14, 920), (−636, 84), (199, −860) — each ~750–930 m
from the nearest existing facility. That is the derived rule: *site the precision tier at the
demand-weighted centroids of the escalation-demand distribution.*

## The pre-flight, and it fires

**Even at optimal placement, four complementary sites bring only 2.26% of demand inside the 20 m
predict radius.** Scaling the site count:

| sites | demand ≤ 20 m | demand ≤ 150 m |
|---|---|---|
| **4** | **2.26%** | 26.55% |
| 8 | 4.61% | 53.99% |
| 16 | 10.29% | 87.59% |
| 32 | 13.82% | 97.46% |
| 64 | 33.41% | 99.86% |
| 128 | 63.16% | 100.00% |
| **256** | **99.48%** | 100.00% |

The demand spans a **1893 m** box, ~3.61 km². A 20 m-radius disc covers 0.00126 km². **Tiling that
area needs ~2874 discs.**

**Mechanism, named:** this is not a placement error that better placement fixes. **It is a scale
mismatch.** Confidence degrades over hundreds of metres — the median demand cycle is 698 m from any
anchor cluster — while the precision tier reaches tens of metres. The ratio is ~30–50×, so covering
the demand needs 10²–10³ times more sites than the deployment has.

> **Refined design rule, superseding the co-location statement:** a confidence-triggered escalation
> to a short-range precision tier is unsatisfiable whenever the spatial scale over which the
> coverage tier's confidence degrades exceeds the precision tier's range. **Co-location makes it
> worst, but re-siting cannot fix it** — the mismatch is one of scale, not placement.

## Registered predictions

| # | prediction | basis |
|---|---|---|
| **1** | **PCS repair alone produces no material change in tier-3 service.** | The coverage gate rejects those cycles by definition — they are the far-from-facility cycles. **This tests the diagnosis, not the architecture, and is reported first.** |
| **2** | **Complementary siting at k = 4 also produces no material change** — revised from "raises service materially" by the pre-flight above, which is recorded rather than the original being quietly replaced. | 2.26% of demand reaches the predict radius |
| **3** | Both together exceed either alone, but remain immaterial | the gates are in series and neither is removed |
| **4** | `static_2p4` gains little or nothing under complementary siting | it is not range-limited here; §13.3's registration is what scores this |

**Falsifier for the diagnosis:** prediction 1 fails — PCS repair alone materially raises tier-3
service. Then the coverage gate was not binding and the anti-correlation account is incomplete.

**Falsifier for the pre-flight:** prediction 2 fails — siting at k = 4 materially raises service.
Then the demand distribution measured on the co-located deployment does not transfer to the
re-sited one, which would itself be a finding about measuring demand under the treatment.

**Prediction 2 was reversed by the pre-flight before any cell ran.** The original expectation, from
the work order, was that siting alone would repair it. The pre-flight arithmetic says otherwise, and
the reversal is recorded here with its evidence rather than presented as the original prediction.

## The mediator — joint, because the finding is joint

**The joint distribution of PCS and UWB-anchor-count-within-20 m**, per cycle. PCS alone or anchor
count alone measures nothing about an anti-correlation. Specifically: the fraction of cycles in each
quadrant of (`pcs < 1.70`) × (`vis ≥ 3`). The repair works if and only if the
(**wants**, **can**) quadrant becomes non-empty.

Checked against what it measures: `vis` is recomputed from `estX`/`estY` against the *deployed*
anchor positions, matching `ca-tbl-policy.cc:1197-1215`, not from a trace column that might encode a
different radius.

## Design

2 × 2, one change per cell, everything else identical to `claim2_rebase`:

    siting  = colocated | complementary(k=4, at the derived centroids)
    pcs     = as-is | repaired
    --duration=3600 --boxHalf=2000 --gwGrid=5 --nlosSource=propagation
    --escalationRequiresEvidence=true --arms=ca_tbl_full,static_2p4
    seeds 1..20

**The PCS repair, chosen and justified.** `pcs = min(term1, term2, term3)` rather than the sum.
Reasons: it is **parameter-free** — this project has withdrawn one uncalibrated scalar and refuses
to ship a second, which rules out a weighting; and it gives a confidence score the semantics it
should have, *as weak as its weakest component*, so any single term can trigger escalation. The
threshold is derived rather than chosen: the sum's range is [0, 3] and `min`'s is [0, 1], so the
existing 1.70 is carried across as the **same fraction of range**, 1.70/3 = **0.5667**. No new
calibrated constant is introduced.

**`static_2p4` is included** so §13.3's registered prediction can be scored.

## Not re-registered

`Simulation.md` §13.3 already registered, before this outcome was knowable: *"if `static_2p4` gains
as much as the tiered arms, this is a **deployment improvement rather than an argument for tiering**,
and must be reported as such."* That is scored, not restated.

§13.3 also schedules tier-1 measurement calibration **before** siting, so that siting does not answer
a question a calibration fix already answers. That ordering concerns the **upper edge** at high
`spacing/r80`; this experiment concerns **tier-3 escalation** at a fixed geometry. Different
question, so the ordering is not violated — stated rather than assumed.

**Claim 9 is not touched.**
