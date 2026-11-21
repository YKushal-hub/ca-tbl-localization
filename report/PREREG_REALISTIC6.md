# REALISTIC-6 — pre-registration (committed before the first cell)

**Why.** A full-system audit after REALISTIC-5 found that the casualty stood at the identical point at the alarm on
every seed of every earlier realistic campaign (one incident under 20 noise draws; at the CLUS and RAND sites pinned to
the box edge), that no campaign tested a crew larger than five, and that the field-measured 2.4 GHz error was only run
before the later fixes (DESIGN_DECISIONS 35). REALISTIC-6 repeats the emergency study with a different incident on every
seed (`--incidentRandom`), adds crews of 10 and 15, and reruns the field error law with every fix on. REALISTIC-1 to -5
stand as registered.

**Binary.** Frozen at launch by `scripts/realistic6_run.sh` (SHA-256 checked before every cell); source = the commit
adding this file; `dirty 0` on every cell.

**Design.** Seeds 601–620 (never used). Every run: REALISTIC-5's flags (device-side estimation, mast-height correction,
the search party fixed, `--finderStaleSearch`, `--reacquireSigmaM=500`, the side of the road known on ROAD sites),
`--incidentRandom`, overdue alarm at 50 min, 2 h. Main cells: ROAD1, ROAD3, CLUS3, RAND3 (2 searchers), ROAD3F4 (4); the
five arms and the TII-2 rule. Reported cells (five arms): CREW10 and CREW15 (ROAD3 with 10 / 15 workers, relay slots
off), ROAD3MUL and CLUS3MUL (`--tofErrorModel=muller`). 280 runs. Measures and statistics:
`scripts/realistic6_analyse.py`, committed with this file.

## Predictions (fixed now)

- **H1.** In every main cell, a searcher reaches the worker under our system no later than under 2.4 GHz only by more
  than 2 min, and sooner by more than 5 min in at least one cell.
- **H2.** In every main cell, our time to 25 m is no worse than the better single-radio design's by more than 2 min.
- **H3.** In every main cell, our 250 m availability is no worse than the better single-radio design's by more than 0.02.
- **H4 (reported).** Crew 10 and 15, and the field-measured error: ours − 2.4 GHz only on searcher arrival, time to
  25 m and availability, and energy per worker-hour.

**Abort / completeness.** Any rc ≠ 0, not `dirty 0`, a manifest mismatch, DD35 not announced, an alarm not at 3000 s, or
fewer than 20 seeds in a cell → incomplete; nothing is evaluated until complete. No cell dropped or chosen.

**Reporting.** `REALISTIC6.md` by the analyser; a Result section is appended here after the run, whatever it says.
