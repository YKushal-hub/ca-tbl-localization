# PREREG CAL-02 — PCS weights and per-tier thresholds by leave-one-out, on the preset

**Registered before any cell runs** (rule 13). Written 2026-10-03. Engine source at
registration: `995eeb5` (M.7, on the preset as at `17073d1`):
- the preset's geometry is GEO-01 stage 2's registered fallback, N8_ring: no candidate met
  the criterion (report/GEO01_RESULTS.md);
- M.7 is fixed: the LOOCV control now has an intercept.
This commit adds this file, `scripts/cal02_run.sh` (the runner) and
`scripts/cal02_analyse.py` (the analysis, which reuses CAL-01's registered FIT-R machinery
from `scripts/cal01_loocv.py` unchanged), and nothing else. The outputs go to
`results/cal02_2026-10-03/`, which does not exist at registration.

Owner decision 3: PCS weights and thresholds come from LOOCV through a registered campaign
(`docs/SPEC_CONFORMANCE_PLAN.md` §3.3). **Owner decision of 2026-10-03, taken before this
registration.** Under the 5 km preset the threshold labels are never true: `visUwb` is
always 0 and `vis2p4` never reaches 3. So the weights are fitted on the preset as planned,
and the thresholds on the preset **pooled with its endgame variant** (`--boxHalf=750
--finderMode=1`, finders converging), where the labels have positives. A threshold with too
few positive cycles keeps the legacy firing rate, rescaled to the calibrated score, and is
reported as unfitted.

---

## 1. Data (`cal02_run.sh gen`)

- **The data-generating configuration** is fixed here to avoid the circularity the plan's
  review found: the full preset (`--specProfile=true`) with PCS left **legacy**. The
  preset sets no `--pcsForm`.
- Two configurations, each seeds 1-40, arms `ca_tbl_full` and `fixed_threshold`, 1 800 s,
  traces on:
  - **main**: the preset;
  - **end**: the preset with `--boxHalf=750 --finderMode=1`.
- **Fit seeds 1-20; held-out seeds 21-40.**

## 2. The terms

For each `ca_tbl_full` fix cycle, from the trace:
- t1 = 10 / (10 + traceSigma);
- t2 = 1 / max(GDOP, 1);
- t3 = PCS_raw − t1 − t2, where PCS_raw is the trace's `pcs`, divided by 0.5 on SUSPECT
  cycles (DC-7 halves a suspect fix's PCS under the preset's downweight).

A fix with no geometry computed (two ranges) has GDOP 0 and PCS 0 under every form. It has
no terms, so it is excluded from both fits and counted. In the G24 runs that was 108 of
10 992 fix cycles, and GDOP 0 coincided with PCS 0 on every one.

## 3. Weights (`cal02_analyse.py fit`)

- **FIT-R**, as CAL-01: the direction on CAL-01's registered grid that minimises the mean
  per-seed Spearman ρ(score, error) over the main fit seeds 1-20.
- `--pcsForm=calibrated` takes only non-negative weights. A negative component is clamped
  to 0, the clamp is reported, and the clamped weights (normalised to sum 3) are what is
  used and scored.
- **Scored** on the main held-out seeds 21-40: A = ρ_control − ρ_FIT per seed, where
  ρ_control = ρ(−traceSigma, error). The mean of A gets a 95 % seed bootstrap (10 000
  resamples, seed 20261003). Verdict:
  - BETTER if the interval is above 0;
  - WORSE if it is below 0;
  - INDISTINGUISHABLE otherwise.

## 4. Thresholds (`cal02_analyse.py fit`)

The calibrated score is s = (w · T), halved on SUSPECT cycles: the value the policy compares
with the threshold. Each threshold has a tier, a label and a firing rule:

| threshold | cycles on | label (from the trace) | fires when |
|---|---|---|---|
| `pcsThrUp868` | 868 MHz | `vis2p4 >= 3` (2.4 GHz would solve) | s < thr (escalate) |
| `pcsThrUp2p4` | 2.4 GHz | `shouldUwb` (UWB would solve) | s < thr (escalate) |
| `pcsThrUpUwb` | UWB | `shouldUwb` (UWB still solves) | s < thr (stay) |
| `pcsThrDown2p4` | 2.4 GHz | `vis868 >= 3` (868 MHz would solve) | s > thr (demote) |
| `pcsThrDownUwb` | UWB | `vis2p4 >= 3` (2.4 GHz would solve) | s > thr (demote) |

- **Pool:** the fit seeds 1-20 of both main and end.
- **Fitted** when the tier has at least 30 pooled cycles with at least 30 positive and 30
  negative labels. The threshold maximises F1 over 1 001 quantiles of s. Ties go to the
  higher threshold: more escalation for an Up, fewer demotions for a Down.
  - It is scored by leave-one-seed-out over the 20 fit seeds, and on the pooled held-out
    seeds 21-40.
- **Unfitted** when there are fewer than 30 positive or negative cycles. The threshold
  fires at the legacy rate: the quantile of s at the fraction of those cycles on which the
  legacy rule (legacy PCS against the legacy threshold) fired.
- **Unexercised** when there are fewer than 30 pooled cycles on the tier. The threshold is
  the legacy value times median(s) / median(legacy PCS) over all pooled fix cycles.
- **Hysteresis:** a Down that lands below its tier's Up is raised to Up × 1.12 (the legacy
  ratio), and the raise is reported.

The eight values go to `fit.json` as the flags of `--pcsForm=calibrated`.

## 5. Evaluation (`cal02_run.sh eval`, `cal02_analyse.py eval`)

The calibrated preset (`--specProfile=true --pcsForm=calibrated` with the eight values),
main and end, held-out seeds 21-40, `ca_tbl_full`. It is compared on the same seeds with
the legacy preset's `ca_tbl_full` and with `fixed_threshold` (both from gen) on:
- availability (the five subjects);
- tag 0's CEP50/95;
- escalations;
- tier occupancy;
- subject energy.

The shift between the fit seeds and the held-out seeds is reported.

## 6. Predictions, made before any cell

- **P1.** The fitted weights are essentially the trace term. Their mean rank agreement
  with t1 on the fit seeds is at least 0.95 (CAL-01 found (3, 0, 0) in three regimes).
- **P2.** On held-out seeds the calibrated score does not rank error better than the
  trace: A is INDISTINGUISHABLE or WORSE, never BETTER.
- **P3.** `pcsThrUp2p4` and `pcsThrUpUwb` are UNFITTED or UNEXERCISED. Even the endgame
  has a subject with at most one finder in UWB reach (report/MS3_UWB_CHECKPOINT.md), so
  `shouldUwb` is rarely or never true.
- **P4.** As the plan expected, the calibrated `ca_tbl_full` behaves more like
  `fixed_threshold` than the legacy one does: its mean absolute tier-occupancy difference
  to `fixed_threshold` is smaller, in both configurations.

Each is written up as held or failed in `report/CAL02_RESULTS.md`.

## 7. Abort conditions

- A run exits non-zero: stop, fix the cause, never drop the cell.
- Back-derived t3 lies outside [−0.001, 1.001] on more than 1 % of a configuration's
  geometry-bearing fix cycles. Then the decomposition is wrong, and nothing fitted on it
  is valid: stop.
- `results/cal02_2026-10-03/gen` or `/eval` exists before its phase: the runner refuses.

## 8. What follows

If the evaluation shows nothing worse than the legacy preset on availability, the preset
adopts `--pcsForm=calibrated` with these values, in its own commit (G24 re-run). "Worse"
means the mean availability is lower by more than 0.01 in either configuration. Otherwise
the preset keeps legacy PCS, and the report says why. CAMP-SPEC then runs on whichever it
is.
