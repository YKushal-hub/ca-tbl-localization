# excl_test — pre-registration

Written before any result directory exists. Committed so the timestamp precedes the first
cell.

## The finding this tests

Claim 9's falsification stands for what was tested: whole-fix **rejection**, in a
fixed-anchor single-hop scenario, at `R = 25 m`. `ca-tbl-policy.cc` discards the entire EKF
update on a gate trip — every range that went into the fix, including the good ones.

The field's standard response is **Fault Detection and Exclusion**: identify which
measurement is faulty, drop that one, re-solve from the remaining redundancy. With ~5
tier-2 anchors typically in range, excluding one bad range leaves an over-determined solve;
rejecting the fix leaves nothing and the cycle falls back to the prior. **That is an
availability loss by construction, and availability is exactly what the ablation measured
`full` losing** — −0.031 (F1) and −0.054 (F3).

This tests whether the response policy is the mechanism of that loss. It is a policy change
inside the existing scenario: **no new topology, no physics change.**

## Design

Identical to `c9_clean` and `abl_clean` except for the policy under test:

    --boxHalf=2000 --gwGrid=5 --escalationRequiresEvidence=true
    --nlosSource=propagation --pcsForm=v2 --suspectForcesEscalation=false
    --mahalUseR=true --injectRangeBias=40 --injectBiasFraction=0.2
    --injectFaultAnchors=0 --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true

Cells: `{exclude}` × `{F1, F3}` × 20 seeds = **40**.

`suspectForcesEscalation=false` remains in the flag set, so this measures
rejection-plus-exclusion with no escalation following a trip — the same conditioning as the
campaigns it is compared against.

## The mechanism, as built

On a gate trip, leave-one-out over the ranges: drop each in turn, re-solve from the
remaining `n−1`, and test the **same** gate at the **same** threshold.

- exactly one exclusion clears → accept that fix, fuse the surviving ranges
- none clears → fall back to the current behaviour (`RevertToPrior`)
- **several clear → also fall back.** The faulty range is not identified and choosing one
  would be guessing. Counted separately as `ambiguous`
- `n < 4` → fall back; `n−1` would be below the 3 ranges a 2-D fix needs

Per-cell counters: cleared / none-cleared / ambiguous / too-few / ranges dropped.

## Registered prediction

**`exclude` loses less availability than `none` in every cell**, because it retains the
remaining measurements where `none` discards the cycle.

**Registered magnitude:** the availability gap `full − no_integrity` should shrink from
−0.031 (F1) and −0.054 (F3) to **at least half** of those — i.e. to −0.016 or better on F1
and −0.027 or better on F3.

## Falsifier

If `exclude` is **no better than `none`** on availability, the "rejection discards
information" hypothesis is **wrong**, and the falsification of Claim 9 is not about the
response policy. That is reported as the finding, not as a null result.

## Explicitly NOT predicted, and not scored

**Whether `exclude` beats `no_integrity`.** That is the question the campaign answers, and
predicting it would be predicting the conclusion.

## Abort conditions

- degeneracy pre-flight on one seed, mechanism `integrity`, must report **LIVE** before
  twenty seeds are spent
- any sidecar recording other than `dirty 0`
- the null control failing byte-identity

## This is not a revival of Claim 9

Claim 9's ledger entry is **not modified**. It is falsified, measured on clean provenance,
and it stays. This is a separately numbered result about the response policy's contribution
to that falsification. If `exclude` closes the gap but still does not beat `no_integrity`,
that is a **better-characterised falsification** — the response was suboptimal *and*
fixing it is not enough at `R = 25 m`.
