# Time to first fix — result

Registered in `TIME_TO_FIX_REGISTRATION.md` (`a1c9d2f`) before computing. 52 seeds,
`results/joint_criterion/`, `source ec48e90 dirty 0`.

## The headline: **zero censoring at every rung**

| `R` (m) | tiered median | p90 | censored | single median | p90 | censored |
|---|---|---|---|---|---|---|
| **0.25** | **132.5 s** | 422.5 | **0/52** | **170.0 s** | 575.0 | **0/52** |
| 0.50 | 40.0 | 130.0 | 0/52 | 30.0 | 97.5 | 0/52 |
| 1.00 | 15.0 | 30.0 | 0/52 | 12.5 | 27.5 | 0/52 |
| 3.00 | 7.5 | 12.5 | 0/52 | 2.5 | 7.5 | 0/52 |
| 25.00 | 5.0 | 5.0 | 0/52 | 2.5 | 2.5 | 0/52 |

**Every seed, both arms, every rung, obtains a fix — including at 0.25 m, the tightest
primary-verified requirement.** The censoring rule was registered and it never fired.

Against the sourced 18-minute budget (Falk 1994's first survival phase):

| `R` (m) | tiered inside 18 min | single-band inside 18 min |
|---|---|---|
| **0.25** | **52/52** | 51/52 |
| 0.50 – 3.00 | 52/52 | 52/52 |

## This reframes the ladder rather than contradicting it

At `R = 0.25 m` the availability ladder reported **4.65%** and **0.93%** — both arms failing
catastrophically. The same traces give median times to first fix of **132.5 s** and **170.0 s**,
both an order of magnitude inside an 18-minute budget.

**Both numbers are correct and they answer different questions.** Availability asks *what fraction
of the time is a fix available*; time-to-fix asks *how long until one arrives*. **For a burial the
second is the operational question**, because survival is a function of elapsed time, not of duty
cycle.

> **The apparent catastrophic failure at tight accuracy is a property of the metric, not of the
> system.** A system that produces a 0.25 m fix within 132 seconds and then loses it is useless by
> availability and adequate by time-to-fix — and for locating a buried person, adequate is the
> correct reading.

This is the strongest qualification the ladder's negative result has received, and it was obtained
by asking a question the project had never posed rather than by relaxing a threshold.

## The arm comparison: **no advantage**, and the rule reverses with `R`

| `R` (m) | mean paired delta (tiered − single) | 95% CI | |
|---|---|---|---|
| 0.25 | −43.7 s | [−123.8, +36.5] | not significant |
| 0.50 | +12.6 | [−7.9, +33.1] | not significant |
| 1.00 | +4.7 | [−0.3, +9.7] | not significant |
| 2.00 | +3.1 | [+0.9, +5.2] | **single-band faster** |
| 3.00 | +3.4 | [+2.1, +4.7] | **single-band faster** |
| 5.00 – 25.00 | +1.7 to +3.0 | all exclude zero | **single-band faster** |

**At 0.25 m the tiered arm's 37.5 s median advantage is not significant** — CI [−123.8, +36.5],
because first-passage times at that accuracy are highly variable. **At every `R ≥ 2 m` the
single-band arm is significantly faster**, by 1.7–3.4 s.

**So time-to-fix produces no operating point either.** The one rung where the tiered arm is nominally
ahead is the rung where the difference cannot be resolved at 52 seeds; everywhere it can be resolved,
the single-band arm wins.

## The registered arithmetic prediction: **split**

Predicted: both medians longer than the independent-fix estimate, because fixes cluster.

| arm | independent estimate | measured | ratio | |
|---|---|---|---|---|
| `ca_tbl_full` | 36.4 s | 132.5 s | **3.64×** | **longer, as predicted** |
| `static_2p4` | 185.5 s | 170.0 s | **0.92×** | **shorter — falsifier fires** |

**The tiered arm's fixes are strongly clustered; the single-band arm's are essentially independent.**
That is the occupancy account confirmed for one arm and refuted for the other, and it is exactly what
the geometry implies: tier-3 fixes bunch at facilities, tier-2 fixes are spread across the walk. The
prediction was stated for both arms and should have been stated per arm — **a single prediction
covering two mechanisms is one prediction too few.**

## The stop condition, applied

**The question is closed.** `R` has been swept from 0.25 m to 25 m; `A` cannot be lowered without
inventing a phase partition that does not exist; and time-to-fix — the last operationally-derivable
framing, on a budget sourced from the survival literature — gives the tiered arm no advantage at any
accuracy where the difference can be resolved.

**There is no operating point for the tiered architecture at this deployment, on any metric this
project can derive from the operation.**

That is a complete negative, and it is bounded: one geometry, one anchor density, one mobility model,
`A ≥ 0.95`, and a policy that reads neither `R` nor elapsed time. **A policy that escalated when a
fix had not yet arrived is a different system and was not built.**

## The limitation, beside the numbers

The policy reads neither `R` nor time; its behaviour is byte-identical at every rung. This
re-adjudicates one policy's output on a new metric. It is the same limitation the ladder carries, and
it bites hardest here: **time-to-fix is precisely the quantity a time-aware policy would optimise**,
and no such policy exists in this codebase.
