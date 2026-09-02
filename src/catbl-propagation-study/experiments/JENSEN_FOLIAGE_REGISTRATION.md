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
