# G11a — registration, written before computing

Registered prospectively. The verifiable timestamp is this file's commit, which precedes
the commit carrying any G11a result.

## Why this exists

G11b — full independent reconciliation at the registered criterion (`r80` within 15%,
margin within 3.0 dB) — is **permanently red**, because `terrainDb` is transcription-only.
That verdict stands unrescoped. G11a asks a different and answerable question.

## What was proposed, and why it was declined

The proposal was to import terrain from the engine's own column as a declared offset.
`probe_v7` has no terrain column and no new runs are authorised, so the only available
terrain value is the residual `X_engine − X_model`. Importing that would make G11a pass at
identically 0.00 dB everywhere and measure nothing — the same circularity that broke the
original `validate.py`. **No terrain value is imported anywhere in G11a.**

## What is registered instead: the terrain-inactive sub-population

Terrain's procedural loss fires only when the minimum Fresnel clearance ratio along the
path falls below 0.6 (`lora-terrain-submodule.cc:145-147`). Where it provably does not
fire, terrain is exactly zero and the model can be compared with nothing imported.

The bound is derived from the field's **declared amplitudes**, not from reproducing its
realisation — `Hash2D` is never implemented here:

- `SmoothNoise` returns `amp x` a bilinear blend of `Hash2D` values, so with
  `Hash2D` in [0,1] each octave is bounded by its amplitude.
- Smoothstep `3t^2 - 2t^3` has maximum slope 1.5, so octave `k` has spatial slope at most
  `amp_k * 1.5 / scale_k`. Summing the three octaves (1.0/500, 0.4/200, 0.1/75) and
  applying the `x10` scaling gives a **field slope of at most 0.080 m/m**.
- A Lipschitz-`L` function deviates from its chord by at most `L*d/2`, so the terrain
  deviation is bounded by **0.040 * d**.

Terrain is guaranteed inactive while, at the path midpoint,

    (h_mid - 0.040*d) / (0.5*sqrt(lambda*d))  >=  0.6

with `lambda` at **868 MHz for every tier** — the frequency is hardcoded
(`lora-terrain-submodule.cc:69-70`). The midpoint is the binding point: `r_F1` is maximal
there and vanishes at both ends, while `rayH` is linear.

| tier | `h_mid` | guaranteed inactive below | probe points in the sub-population |
|---|---|---|---|
| 868 MHz | 8.25 m | 152.0 m | 100 |
| 2.4 GHz | 3.75 m | 60.0 m | 10, 25, 50 |
| UWB | 2.00 m | 27.5 m | 2, 5, 10, 20 |

**Population: 8 of the 36 swept points.** Stated before computing, as the population rule
requires. It is a conservative bound — terrain may also be inactive beyond these
distances, and those points are excluded rather than assumed.

## Registered criterion

**The same numbers as G11b, applied to a different population**: margin within
**3.0 dB** at every point of the sub-population. This is not a relaxation — the threshold
is unchanged; only the set of points differs, and that difference is stated, bounded, and
derived rather than chosen.

`r80` is **not** part of G11a. Every tier's `r80` lies outside the sub-population
(1515 m, 1149 m, 12.8 m against bounds of 152 m, 60 m, 27.5 m — UWB's is inside, the other
two are not), so an `r80` criterion would be inhomogeneous across tiers. Registered as
excluded, before computing, rather than dropped later.

## Registered prediction

**G11a PASSES.** Every active term on this sub-population — path loss with two-ray,
shadowing, NLOS, foliage, fading — is implemented independently and each has already been
checked against the engine's own columns. If it fails, the census is wrong: something
active is missing at short range that is **not** terrain, and the corollary registered in
`LOSS_TERM_CENSUS.md` applies — re-census rather than add a term.

## Falsifier

A margin deviation above 3.0 dB anywhere in the sub-population falsifies the census's
account of the residual, since terrain cannot be responsible there. Report it as a
falsification of the census, not as a G11a tuning problem.

---

# RESULT — G11a **FAILS**. The registered prediction was wrong, and the falsifier fires.

Computed after the registration above was committed (`4e7fb05`, 2026-09-02T09:08:17).

| tier | dist | margin_eng | margin_mdl | deviation | |
|---|---|---|---|---|---|
| 868 MHz | 100 m | 47.10 | 41.68 | **−5.42 dB** | EXCEEDS |
| 2.4 GHz | 10 m | 53.66 | 53.06 | −0.60 | |
| 2.4 GHz | 25 m | 46.60 | 45.76 | −0.84 | |
| 2.4 GHz | 50 m | 40.94 | 39.84 | −1.10 | |
| UWB | 2 m | 25.77 | 25.59 | −0.18 | |
| UWB | 5 m | 16.83 | 18.43 | +1.60 | |
| UWB | 10 m | 9.40 | 12.54 | **+3.14 dB** | EXCEEDS |
| UWB | 20 m | 2.44 | 6.55 | **+4.11 dB** | EXCEEDS |

3 of 8 points exceed 3.0 dB; worst 5.42 dB. **G11a FAIL.**

Per the falsifier registered above, this is reported as a **falsification of the census**,
not as a G11a tuning problem: terrain is provably inactive at every point here, so the
census's account — that terrain explains the residual — is wrong at short range on two of
three tiers. The `LOSS_TERM_CENSUS.md` corollary applies: **re-census, do not add a term.**

## Re-census: one real term found, two anomalies localised

### A seventeenth term, outside `additional_loss` entirely

`effectiveSensitivity -= m_config.receiver.linkHysteresisMarginDb` when the link was
previously receiving (`lora-uwb-physics-engine.cc:664-666`, value **1.5 dB** at
`lora-physics-parameters.cc:401`). `mean_margin_db` is the mean of
`result.sensitivityMarginDb` (`ca_tbl_causal_sim.cc:1356`, `:1371`, `:1386`), so the
engine's margin **denominator moves with PDR** — and therefore with distance.

The census enumerated `additional_loss` at `:597-599` and missed this because it is not in
that sum. It is worth up to 1.5 dB, is largest where PDR is highest, and so *mimics a
distance-growing loss* while being nothing of the kind. Correcting for it as
`X + 1.5*pdr` is applied below.

### After correcting for it — what each tier's residual actually looks like

| tier | inside terrain-inactive bound | beyond it |
|---|---|---|
| **2.4 GHz** | **0.08–0.89 dB** at 10–50 m (bound 60 m) | rises to 11.61 dB at 6 km |
| **UWB** | **1.32 → 5.04 dB** across 2–20 m (bound 27.5 m) | flat 4.7–5.3 dB to 180 m |
| **868 MHz** | **−3.94 dB** at 100 m (bound 152 m) | decays to +0.55 dB at 6 km |

**2.4 GHz is consistent with the census.** Under 0.9 dB everywhere terrain cannot act, and
divergence only past ~600 m. Terrain remains its explanation and G11a passes on 2.4 GHz
alone.

**UWB is not.** It rises 3.7 dB entirely *inside* the region where terrain is provably
inactive, then saturates by 20–30 m and stays flat to 180 m. Terrain is excluded by
geometry, and a term that saturates at 20 m is not terrain in any case.

**868 is not, and its sign is opposite.** The model is 3.94 dB **too lossy** at 100 m,
decaying with distance. This is the anomaly flagged for a bounded look.

### Ruled out for UWB's 3.7 dB, each with its reason

| candidate | ruled out because |
|---|---|
| terrain | provably inactive below 27.5 m by the geometric bound |
| path loss | `mean_pl_db` reproduced to **0.02 dB** at every UWB distance |
| foliage | `ComputeFoliageLossDb` samples depth from a distribution; `txPos`/`rxPos` are passed and **never used**, so it cannot vary with distance |
| NLOS | `nlos_frac` is 0.0005 at 5–20 m; even 30 dB of excess gives 0.015 dB |
| atmospheric | 0.0005 dB at 20 m |
| coherent gain | constant per tier (868: 0.0, 2.4 GHz: 21.07, UWB: 30.1 dB) — cannot produce growth |
| shadowing clamp | +0.041 dB, flat |
| link hysteresis | already corrected for above; it reduces the rise from 4.29 to 3.73 dB, it does not remove it |

**Recorded OPEN.** A saturating ~3.7 dB term active over 2–20 m on UWB only, not in the
census, not any of the eight candidates above. No fifth term hunt was started.
