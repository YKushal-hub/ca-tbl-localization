# REALISTIC-3 — pre-registration (committed before the first cell)

**Why.** In REALISTIC-1's ROAD sites every gateway stands on one line, and a range-only position cannot be told from its
mirror image across that line: filters of every design locked onto the mirror (~800 m off) on several seeds, seen while
REALISTIC-1 was running and before its result. DESIGN_DECISIONS 29 adds `--acquireSideOfRoad` (the operator knows which
side of the access road the work area is on). REALISTIC-3 repeats REALISTIC-1's ROAD runs with it. REALISTIC-1 and -2
stand as registered; this does not revise them.

**Binary.** Frozen at launch by `scripts/realistic3_run.sh` (SHA-256 checked before every cell); source = the commit
adding this file; `dirty 0` on every cell.

**Design.** REALISTIC-1's ROAD cells exactly (same seeds 501–520, same flags, device-side estimation, mast-height
correction, overdue alarm at 50 min, 2 h runs) plus `--acquireSideOfRoad=true`: ROAD1 and ROAD3, 2 or no searchers, the
five arms and the TII-2 rule, and ROAD3 with 2 searchers under the field-measured 2.4 GHz error. 180 runs. Measures and
statistics: `scripts/realistic3_analyse.py`, committed with this file (REALISTIC-1's).

## Predictions (fixed now)

- **S1 (the fix works).** ROAD3 with no searchers: for our system and for 2.4 GHz only, time to 25 m with the flag minus
  REALISTIC-1's without it (same seeds) has a paired upper bound below 0. Every design and cell's difference is reported.
- **S2 (= H1).** In every cell, our system's time to 25 m is no worse than the better single-radio design's by more than
  2 min, and each single-radio design is worse than ours by more than 5 min in at least one cell.
- **S3 (= H2).** With 2 searchers, at each site a searcher reaches the worker no later under our system than under 2.4
  GHz only by more than 2 min, and sooner by more than 5 min at at least one site.
- **S4 (= H3).** In every cell, our system's 250 m availability is no worse than the better single-radio design's by more
  than 0.02.

**Abort / completeness.** Any rc ≠ 0, not `dirty 0`, a manifest mismatch, the flag not announced, an alarm not at
3000 s, or fewer than 20 seeds in a cell → incomplete; nothing is evaluated until complete. No cell dropped or chosen.

**Reporting.** `REALISTIC3.md` by the analyser; a Result section is appended here after the run, whatever it says.
