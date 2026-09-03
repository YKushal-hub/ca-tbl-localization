# B4 — the native fault, and a registered mechanism I cannot register

Written before implementing B4. It revises one instruction in the work order, on evidence from
the source rather than by argument.

## What the existing injector actually does

`ca_tbl_causal_sim.cc:2552-2571`, with the window at `ca_tbl_causal_sim.cc:2386-2388`:

```
biasActive = injectBiasFraction > 0 && t in [biasStartS, biasStartS + fraction*duration)
if (injectRangeBiasM != 0 && biasActive && hitThisAnchor) range += injectRangeBiasM;
```

It adds a **constant +40 m** to every range to the faulted anchor, on every cycle inside a
contiguous window covering 20% of the run.

## The correction

The work order asks: *"register the prediction that the effect is larger here than in the
synthetic case, and state the mechanism: a persistent bias accumulates in the filter where a
per-cycle draw partially averages out."*

**The existing fault is not a per-cycle draw.** It is already persistent — constant in
magnitude, on the same anchors, for every cycle it is active. Nothing about it averages out.
The stated mechanism is false against the source, and registering a prediction built on it would
be registering a premise the code contradicts.

The two faults do differ, in two ways, and neither is the one given:

1. **Geometry.** A range bias `+B` inflates every range to that anchor by exactly `B`,
   unsigned, independent of where the tag is. A **position** error of magnitude `D` displaces
   the anchor by a vector, so the induced range error is `D·cos θ` in the tag's bearing `θ` —
   **signed**, and varying as the tag moves.
2. **Duration.** The range bias is windowed at 20% of the run. A calibration error is permanent:
   there is no un-faulted stretch of the run to serve as an internal baseline.

## What follows, quantitatively

Over uniform bearings, for a position error of magnitude `D`:

    mean |range error| = 2D/pi = 0.637 D          rms = D/sqrt(2) = 0.707 D

Verified numerically over 200 000 bearings: at `D = 40` m, mean 25.43 m against the analytic
25.46, rms 28.25 against 28.28.

**So at equal magnitude the position fault is the weaker perturbation, by a factor of about
0.64.** To match the existing fault's 40 m of mean range error, a position error must be
`D = πB/2 = 62.8 m`.

And the tag **dwells** at facility sites, so within a dwell `θ` is nearly constant and the
position fault presents as a constant range bias of `D·cos θ` — a draw from `[−D, +D]` that is
fixed for the duration of the dwell, and which can be near zero for an unlucky geometry.

## The prediction registered instead

**Not** "the effect is larger". At matched magnitude the effect is **smaller**, and at matched
*mean range error* (`D = 62.8` m against `B = 40` m) it is comparable but higher-variance,
because `cos θ` spreads it across dwells.

Registered, before the run:

| # | prediction | falsifier |
|---|---|---|
| 1 | At equal `D = B = 40` m the position fault produces a **smaller** availability loss than the range-bias fault, ratio in **0.5–0.8** | ratio outside that band ⇒ the `cos θ` geometry is not what governs, and the fault model needs re-deriving before B4 is scored |
| 2 | Per-dwell variance of the induced error is **higher** for the position fault, because `cos θ` is fixed within a dwell and redrawn across dwells | equal or lower variance ⇒ the dwell structure is not resolving the bearing, and the two faults are closer than the geometry says |

**Magnitude choice, fixed now:** run `D = 40` m for the matched-magnitude comparison **and**
`D = 62.8` m for the matched-mean-range-error comparison. Reporting only one of the two would
let the result be read as whichever the number flattered.

## Where B4 can run at all

From `experiments/B2_GAUGE.md`: exclusion needs redundancy, and tier 2 at three anchors per site
has none. The fault as specified lands on tier-2 anchors, exactly where the current `h = 1`
campaign (`results/h1_power/RESULT.md`) is measured. So B4 must either move to tier 3 (five
anchors, redundancy +3) or raise `--t2Anchors` to 5. **That decision belongs to B2's sweep and
is not pre-empted here** — but B4 cannot be run on the deployment as configured, and this is the
second finding that traces back to the same missing redundancy.
