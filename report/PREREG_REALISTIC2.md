# REALISTIC-2 — pre-registration (committed before the first cell, and before REALISTIC-1's result was seen)

**Question.** Do REALISTIC-1's comparisons hold on station layouts and crews it did not test, and what does each design
cost over a quiet shift with no emergency? Plan: `~/catbl_work/tii_paper/PLAN_REALISTIC.md`, Stage 2 and quiet shift.

**Binary.** Frozen at launch by `scripts/realistic2_run.sh` (SHA-256 checked before every cell); source = the commit
adding this file; `dirty 0` on every cell. Every run: device-side estimation and mast-height correction (DESIGN_DECISIONS
28), exactly as REALISTIC-1.

**Part E (emergency).** REALISTIC-1's timeline unchanged (leaves the route at 30 min, stops 10 min later, overdue alarm at
50 min, run ends at 2 h). Seeds **521–540** (never used). Designs: the five arms and the TII-2 rule, as REALISTIC-1.

| cell | stations / crew |
|---|---|
| RAND3 | 3 stations at random spots (`--gwLayout=file`, layout k = seed mod 10 of `scripts/layouts/realistic/rand_n3_k*.csv`), 2 searchers |
| RAND5 | 5 stations, the same with `rand_n5_k*.csv`, 2 searchers |
| ROAD5 | 5 stations along the access road, 2 searchers |
| ROAD3F4 | REALISTIC-1's ROAD3 with **4** searchers |

**Part Q (quiet shift).** 8 h, no incident, no searchers, seeds **541–550**; sites ROAD1, ROAD3, CLUS3 (as REALISTIC-1)
and RAND3 (layout k = seed mod 10); the five arms and TII-2.

320 runs. Measures and statistics: `scripts/realistic2_analyse.py`, committed with this file (REALISTIC-1's measures and
paired 95 % bootstrap; "never" counted as 4200 s).

## Predictions (fixed now)

- **E1 (replication of REALISTIC-1 H1).** In every Part-E cell, our system's time to 25 m is no worse than the better
  single-radio design by more than 120 s (paired upper bound < 120 s).
- **E2 (replication of H3).** In every Part-E cell, our system's 250 m availability is no worse than the better
  single-radio design's by more than 0.02 (paired lower bound ≥ −0.02).
- **E3 (searchers).** ROAD3F4: a searcher reaches the worker no later under our system than under 2.4 GHz only by more
  than 120 s (paired upper bound < 120 s).
- **Q1.** On every quiet site, our system's 250 m availability is no worse than the better single-radio design's by more
  than 0.02, and its battery per hour is reported against every design with its interval (no threshold).
- **Q2 (reported).** TII-2 minus our system, J/h, per quiet site.

**Abort / completeness.** Any rc ≠ 0, not `dirty 0`, a manifest mismatch, an emergency alarm not at 3000 s, or fewer
than the registered seeds in a cell → incomplete; nothing is evaluated until complete. No cell dropped or chosen.

**Not included, and why.** The device-silent alarm (`--searchLaunchOnLoss`) makes the alarm time depend on the design
(a design that keeps contact delays its own alarm), so it does not give a like-for-like comparison; it is left out.
A long thin corridor site needs a scenario change not yet built; ROAD5 (stations every 2 km on the line) stands in.

**Reporting.** `REALISTIC2.md` by the analyser; a Result section is appended here after the run, whatever it says.

**Amendment before any cell (2026-11-20, 23:50 IST).** The binary is built from the commit that adds DESIGN_DECISIONS 29
(`--acquireSideOfRoad`, default off) and REALISTIC-3, one commit after this file. The flag is not passed by this
campaign; its off state is byte-identical to the build before it (DESIGN_DECISIONS 29), so nothing registered here
changes. Order of running: REALISTIC-1, then REALISTIC-3, then this campaign.

## Result (run 2026-11-21 01:32–07:37 IST, alongside REALISTIC-1/3, source eaee45b4b, 240/240 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic2_2026-11-21/REALISTIC2.md`. Traces are not tracked; their SHA-256 manifest is. The
runner was restarted once to raise its parallelism (three cells in flight were rerun from scratch on the same frozen
binary); its RUN.md resume line reads `dirty 1` because REALISTIC-3's result text was uncommitted in the checkout at
the time; every cell's own `run_id.txt` reads `dirty 0`.

**Caveat found after registration, before this result was read.** Every Part-E cell has searchers, and in every
2.4 GHz-only run the search party could not work (DESIGN_DECISIONS 30-32). Searchers also act as moving anchors, so
E1-E3's 2.4 GHz-only side is affected, not only E3. REALISTIC-4 repeats RAND3 and ROAD3F4 with the party fixed.

- **E1: HOLDS.** Ours − the better single-radio design, time to 25 m: RAND3 −2761 s [−3526, −1893], RAND5 −3386 s
  [−3851, −2810], ROAD5 −1057 s [−1837, −367], ROAD3F4 −2169 s [−3192, −1099] (subject to the caveat).
- **E2: HOLDS.** 250 m availability: +0.138 to +0.323, every lower bound > 0 (subject to the caveat).
- **E3: HOLDS** as computed: −1910 s [−2124, −1592]; not evidence (the caveat), see REALISTIC-4.
- **Q1: FAILS.** At the open cluster site, ours' 250 m availability over 8 h is 0.219 against 2.4 GHz only's 0.549
  (−0.330 [−0.411, −0.246]) at 46.2 against 15.8 J/h. Elsewhere ours is at least level: ROAD1 +0.013 [−0.001, 0.035],
  ROAD3 +0.033 [0.000, 0.070], RAND3 +0.275 [0.134, 0.420] (against 868 MHz only).
- **Q2 (reported).** TII-2 − ours, J/h: ROAD1 +1.2, ROAD3 +4.1, CLUS3 −0.5, RAND3 +5.6. Energy per worker-hour over the
  quiet shift: hybrid designs 43–50 J, 868 MHz only 20–21 J, 2.4 GHz only 16–31 J, always-on 917–941 J.
