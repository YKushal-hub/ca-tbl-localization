# COLLAPSE-9 — pre-registration (committed before the first cell)

**Why.** The collapse results (COLLAPSE-6) predate DD 54 (two-range fixes), DD 56 (one 868 MHz ledger per device) and DD 57 (peer
fusion by split covariance intersection). They were not checked for the defects those fix.

**Design.** COLLAPSE-6's pipe cells without `rubble` and `st0`, each with the three mechanisms off and on (same binary, paired).
Seeds 1101-1110, never used; 5 and 8 storeys, central bracket, 1800 s; crews of 20 (four and two stations, field-measured 2.4 GHz
error), 10 and 1. 200 runs. Runner `scripts/collapse9_run.sh`, analyser `scripts/collapse9_analyse.py`, committed with this file.

## Hypotheses (fixed now; paired 95 % bootstrap over seeds)

- **H37:** every crew cell: the lower end of ours' L2, on minus off, is at least -0.05.
- **H38:** on: every worker device's audited trailing-hour 868 MHz airtime (ranging, MAC frames and carried bytes; the run is 1800 s,
  so the 90 s budget of the hour applies to the whole run) is at most 2.5 %.
- **H39:** on, four stations, 20 workers: ours minus the best single radio, lower end at least -0.05, at both storey counts.
- Coverage (errM <= 1.73 sqrt(traceSigma)), off and on: reported, no prediction. N = 1: reported, no prediction.

**Abort / completeness.** Any rc not 0, not `dirty 0`, a manifest mismatch or announcement lines that do not match the cell:
incomplete, nothing is evaluated. No cell dropped. Nothing is adopted as a default by this campaign.

**Not covered.** Searchers' devices; runs longer than 1800 s, where the hourly budget binds harder.

## Result (run 2026-11-24 18:55-21:07 IST; source cd4b69b48; 200/200 cells rc 0, dirty 0, problems 0)

Full tables: `results/collapse9_2026-11-24/COLLAPSE9.md`. "Ours" = `ca_tbl_full`.

- **H37: FAILS.** Cells the registered rule fails (ours, on minus off, lower end below -0.05): 8 storeys two stations -0.040 [-0.065, -0.010] (a real loss, excludes zero),
  8 storeys crew of 10 -0.030 [-0.070, 0.020], 5 storeys field-measured error +0.030 [-0.060, 0.100], and 8 storeys crew of 20 -0.015 [-0.050, 0.025], whose
  lower end equals -0.05 to rounding. 8 storeys field-measured error gains: +0.065 [0.015, 0.115].
- **H38: HOLDS.** 0 of 1420 devices above 2.5 % with the ledger (largest 2.19 %); without it 1360 of 1420 (largest 3.64 %). In 1800 s a device spent more than
  the 90 s its whole hour allows. COLLAPSE-1 to -8 did not use the ledger: do not quote them as legal.
- **H39: FAILS.** Ours minus 868 MHz alone, L2: 5 storeys -0.005 [-0.070, 0.060], 8 storeys -0.040 [-0.100, 0.020]. No difference is detected; non-inferiority
  by 0.05 is not shown. 868 MHz alone is as good as ours for a crew of 20 with four stations.
- **Coverage (reported).** Rises in every crew cell (+0.056 to +0.122, every lower bound above 0) but stays low: 0.40-0.81 against 0.95 nominal. Two stations are
  worst (0.40 and 0.46). The collapse filter is still over-confident.
- **N = 1 (reported).** 868 MHz alone 0.90 / 0.70, ours 0.70 / 0.90 (5 / 8 storeys); ten seeds move L2 in steps of 0.10.
- **Robustness check (the reason for the campaign).** The two-range and peer-fusion defects changed ours' L2 by -0.040 to +0.065. COLLAPSE-6's L2 levels
  survive except a small loss with two stations at 8 storeys.
