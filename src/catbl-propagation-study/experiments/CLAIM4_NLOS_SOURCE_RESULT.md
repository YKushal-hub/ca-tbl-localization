# Claim 4 under the corrected NLOS source — result

Registered in `CLAIM4_NLOS_SOURCE_REGISTRATION.md` before the first cell, amended before scoring
to restrict the arm set (justified by a byte-identity test, recorded there). 80 cells, `dirty 0`,
zero failures.

**Provenance note.** Sidecars carry two commits — `12a4b3b` and `7ac8a32` — because the amendment
was committed while the campaign ran. `git diff` between them touches **one `.md` file and no
source**, so the binary is identical across all 80 cells.

## The prediction: HELD

> Claim 4's saving shrinks materially under `propagation`, in proportion to the reduction in time
> spent off tier 1. The direction may survive; the published −43% will not.

| source | absolute saving | relative saving |
|---|---|---|
| **legacy** | **12.107 J** [11.944, 12.271] | **39.9%** [39.4, 40.4] |
| **propagation** | **5.974 J** [5.745, 6.203] | **24.5%** [23.6, 25.3] |

**The saving halves.** Intervals are nowhere near overlapping. The direction survives — the saving
is real and positive under the corrected source — and the magnitude does not.

## The mediator confirms the mechanism, and it is not a coincidence of two numbers

| source | | fod=false | fod=true | delta |
|---|---|---|---|---|
| legacy | `j_868` (floor energy) | 15.358 | **3.334** | −12.024 |
| legacy | `cycles_uwb` | 102.9 | 104.8 | — |
| propagation | `j_868` | 15.156 | **9.183** | −5.972 |
| propagation | `cycles_uwb` | **0.000** | **0.000** | — |

Three things line up:

1. **The saving *is* the floor.** Under legacy the total saving is 12.107 J and the floor-energy
   drop is 12.024 J — **99.3% of it**. Under propagation, 5.974 J against 5.972 J — **100.0%**.
   `floorOnDemand` does nothing else; whatever it saves, it saves by not running tier 1.
2. **The floor can only be suppressed while a higher tier is serving**, and under propagation the
   node reaches UWB on **zero** cycles against 102.9 under legacy. With less time off tier 1 there
   is less floor to suppress: the floor falls to 3.334 J under legacy but only to 9.183 J under
   propagation.
3. **The `fod=false` baselines differ too** — 30.379 J against 24.373 J — for the same reason W11
   found: under legacy the node parks on an expensive tier, which raises its total independently.

## Correction to the registration's mediator, disclosed rather than substituted

The registration named **`cycles_868`** as the mediator. **It is the wrong column.** It moves
1.7 → 2.1 between the arms — essentially not at all — because it counts cycles where tier 1 was
the *active* tier, and the floor runs *concurrently beneath* a higher tier without ever being
"active". The floor's real footprint is **`j_868`**, and that is what is reported above.

The prediction is unchanged: it was always that time-off-tier-1 drives the saving. Only the
instrument named for it was wrong, and it was wrong in the direction that would have made the
mechanism look absent. Had `cycles_868` been reported as registered, the mediator would have shown
no movement while the endpoint halved, and the honest reading of that would have been "the
mechanism account is wrong" — a false conclusion from a correct procedure. **A registered mediator
is a measurement instrument like any other, and naming it in advance does not exempt it from being
checked against what it measures.**

## Consequence, applied as registered

> Saving shrinks materially but stays positive ⇒ Claim 4 **survives, re-scoped** to the corrected
> source with its true magnitude. The 12/12 predictions were about direction and are unaffected;
> the headline number is restated.

**Claim 4 stands.** It is not withdrawn: coverage-floor optimisation does reduce energy, the
reduction is large, and the mechanism is now measured rather than inferred — 99–100% of the saving
is floor energy, in both sources.

**The headline magnitude is restated.** On a like-for-like pair measured here, the saving is
**24.5% [23.6, 25.3]** under the corrected source against **39.9%** under the defective one.

**What is not claimed:** that the published "42.8%" becomes exactly 24.5%. The published baseline
(28.15 J) is not the `fod=false` arm measured here (30.379 J) — it comes from a directory the
ledger does not name, which is the provenance gap recorded separately. What is measured is the
*ratio between the two sources on one consistent pair*, and that is the quantity the correction
governs.

## Scope

One density, `boxHalf=1500`, `spacing/r80 = 0.78`. Claim 4's second density (1.29) is untested
under the corrected source. Nothing here touches Claim 9.
