# Pre-registration — A6, the hysteresis sweep (CAL-03 / CAL-04)

Written 2026-09-05, **before any cell of this campaign has run.**
Source commit: `1164e5d`. Build profile: optimized.

## 1. Question

Does the escalation/de-escalation hysteresis band, or the number of consecutive healthy
cycles required before demoting, change accuracy or energy — and by enough to matter?

These are CAL-03 and CAL-04. The register defers both to Phase 2.1. They have **never been
swept**, not because a sweep was run and found flat, but because `pcsThresholdDown` and
`nDeescalate` were not registered with `cmd.AddValue` until `f578f16` earlier today. This
is the first time the question can be asked.

## 2. Population and denominator

- **Band** `--pcsHysteresisBand` ∈ {0.0, 0.1, **0.2**, 0.4, 0.8}. The band sets
  `pcsThresholdDown = pcsThresholdUp + band` on every tier. **0.2 is the built-in value**,
  so that cell is the baseline and must reproduce it.
- **Dwell** `--nDeescalate` ∈ {1, **5**, 10}. 5 is the built-in.
- 15 combinations × 20 paired seeds = **300 runs**.
- **Arms:** `ca_tbl_full` and `fixed_threshold` only. Two arms, not six: the static arms
  never escalate, so hysteresis cannot reach them, and including them would spend three
  times the compute to measure nothing. `fixed_threshold` is included because it also
  escalates and gives a second arm to check the direction against.
- **Geometry:** `boxHalf=2000`, `nlosSource=propagation`, `arbitrateDutyBudget=true`,
  `pcsForm=legacy`. Defaults otherwise.
- **Observation window:** the full 3600 s run, all decision cycles.
- **Truncation rule:** a run ending mid-cycle contributes its completed cycles; a cycle
  that has recorded a fix counts, one that has not does not. No cycle is discarded
  retroactively because the run ended.

## 3. Metric and estimator

Per combination, against the **baseline cell (band 0.2, nDeescalate 5)** at the same seed:

- **`cep50M`** and **`totalJ`** — mean of per-seed differences, two-sided sign test.
  Paired: the seed fixes the channel realisation.
- **Escalation and de-escalation counts** as mechanism witnesses, not claims.

A single-seed difference is never reported (rule 8).

## 4. Prediction

1. **The baseline cell reproduces `baseline_v7`-style behaviour exactly** — band 0.2 with
   nDeescalate 5 is the built-in configuration, so its per-seed values must equal a run
   with neither flag passed. This is a null control inside the sweep, and it is the first
   thing checked.
2. **Widening the band reduces escalation count monotonically.** A wider band means the
   score must improve more before demoting, so the arm stays on the higher tier longer —
   fewer round trips.
3. **The accuracy response is flat within the paired CI across the whole grid.** The
   hysteresis band decides *when* a tier changes, not *how well* the tier ranges, and the
   decision layer has already been shown not to carry information beyond the filter trace.

**Prediction 3 predicts a null result, and a flat response is a reportable outcome** —
CAL-03 and CAL-04 would then be answered as *measured and inconsequential*, which is worth
more than their present state of *never asked*.

## 5. Falsifier

- For 1: the baseline cell differs from a no-flag run. The flags are not inert at their
  defaults and `f578f16`'s null control was insufficient.
- For 2: escalation count does not fall monotonically with band.
- For 3: any cell moves `cep50M` by more than its paired CI. Hysteresis would then be a
  live tuning axis and CAL-03/CAL-04 would need a calibrated value rather than a verdict.

## 6. Decision rule — three outcomes, fixed now

- **FLAT.** No cell moves `cep50M` beyond its paired CI. CAL-03/CAL-04 are answered:
  measured, inconsequential at this geometry. **Expected.**
- **LIVE AXIS.** At least one cell moves accuracy beyond CI with a consistent direction
  across both arms. CAL-03/CAL-04 become calibration targets and a value must be chosen
  and justified.
- **INCONCLUSIVE.** Movement that differs in sign between the two arms, or that does not
  order with the band. Reported as such, not resolved by choosing the arm that agrees.

## 7. Abort conditions

- If the baseline cell does not reproduce the no-flag run, the campaign stops: prediction 1
  failing means the instrument is wrong and nothing downstream is interpretable.
- If any cell records `dirty != 0`, that cell is not citable and the campaign stops rather
  than reporting a mixed-provenance set.

## 8. What this campaign cannot establish

- **Anything at other geometries.** One `boxHalf`, one environment.
- **Anything about the repaired PCS forms.** `pcsForm=legacy` throughout, so this measures
  hysteresis on the score the tree was built with, not on `weighted` or `gated`.
- **An optimum.** A flat response says the axis does not matter here; it does not locate a
  best value, and a grid of 15 points could not resolve one if the surface were flat.
