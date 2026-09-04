# Pre-registration — PCS form comparison (D-1 repair)

Written 2026-09-05 03:5x, **before any cell of this campaign has run.**
Source commit: `c1868ef` (wip/lane-b). Build profile: optimized.

## 0. What prompted it, stated so it is not mistaken for the result

A single-seed smoke run at seed 7 moved `ca_tbl_no_integrity` from 11 escalations and
`cep50M` 81.52 m under `--pcsForm=legacy` to 180 escalations and 1.79 m under
`--pcsForm=gated`. **That is not a result.** CLAUDE.md rule 8: a single-seed difference is
never reported as one, and arms consume the shared `macRng` stream at their own rate. It is
the reason this campaign is being run, and it is written down here so that a reader can see
the hypothesis existed before the measurement rather than after it.

## 1. Question

Does repairing the PCS saturation defect — giving the filter-uncertainty term the power to
veto an escalation, which the unweighted sum denies it — change the system's accuracy and
energy, and does the repaired score carry information beyond the filter statistic it is
computed from?

## 2. Population and denominator

- **Forms:** `legacy`, `v2`, `weighted`, `gated` — 4 levels of `--pcsForm`.
- **Arms:** `static_2p4`, `ca_tbl_full`, `ca_tbl_no_integrity`, `fixed_threshold`,
  `static_868`, `static_uwb`.
- **Seeds:** 1–20, paired across forms. The same seed is the same channel realisation.
- **Geometry:** `boxHalf=2000`, defaults otherwise, `nlosSource=propagation`,
  `--arbitrateDutyBudget=true`. One geometry, deliberately: this campaign asks whether the
  repair does anything, not where.
- **Observation window:** the full 3600 s run, all decision cycles.
- **Truncation rule:** a run that ends mid-cycle contributes its completed cycles only; a
  cycle that has already recorded a fix counts, one that has not does not. No cycle is
  discarded retroactively because the run ended (censoring is asymmetric — rule 2).
- 4 × 20 = **80 runs**, six arms each, traced.

## 3. Metric and estimator

Primary, per arm and per form:

- **`cep50M`**, median radial error. Reported as the **mean of per-seed differences**
  against the `legacy` cell at the same seed, with a two-sided sign test beside it.
  Paired, because the seed fixes the channel.
- **`totalJ`**, energy. Same estimator.
- **Escalation count**, as a mechanism witness, not a claim.

Secondary, and this is the one that decides the contribution:

- **The monotone-of-filter-statistic control.** For each form, compute Spearman ρ between
  the per-cycle PCS and the per-cycle radial error, and separately between
  `−traceSigma` (a monotone function of the filter's own covariance trace, which PCS is
  computed from) and the same error. A score that carries information beyond the filter
  statistic must beat the control by more than the paired bootstrap CI. The legacy score
  measured ρ = −0.199 against a control at −0.200; v2, pre-registered before measurement,
  failed the same test.

Bootstrap is **seed-level**, 10 000 resamples. Not row-level: cycles within a seed are
serially correlated and a row-level resample would understate the interval.

## 4. Prediction

1. `gated` and `weighted` both raise escalation count and lower `cep50M` on
   `ca_tbl_no_integrity` relative to `legacy`, by more than the paired CI, on ≥ 15 of 20
   seeds. Direction: accuracy improves, energy rises.
2. `ca_tbl_full` moves **less** than `ca_tbl_no_integrity`, because the integrity gate
   already forces escalations that the saturated score would otherwise suppress — so the
   arm with the gate has less room to gain from repairing the score.
3. **Both repaired forms still fail the monotone control.** Two forms have already failed
   it; the defect being repaired is saturation, which is about the score's *range*, not
   about whether it carries information the covariance trace does not already have.

Prediction 3 is the one that matters, and it predicts the repair does **not** rescue the
confidence-score contribution.

## 5. Falsifier

- For 1: no significant change in `cep50M` on `ca_tbl_no_integrity` under either repaired
  form. The saturation defect would then not be load-bearing for accuracy, and the smoke
  observation was a seed-7 artefact.
- For 2: `ca_tbl_full` moves as much as or more than `ca_tbl_no_integrity`.
- For 3: either repaired form beats the monotone control by more than the paired CI. That
  would mean the information test was failing the *form*, not the concept, and CAL-01's
  LOOCV becomes worth doing properly.

All three are reachable by this campaign.

## 6. Decision rule — three outcomes, fixed now

- **REPAIRED AND INFORMATIVE.** Prediction 1 holds and prediction 3 is falsified. The
  confidence-score contribution is live again; CAL-01's LOOCV becomes the next work item and
  A8's abort condition is lifted.
- **REPAIRED BUT UNINFORMATIVE.** Prediction 1 holds and prediction 3 holds. The defect was
  real and worth fixing — the system works better — but the score still carries nothing
  beyond the filter trace. The engineering claim stands and the novelty claim does not.
  **This is the expected outcome.**
- **NOT REPAIRED.** Prediction 1 falsified. The saturation defect does not drive the
  accuracy loss and the seed-7 observation was noise.

"No arm meets `(A ≥ 0.95, R ≤ 25 m)`" is **not** a PASS under any branch. A criterion no
competitor meets decides nothing.

## 7. Abort conditions

- If the `legacy` cells of this campaign do not reproduce `results/baseline_v7` at seed 7
  byte-for-byte, the tree has moved and the campaign stops until that is explained.
- If any cell records `dirty != 0`, that cell is not citable and the campaign stops rather
  than reporting a mixed-provenance set.

## 8. What this campaign cannot establish

- **Whether the default weights are right.** They are hand-set. CAL-01 specifies LOOCV and
  it has still not been run, so a `weighted` result is a result about *these* weights.
- **Whether the repair helps at other geometries.** One geometry is run.
- **Whether `gated`'s discontinuity is well-placed.** `pcsGateFloor = 0.10` is ours, chosen
  as three times the target sigma, and is not swept here.
- **Anything about the integrity check**, which is a different mechanism measured by A2.
