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
