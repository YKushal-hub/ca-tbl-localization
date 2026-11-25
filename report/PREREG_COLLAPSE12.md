# COLLAPSE-12 — pre-registration (committed before the first cell)

**Why.** COLLAPSE-11 is incomplete by its own abort clause (a wrong announcement check on two single-worker runs). This repeats it on new seeds.

**Design.** COLLAPSE-11's cells, switches and frozen-binary rules, seeds 1801-1810 (never used), 200 runs. The only change: whether `--tofRubbleCorrect` is on is read from each run's command line
(`run_id.txt`), not from a count of corrected ranges. `scripts/collapse12_run.sh`, `scripts/collapse12_analyse.py`, committed with this file.

**Seen before registering.** All of COLLAPSE-11's printed tables (seeds 1701-1710), and the low and high brackets on development seeds 731-732: coverage, 5 / 8 storeys, without and with DD 49: low 0.665 to 0.866 and 0.894 to
0.953; high 0.618 to 0.600 and 0.740 to 0.735. So a pass says nothing about the high bracket, where the correction did not help.

## Hypotheses (fixed now, COLLAPSE-11's under new numbers; paired 95 % bootstrap over seeds)

- **H49:** every crew cell: lower end of ours' L2, on minus off, at least -0.05.
- **H50:** off and on: every worker's and every searcher's device's audited 868 MHz airtime at most 2.5 %.
- **H51:** on, four stations, 20 workers: ours minus the best single radio, lower end at least -0.05, at both storey counts. (COLLAPSE-9, -10 failed this and COLLAPSE-11 printed a fail; it may fail again.)
- **H52:** coverage on minus off: lower end at least -0.02 in every crew cell, above 0 at 5 storeys with four and with two stations.
- **H53:** on, four stations, 20 workers: mean coverage at least 0.85 at both storey counts. (0.853 at 8 storeys in COLLAPSE-11: close to the limit, so it may fail.)

**Abort / completeness.** As COLLAPSE-11 with the corrected check. No cell dropped. Nothing is adopted as a default by this campaign. A Result section is appended after the run.

## Result (run 2026-11-25 23:50 to 2026-11-26 02:52 IST; source 9b668bc46; 200/200 cells rc 0, dirty 0, problems 0)

Full tables: `results/collapse12_2026-11-26/COLLAPSE12.md`. "Ours" = `ca_tbl_full`; `on` adds `--tofRubbleCorrect` (DD 49). Intervals are paired 95 % bootstrap over seeds.

- **H49: FAILS.** Ours' L2, on minus off: 8 storeys two stations -0.015 [-0.075, 0.055] and crew of 10 -0.030 [-0.100, 0.030]: no loss is detected, non-inferiority by 0.05 is not shown. Gains at 5 storeys: crew of 20
  +0.040 [0.005, 0.085], crew of 10 +0.070 [0.010, 0.120], two stations +0.150 [0.100, 0.195]; 8 storeys field-measured error +0.050 [0.005, 0.105].
- **H50: HOLDS.** No worker's or searcher's device above 2.5 %; largest 2.488 %.
- **H51: FAILS.** Ours minus 868 MHz alone, L2: 5 storeys -0.030 [-0.080, 0.020], 8 storeys +0.005 [-0.040, 0.050]. At 8 storeys the interval lies within 0.05; at 5 storeys it does not.
- **H52: FAILS.** Coverage on minus off: above 0 at 5 storeys with four (+0.084 [0.058, 0.114]) and two stations (+0.117 [0.075, 0.157]) as predicted, but the lower end is below -0.02 at 8 storeys with two stations
  (+0.019 [-0.029, 0.072]) and a crew of 10 (-0.010 [-0.060, 0.036]).
- **H53: HOLDS.** Four stations, 20 workers, coverage 0.874 (5 storeys) and 0.858 (8); 0.95 is not reached.
- **Coverage (reported), on:** 0.575 to 0.876 in crew cells; two stations 0.696 and 0.575.
- The correction helps clearly at 5 storeys and not detectably at 8. Central bracket only; development runs showed no help in the high bracket.
