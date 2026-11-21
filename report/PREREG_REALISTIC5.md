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

## Result (run 2026-11-21 11:09–12:45 IST, source 6e7025ad2, 280/280 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic5_2026-11-21/REALISTIC5.md`. Traces are not tracked; their SHA-256 manifest is.

- **G1: HOLDS.** Ours − 2.4 GHz only, searcher within 10 m: ROAD1 −1128 s [−1519, −724], ROAD3 −1691 s [−2002, −1317],
  RAND3 −1896 s [−2646, −1064], ROAD3F4 −1766 s [−2058, −1416]; CLUS3 +44 s [38, 51] (within the 120 s margin).
- **G2: HOLDS.** Ours − the better single-radio design, time to 25 m: −1082 to −1909 s at the ROAD and RAND sites
  (every upper bound < −300 s), 0 at CLUS3.
- **G3: FAILS** at one site: 250 m availability, CLUS3 −0.005 [−0.028, 0.010] against 868 MHz only; elsewhere +0.087 to
  +0.259, every lower bound > 0.
- **G4: HOLDS.** 8 h 250 m availability, ours − the better single-radio design: ROAD1 −0.001 [−0.002, 0.000] (no design
  above 0.003 with one gateway and the crew 3–6 km away), ROAD3 +0.096 [0.030, 0.176], CLUS3 +0.164 [0.091, 0.234]
  (0.579 against 0.415), RAND3 +0.285 [0.168, 0.396].
- **G5 (reported).** J/h, emergency and quiet: hybrid designs 34–51, 868 MHz only 20–22, 2.4 GHz only 17–33, always-on
  912–945; TII-2 − ours −0.9 to +2.5 J/h in the quiet shift.
