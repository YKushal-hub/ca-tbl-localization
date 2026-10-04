# CAL-01 — calibrated PCS is the covariance trace, and nothing more

**Written** 2026-09-29 from `results/cal01_loocv_20260928`: 60 runs (3 regimes × seeds
1–20), each with `ca_tbl_full` and `static_868`. All 60 sidecars read
`source c019c0a dirty 0 profile optimized`. **Registered** in
`report/PREREG_CAL01_LOOCV.md` (`c019c0a`, committed 15:09:12 +0530) before the first cell
ran: the earliest output file is 15:09:18 and the last 15:13:52 (`RUNS.md`). The analysis
is `scripts/cal01_loocv.py`, committed with the registration and applied unchanged.
`ANALYSIS.md` is its verbatim output.

**Verdict: CALIBRATION DOES NOT RESCUE PCS** — the branch prediction P1 named. All five
registered predictions held.

---

## 1. The registered test

Leave-one-seed-out, 20 folds per cell. FIT-R is the weighting (w1, w2, w3) of
t1 = 10/(10 + traceSigma), t2 = 1/GDOP and t3 = the live NLOS term that minimises mean
per-seed ρ(score, error) on 19 seeds. It is scored on the 20th against the registered
control ρ(−traceSigma, error). A = ρ_control − ρ_FIT-R, so positive means the calibrated
score ranks error better. The interval is a 98.33 % seed bootstrap (Bonferroni over the
three primary cells, 10 000 resamples).

| regime | `ca_tbl_full` fix cycles | ρ control | ρ FIT-R | A, 98.33 % CI | verdict |
|---|---|---|---|---|---|
| `default` — tier 1 blankets | 12 193 / 14 400 | −0.6863 | −0.6847 | −0.0016 [−0.0066, +0.0030] | INDISTINGUISHABLE |
| `arc` — tier 1 does not | 8 063 / 14 400 | −0.4346 | −0.4390 | +0.0045 [−0.0129, +0.0217] | INDISTINGUISHABLE |
| `searcher` — 10 658 fixes on 2.4 GHz | 14 042 / 14 400 | −0.4684 | −0.4673 | −0.0011 [−0.0044, +0.0001] | INDISTINGUISHABLE |

The sign-refitted control never flipped (0 of 20 folds in every cell), so it equals the
registered fixed-sign control here and both give the same A. **No calibration of the three
weights ranks error better than the covariance trace, in any regime.** That includes
one where tier 1 leaves 44 % of cycles without a fix, and one where fixes span two tiers.

The secondary arm agrees. `static_868`, whose trajectory the weights cannot steer, is
INDISTINGUISHABLE in all three regimes: +0.0135 [−0.0155, +0.0436],
+0.0055 [−0.0089, +0.0208] and +0.0067 [−0.0024, +0.0165] at 95 %.

## 2. The calibrated weights: the trace term, with rounding error on the other two

The fit on all 20 seeds, in the confidence convention, with 95 % seed-bootstrap intervals:

| regime | FIT-R direction | w2/w1, 95 % CI | w3/w1, 95 % CI | normalised to sum 3 | rank agreement with the trace |
|---|---|---|---|---|---|
| `default` | +(1, −4.2e-4, −1.3e-6) | [−7.5e-4, +1.8e-4] | [−3.2e-4, +1.8e-4] | (3.001, −0.0013, −0.000004) | 0.9844 |
| `arc` | +(1, +5.6e-5, −1.3e-5) | [+1.0e-5, +1.3e-4] | [−5.6e-5, +3.2e-5] | (3.000, +0.0002, −0.00004) | 0.9628 |
| `searcher` | +(1, −2.4e-4, −7.5e-4) | [−5.6e-4, +7.5e-3] | [−4.2e-3, −1.0e-4] | (3.003, −0.0007, −0.0023) | 0.9997 |

w1 > 0 in 10 000 of 10 000 resamples in every primary cell. "Rank agreement" is the mean
per-seed Spearman between the fitted score and t1, which ranks cycles exactly as the
trace does. **The calibration's answer is (3, 0, 0).** PCS should be its trace term alone:
the control, re-expressed. The residual weights are tie-breaks. Where an interval excludes
zero it is either tiny and positive for geometry (`arc`, w2/w1 ≤ 1.3e-4) or **negative for
the NLOS term** (`searcher`). There, the live NLOS input, if it says anything, says it the
wrong way round.

**The default weights in code are not changed**, as registered. (3, 0, 0) cannot be put
into the code anyway: t1 on a tier-1 fix is 1e-6 to 1e-2 (`static_868` rows of
`ANALYSIS.md` §1), so 3·t1 would never reach a threshold of 1.55. The thresholds were set against the hand-set scale and
would need their own campaign. Section 5 explains why that campaign is not worth running.

## 3. Why: neither extra term carries rank information about error

Single-term ρ with error, confidence convention, `ca_tbl_full` (a negative value means
the term ranks error the right way round):

| regime | t2 = 1/GDOP | t3 = live NLOS term |
|---|---|---|
| `default` | **+0.1493** (wrong way round) | −0.0499 |
| `arc` | −0.2121 | **+0.0552** (wrong way round) |
| `searcher` | −0.0117 | −0.0590 |

- **t3 is live and nearly uninformative.** It came from the classifier's score on
  12 077 / 12 193, 7 932 / 8 063 and 13 915 / 14 042 fix cycles (A4), so the input was
  there. But the 868 MHz score it mostly reads is a weak detector (seed 7: tp 19 against
  fn 177), and it ranks error at |ρ| ≤ 0.06, with a wrong sign in one regime. The UWB CIR
  cannot help: the channel model gives it no NLOS positives (`86e43f2`).
- **t2 is redundant where it is informative.** 1/GDOP describes the anchor geometry, which
  the EKF covariance already integrates through JᵀR⁻¹J. In `arc` it carries ρ = −0.21 on
  its own, and FIT-R still gains only +0.0045 [−0.0129, +0.0217] over the trace.

**The hand-set weights lose badly in the corrected topology.** HAND (2, 0.5, 0.5) is the
trace's own `pcs` column. It loses to the control by −0.4742, −0.2963 and −0.2116, with
95 % intervals wholly below zero. The legacy sum loses by −0.6330, −0.3029 and −0.3224.
The reason: on a tier-1 fix 2·t1 moves by hundredths at most (t1 ≤ 0.0126 on
`static_868`/`default`), while 0.5·t2 + 0.5·t3 moves by tenths. So the hand-set score ranks
tier-1 cycles by the two terms that carry least. `weighted` gave term 1 back its veto
against the threshold. It did not give term 1 its voice in the ranking. `PCS_FORMS`
measured the same form at −0.0114, on a different binary, under the legacy topology and
with the tag-based t3. That comparison is qualitative only (rule 8 across binaries). What
it shows is that calibration **recovers the hand-set loss in full, and stops exactly at the
control**, the same ceiling `PCS_FORMS` found for every repaired form.

## 4. A second finding: the committed machinery's acceptance test is not a fair control

`CaTblCalibrateLoocv` (`ca-tbl-loocv.cc`) returns **OK in all six cells**. Its three-term
OLS beats "term 1 alone" on LOOCV RMSE, for example 77.36 m against 121.13 m on
`default`/`ca_tbl_full`. **That OK is an artefact.** The control is fitted without an
intercept. On a tier-1 fix t1 is of order 1e-4, so t1 alone cannot express a mean error of
tens to hundreds of metres, while the three-term fit gets an intercept for free from
t3 ≈ 0.92 and t2 ≈ 0.98. Give both sides an intercept (seed-level LOO) and the gain disappears:

| cell | FIT-LS1 (3 terms + intercept) | t1 + intercept |
|---|---|---|
| `default` / `ca_tbl_full` | 77.47 m | 77.50 m |
| `searcher` / `ca_tbl_full` | 80.17 m | 80.18 m |
| `arc` / `ca_tbl_full` | 293.23 m | 313.30 m |
| `searcher` / `static_868` | 490.57 m | 473.97 m (the control wins) |

`arc` is the one cell where the extra terms cut squared error (6 %), and FIT-R still cannot
turn that into better *ranking*, which is what an escalation threshold uses. The machinery
would have recommended (3.403, −0.276, −0.128) on `default` and (4.301, −0.870, −0.431) on
`searcher`. Its model, fitted fold by fold (FIT-LS), ranks held-out error far worse than
the control: −0.2627 and −0.1882. The machinery's own comment names the failure: *"a control given fewer fitting
opportunities would lose for the wrong reason."* Its control is exactly that.

**Recorded, not fixed here.** The machinery is `src/` code, and a change to it is its own
commit with its own test. The repair is to fit the control with the same intercept as the
model, or to accept on rank. Until then, a `CA_TBL_CAL_OK` from it is not evidence.

## 5. What this means for the PCS claim (`TECH_DEBT` L.3, first row)

K.8 set the order: the monotone control first, then CAL-01, then live inputs for the dead
terms. As far as the code allows, all three are now done:

1. **The control:** every PCS form loses (`PCS_FORMS_RESULTS.md`).
2. **CAL-01, this campaign:** leave-one-seed-out over three regimes. Calibration reaches
   the control and no further. The calibrated score is the trace (rank agreement
   0.96–0.9997).
3. **Live inputs:** the NLOS term is live (`86e43f2`) and carries |ρ| ≤ 0.06. The hop term
   cannot be given one: the measured tag's `Step()` receives `hopCount` as the literal 0
   (`scratch/ca_tbl_causal_sim.cc:7874`). `hopWeight` stays 0 (`CLAUDE.md` §2).

**The confidence-score contribution, that PCS carries information about error beyond the
filter statistic it is computed from, is not supported by any linear calibration of its
terms, and the row stays measured negative.** L.4 anticipated this ending. The claim to
withdraw is the *novelty* one. Escalating on a monotone function of the covariance trace
is still a working mechanism; `fixed_threshold` is that mechanism and needs no PCS. Whether
to withdraw the claim is the owner's decision. This report records the evidence and does
not take the decision.

A closed-loop campaign with fitted weights and re-derived thresholds would test the
calibrated score. That score is the trace, and `fixed_threshold` already measures the
trace. So it is not recommended.

What could still change the conclusion, none of it built:

- a nonlinear form (this campaign is linear, like PCS);
- a hop count that actually reaches the measured tag's PCS;
- an NLOS detector whose score carries information the channel model's own state does not
  hand it for free.

## 6. Predictions, scored

| # | registered | observed | held? |
|---|---|---|---|
| P1 | CALIBRATION DOES NOT RESCUE PCS | INDISTINGUISHABLE in 3 of 3 primary cells | **yes** |
| P2 | rank agreement of FIT-R with t1 ≥ 0.9 in ≥ 2 primary cells | 0.9844, 0.9628, 0.9997 | **yes** |
| P3 | HAND loses at 95 % in ≥ 2 primary cells | loses in 3: −0.4742, −0.2963, −0.2116 | **yes** |
| P4 | FIT-LS loses at 95 % in ≥ 2 primary cells | loses in 3: −0.2627, −0.2405, −0.1882 | **yes** |
| P5 | recomputed t3 disagrees on ≤ 5 % of fix cycles in every cell | 0 of 72 665 | **yes** |

## 7. Checks

- **Aborts.** A1 (provenance, binary path, flag set and outdir, flag for flag), A2
  (60 × 2 traces of exactly 720 rows, counted; one `arm_summary.csv` row per arm), A3, A7,
  A8 and A9 all passed. A4 and A6 passed in every cell: t3 fed on ≥ 98.4 % of fix
  cycles, 20 of 20 seeds evaluable.
- **Rule 3, recounted by a path that shares no code with the analysis.** `awk` on
  `hadFix` gives 12 193 / 14 311 / 8 063 / 9 758 / 14 042 / 14 298 fix cycles, equal to the
  analysis. A separate pure-Python Spearman (csv module, hand-written ranks) reproduces
  ρ_control on all six cells: −0.6863, −0.1392, −0.4346, −0.3114, −0.4684, −0.4889. It
  also reproduces FIT-R's all-seed in-sample ρ on the three primary cells (−0.6877,
  −0.4447, −0.4687), all to four decimals.
- **Rule 4: the zeros in the A7 column come from a check observed to fire.**
  Back-deriving t3 with the legacy weights on `default`/`ca_tbl_full` seed 1 fires A3 on
  393 of 606 fix cycles and A7 on 606 of 606. The sign-refit path gave 0 flips here, and
  it is reachable: the analysis self-test's upside-down fixture flipped 20 of 20 folds.
- **FIT-R's verdict is not the tie rule returning the control.** No fold chose the control
  exactly (0 of 20 in every cell; 9, 9 and 3 distinct directions across the folds). The
  fitted departures from the trace simply did not generalise to the held-out seed.
- **Determinism.** A second run of the analysis gives a byte-identical `ANALYSIS.md`
  (sha256 `5fd834b1…a06d45a`).

## 8. What this campaign cannot establish

See the registration, §13. In short:

- closed-loop behaviour at fitted weights;
- thresholds;
- anything nonlinear;
- the hop term;
- the UWB CIR;
- comparison with any campaign on another binary;
- anything beyond the simulator's own truth.

The grid's ratio step of 1.33× was not refined. Nothing in the results suggests it binds:
the fitted departures from the control were already too small to generalise.

## 9. Reproduce

```
bash scripts/cal01_run.sh batch default 1 20     # likewise arc, searcher; into a NEW root
python3 scripts/cal01_loocv.py --root results/cal01_loocv_20260928 --source c019c0a \
    > results/cal01_loocv_20260928/ANALYSIS.md
python3 scripts/cal01_loocv.py --selftest        # the analysis firing and staying silent
```

Traces and `nlos_*` files are untracked, and each is regenerable from its directory's
`run_id.txt` invocation, whose SHA-256 digest covers them.
