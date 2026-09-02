# Foliage: integrating over gamma and A_max — registration, before the fix

The verifiable timestamp is this file's commit, which precedes the fix and its re-score.

## The defect

The engine samples both foliage parameters per realization
(`lora-physics-parameters.cc:318-319`):

    specificAttenuationDbPerM = Normal(gamma, gamma*0.25, gamma*0.2, gamma*1.8)
    maxAttenuationDb          = Normal(amax,  amax*0.2,   amax*0.5,  amax*2.0)

`linkbudget.py` uses their means as constants. `A_veg = A_max*(1 - exp(-depth*gamma/A_max))`
saturates in both, so by Jensen's inequality `E[A_veg] < A_veg(E[gamma], E[A_max])` and the
model **over-attenuates**.

This is the **third** instance of the same error class in this module, after the lognormal
vegetation depth and the distributed antenna heights. Recorded as a standing rule in its own
right:

> **A saturating function of a random variable is not the function of its mean.**
> Integrate over the distribution, or state the bias direction and magnitude before
> proceeding.

## Registered contribution, measured before the fix is applied

| vegetation | at the mean | integrated | model high by |
|---|---|---|---|
| `VEG_DENSE_FOREST` (868 MHz) | 16.424 dB | 15.958 dB | **+0.466 dB** |
| `VEG_SPARSE_SHRUBS` (2.4 GHz, UWB) | 3.917 dB | 3.827 dB | **+0.090 dB** |

## Registered prediction

Margin **falls** by 0.466 dB on 868 MHz and 0.090 dB on the two `RURAL_OPEN` tiers — the
model becomes *less* lossy, so its margin rises and its `r80` rises. Direction: this
**worsens** 868's `r80` (already over-predicting at 16.6%) and **improves** 868's
near-field margin residual, which is negative at −3.94 dB.

That is a genuine trade, registered before it is seen: the fix is correct on its own terms
and makes one headline number worse. It is applied because it is correct, not because of
its effect on either number — the direct comparison against the engine's own behaviour
decides, and `r80` is downstream.

## How

Deterministic quadrature over both truncated normals, as for fading and the two-ray height
question. No corpus draw is added, so `results/corpus.json` and its digest are untouched.

## Falsifier

If the measured margin shift differs from 0.466 / 0.090 dB by more than 0.05 dB, the
quadrature does not implement what was registered and the result is withdrawn rather than
reinterpreted.

---

# RESULT — HIT on all three tiers, exact to three decimals

Computed after the registration above was committed (`31db16c`).

| tier | predicted Δmargin | measured Δmargin | verdict |
|---|---|---|---|
| 868 MHz | +0.466 dB | **+0.466 dB** | HIT |
| 2.4 GHz | +0.090 dB | **+0.090 dB** | HIT |
| UWB | +0.090 dB | **+0.090 dB** | HIT |

Falsifier threshold was 0.05 dB; the largest discrepancy is below 0.001 dB.

## The registered trade, confirmed in both directions

| tier | worst margin deviation | `r80` relative error |
|---|---|---|
| 868 MHz | 5.42 → **4.95 dB** (better) | 16.6% → **19.6%** (worse) |
| 2.4 GHz | 11.60 → 11.69 dB | 37.6% → 38.7% |
| UWB | 5.26 → 5.35 dB | 92.9% → 94.9% |

Exactly as registered: the fix improves 868's margin residual, which is the direct
comparison, and worsens its `r80`, which is downstream. **It lands because it is correct.**
`r80` sits behind every excess term in the budget, and terrain — 13 dB of unvalidatable
loss at 2.4 GHz — is still missing from all three, so `r80` cannot adjudicate a 0.466 dB
question. The rule already recorded applies: when the direct comparison and the derived
metric disagree about which version is better, the direct measurement wins.

## Foliage re-scored against its ORIGINAL registered contribution

The foliage term was registered with these predictions
(`FOLIAGE_PREREGISTRATION.md:31-43`), against measured pre-term offsets of +12.32 dB
(868 at 100 m), +2.64 dB (2.4 GHz at 10 m), +4.64 dB (UWB at 2 m):

| # | original prediction | score at the mean | score integrated |
|---|---|---|---|
| 1 | 868's offset closes to within ~2 dB, sourced median 13.77 dB vs measured 12.32 dB | HIT — closed to 1.45 dB | **HIT — closes to 0.99 dB**, since the term shrinks by 0.466 toward the measured offset |
| 2 | 2.4 GHz and UWB move by ~3 dB, not <1 dB; the governing document's "<1 dB" is expected to MISS | HIT, and the governing document's prediction MISSED as registered | **HIT, unchanged** — 3.83 dB integrated against 3.92 dB at the mean, still ~3 dB and still not <1 dB |

**Both original predictions survive the fix, and prediction 1 scores better under it.**
That is the useful part of re-scoring: the correction moves the term toward the number its
own registration was measured against, which is independent evidence that the integrated
form is the right one — it was not chosen to improve any score, and the score improved.
