# The requirement ladder — result

Derivation registered and committed in `REQUIREMENT_DERIVATION.md` (`0ad8a4a`) before any
adjudication. Computed from `results/joint_criterion/`, 52 seeds, `source ec48e90 dirty 0` — the
per-cycle errors already on disk. No new simulation: `R` is an adjudication threshold, not a
simulation parameter.

## The ladder — every arm, every rung, losing regions at full resolution

`A ≥ 0.95` over all 1440 cycles, the registered population, unchanged.

| `R` (m) | tiered avail | meets | single-band avail | meets | verdict |
|---|---|---|---|---|---|
| **0.25** *(pinpoint, primary-verified)* | 0.0465 | **0/52** | 0.0093 | **0/52** | both FAIL |
| 0.50 | 0.0795 | 0/52 | 0.0380 | 0/52 | both FAIL |
| 1.00 | 0.1809 | 0/52 | 0.1408 | 0/52 | both FAIL |
| 2.00 | 0.4594 | 0/52 | 0.4366 | 0/52 | both FAIL |
| **3.00** *(fine-search onset, secondary)* | 0.6961 | **0/52** | 0.6905 | **0/52** | both FAIL |
| 5.00 | 0.9067 | 8/52 | 0.9129 | 9/52 | mixed |
| 10.00 | 0.9853 | 51/52 | 0.9878 | **52/52** | single-band only |
| **25.00** *(the published requirement)* | 0.9984 | 52/52 | 0.9968 | 52/52 | both pass |

**Outcome: no band exists, and every arm fails below ~5 m.** Outcomes 2 and 3 of the registered
table, together.

**There is no rung at which the tiered arm meets the requirement and the single-band arm does not.**
At `R = 10` the single-band arm is the one that passes 52/52 while the tiered arm misses a seed.

## The precision tier is real and it is not enough — the sharpest form of the finding

Paired availability advantage, by rung:

| `R` (m) | delta | 95% CI | |
|---|---|---|---|
| **0.25** | **+0.0372** | [+0.0334, +0.0410] | tiered ahead |
| 0.50 | +0.0415 | [+0.0369, +0.0460] | tiered ahead |
| 1.00 | +0.0401 | [+0.0345, +0.0457] | tiered ahead |
| 2.00 | +0.0228 | [+0.0161, +0.0295] | tiered ahead |
| 3.00 | +0.0056 | [−0.0020, +0.0132] | not significant |
| 5.00 | −0.0062 | [−0.0104, −0.0020] | **single-band ahead** |
| 10.00 | −0.0024 | [−0.0045, −0.0004] | **single-band ahead** |
| 25.00 | +0.0016 | [+0.0003, +0.0030] | tiered ahead |

**At the one primary-verified operational requirement — 0.25 m pinpointing — the tiered arm delivers
5.00× the availability of the single-band arm**, 0.0465 against 0.0093, with the interval comfortably
excluding zero. **The precision tier does exactly what it was built to do, and it shows up precisely
where the operation needs it.**

**And it is useless: 4.65% availability.** A tier that is five times better at a requirement neither
arm can meet has demonstrated its accuracy and its irrelevance in the same number.

**The crossover is instructive.** The tiered arm leads below 3 m, loses between 5 m and 10 m, and
leads again at 25 m — three sign changes. The middle region is where tier 2 alone is sufficient and
the tiered arm's extra machinery only costs it seeds; the low region is where tier 3's accuracy
matters and nothing has enough availability; the high region is the published requirement, where the
lead is +0.0016.

## What this settles

**Every falsification in this project traces to one fact, and the ladder is its clearest statement:**
the architecture's precision advantage exists only at accuracies where **no arm can meet an
availability requirement**, and at accuracies where availability is achievable, **the single-band arm
is as good or better**. There is no operating point in between.

That is why Claim 3's cost comparison was adjudicable only where tiering is least needed, why Claim 9
found no headroom at the switching seam, and why the repaired predicate buys +0.0016 availability for
2.17× the energy. **They are one finding seen from four directions.**

## The limitation, beside the numbers and not in a footnote

**The policy does not read `R`.** It escalates on PCS thresholds, so its behaviour is byte-identical
at every rung — the ladder re-adjudicates one policy's output at eight accuracies. **It shows whether
the current policy's output would meet a tighter requirement, not what a policy tuned for that
requirement would do.**

This matters most in the 0.25–2 m region, where the tiered arm leads by 2–4 percentage points on
4–46% availability. A policy that escalated aggressively when tight accuracy was required — rather
than on a confidence threshold that knows nothing about `R` — would plausibly do better there.
**Whether it would do well enough to cross 0.95 is not answered here**, and the gap between 4.65%
and 95% is large enough that no reasonable tuning closes it at 0.25 m.

**A tuned policy is a separate experiment and was not run.**

## What is not claimed

That the architecture is refuted at every requirement. The ladder covers `R` from 0.25 m to 25 m at
one geometry, one duration, one anchor density, with `A ≥ 0.95` held fixed. **A different `A` was not
swept** — and since the failures below 5 m are availability failures rather than accuracy failures,
a requirement with a lower `A` is the one axis on which this result says nothing.
