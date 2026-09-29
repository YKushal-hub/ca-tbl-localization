# PREREG CAL-01 — PCS weight calibration by leave-one-seed-out cross-validation

**Registered before any cell runs** (rule 13). Written 2026-09-29. Engine source at
registration: `d24690c`; this commit adds only this file and the two scripts it names, so
`git status --porcelain -- src scratch` is empty and every run records `dirty 0`. The runs'
`run_id.txt` will cite **this commit's short SHA** (the simulator reads `git rev-parse
HEAD` at run time). `results/cal01_loocv_20260928/` does not exist at registration; every
file in it must postdate this commit.

Owner decision 2026-09-27: run CAL-01 after the CIR NLOS classifier. The default PCS
weights in code are **not** changed by this campaign, whatever it finds.

---

## 0. Why now, and what changed since K.8

The specification: *"w1, w2, w3 — empirically tuned weights, calibrated against the
ground-truth dataset … use leave-one-out cross-validation (LOOCV)."* The fitting machinery
exists (`src/lora-physics-engine/model/ca-tbl-loocv.{h,cc}`, `26d002e`, suite
`ca_tbl_loocv_test` 8/8). The calibration has never been run.

`TECH_DEBT` K.8 fixed the order: the monotone control first, then CAL-01, then live inputs
for the hop and NLOS terms, and warned that a refit alone cannot fix a score whose extra
terms are dead. The control was run (`report/PCS_FORMS_RESULTS.md`): every form loses,
whole 95% CI below zero, `weighted` on `ca_tbl_full` by −0.0114. Since then:

- **The NLOS term has a live input.** `--nlosClassifier --pcsNlosClassifier` (`86e43f2`)
  make term 3 `mean(1 − NLOS_score)` over the fix set's scored links. In the CIR_NLOS
  searcher-regime run (seed 7) it took 523 distinct values in [0.479, 1] on 712 of 717 fix
  cycles.
- **The hop term does not** (§4).

So this is CAL-01 with the one dead term that could be made live made live. It asks whether
that is enough.

## 1. Question

Does any weighting (w1, w2, w3) of PCS's three terms, fitted on training seeds, rank the
realised position error of a **held-out seed** better than the registered
monotone-of-filter-statistic control ρ(−traceSigma, error)? And what are the fitted weights?

## 2. Population, denominator, window, truncation (rule 1)

- **Unit:** one decision cycle of the measured tag (node 1) with `hadFix == 1` in
  `trace_<arm>_seed<N>.csv`.
- **Denominator:** per seed, that seed's fix cycles; per cell, the fix cycles of all 20 seeds.
- **Window:** t = 2.5 … 1800 s, 720 cycles per trace, **counted** (rule 12).
- **Truncation:** none can occur. Each cycle's terms and error are instantaneous values of
  that cycle, and nothing spans the window end.
- **Excluded by control flow, not by choice:** no-fix cycles. `Step()` returns before
  `ComputePcs` when fewer than three ranges arrive (`ca-tbl-policy.cc:965-994`), so those
  cycles have no score to calibrate.

## 3. Cells

**Common to every run** (`scripts/cal01_run.sh`):

```
--arms=ca_tbl_full,static_868 --duration=1800 --trace=true --pcsForm=weighted
--nlosClassifier=true --pcsNlosClassifier=true --seed=<N> --outdir=results/cal01_loocv_20260928/<regime>/seed_<N>
```

- `--pcsForm=weighted` is the only form that has w1..w3 (`ca-tbl-policy.cc:872-875`). Its
  defaults (2, 0.5, 0.5) are hand-set, ours, and the header says so. They are the operating
  point the data are collected at. `legacy` is (1, 1, 1): a point of the same family,
  evaluated below as LEG.
- `--nlosClassifier --pcsNlosClassifier` give t3 its live input. **Term 3's live signal
  comes from the 868 MHz and 2.4 GHz scores, not from the UWB CIR.** The CIR half has no
  NLOS positives in this channel model (`86e43f2`: UWB confusion matrix tp 0 / fp 0 / fn 0;
  the engine's outdoor NLOS probability at UWB range is about 0.1%; in the default topology
  no UWB link is ever polled). The 868 score is RSSI/SNR variability; the 2.4 GHz score is
  ToF against RSSI. It is **circular by construction**: no classifier can find NLOS the
  channel model did not create. The 868 detector is also weak (seed 7, searcher regime:
  tp 19, fp 358, fn 177, tn 4474), so t3 is live and noisy.
- `--nlosSource` stays at its default. The operative tag still sets the NLOS variance
  inflation of R and is t3's fallback when no link is scored. One change at a time.
- Off, at defaults: `--relayedRanging`, `--relayChain`, `--relaySlots`, `--fixRouting`,
  `--tier1Tdoa`, and every other flag not listed.

**Regimes**, 20 seeds each (1–20), 60 runs:

| regime | extra flags | why |
|---|---|---|
| `default` | none | the default deployment, where tier 1 blankets the box (`TECH_DEBT` L.2) |
| `arc` | `--boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180` | **tier 1 does not blanket.** Coverage probe: `ca_tbl_full` no-fix on 192, 455 and 190 of 720 cycles at seeds 7, 11, 13, and the fixes nearly all 868 MHz |
| `searcher` | `arc` + `--nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20` | NC2's regime. The finders carry tier 2 to the casualty: probe seed 7, 582 of 715 fixes on 2.4 GHz, 5 no-fix, so t1 spans tiers |

The probe numbers are exploratory: frozen reference `084a7ff`, legacy form, no NLOS feed,
`~/catbl_probe/v3/*_pops.jsonl`. They motivate the choice of regimes and nothing else.

## 4. `hopWeight` stays 0, and why

1. **The input is structurally zero.** The measured tag's `Step()` is called with
   `hopCount` as the literal `0` (`scratch/ca_tbl_causal_sim.cc:7874`) in every
   configuration, relayed or not. `--relaySlots` (`d24690c`) makes depth-2 chains fuse *on
   the relays*, but no hop count ever reaches the measured tag's PCS. A weight on a
   constant zero cannot be identified: it is the machinery's DEGENERATE branch.
2. **`CLAUDE.md` §2** forbids `hopWeight > 0`.
3. **The specification's calibration names w1..w3.** The hop term is a separate
   subtractive penalty outside the weighted sum (`ca-tbl-policy.cc:885-893`).

So K.8's condition "give the hop term a real input" is **not** met, and nothing here claims
it.

## 5. The terms, and how they are read

- t1 = 10/(10 + traceSigma) and t2 = 1/max(gdop, 1) are recomputed from the trace.
- t3 is not in the trace. It is back-derived from the trace's `pcs` under the run's weights:
  t3 = (pcs − 2·t1 − 0.5·t2)/0.5. At four printed decimals the error is ≤ 1.7e-4.
- **Rule 3:** the back-derivation is checked against a quantity it never read. t3 is
  recomputed per fix cycle from `nlos_links_<arm>_seed<N>.csv`: the fix tier's scored links
  at that instant, or the LOS fraction when none is scored, which is what the policy does
  (`ca-tbl-policy.cc:1065-1080`). This is abort A7.
- **Pipeline dry run, before registration, parsing only.** The loader was run on the
  CIR_NLOS probe output `c_nc2_feed` (`86e43f2`, legacy form, seed 7). The recomputed t3
  agreed within 5e-4 on 717/717, 718/718 and 555/555 fix cycles (`ca_tbl_full`,
  `static_868`, `static_2p4`). The fix-set size equalled the `nlos_links` row count on every
  one. No correlation or fit was computed on those data. The same run fired A1 on the
  mismatched invocation, and `done_nc1_feed` fired A2 on its 360 rows: two forced failures
  of the abort checks.

## 6. The LOOCV unit is the SEED

Cycles within a seed are serially dependent. The filter state carries over from cycle to
cycle, so consecutive traceSigma and error values are not independent draws. A cycle-level
hold-out lets the held-out cycle's neighbours into its own fit and flatters every fitted
model. **Leave-one-seed-out**: 20 folds per cell, training on 19 seeds and evaluating on
the 20th. This is still leave-one-out, as the specification asks, at the level where the
samples are independent. The machinery's cycle-level LOOCV is reproduced on the pooled set
and reported as descriptive only.

## 7. Models

| name | what | role |
|---|---|---|
| CTL | ρ(−traceSigma, error): the registered control of `PREREG_PCS_FORMS.md` | the control |
| CTL* | CTL whose sign is set by the training seeds' mean ρ | the control a sign-free fit must also beat |
| **FIT-R** | the weighting minimising the mean per-seed ρ(score, error) over the training seeds | **decides** |
| FIT-LS | the machinery's model verbatim: OLS of error in metres on (t1, t2, t3), no intercept | reported |
| FIT-LS1 | FIT-LS with an intercept | reported |
| HAND | (2, 0.5, 0.5), no fit: the trace's own `pcs` | reported |
| LEG | (1, 1, 1), no fit | reported |

**FIT-R's search grid.** Rank is invariant to positive scale, so only ratios matter:
w = s·(1, r2, r3) with s = ±1 and r2, r3 ∈ {0} ∪ {±10^(k/8), k = −48…48}; plus the w1 = 0
circle at 0.5°; plus the hand-set direction. That is 76 771 directions. The ratios are
logarithmic because t1 is saturated. On a tier-1 fix t1 sits near 1e-3 while t2 and t3
move by tenths, so letting t1 speak within tier 1 needs w2/w1 near 1e-3, which a uniform
grid on the sphere cannot resolve. **Grid index 0 is (1, 0, 0), which ranks exactly as the
control does**, and argmin takes the first index on a tie. So FIT-R can differ from the
control only where the training data pay for it. Abort A9 checks, on every seed, that
index 0 reproduces the control's ρ exactly.

**Why FIT-R decides.** It minimises the registered loss directly, over a family that
contains the control, so it is the fit most favourable to PCS. If it cannot beat the
control on held-out seeds, no weighting of these three terms can, to the grid's
resolution. If it can, its weights are the calibration. FIT-LS optimises squared error in
metres, which the escalation rule never uses: it thresholds a score, so only the ranking
matters. With no intercept, the near-constant t3 also has to carry the mean error level,
which weights t3 for a reason unrelated to ranking. FIT-LS is reported because it is the
committed machinery; it cannot overturn FIT-R.

## 8. Loss, estimator, verdict per cell

For each held-out seed s with ≥ 30 fix cycles (ours):

- ρ_m,s = Spearman(score_m, error) over s's fix cycles;
- A_s = ρ_CTL*,s − ρ_m,s, positive when model m ranks error better;
- A is the mean of A_s over seeds, with a seed-level percentile bootstrap: 10 000 resamples,
  RNG seed 20260929 + cell index, the same resamples for every model in a cell (paired);
- the primary level is **Bonferroni over the three primary cells, 98.33 % two-sided**, and
  95 % is reported beside it.

**Cell verdict for FIT-R:**

- **BEATS** if the 98.33 % CI of A against CTL* lies above 0 **and** the 98.33 % CI against
  the registered fixed-sign CTL does too;
- **LOSES** if the 98.33 % CI against CTL* lies below 0;
- **INDISTINGUISHABLE** otherwise.

The sign is kept, rather than using `PCS_FORMS`'s |ρ_PCS| − |ρ_ctl|, because K.8 measured
ρ(PCS, error) **positive** in the sparse regime: an absolute value would credit a
wrong-signed score. When both ρ are negative, the two forms are identical.

A cell is **NOT TESTABLE** if t3 was the live score on fewer than half of its fix cycles
(A4), or if fewer than 10 seeds have ≥ 30 fix cycles (A6). A cell with a constant term is
DEGENERATE. The thresholds 30, 10 and ½ are ours.

## 9. Decision rule, fixed now

Primary cells: `ca_tbl_full` in each of `default`, `arc` and `searcher`.

| verdict | condition |
|---|---|
| **CALIBRATED PCS INFORMATIVE** | FIT-R BEATS in ≥ 2 primary cells and LOSES in none |
| **REGIME-DEPENDENT** | FIT-R BEATS in ≥ 1 primary cell, otherwise |
| **CALIBRATION DOES NOT RESCUE PCS** | FIT-R BEATS in no primary cell, with ≥ 2 testable |
| NOT TESTABLE | fewer than 2 primary cells testable |
| HALTED | an abort condition fired (§11) |

**What each means for the PCS claim** (`TECH_DEBT` L.3, first row):

- *INFORMATIVE.* With calibrated weights, the three-term score carries information about
  error that the covariance trace does not. No PCS form has done this before. The claim
  moves from "measured negative" to "supported offline". The next step is closed loop:
  fitted weights, re-derived thresholds, and a registered campaign. The default weights
  are not changed here.
- *REGIME-DEPENDENT.* Supported only where FIT-R beats, and any claim has to be scoped to
  that regime.
- *DOES NOT RESCUE.* K.8's warning holds with the NLOS input live: no calibration of w1..w3
  makes PCS rank error better than the trace it is computed from. With the hop term
  structurally zero, the confidence-score contribution stands withdrawn as L.4 anticipates.
  That is a decision for the owner, recorded here and not taken here.

## 10. Predictions

| # | prediction | how it fails |
|---|---|---|
| P1 | **CALIBRATION DOES NOT RESCUE PCS.** FIT-R is INDISTINGUISHABLE or LOSES in all three primary cells | FIT-R BEATS in any primary cell |
| P2 | The all-seed FIT-R score ranks cycles like the trace: mean per-seed Spearman(FIT-R score, t1) ≥ 0.9 in ≥ 2 primary cells | < 0.9 in two or more |
| P3 | HAND loses to CTL* at 95 % in ≥ 2 primary cells, as `weighted` did in `PCS_FORMS` with the tag-based t3 | it loses in fewer than 2 |
| P4 | FIT-LS, the machinery verbatim, loses to CTL* at 95 % in ≥ 2 primary cells, for the intercept reason in §7 | it loses in fewer than 2 |
| P5 | Rule 3 holds: the recomputed t3 disagrees on ≤ 5 % of fix cycles in every cell. The dry run gave 0 % | > 5 % in any cell |

The reasoning for P1: t2 = 1/GDOP describes the same anchor geometry the EKF covariance
already integrates through JᵀR⁻¹J, and t3's live input comes from a weak detector. P1 is
the prediction that matters, and it predicts **against** the contribution.

## 11. Abort conditions, and what each firing looks like on disk (rule 9)

Checked before any verdict is computed. Any one halts the verdict.

| # | condition | firing on disk |
|---|---|---|
| A1 | every `run_id.txt` cites this commit, `dirty 0`, `profile optimized`, and the registered binary path, flag set and outdir, flag for flag | a `source`/`dirty` line or an invocation that differs |
| A2 | 60 run directories; each `arm_summary.csv` has exactly one row per arm; every trace has exactly 720 rows (counted); every `nlos_links` and `nlos_summary` file present | a missing file, or a row count ≠ 720 |
| A3 | every fix row finite, and the back-derived t3 within [−1e-3, 1 + 1e-3] | t3 outside [0, 1]: the form or the weights were not the registered ones |
| A7 | recomputed t3 mismatches (> 5e-4) on ≤ 5 % of each primary cell's fix cycles | a mismatch share above 5 % |
| A8 | `errM` equals hypot(est − true) within 1e-3 m on every fix row: the target recomputed from columns it does not share | a larger difference: a column shift or the wrong error |
| A9 | grid index 0 reproduces ρ(−traceSigma, error) exactly on every evaluable seed | any difference: the control is not in the family as claimed |

The runner refuses to write into an existing seed directory. If a run exits non-zero, the
campaign stops and the failure is reported. A rerun goes to a new directory, under an
amendment recorded in the results report.

## 12. Secondary, reported and never decisive

- `static_868` cells: the same analysis on an arm whose trajectory the PCS weights cannot
  steer, because it never escalates.
- Every model in every cell at 95 %; single-term ρ for t2 and t3.
- Fitted weights on all 20 seeds, with 95 % seed-bootstrap intervals: FIT-R as ratios and
  normalised to sum 3 (the machinery's scale), and FIT-LS / FIT-LS1.
- RMSE, the machinery's own loss: seed-LOO for FIT-LS against t1 alone (the machinery's
  control), and the machinery's cycle-level LOOCV verdict on the pooled set.

## 13. What this campaign cannot establish

- **Closed-loop behaviour.** The data were generated at the hand-set operating point. A
  policy using fitted weights would escalate differently and see different cycles (the
  off-policy limit of any one-shot calibration).
- **Thresholds.** `pcsThresholdUp/Down` were chosen against the hand-set scale. Fitted
  weights would need re-derived thresholds, which is a campaign of its own.
- **Anything outside the linear family** w·t. The PCS form is linear, and so is the
  calibration.
- **The hop term** (§4) and **the UWB CIR** (§3).
- **Anything about the rest of the evidence tree.** One binary, one campaign. `PCS_FORMS`
  numbers came from another topology, another binary and a tag-based t3. Comparing with
  them is qualitative only (rule 8 across binaries).
- **Grid resolution.** The ratio step is 10^(1/8) ≈ 1.33. Whatever a finer search might
  gain was not measured.
- **The meaning of "ground truth".** It is the simulator's truth. The fitted weights
  describe this error model, not a deployment.

## 14. Compute

60 runs, two arms each, 1800 s. Probe wall times were 5–21 s per two-arm run in these
regimes, so the campaign takes under 10 minutes at 3 concurrent runs. This is the ceiling
while the Bar-3 campaign holds 6 cores. The analysis is single-threaded; the self-test ran
3 synthetic cells in 100 s, so about 6 minutes for the 6 real cells. About 120 MB of traces
and `nlos_links` stay **untracked**. Only `run_id.txt` (which carries a SHA-256 of every
output), `arm_summary.csv` and `.md` files are tracked, as `.gitignore` allows.

## 15. The analysis was observed to fire and to stay silent, before registration

`python3 scripts/cal01_loocv.py --selftest` builds three synthetic 20-seed cells through
the same `analyse_cell` code path:

| fixture | error model | FIT-R verdict | As, 98.33 % CI |
|---|---|---|---|
| null | depends on traceSigma alone | INDISTINGUISHABLE | −0.0002 [−0.0004, +0.0000] |
| positive | traceSigma × GDOP, so geometry carries error beyond the trace | **BEATS** | +0.0785 [+0.0691, +0.0874] |
| sign | the trace ranks error upside down | INDISTINGUISHABLE | −0.0002 [−0.0005, +0.0000] (A against the fixed-sign CTL +1.54: beating a wrong-signed control is refused) |

`SELFTEST PASS`, with A9 exact on every seed.

## 16. Files and the analysis command

- `scripts/cal01_run.sh` holds the registered invocations. Run it as `bash
  scripts/cal01_run.sh batch <regime> 1 20`.
- `scripts/cal01_loocv.py` is the analysis. It replaces an unreviewed, uncommitted draft
  that an earlier session left behind. That draft had the no-intercept OLS decide and a
  direction grid that did not contain the control. It was never run on data.
- The analysis command:

```
python3 scripts/cal01_loocv.py --root results/cal01_loocv_20260928 --source <this commit, short> \
    > results/cal01_loocv_20260928/ANALYSIS.md
```
