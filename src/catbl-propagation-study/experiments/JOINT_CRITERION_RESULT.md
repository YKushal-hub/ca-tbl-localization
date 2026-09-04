# The joint criterion — result

Registered in `JOINT_CRITERION_REGISTRATION.md` (`ec48e90`) before the first cell, on seeds the
observation never touched. **52 cells, seeds 21–72, `source ec48e90 dirty 0` uniform, zero
failures.** Scored once.

## The registered report

> At `boxHalf = 2000`, `nlosSource = propagation`, evidence gate on, PCS `min`: the repaired tiered
> arm meets (A = 0.95, R = 25 m) in **52 of 52** deployments against `static_2p4`'s **52 of 52**, at
> **2.17×** its energy. Whether that trade is worth making is a deployment decision.

No PASS, no FAIL attached to that sentence, as registered. The separate `b49df46` §4 verdict is
below and is reported as its own thing.

| | availability | 95% CI | meets | `totalJ` | 95% CI |
|---|---|---|---|---|---|
| `ca_tbl_full`, repaired | 0.9984 | [0.9975, 0.9993] | **52/52** | 26.795 | [26.55, 27.04] |
| `static_2p4` | 0.9968 | [0.9955, 0.9980] | **52/52** | 12.389 | [12.14, 12.64] |

## The reliability advantage did not reproduce

**This is the result.** The observation that motivated the whole registration was 20/20 against
19/20 — the tiered arm meeting the requirement *more often*. On 52 fresh seeds **both arms meet it
on every seed.** The one-seed margin was a property of the 20-seed set, not of the arms.

The availability difference does reproduce, and is much smaller:

| | paired delta | 95% CI |
|---|---|---|
| observation, seeds 1–20 | **+0.007222** | — |
| **registered, seeds 21–72** | **+0.001629** | **[+0.000251, +0.003008]** |

**The CI excludes zero, so the lead is real — and it is 4.4× smaller than the estimate that
motivated measuring it.** That is the winner's-curse caveat resolving in the direction it was
registered to warn about, the second time in this project after `h1_power` (+0.0053 → +0.0037).
Registering the caveat before running is what makes this a confirmation of a smaller effect rather
than a surprise.

## The `b49df46` §4 verdict, reported separately: **FAIL**

Both arms now meet the requirement on all 52 seeds, so `static_2p4` **qualifies as a competitor** —
which it did not at 20 seeds — and the rule can adjudicate where it previously returned
INCONCLUSIVE:

    M1 = 2.1673, seed-level bootstrap 95% CI [2.1423, 2.1928], entirely above 1.0
    FAIL -- a qualifying single-band arm achieves lower M1

**The criterion that could not express the advantage at 20 seeds can express it at 52, because the
advantage it could not express turned out not to exist.** `[M-33]`'s finding stands as a statement
about the rule's structure — it *is* blind to a reliability advantage — but the instance that
demonstrated it has dissolved, and that is recorded here rather than left standing.

## The one registered prediction: **HELD**

`j_2p4` per tier-2 cycle within ±10% of 0.00889: **measured 0.00867**, inside [0.00800, 0.00978].
The occupancy account — the repair buys availability with tier-2 residency at a flat per-cycle
cost, not with refused-attempt overhead — reproduces on fresh seeds.

## What this settles

**The repair is real, its benefit is small, and its price is 2.17×.** The predicate defect was
genuine and fixing it restored tier-3 service from zero; but at this configuration the repaired
tiered arm buys **+0.0016 availability** over a single-band arm that already meets the requirement
on every seed, for **more than twice the energy**.

**Nothing here supports the architecture at `R = 25 m`.** It is the same conclusion the ledger
already carries from Claim 3's withdrawal and Claim 9's falsification, reached from a new direction
and on prospectively-registered fresh seeds — which makes it the most soundly-evidenced negative in
the project.

**What it does not settle:** whether the architecture pays at a tighter requirement. Every result
here is at `R = 25 m` against a tier-2 arm delivering 2.73 m CEP50 — a 9× margin. The registered
report format now exists and could be run at a smaller `R` without inventing a rule, and that is the
obvious next experiment.
