# Foliage term — pre-registration

Written before the term is implemented or run. Source `8b7be35`.

## The term, from source

`ComputeFoliageLossDb`, `lora-foliage-submodule.cc:36` — ITU-R P.833-10:

    A_veg = A_max * (1 - exp(-depth * gamma / A_max))

`depth ~ LogNormal(ln(100), 0.6, 0, 500)` — `lora-physics-parameters.cc:293`
`gamma`, `A_max` per vegetation type — `lora-physics-parameters.cc:304-309`:

| vegetation | gamma | A_max | tier | A_veg at median depth 100 m |
|---|---|---|---|---|
| `VEG_DENSE_FOREST` | 0.20 | 25.0 | 868 MHz (`vegWide`) | **13.77 dB** |
| `VEG_SPARSE_SHRUBS` | 0.04 | 8.0 | 2.4 GHz, UWB (`vegFacility`) | **3.15 dB** |

`vegWide = VEG_DENSE_FOREST`, `vegFacility = VEG_SPARSE_SHRUBS` —
`ca_tbl_causal_sim.cc:3174-3175`.

**The term is DISTANCE-INDEPENDENT.** `depth` is a per-link lognormal draw, not a
function of link length. So foliage can only explain a constant offset, never a slope.

## The values are sourced, not fitted

`gamma` and `A_max` are read from `lora-physics-parameters.cc:304-309` and were not
chosen with reference to the size of the observed gap. That the median values land
near the measured offsets is a *prediction being tested*, not a fit.

## Registered predictions

Measured constant offsets before the term is added:

    868 MHz  +12.32 dB at 100 m      2.4 GHz  +2.64 dB at 10 m      UWB  +4.64 dB at 2 m

1. **868 MHz's constant offset closes to within ~2 dB.** The sourced median 13.77 dB
   against a measured 12.32 dB offset.
2. **2.4 GHz and UWB move by ~3 dB, not by less than 1 dB.** The governing document
   predicted "less than 1 dB, because `RURAL_OPEN`'s sparse vegetation contributes
   almost nothing". That prediction is expected to **MISS**: `VEG_SPARSE_SHRUBS` is
   not `VEG_NONE`, and the sourced formula gives 3.15 dB at median depth. Recording
   the disagreement in advance rather than discovering it.
3. **The distance-growing component will NOT close.** 2.4 GHz's gap grows from −2.00 dB
   at 430 m to +23.83 dB at 6000 m, and 868's grows ~6 dB over its range. A
   distance-independent term cannot remove a distance-dependent gap. **A second term
   is missing**, and this is stated before running so it cannot be presented as a
   discovery afterwards.
4. **The reconciliation will still FAIL** the 15% r80 criterion on at least 2.4 GHz.

## Falsifier

If adding the sourced term moves 868's constant offset by materially more or less than
the predicted ~13.8 dB, the term is not what is missing, and that is a finding rather
than grounds for adjusting `gamma` or `A_max`.

## Fixture

Detune `gamma` for `VEG_DENSE_FOREST` and confirm the output moves **868 far more than
2.4 GHz**. That asymmetry is the term's signature; movement alone is weaker evidence.
