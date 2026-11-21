# REALISTIC-5 — pre-registration (committed before the first cell)

**Why.** REALISTIC-2's quiet shift (Q1 failed at the cluster site) and its traces showed crew filters that were lost and
confident (DESIGN_DECISIONS 33-34). DD 34 (`--reacquireSigmaM`) re-enters particle acquisition when a filter is lost; a
three-seed pilot (not citable) raised ours' 8 h rough-position availability at the cluster site from 0.09-0.39 to
0.54-0.69. REALISTIC-5 tests the whole realistic study again with it, on new seeds, for every design. REALISTIC-1 to -4
stand as registered.

**Binary.** Frozen at launch by `scripts/realistic5_run.sh` (SHA-256 checked before every cell); source = the commit
adding this file; `dirty 0` on every cell.

**Design.** Every run: REALISTIC-4's flags (device-side estimation, mast-height correction, the search party fixed,
`--finderStaleSearch`, the side of the road known on ROAD sites) and `--reacquireSigmaM=500`.
Part E (emergency, overdue alarm at 50 min, 2 h): ROAD1, ROAD3, CLUS3, RAND3 (2 searchers) and ROAD3F4 (4), seeds
561–580. Part Q (8 h, no incident, no searchers): ROAD1, ROAD3, CLUS3, RAND3, seeds 581–590. Designs: the five arms and
the TII-2 rule. 280 runs. Measures and statistics: `scripts/realistic5_analyse.py`, committed with this file.

## Predictions (fixed now)

- **G1.** In every E cell, a searcher reaches the worker under our system no later than under 2.4 GHz only by more than
  2 min, and sooner by more than 5 min in at least one cell.
- **G2.** In every E cell, our time to 25 m is no worse than the better single-radio design's by more than 2 min.
- **G3.** In every E cell, our 250 m availability is no worse than the better single-radio design's by more than 0.02.
- **G4.** At every quiet-shift site, our 8 h 250 m availability is no worse than the better single-radio design's by
  more than 0.02.
- **G5 (reported).** Energy per worker-hour of every design; TII-2 minus ours.

**Abort / completeness.** Any rc ≠ 0, not `dirty 0`, a manifest mismatch, DD34 not announced, an alarm not at 3000 s, or
fewer than the registered seeds in a cell → incomplete; nothing is evaluated until complete. No cell dropped or chosen.

**Reporting.** `REALISTIC5.md` by the analyser; a Result section is appended here after the run, whatever it says.
