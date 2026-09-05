# The PHY provider modelled one field node — diagnosis, correction, and fix

**2026-09-05.** Cited by the open-items register. This file records a blocker, a
**wrong inference I drew from it**, and the fix that followed once the inference was
checked against the code rather than against the measurement.

---

## 1. The blocker, as measured

C4 (escalation arbitration between two tags) was implemented, null-controlled and
granting, but its deny and pre-empt paths could not be exercised. Rather than guess
between the three plausible causes, the delivery path was instrumented.

Measured, 600 s, `--nTags=2`, arbitration on:

```
peer ESC_REQUEST  sent=162  dropDuty=0  dropCad=0  lostColl=0
anchors           recv=661  dropDedup=0
arbiter           granted 661, denied 0, pre-empted 0, all target 1
```

The peer transmits 162 requests and loses **none** to the duty governor, to CAD, or to
collision — and no anchor receives one. Not a single peer frame appears on the receive
side. All three candidate causes are eliminated by their own counters being zero.

**The cause.** `EnginePhyProvider` was constructed with a single `fieldNodeId` and
carried a single `SetNodePosition()`. `Evaluate()` required one endpoint to equal that
node and returned an empty outcome otherwise. The provider sits **below** the MAC, so
no MAC-level work reaches it: the peer was a transmitter that could not be heard, and
every layer above reported success.

This is the failure mode CLAUDE.md rule 9 is about. A defect whose signature is
*absence* produces counters that all read clean.

---

## 2. The inference that was wrong

From that measurement this file originally concluded:

> The full per-tag refactor would not be sufficient either. Giving each tag its own
> policy, energy model and filter does not make the provider multi-node. C2-full is
> necessary and not sufficient.

The second half is right. **The first half — that C2-full is necessary — is wrong**, and
it was wrong because it was inferred from the measurement instead of read off the code.

The engines were **already multi-node**:

- `m_nodeStates` is a `std::map<uint32_t, NodeState>`, not a scalar
  (`lora-physics-engine.cc:166`).
- Peers were already registered on all three engines
  (`ca_tbl_causal_sim.cc`, the `--nTags` loop) and their positions already pushed every
  step via `UpdateNodePosition`.

Those calls were **inert for delivery**, because the provider — not the engine — decides
whether a frame has an evaluable link. The engine-side support the refactor was assumed
to be needed for already existed. The correct scope was one class, not a scenario-wide
refactor.

**The lesson is rule 3's.** The blocker was found by instrumenting, which was right; the
*scope* was then estimated from the same instrumentation rather than from an independent
reading of the engine's data structures. One `grep` for `m_nodeStates` would have
corrected it, and did, a day later.

---

## 3. The fix

`EnginePhyProvider` now carries a position per field node. The measured tag keeps its
own scalar id and position and is tested first, by scalar compare; peers live in a map
that is **empty unless `--nTags > 1`**.

No new flag. CLAUDE.md §4 requires a default-off flag whose off state reproduces the
baseline byte-for-byte; `--nTags` already exists and already defaults to 1, and that is
the off state.

Pre-registered in `report/PREREG_PROVIDER.md`, committed at `ede157f` **before the first
run**. Code at `3c95a86`.

---

## 4. Results — seed 7, one seed

### P1, null control — PASS

The new binary run on `results/baseline_v7`'s **own recorded invocation** reproduces
**all seven** SHA-256 digests in that baseline's sidecar: `arm_summary.csv` and all six
traces. (Default flags do not reproduce that baseline; that is a flag-set difference,
not a reproduction failure, and the flags were taken from the sidecar.)

### P2, link evaluations — PASS

`macEvals`, last trace row, `--nTags=2`:

| arm | before | after | change |
|---|---:|---:|---:|
| `ca_tbl_full` | 518 404 | 955 509 | +84 % |
| `static_868` | 512 885 | 949 355 | +85 % |
| `static_uwb` | 518 659 | 958 096 | +85 % |

### P3, channel deliveries — PASS

| arm | before | after | change |
|---|---:|---:|---:|
| `static_868` | 290 435 | 402 760 | +112 325 |
| `static_2p4` | 290 729 | 405 265 | +114 536 |
| `static_uwb` | 293 968 | 407 027 | +113 059 |
| `fixed_threshold` | 293 364 | 401 267 | +107 903 |
| `ca_tbl_no_integrity` | 290 503 | 408 991 | +118 488 |
| `ca_tbl_full` | 293 574 | 405 488 | +111 914 |

### What is **not** claimed

Collisions fall by ~8 000 and captures rise by ~5 500 per arm. **Those deltas are not
interpretable as a contention effect and are not reported as one.**

`LoRaDistributionSampler` creates its five random variables with **no `SetStream`**
(CLAUDE.md §5 records this as a decision not to fix), so they draw from ns-3's default
stream — and every submodule on the `ProcessTransmission` path draws from it:
propagation, shadowing, fading, PLL, hardware, mobility. The ~437 000 extra evaluations
therefore **displace the stream for every other link in the run**.

This is rule 8 in a form the rule does not literally cover: not two arms consuming a
shared stream at different rates, but two **binaries** doing so. P2 and P3 survive it
because they are control-flow claims — a path that was structurally unreachable is now
reachable, and a stream displacement cannot manufacture 112 000 deliveries. A 14 % swing
in a collision count is exactly what it *can* manufacture.

**One seed.** No magnitude here is a result in the rule-8 sense. Anything quantitative
about multi-tag contention needs a seed sweep, and none is claimed.

### A belief this falsified

The open-items register carried the note that the 1-ulp trace divergence on the C2/C3/C4
branches was unavoidable because **any** addition to `RunArm` changes its stack frame and
therefore the codegen under `-march=native`. `provider` is a `RunArm` stack local and this
change grew it by a `std::map`, yet all six traces are byte-identical. **That note is too
strong and has been withdrawn.** Whatever causes the divergence on those branches, it is
not the mere fact of touching the frame.

---

## 5. What this re-opens

- **C4 is unblocked** and no longer depends on C2-full. Deny and pre-empt should now be
  exercisable.
- **C3's counts were measured on a substrate where the peer was inaudible.** Its
  qualitative finding is a control-flow claim and should survive — a query naming the
  target is answered, a query naming the peer is ignored — but the specific counts
  (30 answered / 28 ignored) were produced under the defect and **must be re-measured on
  the fixed provider before being cited**.
- **C5 is unchanged.** It is gated on the relayed-ranging error model, which is a
  specification precondition and not a code one.

## 6. What stays out of scope, and why

**Tag-to-tag links remain unevaluable.** They are *reachable* rather than impossible:
`ProcessTransmission` auto-registers an unknown receiver as a gateway with
`fineTimestamp=false` (`lora-physics-engine.cc:298`), which is the correct hardware class
for a tag, so the link budget would evaluate. They are not enabled because nothing would
be entitled to read the `estimatedRangeM` they produce: a tag-to-tag range enters the
solution as a **relayed** measurement, and the relayed-ranging error model that says how
to weight one is the written, reviewed precondition of C5 and does not exist. Enabling
the link first would produce ranges that look usable and are not attributable — a wrong
result that looks like a working mechanism.

**Anchor-to-anchor links remain unevaluable**, as before: every anchor is a hidden
terminal to every other anchor, so beacon loss here is an upper bound, not an estimate.

**The peer is still not a measured subject.** It has no `CaTblPolicy` and no
`CaTblEnergyModel` by design — it is an addressing and contention peer. Making it a
measured subject is C2-full, and it is still not done.
