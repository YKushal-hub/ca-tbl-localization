# h = 1 at adequate power — pre-registration

Written and committed **before the first cell**. Nothing in this file is revised after a
result exists.

## Why re-run a cell that has already been measured

`hsweep` measured `h = 1` at 20 seeds and got advantage **+0.0053**, 95% CI ±0.0051, lower
bound **+0.0002**, 13 of 20 seeds positive. It was scored HELD in direction and marginal in
magnitude, and that is the honest reading of an interval whose lower bound is two ten-thousandths
above zero.

Retrospective power of that run, at the effect and spread it measured:

    SE = 0.0117 / sqrt(20) = 0.00262     half-width = 0.00513     power = 0.53

**A coin flip.** The single result the whole item-B build order rests on was measured by a
design that would have missed it about half the time. That is the reason to re-run, and it is
the only reason — the point estimate is not in dispute.

## Power calculation

Paired design, two-sided α = 0.05, target power 0.80.

    n = (z_0.975 + z_0.80)^2 * (sigma / delta)^2
      = (1.9600 + 0.8416)^2 * (0.0117 / 0.0053)^2
      = 7.8489 * 4.8733
      = 38.25   ->   39 seeds minimum

    n = 39   SE = 0.00187   half-width = 0.00367   power = 0.808
    n = 40   SE = 0.00185   half-width = 0.00363   power = 0.817

`sigma = 0.0117` is the per-seed standard deviation of the **paired difference**, taken from
`hsweep/RESULT.md`. It is internally consistent with that run's reported interval:
1.96 × 0.0117/√20 = 0.00513, which is the ±0.0051 published.

## Registered seed count: **40**

Not 39. 40 is within 1% of the same power, and seeds 1–40 make the first twenty
**byte-comparable with `hsweep/h1_*`**, which turns the overlap into a reproduction check
rather than a coincidence. Run exactly 40. Score once. Do not extend, whichever way it falls.

## The caveat that has to be registered, not discovered

**δ = 0.0053 is very likely an overestimate, and the power above is therefore optimistic.**
`h = 1` was not chosen in advance. It was selected as **the one point of seven** whose interval
excluded zero, and an effect size conditioned on having reached significance is biased upward —
the winner's curse. The true effect may be materially smaller, in which case 40 seeds gives less
than 80% power.

Consequences, fixed now:

- A **positive** result at 40 seeds is a real confirmation, and is not weakened by this caveat.
- A **null** result at 40 seeds is **not** evidence that exclusion does not help at `h = 1`. It
  is evidence that the effect is smaller than the selected estimate. It must be reported that
  way, and it must not be rescued by adding seeds.

## Design

    h              1
    policies       none, exclude
    arms           ca_tbl_full, ca_tbl_no_integrity   (both arms per invocation)
    seeds          1..40
    cells          40 seeds x 2 policies = 80 invocations
    outdir         results/h1_power/h1_{none,exclude}_seed{N}

All flags identical to `hsweep`, which is identical to `c9_clean` / `excl_test`:

    --duration=3600 --boxHalf=2000 --gwGrid=5 --escalationRequiresEvidence=true
    --nlosSource=propagation --pcsForm=v2 --suspectForcesEscalation=false
    --mahalUseR=true --injectRangeBias=40 --injectBiasFraction=0.2
    --injectFaultAnchors=1 --faultMultiplicity=1 --trace=true

Estimator, unchanged: fraction of 1440 cycles with a fix whose `errM <= 25 m`.

## Two questions, scored separately

**Question 1 — does `exclude` beat `none`?**
Registered prediction: **yes**, advantage positive with the 95% CI excluding zero.
Falsifier: interval containing zero ⇒ the `h = 1` advantage does not survive adequate power,
and item B's stage B4 must be built knowing that its response policy is unproven.

**Question 2 — does `ca_tbl_full` beat `ca_tbl_no_integrity`?**
**No prediction is registered.** That is the ledger's question and predicting it would be
predicting the conclusion. It is scored, reported, and not forecast.

If question 2 comes out positive it is worth **+0.0025 availability** on the scale this run
measures. It adds a scope line to Claim 9. **It does not overturn a falsification measured at
`h = 4` and `h = 12`,** and it will not be written up as though it does.

## Pre-flight, before spending the full count

Per-cell wall time measured on the **first five cells of this registered design** — not a
separate probe — and the projection reported before the remaining 75 are launched. The last two
such estimates were 7× low and off by an order of magnitude, so the estimate is measured, not
guessed. The seed count does not change on the basis of what the projection says.

## Provenance

`dirty 0` at launch, verified again in the sidecars at the end. No simulator source has changed
since the binary was built at `a3b468e` — the only commits since touch `.md` files and gate
scripts. Nothing under the dirty counter's scope is edited while this runs.
