# Does Claim 3's only PASS survive the corrected NLOS source? — pre-registration

Written and committed **before the first cell**. Nothing here is revised after a result exists.

## Why

The tier-occupancy census (`TIER_OCCUPANCY_CENSUS.md`) found that `nlosSource=legacy` —
`(nlosDb > 0) || (hiddenState > 0)` at `ca_tbl_causal_sim.cc:416-423`, the conflation this project
already records as a defect — inflates tier-3 **service** tenfold at fixed dwell and box size:

| configuration | uwb occupancy | uwb fix share |
|---|---|---|
| `c_ef0_legacy_dw800_b1500` | 32.8% | 31.6% |
| `c_ef3_propagation_dw800_b1500` | 3.1% | 3.3% |

**Claim 3's replacement entry is that first cell.** `RURAL_OPEN, nlosSource=legacy, dwell=800,
boxHalf=1500`, M1 = 0.9531 [0.9296, 0.9775] — the only PASS in 320 cells.

## Why it is not obviously invalid, and not obviously fine

Both arms ran under the same flag, and M1 is an energy comparison at equal reliability, so the
defect is not *obviously* asymmetric. But:

- `static_2p4` **has no tier 3**. Legacy's inflated escalation cannot touch its energy.
- `ca_tbl_full` escalates to tier 3 on **31.6%** of cycles under legacy and **~3%** under
  propagation. Tier 3 is the most expensive tier in the project.

The M1 ratio is `ca_tbl_full / static_2p4`. **A flag that moves the numerator's most expensive
tier by an order of magnitude and cannot move the denominator is the shape of W1** — a defect's
effect attributed to the design.

## Design

One cell, everything identical to the published one except the flag under test:

    --duration=3600 --boxHalf=1500 --gwGrid=5 --escalationRequiresEvidence=true
    --floorOnDemand=true --trace=true --envFacility=0 --dwell=800
    --nlosSource=propagation          <-- the only change, was `legacy`
    seeds 1..20, all six arms, as the original

20 seeds because the published result is 20 seeds and the comparison must be like-for-like. Not a
power calculation: this re-measures a published quantity under one changed flag, and changing the
seed count would confound the flag with the sample size.

## Registered criterion — `b49df46` §4, verbatim

> **PASS** — `ca_tbl_full` meets (A, R) AND its M1 is lower than every single-band arm that also
> meets (A, R), with the geometric mean of per-seed ratios < 1.0 and the seed-level bootstrap 95%
> CI entirely below 1.0.
> **FAIL** — `ca_tbl_full` meets (A, R) but some single-band arm that also meets (A, R) achieves
> lower M1, CI entirely below 1.0 in that direction.
> **INCONCLUSIVE** — the CI spans 1.0, or no arm other than `ca_tbl_full` meets (A, R), or fewer
> than 2 testable geometries exist.

A = 0.95, R = 25 m. M1 is `totalJ`. Estimator: geometric mean of per-seed ratios, seed-level
bootstrap 95% CI, 20 000 resamples.

**The scorer is validated before use.** `scripts/claim3_m1.py` reproduces the published cell
exactly: 0.9531, CI [0.9296, 0.9772] against the published [0.9296, 0.9775] — the third decimal of
the upper bound differs by bootstrap randomness — with mean energies 14.523 J and 15.201 J
matching to the digit, and 20/20 on both arms.

## No prediction is registered

Three premises have dissolved in five passes. This is a check, not a hypothesis. The criterion is
registered; the outcome is not.

## Consequences, fixed now so the result cannot be re-read afterwards

| outcome | consequence |
|---|---|
| M1 below 1.0, CI clear | The PASS **survives the flag** and is stronger for having been tested against it. The test and its result are recorded beside the entry |
| M1 crosses 1.0 | **The project's only PASS depends on a documented defect.** Withdrawn with its reason in the W1 style — *"measured under an NLOS source that inflates tier-3 escalation tenfold"* — original text retained, and Claim 3 returns to withdrawn-with-no-replacement |
| CI straddles 1.0 | Reported as **unresolved**, with the seed count that would resolve it stated. Not resolved by choosing a reading |

Note that INCONCLUSIVE has a second route here that did not apply to the original: if
`static_2p4` no longer meets (A, R) under the corrected source, there is nothing to compare
against, and the rule makes that INCONCLUSIVE rather than PASS by explicit design.

**Claim 9 is not touched.** This concerns Claim 3.
