# A4 — tier-1 measurement calibration: sigma is over-stated, and the baseline does not reconcile

**2026-09-06.** Pre-registered in `report/PREREG_A4.md`, committed at `fd6b507` **before the
first cell ran**. Sweep: `--gwGrid ∈ {9,12,16,20}` at `--boxHalf=3000`, seeds 1–5,
`--duration=3600`, `--trace=true`. **20/20 cells complete at exactly 8 646 rows each.**

## The claim this withdraws first

A4 was carried as **blocked** because its specification says to work *"from the existing
traces"* and those traces do not exist. That was true and it was never a blocker: the
traces are regenerable, and the whole sweep took **about 50 minutes** of wall-clock at
8-way parallelism. **It was an uncosted claim, not an uncosted campaign**, and it sat in
the register behind a sentence from the work package that nobody had tested.

## Result

`NEES = 2·errM² / traceSigma`, 2 DOF, expectation **2.0** for a correctly specified filter.

### Tier-1-only fix cycles (`fixTier == 868MHz`, `hadFix == 1`)

| gwGrid | n | mean NEES | median | above 95 % bound |
|---:|---:|---:|---:|---:|
| 9 | 23 499 | 0.635 | 0.240 | 0.7 % |
| 12 | 23 490 | 0.682 | 0.273 | 0.7 % |
| 16 | 23 645 | 0.645 | 0.274 | 0.6 % |
| 20 | 23 642 | 0.710 | 0.315 | 0.6 % |
| **pooled** | **94 276** | **0.668** | **0.274** | **0.6 %** |

**P1 HOLDS.** 0.668 < 2.0. The filter's claimed uncertainty **exceeds the error it
actually makes** — sigma is over-stated by roughly **√(2/0.668) = 1.73× in sigma terms**.

**P2 HOLDS.** The sign of `NEES − 2` is negative at all four densities. The effect is a
property of the sigma model, not of geometry, so recalibrating would not be fitting
geometry into a noise model.

**P3 DOES NOT RECONCILE.** All-tier pooled NEES is **1.342** (median 0.288, 0.8 % above the
95 % bound) against the baseline's reported **2.95** (median 0.58, 3.3 %). That is a factor
of ~2.2 on the mean and ~2 on the median, in the same direction on both. The
pre-registration named this as a falsifier and named the consequence: **no calibration is
applied on the strength of a statistic that does not reproduce.**

The two are not obviously the same measurement — the baseline is "the one passing cell" at
a different geometry and 20 seeds, and NEES is geometry-dependent — but *that is exactly
the point*. Until it is established that they measure the same quantity, the baseline
cannot be used as the comparison the work package asks for.

## What is **not** concluded, and why

**The 1.73× is not attributable to tier-1 ranging sigma alone.** NEES here is normalised by
the filter's **posterior** covariance, which is inflated by the measurement noise `R` *and*
by the process noise `Q`. Restricting to cycles whose fix came from tier 1 makes `R`
tier-1-dominated; it does not remove `Q`. **Reporting "tier-1 range sigma is 1.73× too
large" would be attributing the whole of a posterior discrepancy to one of its two
sources** — the same error class as §5's margin attribution, which cost this project a
wrong result once already.

What is established: **the tier-1 fix path is systematically over-conservative**, by a
factor near 1.7 in sigma, consistently across a 2.2× range of anchor density.

**The isotropy step is an approximation.** The trace carries `trace(P_pos)`, not the matrix,
so `NEES = 2·errM²/traceSigma` assumes `P_pos = diag(p,p)`. The covariance is demonstrably
not isotropic — `Pxy` is non-zero throughout the C5 relay measurements. This is the right
direction indicator and the wrong number to quote to three figures.

**No recalibration is applied.** The pre-registration said so before the run, and it stays
true: changing the sigma model changes every result in the tree. That is a campaign, not a
bug fix — the same reasoning that keeps the PCS saturation defect recorded rather than
corrected.

## Recommendation

1. **The A4 question is answered in the affirmative, with a caveat on attribution.** Tier-1
   uncertainty is over-stated, and the filter is therefore discounting exactly the
   measurements that would carry sparse-anchor cycles — which is the mechanism the work
   package proposed.
2. **Before any recalibration, separate `R` from `Q`.** A run with process noise held at
   zero over a static segment would attribute the 1.73× to one or the other. That is a
   cheap experiment and it is the one that makes a calibration defensible.
3. **Resolve the P3 discrepancy first.** Either the baseline's 2.95 and this 1.342 are the
   same statistic under different geometry — in which case say so and stop citing 2.95 as
   a target — or they are not, in which case one of them is mis-specified.
4. **A5 may proceed.** The ordering rule exists so that A5 does not answer a question a
   calibration fix has already answered. No calibration fix has been applied, so A5
   measures siting against the same filter every other result used.
