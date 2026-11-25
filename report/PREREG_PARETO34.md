# PARETO-3 and PARETO-4 — pre-registration (committed before the first cell)

**Why.** The decision-rule section rests on PARETO-1/2, run with the two-range and peer-fusion defects and over the 2.5 % 868 MHz limit. REPLAY-DD54-PARETO
was stopped unanalysed for the second reason. This repeats both with DD 54, 56 and 57 on.

**Design.** PARETO-3 = PARETO-1's cells and rules, PARETO-4 = PARETO-2's, both with the DD 41 replay's deployment pin (`PIN_PRE_DD27`), plus
`--sparseRangeGeometry --peerSplitCi --sharedDuty868 --dutyReport868` on every cell. New seeds 1301-1320 and 1321-1340. 560 + 400 = 960 runs.
Runners `scripts/pareto{3,4}_run.sh`, analysers `scripts/pareto{3,4}_analyse.py` (copies; the only changes are seeds, names, the L1 and switch checks and, in
PARETO-4, printing P2 and P3), committed with this file. Binary frozen at launch, SHA-256 before every cell, `dirty 0`.

**Seen before registering.** PARETO-1/2 and their replays (both P1: NO-GO on record); REALISTIC-8/9 and COLLAPSE-9. Development seed 721: three cells
ran through (largest audited peak 2.09 %); a fourth was cut off by a time limit. Accuracy and energy were not looked at.

## Predictions (fixed now)

- **PARETO-3 P1-P4** and **PARETO-4 P1-P3:** `PREREG_PARETO1.md` and `PREREG_PARETO2.md`, unchanged, each by its own rule. Each verdict is reported beside the
  earlier one.
- **L1 (both):** every worker device of every design in every cell: audited trailing-hour 868 MHz peak at most 2.5 %. The run is 1800 s, so the hour's 90 s budget
  applies to the whole run and binds less than in a full hour.

**Not covered.** Searchers' devices, gateways, 2.4 GHz and UWB airtime, runs longer than 1800 s.

**Abort / completeness.** As PARETO-1/2, plus: a missing switch announcement or device line. No cell dropped or chosen. No default is adopted here.
A Result section is appended whatever it says.

## Result (run 2026-11-25, 01:42–07:22 IST, source 7e63e296b, 560/560 and 400/400 cells rc 0, dirty 0, problems 0)

Full tables: `results/pareto3_2026-11-25/PARETO3.md`, `results/pareto4_2026-11-25/PARETO4.md`. Traces are not tracked. Each verdict is by its own registered rule.

| prediction | earlier (PARETO-1/2) | this campaign |
|---|---|---|
| PARETO-3 P1 | NO-GO | **NO-GO** |
| PARETO-3 P2 | holds | **HOLDS** |
| PARETO-3 P3 | holds | **HOLDS** |
| PARETO-3 P4 (falsifier) | not triggered | **not triggered** |
| PARETO-4 P1 | NO-GO | **NO-GO** |
| PARETO-4 P2 | holds (+0.024 [0.007, 0.046]) | **FAILS** |
| PARETO-4 P3 | holds | **HOLDS** |
| L1 (both) | not tested | **HOLDS** |

- **PARETO-3 P1, NO-GO.** Every BEN setting is feasible (five-worker A(25 m) 0.987–0.988 at 5 km), but so is 2.4 GHz only (0.984 at 7.53 J), which is E*.
  BEN costs 1.46–1.57 times E* (BEN10 +3.49 [3.21, 3.77] J) and gains 0.003–0.004 of A(25 m), so condition (ii) fails for every λ.
- **PARETO-3 P2.** Every BEN λ is below FT12 in A (BEN10 −3.14 [−3.42, −2.86] J, 14.16 → 11.02 J) at −0.001 to −0.002 A(25 m).
- **PARETO-3 P3.** Subject 868 MHz energy, five-worker mean, BNS100 against BEN100: 5.67 vs 2.64 J (A), 7.16 vs 4.15 J (B). Without the skip the trigger
  does not differ from FT12 in A (BNS100 −0.19 [−0.42, 0.04] J).
- **PARETO-3 P4.** Every one of the 240 BEN cells reports at least 3177 skipped 868 MHz slots (count over the TII-1 log line).
- **PARETO-4 P1, NO-GO.** E* is 2.4 GHz only (7.62 J, 0.984); TII-2 costs 1.63–1.73 times E* (BEN10 +4.80 [4.64, 4.97] J).
- **PARETO-4 P2 fails.** BEN1000 (TII-2) minus P1BEN1000: 0.001 [−0.000, 0.001]. PARETO-1's form no longer loses accuracy here, so the three-range
  condition has nothing to repair; P1BEN1000 also spends less (11.06 against 12.56 J).
- **PARETO-4 P3.** BEN1000 − FT12, subject J: −1.71 [−2.17, −1.30].
- **L1.** No worker device above 2.5 %. Largest audited peak 2.315 % in both campaigns.
- **Against the flat threshold (unregistered contrast).** `ca_tbl_full` minus FT12, subject J in A: +0.03 [−0.24, 0.32] (PARETO-3), −0.28 [−0.67, 0.05]
  (PARETO-4); A(25 m) differs by at most 0.001 in both. Our trigger is not separable from the flat threshold.
- **Not comparable across campaigns.** Earlier levels (static 2.4 GHz 0.943/0.954) differ from 0.984 here; seeds, the deployment pin and the fixes all changed,
  so the change is not attributed to any one of them.
