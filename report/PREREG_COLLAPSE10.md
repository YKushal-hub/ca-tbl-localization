# COLLAPSE-10 — pre-registration (committed before the first cell)

**Why.** In collapse the 868 MHz time-difference fix has a constant sub-metre error per device (DD 62: 0.79 m and 0.45 m median, noiseless), fused as if independent, and its stamps had no rubble delay (DD 63). Every earlier collapse
result therefore had an optimistic 868 MHz world, and searchers' devices above the cap (DD 60).

**Design.** COLLAPSE-9's cells, paired: `off` = DD 54, 56, 57, the searchers' ledger (DD 60) and the rubble delay in the stamps (DD 63); `on` = the same plus `--tdoaHeightCorrect` (DD 62).
Seeds 1601-1610, never used; 200 runs; frozen binary, SHA-256 before every cell, `dirty 0`. `scripts/collapse10_run.sh`, `scripts/collapse10_analyse.py`, committed with this file.

**Seen before registering.** COLLAPSE-9 in full; development seeds 731-733 (DD 62, DD 63 in `docs/DESIGN_DECISIONS.md`): with DD 63, coverage off to on 0.734 to 0.787 (5 storeys) and 0.858 to 0.868 (8);
on seed 731 alone at 5 storeys it fell, 0.778 to 0.757. Two stations: no time-difference fix, so no effect is possible there. DD 59 is off (no effect in collapse on 731-733).

## Hypotheses (fixed now; paired 95 % bootstrap over seeds)

- **H40:** every crew cell: the lower end of ours' L2, on minus off, is at least -0.05.
- **H41:** off and on: every worker's and every searcher's device's audited 868 MHz airtime is at most 2.5 %.
- **H42:** on, four stations, 20 workers: ours minus the best single radio, lower end at least -0.05, at both storey counts.
- **H43:** four stations, 20 workers: coverage on minus off, lower end at least -0.02 at both storey counts and above 0 at one.
- Coverage against 0.95: reported, no prediction. It is not expected to reach 0.95.

**Abort / completeness.** As COLLAPSE-9, with the announcement lines of this design. No cell dropped. Nothing is adopted as a default by this campaign. A Result section is appended after the run.
