# Phase 10 pre-registration

Written and committed **before** any Phase 10 run. Every result produced under this
plan cites this file's commit sha. The point of the discipline is that the decision
rule cannot be adjusted after seeing the numbers; it caught an n=1 false positive
earlier in this project and it is the reason the regime change below is not
result-tuning.

## 0. What prompted this, stated so it can be checked

Claims 1 and 2 have been tested at `boxHalf=1000, gwGrid=3`. Measured from the
existing trace `results/causal/trace_ca_tbl_full_seed7.csv`, no new run required:

    TIER-2 coverageFrac : 1.0000    boundary crossings/hour : 0.0
    vis2p4              : min 5, max 12, mean 7.39, never below 3 in 1440 cycles
    TIER-3 coverageFrac : 0.1104    boundary crossings/hour : 8.0

The tag is inside tier-2 coverage at every cycle of the run, with a floor of five
rangeable anchors against a requirement of three. The tier-2 escalation decision is
therefore never posed: the correct action is "use tier 2" at every cycle, and any
policy that sometimes does otherwise can only lose. This is a property of the
geometry, not of the architecture.

Tier 3 is exercised (11% coverage, 8 crossings/hour), so the scenario is not
degenerate for tier-3 decisions. The defect is specific to tier 2.

## 1. Testability predicate

A geometry is **testable** for claims 1 and 2 if both hold, measured on the
`ca_tbl_full` arm:

    coverageFrac_tier2      in [0.35, 0.85]
    boundaryCrossings/hour  >= 4

`coverageFrac` is the fraction of cycles with `vis2p4 >= 3` (three rangeable tier-2
anchors, the minimum for a fix). `boundaryCrossings` counts transitions of that
predicate along the trajectory, per hour.

**Justification of the band, fixed in advance.** Above 0.85 the escalation decision
is nearly always "yes" and carries little information; below 0.35 tier 2 is mostly
unavailable and the decision is nearly always "no". Neither tail can distinguish a
good policy from a constant one. The crossing requirement exists because a geometry
can sit at 0.5 coverage with the tag spending half the run inside one region and
half outside, which poses the decision twice rather than continuously.

**These bounds are pre-registered as proposed and are derived in A.1.** If the
derivation contradicts them, the derivation is recorded, the bounds are updated
BEFORE the claim runs, and both versions appear here. What must not happen is
choosing the band after seeing which geometry makes a claim pass.

Geometries outside the band are reported as **degenerate**, with their measured
coverage, and excluded from claim testing. `boxHalf=1000, gwGrid=3` is expected to
be excluded; that exclusion is a finding about every prior negative result, not a
convenience.

## 2. Availability requirement (A, R)

Derived from the use case rather than from convenience.

`R` — a ground searcher sweeping on foot detects a casualty within roughly one
visual radius in open terrain and much less in rubble or vegetation. A fix is
**actionable** if its error is inside that radius, because the searcher walking to
the reported position will then find the subject without a further search pattern.
Pre-registered: **R = 25 m**, to be justified against the sweep-width literature in
the results write-up, or corrected there with the correction recorded.

`A` — the fraction of cycles at which an actionable fix must exist. A coordinator
directing a team cannot use a system that loses the subject for long intervals.
Pre-registered: **A = 0.95**.

Both numbers are fixed here. If the analysis wants different ones, it reports both
and says which was pre-registered.

## 3. Metrics

Three, reported together. M1 is primary.

**M1 — energy to sustain the requirement.**

    M1 = totalJ, over arms achieving availability >= A at accuracy <= R
    Arms failing the requirement are EXCLUDED, not scored.

Exclusion is the correct treatment, not a penalty: a system that cannot locate the
subject 5% of the time is not a candidate, and scoring it against one that can
invites the comparison the current metric makes.

**M2 — availability-weighted error.** Every cycle scored, nothing censored:

    awRMSE = sqrt( mean over ALL cycles of err^2 )
    no-fix cycles are charged the dead-reckoned error: last known position
    propagated by elapsed time x assumed speed

**M3 — `accuracyPerJoule = 1/(rmseM x totalJ)`, unchanged.** Retained and reported
alongside. Where M1 and M3 disagree, the disagreement is the finding and both are
quoted. M3 is never quoted alone.

**Why the metric is being replaced, stated as a defect not a preference.** `rmseM`
is computed over cycles that produced a fix. Already measured (report section 1.4):

    arm             cep50M    rmseM     outage
    static_2p4      6.03 m    57.86 m   46.25%
    ca_tbl_full    26.12 m    57.16 m    1.54%

`static_2p4`'s median is scored on the 53.75% of cycles where it worked, and an arm
that fails to hear an anchor spends no energy, so low coverage also improves its
denominator. The metric rewards the failure mode. This is the censoring rule the
project already applies to range measurements, applied to the primary metric.

## 4. Decision rule — three outcomes, fixed in advance

At each testable geometry, 20 seeds, paired per seed.

    PASS          ca_tbl_full meets (A, R) AND its M1 is lower than every
                  single-band arm that also meets (A, R), with the geometric mean
                  of per-seed ratios < 1.0 and the seed-level bootstrap 95% CI
                  entirely below 1.0.

    FAIL          ca_tbl_full meets (A, R) but some single-band arm that also meets
                  (A, R) achieves lower M1, CI entirely below 1.0 in that direction.

    INCONCLUSIVE  the CI spans 1.0, or no arm other than ca_tbl_full meets (A, R)
                  (nothing to compare against), or fewer than 2 testable geometries
                  exist.

"No competitor meets the requirement" is deliberately INCONCLUSIVE and not PASS. An
uncontested win is not evidence of superiority, and recording it as PASS is exactly
the move this document exists to prevent.

## 5. Procedure

20 seeds. Paired per seed. Geometric mean of per-seed ratios. Seed-level bootstrap
95% CI, 10000 resamples. Unchanged from prior phases.

## 6. Trajectory

The mobility model is **already** a facility tour, not a random walk — it was
changed for this reason previously (see the `Tourist` docstring: a random walk
"almost never enters" a small coverage region, so the escalation decision is never
posed). The measurement above shows the tour does cross tier-3 boundaries 8 times
per hour.

**No trajectory change is pre-registered.** The tier-2 problem is coverage
saturation, not mobility: a trajectory cannot cross a boundary that does not exist
anywhere in the deployment. If A.1 finds geometries in the band whose crossing rate
is still below 4/hour, a trajectory change will be pre-registered then, in an
amendment to this file, before those runs.

## 7. What this plan does not do

No gate is weakened, no threshold relaxed, no parameter tuned toward a target, no
existing number reinterpreted. The operating regime and the scoring function change;
both are fixed here in advance; whatever comes out is reported. If the architecture
still loses under an availability requirement it can meet and the single-band arms
cannot, that is the answer and it goes in the report.
