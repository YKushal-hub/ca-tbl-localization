# CAL-05 — the integrity check is a working detector, measured

**Written** 2026-09-05 from `results/c9_stage2` (160 runs, `dirty 0`) plus the 40 m point
already in `results/c9_escalate`. Policy `bounded`, arm `ca_tbl_full`, 20 seeds per point.
This is the calibration the thesis register defers to Phase 2.1 and the project has never
produced.

## The curve

| bias (m) | fault | precision | recall | TP | FP | FN |
|---|---|---|---|---|---|---|
| 10 | F1 | 0.897 | **0.035** | 201 | 23 | 5559 |
| 20 | F1 | 0.967 | 0.137 | 787 | 27 | 4973 |
| 40 | F1 | 0.980 | 0.389 | 2238 | 46 | 3522 |
| 80 | F1 | 0.977 | 0.641 | 3689 | 88 | 2071 |
| 160 | F1 | 0.966 | **0.831** | 4786 | 171 | 974 |
| 10 | F3 | 0.939 | **0.091** | 521 | 34 | 5239 |
| 20 | F3 | 0.962 | 0.308 | 1774 | 71 | 3986 |
| 40 | F3 | 0.974 | 0.610 | 3514 | 94 | 2246 |
| 80 | F3 | 0.967 | 0.845 | 4865 | 165 | 895 |
| 160 | F3 | 0.961 | **0.910** | 5243 | 211 | 517 |

Recall rises **monotonically** with bias magnitude on both fault models while precision
stays flat at 0.90–0.98. That is what a correctly behaving Mahalanobis test should do: a
larger displacement sits further from the prior, so it crosses χ²₀.₉₉ = 9.21 more often,
and the false-alarm rate — which the threshold fixes — does not move with the fault.

## This changes how stage 1 should be read

Stage 1 reported recall 0.29–0.72 at 40 m and I described the check as "precise and
insensitive". The curve says that was **a statement about the operating point, not about
the mechanism**. 40 m sits in the middle of the transition. The check is not insensitive;
it was being measured where it is half-sensitive.

Interpolating the 50 % crossing:

- **F1** (single-anchor, the canonical RAIM fault): ≈ **58 m**
- **F3** (common-mode on every tier-2 link): ≈ **33 m**

So the usable calibration statement is: *the integrity check reliably detects range biases
above roughly 60 m on a single displaced anchor, and above roughly 35 m when the bias is
common-mode, at a false-alarm rate under 4 %.*

## The limitation below 20 m is real, and it is not a threshold choice

At 10 m the check finds 3.5 % (F1) and 9.1 % (F3) of injected cycles. **No threshold fixes
that.** A 10 m common-mode bias is comparable to the tier-2 ranging uncertainty it is being
tested against, so it is not separable from ordinary measurement noise by a test on the
innovation — lowering χ² to catch it would raise false alarms on unbiased cycles by the
same mechanism. This is a detectability floor set by the ratio of fault size to measurement
noise, not a tuning failure.

That floor is the honest limitation to state: **the mechanism cannot detect faults smaller
than its own measurement noise, and no choice of threshold changes that.**

## F3 is easier than F1 at every magnitude

Consistently, and by a wide margin at the low end — 0.091 vs 0.035 at 10 m, 0.308 vs 0.137
at 20 m. A common-mode bias displaces every link and moves the whole fix; a single
displaced anchor is partly absorbed by the multilateration over the others. The two fault
models are reported separately for exactly this reason and must not be pooled.

## What this does NOT establish

- **It does not decide claim 2.** The registered decision rule is about the *availability*
  difference between `ca_tbl_full` and `ca_tbl_no_integrity`, and stage 1 showed that metric
  cannot resolve the treatment at all — mean per-seed difference `+0.00000` with no non-zero
  seeds on any F1 cell. A working detector and a decision rule that can see its effect are
  different things.
- **The prior is not the specified one.** The check tests against a constant-velocity prior
  propagated from previous radio fixes, not the independent prior `ch10 §10.7.2` requires.
  Every number here is evidence about the implemented check.
- **One policy, one window fraction.** `bounded`, 0.2. The recovery policy should not affect
  detection — detection precedes recovery — but that is an argument, not a measurement.
