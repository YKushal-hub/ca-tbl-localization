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

## Result (run 2026-11-21 01:32–02:58 IST, source eaee45b4b, 180/180 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic3_2026-11-21/REALISTIC3.md`. Traces are not tracked; their SHA-256 manifest is.
Times are from the alarm; "never" (in brackets, share of runs) is counted as 4200 s.

- **S1: FAILS** (registered as both designs). Our system: time to 25 m with minus without the flag, ROAD3 with no
  searchers, −2212 s [−3192, −1232] (2422 → 210 s). 2.4 GHz only: −1039 s [−2362, 274], an interval that includes 0.
  The flag shortened time to 25 m for every hybrid design in every ROAD3 cell (−368 to −2668 s) and in ROAD1 with 2
  searchers (−290 to −569 s).
- **S2: FAILS** on non-inferiority in one cell: ROAD3 with no searchers, ours − 2.4 GHz only −259 s [−930, 411] (upper
  bound above the 120 s margin; the mean favours ours). Elsewhere ours is non-inferior and clearly better: ROAD1 with
  2 searchers −2560 s [−3363, −1696] against 2.4 GHz only, ROAD3 with 2 searchers −1643 s [−2527, −788]; and each
  single-radio design is worse by more than 300 s somewhere (868 MHz only never reaches 25 m in any cell).
- **S3: HOLDS.** A searcher reaches the worker sooner under our system than under 2.4 GHz only: ROAD1 −1487 s
  [−1912, −1062], ROAD3 −1888 s [−2111, −1572]. Under 2.4 GHz only no searcher reached the worker in any run.
- **S4: HOLDS.** Ours − the better single-radio design, 250 m availability: +0.058 to +0.195, every lower bound > 0.

Also observed (not registered): with one gateway and no searchers no design finds the worker (ours 2 of 20 runs within
25 m); under the field-measured 2.4 GHz error (ROAD3, 2 searchers) times rise for every design (ours 2918 s to 25 m,
searcher reach in 3 of 20 runs; 2.4 GHz only 3 of 20 within 25 m, searcher reach in none).
