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

## Result (run 2026-11-21 14:01–16:16 IST, source b0b9cbbd1, 280/280 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic6_2026-11-21/REALISTIC6.md`. Traces are not tracked; their SHA-256 manifest is. Every run
drew its own incident (DD35 line in every log).

- **H1: HOLDS.** Ours − 2.4 GHz only, searcher within 10 m: ROAD3 −1488 s [−2034, −892], RAND3 −1471 s [−2103, −699],
  ROAD3F4 −1725 s [−2171, −1255]; ROAD1 −584 s [−1173, 72]; CLUS3 +8 s [−1, 25].
- **H2: HOLDS.** Ours − the better single-radio design, time to 25 m: ROAD1 −862 s [−1570, −198], ROAD3 −1312 s
  [−2211, −418], RAND3 −2472 s [−3329, −1473], ROAD3F4 −2355 s [−3326, −1274], CLUS3 0.
- **H3: FAILS** at one site: 250 m availability, CLUS3 −0.017 [−0.039, −0.001] against 2.4 GHz only; elsewhere +0.094 to
  +0.272, every lower bound > 0.
- **H4 (reported).** Crew of 10 / 15 (ROAD3, relay slots off), ours − 2.4 GHz only: searcher −825 s [−1517, −69] /
  −1470 s [−1922, −1017]; time to 25 m −1364 s / −1901 s; A250 +0.045 / +0.047; J/h ours 31.9 / 31.7, 2.4 GHz only 13.3 /
  11.6. Field-measured 2.4 GHz error: ROAD3 searcher −522 s [−1038, −29], time to 25 m −104 s [−844, 632], A250 +0.009
  [−0.017, 0.032]; CLUS3 searcher 0, time to 25 m −82 s [−330, 83], A250 −0.025 [−0.054, −0.001]. Under that error law
  every design slows: ours reached 25 m in 60 % (ROAD3) and 25 % (CLUS3) of runs.
