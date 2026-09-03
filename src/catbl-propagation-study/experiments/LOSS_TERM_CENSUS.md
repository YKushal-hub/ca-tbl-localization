# The engine's loss sum: a census of all thirteen terms

Written before implementing anything further. Three passes asked *which term is missing*,
in the singular. `lora-physics-engine.cc:598-600` shows the engine sums thirteen terms plus
three outside `additional_loss`. If the residual is a sum, the singular question has no
correct answer, and each pass will keep producing a term that closes part of the gap and
reads as a partial failure.

Scope: the `probe_v7` scenario only — `ENV_RURAL_FORESTED` (868 MHz) and `ENV_RURAL_OPEN`
(2.4 GHz, UWB), `WEATHER_CLEAR_DRY`, `FADING_RICIAN`, `MOB_CONSTANT_VELOCITY`, outdoor,
`ca_tbl_causal_sim.cc:1039-1063`. **An "inactive" verdict below is scenario-scoped and does
not generalise.** No new ns-3 runs: every magnitude is analytic from source constants, or
read from a `probe_v7` column.

## The census

| # | term | `file:line` | in model? | active here? | evidence | magnitude (dB) | distance |
|---|---|---|---|---|---|---|---|
| 1 | `positiveShadowing` + `negativeShadowingGain` | `lora-physics-engine.cc:585-586` | **yes** | **ACTIVE** | `shadow_sd_eff_db` 5.00–8.88 | net mean **+0.04** (asymmetric clamp) | flat |
| 2 | `result.fadingDb` | `lora-fading-submodule.cc:74` | **no** | **ACTIVE** | `FADING_RICIAN` set at `ca_tbl_causal_sim.cc:1042` | **+0.53** open, **+1.22** forested | flat |
| 3 | `result.nlosDb` | `lora-physics-engine.cc:357` | **yes** | **ACTIVE** | `nlos_frac` 0.000→0.535 | 0 → ~29 | growing |
| 4 | `result.foliageDb` | `lora-physics-engine.cc:527` | **yes** | **ACTIVE** | model X ≈ 3.9 flat matches | ~3.6–4.0 | flat |
| 5 | `result.terrainDb` | `lora-physics-engine.cc:400`, `lora-terrain-submodule.cc:191` | **no** | **ACTIVE** | `terrain.enabled = true` `lora-physics-parameters.cc:471` | **~5.5 UWB, ~13 at 2.4 GHz** | **growing** |
| 6 | `result.dopplerPenaltyDb` | `lora-physics-engine.cc:536` then **`lora-physics-engine.cc:537`** | no | **INACTIVE** | `lora-physics-engine.cc:537` assigns `0.0`, overwriting `lora-physics-engine.cc:536` | 0 | — |
| 7 | `result.interferencePenaltyDb` | `lora-physics-engine.cc:569` | no | **INACTIVE** | probe transmits one node at a time | 0 | — |
| 8 | `multipathPenaltyDb` | `lora-physics-engine.cc:489` then **`lora-physics-engine.cc:596`** | no | **INACTIVE** | `lora-physics-engine.cc:596` assigns `0.0` unconditionally | 0 | — |
| 9 | `thermalEfficiencyLossDb` | `lora-physics-engine.cc:583` | no | **INACTIVE** | initialised `0.0`, never reassigned | 0 | — |
| 10 | `tier2LossDb` | `lora-physics-engine.cc:589-590` | no | **INACTIVE** | all five components gated, below | 0.10 | flat |
| 11 | `tier3LossDb` | `lora-physics-engine.cc:593-594` | no | **INACTIVE** | `soil` = 0 at `lora-physics-engine.cc:593`; tunnel needs `ENV_TUNNEL_CONFINED` | 0 | — |
| 12 | `result.atmosphericDb` | `lora-atmospheric-submodule.cc:60`, applied at `lora-physics-engine.cc:498` | no | **ACTIVE** | `WEATHER_CLEAR_DRY`: rain 0, fog 0 | 0.0096 @180 m → **0.32** @6 km | growing, negligible |
| 13 | `state.propagationLossBiasDb` | `lora-node-position-uncertainty-submodule.cc:54` | no | **INACTIVE** | `lora-ranging-submodule.cc:214`, `lora-ranging-submodule.cc:247` assign `0.0` under `RANGING_PHYSICS_DERIVED` (`ca_tbl_causal_sim.cc:1047`) | 0 | — |
| + | `result.hwTxVariationDb` | `lora-physics-engine.cc:345` | no | **ACTIVE** | hardware TX variation | small, zero-mean | flat |
| + | `result.antennaGainDb` | **`lora-physics-engine.cc:341`** | n/a | **INACTIVE** | assigned `0.0`, overwriting `lora-physics-engine.cc:340` | 0 | — |

### `tier2LossDb` broken out — five terms, all inactive, each for a stated reason

| component | gate | verdict |
|---|---|---|
| `buildingEntryDb` | `!params.enabled`, then env must be indoor/industrial | inactive |
| `wallPenetrationDb` | `!state.isIndoor` → `0.0` (`lora-wall-material-submodule.cc:10-12`) | inactive |
| `floorAttenuationDb` | `!state.isIndoor` → `0.0` (`lora-floor-attenuation-submodule.cc:10-12`) | inactive |
| `bodyShadowingDb` | `nodeIsBodyWorn = false` (`lora-physics-parameters.cc:1287`), never overridden | inactive |
| `temporalBlockageDb` | rural branch `p = 0.02`, loss `N(5,2,1,15)` | **0.10 dB**, flat |

`state.isIndoor` defaults `false` (`lora-physics-parameters.h:51`) and is written only by
the building-entry submodule, which leaves it false outside indoor environments.

## What the census says

**Eight of the thirteen are structurally zero here** — four of them hard-assigned `0.0` on
a line *after* the line that computes them (`lora-physics-engine.cc:341`, `lora-physics-engine.cc:537`, `lora-physics-engine.cc:596`, and `lora-physics-engine.cc:593`). They are
**inactive, not missing.** They leave the search space permanently, for this scenario.

**The residual is a sum of two terms, not one.** Predicted, before implementing:

| tier | fading (flat) | terrain (growing) | atmospheric | blockage | predicted sum |
|---|---|---|---|---|---|
| 868 MHz | +1.22 | ~? | 0.32 @6 km | 0.10 | — |
| 2.4 GHz | +0.53 | ~13 @6 km | 0.32 | 0.10 | ~14 |
| UWB | +0.53 | ~5.5 @180 m | 0.01 | 0.10 | ~6.1 |

Measured excess to explain: 13.97 dB (2.4 GHz, 10 m→6 km) and 5.50 dB (UWB, 2 m→180 m).
The near-field floor is fading + blockage ≈ 0.63 dB, and the growth is terrain.

## Two facts about `terrainDb` that change earlier reasoning

1. **Terrain hardcodes 868 MHz** (`lora-terrain-submodule.cc:70-71`) — its Fresnel radius
   is computed at 868 MHz for *every* tier, including UWB at 7.99 GHz. The engine's terrain
   is frequency-independent.
2. **The earth-curvature path is never reached.** It requires `horizontalDistM >
   d_horizon`, and the radio horizons are 21.0 km (868), 15.1 km (2.4 GHz), 11.6 km (UWB)
   against sweep maxima of 6 km, 6 km, 180 m. Only `proceduralLoss` can fire.

### W10 — WITHDRAWN: "terrain is implausible because Fresnel radius scales as 1/√f"

That argument dismissed terrain in the original localisation. It reasons about physics the
engine does not implement: the frequency is a hardcoded constant, so terrain does not scale
with frequency at all. The argument was about the real world and the claim was about this
engine, and they are not the same object. This is the **third** void argument in this
sequence whose conclusion could not be trusted because its reasoning never touched the
source (see W8).

## THE INFERENCE RULE — registered before any term is added

**With a sum, closing part of the gap is a hit, not a partial refutation.**

Each term is scored on whether it moves the residual in the **predicted direction** by the
**predicted magnitude**, judged alone, against the contribution registered for it *before*
it is implemented. It is **not** scored on whether the total reconciles afterwards.

A term that contributes its predicted 0.53 dB and leaves 13 dB outstanding has **passed**.
Reading that as a partial refutation is the error that nearly cost two-ray, whose
implementation was defective while the mechanism was correct and whose diagnostic column
showed it firing on 0.89–1.00 of links throughout.

Corollary, also registered: **the running total is the test of the census, not of any
term.** If each term hits its own prediction but the running total does not track the
predicted sum, the census is wrong — stop and re-census rather than adding a term to
absorb the difference.

## §2.1 — where this stops being a validation

A model that reproduces the engine term-by-term from the engine's source is a
transcription, and a transcription cannot validate its source. Each term is therefore
classified before implementation:

| term | classification | reason |
|---|---|---|
| fading | **PHYSICS** | Rician envelope with `E[r²] = 1`, `−20·log10 r`. Standard, derivable from the K-factor alone. The engine's constants (`K ~ N(8,2,4,15)` open, `N(3,1,1,6)` forested) are *parameters*, cited; the relation is not copied. |
| terrain | **TRANSCRIPTION-ONLY — cannot be validated** | Its diffraction relation is ITU-R P.526 knife-edge and is physics. Its **input** is `GetProceduralElevation`: three hardcoded octaves of fBm (wavelengths 500/200/75 m, amplitudes 1.0/0.4/0.1, offset 0.75, scale ×10) over a bespoke `SmoothNoise`. That is an invented terrain field, not a documented effect. Reproducing the engine's terrain means copying that generator; anything else produces a different realisation whose disagreement is uninterpretable. |
| atmospheric | PHYSICS, but negligible | ITU-R P.838-3 form; 0.0096–0.32 dB. Not worth implementing. |
| blockage | parameter-only | A 2% Bernoulli with a fitted loss distribution; no physics to implement independently. 0.10 dB. |

**This is the reportable limit.** The single largest missing term is the one this module
cannot independently validate. That is a real constraint on what §7 can establish, and it
is better learned now than after ten terms are in.

---

# Implementation, in census order

## Term 1 of 2: Rician fading — **HIT**

Registered before implementing: a **flat** contribution of **+0.531 dB** on `RURAL_OPEN`
and **+1.228 dB** on `RURAL_FORESTED`, with no distance dependence.

| tier | predicted Δmargin | measured Δmargin | verdict |
|---|---|---|---|
| 868 MHz | −1.228 dB | **−1.228 dB** | HIT |
| 2.4 GHz | −0.531 dB | **−0.531 dB** | HIT |
| UWB | −0.531 dB | **−0.531 dB** | HIT |

Scored alone, against its own registered contribution, per the inference rule above.

Two independent computations of the quantity agree: a Monte-Carlo draw reproducing the
engine's sampling path including its ±4σ truncation gives +0.531 / +1.223 dB, and the
deterministic quadrature actually used gives +0.531 / +1.228 dB.

**Implementation note.** Fading enters `pdr_at` as a *convolution* — the reception
decision is averaged over the fade distribution — not as a mean fade applied to the margin
followed by one decision. The two differ whenever the logistic is sharp, and it is
(0.35 dB). The quadrature is deterministic, so `results/corpus.json` and its digest are
untouched; no corpus draw was added.

**Registered omission, not an assumed zero.** Only the LOS branch is modelled. The engine
also lowers K under NLOS (`lora-fading-submodule.cc:44-48`: `kNlosMean` 1.0 open, 0.5
forested), worth +1.92 / +2.06 dB on those links. On the `RURAL_OPEN` tiers `nlos_frac` is
0.000–0.004 so the omission is under 0.01 dB. On 868 MHz `nlos_frac` reaches 0.535 at
6 km, where the omission is **≈0.44 dB** — the largest single number in this document that
is known-missing rather than measured.

### Running total against the census

| tier | residual before fading | after fading | census predicted remainder |
|---|---|---|---|
| 868 MHz | 4.19 dB / r80 30.9% | 5.42 dB / r80 **16.6%** | — |
| 2.4 GHz | 12.13 dB / r80 44.6% | 11.60 dB / r80 **37.6%** | terrain ≈13 dB @ 6 km |
| UWB | 5.79 dB / r80 113.3% | 5.26 dB / r80 **92.9%** | terrain ≈5.5 dB @ 180 m |

The remainders track the census's terrain prediction on both `RURAL_OPEN` tiers. The
census is not contradicted, so no re-census is triggered.

**868's worst margin deviation rose, 4.19 → 5.42 dB, while its r80 error nearly halved.**
Reported rather than smoothed: 868's residual is *negative* in the near field (the model
already has too much loss at 100–500 m), so a flat added loss worsens the worst-case point
while improving the distance at which PDR crosses 0.80. That is a second, opposite-signed
868 discrepancy which this census does not explain and which nothing here addresses.

## Term 2 of 2: terrain — **NOT IMPLEMENTED, and not implementable here**

Terrain is the larger term by an order of magnitude and it is stopped on the §2.1 rule,
not on effort. Its active path in this scenario is `proceduralLoss`, whose input is
`GetProceduralElevation` — three hardcoded fBm octaves over a bespoke `SmoothNoise`
(`lora-terrain-submodule.cc`). Reproducing it means transcribing that generator. Any
independent terrain model produces a *different realisation* of a random field, so its
disagreement with the engine would be uninterpretable — neither confirming nor refuting.

**Classified UNVALIDATED. This is the reportable limit on §7.** The dominant missing term
is one this module cannot independently check, and no amount of further work inside the
module changes that. Stating it is the result.
