# Two-ray ground reflection — pre-registration

Written before implementation. The term was identified by localisation, not by trying
candidates: see `DEFECTS.md`, "Localising the residual".

## Why this term and not the other two

- **Markov NLOS additions: REFUTED.** `residual / nlos_frac` is not constant and changes
  sign, and the residual is −1.27 to −4.88 dB on 2.4 GHz where `nlos_frac` ≈ 0.001. An
  NLOS-conditioned term contributes nothing on LOS links.
- **terrain: implausible and unsupported.** The deficit is on 2.4 GHz in `RURAL_OPEN`
  while 868 MHz in `RURAL_FORESTED` is nearly right, and Fresnel radius scales as
  `1/√f`, so the higher-frequency tier should clear terrain more easily, not less.
- **two-ray: supported by three independent observations.** UWB's slope matches the
  model and UWB is the only tier that never reaches its breakpoint; `two_ray_frac` is
  ~1.0 on 2.4 and UWB and 0.0 on 868; and 2.4 GHz's residual crosses zero at ~1200 m
  against a computed breakpoint of 977 m.

## The physics, and where the constants come from

`d_b = 4·h_t·h_r/λ`, with heights from `lora-physics-parameters.cc:13-14`
(`gatewayHeightM ~ Normal(30, 10, 10, 50)`, `nodeHeightM ~ Normal(1.0, 0.3, 0.5, 2.0)`).
Below `d_b` the two-ray field is approximately free-space; above it the slope steepens
toward `n = 4`.

**The engine's own implementation must be located and its form reproduced before any
constant is chosen.** `two_ray_frac` is a diagnostic column; where the corresponding
loss is applied has not yet been traced, and this pre-registration does not assume the
textbook form is what the engine uses.

## Registered predictions

1. **UWB moves by less than 1 dB.** Its sweep tops out at 180 m against a 3198 m
   breakpoint, so the term is inactive throughout its range. This is the signature test:
   a term that moves UWB materially is not two-ray.
2. **868 MHz does not move at all.** `two_ray_frac` is 0.0000 at every 868 distance, so
   whatever gates the mechanism excludes that tier.
3. **2.4 GHz's far-field residual closes from +19.91 dB to within a few dB**, and its
   near-field residual is unchanged, because the term is inactive below 977 m.
4. **868's residual is unchanged** at −4.10 dB near and +1.50 dB far. It is already
   nearly closed and this term should not touch it.

## Falsifier

If 2.4 GHz's residual does not close, or if UWB or 868 move materially, two-ray is not
the missing term and the result is reported as such rather than adjusted.

## Constraint

One term, one prediction, one verdict. If this does not close the residual, stop and
report; do not proceed to a second candidate in the same pass.

---

# VERDICT: mechanism VERIFIED, r80 predictions MISSED

**Two-ray is not refuted.** It is confirmed directly against the engine's own
`mean_pl_db` column. The first implementation of it was defective; that is evidence about
the implementation, not about the physics, and the two are recorded separately here.

## The mechanism, checked against the engine's path-loss column

The strongest available test is not `r80` — it is whether the model reproduces the
engine's `mean_pl_db` distance by distance. Doing that identifies which base term is
right without any excess term in the way:

| tier | log-distance `n` | two-ray | engine env |
|---|---|---|---|
| 868 MHz | **0.11 dB** | 9.06 dB | `RURAL_FORESTED` — two-ray gated off, correctly |
| 2.4 GHz | 7.77 dB | **0.96 dB** | `RURAL_OPEN` |
| UWB | 4.49 dB | **0.02 dB** | `RURAL_OPEN` |

Worst absolute error over the whole sweep. On the two tiers where the engine says the
mechanism fires, two-ray reproduces its path loss to a fraction of a dB and log-distance
does not. On the tier where the engine says it does not fire, the reverse holds. That is
the mechanism verified on all three tiers, in both directions.

## The two defects in the first implementation — both mine, neither two-ray's

1. **Wrong heights.** I used `GW_HEIGHT_M = 30.0`, the parameter default from
   `lora-physics-parameters.cc:13`. The engine never consults it: it takes
   `rxPosition.z` / `txPosition.z` (`lora-physics-engine.cc:385-386`) and falls back to
   the distribution only when `z <= 0`. The probe registers real anchors at
   `anchorH = {15.0, 6.0, 2.5}` with `nodeH = 1.5`
   (`ca_tbl_causal_sim.cc:1116-1117`), so the fallback never fires. This put the 2.4 GHz
   breakpoint at 3068 m instead of **920 m**, keeping the whole tier in the free-space
   branch through the entire region where its `r80` lives. Averaging over the height
   *distribution* does not fix it — that was tested and recovered under 1 dB of the
   ~10 dB error, which is what ruled the distribution out as the explanation.
2. **Horizontal instead of slant distance.** The engine works from 3-D positions. At
   10 m under the 6 m 2.4 GHz anchor the slant range is 0.80 dB longer, and that is
   exactly the error that was observed there.

Both constants come from source. Neither was chosen from the size of the residual.

## Registered predictions — reported as they fell

| Prediction | Registered | Measured | |
|---|---|---|---|
| 1 | UWB moves < 1 dB (**signature test**) | worst deviation 2.05 → **5.79 dB**; r80 59.2% → 113.3% | **MISS** |
| 2 | 868 MHz does not move at all | 30.9% → 30.9%; margin 4.10 → 4.19 dB | HOLD |
| 3 | 2.4 GHz far residual closes from +19.91 dB | → **+12.13 dB**; r80 27.6% → 44.6% | **MISS** |
| 4 | 868's residual unchanged | unchanged | HOLD |

**Prediction 1's premise was wrong, and that is why it missed.** It assumed two-ray is
*inactive* below the breakpoint. It is not inactive — it is free-space, `n = 2.0`
(`lora-two-ray-ground-submodule.cc:31`), which replaces the environment's `n = 2.2`. UWB
never reaches its breakpoint and is still changed by up to 4.49 dB, because the *branch*
changed, not the breakpoint. A term that moves UWB materially can therefore still be
two-ray, and the signature test was not diagnostic of what it was written to test.

The misses are recorded as misses. They do not overturn the `mean_pl_db` comparison
above, which is the more direct measurement of the same claim.

## Two facts the trace established that this registration had wrong

1. **The breakpoint is `4π·h_t·h_r/λ`, not `4·h_t·h_r/λ`**
   (`lora-two-ray-ground-submodule.cc:26`). Every breakpoint above is low by π.
2. **`two_ray_frac` is a gating diagnostic, not a distance one.** It reports
   `ENV_RURAL_OPEN && !isNLOS` (`:11`, `:17`) — confirmed exactly, since
   `two_ray_frac + nlos_frac = 1.0000` at every 2.4 GHz distance. The registration's
   third supporting observation read a breakpoint claim out of a gating column, and
   compounded it with the π error. That argument is void even though its conclusion
   turned out right — which is the uncomfortable part worth recording.

## What is left, and what it is not

With the base term verified, the remaining residual is provably **not** in the path loss.
Isolating the engine's non-path-loss loss as `X = tx − pl_eng − sens − margin_eng`:

| tier | X_engine, near | X_engine, far | model X | `nlos_frac` |
|---|---|---|---|---|
| 2.4 GHz | 3.84 dB @ 10 m | 17.81 dB @ 6000 m | 3.9 → 5.7 dB | 0.000 → 0.111 |
| UWB | 4.27 dB @ 2 m | 9.77 dB @ 180 m | flat 3.92 dB | 0.000 → 0.004 |

A distance-growing excess of 5.5 dB (UWB) to 14 dB (2.4 GHz) on links that are
effectively LOS throughout. No further term was implemented in this pass.
