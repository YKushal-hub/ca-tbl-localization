# K13 — searching works, and it is not enough. The binding constraint is search capacity.

Four cells, 20 paired seeds, 4 simulated hours, `dirty 0`. Stationary subject 11.6 km from
the camp, **walking searchers in both arms**, so the only thing that moves between a pair
is the steering behaviour.

---

## The result

| | hold | search | median Δ | sign test |
|---|---|---|---|---|
| **sparse WAN + mesh** | | | | |
| cep95 | 16 590 m | 20 758 m | +0.0 | 8+/6− p 0.79 |
| fix cycles | 2 | 2.5 | +0.0 | 9+/8− p 1.00 |
| energy | 96.6 J | **84.0 J** | **+6.6 J** | 15+/5− **p 0.041** |
| **WAN + mesh** | | | | |
| cep95 | 17 971 m | 15 066 m | −238 | 8+/10− p 0.81 |
| fix cycles | 5 | **25** | +3.0 | 13+/5− p 0.096 |
| energy | 78.8 J | **65.5 J** | **+8.5 J** | 19+/1− **p 0.0000** |

**Searching is significantly cheaper** — 6.6–8.5 J, p < 0.05 in both deployments — and it
makes **five times more contact** in the full deployment (5 → 25 fix cycles), though that
falls just short of significance at p 0.096.

**It does not significantly improve the position estimate anywhere.**

## And nothing beats doing nothing

The last-known-position baseline is **11 625 m**. Search reaches 15 066 m and 20 758 m.
**Both steering modes, in both deployments, are worse than simply reporting where the
subject was last seen.**

## Why, and it is not the algorithm

Availability is **0.04% and 0.43%** — 2 to 25 fix cycles out of 5760. The searchers are
walking, sweeping outward correctly, and covering ground at 0.9 × 1.2 m/s. They still
almost never come within 1515 m of the subject.

**Four searchers, four hours, a 12 km box, one stationary target.** The geometry does not
close. A spiral from the camp reaches 15.5 km of radius in four hours, but the *area* it
sweeps at useful density is a small fraction of the box, and the subject occupies one point
in it.

**The binding constraint is search capacity, not localisation.** No radio architecture
fixes this: the system cannot localise something it never hears, and with this party size
it almost never hears it. The localisation layer only begins to matter once the search is
dense enough to make contact at all.

## What this establishes, and it is worth stating positively

1. **For a MOVING subject the architecture works and beats published estimators** — K10:
   2 016 m against MLAT's 20 831 m, 20+/0−, and better than no system at all.
2. **For a STILL subject at this party size, no localisation system helps**, because the
   contact rate is ~0. That is a statement about the *deployment*, and it is quantifiable:
   the useful question is how many searcher-hours are needed before contact probability
   becomes non-negligible, and that is a sweep worth running.
3. **Searching still pays for itself on energy** even when it does not improve the fix —
   significantly, in both deployments.

## The honest limit of the still-subject work

Three defects were found and fixed getting here — `--speed` freezing the searchers, the
sweep radius bounded by the subject's speed, and the `finderMode` comma. Each was real. But
**fixing all three did not produce a system that finds a stationary subject**, and the
reason is arithmetic rather than software.

The next question is not another steering change. It is: **how many searchers, over how
long, before the contact rate stops being zero?** That is a resource curve, and it is the
one number a search planner would actually want.
