# B1 — anchor-to-anchor ranging: built, null control holds, and the prediction cannot be scored as posed

## Null control — reported first, and re-run after every change

Under `--anchorSurveyed=true` (default), against the committed `results/h1_power/h1_exclude_seed1`
cell, all three files **byte-identical**: `arm_summary.csv`, `trace_ca_tbl_full_seed1.csv`,
`trace_ca_tbl_no_integrity_seed1.csv`.

Verified **three times** — at first implementation, and again after each of the two scheduling
corrections below. A null control checked only once is a null control for the version that no
longer exists. The guard is an early-out: no `ProcessTransmission` executes, so not one draw is
taken from the shared stream. D1 is why that distinction is load-bearing.

## Node registration

Anchors are **not** registered as nodes, and nothing else changed. `RegisterNode` inserts into
each engine's node set, which feeds interference and neighbour computation for the field node's
own links, so registering twelve tier-2 anchors there would move the baseline.
`ProcessTransmission` needs no registration: it takes both endpoint positions explicitly and
creates per-link state on demand, keyed on `MakeLinkId(tx, rx)`. Field node is id 1, anchors are
101+/201+/301+, so anchor-anchor link ids cannot collide with node-anchor ones.

## The MAC contradiction, and the way it actually bit

Scoped as planned — range over the channel as it stands, claim link feasibility and not
contention. **What was not anticipated: the MAC does not model inter-anchor audibility, but the
interference submodule does see concurrent anchor transmissions.** Two scheduling defects
followed, both mine, both caught by the same signature.

**First: all pairs fired at one instant.**

| pair | delivered |
|---|---|
| 201→202 | 0.975 |
| 201→203 | 0.975 |
| 202→203 | 0.008 |
| 204→205, identical 259.8 m | **0.000** |

Twelve concurrent tier-2 transmitters; all but the first collided. A survey that transmits
simultaneously from every anchor is a jamming experiment.

**Second: the fix was wrong in the same way.** Selecting `pi % pairsPerSite` picks one pair from
*every* site — four concurrent transmitters, not one. The signature was identical in kind: site 0
at 0.98, sites 1–3 at 0.00–0.01. **Reducing concurrency from twelve to four does not fix a
collision artefact, it shrinks it.** Corrected to one globally-selected pair per tier per epoch.

## Per-link delivery, engine against model

| link | engine | model (residual-corrected) |
|---|---|---|
| tier 2, 259.81 m | **0.9826** | 0.9974 |
| UWB across, 19.02 m | **0.7208** | 0.7296 |
| UWB adjacent, 11.76 m | **0.7458** | 0.9162 |

Tier 2 and the UWB across-ring link agree closely — the across link to within 1.2%, which is the
better agreement of the two and on the link the model was least confident about.

**The UWB adjacent figure is not trusted.** 0.7458 at 11.76 m against 0.7208 at 19.02 m is
essentially flat across a 1.6× distance ratio, which is not what path loss does. It is a floor,
consistent with residual contention against the twenty UWB anchors' own beacon traffic. Reported
as a **lower bound**, not as the channel's answer.

## The registered prediction: NOT SCORED, because its premise is not physically available

The figure on the record is `P(cluster solvable in a single epoch) = 0.9191`, computed by
enumerating edge subsets under independent per-link delivery.

**That presumes all ten pairs can be attempted in the same epoch, and the engine says they
cannot** — attempting them simultaneously is exactly the collision artefact above. The
measurement the prediction asks for is not one the channel permits. This is not a miss and not a
hit; **the question was posed in a regime that does not exist**, and re-posing it after seeing
data would be fitting.

What replaces it, stated so the successor is unambiguous: **solvability is a per-TDMA-round
property, not a per-epoch one.** Using the engine's measured delivery, over a full round in which
each of the ten pairs is attempted once:

    P(solvable | 1 round)  = 0.7581
    P(solvable | 2 rounds) = 0.9942
    P(solvable | 3 rounds) = 0.9999

**Cost, stated as required.** A round is 10 epochs per site at one pair per epoch. Two rounds to
0.994 is therefore ~20 survey epochs — 50 s at the 2.5 s cycle. Calibration is not a per-cycle
operation, so this is cheap; but it is a real latency and it did not exist in the model's framing.

The single-round 0.7581 is *below* the model's 0.9191, and the two-round 0.9942 is above it. The
model was not wrong about the cluster being solvable — it was wrong about the unit of time.

## What is not claimed

B1 says nothing about how calibration traffic contends with tag traffic. It now has a concrete
reason to care: the survey transmits before the tag's poll in the same cycle, so with
`--anchorSurveyed=false` it becomes interference to the tag's own ranging. Quantifying that needs
the audibility model and a re-baselined comparison, and remains a separate work order.

---

# Addendum — the detector, and what it found about the adjacent link

## G16, and the re-measurement it made possible

`concurrentInTier` is now emitted per exchange; the runtime aborts on a violation naming the pair
and the count, and `scripts/catbl_concurrency_gate.py` re-checks a finished trace offline with an
**independent recount**, because a self-reported counter cannot detect a defect in itself.

Re-running the survey under a G16-verified serialised schedule returned rates **identical to the
digit**:

| link | pre-detector | G16-verified serialised | delta |
|---|---|---|---|
| tier 2, 259.81 m | 0.9826 | 0.9826 | **+0.0000** |
| UWB adjacent, 11.76 m | 0.7458 | 0.7458 | **+0.0000** |
| UWB across, 19.02 m | 0.7208 | 0.7208 | **+0.0000** |

So the second correction had already serialised the survey; G16 proves it rather than assuming
it. **The registered prediction — that eliminating concurrency would raise the adjacent rate —
therefore had nothing left to eliminate, and the rates did not move.**

## Scoring, and the distinction that matters

The prediction's *mechanism* was contention. Its *locus* was survey self-concurrency. The
mechanism is confirmed and the locus was wrong: the contention is **beacon traffic**, which the
survey does not schedule and which G16 by construction does not count.

Setting `--beaconUwb=600` (effectively quiescing tier-3 beacons) against the 2 s default, on a
600 s run:

| link | beacons 2 s | beacons 600 s | delta |
|---|---|---|---|
| UWB adjacent, 11.76 m | 0.7417 | **0.9250** | **+0.1833** |
| UWB across, 19.02 m | 0.8000 | 0.7333 | −0.0667 |
| tier 2, 259.81 m | 0.9958 | 0.9958 | **+0.0000** |

**The asymmetry held, and tier 2 is an exact control** — UWB beacons cannot affect a 2.4 GHz link,
and the rate is unchanged to four decimals, which is what says the effect is real rather than a
global shift. Adjacent rises to 0.9250 against a model prediction of 0.9162: agreement to ~1%,
on the figure previously reported as an untrusted lower bound. The across-ring change is −0.0667
on n = 120 and is not distinguishable from noise.

**So the flat distance dependence was contention after all**, and the earlier decision to report
0.7458 as a lower bound rather than as the channel's answer was correct.

## Solvability recomputed

| delivery basis | 1 round | 2 rounds | 3 rounds |
|---|---|---|---|
| measured **under beacon contention** (0.7458 / 0.7208) | 0.7581 | 0.9942 | 0.9999 |
| measured, **beacons quiesced** (0.9250 / 0.7208) | **0.9206** | 0.9995 | 1.0000 |
| model, residual-corrected (0.9162 / 0.7296) | 0.9191 | 0.9995 | 1.0000 |

**The engine's 0.9206 against the model's 0.9191 is agreement to 0.16%.**

That deserves a precise statement rather than a victory lap. The model's **edge-availability
arithmetic is validated** — enumerate ten links, each independently available, ask whether the
subset is rigid — and it is validated on a quantity nobody tuned. What is *not* rescued is the
model's temporal claim: it asserted this per **epoch**, with all ten links attempted at once, and
the engine says that attempt is a collision. The number was right; the unit was not. The two
happen to coincide numerically because a serialised round and a hypothetical simultaneous epoch
both reduce to the same independent-availability calculation.

The first row is retained and labelled: it is the honest figure for a deployment whose anchors
beacon on the default schedule, which is the deployment as configured.
