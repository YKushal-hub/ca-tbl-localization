# Does the unattributed UWB residual account for the model–engine r80 gap?

Written and committed **before the computation**. Nothing here is revised after a result exists.

## The two numbers, and why the test is nearly free

| quantity | value | source |
|---|---|---|
| engine tier-3 r80 | **12.9 m** | `ca_tbl_causal_sim.cc:220`, 64-site decorrelated probe, 2000 trials/point |
| model tier-3 r80 | **25.2 m** | `linkbudget.py` via `r80()`, computed this session |
| gap | **12.3 m** | |
| unattributed UWB margin residual | **~3.7 dB** | `REPORT_1_Propagation_Module.md:577`, `:1231` |
| UWB `dr80/dmargin` | **2.08 m/dB** | `DEFECTS.md:1113` |

`3.7 × 2.08 = 7.7 m`, against a 12.3 m gap — **about 63% on the linearised estimate.** The test
replaces that linearisation with the exact computation: re-run `r80()` with the residual applied
as a margin offset and read the result.

## This is a replication, not a first look — and the precedent matters

`DEFECTS.md:1115-1120` already ran this comparison once, at the then-current numbers: a **2.05 dB**
margin error against a **7.6 m** gap (model 20.4 m, engine 12.8 m). It found the margin error
explained **about half**, and attributed the remaining ~26 percentage points provisionally to the
**transition width** — "a separate finding from the margin residual and *not asserted as
understood*."

So the registered expectation below is a replication of a result that already came out at "about
half", under a model that has since changed (two-ray, Jensen-corrected foliage) and whose r80 has
moved **away** from the engine, 20.4 → 25.2 m, while the engine's figure stayed put. That
divergence is itself reported, whichever way the test falls.

## Registered

**Prediction.** Applying the measured UWB residual as a margin offset moves the model's tier-3 r80
from 25.2 m toward the engine's 12.9 m, closing **at least half** of the 12.3 m gap — i.e. a
corrected r80 of **19.1 m or below**.

**Falsifier.** It closes **less than a quarter** (corrected r80 above 22.1 m), or it **overshoots**
below 12.9 m. Either way the residual and the r80 disagreement are separate problems and both stay
open.

**Not predicted.** Whether the remainder is attributable at all. The precedent already names
transition width as the leading candidate for the remainder and explicitly declines to assert it.
Two open items partially explaining each other is a result; forcing them to explain each other
fully is not.

**Direction is fixed in advance.** The residual is applied as a **negative** margin offset
(`margin_offset = −3.7`), because the model over-predicts range and a residual that closes the gap
must reduce margin. Applying it in the sign that helps, after seeing the answer, would be fitting.

## What the outcome decides

Only one thing, and it is stated now so the test cannot be re-purposed afterwards: **which r80
tier 3's geometry is designed against** (`experiments/B2_GAUGE.md` §2, and the ring-radius rule).
It does not decide whether the residual is understood, and it does not close the residual as an
open item.
