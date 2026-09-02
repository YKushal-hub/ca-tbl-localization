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
