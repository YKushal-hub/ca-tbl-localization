# Is the anti-correlation repairable? — result

Registered in `SITING_REPAIR_REGISTRATION.md` before the first cell. 80 cells, 20 seeds × 4,
zero failures, **`source 56ec9b6 dirty 3` uniformly across all 80** — the engine change was held
uncommitted for the campaign's duration so provenance would not split mid-run, and committed
immediately after as `293dc3a`.

## Prediction 1 — the diagnosis test — **FALSIFIED**

Registered: *PCS repair alone produces no material change in tier-3 service, because the coverage
gate rejects those cycles **by definition**.*

| cell | UWB cycles/seed | 95% CI | seeds > 0 |
|---|---|---|---|
| co-located, PCS as-is (**control**) | **0.0** | [0.0, 0.0] | 0/20 |
| **co-located, PCS repaired** | **83.0** | [76.9, 89.2] | **20/20** |
| complementary siting, PCS as-is | 0.5 | [−0.2, 1.2] | 2/20 |
| both | 13.4 | [8.4, 18.5] | 13/20 |

Paired delta from the repair: **+83.0, CI [+76.9, +89.2]**. Not a material change — a change from
nothing to something, on every seed.

**The words "by definition" were the error.** I measured that 100% of PCS-permitting cycles were
far from facilities, and treated that as a property of *wanting to escalate*. It was a property of
**what the sum form measures**.

## Prediction 2 — **HELD**

Siting alone: 0.0 → 0.5 cycles, CI [−0.2, +1.2], contains zero. The pre-flight arithmetic — four
sites reaching 2.26% of demand — was right, and the reversal recorded in the registration before
running was the correct call.

## Prediction 3 — **FALSIFIED, in the opposite direction**

Registered: both together exceed either alone. Measured: **both (13.4) is far worse than the repair
alone (83.0)**, paired delta −69.6, CI [−76.7, −62.5].

## Why: the anti-correlation was a property of the predicate, not the geometry

Separating escalation **events** from UWB **occupancy** — occupancy is a dwell, not an event:

| cell | events/seed | cycles/seed | cycles/event |
|---|---|---|---|
| co-located, as-is | 0.00 | 0.0 | — |
| **co-located, repaired** | **2.15** | 83.0 | **38.6** |
| complementary, as-is | 0.10 | 0.5 | 5.0 |
| complementary, repaired | 0.85 | 13.4 | 15.8 |

**Complementary siting reduces both the opportunity count and the dwell.** That is the inversion:

- The **sum** form's `term3 = nLos/nTotal` falls where links are NLOS, which is **far from anchors**.
  A predicate triggering on it wants to escalate exactly where the precision tier is absent — hence
  the anti-correlation, which is real for that predicate.
- The **min** form is dominated by `term1 = traceRef/(traceRef + traceSigma)`, i.e. **filter
  uncertainty**. That is highest where the node **dwells**, and the node dwells at facilities —
  which is where the co-located anchors are.

> **Co-location is correct for a predicate that triggers on filter uncertainty and self-defeating
> for one that triggers on LOS fraction. The anti-correlation was never a property of the
> deployment; it was a property of what the confidence score measures.**

That supersedes the design rule in `ESCALATION_PREDICATE_DIAGNOSIS.md` §4 and in
`SITING_REPAIR_REGISTRATION.md`, both of which are corrected rather than deleted.

## The mediator was the right quantity at the wrong unit

Registered: the joint distribution of PCS × coverage, as quadrant fractions over tier-2 cycles.

| cell | W&C | W&!C |
|---|---|---|
| co-located, as-is | 0.01% | 11.09% |
| co-located, repaired | **0.16%** | 51.36% |
| complementary, as-is | **0.27%** | 11.37% |
| complementary, repaired | 0.11% | 51.79% |

**The quadrants do not rank with the outcome.** `complementary_asis` has the *largest* W&C (0.27%)
and near-zero service; `colocated_min` has 0.16% and 83 cycles. The mediator is joint, as required,
but measured **per tier-2 cycle** — and occupancy is not a per-cycle quantity. Two escalation events
per seed produce 83 cycles because each is followed by a 38.6-cycle dwell.

**Third instance of a mediator failing on its unit or definition rather than its choice**, after
`cycles_868` (active-tier cycles, not floor operation) and this one. The rule stands and needs a
clause: *name the mediator, check it against what it measures — and check that its **unit** matches
the endpoint's.*

## `static_2p4` — scoring `Simulation.md` §13.3's existing registration

> *"If `static_2p4` gains as much as the tiered arms, this is a deployment improvement rather than
> an argument for tiering, and must be reported as such."*

Gain from co-located → complementary siting, paired:

| arm | PCS | delta | 95% CI |
|---|---|---|---|
| `ca_tbl_full` | as-is | +0.0023 | [−0.0006, +0.0052] |
| `ca_tbl_full` | repaired | −0.0004 | [−0.0018, +0.0011] |
| `static_2p4` | either | +0.0011 | [−0.0004, +0.0026] |

**The registered condition does not arise: nobody gains materially from siting.** Every interval
contains zero. §13.3's prediction was conditional on a gain that did not occur, so it is scored as
**not triggered** — which is not the same as failing, and is reported as such.

## Unregistered, and labelled as such

The PCS repair raises `ca_tbl_full` availability from **0.9852 → 0.9989** while `static_2p4` is
unaffected at 0.9917. Paired, `ca_tbl_full − static_2p4`:

| cell | delta | 95% CI | |
|---|---|---|---|
| co-located, as-is | −0.0065 | [−0.0152, +0.0022] | not distinguishable |
| co-located, repaired | **+0.0072** | [−0.0009, +0.0153] | **not distinguishable** |

The repair moves the tiered arm from behind to ahead, but **both intervals contain zero and no
claim is available.** This is the ledger's question, no prediction was registered on it, and it is
not being converted into one after the fact. It is the obvious next registration.

## What this does and does not establish

**Establishes:** the escalation predicate is repairable, by a parameter-free change, and the repair
restores tier-3 service from zero on 20/20 seeds. The architecture's third tier is **not**
unreachable by construction — my diagnosis said it was, and that is withdrawn.

**Does not establish:** that the repaired system is better. Availability is not distinguishable
from a single-band arm, and the **energy cost of 83 UWB cycles per seed is unmeasured here**. A
repair that buys tier-3 service at a worse energy ratio would re-open Claim 2's question, not
answer it.
