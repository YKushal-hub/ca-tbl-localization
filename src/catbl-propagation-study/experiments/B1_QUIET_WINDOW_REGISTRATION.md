# Quiet-window cost, and delivery rates across seeds — pre-registration

Written and committed **before the first cell**. Nothing here is revised after a result exists.

## Why both are in one registration

§3 puts a confidence interval on the delivery rates because **B2's round count inherits them**,
and the current basis is n = 120 per link on a single seed. §2 measures what a quiet window costs.
They share a campaign: the same runs, beacons on and off, answer both.

## Design

    seeds        1..5                          registered here, run exactly, scored once
    duration     3600 s, dt 2.5 s              1440 epochs
    survey       --anchorRangePeriodS=2.5 --anchorPairsPerEpoch=1 --anchorMaxConcurrent=1
    conditions   --beaconUwb=2.0   (default, beacons on)
                 --beaconUwb=100000 (quiesced)
    arms         ca_tbl_full
    cells        5 seeds x 2 conditions = 10

Five seeds, not more: the adjacent effect measured on one seed was **+0.1833**, roughly ten times
the across-ring delta that was labelled noise. An effect that large does not need forty seeds to
separate from zero, and the purpose here is an interval on a rate rather than a hypothesis test.
**If five seeds do not separate it, that is the finding and the count is not extended.**

## §3 — registered

**Prediction.** UWB adjacent delivery with beacons quiesced is materially above the
beacons-on rate, with the two 95% intervals **not overlapping**. UWB across-ring shows **no
material change** — its intervals overlap. Tier 2 is unchanged.

**Falsifier.** Overlapping intervals on adjacent ⇒ the single-seed +0.1833 was seed-specific and
the beacon-contention account needs withdrawing.

**The control, and it is the load-bearing part.** Tier-2 delivery must be **unchanged** between
conditions: UWB beacons cannot reach a 2.4 GHz link. A change there means something other than
beacon contention moved, and the whole account fails regardless of what adjacent does. This
control was not constructed for the purpose — it is already in the survey — which is what makes
it worth more than one that was.

## §2 — registered

**Prediction.** Suppressing UWB beacons degrades **tier-3-served** cycles — availability down,
`cep50M` up — while leaving **tier-2-served** cycles unchanged. Same asymmetry, used as the
control rather than as the signal.

**Falsifier.** Tier-2-served cycles move materially ⇒ suppression is not acting through tier-3
discovery and the cost attribution is wrong.

**Not predicted:** the magnitude. There is no prior estimate of what losing tier-3 discovery costs
availability, and inventing one to score against would be predicting the answer.

**Reported as a rate, not a total**, so it composes with §4's round count: *calibration costs X
availability for Y seconds per round, and needs Z rounds.*

## Scoring

Delivery rates as mean over 5 seeds with a 95% interval from the per-seed spread. Solvability
recomputed at the **interval bounds**, not only the point estimate, so B2's round count arrives
as a range. A point estimate that later needs withdrawing is the failure this avoids.
