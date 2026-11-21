# REALISTIC-4 — pre-registration (committed before the first cell)

**Why.** REALISTIC-1's H2 and REALISTIC-3's S3 (searchers reach the casualty sooner under our system than under 2.4 GHz
only) rest on runs in which the 2.4 GHz-only search party could not work: it polled and steered on its 868 MHz radio,
jumped 3.2 km at dispatch, and swept round the camp instead of going to the last known position (DESIGN_DECISIONS
30-32, found on traces after those results). Those results stand as registered and are reported with this defect
stated. REALISTIC-4 repeats every searcher cell with the party fixed, for every design.

**Binary.** Frozen at launch by `scripts/realistic4_run.sh` (SHA-256 checked before every cell); source = the commit
adding this file; `dirty 0` on every cell.

**Design.** Every run: device-side estimation, mast-height correction, overdue alarm at 50 min, 2 h, two searchers
(ROAD3F4: four), `--finderSweepFromDispatch --finderDigestOnArmTier --finderLkpFirst --finderStaleSearch`; the ROAD
sites with `--acquireSideOfRoad`. Sites ROAD1, ROAD3, CLUS3 on seeds 501–520 (pairing with REALISTIC-1/3), RAND3 and
ROAD3F4 on seeds 521–540 (pairing with REALISTIC-2). Designs: the five arms and the TII-2 rule. 200 runs. Measures and
statistics: `scripts/realistic4_analyse.py`, committed with this file.

## Predictions (fixed now)

- **F1 (H2, party fixed).** In every cell, a searcher reaches the worker under our system no later than under 2.4 GHz
  only by more than 2 min (paired upper bound < 120 s), and sooner by more than 5 min in at least one cell.
- **F2 (H1).** In every cell, our system's time to 25 m is no worse than the better single-radio design's by more than
  2 min.
- **F3 (H3).** In every cell, our system's 250 m availability is no worse than the better single-radio design's by more
  than 0.02.
- **F4 (reported).** For every design, searcher arrival with minus without the fixes (same seeds).

Pilot evidence that motivated the fixes (not citable): on seeds 501–502 the fixed 2.4 GHz-only party reached the
casualty at the cluster site as fast as ours and on ROAD3 later or not at all.

**Abort / completeness.** Any rc ≠ 0, not `dirty 0`, a manifest mismatch, a fix not announced, an alarm not at
3000 s, or fewer than 20 seeds in a cell → incomplete; nothing is evaluated until complete. No cell dropped or chosen.

**Reporting.** `REALISTIC4.md` by the analyser; a Result section is appended here after the run, whatever it says.

## Result (run 2026-11-21 06:45–08:13 IST, source fcfae4466, 200/200 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic4_2026-11-21/REALISTIC4.md`. Traces are not tracked; their SHA-256 manifest is.

- **F1: FAILS** at one site. Ours − 2.4 GHz only, searcher within 10 m: CLUS3 +119 s [40, 260] (818 against 699 s;
  the casualty is 0.8 km from the office, inside 2.4 GHz reach in open terrain). Everywhere else ours is clearly
  sooner: ROAD1 −983 s [−1446, −508], ROAD3 −901 s [−1383, −398], RAND3 −2173 s [−2804, −1505], ROAD3F4 −1269 s
  [−1686, −826]. With the party fixed, 2.4 GHz-only searchers reached the casualty in 60 % (ROAD1), 45 % (ROAD3),
  15 % (RAND3), 35 % (ROAD3F4) and 100 % (CLUS3) of runs; ours in 95 %, 85 %, 75 %, 95 % and 100 %.
- **F2: HOLDS.** Ours − the better single-radio design, time to 25 m: ROAD1 −1195 s [−2060, −390], ROAD3 −1079 s
  [−1854, −396], CLUS3 0, RAND3 −1926 s [−2992, −716], ROAD3F4 −757 s [−1606, 103].
- **F3: FAILS** at one site: 250 m availability, CLUS3 +0.007 [−0.030, 0.047]; elsewhere +0.121 to +0.303, every
  lower bound > 0.
- **F4 (reported).** Fixing the party moved 2.4 GHz only's searcher arrival by −328 to −3501 s and the hybrid designs'
  by −628 to +181 s (paired with REALISTIC-1/2/3, same seeds).
