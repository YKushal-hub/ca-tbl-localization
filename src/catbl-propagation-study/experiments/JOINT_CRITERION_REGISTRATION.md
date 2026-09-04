# The joint availability-and-energy criterion — prospective registration

Written and committed **before the first cell**. A genuine pre-registration: these cells do not
exist, and they use **seeds the observation never touched**.

## Why prospectively, and on fresh seeds

The observation that motivates this — the repaired arm leading `static_2p4` on availability — came
from `results/siting_repair/colocated_min`, seeds 1–20. **Registering a claim on the data that
suggested it is the move this project has refused throughout.** So the criterion is registered
before running, and the cells are **seeds 21–72**, disjoint from the observation.

## Power calculation

Paired per-seed availability difference, `ca_tbl_full` (repaired) − `static_2p4`, measured on the
observation set:

    mean delta   +0.007222
    per-seed sd   0.018474
    n = (z0.975 + z0.80)^2 * (sd/delta)^2
      = 7.8489 * (0.018474 / 0.007222)^2
      = 7.8489 * 6.5433 = 51.36  ->  52 seeds

| n | half-width | power |
|---|---|---|
| **52** | 0.005021 | **0.805** |
| 60 | 0.004675 | 0.857 |
| 80 | 0.004048 | 0.938 |

**Registered seed count: 52**, seeds 21–72. Run exactly. Score once. Do not extend.

**The winner's-curse caveat, registered rather than discovered.** δ = +0.0072 was **selected** as the
observation worth pursuing, so it is likely an overestimate and 52 seeds is therefore optimistic
power — the same caveat that was registered for `h1_power` and that resolved in the warned direction
(+0.0053 → +0.0037). A **null** result here is not evidence that the repaired arm fails to lead; it
is evidence the lead is smaller than the selected estimate, and **it must not be rescued by adding
seeds.**

## The registered output is a report, not a verdict

The precedent is this project's own H_reliability handling: *"Attach no PASS/FAIL. There is no
registered rule for this claim and one must not be invented now."* That applies with more force
here, because `b49df46` §4 has just been shown unable to express a reliability advantage
(`[M-33]`).

**The registered output is a sentence of this form, with the blanks filled from measurement:**

> At `boxHalf = 2000`, `nlosSource = propagation`, evidence gate on, PCS `min`: the repaired tiered
> arm meets (A = 0.95, R = 25 m) in **N of 52** deployments against `static_2p4`'s **K of 52**, at
> **X×** its energy. Whether that trade is worth making is a deployment decision.

**No PASS. No FAIL. No threshold on X.** Inventing an energy threshold now would be inventing the
rule after seeing the direction, which is exactly what `b49df46` §4 exists to prevent — and its
being ill-suited here is not licence to replace it with something more convenient.

## What is reported alongside, and what is not predicted

Reported: per-seed availability with a 95% CI, seeds-meeting-spec for both arms, `totalJ` for both,
and the M1 ratio with its bootstrap CI.

**Not predicted:** the direction or size of any of them. The availability lead is an observation
from a different seed set; this measures whether it reproduces, and reproduction is the question.

**One quantity is predicted**, because the mechanism constrains it and the last pass established
the constraint: **`j_2p4` per tier-2 cycle stays within ±10% of 0.00889 J.** The 3.92 J tier-2 rise
was shown to be *occupancy* (`cycles_2p4` 959 → 1355) at a flat per-cycle cost (0.00847 → 0.00889),
not overhead. If per-cycle cost moves materially on fresh seeds, that account is wrong.

## Design

    --duration=3600 --boxHalf=2000 --gwGrid=5 --nlosSource=propagation
    --escalationRequiresEvidence=true --pcsForm=min --trace=true
    --arms=ca_tbl_full,static_2p4
    seeds 21..72

Identical to `siting_repair/colocated_min` except the seed range. Default siting (co-located), since
complementary siting was measured to make things worse.

**Claim 9 is not touched.**
