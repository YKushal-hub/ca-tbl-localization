# PARETO-1 — pre-registration (committed before the first cell)

**Question.** Does the predicted-benefit hand-off rule (TII-1, `--benefitLambda`, commit 7f3fbd6b) give a better
accuracy-energy trade-off than every simple rule, on the evaluated deployment? This is the go/no-go gate of the TII
plan: GO keeps IEEE TII as the target; NO-GO retargets to an evaluation paper.

**Binary.** Frozen at launch (`scripts/pareto1_run.sh` copies binary + libraries and checks their SHA-256 before every
cell). Source: the commit that adds this file; `dirty 0` required on every cell.

**Design.** Preset (`--specProfile=true`), 12 gateways, 12 finders, 1800 s, seeds **401–420** (never used before).
A = 5 km; B = endgame (`--boxHalf=750 --finderMode=1`). Cells (`<G>_<cell>/s<seed>`):

| cell | arms / flags | role |
|---|---|---|
| BASE | ca_tbl_full, fixed_threshold (12 m²), static_868, static_2p4, lora_fusion, poucet_fusion | the existing strategies |
| FT3, FT30, FT100, FT300 | fixed_threshold, `--fixedVarThreshold` = 3, 30, 100, 300 | flat-threshold family |
| OPP | ca_tbl_full `--c2bEscalation=true` | opportunistic: escalate whenever the next tier is in reach |
| D868 | fixed_threshold `--dualBase868OnNeed=true` | simple 868 MHz skip: skip when 2.4 GHz gave ≥ 3 ranges |
| BEN0.01 … BEN1000 | fixed_threshold `--benefitLambda` = 0.01, 0.1, 1, 10, 100, 1000 `--benefitSkip868=true` | the rule |
| BNS100 | fixed_threshold `--benefitLambda=100` | ablation: the rule without the 868 MHz skip |

560 runs. Metrics (scripts/pareto1_analyse.py, committed with this file): five-worker and lost-worker A(25 m) from the
traces (goal_report.py), lost-worker CEP95, mean subject energy (subject_summary.csv), time to the lost worker's first
cycle within 25 m, time to first fix. Paired 95 % bootstrap intervals (10 000 resamples), seed-paired.

**Development before registration (not citable).** Seed 7, 600 s, endgame and 5 km, λ ∈ {1, 30, 1000} then
{1, 100, 10000}: the first form of the rule demoted 2.4 → 868 MHz and re-escalated to UWB (more UWB energy as λ
rose); the registered form moves to 2.4 GHz whenever it is in reach and prices only UWB and the 868 MHz slot. On
seed 7 it cut subject 868 MHz energy ~65 %, and static_2p4 remained the cheapest arm. The grid was widened downward
because the 868 MHz skip saturates by λ = 1.

## Predictions and decision rule

- **P1 (decision).** S = simple rules {ca_tbl_full, FT12, static_2p4, lora_fusion, FT3–FT300, OPP, D868}. FEASIBLE =
  mean five-worker A(25 m) ≥ 0.95 in A and in B. E* = the feasible simple rule with the lowest mean subject energy in
  A. **GO** if some BEN λ is feasible and, against E* in A, (i) its paired subject-energy difference has an upper 95 %
  bound below 0, (ii) its mean subject energy is ≤ 0.80 × E*'s, and (iii) its paired five-worker A(25 m) difference
  has a lower bound ≥ −0.02 in A and in B. If no simple rule is feasible, GO needs a feasible BEN λ. Otherwise NO-GO.
- **P2.** Some BEN λ has lower mean subject energy than FT12 in A with the paired interval below 0.
- **P3.** BNS100 spends more 868 MHz energy than BEN100 (the skip, not the trigger, carries the 868 MHz saving).
- **P4 (falsifier of the mechanism).** If no BEN cell skips any 868 MHz slot (the TII-1 line reports 0 skipped), the
  mechanism did not run and the campaign is void.

**Abort conditions.** Any cell with rc ≠ 0, a `run_id.txt` not `dirty 0`, a binary-manifest mismatch, or fewer than
20 seeds in any cell → the campaign is incomplete and P1 is not evaluated until it is complete. No cell is dropped or
chosen; an interrupted run resumes the same frozen binary.

**Reporting.** `PARETO1.md` written by the analyser; result section appended here after the run, whatever it says.
