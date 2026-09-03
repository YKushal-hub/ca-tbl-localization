# Residual versus r80 — result

Registered in `UWB_R80_RESIDUAL_REGISTRATION.md`, committed before the computation.

## Scored: HIT

| | |
|---|---|
| model r80, no offset | 25.20 m |
| engine r80 | 12.90 m |
| gap | 12.30 m |
| **registered hit criterion** | **corrected r80 ≤ 19.1 m** (closes ≥ half) |
| falsifier | > 22.1 m, or overshoot below 12.9 m |

| margin offset | corrected r80 | gap closed |
|---|---|---|
| −3.5 dB | 16.82 m | 68.1% |
| **−3.7 dB (the measured residual)** | **16.44 m** | **71.2%** |
| −4.0 dB | 15.88 m | 75.8% |

**HIT, with margin, and no overshoot.** The exact computation closes *more* than the linearised
estimate predicted — 71.2% against 3.7 × 2.08 = 7.7 m ≈ 62.6% — because the PDR curve is convex
over that interval, so a linear sensitivity taken at the operating point understates the movement.

**Replication holds.** `DEFECTS.md:1115-1120` ran this once before at 2.05 dB against a 7.6 m gap
and found the margin error explained "about half", with ~26 percentage points from elsewhere.
Here the residual explains 71.2% with **28.8% from elsewhere** — the same remainder, to within
three percentage points, on a model that has changed and a gap that has grown.

**Reported and not explained: the model has moved away from the engine.** Model r80 went 20.4 →
25.2 m across the two-ray and Jensen-foliage corrections while the engine's 12.9 m stayed put.
Both corrections were independently justified and neither was fitted to r80. That the model got
*less* accurate on this observable while getting more correct in its physics is not resolved here.

**Not concluded**, exactly as registered: whether the remaining 28.8% is attributable. The
precedent nominates transition width — r80 depends on the *shape* of the PDR curve, not only its
mean position — and explicitly declines to assert it. That stands. Two open items partially
explaining each other is the result.

## What this decides, and only this

Tier 3's geometry is designed against the **engine's 12.9 m**, not the model's 25.2 m. The
residual-corrected model at 16.44 m is the better model, but the engine is what the simulation
runs, and where they still differ the engine wins.

The residual remains an open item. This gives it its first consequence: it is now known to be
worth roughly 8.8 m of UWB r80, which is most of the reason the model and the engine disagree
about tier-3 anchor connectivity.
