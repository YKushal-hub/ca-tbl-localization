# PARETO-2 — pre-registration (committed before the first cell)

**Why.** PARETO-1 (report/PREREG_PARETO1.md) was NO-GO and stays so: no predicted-benefit setting reached 0.95
five-worker A(25 m) at 5 km (best 0.942), although every setting was 28 % cheaper than the flat threshold. Its tables
point to one cause, the 868 MHz slot skipped on cycles where 2.4 GHz alone could not fix. PARETO-2 tests a NEW rule,
TII-2 (`--benefitSkip868Min3`, commit 573a438a): the slot may be skipped only when the active tier delivered >= 3
ranges and the predicted benefit says skip. Its outcome stands either way; it does not revise PARETO-1.

**Binary.** Frozen at launch by `scripts/pareto2_run.sh` (SHA-256 checked before every cell); source = the commit
adding this file; `dirty 0` on every cell.

**Design.** Preset, 12 gateways, 12 finders, 1800 s, seeds **421–440** (never used). A = 5 km; B = endgame.

| cell | arms / flags |
|---|---|
| BASE | ca_tbl_full, fixed_threshold (12 m²), static_868, static_2p4, lora_fusion, poucet_fusion |
| FT30, FT100 | fixed_threshold, `--fixedVarThreshold` 30, 100 |
| D868 | fixed_threshold `--dualBase868OnNeed=true` |
| BEN1 … BEN10000 | fixed_threshold `--benefitLambda` 1, 10, 100, 1000, 10000 `--benefitSkip868=true --benefitSkip868Min3=true` |
| P1BEN1000 | fixed_threshold `--benefitLambda=1000 --benefitSkip868=true` (PARETO-1's best form, replicated) |

400 runs. Analyser `scripts/pareto2_analyse.py` (PARETO-1's, same metrics and statistics). FT3, FT300 and OPP are
dropped: in PARETO-1 none was the cheapest feasible rule, and OPP cost more than every threshold rule.

## Predictions

- **P1 (decision, PARETO-1's rule unchanged).** S = {ca_tbl_full, FT12, static_2p4, lora_fusion, FT30, FT100, D868}.
  FEASIBLE = mean five-worker A(25 m) ≥ 0.95 in A and B; E* = the feasible simple rule with the lowest mean subject
  energy in A. **GO** if some BEN λ (TII-2) is feasible and, against E* in A, its paired subject-energy difference has
  an upper bound below 0 with mean ≤ 0.80 × E*'s, and its paired five-worker A(25 m) lower bound is ≥ −0.02 in A and B.
  If no simple rule is feasible, GO needs a feasible BEN λ. Otherwise NO-GO.
- **P2.** BEN1000 (TII-2) has higher five-worker A(25 m) than P1BEN1000 in A (paired interval above 0).
- **P3.** BEN1000 (TII-2) spends less subject energy than FT12 in A (paired interval below 0).

**Abort / completeness** as PARETO-1: any rc ≠ 0, not `dirty 0`, manifest mismatch or < 20 seeds in a cell → P1 is
not evaluated until complete; no cell dropped or chosen.

## Result (run 2026-11-20 10:16–12:22 IST, source a74404c5, 400/400 cells rc 0, dirty 0)

Full tables: `results/pareto2_2026-11-20/PARETO2.md`. Traces are not tracked; their SHA-256 manifest is.

- **P1: NO-GO.** Every TII-2 setting is feasible (five-worker A(25 m) 0.966–0.968 at 5 km, 0.993 in the endgame), but
  on these seeds static_2p4 is feasible too (0.954 at 5 km, 0.994 in the endgame) and becomes E* at 6.14 J; TII-2 spends
  1.96–2.10× that (BEN1000 +5.97 [5.73, 6.24] J), so condition (ii) fails for every λ.
- **P2: HOLDS.** BEN1000 (TII-2) minus P1BEN1000 (PARETO-1's form), five-worker A(25 m) at 5 km:
  +0.024 [0.007, 0.046] (0.966 vs 0.941). The three-range condition removes PARETO-1's accuracy loss.
- **P3: HOLDS.** BEN1000 minus FT12, subject energy at 5 km: −1.16 [−1.47, −0.85] J (12.11 vs 13.27, −9 %), five-worker
  A(25 m) −0.000 [−0.011, 0.014]. In the endgame BEN100–BEN10000 spend 19.6–19.7 J against FT12's 26.1 J.

Also observed (not registered): static_2p4 was 0.943 at 5 km on PARETO-1's seeds and 0.954 here, so its feasibility
under the "mean ≥ 0.95" rule turns on the seed set; P1BEN1000 replicated PARETO-1 (0.941 vs 0.942).
