# A1 — dwell drives coverage, and coverage does not rescue the conjunction

**Written** 2026-09-05 from `results/dwell_b3000`, 100 cells, 5 dwells × 20 seeds,
**100/100 recording `dirty 0`**. No code change: `--dwell` has existed throughout and
this campaign had simply never been run.

## The measurement

`ca_tbl_full`, `boxHalf=3000`, `gwGrid=3`. Coverage fraction is
`cyclesInUwbCoverage / steps` over all 1440 cycles; a cycle out of coverage counts
against.

| dwell | predicted coverage | measured | delta | median cep50M | median A | meets (A≥0.95, R≤25) |
|---|---|---|---|---|---|---|
| 90 | 4.57 % | 3.39 % | −1.18 pp | 7.05 m | 0.6257 | **0/20** |
| 150 | 7.39 % | 5.01 % | −2.38 pp | 7.00 m | 0.6257 | **0/20** |
| 245 | 11.53 % | **7.24 %** | −4.29 pp | 6.53 m | 0.6365 | **0/20** |
| 400 | 17.54 % | 11.32 % | −6.23 pp | 6.01 m | 0.6698 | **0/20** |
| 800 | 29.85 % | 21.97 % | −7.88 pp | 5.08 m | 0.7042 | **0/20** |

## Three findings, and they separate cleanly

### 1. The dwell/transit model is falsified as a predictor

The registered falsifier was measured coverage failing to track
`dwell/(dwell + 1880)` to within 1–2 percentage points. It does not: the delta is
**−1.18, −2.38, −4.29, −6.23, −7.88** — it grows monotonically with dwell, which is
precisely the signature the pre-registration named for *the transit estimate being
wrong*.

`transit ≈ 1880 s` is too small. Measured coverage sits consistently below prediction
and the gap widens, so the walker spends longer in transit at b3000 than the model
assumed. The **245 s** figure, derived from that model to restore b1000's 11.49 %,
delivers **7.24 %** instead. Reaching 11.49 % takes **dwell ≈ 400 s**.

### 2. The mechanism holds anyway

Coverage rises monotonically with dwell, 3.39 % → 21.97 %, and median accuracy
improves with it, 7.05 m → 5.08 m. Dwell *is* the lever on coverage; only the constant
was wrong. That is a calibration error in the prediction, not a refutation of the
hypothesis that coverage fraction is what dwell controls.

### 3. Coverage is not sufficient, and this is the result that matters

**The requirement is unmet at every dwell — 0/20 in all five cells.** Not marginally:
median availability runs 0.6257 → 0.7042 against a threshold of 0.95, while median
accuracy is already comfortable at 5–7 m throughout.

So the binding constraint at b3000 is **availability**, and dwell barely moves it —
+0.08 across a nine-fold increase in dwell. Restoring the coverage fraction restores
neither the requirement nor the conjunction.

This is the registered caveat firing: *"If the conjunction returns under one and not
the other, coverage fraction is not the whole story."* It returned under neither.
**Coverage fraction is necessary but not sufficient**, and the conjunction claim cannot
be rescued by a deployment parameter — at least not by this one.

## What this does to the claim

The hope was to reframe *"the conjunction fails at scale"* — fatal — into *"the
conjunction requires a precision-tier coverage fraction above a threshold"* — a
deployment requirement. **That reframing is not available.** Coverage was raised past
b1000's level and the requirement still failed 0/20, because what fails at b3000 is
availability rather than precision-tier coverage.

The honest statement is narrower and it is what the data supports: at b3000 with 3
gateways, the system does not meet `(A ≥ 0.95, R ≤ 25 m)` at any dwell tested, and the
reason is availability, not accuracy and not coverage.

## What this campaign cannot establish

- **Whether more facility sites would do it.** Raising dwell and adding sites both
  raise coverage but are different interventions — more sites shortens transit, longer
  dwell lengthens stationary time. The facility-count sweep needs a `BuildDeployment()`
  change and is the cleaner test; this was the cheap one.
- **Whether a corrected transit constant changes anything.** It would move the
  predicted column, not the measured one.
- **Anything about gateway count.** `gwGrid=3` throughout.
