# B1 — anchor-to-anchor ranging: implementable, but not "not new physics"

Written before the patch. It revises the work order's framing of B1 on evidence from the source.

## The stated premise, and what the source says

The work order: *"Anchors already carry tier-2 and tier-3 radios. This is a **new measurement
graph**, not new physics: anchors range to each other using the existing ranging code."*

`ca_tbl_causal_sim.cc:447-455` says the opposite, in capitals, as a documented scoping decision:

> **ANCHOR-TO-ANCHOR LINKS DO NOT EXIST.** Only the field node is registered as a node on the
> engines; anchors are registered as gateways on their own tier's engine only. A pair of anchors
> has no evaluable link, so `Evaluate()` returns "not received" for it. […] every anchor is a
> HIDDEN TERMINAL to every other anchor. Their CAD never detects each other, so they transmit
> into each other freely […] That is a harsher contention regime than a real deployment with
> inter-anchor audibility, so **beacon loss here is an upper bound, not an estimate.**

## What is actually true, checked rather than inferred

The limitation is in the **scenario**, not the engine. `ProcessTransmission(txNodeId, rxGatewayId,
txPos, rxPos, …)` takes both endpoint positions explicitly and keys its per-link state on
`MakeLinkId(txId, rxId)` (`lora-2p4-physics-engine.cc:131`, `:311`), creating the entry on
demand. Calling it with two anchor ids and two anchor positions therefore evaluates a real link
budget.

Identifier safety: the field node is id **1** (`ca_tbl_causal_sim.cc:1105`, `:1902`); anchors are
101+, 201+, 301+. `MakeLinkId(1, 201)` and `MakeLinkId(201, 202)` cannot collide.

**So B1 is implementable. The framing "not new physics" is what does not survive.** Three
constraints follow, and the third is the one that decides the design.

## 1. Anchors must NOT be registered as nodes

`RegisterNode` would put anchors into the engines' node set, which participates in interference
and neighbour computation for the tag's own links. That changes the baseline. Anchor–anchor
evaluation must go through a direct `ProcessTransmission` call with explicit positions and no
registration.

## 2. The null control binds on the RNG stream, not on the output

This project has already paid for this once. `DEFECTS.md`, on the D1 regeneration:

> All three engines are constructed, stepped for mobility continuity and beaconed on every cycle
> of every arm, **and they share an RNG stream. Changing how many tier-2 beacons are received
> changes how many draws are consumed, which offsets the stream every later consumer reads.**

Every `ProcessTransmission` call draws from that stream. So under `--anchorSurveyed=true` the
anchor–anchor path must **not be entered at all** — not entered and discarded, not entered and
zeroed. Byte-identity is unobtainable any other way, and a null control that only compares
summary statistics would hide exactly this.

## 3. The contradiction that has to be declared, not designed around

The MAC models anchors as **mutual hidden terminals**. B1's calibration exchange assumes anchors
**can hear each other**. Both cannot be true of the same deployment.

- Range over the channel as it stands, and the calibration ranges are computed between two
  radios that the contention model says cannot detect each other. The ranges are physically
  meaningful — the link budget is real — but the MAC around them is not.
- Model inter-anchor audibility properly, and the contention regime for the **tag's** beacons
  changes too, because CAD now suppresses transmissions that previously collided. That is not
  additive, it moves the baseline, and it retires the standing claim that beacon loss is an
  upper bound.

**Decision: range over the channel as it stands, and scope the claim.** B1 measures whether the
calibration *link budget* supports anchor-to-anchor ranging, and states that the MAC does not
model inter-anchor audibility, so the result is a statement about link feasibility and not about
contention. This is the same move the source already makes one paragraph earlier about antenna
and TX-power asymmetry — *"stated, not assumed away"* (`ca_tbl_causal_sim.cc:443-445`) — and it
is preferable to a silent change of the contention model in a build whose whole discipline is
the null control.

The cost is stated: **B1 cannot say anything about how anchor-calibration traffic contends with
tag traffic.** If that becomes the question, it needs the audibility model and a re-baselined
comparison, and it is a separate work order.

## Patch shape

- `--anchorSurveyed` (default `true`) and `--anchorRangePeriodS` (default 0 = off). Under the
  defaults **no new code executes**.
- A single `AnchorSurvey` pass per period, per tier, over intra-site pairs only — cross-site
  pairs are 1880 m apart and out of range on both tiers (`experiments/B2_GAUGE.md` §2), so
  enumerating them wastes draws for nothing.
- New trace columns behind `--traceDiagnostic`, in a **separate** `trace_anchor_ranges_*.csv`
  rather than widening the existing trace, so the existing trace stays byte-comparable.
- Report the byte-identity null control against a baseline re-run **at the same
  `--t2Anchors`**, per `experiments/B2_GAUGE.md` §3.
