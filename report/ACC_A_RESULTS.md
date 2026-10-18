# ACC-A — results: correcting the IMU covariance improves accuracy across arms and makes the filter less overconfident; PCS still adds nothing over P; forced escalation is redundant in the preset; the CA-TBL rule buys endgame availability with energy, not accuracy

**Registered in `report/PREREG_ACC_A_FILTER.md` (`681e90c8`) before any cell ran.** Grade:
**exploratory**. All 240 cells exit 0 and read `source 681e90c8 dirty 0`.

The registered analysis was applied with one disclosed correction (`4d219963`). Its abort check
was changed to test the condition the registration names: the flag reached the trace. A
`lora_fusion` seed with no fix had tripped it. Tables and verdicts are identical before and after,
and the run reports "Problems: 0". Tables: `results/acc_a_2026-10-09/ACC_A.md`. Provenance:
`RUN.md`. Paired over seeds 1-20, as mean [95 % bootstrap].

## 1. Covariance effect (k = 10 − k = 1)

| | 5 km (A) | endgame (B) |
|---|---|---|
| `ca_tbl_full` tag 0 CEP50 / CEP95 per seed (m) | 24.4 / 57.8 → 16.6 / 48.3; Δ **−7.9 [−13.9, −2.9]** / −9.5 [−22.2, 1.7] | 5.7 / 12.0 → 2.9 / 7.5; Δ **−2.8 [−3.8, −2.0]** / **−4.5 [−6.5, −2.7]** |
| pooled (S2's convention) | 16.2 / 85.0 → 11.7 / 61.7 | 4.8 / 16.1 → 2.7 / 8.0 |
| ANEES median (consistent ≈ 2) | 105 → 15.6 | 45.6 → 5.7 |
| availability | −0.007 [−0.020, 0.004] | **+0.013 [0.003, 0.023]** |
| J per node | +0.60 [−0.12, 1.39] | **+3.04 [0.90, 5.15]** |

- **Effect on other arms.** It is a filter-wide fix, and every arm's endgame CEP95 improves
  except LoRa fusion's (Δ −1.1 n.s.):
  - `fixed_threshold` −4.6;
  - `static_2p4` −3.6;
  - `static_868` −10.3;
  - Poucet −4.5.

  At 5 km the effect splits. `ca_tbl_full`, `ca_tbl_no_integrity` and `static_2p4` improve
  CEP50. `fixed_threshold`, LoRa fusion and Poucet worsen their pooled CEP95 tail by more
  than 10 %; `fixed_threshold` goes 72.9 → 100.4 m (Q7 failed).
- **Dose-response (`ca_tbl_full`, per-seed CEP95, median ANEES):**
  - k = 1: 57.8 m and 12.0 m; ANEES 105 (A) and 46 (B).
  - k = 3: 52.2 m and 9.0 m; ANEES 28 and 15.
  - k = 10: 48.3 m and 7.5 m; ANEES 15.6 and 5.7.
  - k = 30: 48.4 m and 6.5 m; ANEES 3.3 and 2.3.

  So consistency needs k ≈ 30. Accuracy saturates at 10 in A and still improves to 30 in B.
  The intervention is a stand-in. The ANEES trend says the true missing variance is about 30×,
  inside S2's measured 3-54×.

## 2. Consumers of P (`ca_tbl_full`, tag 0)

| consumer | k = 1 → 10, A | k = 1 → 10, B | verdict |
|---|---|---|---|
| NIS > 9.21 share (consistent ≈ 0.01) | 0.458 → 0.423 (n.s.) | 0.285 → 0.255 (n.s.) | inconsistent at both k (Q4 failed) |
| TDoA innovation-gate refusal rate | 0.160 → 0.231 (**+0.107 [0.028, 0.206]**) | 0.026 → 0.009 (n.s.) | refuses more at 5 km, not less |
| SUSPECT share (fault-free) | 0.0189 → 0.0147 | 0.0044 → 0.0007 (**−0.0037**) | already rare; rarer |
| PCS − trace informativeness | −0.082 n.s. → **−0.165 [−0.258, −0.093]** | −0.004 → −0.002 | **PCS never beats the trace it is computed from (Q8 held: stop rule met)** |
| escalations per run | 34.5 → 19.2 (**−15.3 [−21.2, −9.5]**) | 52.8 → 56.8 (n.s.) | fewer at 5 km; UWB fixes +2.0 [0.1, 4.1] in B |

**PCS stop rule (Part A §11).** With the corrected covariance, PCS still predicts error no better
than P, and at 5 km significantly worse. No further PCS form is justified.

## 3. Integrity trigger

- **Forced escalation is redundant in the preset.** `--suspectForcesEscalation=true` gives
  outputs byte-identical to the preset on all 80 seed-cells (E against M, every trace). The
  reason is a control-flow one. The preset downweights a SUSPECT fix's PCS by 0.5
  (`suspectPcsFactor`), and on all 350 SUSPECT rows that already puts PCS below the tier's up
  threshold. So the fix escalates without the flag. Q10 held, trivially.
- **The integrity check itself (`ca_tbl_no_integrity` − preset):**
  - no effect at k = 1 on any metric;
  - at k = 10, availability +0.013 [0.004, 0.023] at 5 km (the check costs a little) and 3.0
    fewer escalations in B.
  - At χ² 1992.9 it fires on 1-8 fixes per run, so its causal effect is small either way.
    Q11 failed on the A k = 10 availability.

## 4. The decision rule against `fixed_threshold` (same preset fixes, same k)

| | A, k = 1 | A, k = 10 | B, k = 1 | B, k = 10 |
|---|---|---|---|---|
| availability | **+0.011** | −0.001 | **+0.017** | **+0.031 [0.021, 0.041]** |
| CEP50 / CEP95 | tie / tie | −9.0 / −19.3 (both n.s.) | tie / tie | tie / tie |
| J per node | −0.25 | +0.56 | +0.95 | **+6.11 [4.05, 8.23]** |

The CA-TBL rule buys endgame availability, more of it at the corrected covariance, and pays for it
in energy: +6.1 J per node at k = 10, mostly UWB. On accuracy it never separates from the
fixed threshold. Q12 failed: at 5 km the k = 10 availability is a tie.

## 5. Scoreboard at k = 10 (`ca_tbl_full` − each arm)

- **WIN on every metric** (availability, within 25 m, CEP50, CEP95), A and B, against
  `static_868`, LoRa fusion and Poucet.
- **Against `static_2p4`:**
  - availability WIN (+0.25 to +0.31);
  - five-node within 25 m LOSS at 5 km (−0.067 [−0.128, −0.015]), the subjects' loss (N.35);
  - endgame accuracy tie. At k = 1 the endgame CEP50 was a 0.9 m LOSS; k = 10 closes it.
- **Against `fixed_threshold`:** §4.

## 6. Energy decomposition (tag 0, endgame, J per run at k = 1 → 10)

| arm | 868 / 2.4 / UWB | J per cycle within 25 m |
|---|---|---|
| `ca_tbl_full` | 7.92 / 5.28 / 17.48 → 6.75 / 5.52 / 19.35 | 0.043 → 0.044 |
| `fixed_threshold` | 7.92 / 5.25 / 15.55 → 6.88 / 5.23 / 16.45 | 0.040 → 0.040 |
| `static_2p4` | 0 / 9.95 / 0 → 0 / 9.65 / 0 | 0.014 → 0.013 |

UWB dominates the hybrids' endgame energy. The TDoA overhead is inside the 868 MHz figure and is
not separable.

## 7. Registered predictions

| # | verdict |
|---|---|
| Q1 replication (pooled CEP at k = 10 within S2's thresholds) | **held** (11.7 / 61.7; 2.7 / 8.0) |
| Q2 per-seed CEP95 lower at k = 10, A and B | **failed**: A −9.5 [−22.2, 1.7]; B held |
| Q3 ANEES > 4 at k = 1 and lower at k = 10 | **held** |
| Q4 NIS > 9.21 share lower at k = 10 | **failed** (both n.s.) |
| Q5 the gate still discriminates at k = 10 | **held** |
| Q6 no arm's availability moves > 0.01 | **failed** (`ca_tbl_full` B +0.013; `ca_tbl_no_integrity` B +0.020) |
| Q7 no arm's pooled CEP95 worsens > 10 % | **failed** (5 km: `fixed_threshold`, LoRa fusion, Poucet) |
| Q8 PCS not better than the trace at k = 10 | **held** |
| Q9 SUSPECT ≤ 0.02 at k = 1, not higher at k = 10 | **held** |
| Q10 forced escalation changes nothing at k = 1 | **held** (byte-identical; redundant, §3) |
| Q11 integrity off − preset contains 0 | **failed** (A k = 10 availability +0.013) |
| Q12 k = 10: availability WIN and CEP95 tie against `fixed_threshold` | **failed** (A availability tie) |
| Q13 more escalations at k = 10 | **failed** (A −15.3; B n.s.) |

## 8. Attribution (Part A §17)

| cause | what ACC-A (with TOF-FIELD, ACC-B) shows |
|---|---|
| **covariance consistency** | the largest accuracy lever: endgame CEP95 −4.5 m, filter-wide, not CA-TBL-specific. It also brings the 5 km tail of the threshold and fusion arms into question |
| **measurement / fusion fixes** | the source of CA-TBL's wins over the fusion comparators. They are in the preset for both switching arms, and they carry robustness to field ToF error (TOF-FIELD) |
| **CA-TBL decision policy** | endgame availability +0.017 to +0.031 over `fixed_threshold`; no accuracy; costs energy (+6.1 J at k = 10) |
| **integrity trigger** | forced escalation is redundant (control flow); the check itself has a small effect at χ² 1992.9 |
| **energy-aware escalation** | not built (Part A §10). The +6.1 J is what it would target |
| **propagation assumptions** | outside CA-TBL. Tier-2 accuracy is conditional on the 3 m floor (N.40), and the foliage clamp is N.41 |

## 9. What this cannot establish

- **One machine, exploratory grade, k not tuned** (no seeds 21-30). The local machine's
  `LOCAL_RUN_2026_10_09` step 5 is an independent tuning sweep.
- **`--imuQuScale` is a stand-in.** An error-state filter (S2's A2) is the principled fix and is
  not built.
- **PCS thresholds (CAL-03) were fitted on the uncorrected covariance.** At k = 10 the rule's
  escalations shift for that reason too. A refit is its own registered step.
