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
