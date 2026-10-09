# ACC-A decision diagnosis — why `ca_tbl_full` does not separate from `fixed_threshold`

**Post-hoc and exploratory, not registered.** Written 2026-10-09 from ACC-A's own traces
(`681e90c8` build; M cells, tag 0, seeds 1-20, 720 cycles per run). No cell was run for this.
Script: `scripts/acc_a_decision_diag.py <ACC-A outdir>`. It reads only and prints every number
below.

ACC-A §4 found that, on the same measurements, the CA-TBL rule gains endgame availability
(+0.031 at k = 10) for +6.1 J per node and never separates from `fixed_threshold` on accuracy. This
note finds the mechanism in the code and the traces. The test it implies is registered separately in
`report/PREREG_ACC_A_OPPOINT.md`.

**Definitions:**
- A **decision row** is one where `Step()` reached the decision: `hadFix` 1 and `nAnchors` ≥ 3.
- Two-range rows and coasts take the dropout path. That path computes no PCS and logs it as 0.
- An escalation on any other row, or on a decision row whose own rule condition is false, came
  from the **NW-4 URGENT-query hook** (`ca-tbl-policy.cc:1802`, `:1310`). The only other writer of
  `escalated` in `Step()` is `suspectForcesEscalation`, which is off in the preset.
- A UWB grant that lands later changes the tier without setting `escalated`, and is not counted.

## 1. In the preset, PCS is a function of trace(P) alone

`spec_profile.txt` sets the PCS from CAL-03's fit:
- W1 2.994675 on term 1 = traceRef / (traceRef + trace P), with traceRef 10;
- **W2 = 0** on geometry (GDOP);
- W3 0.005325 on line-of-sight.

So PCS = 2.9947 · 10 / (10 + trace P) + [0, 0.0053]. That holds on **every non-SUSPECT decision
row**: 8 173/8 173, 8 025/8 025, 14 055/14 056 and 14 146/14 146 in the four M cells.

The PCS thresholds are therefore trace thresholds:

| | `ca_tbl_full` (PCS thresholds as trace P, m²) | `fixed_threshold` |
|---|---|---|
| escalate from 868 MHz | trace > 8.2–8.3 | trace > 100 |
| escalate from 2.4 GHz | trace > 11.8 | trace > 100 |
| healthy (counts toward demotion) at 2.4 GHz | trace < 9.4–9.5 | trace < 50 |
| healthy at UWB | trace < 5.9–6.0 | trace < 50 |

**`ca_tbl_full` is a variance-threshold rule.** It has per-tier thresholds about an order of
magnitude lower than `fixed_threshold`'s 100/50 m², and the integrity check, at χ² 1992.9, fires on
1-8 fixes per run (ACC-A §3). It has no input that `fixed_threshold` lacks, so the two rules can
differ only in operating point. This is why ACC-A §2 found PCS never more informative than the
trace: in this preset it *is* the trace. CAL-03's fit put zero weight on geometry and 0.18 % on
line-of-sight.

## 2. Most tier changes are not made by either rule

**Endgame (B), k = 10, tag 0:**

| | `ca_tbl_full` | `fixed_threshold` |
|---|---|---|
| escalation decisions per run | 56.9 | 54.5 |
| … by the rule / by the URGENT-query hook | 14.6 % / **85.4 %** | 0.5 % / **99.5 %** |
| demotions per run | 56.0 | 55.6 |
| … healthy rule / dropout path | 97.2 % / 2.8 % | 99.5 % / 0.5 % |
| healthy at UWB (of decision rows) | 81.1 % | 100 % |
| UWB cycles / UWB-solved fixes per run | 41.5 / 7.55 | 33.0 / 4.60 |

In the endgame, both arms are mostly the same machine:
- the URGENT-query hook, which both hybrid arms share, pushes the node up;
- the rule finds it healthy and demotes it after `nDeescalate` = 5 cycles.

The difference is **the 14.6 % of escalations `ca_tbl_full` makes itself**: 8.3 per run against
0.3. Its stricter healthy test (81 % against 100 % of UWB rows) adds to them. Together they give
+26 % UWB cycles and +64 % UWB-solved fixes, and that is the endgame availability gain and its
energy cost.

At k = 1 the hook makes 97.1 % and 99.5 % of the escalations. That is why the k = 1 differences
are smaller.

**5 km (A), k = 10:**
- `ca_tbl_full`'s rule wants UWB on 85.6 % of its 2.4 GHz decision rows; `fixed_threshold`'s on
  30.0 %.
- UWB occupancy stays at 0.6 % and 0.1 %, with no UWB-solved fix in either arm. No finder is in
  UWB reach, so the shared preconditions decide.
- These rows do not separate a blocked escalation from one waiting on a pending UWB grant.

## 3. The 868 MHz ↔ 2.4 GHz decision barely changes what is measured

With the dual base (FIXES-N26 fix 1), a node "at 868 MHz" also ranges on 2.4 GHz. The share of
fixes solved on 2.4 GHz ranges is nearly the same at either tier:

| fixes solved on 2.4 GHz ranges, at 868 / at 2.4 | `ca_tbl_full` | `fixed_threshold` |
|---|---|---|
| A, k = 10 | 73.3 % / 69.3 % | 72.4 % / 67.1 % |
| B, k = 10 | 96.8 % / 96.6 % | 96.9 % / 96.0 % |

This is the decision on which the two rules differ most at 5 km. At k = 10, `ca_tbl_full` spends
91.2 % of cycles at 2.4 GHz and `fixed_threshold` 64.7 %. The reason is the healthy threshold at
2.4 GHz: 9.4 against 50 m², met on 7.4 % against 45.2 % of rows.

The difference changes which radio is *active*, not which ranges reach the filter. That is
consistent with ACC-A §4's 5 km CEP differences (−9.0 / −19.3 m), which are not significant.

## 4. Reading

None of these reasons is about tuning:
1. **The score carries no information the variance lacks.** CAL-03's own fit removed geometry,
   and the integrity check is near-silent at χ² 1992.9.
2. **The decisions that move UWB are made by shared mechanisms.** In the endgame it is the
   URGENT-query hook (85-99.5 % of escalations); at 5 km it is the preconditions, because no
   finder is in UWB reach.
3. **The decision the rules disagree on most (868 ↔ 2.4) is neutralised by the dual base.**
4. **What is left is an operating point.** `ca_tbl_full`'s lower thresholds buy a few more UWB
   visits: availability for energy, and no accuracy. The endgame is near the 3 m floor (ACC-A §1,
   TOF-FIELD), so there is little accuracy left to buy.

## 5. What this cannot establish

- **Post-hoc, one machine, tag 0 only.** Subjects run the same policy code, but this note does not
  count their decisions.
- **"By the hook" is inferred** from the rule condition being false on the row. A per-decision
  cause column does not exist in the trace.
- **Unmet "wants" are not split** into a precondition refusal and a pending UWB grant.
- **Reading 4 is a hypothesis until tested.** If `ca_tbl_full`'s gain is only an operating point,
  `fixed_threshold` with its single threshold lowered to the same region should reproduce it.
  That is `PREREG_ACC_A_OPPOINT.md`.
