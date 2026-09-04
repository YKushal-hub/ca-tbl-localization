# Does Claim 4's coverage-floor saving survive the corrected NLOS source? — pre-registration

Written and committed **before the first cell**. A genuine pre-registration: these cells do not
exist.

## Exposure, established from the sidecar

Claim 4 is PROVEN on `[M-16]`, which cites `PREDICTION_P1_v2.md`. That file lives in
`results/p1_condfloor/`, which identifies the directory. Its sidecars carry **no `--nlosSource`
flag**, and the binary's default is `legacy` (`ca_tbl_causal_sim.cc:3540`).

**Claim 4 inherited the defect without ever naming it.** That is the exposure finding in one line,
and enumerating explicit `--nlosSource=legacy` directories would have missed it.

## Why the naive asymmetry argument does not clear it

Claim 3 was exposed because `static_2p4` has no tier 3 and could not move, so the flag hit only the
numerator. Claim 4 compares `floorOnDemand=true` against `false` and **both arms are
`ca_tbl_full`**, so there is no structurally immune denominator. That makes Claim 4 less exposed to
W11's *specific* asymmetry.

**But Claim 4's claim is a magnitude, and the magnitude is proportional to the quantity the defect
inflated.** `floorOnDemand` suppresses the tier-1 floor *while a higher tier is serving*, so the
saving it buys is proportional to **time spent off tier 1**. W11 measured exactly that quantity:

| | legacy | propagation |
|---|---|---|
| cycles on UWB | 553 of 1440 | **0** |
| 868 MHz energy | 3.427 J | 9.093 J |

A mechanism that pays out per cycle-off-tier-1 has far fewer cycles to pay out over under the
corrected source. **A surviving ratio would not mean a surviving magnitude.**

## Registered prediction

> **Claim 4's saving shrinks materially under `propagation`**, in proportion to the reduction in
> time spent off tier 1. The direction may survive; the published **−43%** will not.

**Falsifier.** The saving is unchanged in magnitude. Then it does not come from suppressing the
floor during higher-tier occupancy, the mechanism account is wrong, and that needs explaining
before Claim 4 is kept or moved.

## The mediator is measured, not only the endpoint

**`cycles_868` per arm under both sources**, reported alongside the energy. This is what makes the
test a mechanism test rather than a re-measurement: the prediction is that the endpoint moves
*because* the mediator moves, and both are observable. W11's dwell check was free for the same
reason — an account that says where an effect should vanish can be checked where it should vanish.

## Design

Four cells, everything from Claim 4's own sidecar except the two flags under test:

    --duration=3600 --boxHalf=1500 --gwGrid=5 --escalationRequiresEvidence=true
    --floorSkipK=1 --floorHeartbeatN=0 --trace=true
    --floorOnDemand={true,false}  x  --nlosSource={legacy,propagation}
    seeds 1..20

`boxHalf=1500` is the `spacing/r80 = 0.78` density, the first of Claim 4's two. 20 seeds to match
the published depth — not a power calculation, because this re-measures a published quantity and
changing the depth would confound the flag with the sample size.

**The legacy arm is re-run rather than taken from disk**, so the binary confound is ruled out by
construction: all four cells come from one binary.

## Consequences, fixed now

| outcome | consequence |
|---|---|
| saving shrinks materially, stays positive | Claim 4 **survives, re-scoped** to the corrected source with its true magnitude. The 12/12 predictions were about direction and are unaffected; the headline number is restated |
| saving vanishes or reverses | Claim 4 **withdrawn as W12**, W11 style, original text retained, `[M-16]` annotated not deleted |
| saving unchanged | **Falsifier fires.** Reported as such — an unchanged saving contradicts the parking mechanism, and both cannot be right |

**Claim 9 is not touched.**
