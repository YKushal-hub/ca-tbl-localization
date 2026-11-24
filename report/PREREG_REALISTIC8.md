# REALISTIC-8 — pre-registration (committed before the first cell)

**Why.** REALISTIC-7 fixed two-range fixes. Two problems remained. (1) A device's true 868 MHz airtime, ranging plus every sent MAC frame, was
3.6-4.8 % on development runs against the legal 2.5 %, because two governors each allowed 2.5 %. (2) The crew's 95 % region still covered too
little at the office (0.77), because peer ranges were fused as independent. DD 56 (`--sharedDuty868`) and DD 57 (`--peerSplitCi`) address them.

**Binary.** Frozen at launch by `scripts/realistic8_run.sh` (SHA-256 checked before every cell); source = the commit adding this file;
`dirty 0` on every cell.

**Design.** Seeds 651-670 (emergency) and 671-680 (8 h quiet shift), never used. REALISTIC-6's cells, every design with DD 54, DD 56 (ranging share 0.5)
and DD 57 on. 400 runs. Measures and statistics: `scripts/realistic8_analyse.py`, committed with this file.

**Seen before registering (development seeds 721-724, four seeds, not evidence).** Ours with all three on: no device above 2.5 %; coverage 0.88-0.92; crew A(25 m)
0.34 on the three-gateway road (0.39 without the cap), 0.40 at random gateways (0.59), 0.70 at the office (0.77); searcher arrival 2845 s on the three-gateway
road against 2216 s for 2.4 GHz only.

## Predictions (fixed now)

- **H1-H3.** REALISTIC-6's rules, every design with the three mechanisms on. On the evidence above H1 and H2 may fail.
- **L1.** Every worker device of ours, in every main, crew and field-error cell: mean and trailing-hour peak 868 MHz duty cycle at most 2.5 %.
- **C1.** Every main cell: ours, mean crew coverage at least 0.85.
- **T1, Q1 (reported, no prediction).**

**Not covered.** Searchers' devices, gateways and 2.4 GHz and UWB airtime.

**Abort / completeness.** Any rc not 0, not `dirty 0`, a manifest mismatch, a missing DD54/56/57 line, an alarm not at 3000 s, or fewer than the registered
seeds in a cell: incomplete, nothing is evaluated. No cell dropped or chosen. Nothing is adopted as a default by this campaign.

**Reporting.** `REALISTIC8.md` by the analyser; a Result section is appended here after the run, whatever it says.
