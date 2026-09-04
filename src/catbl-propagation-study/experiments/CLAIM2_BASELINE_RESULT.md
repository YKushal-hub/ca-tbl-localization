# Claim 2's energy ratio, re-measured — result

Registered in `CLAIM2_BASELINE_REGISTRATION.md` before the first cell. 80 cells, zero failures.

**Provenance, disclosed.** Sidecars carry `186990f` and `01abbed`; `git diff` between them touches
**one `.md` file and no source**, so the binary is identical across all 80 cells. **One cell**
(`legacy_evfalse/seed_2`) records `dirty 1` — it ran while `DEFECTS.md` was edited and uncommitted.
That violates the `dirty 0` rule, it is a documentation file, and it is reported rather than
quietly tolerated.

## The result, with D1 and the NLOS source separated

| | ev=false | ev=true | ratio |
|---|---|---|---|
| pre-D1, legacy (**published**) | 622.925 | 24.260 | **25.68×** |
| post-D1, legacy | 640.771 | 27.094 | **23.66×** CI [23.39, 23.93] |
| post-D1, **propagation** | **149.524** | 22.755 | **6.53×** CI [5.11, 7.95] |

Registered band for a material change: outside [23.11, 28.25].

**D1: falsifier fires.** 23.66× is inside the band. The sensitivity correction does not reach this
comparison — it makes escalation more expensive on *both* sides of the gate, so the ratio survives.
The pre-D1 provenance on Claim 2 is a **record-keeping defect that did not change the number**.

That is worth stating precisely, because it narrows a rule this project applies as a default: *a
baseline predating D1 invalidates the ratio built on it* means **cannot be relied on without
re-measurement**, not **is necessarily wrong**. Tested here, it did not bite.

**NLOS source: prediction held, decisively.** 6.53× is far outside the band. The ratio falls to
**a quarter** of the published figure.

## The mediator: the baseline *is* the defect

The `ev=false` arm is the "before" number — the 622.92 J the whole claim is a ratio to:

| `ev=false` arm | legacy | propagation |
|---|---|---|
| `j_uwb` | **623.828 J** | 128.234 J |
| `cycles_uwb` | **1170.1** | 212.3 |
| `j_868` | 14.796 | 14.631 |
| `j_2p4` | 2.147 | 6.659 |
| **total** | **640.771** | **149.524** |

**Under legacy, 623.8 J of the 640.8 J baseline is UWB energy** — 97.4% of it — burned across 1170
cycles of escalation. Under the corrected source the same arm spends 128.2 J across 212 cycles.

So the "before" number is not a measurement of an ungated system. **It is a measurement of the
legacy NLOS conflation driving continuous escalation into the most expensive tier.** The evidence
gate then switches that off, and the ratio records how much defect it removed.

This is the third appearance of one mechanism, now in its clearest form. W11: the defect parked the
node on UWB and suppressed the floor, manufacturing an apparent saving. Claim 4: the saving halved
because there was less time off tier 1 to suppress. Here: the defect inflates the **baseline**
rather than the treatment, which is the same lever operating on the other side of a ratio.

## Consequence, applied as registered

> Ratio shrinks but stays large ⇒ Claim 2 **re-scoped**, as Claim 4 was — direction stands,
> headline restated.

**Claim 2 is not withdrawn.** A 6.53× energy reduction from gating escalation on evidence is a
real and large result, measured on a post-D1 binary at `dirty 0` (bar the one disclosed cell), with
the NLOS source stated rather than defaulted.

**The headline is restated: 6.53× [5.11, 7.95], not 25.68×.**

The interval is wide because the ratio's numerator varies strongly across seeds once the defect is
removed — which is itself informative: the ungated system's cost is dominated by how often
escalation happens to fire, and that is far more variable than the legacy figure suggested.

## Scope

One geometry, `boxHalf=2000`, matching the published cell. Claim 2's second reported quantity —
escalation efficiency 0.071 → 0.827 — is **not** re-measured here and remains on pre-D1,
defaulted-legacy evidence.
