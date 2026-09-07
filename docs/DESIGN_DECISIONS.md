# Design decisions not specified by the project brief

**2026-09-07.** The owner's instruction: *every node carries all three bands; there is a
lost node, fixed gateways, and finder nodes; only gateways are immobile; everything else
follows from that — and **any design decision not in that brief has to be documented**.*

This file is that record. Each entry states the choice, what it was chosen over, and why
— so a reader can disagree with a specific decision instead of with the whole model.

---

## 0. The error this corrects, first

The simulator modelled **three disjoint anchor populations**, one per tier, in different
places: tier-1 gateways on a `gwGrid × gwGrid` array, tier-2 anchors ringed at 150 m
around four facility sites, tier-3 anchors ringed at 10 m around the same sites.
`Architecture.md` §140–143 specified it that way too, so this was not code drifting from
a spec — **the spec had it wrong as well.**

The system has **one population of tri-band nodes**. A tier is a choice of *which radio
to use*, not of *whom to talk to*.

**Why it mattered, concretely.** Under the old model the lost node reached UWB by
wandering near a facility. Under the corrected one, UWB (tens of metres) barely reaches
from a fixed gateway at all — measured mean UWB neighbours **0.01** with no finders. The
old topology manufactured tier-3 coverage that the architecture is supposed to *earn*,
and in doing so it hid the mechanism the whole design rests on: a finder carries tier 3
to the casualty.

**Blast radius, stated plainly.** This changes the deployment, so it changes every
result. `baseline_v7` does not reproduce under the corrected topology and is not
supposed to. The legacy model is retained behind `--topology=1` **only** so results
published before this date remain reproducible for the record; it does not describe the
system and is not a configuration anyone should run for new work.

---

## 1. Finder mobility — converge on the estimate

**Decision.** `--finderMode=1` (default): finders move toward the system's **current
position estimate** of the lost node at walking speed, standing off 8 m once inside it.

**Over:** touring the facility sites like any other field node (`--finderMode=0`), which
is what the code did when finders were first added and which produced almost no tier-3
contact (mean UWB neighbours 0.15 against 0.78 converging).

**Why.** The purpose of localising a casualty is to send someone there. A finder that
wanders is not a searcher. Critically it converges on the **estimate**, not the true
position — nobody knows the true position — so a bad fix sends searchers to the wrong
place and the cost of poor localisation appears as distance walked rather than only as a
number in a table.

**Not in the brief.** The brief says finders move; it does not say how. This is the
strongest modelling assumption in the corrected system and the one most worth arguing
with.

**Known limitation.** All finders converge on the same point. Real search teams spread
to cover an uncertainty region, so this is optimistic about contact and pessimistic about
area coverage. A dispersal model keyed to the covariance is the obvious next step.

## 2. The 8 m stand-off

**Decision.** A converging finder stops 8 m from the estimate.

**Why.** Without it the finder oscillates across the estimate every cycle, which both
looks wrong and repeatedly breaks the short-range link the approach exists to establish.

**Not in the brief. The constant is ours** and is not calibrated against anything.

## 3. Finders take the 2.5 % duty cap, not 10 %

**Decision.** `isAccessPoint = false` for finders.

**Why.** The context document is explicit that under G.S.R. 853(E) *"gateways/anchors
(your network access points) get a 10 % duty cycle budget, field nodes get 2.5 %."* A
finder is a person carrying a radio, not installed infrastructure. Getting this backwards
would hand searchers four times the airtime they are entitled to.

**Derived from the brief rather than chosen**, but recorded because it is a judgement
about which category a finder falls into.

## 4. Finders are modelled as mobile gateways

**Decision.** A finder is registered on all three tiers exactly like a gateway, beacons
on all three, and is discovered and ranged through the ordinary neighbour table. The only
difference from a gateway is that its position changes.

**Why.** It requires no special case in the ranging, energy or MAC paths, so a finder
cannot accidentally get accounting a gateway would not. It also matches the brief: the
only stated difference between the node classes is mobility.

**Consequence not in the brief.** The lost node ranges *to* finders. Finder-initiated
ranging, and finders relaying to gateways, are not modelled.

## 5. Gateway placement and count

**Decision.** Gateways stay on the existing `gwGrid × gwGrid` array.

**The brief says 3 gateways, 5 field nodes, 5 km radius.** The default here is
`gwGrid=3`, i.e. **9** gateways over a 2 km box. **This is a divergence and it is not
resolved.** Reaching the brief's figures means `gwGrid` cannot stay a square array —
three gateways is not a square number — so it needs explicit placement, which is not
built.

**Anyone quoting a gateway count should read it off `deployment.json`, not off the brief.**

## 6. Facilities remain, as places rather than infrastructure

**Decision.** The four facility sites still exist and the lost node still dwells at them,
but they no longer host anchors.

**Why.** They were doing two jobs: mobility targets and anchor sites. The second was part
of the error. The first is a reasonable model of a search area with huts, gates and
checkpoints, and removing it would change the mobility as well as the topology, confounding
two changes in one step.

## 7. Node id ranges

Gateways `101+`, lost node `1`, peer/relay tags `2+`, finders `501+`. Arbitrary, but
fixed, so a reader can identify a node class from its id alone in the traces and in the
packet log.

---

## Still unmodelled, and named so the gap is visible

- **Gateways as edge servers.** The brief has gateways performing EKF fusion and NLOS/hop
  computation rather than the battery-constrained field node. Here the field node runs its
  own filter. This changes *where* computation happens, not what is computed, so no result
  moves — but the energy story for the field node is optimistic if the brief's split is
  the intended one.
- **Finder-to-gateway relaying.** Finders do not carry the casualty's fixes back to
  infrastructure; they are ranging targets only.
- **A still lost node.** Supported via `--speed=0`, not yet exercised as a scenario.
- **5 km radius.** Untested; all corrected-topology runs so far are at `boxHalf=1000`.
