# Time to first fix — registration

Written and committed **before computing anything**. Computed from traces already on disk, so this
registers **before looking**, not before running, and is labelled as such.

## Why this and not a lower `A`

The ladder left one untested axis: availability. **Lowering `A` until something passes is not the way
to test it** — and "A = 0.05 is acceptable for pinpointing" cannot be derived, because the phase
partition needed to justify a phase-specific availability does not exist in this simulation
(`REQUIREMENT_DERIVATION.md` §2).

**Time to first fix asks a different question**, one this project has never posed. Every metric here
is a fraction of cycles; the operational question is not *what fraction of the time is there a fix*
but **how long the rescuer waits for one**. It is a first-passage time, needs no phase partition and
no invented denominator, and maps onto an operation whose survival probability is an explicit
function of elapsed time.

## The time budget, sourced

**Sourced — total burial duration:**

- **Falk, Brugger & Adler-Kastner (1994)**, *Nature*, the first data-based survival analysis (422
  critically buried victims, 1981–91, Turnbull algorithm): a first phase of **high survival lasting
  to ~18 minutes**, after which survival falls steeply; ~21% by 60 min.
- **Procter et al. (2016)**, *Resuscitation*, 633 completely buried victims in Austria and
  Switzerland, updating both curves: *"a rapid initial drop in survival probability and a second drop
  to **25–28% survival probability after burial duration of ca. 35 min**, where an inflection point
  exists and the curve levels off."* Overall survival 56%.

**Not sourced — a pinpoint-phase budget.** The literature gives the **total** burial budget. Splitting
it across signal/coarse/fine/pinpoint requires knowing how long the preceding phases take, which is a
rescue-team performance question the survival curve does not answer. **That allocation is not
invented**, and no phase-specific time criterion is registered.

**What is registered is the whole-process bound**, which is what the sourced curve actually supports:

> **A fix within `R` must arrive within 18 minutes** for the located person to still be in the
> first, high-survival phase of the curve. At `dt = 2.5 s` that is **1080 s**, cycle 432 of 1440.

This is a bound on the entire localisation process, not on one phase, and it is used as such.

## Registered choices

**Origin: run start**, `t = 0`. The alternative — first entry into a stated region — needs a region
the simulation does not define, and inventing one is the same defect as inventing a phase partition.
Run start is arbitrary with respect to the node's trajectory, which is what makes it neutral between
the arms.

**Censoring: seeds with no qualifying fix are CENSORED, not dropped.** They are reported as a
censored fraction beside every statistic. Dropping them would select for the seeds on which the arm
worked — the survivorship defect already recorded in this project. Where the censored fraction
exceeds 50% the median is **undefined and reported as such**, not extrapolated.

**Statistics:** median and 90th percentile of time to first fix within `R`, per arm, per rung of the
existing ladder, with the censored fraction stated beside each.

**No prediction is registered on the arm comparison.** That is the ledger's question.

**One thing is predicted**, because it is arithmetic rather than a claim: at `R = 0.25 m` the
availabilities are 4.65% and 0.93%, i.e. about one cycle in 21 and one in 107. **If fixes were
independent**, median first-passage would be ~37 s and ~186 s. **They are not expected to be
independent** — fixes cluster when the node is near anchors — so the measured medians are predicted
to be **longer than those figures for both arms**. Falsifier: a median at or below the independent
estimate, which would mean fixes are dispersed rather than clustered and the occupancy account is
wrong.

## The limitation, beside the numbers

**The policy reads neither `R` nor time.** Its behaviour is byte-identical at every rung, so this
re-adjudicates one policy's output on a new metric. A policy that escalated when a fix had not yet
been obtained would be a different experiment and is not run.

## The stop condition

**This is the last experiment.** `R` has been swept, `A` cannot be lowered without inventing a
denominator, and time-to-fix is the last operationally-derivable framing. If it produces no operating
point, **the question is closed and the result says so explicitly** rather than deferring again.

**Claim 9 is not touched.**
