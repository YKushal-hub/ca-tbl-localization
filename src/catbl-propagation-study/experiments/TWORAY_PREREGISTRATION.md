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
