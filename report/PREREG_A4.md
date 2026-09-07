# Pre-registration — A4, tier-1 measurement calibration

**Committed before the first cell runs.** Rule 13.

## The correction that makes this runnable

A4 has been carried as **blocked** on the grounds that its specification says to work
*"from the existing traces"* and those traces do not exist. That is true and it is not a
blocker: the traces are **regenerable**, the sweep parameters are `--gwGrid` and
`--boxHalf`, and the trace already carries every column the analysis needs — `trueX`,
`trueY`, `estX`, `estY`, `errM`, `meanSigmaM`, `meanSigmaRawM`, `traceSigma`,
`activeTier`, `fixTier`, `hadFix`, `nAnchors`.

**Cost, now that it is costed:** 4 densities × 5 seeds × 3600 s, six arms, ~5 minutes
wall-clock at 8-way parallelism. It was never an uncosted campaign; it was an uncosted
*claim*.

## The question

Is tier-1 reported range uncertainty **over-stated relative to realised error**? If it is,
the filter is discounting exactly the measurements that would carry the sparse-anchor
cycles, and the upper density edge could move **with no siting change at all** — which is
why A4 is ordered before A5.

## Design

- **Sweep:** `--gwGrid ∈ {9, 12, 16, 20}` at `--boxHalf=3000`, `--seed ∈ {1..5}`,
  `--duration=3600`, `--trace=true`.
- **Cycle selection:** tier-1 cycles only — `fixTier == 0` and `hadFix == 1`. The
  restriction matters: a cycle whose fix came from tier 2 or 3 says nothing about tier-1
  sigma, and pooling them would dilute the effect toward zero.
- **Statistic:** NEES on the position residual, 2 DOF.
  `NEES = errM² / σ²_pos`, expectation **2.0** under a correctly specified filter.

## Predictions

**P1 — the direction.** Tier-1 NEES on tier-1-only cycles is **below 2.0**, which is the
signature of an *over-stated* sigma: the filter's claimed uncertainty exceeds the error it
actually makes.

*Falsifier:* NEES ≥ 2.0. That would mean tier-1 sigma is correctly scaled or *under*-stated,
the premise of A4 is wrong, and **no recalibration is applied** — the finding is reported
as a negative and A5 proceeds without a calibration fix.

**P2 — monotonicity in density.** The ratio does not reverse sign across the four
densities. A calibration error is a property of the sigma model, so it should not appear at
one density and invert at another.

*Falsifier:* the sign of `NEES − 2.0` flips across densities. That would indicate the
effect is geometry-driven rather than calibration-driven, and **recalibrating would be
fitting geometry into a sigma model** — in which case A4 is closed as mis-specified rather
than acted on.

**P3 — the baseline reconciles.** The one passing cell reported NEES **2.95** at 20 seeds
(median 0.58, 3.3 % of cycles above the 95 % bound, concentrated in UWB). This sweep's
**all-tier** NEES should land in the same neighbourhood, or the two measurements are not of
the same quantity and the comparison in the work package is void.

*Falsifier:* all-tier NEES far from ~3. Reported as a reconciliation failure, and **no
calibration is applied on the strength of a statistic that does not reproduce.**

## Abort conditions

- **Fewer than 200 tier-1-only fix cycles pooled** → the estimate is too thin to act on;
  report the count and stop rather than recalibrate on noise.
- **Any cell fails to produce a trace** → that cell is excluded and said so, not silently
  dropped. Rule 12: the completeness check counts rows, it does not test for non-empty.

## Stated in advance

- **No recalibration is applied in this work package.** A4 asks whether sigma is
  over-stated. Changing the sigma model changes every result in the tree and is a
  campaign, not a bug fix — the same reasoning that keeps the PCS saturation defect
  recorded rather than corrected. **A4 delivers the measurement and the recommendation.**
- **NEES here is normalised by the filter's own position covariance**, which is the
  quantity the filter acts on. It is not the innovation covariance. The distinction is the
  same one recorded against `MahalanobisFromPrior`, and it is stated so the number is not
  later read as something it is not.
- Median is reported alongside the mean. The baseline's mean 2.95 against median 0.58 says
  the distribution is heavily skewed, so a mean alone would be a poor summary of either.
