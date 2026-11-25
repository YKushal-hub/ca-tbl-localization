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
